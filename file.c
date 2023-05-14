#include <stdbool.h>
#include <stdio.h>
#include <fileapi.h>
#include <windows.h>
#include <winbase.h>
#include "file.h"
#include "wrapper_functions.h"

#define DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY 10


/* Generic Helper Functions */

static bool is_directory(char* path) {
	return _GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY;
}


/* File Helper Functions */

static void get_file_name(file* file, char* path) {
	// Cut out preceding dot and slash if in path
	if(path[0] == '.' && path[1] == '\\')
		path += 2;

	uint16_t name_length = strlen(path);

	// Cut out trailing slash if file is a directory and path has it
	if(file->is_directory && path[name_length - 1] == '\\')
		name_length--;
	
	file->name_length = name_length;
	
	file->name = Malloc(name_length);
	memcpy(file->name, path, name_length);
}

static void get_file_children(file* f) {
	WIN32_FIND_DATA fdFile;

	// Append "\*" to file name to get all files in directory
	char path[f->name_length + 3];
	memcpy(path, f->name, f->name_length);
	memcpy(path + f->name_length, "\\*", 3);

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

		// Copy path and found file name into buffer, inserting a slash in between
		unsigned file_name_length = f->name_length + strlen(fdFile.cFileName) + 2;
		char file_name[file_name_length];
		memcpy(file_name, f->name, f->name_length);
		file_name[f->name_length] = '\\';
		memcpy(file_name + f->name_length + 1, fdFile.cFileName, file_name_length - f->name_length - 1);

		/*
		 * Create child file.
		 * 
		 * This recursively creates the child's children (if a directory) and so on, meaning
		 * it only needs to be manually called once on each of the root folder's children
		 * to capture all of its internal content.
		*/
		file* child = get_file(file_name);

		// Save child file
		f->children[f->num_children++] = child;
    }

    _FindClose(hFind);

	Realloc(f->children, sizeof(file) * f->num_children);
}

static void get_file_data(file* file) {
	// Get file size
	FILE* fp = Fopen(file->name, "rb");
	Fseek(fp, 0L, SEEK_END);
	uint32_t file_size = Ftell(fp);
	rewind(fp);

	// Read file data
	unsigned char* file_data = Malloc(file_size);
	fread(file_data, file_size, 1, fp);
	Fclose(fp);

	file->size = file_size;
	file->data = file_data;
}

static void get_file_mod_time(file* file) {
	HANDLE fileHandle = _CreateFile(file->name);

	// Get file last write time
	FILETIME lastWriteTime;
	_GetFileTime(fileHandle, NULL, NULL, &lastWriteTime);
	_CloseHandle(fileHandle);

	// Convert it to DOS date and time
	WORD lpFatDate, lpFatTime;
	_FileTimeToDosDateTime(&lastWriteTime, &lpFatDate, &lpFatTime);

	file->mod_time = lpFatTime;
	file->mod_date = lpFatDate;
}


/* Header Implementations */

file* get_file(char* path) {
	file* file = Calloc(1, sizeof(file));

	get_file_name(file, path);

	file->is_directory = is_directory(file->name);

	if(file->is_directory)
		get_file_children(file);
	else {
		get_file_data(file);
		get_file_mod_time(file);
	}

    return file;
}

void destroy_file(file* file) {
	Free(file->children);
	Free(file->name);
	Free(file->data);
	Free(file);
}