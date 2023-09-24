 #ifndef _MACROS_H
 #define _MACROS_H

 #define MAX(a,b) \
	({ __typeof__ (a) _a = (a); \
    	__typeof__ (b) _b = (b); \
    	_a > _b ? _a : _b; })

 #define MIN(a,b) \
	({ __typeof__ (a) _a = (a); \
    	__typeof__ (b) _b = (b); \
    	_a < _b ? _a : _b; })

void replace_char(char* str, char find, char replace);

#endif
