#ifndef _MACROS_H
#define _MACROS_H

#include <stdint.h>
#include <windows.h>

#define MAX(a,b) \
	({ __typeof__ (a) _a = (a); \
   	__typeof__ (b) _b = (b); \
   	_a > _b ? _a : _b; })

#define MIN(a,b) \
	({ __typeof__ (a) _a = (a); \
   	__typeof__ (b) _b = (b); \
   	_a < _b ? _a : _b; })

#endif
