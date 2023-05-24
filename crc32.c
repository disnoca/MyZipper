#include <stdio.h>
#include <windows.h>
#include "wrapper_functions.h"
#include "crc32.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

#define _1MB 1048576

#define REVERSED_POLYNOMIAL 0xEDB88320UL

#define BUFFER_SIZE 4096

/* Helper Functions */

unsigned get_num_cores() {
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}


/* 
 * The following section is taken from the zlib library.
 * CRC32 parts combination
 */

#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */

static unsigned long gf2_matrix_times(unsigned long* mat, unsigned long vec) {
    unsigned long sum;

    sum = 0;
    while (vec) {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        mat++;
    }
    return sum;
}

static void gf2_matrix_square(unsigned long* square, unsigned long* mat) {
    int n;

    for (n = 0; n < GF2_DIM; n++)
        square[n] = gf2_matrix_times(mat, mat[n]);
}

static uint32_t crc32_combine(uint32_t crc1, uint32_t crc2, uint64_t len2){
    int n;
    unsigned long row;
    unsigned long even[GF2_DIM];    /* even-power-of-two zeros operator */
    unsigned long odd[GF2_DIM];     /* odd-power-of-two zeros operator */

    /* degenerate case (also disallow negative lengths) */
    if (len2 <= 0)
        return crc1;

    /* put operator for one zero bit in odd */
    odd[0] = REVERSED_POLYNOMIAL;          /* CRC-32 polynomial */
    row = 1;
    for (n = 1; n < GF2_DIM; n++) {
        odd[n] = row;
        row <<= 1;
    }

    /* put operator for two zero bits in even */
    gf2_matrix_square(even, odd);

    /* put operator for four zero bits in odd */
    gf2_matrix_square(odd, even);

    /* apply len2 zeros to crc1 (first square will put the operator for one
       zero byte, eight zero bits, in even) */
    do {
        /* apply zeros operator for this bit of len2 */
        gf2_matrix_square(even, odd);
        if (len2 & 1)
            crc1 = gf2_matrix_times(even, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
        if (len2 == 0)
            break;

        /* another iteration of the loop with odd and even swapped */
        gf2_matrix_square(odd, even);
        if (len2 & 1)
            crc1 = gf2_matrix_times(odd, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
    } while (len2 != 0);

    /* return combined crc */
    crc1 ^= crc2;
    return crc1;
}


/* Paralel CRC32 calculation */

typedef struct {
	char* file_name;
	uint64_t start_byte;
	uint64_t bytes_to_read;
	uint32_t result;
} crc32_thread_data;

DWORD WINAPI thread_crc32(void* data) {
	crc32_thread_data* crc32td = (crc32_thread_data*) data;

  	FILE* fp = Fopen(crc32td->file_name, "rb");
	Fseek(fp, crc32td->start_byte, SEEK_SET);

	unsigned char* buffer = Malloc(BUFFER_SIZE);
	uint64_t batch_size, total_bytes_read = 0;

	uint32_t crc = 0xFFFFFFFF;

	while(total_bytes_read < crc32td->bytes_to_read) {
		batch_size = MIN(BUFFER_SIZE, crc32td->bytes_to_read - total_bytes_read);
		total_bytes_read += batch_size;

		fread(buffer, 1, batch_size, fp);
		for(unsigned i = 0; i < batch_size; i++) {
			crc ^= buffer[i];
			for(unsigned j = 0; j < 8; j++)
				crc = (crc >> 1) ^ (REVERSED_POLYNOMIAL & -(crc & 1));
		}
	}

	crc32td->result = ~crc;

	Free(buffer);
	Fclose(fp);
  	return 0;
}

uint32_t file_crc32(char* file_name, uint64_t file_size) {
	unsigned num_threads = file_size > 	_1MB ? get_num_cores() : 1;

	crc32_thread_data* threads_data = Malloc(num_threads * sizeof(crc32_thread_data));
	HANDLE* threads = Malloc(num_threads * sizeof(HANDLE));

	uint64_t bytes_per_thread = file_size / num_threads;
	unsigned remainder = file_size % num_threads;

	for(unsigned i = 0; i < num_threads; i++) {
		threads_data[i].file_name = file_name;
		threads_data[i].start_byte = bytes_per_thread * i;
		threads_data[i].bytes_to_read = bytes_per_thread;
		if(i == num_threads - 1)
			threads_data[i].bytes_to_read += remainder;

		threads[i] = _CreateThread(NULL, 0, thread_crc32, threads_data + i, 0, NULL);
	}

	_WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);

	uint32_t crc32 = threads_data[0].result;
	_CloseHandle(threads[0]);

	for(unsigned i = 1; i < num_threads; i++) {
		crc32 = crc32_combine(crc32, threads_data[i].result, threads_data[i].bytes_to_read);
		_CloseHandle(threads[i]);
	}

	Free(threads_data);
	Free(threads);
	return crc32;
}
