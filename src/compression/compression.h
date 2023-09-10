#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <windows.h>

#define NO_COMPRESSION 0

typedef struct {
	uint64_t destination_size;
	uint32_t crc32;
} compression_result;

typedef compression_result decompression_result;

/**
 * Copies data from the specified origin file to the specified destination file and
 * returns the compression result.
 * 
 * @param origin_name the name of the origin file
 * @param dest_name the name of the destination file
 * @param dest_offset the offset in the destination file to start writing data to
 * @param file_size the number of bytes to copy
 * @return the compression result
*/
compression_result no_compression_compress(LPWSTR origin_name, LPWSTR dest_name, uint64_t dest_offset, uint64_t file_size);

/**
 * Copies data from the specified origin file to the specified destination file
 * and returns the file's crc32.
 * 
 * @param origin_name the name of the origin file
 * @param dest_name the name of the destination file
 * @param origin_offset the offset in the origin file to start reading data from
 * @param file_size the number of bytes to copy
 * @return the file's crc32
*/
uint32_t no_compression_decompress(LPWSTR origin_name, LPWSTR dest_name, uint64_t origin_offset, uint64_t file_size);

#endif
