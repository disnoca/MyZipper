#include <stdio.h>
#include "wrapper_functions.h"
#include "crc32.h"

#define REVERSED_POLYNOMIAL 0xEDB88320

#define BUFFER_SIZE 4096

uint32_t file_crc32(FILE* fp) {
	rewind(fp);
	unsigned char* buffer = Malloc(BUFFER_SIZE);
	unsigned bytes_read;

	uint32_t crc = 0xFFFFFFFF;

	while((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
		for(unsigned i = 0; i < bytes_read; i++) {
			crc ^= buffer[i];
			for(unsigned j = 0; j < 8; j++)
				crc = (crc >> 1) ^ (REVERSED_POLYNOMIAL & -(crc & 1));
		}
	}

	Free(buffer);
	rewind(fp);
	return ~crc;
}