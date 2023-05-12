#ifndef MYFILE
#define MYFILE

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	bool is_directory;
	char* name;
	uint16_t name_length;
	uint32_t size;
	unsigned char* data;
	uint16_t mod_time, mod_date;
} file;

/**
 * Returns a file struct pointer with the file's data and info.
 * 
 * @param path the relative path to the file
 * @return a file struct pointer with the file's data and info
*/
file* get_file(char* path);

/**
 * Destroys the specified file struct, freeing its allocated memory.
 * 
 * @param file the file struct to destroy
*/
void destroy_file(file* file);

#endif