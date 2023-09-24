#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include "../zip.h"
#include "../compression/compression.h"
#include "../wrapper_functions.h"
#include "../utils.h"


static void create_directory(LPWSTR dir_name, uint16_t dir_attributes) {
	if(!CreateDirectoryW(dir_name, NULL)) {
		if(GetLastError() != ERROR_PATH_NOT_FOUND)
			exit_with_error("CreateDirectoryW error: %lu\n", GetLastError());

		// Get the full path of the directory
		WCHAR full_path[MAX_PATH];
		_GetFullPathNameW(dir_name, MAX_PATH, full_path, NULL);
		
		// Create the directory recursively
		_SHCreateDirectoryExW(NULL, full_path, NULL);
	}

	_SetFileAttributesW(dir_name, dir_attributes);
}

static void create_file(LPWSTR file_name, uint16_t file_attributes) {
	HANDLE hFile = CreateFileW(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, file_attributes, NULL);

	if(hFile == INVALID_HANDLE_VALUE) {
		if(GetLastError() != ERROR_PATH_NOT_FOUND)
			exit_with_error("CreateFileW error: %lu\n", GetLastError());

		// Get the parent path of the file
		WCHAR parent_path[MAX_PATH];
		wcscpy(parent_path, file_name);
		*wcsrchr(parent_path, L'/') = L'\0';
		
		// Create the parent directory and try to create the file again
		create_directory(parent_path, FILE_ATTRIBUTE_DIRECTORY);
		hFile = _CreateFileW(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, file_attributes, NULL);
	}

	_CloseHandle(hFile);
}

static void extract_file(LPWSTR zip_name, LPWSTR file_name, central_directory_header cdr) {
	if(cdr.external_file_attributes & FILE_ATTRIBUTE_DIRECTORY) {
		create_directory(file_name, cdr.external_file_attributes & 0xFF);
		return;
	}

	create_file(file_name, cdr.external_file_attributes & 0xFF);

	if(cdr.uncompressed_size == 0)
		return;

	uint64_t file_data_offset = cdr.local_header_offset + sizeof(local_file_header) + cdr.file_name_length + cdr.extra_field_length;
	uint32_t crc32;

	switch(cdr.compression) {
	case(NO_COMPRESSION): crc32 = no_compression_decompress(zip_name, file_name, file_data_offset, cdr.compressed_size); break;
	default: break;
	}

	if(cdr.crc32 != crc32)
		printf("%ls is corrupted.\n", file_name);
}

int main() {
	int argc;
	LPWSTR* argv = _CommandLineToArgvW(GetCommandLineW(), &argc);

	LPWSTR zip_name = argv[1];
	HANDLE hZip = _CreateFileW(zip_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	end_of_central_directory_record eocdr = find_end_of_central_directory_record(zip_name);

	// Set file pointer to the start of the central directory
	LARGE_INTEGER li = {.QuadPart = eocdr.central_directory_start_offset};
	_SetFilePointerEx(hZip, li, NULL, FILE_BEGIN);

	central_directory_header cdr;
	char utf8_file_name[MAX_PATH];
	WCHAR utf16_file_name[MAX_PATH];

	// Iterate over the central directory records
	for(uint16_t i = 0; i < eocdr.total_num_records; i++) {
		_ReadFile(hZip, &cdr, sizeof(central_directory_header), NULL, NULL);

		if(cdr.signature != CENTRAL_DIRECTORY_HEADER_SIGNATURE)
			exit_with_error("Invalid central directory header signature.\n");

		// Read the file name and convert it to UTF-16
		_ReadFile(hZip, utf8_file_name, cdr.file_name_length, NULL, NULL);

		// Exclude trailing slash if present
		if(utf8_file_name[cdr.file_name_length - 1] == '/') {
			cdr.external_file_attributes |= FILE_ATTRIBUTE_DIRECTORY;
			utf8_file_name[cdr.file_name_length - 1] = '\0';
		}
		else
			utf8_file_name[cdr.file_name_length] = '\0';
		
		_MultiByteToWideChar(CP_UTF8, 0, utf8_file_name, -1, utf16_file_name, MAX_PATH);

		printf("Extracting %ls\n", utf16_file_name);
		extract_file(zip_name, utf16_file_name, cdr);

		li.QuadPart = cdr.extra_field_length + cdr.file_comment_length;
		_SetFilePointerEx(hZip, li, NULL, FILE_CURRENT);
	}

	printf("Done\n");

	_CloseHandle(hZip);
	return 0;
}