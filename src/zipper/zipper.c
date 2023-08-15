#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../zip.h"
#include "zipper_file.h"
#include "queue.h"
#include "../compression/compression.h"
#include "../wrapper_functions.h"

 #define MIN(a,b) \
	({ __typeof__ (a) _a = (a); \
    	__typeof__ (b) _b = (b); \
    	_a < _b ? _a : _b; })

typedef struct {
    LPWSTR zip_file_name;
	HANDLE hZip;

	uint16_t num_records;

	queue* file_queue;
} zipper_context;


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
	lfh.file_name_length = zf->name_length;
	lfh.extra_field_length = zf->zip64_extra_field_length;

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
	cdfh.compressed_size = zf->uncompressed_size >= 0xFFFFFFFF ? 0xFFFFFFFF : zf->compressed_size;
	cdfh.uncompressed_size = MIN(zf->uncompressed_size, 0xFFFFFFFF);
	cdfh.file_name_length = zf->name_length;
	cdfh.extra_field_length = zf->zip64_extra_field_length;
	cdfh.file_comment_length = 0;
	cdfh.disk_number_start = 0;
	cdfh.internal_file_attributes = 0;
	cdfh.external_file_attributes = 0;
	cdfh.local_header_offset = MIN(zf->local_header_offset, 0xFFFFFFFF);

	return cdfh;
}

static central_directory_record_tail get_central_directory_record_tail(uint16_t num_records, uint64_t central_directory_size, uint64_t central_directory_start_offset) {
	central_directory_record_tail cdrt;

	cdrt.signature = CENTRAL_DIRECTORY_RECORD_TAIL_SIGNATURE;
	cdrt.disk_number = 0;
	cdrt.central_directory_start_disk_number = 0;
	cdrt.num_records_on_disk = num_records;
	cdrt.num_total_records = num_records;
	cdrt.central_directory_size = central_directory_size;
	cdrt.central_directory_start_offset = central_directory_start_offset;
	cdrt.comment_length = 0;

	return cdrt;
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

	z64ef.data_size = num_extra_fields * sizeof(uint64_t);

	return z64ef;
}

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

	if(zf->uncompressed_size >= 0xFFFFFFFF || zf->local_header_offset >= 0xFFFFFFFF)
		zf->zip64_extra_field_length = ZIP64_EXTRA_FIELD_BASE_SIZE + sizeof(uint64_t) * (2 * (zf->uncompressed_size >= 0xFFFFFFFF) + (zf->local_header_offset >= 0xFFFFFFFF));

	// Write the file's compressed data if it's not a directory
	if(!zf->is_directory) {
		uint64_t header_size = sizeof(local_file_header) + zf->name_length + zf->zip64_extra_field_length;
		zfile_compress_and_write(zf, zc->zip_file_name, zf->local_header_offset + header_size);
	}

	// Write the header
	local_file_header lfh = get_file_header(zf);
	_WriteFile(zc->hZip, &lfh, sizeof(local_file_header), NULL, NULL);
	_WriteFile(zc->hZip, zf->name, zf->name_length, NULL, NULL);

	// Write the zip64 extra field if necessary
	if(zf->zip64_extra_field_length > 0) {
		zip64_extra_field z64ef = get_zip64_extra_field(zf);
		_WriteFile(zc->hZip, &z64ef, zf->zip64_extra_field_length, NULL, NULL);
	}

	if(!zf->is_directory) 
		// Advance the file pointer back to the end of the file's data
		_SetFilePointerEx(zc->hZip, (LARGE_INTEGER){.QuadPart = zf->compressed_size}, NULL, FILE_CURRENT);

	zc->num_records++;
}

void write_central_directory_to_zip(zipper_context* zc) {
	uint64_t central_directory_start_offset = _GetFilePointerEx(zc->hZip);

	// Write each zipper_file's central directory zipper_file header to the zip
	while(zc->file_queue->size > 0) {
		zipper_file* zf = queue_dequeue(zc->file_queue);

		// Get the zipper_file's central directory zipper_file header and write it to the zip
		central_directory_file_header cdfh = get_central_directory_file_header(zf);
		_WriteFile(zc->hZip, &cdfh, sizeof(central_directory_file_header), NULL, NULL);
		_WriteFile(zc->hZip, zf->name, zf->name_length, NULL, NULL);
		
		// Write the zip64 extra field if necessary
		if(zf->zip64_extra_field_length > 0) {
			zip64_extra_field z64ef = get_zip64_extra_field(zf);
			_WriteFile(zc->hZip, &z64ef, zf->zip64_extra_field_length, NULL, NULL);
		}
		
		zfile_destroy(zf);
	}

	uint64_t central_directory_size = _GetFilePointerEx(zc->hZip) - central_directory_start_offset;

	// Write the central directory record tail to the zip
	central_directory_record_tail cdrt = get_central_directory_record_tail(zc->num_records, central_directory_size, central_directory_start_offset);
	_WriteFile(zc->hZip, &cdrt, sizeof(central_directory_record_tail), NULL, NULL);
}

int main() {
	int argc;
	LPWSTR* argv = _CommandLineToArgvW(GetCommandLineW(), &argc);

	zipper_context zc = {0};

	zc.zip_file_name = argv[1];
	zc.hZip = _CreateFileW(zc.zip_file_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

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
