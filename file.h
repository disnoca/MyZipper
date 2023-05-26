#ifndef _FILE_H
#define _FILE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct file file;

struct file {
	bool is_directory;
	unsigned num_children;
	file** children;
	FILE* fp;
	char* name;
	uint16_t name_length;
	bool is_large;
	uint64_t uncompressed_size, compressed_size;
	uint16_t compression_method;
	uint16_t mod_time, mod_date;
	uint32_t crc32;
	uint32_t local_header_offset;
	void (*compression_func)(FILE* origin, FILE* dest);
};

/**
 * Returns a file struct pointer with the file's data and info.
 * 
 * @param path the relative path to the file
 * @param compression_method the compression method to use
 * @return a file struct pointer with the file's data and info
*/
file* file_create(char* path, unsigned compression_method);

/**
 * Destroys the specified file struct, freeing its allocated memory.
 * 
 * @param f the file struct to destroy
*/
void file_destroy(file* f);

/**
 * Compresses and writes the file specified by the file struct to the destination file.
 * 
 * @param f the file struct of the file to compress the data from
 * @param dest the file to write the compressed data to
*/
void compress_and_write(file* f, FILE* dest);

#endif