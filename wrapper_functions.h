#ifndef _WRAPPER_FUNCTIONS_H
#define _WRAPPER_FUNCTIONS_H

#include <stdlib.h>
#include <windows.h>

void exit_with_error(const char* format, ...);


void* Malloc(size_t size);
void* Calloc(size_t nitems, size_t size);
void* Realloc(void* ptr, size_t size);
void Free(void* ptr);


HANDLE _CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
void _CloseHandle(HANDLE hObject);
BOOL _ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
void _WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
void _SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
void _Rewind(HANDLE hFile);

DWORD _GetFileAttributes(LPCSTR lpFileName);

void _GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
void _FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime);
void _SystemTimeToTzSpecificLocalTime(const TIME_ZONE_INFORMATION* lpTimeZoneInformation, const SYSTEMTIME* lpUniversalTime, LPSYSTEMTIME lpLocalTime);

HANDLE _FindFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
BOOL _FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
void _FindClose(HANDLE hFindFile);

void _GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);

HANDLE _CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE  lpStartAddress, __drv_aliasesMem LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
DWORD _WaitForMultipleObjects(DWORD nCount,const HANDLE* lpHandles,BOOL bWaitAll,DWORD dwMilliseconds);


#endif