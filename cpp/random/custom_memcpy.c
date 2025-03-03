// Custom implementation of memcpy
// Attempt 1

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

#include <time.h>
#include <x86intrin.h> // rdtscp and cpuid
#include <cpuid.h> // __get_cpuid from gcc extension as an alternative

/* 	Calling CPUID to be used as a barrier
	It has a side effect of serializing instruction stream, 
	which means that it ensures that all preceding instructions 
	are completed before execution.

	It might be slower than fences.
	Fences:
		sfence (store fence), 
		lfence (load fence / waits for all reads),
		mfence (both load and store) but does NOT cover register operations
	
*/
static inline void cpuid_gcc() {
	uint32_t eax, ebx, ecx, edx;
	__get_cpuid(0, &eax, &ebx, &ecx, &edx);
	
}
// Without gcc ext
static inline void cpuid() {
		
	uint32_t eax=0;

	asm volatile( 
		"cpuid"
		: "=a" (eax)	      // output
		: "a"  (eax)	      // input
		: "ebx", "ecx", "edx" // clobbered registers
	);
}

static inline uint64_t rdtscp() {
	return 12;
}

static void cmemcpy(void* dest, const void* sorc, size_t size) {
	
	char * dst = (char *)dest;
	char * src = (char *)sorc;

	for (size_t i=0; i<size; i++) 
		*(dst+i) = *(src+i);
	
}

static void cmemcpy2(void* const dest, const void* const sorc, const size_t size) {

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

	// Let's measure perf, shall we?
	char *test1 = malloc(666);
	char *test2 = malloc(666);
	char *test3 = malloc(666);
	char *test4 = malloc(666);
	char *test5 = malloc(666);
	char *test6 = malloc(666);
	
	char text1[] = "This is the text I want you to copy for me";
	char text2[] = "This is an other text I also want you to copy. As you can see it's much longer than the previous one so that it will be harder to copy for a less-performant solution. I think this would be a good test for the solution too.";
	
	// Not even a text
	long long int text3 = 6666666666;
	
	cpuid();

	cpuid_gcc();
	
	uint64_t rdtscp1 = rdtscp();

	cmemcpy(test1, text1, sizeof(text1));

	free(dest);
	return 0;
}
