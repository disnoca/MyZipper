#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <windows.h>

#define NO_COMPRESSION 0

/**
 * Copies data from the specified origin zipper_file to the specified destination zipper_file and
 * sets the zipper_file's CRC32 and compressed size to the file's.
 * 
 * @param zf the zipper_file to copy data from
 * @param dest the zipper_file to copy data to
 * @param dest_offset the offset in the destination zipper_file to start writing data to
*/
void no_compression_compress(zipper_file* zf, LPWSTR dest_name, uint64_t dest_offset);

/**
 * Copies data from the specified origin file to the specified destination file.
 * 
 * @param origin_name the name of the origin file
 * @param dest_name the name of the destination file
 * @param origin_offset the offset in the origin file to start reading data from
 * @param file_size the number of bytes to copy
*/
void no_compression_decompress(LPWSTR origin_name, LPWSTR dest_name, uint64_t origin_offset, uint64_t file_size);

#endif
