#ifndef MYFILE
#define MYFILE

#include <stdint.h>
#include <stdbool.h>

typedef struct file file;

struct file {
	bool is_directory;
	unsigned num_children;
	file** children;
	char* name;
	uint16_t name_length;
	uint32_t uncompressed_size, compressed_size;
	unsigned char* compressed_data;
	uint16_t compression_method;
	uint16_t mod_time, mod_date;
	uint32_t crc32;
	uint32_t local_header_offset;
};

/**
 * Returns a file struct pointer with the file's data and info.
 * 
 * @param path the relative path to the file
 * @return a file struct pointer with the file's data and info
*/
file* file_create(char* path);

/**
 * Destroys the specified file struct, freeing its allocated memory.
 * 
 * @param f the file struct to destroy
*/
void file_destroy(file* f);

/**
 * Compresses the specified file with the specified compression method and loads its data.
 * 
 * @param f the file to have its data compressed and loaded
*/
void file_compress_and_load_data(file* f, uint16_t compression_method);

/**
 * Frees the data of the specified file.
 * 
 * @param f the file to have its data freed
*/
void file_free_data(file* f);

#endif