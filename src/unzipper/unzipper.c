#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include "../zip.h"
#include "../compression/compression.h"
#include "../wrapper_functions.h"


static void extract_file(LPWSTR zip_name, LPWSTR file_name, central_directory_header cdr) {
	// todo: ask user if he wants to overwrite
	HANDLE hFile = _CreateFileW(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, cdr.external_file_attributes & 0xFF, NULL);
	_CloseHandle(hFile);

	if(cdr.uncompressed_size == 0)
		return;

	uint64_t file_data_offset = cdr.local_header_offset + sizeof(local_file_header) + cdr.file_name_length + cdr.extra_field_length;
	uint32_t crc32;

	switch(cdr.compression) {
	case(NO_COMPRESSION): crc32 = no_compression_decompress(zip_name, file_name, file_data_offset, cdr.compressed_size); break;
	default: break;
	}

	wprintf(L"%S is corrupted.\n", file_name);
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
	LPWSTR file_name;

	// Iterate over the central directory records
	for(uint16_t i = 0; i < eocdr.total_num_records; i++) {
		_ReadFile(hZip, &cdr, sizeof(central_directory_header), NULL, NULL);

		file_name = Realloc(file_name, cdr.file_name_length + sizeof(WCHAR));
		_ReadFile(hZip, file_name, cdr.file_name_length, NULL, NULL);
		file_name[cdr.file_name_length] = L'\0';

		extract_file(zip_name, file_name, cdr);

		_ReadFile(hZip, NULL, cdr.extra_field_length + cdr.file_comment_length, NULL, NULL);
	}

	return 0;
}