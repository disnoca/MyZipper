#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../zip.h"
#include "../wrapper_functions.h"
#include "../macros.h"

#define SEARCH_BUFFER_SIZE 										1024
#define MAX_COMMENT_SIZE 										0xFFFF
#define MAX_SEARCH_ITERATIONS 									MAX_COMMENT_SIZE / SEARCH_BUFFER_SIZE

#define END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE_FIRST_BYTE 	END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE & 0xFF


/* Helper Functions */

/**
 * Returns the end of central directory record of the specified zip file. If not found, returns an empty struct.
 * 
 * @param hZip the handle to the zip file
 * @return if found, the end of central directory record of the specified zip file, outherwise, an empty struct
 * 
*/
static end_of_central_directory_record get_end_of_central_directory_record(HANDLE hZip) {
	end_of_central_directory_record eocdr = {0};
	uint8_t* buffer = Malloc(SEARCH_BUFFER_SIZE);

	// Check if zip is empty
	_Rewind(hZip);
	_ReadFile(hZip, &eocdr, sizeof(end_of_central_directory_record), NULL, NULL);
	if(eocdr.signature == END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE)
		return eocdr;

	// Set file pointer to the start of the end of central directory record assuming there is no comment
	LARGE_INTEGER li = {.QuadPart = -sizeof(end_of_central_directory_record)};
	_SetFilePointerEx(hZip, li, &li, FILE_END);
	_ReadFile(hZip, &eocdr, sizeof(end_of_central_directory_record), NULL, NULL);
	if(eocdr.signature == END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE)
		return eocdr;

	uint32_t signature = END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE;

	// Work backwards in batches until signature is found, file ends or end of central directory record's max size is reached
	for(int i = 0; li.QuadPart > 0 && i < MAX_SEARCH_ITERATIONS; i++) {
		li.QuadPart = MAX(li.QuadPart - SEARCH_BUFFER_SIZE, 0);
		_SetFilePointerEx(hZip, li, &li, FILE_BEGIN);
		_ReadFile(hZip, buffer, SEARCH_BUFFER_SIZE, NULL, NULL);

		for(int i = 0; i < SEARCH_BUFFER_SIZE; i++)
			if(buffer[i] == END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE_FIRST_BYTE && !memcmp(buffer, &signature, sizeof(uint32_t)))
				return *(end_of_central_directory_record*)(buffer + i);
	}
	
	return (end_of_central_directory_record){0};
}


/* Main Functions */

static void read_central_directory(HANDLE hZip) {

}

int main() {
	int argc;
	LPWSTR* argv = _CommandLineToArgvW(GetCommandLineW(), &argc);

	LPWSTR zip_name = argv[1];
	HANDLE hZip = _CreateFileW(zip_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	read_central_directory(hZip);
}