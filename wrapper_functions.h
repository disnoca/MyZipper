#ifndef WRAPPER_FUNCTIONS
#define WRAPPER_FUNCTIONS

#include <stdio.h>
#include <stdlib.h>
#include <fileapi.h>

void exit_with_error(const char* format, ...);


void* Malloc(size_t size);
void* Calloc(size_t nitems, size_t size);
void Free(void* ptr);

FILE* Fopen(const char* filename, const char* mode);
void Fclose(FILE* stream);
void Fseek(FILE* stream, long int offset, int origin);
long Ftell(FILE* stream);


DWORD _GetFileAttributes(LPCSTR lpFileName);
HANDLE _CreateFile(LPCSTR lpFileName);
void _GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
void _CloseHandle(HANDLE hObject);
void _FileTimeToDosDateTime(const FILETIME* lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime);

#endif