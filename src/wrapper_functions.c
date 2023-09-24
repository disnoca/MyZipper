#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <shlobj.h>
#include "wrapper_functions.h"

void exit_with_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

void* Malloc(size_t size) {
    void* ptr = malloc(size);
    if(ptr == NULL) 
        exit_with_error("%s\n", "Malloc error");
    return ptr;
}

void* Calloc(size_t nitems, size_t size) {
    void* ptr = calloc(nitems, size);
    if(ptr == NULL) 
        exit_with_error("%s\n", "Calloc error");
    return ptr;
}

void* Realloc(void* ptr, size_t size) {
    if(size == 0) {
        Free(ptr);
        return NULL;
    }

    ptr = realloc(ptr, size);
    if(ptr == NULL) 
        exit_with_error("%s\n", "Realloc error");
    return ptr;
}

void Free(void* ptr) {
    free(ptr);
    ptr = NULL;
}


HANDLE _CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if(hFile == INVALID_HANDLE_VALUE)
        exit_with_error("CreateFileW error: %lu\n", GetLastError());
    return hFile;
}

void _CloseHandle(HANDLE hObject) {
    if(!CloseHandle(hObject))
        exit_with_error("CloseHandle error: %lu\n", GetLastError());
}


void _CreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) {
    if(!CreateDirectoryW(lpPathName, lpSecurityAttributes))
        exit_with_error("CreateDirectoryW error: %lu\n", GetLastError());
}

void _SHCreateDirectoryExW(HWND hwnd, LPCWSTR pszPath, const SECURITY_ATTRIBUTES *psa) {
    if(SHCreateDirectoryExW(hwnd, pszPath, psa) != ERROR_SUCCESS)
        exit_with_error("SHCreateDirectoryExA error: %lu\n", GetLastError());
}

DWORD _GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart) {
    DWORD dwRetVal = GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);
    if(dwRetVal == 0)
        exit_with_error("GetFullPathNameW error: %lu\n", GetLastError());
    return dwRetVal;
}



BOOL _ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    if(!ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped)) {
        if(GetLastError() != ERROR_IO_PENDING)
            exit_with_error("ReadFile error: %lu\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

void _WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if(!WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped))
        if(GetLastError() != ERROR_IO_PENDING)
            exit_with_error("WriteFile error: %lu\n", GetLastError());
}


void _SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod) {
    if(!SetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod))
        exit_with_error("SetFilePointerEx error: %lu\n", GetLastError());
}

LONGLONG _GetFilePointerEx(HANDLE hFile) {
    LARGE_INTEGER fp = {0};
    SetFilePointerEx(hFile, fp, &fp, FILE_CURRENT);
    return fp.QuadPart;
}


void _Rewind(HANDLE hFile) {
    if(SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        exit_with_error("Rewind error: %lu\n", GetLastError());
}

void _GetOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
    if(!GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait))
        exit_with_error("GetOverlappedResult error: %lu\n", GetLastError());
}


DWORD _GetFileAttributesW(LPCWSTR lpFileName) {
    DWORD dwAttributes = GetFileAttributesW(lpFileName);
    if(dwAttributes == INVALID_FILE_ATTRIBUTES)
        exit_with_error("GetFileAttributesW error: %lu\n", GetLastError());
    return dwAttributes;
}


void _GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) {
    if(!GetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime))
        exit_with_error("GetFileTime error: %lu\n", GetLastError());
}

void _FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime) {
    if(!FileTimeToSystemTime(lpFileTime, lpSystemTime))
        exit_with_error("FileTimeToSystemTime error: %lu\n", GetLastError());
}

void _SystemTimeToTzSpecificLocalTime(const TIME_ZONE_INFORMATION* lpTimeZoneInformation, const SYSTEMTIME* lpUniversalTime, LPSYSTEMTIME lpLocalTime) {
    if(!SystemTimeToTzSpecificLocalTime(lpTimeZoneInformation, lpUniversalTime, lpLocalTime))
        exit_with_error("SystemTimeToTzSpecificLocalTime error: %lu\n", GetLastError());
}


HANDLE _FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData) {
    HANDLE hFindFile = FindFirstFileW(lpFileName, lpFindFileData);
    if(hFindFile == INVALID_HANDLE_VALUE)
        exit_with_error("FindFirstFileW error: %lu\n", GetLastError());
    return hFindFile;
}

BOOL _FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData) {
    if(!FindNextFileW(hFindFile, lpFindFileData)) {
        if(GetLastError() != ERROR_NO_MORE_FILES)
            exit_with_error("FindNextFileW error: %lu\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

void _FindClose(HANDLE hFindFile) {
    if(!FindClose(hFindFile))
        exit_with_error("FindClose error: %lu\n", GetLastError());
}


void _GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize) {
    if(!GetFileSizeEx(hFile, lpFileSize))
        exit_with_error("GetFileSizeEx error: %lu\n", GetLastError());
}


HANDLE _CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE  lpStartAddress, __drv_aliasesMem LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    HANDLE hThread = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
    if(hThread == NULL)
        exit_with_error("CreateThread error: %lu\n", GetLastError());
    return hThread;
}

DWORD _WaitForMultipleObjects(DWORD nCount,const HANDLE* lpHandles,BOOL bWaitAll,DWORD dwMilliseconds) {
    DWORD dwWaitResult = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
    if(dwWaitResult == WAIT_FAILED)
        exit_with_error("WaitForMultipleObjects error: %lu\n", GetLastError());
    return dwWaitResult;
}

LPWSTR* _CommandLineToArgvW(LPCWSTR lpCmdLine, int *pNumArgs) {
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, pNumArgs);
    if(argv == NULL)
        exit_with_error("CommandLineToArgvW error: %lu\n", GetLastError());
    return argv;
}


int _WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar) {
    int ret = WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
    if(ret == 0)
        exit_with_error("WideCharToMultiByte error: %lu\n", GetLastError());
    return ret;
}

int _MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar) {
    int ret = MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
    if(ret == 0)
        exit_with_error("MultiByteToWideChar error: %lu\n", GetLastError());
    return ret;
}
