#include <stdint.h>
#include <stdbool.h>
#include "../../file.h"
#include "../../concurrency.h"
#include "../../crc32.h"
#include "../../wrapper_functions.h"
#include "../compression.h"

#define BUFFER_SIZE 4096

typedef struct {
	char *origin_name, *dest_name;
	uint64_t origin_offset, dest_offset;
	uint64_t num_bytes_to_write;
	uint32_t crc32;
} file_write_thread_data;

DWORD WINAPI thread_file_write(void* data) {
	file_write_thread_data* fwtd = (file_write_thread_data*) data;
	uint32_t crc32 = CRC32_INITIAL_VALUE;

  	HANDLE hOrigin = _CreateFileA(fwtd->origin_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  	HANDLE hDest = _CreateFileA(fwtd->dest_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	_SetFilePointerEx(hOrigin, (LARGE_INTEGER) {.QuadPart = fwtd->origin_offset}, NULL, FILE_BEGIN);

	unsigned char* buffer = Malloc(BUFFER_SIZE);
	DWORD batch_size;
    uint64_t total_bytes_written = 0, curr_offset = fwtd->dest_offset;

	while(total_bytes_written < fwtd->num_bytes_to_write) {
		OVERLAPPED overlapped = {0};
		overlapped.Offset = curr_offset & 0xFFFFFFFF;
		overlapped.OffsetHigh = curr_offset >> 32;

		batch_size = MIN(BUFFER_SIZE, fwtd->num_bytes_to_write - total_bytes_written);
		total_bytes_written += batch_size;
		curr_offset += batch_size;

		// Read data and write it asynchronously
		_ReadFile(hOrigin, buffer, batch_size, NULL, NULL);	
		_WriteFile(hDest, buffer, batch_size, NULL, &overlapped);

		// Calculate CRC32
		for(DWORD i = 0; i < batch_size; i++) {
			crc32 ^= buffer[i];
			for(unsigned char j = 0; j < 8; j++)
				crc32 = (crc32 >> 1) ^ (CRC32_REVERSED_POLYNOMIAL & -(crc32 & 1));
		}

		/*
		 * It's fine to "wait" here instead of continuing with the next iteration because the CRC32 calculation
		 * takes so much time that by the time it's done the write has already completed.
		*/
		_GetOverlappedResult(hDest, &overlapped, &batch_size, TRUE);
	}

	fwtd->crc32 = ~crc32;

	_CloseHandle(hOrigin);
	_CloseHandle(hDest);
	Free(buffer);
  	return 0;
}

void no_compression(file* f, char* dest_name, uint64_t dest_file_offset) {
	unsigned num_threads = f->uncompressed_size > MIN_SIZE_FOR_CONCURRENCY ? num_cores() : 1;

	file_write_thread_data* threads_data = Malloc(num_threads * sizeof(file_write_thread_data));
	HANDLE* threads = Malloc(num_threads * sizeof(HANDLE));

	uint64_t bytes_per_thread = f->uncompressed_size / num_threads;
	unsigned remainder = f->uncompressed_size % num_threads;

	for(unsigned i = 0; i < num_threads; i++) {
		threads_data[i].origin_name = f->name;
		threads_data[i].dest_name = dest_name;
		threads_data[i].origin_offset = bytes_per_thread * i;
		threads_data[i].dest_offset = dest_file_offset + threads_data[i].origin_offset;
		threads_data[i].num_bytes_to_write = bytes_per_thread;
		if(i == num_threads - 1)
			threads_data[i].num_bytes_to_write += remainder;

		threads[i] = _CreateThread(NULL, 0, thread_file_write, threads_data + i, 0, NULL);
	}

	_WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);
	
	for(unsigned i = 0; i < num_threads; i++)
		_CloseHandle(threads[i]);

	// Calculate the final CRC32 value
	uint32_t crc32 = threads_data[0].crc32;
	for(unsigned i = 1; i < num_threads; i++)
		crc32 = crc32_combine(crc32, threads_data[i].crc32, threads_data[i].num_bytes_to_write);

	f->crc32 = crc32;
	f->compressed_size = f->uncompressed_size;

	Free(threads_data);
	Free(threads);
}
