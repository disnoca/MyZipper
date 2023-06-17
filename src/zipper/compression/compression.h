#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <windows.h>

#define NO_COMPRESSION 0

/**
 * Copies data from the specified origin zipper_file to the specified destination zipper_file and returns its CRC32.
 * 
 * @param zf the zipper_file to copy data from
 * @param dest the zipper_file to copy data to
 * @param dest_offset the offset in the destination zipper_file to start writing data to
*/
void no_compression(zipper_file* zf, LPWSTR dest_name, uint64_t dest_offset);

#endif
