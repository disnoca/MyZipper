#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../zip.h"
#include "zipper_file.h"
#include "queue.h"
#include "../compression/compression.h"
#include "../wrapper_functions.h"
#include "../macros.h"

typedef struct {
    LPWSTR zip_name;
	HANDLE hZip;

	uint64_t num_records;

	queue* file_queue;
} zipper_context;


/* Zip Structs Functions */

static local_file_header get_file_header(zipper_file* zf) {
	local_file_header lfh;

	lfh.signature = LOCAL_FILE_HEADER_SIGNATURE;
	lfh.version = ZIP_VERSION;
	lfh.flags = 0x0000;
	lfh.compression = zf->compression_method;
	lfh.mod_time = zf->mod_time;
	lfh.mod_date = zf->mod_date;
	lfh.crc32 = zf->crc32;
	lfh.compressed_size = zf->uncompressed_size >= 0xFFFFFFFF ? 0xFFFFFFFF : zf->compressed_size;
	lfh.uncompressed_size = MIN(zf->uncompressed_size, 0xFFFFFFFF);
	lfh.file_name_length = zf->utf8_name_length;
	lfh.extra_field_length = zf->zip64_extra_field_length;

	return lfh;
}

static central_directory_header get_central_directory_header(zipper_file* zf) {
	central_directory_header cdh;

	cdh.signature = CENTRAL_DIRECTORY_HEADER_SIGNATURE;
	cdh.version_made_by = (WINDOWS_NTFS << 8) | ZIP_VERSION;
	cdh.version_needed_to_extract = ZIP_VERSION;
	cdh.flags = UTF8_ENCODING;
	cdh.compression = zf->compression_method;
	cdh.mod_time = zf->mod_time;
	cdh.mod_date = zf->mod_date;
	cdh.crc32 = zf->crc32;
	cdh.compressed_size = zf->uncompressed_size >= 0xFFFFFFFF ? 0xFFFFFFFF : zf->compressed_size;
	cdh.uncompressed_size = MIN(zf->uncompressed_size, 0xFFFFFFFF);
	cdh.file_name_length = zf->utf8_name_length;
	cdh.extra_field_length = zf->zip64_extra_field_length;
	cdh.file_comment_length = 0;
	cdh.disk_number_start = 0;
	cdh.internal_file_attributes = 0;
	cdh.external_file_attributes = zf->windows_file_attributes;
	cdh.local_header_offset = MIN(zf->local_header_offset, 0xFFFFFFFF);

	return cdh;
}

static end_of_central_directory_record get_end_of_central_directory_record(uint64_t num_records, uint64_t central_directory_size, uint64_t central_directory_start_offset) {
	end_of_central_directory_record eoccr;

	eoccr.signature = END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE;
	eoccr.disk_number = 0;
	eoccr.central_directory_start_disk_number = 0;
	eoccr.num_records_on_disk = MIN(num_records, 0xFFFF);
	eoccr.total_num_records = MIN(num_records, 0xFFFF);
	eoccr.central_directory_size = MIN(central_directory_size, 0xFFFFFFFF);
	eoccr.central_directory_start_offset = MIN(central_directory_start_offset, 0xFFFFFFFF);
	eoccr.comment_length = 0;

	return eoccr;
}

static zip64_extra_field get_zip64_extra_field(zipper_file* zf) {
	zip64_extra_field z64ef;
	unsigned char num_extra_fields = 0;

	z64ef.header_id = ZIP64_EXTRA_FIELD_HEADER_ID;

	if(zf->uncompressed_size >= 0xFFFFFFFF) {
		z64ef.extra_fields[num_extra_fields++] = zf->uncompressed_size;
		z64ef.extra_fields[num_extra_fields++] = zf->compressed_size;
	}

	if(zf->local_header_offset >= 0xFFFFFFFF)
		z64ef.extra_fields[num_extra_fields++] = zf->local_header_offset;

	z64ef.data_size = sizeof(uint64_t) * num_extra_fields;

	return z64ef;
}

static zip64_end_of_central_directory_record get_zip64_end_of_central_directory_record(uint64_t num_records, uint64_t central_directory_size, uint64_t central_directory_start_offset) {
	zip64_end_of_central_directory_record z64eoccr;

	z64eoccr.signature = ZIP64_END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE;
	z64eoccr.size_of_remaining_zip64_end_of_central_directory_record = ZIP64_END_OF_CENTRAL_DIRECTORY_RECORD_REMAINING_FIXED_FIELDS_SIZE;
	z64eoccr.version_made_by = (WINDOWS_NTFS << 8) | ZIP_VERSION;
	z64eoccr.version_needed_to_extract = ZIP_VERSION;
	z64eoccr.disk_number = 0;
	z64eoccr.central_directory_start_disk_number = 0;
	z64eoccr.num_records_on_disk = num_records;
	z64eoccr.total_num_records = num_records;
	z64eoccr.central_directory_size = central_directory_size;
	z64eoccr.central_directory_start_offset = central_directory_start_offset;

	return z64eoccr;
}

static zip64_end_of_central_directory_locator get_zip64_end_of_central_directory_locator(uint64_t zip64_end_of_central_directory_start_offset) {
	zip64_end_of_central_directory_locator z64eoccl;

	z64eoccl.signature = ZIP64_END_OF_CENTRAL_DIRECTORY_LOCATOR_SIGNATURE;
	z64eoccl.zip64_end_of_central_directory_record_disk_number = 0;
	z64eoccl.zip64_end_of_central_directory_record_offset = zip64_end_of_central_directory_start_offset;
	z64eoccl.total_num_disks = 1;

	return z64eoccl;
}


/* Main Functions */

static void write_file_to_zip(zipper_context* zc, zipper_file* zf) {
	/*
	 * Add the directory's children to the zip
	 * This will recursively add all the files containted in the directory
	 * Directories are not written to the zip unless they're empty
	*/
	if(zf->is_directory && zf->num_children > 0) {
		for(unsigned i = 0; i < zf->num_children; i++)
			write_file_to_zip(zc, zf->children[i]);

		zfile_destroy(zf);
		return;
	}

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
	local_file_header lfh = get_file_header(zf);
	_WriteFile(zc->hZip, &lfh, sizeof(local_file_header), NULL, NULL);
	_WriteFile(zc->hZip, zf->utf8_name, zf->utf8_name_length, NULL, NULL);

	// Write the zip64 extra field if necessary
	if(zf->zip64_extra_field_length > 0) {
		zip64_extra_field z64ef = get_zip64_extra_field(zf);
		_WriteFile(zc->hZip, &z64ef, zf->zip64_extra_field_length, NULL, NULL);
	}

	if(zf->uncompressed_size > 0) 
		// Advance the file pointer back to the end of the file's data
		_SetFilePointerEx(zc->hZip, (LARGE_INTEGER){.QuadPart = zf->compressed_size}, NULL, FILE_CURRENT);

	zc->num_records++;
}

static void write_end_of_central_directory_to_zip(zipper_context* zc, uint64_t central_directory_size, uint64_t central_directory_start_offset) {
	// Write a zip64 end of central directory record (and locator) if necessary
	if(zc->num_records > 0xFFFF || central_directory_size > 0xFFFFFFFF || central_directory_start_offset > 0xFFFFFFFF) {
		uint64_t zip64_end_of_central_directory_start_offset = central_directory_start_offset + central_directory_size;
		zip64_end_of_central_directory_record z64eoccr = get_zip64_end_of_central_directory_record(zc->num_records, central_directory_size, central_directory_start_offset);
		zip64_end_of_central_directory_locator z64eoccl = get_zip64_end_of_central_directory_locator(zip64_end_of_central_directory_start_offset);

		_WriteFile(zc->hZip, &z64eoccr, sizeof(zip64_end_of_central_directory_record), NULL, NULL);
		_WriteFile(zc->hZip, &z64eoccl, sizeof(zip64_end_of_central_directory_locator), NULL, NULL);
	}

	end_of_central_directory_record eoccr = get_end_of_central_directory_record(zc->num_records, central_directory_size, central_directory_start_offset);
	_WriteFile(zc->hZip, &eoccr, sizeof(end_of_central_directory_record), NULL, NULL);
}

static void write_central_directory_to_zip(zipper_context* zc) {
	uint64_t central_directory_start_offset = _GetFilePointerEx(zc->hZip);

	while(zc->file_queue->size > 0) {
		zipper_file* zf = queue_dequeue(zc->file_queue);

		// Get the central directory header and write it to the zip
		central_directory_header cdh = get_central_directory_header(zf);
		_WriteFile(zc->hZip, &cdh, sizeof(central_directory_header), NULL, NULL);
		_WriteFile(zc->hZip, zf->utf8_name, zf->utf8_name_length, NULL, NULL);
		
		// Write the zip64 extra field if necessary
		if(zf->zip64_extra_field_length > 0) {
			zip64_extra_field z64ef = get_zip64_extra_field(zf);
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

	zipper_context zc = {0};

	zc.zip_name = argv[1];
	zc.hZip = _CreateFileW(zc.zip_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	zc.file_queue = queue_create();

	for(int i = 2; i < argc; i++) {
		zipper_file* zf = zfile_create(argv[i], NO_COMPRESSION);
		write_file_to_zip(&zc, zf);
	}

	write_central_directory_to_zip(&zc);

	Free(zc.file_queue);
	_CloseHandle(zc.hZip);

	return 0;
}
