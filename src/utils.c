#include <string.h>
#include "utils.h"

void replace_char(char* str, char find, char replace) {
	char* current_pos = strchr(str, find);
	while(current_pos) {
		*current_pos = replace;
		current_pos = strchr(current_pos, find);
	}
}