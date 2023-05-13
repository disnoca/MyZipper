#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include "wrapper_functions.h"

void exit_with_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

void* Malloc(size_t size) {
    void* ptr;
    if((ptr = malloc(size)) == NULL) 
        exit_with_error("%s\n", "Malloc error");
    return ptr;
}

void* Calloc(size_t nitems, size_t size) {
    void* ptr;
    if((ptr = calloc(nitems, size)) == NULL) 
        exit_with_error("%s\n", "Calloc error");
    return ptr;
}

void Free(void* ptr) {
    free(ptr);
    ptr = NULL;
}


FILE* Fopen(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);
    if(fp == NULL)
        exit_with_error("Fopen error: %s\n", filename);
    return fp;
}

void Fclose(FILE* stream) {
    if(fclose(stream) == EOF)
        exit_with_error("%s\n", "Fclose error");
}

void Fseek(FILE* stream, long int offset, int origin) {
    if(fseek(stream, offset, origin) != 0)
        exit_with_error("%s\n", "Fseek error");
}

long Ftell(FILE* stream) {
    long offset = ftell(stream);
    if(offset == -1L)
        exit_with_error("%s\n", "Ftell error");
    return offset;
}



DWORD _GetFileAttributes(LPCSTR lpFileName) {
    DWORD dwAttrib = GetFileAttributes(lpFileName);
    if(dwAttrib == INVALID_FILE_ATTRIBUTES)
        exit_with_error("GetFileAttributes error: %lu\n", GetLastError());
    return dwAttrib;
}

HANDLE _CreateFile(LPCSTR lpFileName) {
    HANDLE fileHandle = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(fileHandle == INVALID_HANDLE_VALUE)
        exit_with_error("CreateFile error: %lu\n", GetLastError());
    return fileHandle;
}

void _GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) {
    if(!GetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime))
        exit_with_error("GetFileTime error: %lu\n", GetLastError());
}

void _CloseHandle(HANDLE hObject) {
    if(!CloseHandle(hObject))
        exit_with_error("CloseHandle error: %lu\n", GetLastError());
}

void _FileTimeToDosDateTime(const FILETIME* lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime) {
    if(!FileTimeToDosDateTime(lpFileTime, lpFatDate, lpFatTime))
        exit_with_error("FileTimeToDosDateTime error: %lu\n", GetLastError());
}