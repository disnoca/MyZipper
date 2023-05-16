#include <stdbool.h>
#include <stdio.h>
#include <fileapi.h>
#include <windows.h>
#include <winbase.h>
#include "file.h"
#include "compress.h"
#include "wrapper_functions.h"

#define DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY 10


/* Generic Helper Functions */

static bool is_directory(char* path) {
	return _GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY;
}


/* File Helper Functions */

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
		file* child = file_create(file_name);

		// Save child file
		f->children[f->num_children++] = child;
    }

    _FindClose(hFind);
	
	f->children = Realloc(f->children, sizeof(file*) * f->num_children);
}

static void get_file_mod_time(file* f) {	// TODO: add time difference
	HANDLE fileHandle = _CreateFile(f->name);

	// Get file last write time
	FILETIME lastWriteTime;
	_GetFileTime(fileHandle, NULL, NULL, &lastWriteTime);
	_CloseHandle(fileHandle);

	// Convert it to DOS date and time
	WORD lpFatDate, lpFatTime;
	_FileTimeToDosDateTime(&lastWriteTime, &lpFatDate, &lpFatTime);

	f->mod_time = lpFatTime;
	f->mod_date = lpFatDate;
}


/* Header Implementations */

file* file_create(char* path) {
	file* f = Calloc(1, sizeof(file));

	f->is_directory = is_directory(path);

	get_file_name(f, path);

	if(f->is_directory)
		get_file_children(f);
	else
		get_file_mod_time(f);

    return f;
}

void file_destroy(file* f) {
	if(f->num_children > 0)
		Free(f->children);
		
	if(f->compressed_data != NULL)
		Free(f->compressed_data);

	Free(f->name);
	Free(f);
}

void file_compress_and_load_data(file* f, uint16_t compression_method) {
	switch(compression_method) {
	case NO_COMPRESSION: no_compression(f); break;
	}
}

void file_free_data(file* f) {
	Free(f->compressed_data);
	f->compressed_data = NULL;
}
