#ifndef WRAPPER_FUNCTIONS
#define WRAPPER_FUNCTIONS

#include <stdlib.h>
#include <fileapi.h>

void exit_with_error(char* format, char* msg);

void* Malloc(size_t size);
void* Calloc(size_t nitems, size_t size);
void Free(void* ptr);


DWORD _GetFileAttributes(LPCSTR lpFileName);
HANDLE _CreateFile(LPCSTR lpFileName);
void _GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
void _CloseHandle(HANDLE hObject);
void _FileTimeToDosDateTime(const FILETIME* lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime);

#endif