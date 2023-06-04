#include <stdbool.h>
#include <windows.h>
#include "file.h"
#include "crc32.h"
#include "compression/compression.h"
#include "wrapper_functions.h"

#define DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY 10


/* Helper Functions */

static bool is_directory(char* path) {
	return _GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY;
}

static void get_file_name(file* f, char* path) {
	// Cut out preceding dot and slash if in path
	if(path[0] == '.' && path[1] == '\\')
		path += 2;

	f->name_length = strlen(path);

	// If file is a directory and does not have a trailing slash, add one
	if(f->is_directory && path[f->name_length - 1] != '\\') {
		f->name = Malloc(f->name_length + 2);
		memcpy(f->name, path, f->name_length);
		memcpy(f->name + f->name_length++, "\\", 2);
	} 
	// Otherwise, just copy path into file name
	else {
		f->name = Malloc(f->name_length + 1);
		memcpy(f->name, path, f->name_length + 1);
	}
}

static void get_file_size(file* f) {
	HANDLE fileHandle = _CreateFileA(f->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	LARGE_INTEGER fileSize;
	_GetFileSizeEx(fileHandle, &fileSize);

	_CloseHandle(fileHandle);

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
	WIN32_FIND_DATA fdFile;

	// Append "*" to path to get all files in directory
	char path[f->name_length + 2];
	memcpy(path, f->name, f->name_length);
	memcpy(path + f->name_length, "*", 2);

	// First two file finds will always return "." and ".."
    HANDLE hFind = _FindFirstFile(path, &fdFile);
	_FindNextFile(hFind, &fdFile);

	// Allocate memory for found file names
	unsigned children_array_capacity = DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY;
	f->children = Malloc(sizeof(file*) * children_array_capacity);

    while(_FindNextFile(hFind, &fdFile)) {
		// Allocate more memory for array if necessary
		if(f->num_children == children_array_capacity) {
			children_array_capacity *= 2;
			f->children = Realloc(f->children, sizeof(file*) * children_array_capacity);
		}

		// Copy path and found file name into buffer
		unsigned file_name_length = f->name_length + strlen(fdFile.cFileName) + 1;
		char file_name[file_name_length];
		memcpy(file_name, f->name, f->name_length);
		memcpy(file_name + f->name_length, fdFile.cFileName, file_name_length - f->name_length);

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
	HANDLE fileHandle = _CreateFileA(f->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	FILETIME lastModFileTime;
	SYSTEMTIME lastModUTCTime, lastModLocalTime;

	// Get file last mod time
	_GetFileTime(fileHandle, NULL, NULL, &lastModFileTime);
	_CloseHandle(fileHandle);
	_FileTimeToSystemTime(&lastModFileTime, &lastModUTCTime);
	_SystemTimeToTzSpecificLocalTime(NULL, &lastModUTCTime, &lastModLocalTime);

	f->mod_time = lastModLocalTime.wHour << 11 | lastModLocalTime.wMinute << 5 | lastModLocalTime.wSecond / 2;
	f->mod_date = (lastModLocalTime.wYear - 1980) << 9 | lastModLocalTime.wMonth << 5 | lastModLocalTime.wDay;
}


/* Header Implementations */

file* file_create(char* path, unsigned compression_method) {
	file* f = Calloc(1, sizeof(file));

	f->compression_method = compression_method;
	f->is_directory = is_directory(path);

	get_file_name(f, path);

	if(f->is_directory)
		get_file_children(f);
	else {
		f->hFile = _CreateFileA(f->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
	Free(f);
}

void compress_and_write(file* f, char* dest_name, uint64_t dest_offset) {
	f->compression_func(f, dest_name, dest_offset);
}
