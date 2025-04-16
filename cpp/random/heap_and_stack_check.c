#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void stack_alloc(void) {
	int stack_array[10];
	for (int i = 0; i < 1; i++) {
		stack_array[i] = i;
	}
}

int *heap_alloc(int i) {
	int *heap_array = (int *)malloc(i * sizeof(int));
	if (heap_array == NULL) {
		fprintf(stderr, "Memory allocation failed :<\n");
		return NULL;
	} 
	return heap_array;
}

int *heap_realloc(int i, int *heap) {
	heap = (int *)realloc(heap, i * sizeof(int));
	if (heap == NULL) {
		fprintf(stderr, "Memory reallocation failed :<\n");
		return NULL;
	} 
	return heap;
}

int *heap_calloc(int i) {
	int *calloc_array = (int *)calloc(i, sizeof(int));
	if (calloc_array == NULL) {
		fprintf(stderr, "Memory allocation failed :<\n");
		return NULL;
	} 
	return calloc_array;
}


int main(void) {
	double a = 25.0;	
	double b = sqrt(a);

	stack_alloc();
	int *heap_array = heap_alloc(10);

	for(int i = 0; i < 10; i++) {
		printf("%d ", heap_array[i]);
	}
	printf("\n");	

	for(int i = 0; i < 10; i++) {
		int z = (int)ceil(1.666*i + 5); 	
		heap_array[i] = z%11 + z + 20;
		printf("%d ", heap_array[i]);
	}
	printf("\n");
	
	free(heap_array); // freeing the heap arr
	heap_array = NULL;

	heap_array = heap_alloc(5);
	heap_array = heap_realloc(20, heap_array);	
	

	int *calloc_array = heap_calloc(30);
	for (int i = 0; i < 30; i++) {
		calloc_array[i] = (int)(i*7 + (float)i*6.66+11/(i+1)); 	
		printf("%d ", calloc_array[i]);
	}
	printf("\n");

	free(calloc_array);
	calloc_array = NULL;

	return 0;
}

