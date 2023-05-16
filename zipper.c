#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "zip.h"
#include "file.h"
#include "queue.h"
#include "wrapper_functions.h"

#define WINDOWS_NTFS_FS 0x0A00

static FILE* zip_file;
static queue* file_queue;

static uint16_t num_records;
static uint32_t zip_body_size, central_directory_size;


static local_file_header get_file_header(file* f) {
	local_file_header lfh;

	lfh.signature = LOCAL_FILE_HEADER_SIGNATURE;
	lfh.version = 10;
	lfh.flags = 0x0000;
	lfh.compression = 0;
	lfh.mod_time = f->mod_time;
	lfh.mod_date = f->mod_date;
	lfh.crc32 = f->crc32;
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
	cdfh.crc32 = f->crc32;
	cdfh.compressed_size = f->size;
	cdfh.uncompressed_size = f->size;
	cdfh.file_name_length = f->name_length;
	cdfh.extra_field_length = 0;
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

	// Get the file's local file header and write it to the zip
	local_file_header lfh = get_file_header(f);
	Fwrite(&lfh, sizeof(local_file_header), 1, zip_file);
	Fwrite(f->name, f->name_length, 1, zip_file);

	// Write the file's data to the zip if not an empty directory
	if(!f->is_directory) {
		Fwrite(f->data, f->size, 1, zip_file);
		Free(f->data);
		f->data = NULL;		// ???????????????????????????
	}

	// Update zip metadata
	num_records++;
	zip_body_size += sizeof(local_file_header) + f->name_length + f->size;
}

void write_central_directory_to_zip() {
	// Write each file's central directory file header to the zip
	while(file_queue->size > 0) {
		file* f = queue_dequeue(file_queue);
		central_directory_file_header cdfh = get_central_directory_file_header(f);

		Fwrite(&cdfh, sizeof(central_directory_file_header), 1, zip_file);
		Fwrite(f->name, f->name_length, 1, zip_file);

		central_directory_size += sizeof(central_directory_file_header) + f->name_length;
		file_destroy(f);
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
		file* f = file_create(argv[i]);
		write_file_to_zip(f);
	}
	
	write_central_directory_to_zip();
	
	Fclose(zip_file);

	return 0;
}
