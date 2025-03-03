// Custom implementation of memcpy

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <inttypes.h>

#include <time.h>
#include <x86intrin.h> 	// rdtscp and cpuid
#include <cpuid.h> 	// __get_cpuid from gcc extension as an alternative
#include <immintrin.h>  // __rdtscp from intel intrinsics


/* 	Calling CPUID to be used as a barrier
	It has a side effect of serializing instruction stream, 
	which means that it ensures that all preceding instructions 
	are completed before execution.

	It might be slower than fences.
	Fences:
		sfence (store fence), 
		lfence (load fence / waits for all reads),
		mfence (both load and store) but does NOT cover register operations
	
	Rdtscp doesn't flush writeback buffer  	
*/
static inline void cpuid() {
		
	uint32_t eax=0;

	asm volatile( 
		"cpuid"
		: "=a" (eax)	      // output
		: "a"  (eax)	      // input
		: "ebx", "ecx", "edx" // clobbered registers
	);
}

// This evolved over time into a function that checks both rdtscp and invariant_tsc
static inline uint32_t* cpuid_gcc() {
	static uint32_t retvals[3] = {0,0,0};

	uint32_t eax, ebx, ecx, edx, max_leaf, has_rdtscp, has_invariant_tsc;
	__get_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
	max_leaf = eax;
	retvals[0] = max_leaf;
	printf("Max leaf: %" PRIu32 "\n",max_leaf);
	
	if (max_leaf <= 0x80000001)
		return retvals;

	eax = 0, ebx = 0, ecx = 0, edx = 0;
	__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
 	has_rdtscp = edx & (1 << 27);
	retvals[1] = has_rdtscp;
	printf("Has rdtscp: %" PRIu32 "\n",has_rdtscp);
	
	if (max_leaf <= 0x80000007)
		return retvals;

	eax = 0, ebx = 0, ecx = 0, edx = 0;
	__get_cpuid(0x80000007, &eax, &ebx, &ecx, &edx);
	has_invariant_tsc = edx & (1 << 8); 
	retvals[2] = has_invariant_tsc;
	printf("Has invariant tsc: %" PRIu32 "\n", has_invariant_tsc);

	return retvals;
}

static inline uint64_t rdtscp_intel() {
		
	int64_t rdtscp;
	int32_t aux;
	rdtscp = __rdtscp(&aux);

	return rdtscp;
}
static inline uint64_t rdtscp() {
		
	// edx = higher bits
	// eax = lower bits
	uint32_t eax, edx;

	asm volatile(
		"rdtscp"
		: "=a" (eax), "=d" (edx) // out
		:			 // in
		: "ecx"			 // clobbers
	);

	// Cast edx to int64_t, then bitwise shift by 32,
	// Next OR with eax
	
	// 32:32
	// edx:eax
	return (int64_t)edx << 32 | eax;
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

// Get avg clockrate
static double getavgclockrate() {

	double clockrate[66];
	FILE *fp = fopen("/proc/cpuinfo", "r");
	
	char *pos = NULL;
	char buffer[4096];
	int iter = 0;
	
	char linecon[] = "cpu MHz"; 
	char colon[] = ":";
	while(fgets(buffer, sizeof(buffer), fp) != NULL) {

		pos = strstr(buffer, linecon);
		if (pos == NULL)
			continue;
	
		pos = strstr(buffer, colon);
		if (pos == NULL)
			continue;

		char * a = pos + 1;
		clockrate[iter] = strtod(a, NULL);	
		iter ++;
	}
	
	double sum = 0;
	for (int i=0; i<iter; i++) {
		printf("Clockrate: %f\n", clockrate[i]);
		sum += clockrate[i];

	}
	double avg = sum/(double)(long long int)iter;

	return avg;
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
	
	// Ended up useless, but good learning experience
	double avgclock = getavgclockrate();	
	printf("Clockrate: %f\n", avgclock);


	// Test rdtscp and cpuid
	uint32_t *cpuid_ret = cpuid_gcc();
	if (cpuid_ret[1] == 0 || cpuid_ret[2] == 0) {
		printf("No invariant tsc or rdtscp\n");
		return 0;
	}
	

	cpuid();
	
	uint64_t rdtscp1__ = rdtscp();
	printf("RDTSCP1: %" PRIu64 "\n", rdtscp1__);

	cpuid();
	
	uint64_t rdtscp2__ = rdtscp_intel();
	printf("RDTSCP2: %" PRIu64 "\n", rdtscp2__);


	//  Now lets use those
	cpuid();

	uint64_t rdtscp1_ = rdtscp();
	cmemcpy(test1, text1, sizeof(text1));
	uint64_t rdtscp2_ = rdtscp();
	
	cpuid();

	free(dest);
	return 0;
}
