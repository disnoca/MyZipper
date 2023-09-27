#include <windows.h>
#include "concurrency.h"

static DWORD _num_cores;

DWORD num_cores() {
	if (_num_cores == 0) {
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		_num_cores = sysinfo.dwNumberOfProcessors;
	}
	
	return _num_cores;
}