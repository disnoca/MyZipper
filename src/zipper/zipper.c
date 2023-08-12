#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../zip.h"
#include "zipper_file.h"
#include "../queue.h"
#include "../compression/compression.h"
#include "../wrapper_functions.h"

static LPWSTR zip_file_name;
static HANDLE hZip;
static queue* file_queue;

static uint16_t num_records;
static uint64_t zip_body_size, central_directory_size;


static local_file_header get_file_header(zipper_file* zf) {
	local_file_header lfh;

	lfh.signature = LOCAL_FILE_HEADER_SIGNATURE;
	lfh.version = ZIP_VERSION;
	lfh.flags = 0x0000;
	lfh.compression = zf->compression_method;
	lfh.mod_time = zf->mod_time;
	lfh.mod_date = zf->mod_date;
	lfh.crc32 = 0;
	lfh.compressed_size = 0;
	lfh.uncompressed_size = zf->is_large ? 0xFFFFFFFF : zf->uncompressed_size;
	lfh.file_name_length = zf->name_length;
	lfh.extra_field_length = zf->is_large ? sizeof(zip64_extra_field) : 0;

	return lfh;
}

static central_directory_file_header get_central_directory_file_header(zipper_file* zf) {
	central_directory_file_header cdfh;

	cdfh.signature = CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE;
	cdfh.version_made_by = (WINDOWS_NTFS << 8) | ZIP_VERSION;
	cdfh.version_needed_to_extract = ZIP_VERSION;
	cdfh.flags = UTF8_ENCODING;
	cdfh.compression = zf->compression_method;
	cdfh.mod_time = zf->mod_time;
	cdfh.mod_date = zf->mod_date;
	cdfh.crc32 = zf->crc32;
	cdfh.compressed_size = zf->is_large ? 0xFFFFFFFF : zf->compressed_size;
	cdfh.uncompressed_size = zf->is_large ? 0xFFFFFFFF : zf->uncompressed_size;
	cdfh.file_name_length = zf->name_length;
	cdfh.extra_field_length = zf->is_large ? sizeof(zip64_extra_field) : 0;
	cdfh.file_comment_length = 0;
	cdfh.disk_number_start = 0;
	cdfh.internal_file_attributes = 0;
	cdfh.external_file_attributes = 0;
	cdfh.local_header_offset = zf->local_header_offset;

	return cdfh;
}

static central_directory_record_tail get_central_directory_record_tail() {
	central_directory_record_tail cdrt;

	cdrt.signature = CENTRAL_DIRECTORY_RECORD_TAIL_SIGNATURE;
	cdrt.disk_number = 0;
	cdrt.central_directory_start_disk_number = 0;
	cdrt.num_records_on_disk = num_records;
	cdrt.num_total_records = num_records;
	cdrt.central_directory_size = central_directory_size;
	cdrt.central_directory_start_offset = zip_body_size;
	cdrt.comment_length = 0;

	return cdrt;
}

static zip64_extra_field get_zip64_extra_field(zipper_file* zf) {
	zip64_extra_field z64ef;

	z64ef.header_id = ZIP64_EXTRA_FIELD_HEADER_ID;
	z64ef.data_size = sizeof(uint64_t) * 2;
	z64ef.uncompressed_size = zf->uncompressed_size;
	z64ef.compressed_size = 0;

	return z64ef;
}

static void write_file_to_zip(zipper_file* zf) {
	/*
	 * Add the directory's children to the zip
	 * This will recursively add all the files containted in the directory
	 * Directories are not written to the zip unless they're empty
	*/
	if(zf->is_directory && zf->num_children > 0) {
		for(unsigned i = 0; i < zf->num_children; i++)
			write_file_to_zip(zf->children[i]);

		zfile_destroy(zf);
		return;
	}

	// Set the zipper_file's local header offset and save it in the queue
	zf->local_header_offset = zip_body_size;
	queue_enqueue(file_queue, zf);

	uint64_t local_file_header_crc32_offset = zip_body_size + LOCAL_FILE_HEADER_CRC32_OFFSET;

	// Get the zipper_file's local zipper_file header and write it to the zip
	local_file_header lfh = get_file_header(zf);
	_WriteFile(hZip, &lfh, sizeof(local_file_header), NULL, NULL);
	_WriteFile(hZip, zf->name, zf->name_length, NULL, NULL);

	zip_body_size += sizeof(local_file_header) + zf->name_length;

	// Write the zipper_file's zip64 extra field if it's large
	if(zf->is_large) {
		zip64_extra_field z64ef = get_zip64_extra_field(zf);
		_WriteFile(hZip, &z64ef, sizeof(zip64_extra_field), NULL, NULL);
		zip_body_size += sizeof(zip64_extra_field);
	}

	if(!zf->is_directory) {
		// Compress and write the zipper_file's data to the zip
		zfile_compress_and_write(zf, zip_file_name, zip_body_size);

		// Set the zipper_file pointer to the local zipper_file header's CRC32 and compressed size fields and update them
		_SetFilePointerEx(hZip, (LARGE_INTEGER) {.QuadPart = local_file_header_crc32_offset}, NULL, FILE_BEGIN);
		uint32_t compressed_file_size = zf->is_large ? 0xFFFFFFFF : zf->compressed_size;
		_WriteFile(hZip, &zf->crc32, sizeof(uint32_t), NULL, NULL);
		_WriteFile(hZip, &compressed_file_size, sizeof(uint32_t), NULL, NULL);

		if(zf->is_large) {
			// Set the zipper_file pointer to the local zipper_file header's uncompressed size field and update it
			_SetFilePointerEx(hZip, (LARGE_INTEGER) {.QuadPart = zip_body_size - ZIP64_EXTRA_FIELD_COMPRESSED_SIZE_OFFSET_FROM_END}, NULL, FILE_BEGIN);
			_WriteFile(hZip, &zf->compressed_size, sizeof(uint64_t), NULL, NULL);
		}

		// Restore the zipper_file pointer to the end of the written data
		zip_body_size += zf->compressed_size;
		_SetFilePointerEx(hZip, (LARGE_INTEGER) {.QuadPart = zip_body_size}, NULL, FILE_BEGIN);
	}

	num_records++;
}

void write_central_directory_to_zip() {
	// Write each zipper_file's central directory zipper_file header to the zip
	while(file_queue->size > 0) {
		zipper_file* zf = queue_dequeue(file_queue);

		// Get the zipper_file's central directory zipper_file header and write it to the zip
		central_directory_file_header cdfh = get_central_directory_file_header(zf);
		_WriteFile(hZip, &cdfh, sizeof(central_directory_file_header), NULL, NULL);
		_WriteFile(hZip, zf->name, zf->name_length, NULL, NULL);

		central_directory_size += sizeof(central_directory_file_header) + zf->name_length;
		
		// Write the zipper_file's zip64 extra field if it's large
		if(zf->is_large) {
			zip64_extra_field z64ef = get_zip64_extra_field(zf);
			_WriteFile(hZip, &z64ef, sizeof(zip64_extra_field), NULL, NULL);
			central_directory_size += sizeof(zip64_extra_field);
		}
		
		zfile_destroy(zf);
	}

	Free(file_queue);

	// Write the central directory record tail to the zip
	central_directory_record_tail cdrt = get_central_directory_record_tail();
	_WriteFile(hZip, &cdrt, sizeof(central_directory_record_tail), NULL, NULL);
}

int main() {
	int argc;
	LPWSTR* argv = _CommandLineToArgvW(GetCommandLineW(), &argc);

	zip_file_name = argv[1];
	hZip = _CreateFileW(zip_file_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	file_queue = queue_create();

	for(int i = 2; i < argc; i++) {
		zipper_file* zf = zfile_create(argv[i], NO_COMPRESSION);
		write_file_to_zip(zf);
	}

	write_central_directory_to_zip();

	_CloseHandle(hZip);

	return 0;
}
