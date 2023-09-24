#include "zip.h"
#include "utils.h"
#include "wrapper_functions.h"

#define SEARCH_BUFFER_SIZE 										1024
#define MAX_COMMENT_SIZE 										0xFFFF
#define MAX_SEARCH_ITERATIONS 									(MAX_COMMENT_SIZE / SEARCH_BUFFER_SIZE)

#define END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE_FIRST_BYTE 	(END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE & 0xFF)


end_of_central_directory_record find_end_of_central_directory_record(LPWSTR zip_name) {
	end_of_central_directory_record eocdr = {0};

	// Check if zip is empty
	HANDLE hZip = _CreateFileW(zip_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	_ReadFile(hZip, &eocdr, sizeof(end_of_central_directory_record), NULL, NULL);
	if(eocdr.signature == END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE)
		return eocdr;

	// Set file pointer to the start of the end of central directory record assuming there is no comment
	LARGE_INTEGER li = {.QuadPart = -sizeof(end_of_central_directory_record)};
	_SetFilePointerEx(hZip, li, &li, FILE_END);
	_ReadFile(hZip, &eocdr, sizeof(end_of_central_directory_record), NULL, NULL);
	if(eocdr.signature == END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE)
		return eocdr;

	uint8_t buffer[SEARCH_BUFFER_SIZE];
	uint32_t signature = END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE;

	// Work backwards in batches until signature is found, file ends or end of central directory record's max size is reached
	for(int i = 0; li.QuadPart > 1 && i < MAX_SEARCH_ITERATIONS; i++) {
		li.QuadPart = MAX(li.QuadPart - SEARCH_BUFFER_SIZE, 1);
		_SetFilePointerEx(hZip, li, NULL, FILE_BEGIN);
		_ReadFile(hZip, buffer, SEARCH_BUFFER_SIZE + 3, NULL, NULL);

		for(int j = 0; j < SEARCH_BUFFER_SIZE; j++)
			if(buffer[j] == END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE_FIRST_BYTE && !memcmp(buffer + j, &signature, sizeof(uint32_t))) {
				// buffer might not have all the necessary data, in which case read it from file
				if(j <= SEARCH_BUFFER_SIZE - sizeof(end_of_central_directory_record))
					memcpy(&eocdr, buffer + j, sizeof(end_of_central_directory_record));
				else {
					li.QuadPart += j;
					_SetFilePointerEx(hZip, li, NULL, FILE_BEGIN);
					_ReadFile(hZip, &eocdr, sizeof(end_of_central_directory_record), NULL, NULL);
				}

				_CloseHandle(hZip);
				return eocdr;
			}
	}
	
	_CloseHandle(hZip);
	return (end_of_central_directory_record){0};
}
