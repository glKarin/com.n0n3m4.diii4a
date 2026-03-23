#include <stdlib.h>
#include <stdint.h>
#include <mach-o/dyld.h>

int main(int argn, const char** argv)
{
	char full[16384];
	char path[16384];
	uint32_t size = sizeof(path);
	int i;
	_NSGetExecutablePath(path, &size);
	for (i=strlen(path)-1; i>=0; i--)
		if (path[i] == '/') {
			path[i] = 0;
			break;
		}
	if (!i) strcpy(path, ".");
	sprintf(full, "\"%s/doom3\" +set r_fullscreen 0", path);
	system(full);
	return 0;
}
