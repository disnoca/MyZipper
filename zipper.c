#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "zip.h"
#include "file.h"
#include "wrapper_functions.h"

uint32_t crc32(const unsigned char* data, unsigned length) {
	uint32_t crc = 0xFFFFFFFF;

	for (unsigned i = 0; i < length; i++) {
		crc ^= data[i];
		for (unsigned j = 0; j < 8; j++)
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
	}

	return ~crc;
}

local_file_header get_file_header(uint32_t file_size, uint16_t file_name_length, uint32_t crc32) {
	local_file_header lfh;

	lfh.signature = LOCAL_FILE_HEADER_SIGNATURE;
	lfh.version = 10;
	lfh.flags = 0x0000;
	lfh.compression = 0;
	lfh.mod_time = 0x7d1c;
	lfh.mod_date = 0x354b;
	lfh.crc32 = crc32;
	lfh.compressed_size = file_size;
	lfh.uncompressed_size = file_size;
	lfh.file_name_length = file_name_length;
	lfh.extra_field_length = 0;

	return lfh;
}

central_directory_file_header get_central_directory_file_header(uint32_t file_size, uint16_t file_name_length, uint32_t crc32) {
	central_directory_file_header cdfh;

	cdfh.signature = CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE;
	cdfh.version_made_by = 10;
	cdfh.version_needed_to_extract = 10;
	cdfh.flags = 0x0000;
	cdfh.compression = 0;
	cdfh.mod_time = 0x7d1c;
	cdfh.mod_date = 0x354b;
	cdfh.crc32 = crc32;
	cdfh.compressed_size = file_size;
	cdfh.uncompressed_size = file_size;
	cdfh.file_name_length = file_name_length;
	cdfh.extra_field_length = 0;
	cdfh.file_comment_length = 0;
	cdfh.disk_number_start = 0;
	cdfh.internal_file_attributes = 0;
	cdfh.external_file_attributes = 0;
	cdfh.local_header_relative_offset = 0;

	return cdfh;
}

central_directory_record_tail get_central_directory_record_tail(uint32_t file_size, uint16_t file_name_length) {
	central_directory_record_tail cdrt;

	cdrt.signature = CENTRAL_DIRECTORY_RECORD_TAIL_SIGNATURE;
	cdrt.disk_number = 0;
	cdrt.central_directory_start_disk_number = 0;
	cdrt.num_records_on_disk = 1;
	cdrt.num_total_records = 1;
	cdrt.central_directory_size = sizeof(central_directory_file_header) + file_name_length;
	cdrt.central_directory_start_offset = sizeof(local_file_header) + file_name_length + file_size;
	cdrt.comment_length = 0;

	return cdrt;
}

int main() {
	//char* file_name = "img.jpg";
//
	//FILE* fp = fopen(file_name, "rb");
	//fseek(fp, 0L, SEEK_END);
	//uint32_t file_size = ftell(fp);
	//rewind(fp);
//
	//unsigned char* file_data = Malloc(file_size);
	//fread(file_data, file_size, 1, fp);
	//fclose(fp);
	//
	//uint16_t file_name_length = strlen(file_name);
	//uint32_t crc = crc32(file_data, file_size);
//
	//local_file_header lfh = get_file_header(file_size, file_name_length, crc);
	//central_directory_file_header cdfh = get_central_directory_file_header(file_size, file_name_length, crc);
	//central_directory_record_tail cdrt = get_central_directory_record_tail(file_size, file_name_length);
//
	//FILE* zip_file = fopen("my_zip.zip", "wb");
	//fwrite(&lfh, sizeof(local_file_header), 1, zip_file);
	//fwrite(file_name, file_name_length, 1, zip_file);
	//fwrite(file_data, file_size, 1, zip_file);
	//fwrite(&cdfh, sizeof(central_directory_file_header), 1, zip_file);
	//fwrite(file_name, file_name_length, 1, zip_file);
	//fwrite(&cdrt, sizeof(central_directory_record_tail), 1, zip_file);
//
	//fclose(zip_file);

	get_file(".\\img.jpg");

	return 0;
}
