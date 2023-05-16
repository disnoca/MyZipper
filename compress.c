#include <stdio.h>
#include "compress.h"
#include "wrapper_functions.h"

/* Helper Functions */

static uint32_t calculate_crc32(unsigned char* data, uint32_t length) {
	uint32_t crc = 0xFFFFFFFF;

	for (unsigned i = 0; i < length; i++) {
		crc ^= data[i];
		for (unsigned j = 0; j < 8; j++)
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
	}

	return ~crc;
}


/* Header Implementation */

void no_compression(file* f) {
	f->compression_method = NO_COMPRESSION;

	// Get file size
	FILE* fp = Fopen(f->name, "rb");
	Fseek(fp, 0L, SEEK_END);
	f->uncompressed_size = Ftell(fp);
	f->compressed_size = f->uncompressed_size;
	rewind(fp);

	f->compressed_data = Malloc(f->compressed_size);

	// Read file data
	fread(f->compressed_data, f->compressed_size, 1, fp);
	Fclose(fp);
	
	f->crc32 = calculate_crc32(f->compressed_data, f->uncompressed_size);
}
