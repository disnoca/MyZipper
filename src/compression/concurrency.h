#ifndef _CONCURRENCY_H
#define _CONCURRENCY_H

#define MIN(a,b) (((a)<(b))?(a):(b))

/**
 * Returns the number of cores in the system.
 * 
 * @return the number of cores in the system
*/
unsigned num_cores();

#endif