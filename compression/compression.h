#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <windows.h>

#define NO_COMPRESSION 0

/**
 * Copies data from the specified origin file to the specified destination file.
 * 
 * @param origin the file to copy data from
 * @param dest the file to copy data to
*/
void no_compression(char* origin_name, char* dest_name, uint64_t file_size, uint64_t dest_file_offset);

#endif
