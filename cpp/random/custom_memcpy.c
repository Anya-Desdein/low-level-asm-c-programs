// Custom implementation of memcpy
// Attempt 1

#include <stdio.h>
#include <stdlib.h>

void cmemcpy(void* dest, const void* sorc, size_t size) {
	
	char * dst = (char *)dest;
	char * src = (char *)sorc;

	for (size_t i=0; i<size; i++) 
		*(dst+i) = *(src+i);
	
}

void cmemcpy2(void* const dest, const void* const sorc, const size_t size) {

	const size_t divisor      = sizeof(long long int);
	const size_t numberofints = size/divisor;
	      size_t reminder     = size % divisor;

	/* Copy 64 bit chunks */
	long long int * dst_u64 = (long long int *)dest;
	long long int * src_u64 = (long long int *)sorc;
	for (int i = 0; i < numberofints; i++) {
		*dst_u64 = *src_u64;
		++dst_u64;
		++src_u64;
	}

	/* Copy reminder */
	char * dst = (char *)dst_u64;
	char * src = (char *)src_u64;
	while (reminder) {
		*dst = *src;
		++dst;
		++src;

		--reminder;
	}
}


int main() {
	char *dest = malloc(888);

	char copymecd[] = "declare p as pointer to function (pointer to function (double, float) returning pointer to void) returning pointer to const pointer to pointer to function() returning int\n";
	size_t size = sizeof(copymecd) - 1;

	char copyme[] = "int(** const *(*p)(void*(*)(double, float)))())\n";
	size_t size2 = sizeof(copyme);


	char *dest2 = (char *)((size_t)dest + size);
	cmemcpy2(dest, &copymecd, size);
	cmemcpy(dest2, &copyme, size2);

	printf("%s", dest);
	return 0;
}
