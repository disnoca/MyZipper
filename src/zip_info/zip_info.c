#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
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

	eocdr = (end_of_central_directory_record){0};
	uint8_t* buffer = Malloc(SEARCH_BUFFER_SIZE);
	uint32_t signature = END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE;

	// Work backwards in batches until signature is found, file ends or end of central directory record's max size is reached
	for(int i = 0; li.QuadPart > 0 && i < MAX_SEARCH_ITERATIONS; i++) {
		li.QuadPart = MAX(li.QuadPart - SEARCH_BUFFER_SIZE, 0);
		_SetFilePointerEx(hZip, li, &li, FILE_BEGIN);
		_ReadFile(hZip, buffer, SEARCH_BUFFER_SIZE, NULL, NULL);

		for(int i = 0; i < SEARCH_BUFFER_SIZE; i++)
			if(buffer[i] == END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE_FIRST_BYTE && !memcmp(buffer, &signature, sizeof(uint32_t))) {
				memcpy(&eocdr, buffer + i, sizeof(end_of_central_directory_record));
				break;
			}
	}
	
	Free(buffer);
	return eocdr;
}


/* Main Functions */

static void read_central_directory(HANDLE hZip) {
	end_of_central_directory_record eocdr = get_end_of_central_directory_record(hZip);
	if(!eocdr.signature) {
		printf("End of central directory record not found\n");
		return;
	}

	printf("Number of Records: %hu\n\n", eocdr.total_num_records);

	// Set file pointer to the start of the central directory
	LARGE_INTEGER li = {.QuadPart = eocdr.central_directory_start_offset};
	_SetFilePointerEx(hZip, li, &li, FILE_BEGIN);

	central_directory_header cdr;
	char *file_name = NULL, *comment = NULL;

	// Iterate over the central directory records
	for(uint16_t i = 0; i < eocdr.total_num_records; i++) {
		_ReadFile(hZip, &cdr, sizeof(central_directory_header), NULL, NULL);

		// Check for signature
		if(cdr.signature != CENTRAL_DIRECTORY_HEADER_SIGNATURE) {
			printf("Zip file is corrupt\n");
			return;
		}

		// Read variable length fields and convert them to wide char
		if(cdr.file_name_length > 0) {
			file_name = Realloc(file_name, cdr.file_name_length + 1);
			_ReadFile(hZip, file_name, cdr.file_name_length, NULL, NULL);
			file_name[cdr.file_name_length] = '\0';
		}

		if(cdr.extra_field_length > 0)
			_ReadFile(hZip, NULL, cdr.extra_field_length, NULL, NULL);

		if(cdr.file_comment_length > 0) {
			comment = Realloc(comment, cdr.file_comment_length + 1);
			_ReadFile(hZip, comment, cdr.file_comment_length, NULL, NULL);
			comment[cdr.file_name_length] = '\0';
		}

		// Print information
		printf("File Name: %s\n", cdr.file_name_length > 0 ? file_name : "");
		printf("Comment: %s\n", cdr.file_comment_length > 0 ? comment : "");
		printf("Compression: %hu\n", cdr.compression);
		printf("Modified Time: %hx\n", cdr.mod_time);
		printf("Modified Date: %hx\n", cdr.mod_date);
		printf("CRC32: %x\n", cdr.crc32);
		printf("Compressed Size: %u\n", cdr.compressed_size);
		printf("Uncompressed Size: %u\n", cdr.uncompressed_size);
		printf("Local Header Offset: %u\n\n", cdr.local_header_offset);
	}
	
	Free(file_name);
	Free(comment);
}

int main() {
	int argc;
	LPWSTR* argv = _CommandLineToArgvW(GetCommandLineW(), &argc);

	LPWSTR zip_name = argv[1];
	HANDLE hZip = _CreateFileW(zip_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	read_central_directory(hZip);

	_CloseHandle(hZip);

	return 0;
}