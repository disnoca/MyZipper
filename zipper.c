#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "zip.h"
#include "file.h"
#include "queue.h"
#include "compression/compression.h"
#include "wrapper_functions.h"

static LPWSTR zip_file_name;
static HANDLE hZip;
static queue* file_queue;

static uint16_t num_records;
static uint64_t zip_body_size, central_directory_size;


static local_file_header get_file_header(file* f) {
	local_file_header lfh;

	lfh.signature = LOCAL_FILE_HEADER_SIGNATURE;
	lfh.version = ZIP_VERSION;
	lfh.flags = 0x0000;
	lfh.compression = f->compression_method;
	lfh.mod_time = f->mod_time;
	lfh.mod_date = f->mod_date;
	lfh.crc32 = 0;
	lfh.compressed_size = 0;
	lfh.uncompressed_size = f->is_large ? 0xFFFFFFFF : f->uncompressed_size;
	lfh.file_name_length = f->name_length;
	lfh.extra_field_length = f->is_large ? sizeof(zip64_extra_field) : 0;

	return lfh;
}

static central_directory_file_header get_central_directory_file_header(file* f) {
	central_directory_file_header cdfh;

	cdfh.signature = CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE;
	cdfh.version_made_by = (WINDOWS_NTFS << 8) | ZIP_VERSION;
	cdfh.version_needed_to_extract = ZIP_VERSION;
	cdfh.flags = UTF8_ENCODING;
	cdfh.compression = f->compression_method;
	cdfh.mod_time = f->mod_time;
	cdfh.mod_date = f->mod_date;
	cdfh.crc32 = f->crc32;
	cdfh.compressed_size = f->is_large ? 0xFFFFFFFF : f->compressed_size;
	cdfh.uncompressed_size = f->is_large ? 0xFFFFFFFF : f->uncompressed_size;
	cdfh.file_name_length = f->name_length;
	cdfh.extra_field_length = f->is_large ? sizeof(zip64_extra_field) : 0;
	cdfh.file_comment_length = 0;
	cdfh.disk_number_start = 0;
	cdfh.internal_file_attributes = 0;
	cdfh.external_file_attributes = 0;
	cdfh.local_header_offset = f->local_header_offset;

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

static zip64_extra_field get_zip64_extra_field(file* f) {
	zip64_extra_field z64ef;

	z64ef.header_id = ZIP64_EXTRA_FIELD_HEADER_ID;
	z64ef.data_size = sizeof(uint64_t) * 2;
	z64ef.uncompressed_size = f->uncompressed_size;
	z64ef.compressed_size = 0;

	return z64ef;
}

static void write_file_to_zip(file* f) {
	/*
	 * Add the directory's children to the zip
	 * This will recursively add all the files containted in the directory
	 * Directories are not written to the zip unless they're empty
	*/
	if(f->is_directory && f->num_children > 0) {
		for(unsigned i = 0; i < f->num_children; i++)
			write_file_to_zip(f->children[i]);

		file_destroy(f);
		return;
	}

	// Set the file's local header offset and save it in the queue
	f->local_header_offset = zip_body_size;
	queue_enqueue(file_queue, f);

	uint64_t local_file_header_crc32_offset = zip_body_size + LOCAL_FILE_HEADER_CRC32_OFFSET;

	// Get the file's local file header and write it to the zip
	local_file_header lfh = get_file_header(f);
	_WriteFile(hZip, &lfh, sizeof(local_file_header), NULL, NULL);
	_WriteFile(hZip, f->name, f->name_length, NULL, NULL);

	zip_body_size += sizeof(local_file_header) + f->name_length;

	// Write the file's zip64 extra field if it's large
	if(f->is_large) {
		zip64_extra_field z64ef = get_zip64_extra_field(f);
		_WriteFile(hZip, &z64ef, sizeof(zip64_extra_field), NULL, NULL);
		zip_body_size += sizeof(zip64_extra_field);
	}

	if(!f->is_directory) {
		// Compress and write the file's data to the zip
		compress_and_write(f, zip_file_name, zip_body_size);

		// Set the file pointer to the local file header's CRC32 and compressed size fields and update them
		_SetFilePointerEx(hZip, (LARGE_INTEGER) {.QuadPart = local_file_header_crc32_offset}, NULL, FILE_BEGIN);
		uint32_t compressed_file_size = f->is_large ? 0xFFFFFFFF : f->compressed_size;
		_WriteFile(hZip, &f->crc32, sizeof(uint32_t), NULL, NULL);
		_WriteFile(hZip, &compressed_file_size, sizeof(uint32_t), NULL, NULL);

		if(f->is_large) {
			// Set the file pointer to the local file header's uncompressed size field and update it
			_SetFilePointerEx(hZip, (LARGE_INTEGER) {.QuadPart = zip_body_size - ZIP64_EXTRA_FIELD_COMPRESSED_SIZE_OFFSET_FROM_END}, NULL, FILE_BEGIN);
			_WriteFile(hZip, &f->compressed_size, sizeof(uint64_t), NULL, NULL);
		}

		// Restore the file pointer to the end of the written data
		zip_body_size += f->compressed_size;
		_SetFilePointerEx(hZip, (LARGE_INTEGER) {.QuadPart = zip_body_size}, NULL, FILE_BEGIN);
	}

	num_records++;
}

void write_central_directory_to_zip() {
	// Write each file's central directory file header to the zip
	while(file_queue->size > 0) {
		file* f = queue_dequeue(file_queue);

		// Get the file's central directory file header and write it to the zip
		central_directory_file_header cdfh = get_central_directory_file_header(f);
		_WriteFile(hZip, &cdfh, sizeof(central_directory_file_header), NULL, NULL);
		_WriteFile(hZip, f->name, f->name_length, NULL, NULL);

		central_directory_size += sizeof(central_directory_file_header) + f->name_length;
		
		// Write the file's zip64 extra field if it's large
		if(f->is_large) {
			zip64_extra_field z64ef = get_zip64_extra_field(f);
			_WriteFile(hZip, &z64ef, sizeof(zip64_extra_field), NULL, NULL);
			central_directory_size += sizeof(zip64_extra_field);
		}
		
		file_destroy(f);
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
		file* f = file_create(argv[i], NO_COMPRESSION);
		write_file_to_zip(f);
	}

	write_central_directory_to_zip();

	_CloseHandle(hZip);

	return 0;
}
