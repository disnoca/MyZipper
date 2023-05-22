#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "zip.h"
#include "file.h"
#include "queue.h"
#include "compression/compression.h"
#include "wrapper_functions.h"

#define WINDOWS_NTFS 0x0A00

static FILE* zip_file;
static queue* file_queue;

static uint16_t num_records;
static uint32_t zip_body_size, central_directory_size;


static local_file_header get_file_header(file_info* fi) {
	local_file_header lfh;

	lfh.signature = LOCAL_FILE_HEADER_SIGNATURE;
	lfh.version = 10;
	lfh.flags = 0x0000;
	lfh.compression = fi->compression_method;
	lfh.mod_time = fi->mod_time;
	lfh.mod_date = fi->mod_date;
	lfh.crc32 = fi->crc32;
	lfh.compressed_size = fi->compressed_size & 0xFFFFFFFF;
	lfh.uncompressed_size = fi->uncompressed_size & 0xFFFFFFFF;
	lfh.file_name_length = fi->name_length;
	lfh.extra_field_length = fi->is_large ? sizeof(uint64_t) : 0;

	return lfh;
}

static central_directory_file_header get_central_directory_file_header(file_info* fi) {
	central_directory_file_header cdfh;

	cdfh.signature = CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE;
	cdfh.version_made_by = WINDOWS_NTFS || 10;
	cdfh.version_needed_to_extract = 10;
	cdfh.flags = 0x0000;
	cdfh.compression = fi->compression_method;
	cdfh.mod_time = fi->mod_time;
	cdfh.mod_date = fi->mod_date;
	cdfh.crc32 = fi->crc32;
	cdfh.compressed_size = fi->compressed_size & 0xFFFFFFFF;
	cdfh.uncompressed_size = fi->uncompressed_size & 0xFFFFFFFF;
	cdfh.file_name_length = fi->name_length;
	cdfh.extra_field_length = fi->is_large ? sizeof(uint64_t) : 0;
	cdfh.file_comment_length = 0;
	cdfh.disk_number_start = 0;
	cdfh.internal_file_attributes = 0;
	cdfh.external_file_attributes = 0;
	cdfh.local_header_offset = fi->local_header_offset;

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

static void write_file_to_zip(file_info* fi) {
	/*
	 * Add the directory's children to the zip
	 * This will recursively add all the files containted in the directory
	 * Directories are not written to the zip unless they're empty
	*/
	if(fi->is_directory && fi->num_children > 0) {
		for(unsigned i = 0; i < fi->num_children; i++)
			write_file_to_zip(fi->children[i]);

		fi_destroy(fi);
		return;
	}

	// Set the file's local header offset and save it in the queue
	fi->local_header_offset = zip_body_size;
	queue_enqueue(file_queue, fi);

	// Get the file's local file header and write it to the zip
	local_file_header lfh = get_file_header(fi);
	Fwrite(&lfh, sizeof(local_file_header), 1, zip_file);
	Fwrite(fi->name, fi->name_length, 1, zip_file);
	if(fi->is_large)
		Fwrite(&fi->uncompressed_size, sizeof(uint64_t), 1, zip_file);

	// Compress and write the file's data to the zip
	if(!fi->is_directory)
		compress_and_write(fi, zip_file);

	// Update zip metadata
	num_records++;
	zip_body_size += sizeof(local_file_header) + fi->name_length + fi->uncompressed_size;
}

void write_central_directory_to_zip() {
	// Write each file's central directory file header to the zip
	while(file_queue->size > 0) {
		file_info* fi = queue_dequeue(file_queue);

		// Get the file's central directory file header and write it to the zip
		central_directory_file_header cdfh = get_central_directory_file_header(fi);
		Fwrite(&cdfh, sizeof(central_directory_file_header), 1, zip_file);
		Fwrite(fi->name, fi->name_length, 1, zip_file);
		if(fi->is_large)
			Fwrite(&fi->uncompressed_size, sizeof(uint64_t), 1, zip_file);

		central_directory_size += sizeof(central_directory_file_header) + fi->name_length;
		fi_destroy(fi);
	}

	Free(file_queue);

	// Write the central directory record tail to the zip
	central_directory_record_tail cdrt = get_central_directory_record_tail();
	Fwrite(&cdrt, sizeof(central_directory_record_tail), 1, zip_file);
}

int main(int argc, char** argv) {
	char* zip_file_name = argv[1];
	zip_file = Fopen(zip_file_name, "wb");

	file_queue = queue_create();

	for(int i = 2; i < argc; i++) {
		file_info* fi = fi_create(argv[i], NO_COMPRESSION);
		write_file_to_zip(fi);
	}
	
	write_central_directory_to_zip();
	
	Fclose(zip_file);

	return 0;
}
