#include <stdbool.h>
#include <stdio.h>
#include <fileapi.h>
#include <windows.h>
#include <winbase.h>
#include "file.h"
#include "crc32.h"
#include "compression/compression.h"
#include "wrapper_functions.h"

#define DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY 10


/* Generic Helper Functions */

static bool is_directory(char* path) {
	return _GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY;
}

static unsigned char days_in_month(unsigned char month, unsigned char year) {
	switch(month) {
		case 1: return 31;
		case 2: return (year % 4 == 0) ? 29 : 28;
		case 3: return 31;
		case 4: return 30;
		case 5: return 31;
		case 6: return 30;
		case 7: return 31;
		case 8: return 31;
		case 9: return 30;
		case 10: return 31;
		case 11: return 30;
		case 12: return 31;
		default: return 0;
	}
}

static void add_timezone_difference(file_info* fi) {
	// Get timezone difference in hours
	TIME_ZONE_INFORMATION tz_info;
	int16_t daylight_is_active = GetTimeZoneInformation(&tz_info) == TIME_ZONE_ID_DAYLIGHT;
	int16_t tz_diff = tz_info.Bias / 60 + daylight_is_active;

	// Convert dos date and time to individual components
	unsigned char utc_relative_year = fi->mod_date >> 9;
	unsigned char utc_month = (fi->mod_date >> 5) & 0x0F;
	unsigned char utc_day = fi->mod_date & 0x1F;
	signed char utc_hour = fi->mod_time >> 11;

	// Add timezone difference to hour
	utc_hour += tz_diff;

	// Adjust day if hour is out of bounds
	if(utc_hour < 0) {
		utc_hour += 24;
		utc_day--;
	}
	else if(utc_hour > 23) {
		utc_hour -= 24;
		utc_day++;
	}

	// Adjust month if day is out of bounds
	if(utc_day < 1) {
		utc_month--;
		if(utc_month < 1) {
			utc_month += 12;
			utc_relative_year--;
		}

		utc_day += days_in_month(utc_month, utc_relative_year + 1980);
	}
	else if(utc_day > days_in_month(utc_month, utc_relative_year + 1980)) {
		utc_month++;
		if(utc_month > 12) {
			utc_month -= 12;
			utc_relative_year++;
		}

		utc_day -= days_in_month(utc_month, utc_relative_year + 1980);
	}

	// Adjust year if month is out of bounds
	if(utc_month < 1) {
		utc_month += 12;
		utc_relative_year--;
	}
	else if(utc_month > 12) {
		utc_month -= 12;
		utc_relative_year++;
	}

	// Save adjusted date and time
	fi->mod_date = (utc_relative_year << 9) | (utc_month << 5) | utc_day;
	fi->mod_time = (utc_hour << 11) | (fi->mod_time & 0x07FF);
}


/* File Helper Functions */

static void get_file_name(file_info* fi, char* path) {
	// Cut out preceding dot and slash if in path
	if(path[0] == '.' && path[1] == '\\')
		path += 2;

	fi->name_length = strlen(path);

	// If file is a directory and does not have a trailing slash, add one
	if(fi->is_directory && path[fi->name_length - 1] != '\\') {
		fi->name = Malloc(fi->name_length + 2);
		memcpy(fi->name, path, fi->name_length);
		memcpy(fi->name + fi->name_length++, "\\", 2);
	} 
	// Otherwise, just copy path into file name
	else {
		fi->name = Malloc(fi->name_length + 1);
		memcpy(fi->name, path, fi->name_length + 1);
	}
}

static void get_file_size(file_info* fi) {
	HANDLE fileHandle = _CreateFileA(fi->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	LARGE_INTEGER fileSize;
	_GetFileSizeEx(fileHandle, &fileSize);

	_CloseHandle(fileHandle);

	fi->uncompressed_size = fileSize.QuadPart;
	fi->is_large = fi->uncompressed_size > 0xFFFFFFFF;
}

static void get_compression_function_and_size(file_info* fi) {
	switch(fi->compression_method) {
		case(NO_COMPRESSION): 
			fi->compression_func = no_compression; 
			fi->compressed_size = fi->uncompressed_size;
			break;
	}
}

static void get_file_children(file_info* fi) {
	WIN32_FIND_DATA fdFile;

	// Append "*" to path to get all files in directory
	char path[fi->name_length + 2];
	memcpy(path, fi->name, fi->name_length);
	memcpy(path + fi->name_length, "*", 2);

	// First two file finds will always return "." and ".."
    HANDLE hFind = _FindFirstFile(path, &fdFile);
	_FindNextFile(hFind, &fdFile);

	// Allocate memory for found file names
	unsigned children_array_capacity = DIRECTORY_FILES_BUFFER_INITIAL_CAPACITY;
	fi->children = Malloc(sizeof(file_info*) * children_array_capacity);

    while(_FindNextFile(hFind, &fdFile)) {
		// Allocate more memory for array if necessary
		if(fi->num_children == children_array_capacity) {
			children_array_capacity *= 2;
			fi->children = Realloc(fi->children, sizeof(file_info*) * children_array_capacity);
		}

		// Copy path and found file name into buffer
		unsigned file_name_length = fi->name_length + strlen(fdFile.cFileName) + 1;
		char file_name[file_name_length];
		memcpy(file_name, fi->name, fi->name_length);
		memcpy(file_name + fi->name_length, fdFile.cFileName, file_name_length - fi->name_length);

		/*
		 * Create child file.
		 * 
		 * This recursively creates the child's children (if a directory) and so on, meaning
		 * it only needs to be manually called once on each of the root folder's children
		 * to capture all of its internal content.
		*/
		file_info* child = fi_create(file_name, fi->compression_method);

		// Save child file
		fi->children[fi->num_children++] = child;
    }

    _FindClose(hFind);
	
	fi->children = Realloc(fi->children, sizeof(file_info*) * fi->num_children);
}

static void get_file_mod_time(file_info* fi) {	// TODO: add time difference
	HANDLE fileHandle = _CreateFileA(fi->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	// Get file last write time
	FILETIME lastWriteTime;
	_GetFileTime(fileHandle, NULL, NULL, &lastWriteTime);
	_CloseHandle(fileHandle);

	// Convert it to DOS date and time
	WORD lpFatDate, lpFatTime;
	_FileTimeToDosDateTime(&lastWriteTime, &lpFatDate, &lpFatTime);

	fi->mod_time = lpFatTime;
	fi->mod_date = lpFatDate;
	add_timezone_difference(fi);
}


/* Header Implementations */

file_info* fi_create(char* path, unsigned compression_method) {
	file_info* fi = Calloc(1, sizeof(file_info));

	fi->compression_method = compression_method;
	fi->is_directory = is_directory(path);

	get_file_name(fi, path);

	if(fi->is_directory)
		get_file_children(fi);
	else {
		fi->fp = Fopen(fi->name, "rb");
		get_file_size(fi);
		get_compression_function_and_size(fi);
		get_file_mod_time(fi);
		fi->crc32 = file_crc32(fi->name, fi->uncompressed_size);
	}

    return fi;
}

void fi_destroy(file_info* fi) {
	if(fi->is_directory)
		Free(fi->children);
	else
		Free(fi->fp);

	Free(fi->name);
	Free(fi);
}



void compress_and_write(file_info* fi, FILE* dest) {
	fi->compression_func(fi->fp, dest);
}
