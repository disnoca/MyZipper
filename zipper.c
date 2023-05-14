#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "zip.h"
#include "file.h"
#include "wrapper_functions.h"

#define WINDOWS_NTFS_FS 0x0A00

static uint16_t num_records;
static uint32_t body_size, central_directory_offset;


static local_file_header get_file_header(file* f) {
	local_file_header lfh;

	lfh.signature = LOCAL_FILE_HEADER_SIGNATURE;
	lfh.version = 10;
	lfh.flags = 0x0000;
	lfh.compression = 0;
	lfh.mod_time = f->mod_time;
	lfh.mod_date = f->mod_date;
	lfh.crc32 = f->crc;
	lfh.compressed_size = f->size;
	lfh.uncompressed_size = f->size;
	lfh.file_name_length = f->name_length;
	lfh.extra_field_length = 0;

	return lfh;
}

static central_directory_file_header get_central_directory_file_header(file* f) {
	central_directory_file_header cdfh;

	cdfh.signature = CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE;
	cdfh.version_made_by = WINDOWS_NTFS_FS || 10;
	cdfh.version_needed_to_extract = 10;
	cdfh.flags = 0x0000;
	cdfh.compression = 0;
	cdfh.mod_time = f->mod_time;
	cdfh.mod_date = f->mod_date;
	cdfh.crc32 = f->crc;
	cdfh.compressed_size = f->size;
	cdfh.uncompressed_size = f->size;
	cdfh.file_name_length = f->name_length;
	cdfh.extra_field_length = 0;
	cdfh.file_comment_length = 0;
	cdfh.disk_number_start = 0;
	cdfh.internal_file_attributes = 0;
	cdfh.external_file_attributes = 0;
	cdfh.local_header_offset = central_directory_offset;

	return cdfh;
}

static central_directory_record_tail get_central_directory_record_tail() {
	central_directory_record_tail cdrt;

	cdrt.signature = CENTRAL_DIRECTORY_RECORD_TAIL_SIGNATURE;
	cdrt.disk_number = 0;
	cdrt.central_directory_start_disk_number = 0;
	cdrt.num_records_on_disk = num_records;
	cdrt.num_total_records = num_records;
	cdrt.central_directory_size = central_directory_offset;
	cdrt.central_directory_start_offset = body_size;
	cdrt.comment_length = 0;

	return cdrt;
}

static void add_file_to_zip(FILE* zip_file, char* path) {
	file* f = file_create(path);

	local_file_header lfh = get_file_header(f);
	central_directory_file_header cdfh = get_central_directory_file_header(f);

	Fwrite(&lfh, sizeof(local_file_header), 1, zip_file);
	Fwrite(f->name, f->name_length, 1, zip_file);
	Fwrite(f->data, f->size, 1, zip_file);
	// TODO: save central directory file header

	num_records++;
	body_size += sizeof(local_file_header) + f->name_length + f->size;
	central_directory_offset += sizeof(central_directory_file_header) + f->name_length;

	file_destroy(f);
}

int main() {
	FILE* zip_file = Fopen("my_zip.zip", "wb");

	add_file_to_zip(zip_file, "img.jpg");
	add_file_to_zip(zip_file, "file.h");
	
	central_directory_record_tail cdrt = get_central_directory_record_tail();

	// TODO: write saved central directory file headers
	Fwrite(&cdrt, sizeof(central_directory_record_tail), 1, zip_file);
	
	Fclose(zip_file);

	return 0;
}
