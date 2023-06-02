#ifndef _CRC32_H
#define _CRC32_H

#include <stdint.h>

#define CRC32_INITIAL_VALUE 	  0xFFFFFFFF
#define CRC32_REVERSED_POLYNOMIAL 0xEDB88320

/**
 * Combines two specified CRC32 values.
 * 
 * @param crc1 the first CRC32 value
 * @param crc2 the second CRC32 value
*/
uint32_t crc32_combine(uint32_t crc1, uint32_t crc2, uint64_t len2);

#endif