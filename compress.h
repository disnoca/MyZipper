#ifndef COMPRESS
#define COMPRESS

#include "file.h"

#define NO_COMPRESSION 0

/**
 * Loads and does not compress the specified file's data.
 * 
 * @param f the file to have its data loaded
*/
void no_compression(file* f);

#endif
