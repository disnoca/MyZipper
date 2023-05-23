#ifndef _CRC32_H
#define _CRC32_H

#include <stdint.h>

/**
 * Returns the CRC32 checksum of the specified file.
 * 
 * @param file_name the path to the file to calculate the CRC32 checksum of
*/
uint32_t file_crc32(char* file_name, uint64_t file_size);

#endif