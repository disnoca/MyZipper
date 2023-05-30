#include <stdint.h>
#include <stdbool.h>
#include "../../concurrency.h"
#include "../../wrapper_functions.h"
#include "../compression.h"

#define BUFFER_SIZE 4096

typedef struct {
	char *origin_name, *dest_name;
	uint64_t origin_offset;
	uint64_t start_byte;
	uint64_t bytes_to_write;
} file_write_thread_data;

DWORD WINAPI thread_file_write(void* data) {
	file_write_thread_data* fwtd = (file_write_thread_data*) data;

  	HANDLE ofh = _CreateFileA(fwtd->origin_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  	HANDLE dfh = _CreateFileA(fwtd->dest_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	_SetFilePointerEx(ofh, (LARGE_INTEGER) {.QuadPart = fwtd->origin_offset}, NULL, FILE_BEGIN);
	_SetFilePointerEx(dfh, (LARGE_INTEGER) {.QuadPart = fwtd->start_byte}, NULL, FILE_BEGIN);

	unsigned char* buffer = Malloc(BUFFER_SIZE);
	DWORD batch_size;
    uint64_t total_bytes_written = 0;

	while(total_bytes_written < fwtd->bytes_to_write) {
		batch_size = MIN(BUFFER_SIZE, fwtd->bytes_to_write - total_bytes_written);
		total_bytes_written += batch_size;

		_ReadFile(ofh, buffer, batch_size, NULL, NULL);
		_WriteFile(dfh, buffer, batch_size, NULL, NULL);
	}

	_CloseHandle(ofh);
	_CloseHandle(dfh);
	Free(buffer);
  	return 0;
}

void no_compression(char* origin_name, char* dest_name, uint64_t file_size, uint64_t dest_file_offset) {
	unsigned num_threads = file_size > MIN_SIZE_FOR_CONCURRENCY ? num_cores() : 1;

	file_write_thread_data* threads_data = Malloc(num_threads * sizeof(file_write_thread_data));
	HANDLE* threads = Malloc(num_threads * sizeof(HANDLE));

	uint64_t bytes_per_thread = file_size / num_threads;
	unsigned remainder = file_size % num_threads;

	for(unsigned i = 0; i < num_threads; i++) {
		threads_data[i].origin_name = origin_name;
		threads_data[i].dest_name = dest_name;
		threads_data[i].origin_offset = bytes_per_thread * i;
		threads_data[i].start_byte = dest_file_offset + threads_data[i].origin_offset;
		threads_data[i].bytes_to_write = bytes_per_thread;
		if(i == num_threads - 1)
			threads_data[i].bytes_to_write += remainder;

		threads[i] = _CreateThread(NULL, 0, thread_file_write, threads_data + i, 0, NULL);
	}

	_WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);

	Free(threads_data);
	Free(threads);
}
