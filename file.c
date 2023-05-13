#include <stdbool.h>
#include <stdio.h>
#include <fileapi.h>
#include <windows.h>
#include <winbase.h>
#include "file.h"
#include "wrapper_functions.h"

/* Helper Functions */

bool is_directory(char *path) {
	return _GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY;
}

void get_file_path(file* file, char* path) {
	// Ignore preceding dot and slash if in path
	if(path[0] == '.' && path[1] == '\\')
		path += 2;

	uint16_t name_length = strlen(path);
	file->name_length = name_length;
	
	file->name = Malloc(name_length);
	memcpy(file->name, path, name_length);
}

void get_file_data(file* file) {
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

void get_file_mod_time(file* file) {
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

	file->is_directory = is_directory(path);
	get_file_path(file, path);
	get_file_data(file);
	get_file_mod_time(file);

    return file;
}

void destroy_file(file* file) {
	Free(file->children);
	Free(file->name);
	Free(file->data);
	Free(file);
}