#ifndef MYFILE
#define MYFILE

#include <stdint.h>
#include <stdbool.h>

typedef struct file file;

struct file {
	char* name;
	uint16_t name_length;
	bool is_directory;
	unsigned num_children;
	file** children;
	uint32_t size;
	unsigned char* data;
	uint16_t mod_time, mod_date;
	uint32_t crc;
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
 * @param file the file struct to destroy
*/
void file_destroy(file* file);

#endif