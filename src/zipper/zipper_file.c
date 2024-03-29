#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include "zipper_file.h"
#include "../compression/compression.h"
#include "../wrapper_functions.h"

#define DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY 10


/* Helper Functions */

void replace_char(char* str, char find, char replace) {
	char* current_pos = strchr(str, find);
	while(current_pos) {
		*current_pos = replace;
		current_pos = strchr(current_pos, find);
	}
}

static void get_file_name(zipper_file* zf, LPWSTR path) {
	// Cut out preceding dot and slash if in path
	if(path[0] == L'.' && path[1] == L'\\')
		path += 2;

	unsigned path_length = wcslen(path);
	bool needs_trailing_slash = zf->is_directory && path[path_length - 1] != L'\\';

	// Copy path to wide_char_name
	unsigned wide_char_name_length = path_length + needs_trailing_slash;
	zf->utf16_name = Malloc((wide_char_name_length + 1) * sizeof(WCHAR));
	memcpy(zf->utf16_name, path, (path_length + 1) * sizeof(WCHAR));

	// Add a trailing slash if directory is missing one
	if(needs_trailing_slash) {
		zf->utf16_name[wide_char_name_length - 1] = L'\\';
		zf->utf16_name[wide_char_name_length] = L'\0';
	}

	// Convert name to UTF-8
	zf->utf8_name_length = _WideCharToMultiByte(CP_UTF8, 0, zf->utf16_name, -1, NULL, 0, NULL, NULL) - 1;
	zf->utf8_name = Malloc(zf->utf8_name_length + 1);
	_WideCharToMultiByte(CP_UTF8, 0, zf->utf16_name, -1, zf->utf8_name, zf->utf8_name_length + 1, NULL, NULL);

	// Replace backward slashes with forward slashes in UTF-8 name (the one written to the ZIP file)
	replace_char(zf->utf8_name, '\\', '/');
}

static void get_file_size(zipper_file* zf) {
	LARGE_INTEGER fileSize;
	_GetFileSizeEx(zf->hFile, &fileSize);

	zf->uncompressed_size = fileSize.QuadPart;
}

static void get_compression_function(zipper_file* zf) {
	switch(zf->compression_method) {
		case(NO_COMPRESSION): zf->compression_func = no_compression_compress; break;
	}
}

static void get_file_children(zipper_file* zf) {
	WIN32_FIND_DATAW fdFile;

	unsigned path_length = wcslen(zf->utf16_name);

	// Append "*" to path to get all files in directory
	WCHAR path[path_length + 2];
	memcpy(path, zf->utf16_name, path_length * sizeof(WCHAR));
	memcpy(path + path_length, L"*", 2 * sizeof(WCHAR));

	// First two zipper_file finds will always return "." and ".."
    HANDLE hFind = _FindFirstFileW(path, &fdFile);
	_FindNextFileW(hFind, &fdFile);

	// Allocate memory for found zipper_file names
	unsigned children_array_capacity = DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY;
	zf->children = Malloc(sizeof(zipper_file*) * children_array_capacity);

    while(_FindNextFileW(hFind, &fdFile)) {
		// Allocate more memory for array if necessary
		if(zf->num_children == children_array_capacity) {
			children_array_capacity *= 2;
			zf->children = Realloc(zf->children, sizeof(zipper_file*) * children_array_capacity);
		}

		// Copy path and found zipper_file name into buffer
		unsigned file_name_length = path_length + wcslen(fdFile.cFileName) + 1;
		WCHAR file_name[file_name_length];
		memcpy(file_name, zf->utf16_name, path_length * sizeof(WCHAR));
		memcpy(file_name + path_length, fdFile.cFileName, (file_name_length - path_length) * sizeof(WCHAR));

		/*
		 * Create child zipper_file.
		 * 
		 * This recursively creates the child's children (if a directory) and so on, meaning
		 * it only needs to be manually called once on each of the root folder's children
		 * to capture all of its internal content.
		*/
		zipper_file* child = zfile_create(file_name, zf->compression_method);

		// Save child zipper_file
		zf->children[zf->num_children++] = child;
    }

    _FindClose(hFind);
	
	zf->children = Realloc(zf->children, sizeof(zipper_file*) * zf->num_children);
}

static void get_file_mod_time(zipper_file* zf) {
	FILETIME lastModFileTime;
	SYSTEMTIME lastModUTCTime, lastModLocalTime;

	// Get zipper_file last mod time
	_GetFileTime(zf->hFile, NULL, NULL, &lastModFileTime);
	_FileTimeToSystemTime(&lastModFileTime, &lastModUTCTime);
	_SystemTimeToTzSpecificLocalTime(NULL, &lastModUTCTime, &lastModLocalTime);

	zf->mod_time = lastModLocalTime.wHour << 11 | lastModLocalTime.wMinute << 5 | lastModLocalTime.wSecond / 2;
	zf->mod_date = (lastModLocalTime.wYear - 1980) << 9 | lastModLocalTime.wMonth << 5 | lastModLocalTime.wDay;
}


/* Header Implementations */

zipper_file* zfile_create(LPWSTR path, unsigned compression_method) {
	printf("Creating zipper_file for %ls\n", path);

	zipper_file* zf = Calloc(1, sizeof(zipper_file));
	
	zf->windows_file_attributes = _GetFileAttributesW(path);
	zf->is_directory = zf->windows_file_attributes & FILE_ATTRIBUTE_DIRECTORY;

	get_file_name(zf, path);

	if(zf->is_directory)
		get_file_children(zf);
	else {
		zf->hFile = _CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		get_file_size(zf);
		get_file_mod_time(zf);
	}

	if(zf->uncompressed_size > 0) {
		zf->compression_method = compression_method;
		get_compression_function(zf);
	}

    return zf;
}

void zfile_destroy(zipper_file* zf) {
	if(zf->is_directory)
		Free(zf->children);
	else
		_CloseHandle(zf->hFile);

	Free(zf->utf8_name);
	Free(zf->utf16_name);
	Free(zf);
}

void zfile_compress_and_write(zipper_file* zf, LPWSTR dest_name, uint64_t dest_offset) {
	if(zf->uncompressed_size == 0)
		return;

	compression_result cr = zf->compression_func(zf->utf16_name, dest_name, dest_offset, zf->uncompressed_size);
	zf->compressed_size = cr.destination_size;
	zf->crc32 = cr.crc32;
}
