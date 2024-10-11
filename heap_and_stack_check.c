#include <stdio.h>
#include <stdlib.h>

void stack_alloc() {
	int stack_array[10];
	for (int i = 0; i < 1; i++) {
		stack_array[i] = i;
	}
}

void heap_alloc() {
	int *heap_array = (int *)malloc(10 * sizeof(int));
	if (heap_array == NULL) {
		fprintf(stderr, "Memory allocation failed :<\n");
		return;
	} 
}

int main() {
	stack_alloc();
	heap_alloc();
	return 0;
}

