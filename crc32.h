#ifndef _CRC32_H
#define _CRC32_H

#include <stdint.h>

/**
 * Returns the CRC32 checksum of the specified file.
 * 
 * @param fp the file to calculate the CRC32 checksum of
*/
uint32_t file_crc32(FILE* fp);

#endif