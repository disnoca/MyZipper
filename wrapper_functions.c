#include <stdio.h>
#include <windows.h>
#include "wrapper_functions.h"

void exit_with_error(char* format, char* msg) {
    fprintf(stderr, format, msg);
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


DWORD _GetFileAttributes(LPCSTR lpFileName) {
    DWORD dwAttrib = GetFileAttributes(lpFileName);
    if(dwAttrib == INVALID_FILE_ATTRIBUTES)
        exit_with_error("GetFileAttributes error: %d\n", GetLastError());
    return dwAttrib;
}

HANDLE _CreateFile(LPCSTR lpFileName) {
    HANDLE fileHandle = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(fileHandle == INVALID_HANDLE_VALUE)
        exit_with_error("CreateFile error: %d\n", GetLastError());
    return fileHandle;
}

void _GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) {
    if(!GetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime))
        exit_with_error("GetFileTime error: %d\n", GetLastError());
}

void _CloseHandle(HANDLE hObject) {
    if(!CloseHandle(hObject))
        exit_with_error("CloseHandle error: %d\n", GetLastError());
}

void _FileTimeToDosDateTime(const FILETIME* lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime) {
    if(!FileTimeToDosDateTime(lpFileTime, lpFatDate, lpFatTime))
        exit_with_error("FileTimeToDosDateTime error: %d\n", GetLastError());
}