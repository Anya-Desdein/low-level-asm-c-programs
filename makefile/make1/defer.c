#include <stdio.h>

void dtor(int *ptr) {
	printf(" %p\n %lu\n %lx\n", ptr, ptr, ptr);
}

int main(void) {
	int __attribute__((cleanup(dtor))) x = 2137;
	return 0;
}
