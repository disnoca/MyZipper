#include <stdio.h>
#include "../../wrapper_functions.h"
#include "../compression.h"

#define BUFFER_SIZE 4096

void no_compression(FILE* origin, FILE* dest) {
	rewind(origin);
	unsigned char* buffer = Malloc(BUFFER_SIZE);
	unsigned bytes_read;

	while((bytes_read = fread(buffer, 1, BUFFER_SIZE, origin)) > 0)
		Fwrite(buffer, 1, bytes_read, dest);

	rewind(origin);
}
