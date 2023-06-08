#include <stdbool.h>
#include <windows.h>
#include "file.h"
#include "crc32.h"
#include "compression/compression.h"
#include "wrapper_functions.h"

#define DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY 10


/* Helper Functions */

static bool is_directory(LPWSTR path) {
	return _GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY;
}

static void get_file_name(file* f, LPWSTR path) {
	// Cut out preceding dot and slash if in path
	if(path[0] == L'.' && path[1] == L'\\')
		path += 2;

	unsigned path_length = wcslen(path);
	bool needs_trailing_slash = f->is_directory && path[path_length - 1] != L'\\';

	// Copy path to wide_char_name
	unsigned wide_char_name_length = path_length + needs_trailing_slash;
	f->wide_char_name = Malloc((wide_char_name_length + 1) * sizeof(WCHAR));
	memcpy(f->wide_char_name, path, (path_length + 1) * sizeof(WCHAR));

	// Add a trailing slash if directory is missing one
	if(needs_trailing_slash) {
		f->wide_char_name[wide_char_name_length - 1] = L'\\';
		f->wide_char_name[wide_char_name_length] = L'\0';
	}

	// Convert name to UTF-8
	f->name_length = _WideCharToMultiByte(CP_UTF8, 0, f->wide_char_name, -1, NULL, 0, NULL, NULL);
	f->name = Malloc(f->name_length + 1);
	_WideCharToMultiByte(CP_UTF8, 0, f->wide_char_name, wide_char_name_length + 1, f->name, f->name_length + 1, NULL, NULL);
}

static void get_file_size(file* f) {
	LARGE_INTEGER fileSize;
	_GetFileSizeEx(f->hFile, &fileSize);

	f->uncompressed_size = fileSize.QuadPart;
	f->is_large = f->uncompressed_size > 0xFFFFFFFF;
}

static void get_compression_function_and_size(file* f) {
	switch(f->compression_method) {
		case(NO_COMPRESSION): 
			f->compression_func = no_compression;
			break;
	}
}

static void get_file_children(file* f) {
	WIN32_FIND_DATAW fdFile;

	unsigned path_length = wcslen(f->wide_char_name);

	// Append "*" to path to get all files in directory
	WCHAR path[path_length + 2];
	memcpy(path, f->wide_char_name, path_length * sizeof(WCHAR));
	memcpy(path + path_length, L"*", 2 * sizeof(WCHAR));

	// First two file finds will always return "." and ".."
    HANDLE hFind = _FindFirstFileW(path, &fdFile);
	_FindNextFileW(hFind, &fdFile);

	// Allocate memory for found file names
	unsigned children_array_capacity = DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY;
	f->children = Malloc(sizeof(file*) * children_array_capacity);

    while(_FindNextFileW(hFind, &fdFile)) {
		// Allocate more memory for array if necessary
		if(f->num_children == children_array_capacity) {
			children_array_capacity *= 2;
			f->children = Realloc(f->children, sizeof(file*) * children_array_capacity);
		}

		// Copy path and found file name into buffer
		unsigned file_name_length = path_length + wcslen(fdFile.cFileName) + 1;
		WCHAR file_name[file_name_length];
		memcpy(file_name, f->wide_char_name, path_length * sizeof(WCHAR));
		memcpy(file_name + path_length, fdFile.cFileName, (file_name_length - path_length) * sizeof(WCHAR));

		/*
		 * Create child file.
		 * 
		 * This recursively creates the child's children (if a directory) and so on, meaning
		 * it only needs to be manually called once on each of the root folder's children
		 * to capture all of its internal content.
		*/
		file* child = file_create(file_name, f->compression_method);

		// Save child file
		f->children[f->num_children++] = child;
    }

    _FindClose(hFind);
	
	f->children = Realloc(f->children, sizeof(file*) * f->num_children);
}

static void get_file_mod_time(file* f) {
	FILETIME lastModFileTime;
	SYSTEMTIME lastModUTCTime, lastModLocalTime;

	// Get file last mod time
	_GetFileTime(f->hFile, NULL, NULL, &lastModFileTime);
	_FileTimeToSystemTime(&lastModFileTime, &lastModUTCTime);
	_SystemTimeToTzSpecificLocalTime(NULL, &lastModUTCTime, &lastModLocalTime);

	f->mod_time = lastModLocalTime.wHour << 11 | lastModLocalTime.wMinute << 5 | lastModLocalTime.wSecond / 2;
	f->mod_date = (lastModLocalTime.wYear - 1980) << 9 | lastModLocalTime.wMonth << 5 | lastModLocalTime.wDay;
}


/* Header Implementations */

file* file_create(LPWSTR path, unsigned compression_method) {
	file* f = Calloc(1, sizeof(file));

	f->compression_method = compression_method;
	f->is_directory = is_directory(path);

	get_file_name(f, path);

	if(f->is_directory)
		get_file_children(f);
	else {
		f->hFile = _CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		get_file_size(f);
		get_compression_function_and_size(f);
		get_file_mod_time(f);
	}

    return f;
}

void file_destroy(file* f) {
	if(f->is_directory)
		Free(f->children);
	else
		_CloseHandle(f->hFile);

	Free(f->name);
	Free(f->wide_char_name);
	Free(f);
}

void compress_and_write(file* f, LPWSTR dest_name, uint64_t dest_offset) {
	if(f->uncompressed_size > 0)
		f->compression_func(f, dest_name, dest_offset);
}
