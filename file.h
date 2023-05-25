#ifndef _FILE_H
#define _FILE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct file_info file_info;

struct file_info {
	void (*compression_func)(FILE* origin, FILE* dest);
	bool is_directory;
	unsigned num_children;
	file_info** children;
	FILE* fp;
	char* name;
	uint16_t name_length;
	bool is_large;
	uint64_t uncompressed_size, compressed_size;
	uint16_t compression_method;
	uint16_t mod_time, mod_date;
	uint32_t crc32;
	uint32_t local_header_offset;
};

/**
 * Returns a file struct pointer with the file's data and info.
 * 
 * @param path the relative path to the file
 * @param compression_method the compression method to use
 * @return a file struct pointer with the file's data and info
*/
file_info* fi_create(char* path, unsigned compression_method);

/**
 * Destroys the specified file struct, freeing its allocated memory.
 * 
 * @param fi the file struct to destroy
*/
void fi_destroy(file_info* fi);

/**
 * Compresses and writes the file specified by the file_info struct to the destination file.
 * 
 * @param fi the file_info struct of the file to compress the data from
 * @param dest the file to write the compressed data to
*/
void compress_and_write(file_info* fi, FILE* dest);

#endif