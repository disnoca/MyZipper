#include "../../wrapper_functions.h"
#include "../compression.h"

#define BUFFER_SIZE 4096

void no_compression(HANDLE origin, HANDLE dest) {
	_Rewind(origin);
	unsigned char* buffer = Malloc(BUFFER_SIZE);
	DWORD bytes_read;

	while(_ReadFile(origin, buffer, BUFFER_SIZE, &bytes_read, NULL) && bytes_read > 0)
		_WriteFile(dest, buffer, bytes_read, NULL, NULL);

	_Rewind(origin);
}
