#include <stdlib.h>
#include <stdio.h>

char *a(void) {
	return "Raptorlake";
}

char *b(void) {
	return "Tigerlake";
}

char *c(void) {
	return "Other";
}

typedef char *cputype_t(void);

static cputype_t *resolver(void) {

	__builtin_cpu_init();

	if(__builtin_cpu_is("raptorlake"))
		return a;
	
	if(__builtin_cpu_is("tigerlake"))
		return b;
	
	return c;
}

cputype_t cputype __attribute__((ifunc("resolver")));

int main() {

	printf("CPU type: %s\n", cputype());

}
