#ifndef _COMPRESSION_H
#define _COMPRESSION_H

#include <stdio.h>

#define NO_COMPRESSION 0

/**
 * Copies data from the specified origin file to the specified destination file.
 * 
 * @param origin the file to copy data from
 * @param dest the file to copy data to
*/
void no_compression(FILE* origin, FILE* dest);

#endif
