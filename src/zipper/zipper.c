#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include "../zip.h"
#include "zipper_file.h"
#include "queue.h"
#include "../compression/compression.h"
#include "../wrapper_functions.h"
#include "../utils.h"

typedef struct {
    LPWSTR zip_name;
	HANDLE hZip;

	uint64_t num_records;

	queue* file_queue;
} zipper_context;


/* Zip Structs Functions */

static void create_local_file_header(const zipper_file* zf, local_file_header* out_lfh) {
	out_lfh->signature = LOCAL_FILE_HEADER_SIGNATURE;
	out_lfh->version = ZIP_VERSION;
	out_lfh->flags = 0x0000;
	out_lfh->compression = zf->compression_method;
	out_lfh->mod_time = zf->mod_time;
	out_lfh->mod_date = zf->mod_date;
	out_lfh->crc32 = zf->crc32;
	out_lfh->compressed_size = zf->uncompressed_size >= 0xFFFFFFFF ? 0xFFFFFFFF : zf->compressed_size;
	out_lfh->uncompressed_size = MIN(zf->uncompressed_size, 0xFFFFFFFF);
	out_lfh->file_name_length = zf->utf8_name_length;
	out_lfh->extra_field_length = zf->zip64_extra_field_length;
}

static void create_central_directory_header(const zipper_file* zf, central_directory_header* out_cdh) {
	out_cdh->signature = CENTRAL_DIRECTORY_HEADER_SIGNATURE;
	out_cdh->version_made_by = (WINDOWS_NTFS << 8) | ZIP_VERSION;
	out_cdh->version_needed_to_extract = ZIP_VERSION;
	out_cdh->flags = UTF8_ENCODING;
	out_cdh->compression = zf->compression_method;
	out_cdh->mod_time = zf->mod_time;
	out_cdh->mod_date = zf->mod_date;
	out_cdh->crc32 = zf->crc32;
	out_cdh->compressed_size = zf->uncompressed_size >= 0xFFFFFFFF ? 0xFFFFFFFF : zf->compressed_size;
	out_cdh->uncompressed_size = MIN(zf->uncompressed_size, 0xFFFFFFFF);
	out_cdh->file_name_length = zf->utf8_name_length;
	out_cdh->extra_field_length = zf->zip64_extra_field_length;
	out_cdh->file_comment_length = 0;
	out_cdh->disk_number_start = 0;
	out_cdh->internal_file_attributes = 0;
	out_cdh->external_file_attributes = zf->windows_file_attributes;
	out_cdh->local_header_offset = MIN(zf->local_header_offset, 0xFFFFFFFF);
}

static void create_end_of_central_directory_record(end_of_central_directory_record* out_eoccr,
			uint64_t num_records, uint64_t central_directory_size, uint64_t central_directory_start_offset) {
	out_eoccr->signature = END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE;
	out_eoccr->disk_number = 0;
	out_eoccr->central_directory_start_disk_number = 0;
	out_eoccr->num_records_on_disk = MIN(num_records, 0xFFFF);
	out_eoccr->total_num_records = MIN(num_records, 0xFFFF);
	out_eoccr->central_directory_size = MIN(central_directory_size, 0xFFFFFFFF);
	out_eoccr->central_directory_start_offset = MIN(central_directory_start_offset, 0xFFFFFFFF);
	out_eoccr->comment_length = 0;
}

static void create_zip64_extra_field(const zipper_file* zf, zip64_extra_field* out_z64ef) {
	unsigned char num_extra_fields = 0;

	out_z64ef->header_id = ZIP64_EXTRA_FIELD_HEADER_ID;

	if(zf->uncompressed_size >= 0xFFFFFFFF) {
		out_z64ef->extra_fields[num_extra_fields++] = zf->uncompressed_size;
		out_z64ef->extra_fields[num_extra_fields++] = zf->compressed_size;
	}

	if(zf->local_header_offset >= 0xFFFFFFFF)
		out_z64ef->extra_fields[num_extra_fields++] = zf->local_header_offset;

	out_z64ef->data_size = sizeof(uint64_t) * num_extra_fields;
}

static void create_zip64_end_of_central_directory_record(zip64_end_of_central_directory_record* out_z64eoccr,
			uint64_t num_records, uint64_t central_directory_size, uint64_t central_directory_start_offset) {
	out_z64eoccr->signature = ZIP64_END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE;
	out_z64eoccr->size_of_remaining_zip64_end_of_central_directory_record = ZIP64_END_OF_CENTRAL_DIRECTORY_RECORD_REMAINING_FIXED_FIELDS_SIZE;
	out_z64eoccr->version_made_by = (WINDOWS_NTFS << 8) | ZIP_VERSION;
	out_z64eoccr->version_needed_to_extract = ZIP_VERSION;
	out_z64eoccr->disk_number = 0;
	out_z64eoccr->central_directory_start_disk_number = 0;
	out_z64eoccr->num_records_on_disk = num_records;
	out_z64eoccr->total_num_records = num_records;
	out_z64eoccr->central_directory_size = central_directory_size;
	out_z64eoccr->central_directory_start_offset = central_directory_start_offset;
}

static void create_zip64_end_of_central_directory_locator(zip64_end_of_central_directory_locator* out_z64eoccl,
			uint64_t zip64_end_of_central_directory_start_offset) {
	out_z64eoccl->signature = ZIP64_END_OF_CENTRAL_DIRECTORY_LOCATOR_SIGNATURE;
	out_z64eoccl->zip64_end_of_central_directory_record_disk_number = 0;
	out_z64eoccl->zip64_end_of_central_directory_record_offset = zip64_end_of_central_directory_start_offset;
	out_z64eoccl->total_num_disks = 1;
}


/* Main Functions */

static void write_file_to_zip(zipper_context* zc, zipper_file* zf) {
	printf("Writing %ls to zip\n", zf->utf16_name);

	queue_enqueue(zc->file_queue, zf);

	zf->local_header_offset = _GetFilePointerEx(zc->hZip);

	// Calculate the zip64 extra field's length if applicable
	if(zf->uncompressed_size >= 0xFFFFFFFF || zf->local_header_offset >= 0xFFFFFFFF)
		zf->zip64_extra_field_length = ZIP64_EXTRA_FIELD_FIXED_FIELDS_SIZE + sizeof(uint64_t) * (2 * (zf->uncompressed_size >= 0xFFFFFFFF) + (zf->local_header_offset >= 0xFFFFFFFF));

	// Write the file's compressed data if it's not empty
	if(zf->uncompressed_size > 0) {
		uint64_t header_size = sizeof(local_file_header) + zf->utf8_name_length + zf->zip64_extra_field_length;
		zfile_compress_and_write(zf, zc->zip_name, zf->local_header_offset + header_size);
	}

	// Write the header
	local_file_header lfh;
	create_local_file_header(zf, &lfh);
	_WriteFile(zc->hZip, &lfh, sizeof(local_file_header), NULL, NULL);
	_WriteFile(zc->hZip, zf->utf8_name, zf->utf8_name_length, NULL, NULL);

	// Write the zip64 extra field if necessary
	if(zf->zip64_extra_field_length > 0) {
		zip64_extra_field z64ef;
		create_zip64_extra_field(zf, &z64ef);
		_WriteFile(zc->hZip, &z64ef, zf->zip64_extra_field_length, NULL, NULL);
	}

	if(zf->uncompressed_size > 0) 
		// Advance the file pointer back to the end of the file's data
		_SetFilePointerEx(zc->hZip, (LARGE_INTEGER){.QuadPart = zf->compressed_size}, NULL, FILE_CURRENT);
	
	// Write any children if any
	if(zf->num_children > 0)
		for(unsigned i = 0; i < zf->num_children; i++)
			write_file_to_zip(zc, zf->children[i]);

	zc->num_records++;
}

static void write_end_of_central_directory_to_zip(zipper_context* zc, uint64_t central_directory_size, uint64_t central_directory_start_offset) {
	// Write a zip64 end of central directory record (and locator) if necessary
	if(zc->num_records > 0xFFFF || central_directory_size > 0xFFFFFFFF || central_directory_start_offset > 0xFFFFFFFF) {
		uint64_t zip64_end_of_central_directory_start_offset = central_directory_start_offset + central_directory_size;
		zip64_end_of_central_directory_record z64eoccr;
		create_zip64_end_of_central_directory_record(&z64eoccr, zc->num_records, central_directory_size, central_directory_start_offset);
		zip64_end_of_central_directory_locator z64eoccl;
		create_zip64_end_of_central_directory_locator(&z64eoccl, zip64_end_of_central_directory_start_offset);

		_WriteFile(zc->hZip, &z64eoccr, sizeof(zip64_end_of_central_directory_record), NULL, NULL);
		_WriteFile(zc->hZip, &z64eoccl, sizeof(zip64_end_of_central_directory_locator), NULL, NULL);
	}

	end_of_central_directory_record eoccr;
	create_end_of_central_directory_record(&eoccr, zc->num_records, central_directory_size, central_directory_start_offset);
	_WriteFile(zc->hZip, &eoccr, sizeof(end_of_central_directory_record), NULL, NULL);
}

static void write_central_directory_to_zip(zipper_context* zc) {
	uint64_t central_directory_start_offset = _GetFilePointerEx(zc->hZip);

	while(zc->file_queue->size > 0) {
		zipper_file* zf = queue_dequeue(zc->file_queue);

		// Get the central directory header and write it to the zip
		central_directory_header cdh;
		create_central_directory_header(zf, &cdh);
		_WriteFile(zc->hZip, &cdh, sizeof(central_directory_header), NULL, NULL);
		_WriteFile(zc->hZip, zf->utf8_name, zf->utf8_name_length, NULL, NULL);
		
		// Write the zip64 extra field if necessary
		if(zf->zip64_extra_field_length > 0) {
			zip64_extra_field z64ef;
			create_zip64_extra_field(zf, &z64ef);
			_WriteFile(zc->hZip, &z64ef, zf->zip64_extra_field_length, NULL, NULL);
		}
		
		zfile_destroy(zf);
	}

	uint64_t central_directory_size = _GetFilePointerEx(zc->hZip) - central_directory_start_offset;

	write_end_of_central_directory_to_zip(zc, central_directory_size, central_directory_start_offset);
}

int main() {
	int argc;
	LPWSTR* argv = _CommandLineToArgvW(GetCommandLineW(), &argc);

	if(argc < 2) {
		printf("Usage: zipper archive_name file_to_add_1 ... file_to_add_n\n");
		return 0;
	}

	zipper_context zc = {0};

	zc.zip_name = argv[1];
	zc.hZip = _CreateFileW(zc.zip_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	zc.file_queue = queue_create();

	for(int i = 2; i < argc; i++) {
		zipper_file* zf = zfile_create(argv[i], NO_COMPRESSION);
		write_file_to_zip(&zc, zf);
	}

	printf("Writing central directory to zip\n");

	write_central_directory_to_zip(&zc);

	printf("Done\n");

	Free(zc.file_queue);
	_CloseHandle(zc.hZip);
	return 0;
}
