#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "../zip.h"
#include "../wrapper_functions.h"


/* Main Functions */

static void read_central_directory(HANDLE hZip, end_of_central_directory_record* eocdr) {
	if(!eocdr->signature) {
		printf("End of central directory record not found\n");
		return;
	}

	printf("Number of Records: %hu\n\n", eocdr->total_num_records);

	// Set file pointer to the start of the central directory
	LARGE_INTEGER li = {.QuadPart = eocdr->central_directory_start_offset};
	_SetFilePointerEx(hZip, li, NULL, FILE_BEGIN);

	central_directory_header cdr;
	char file_name[MAX_PATH];

	// Iterate over the central directory records
	for(uint16_t i = 0; i < eocdr->total_num_records; i++) {
		_ReadFile(hZip, &cdr, sizeof(central_directory_header), NULL, NULL);

		// Check for signature
		if(cdr.signature != CENTRAL_DIRECTORY_HEADER_SIGNATURE) {
			printf("Zip file is corrupt\n");
			return;
		}

		// Read name and skip other variable length fields
		if(cdr.file_name_length > 0) {
			_ReadFile(hZip, file_name, cdr.file_name_length, NULL, NULL);
			file_name[cdr.file_name_length] = '\0';
		}

		li.QuadPart = cdr.extra_field_length + cdr.file_comment_length;
		_SetFilePointerEx(hZip, li, NULL, FILE_CURRENT);

		// Print information
		printf("File Name: %s\n", cdr.file_name_length > 0 ? file_name : "");
		printf("Compression: %hu\n", cdr.compression);
		printf("Modified Time: %hx\n", cdr.mod_time);
		printf("Modified Date: %hx\n", cdr.mod_date);
		printf("CRC32: %x\n", cdr.crc32);
		printf("Compressed Size: %u\n", cdr.compressed_size);
		printf("Uncompressed Size: %u\n", cdr.uncompressed_size);
		printf("Local Header Offset: %u\n\n", cdr.local_header_offset);
	}
}

int main() {
	int argc;
	LPWSTR* argv = _CommandLineToArgvW(GetCommandLineW(), &argc);

	if(argc != 2) {
		printf("Usage: zip_info archive_name\n");
		return 0;
	}

	LPWSTR zip_name = argv[1];
	HANDLE hZip = _CreateFileW(zip_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	end_of_central_directory_record eocdr;
	find_end_of_central_directory_record(zip_name, &eocdr);
	read_central_directory(hZip, &eocdr);

	_CloseHandle(hZip);

	return 0;
}
