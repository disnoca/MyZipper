#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <windows.h>

#define NO_COMPRESSION 0

/**
 * Copies data from the specified origin file to the specified destination file and returns its CRC32.
 * 
 * @param f the file to copy data from
 * @param dest the file to copy data to
 * @param dest_file_offset the offset in the destination file to start writing data to
*/
void no_compression(file* f, char* dest_name, uint64_t dest_file_offset);

#endif
