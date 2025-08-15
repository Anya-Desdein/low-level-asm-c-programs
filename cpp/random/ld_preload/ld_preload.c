#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

void *mymalloc(size_t size) {
	
	write(1, "malloc will be called\n", 22);
	static void *(*real_malloc)(size_t) = NULL;
	real_malloc = dlsym(RTLD_NEXT, "malloc");

	size_t val = real_malloc(size);
	write(1, "malloc was be called\n", 22);
	return val;
}
