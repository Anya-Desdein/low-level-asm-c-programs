#define _GNU_SOURCE 	// GNU-specific extensions like
			// sched_setaffinity() sched_getaffinity() 
			// these are not part of standard POSIX API

#include <sched.h>      // Set thread's CPU affinity
#include <unistd.h> 	// System-related functions like getpid()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <inttypes.h>

#include <time.h>
#include <x86intrin.h> 	// rdtscp and cpuid
#include <cpuid.h> 	// __get_cpuid from gcc extension as an alternative
#include <immintrin.h>  // __rdtscp from intel intrinsics

#include <dlfcn.h> 	// dynamic linking library 

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

int main() {

	// PURPOSE OF THIS SECTION: Pin this process to only one core in order to measure performance

	
	/*
		I think that I've finally understood how cpu_set mask works
		Each bit corresponds to a certain CPU and can be 1 or 0
		
		It's arranged by their indexes (logical CPU ID)
		
		CPU 3 	CPU 2 	CPU 1 	CPU 0
		 0        1      1       0 

		In this example only CPU 2 and 1 can run the process
		
	*/	
	cpu_set_t cpu_set; // this is a mask
	size_t cpuset_size = sizeof(cpu_set);
	printf("Size of cpu_set mask: %zu bytes\n", sizeof(cpu_set));
	
	int all_cpus = sysconf(_SC_NPROCESSORS_CONF);
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	printf("Total CPU count: %d, Online: %d\n", all_cpus, online_cpus);

	CPU_ZERO(&cpu_set);		    // clear set and inits struct
	CPU_SET((online_cpus-1), &cpu_set); // assign last

	pid_t pid = getpid();
	printf("Current process ID: %d\n", (int)pid);

	char *dest = malloc(888);

	char copymecd[] = "declare p as pointer to function (pointer to function (double, float) returning pointer to void) returning pointer to const pointer to pointer to function() returning int =\n";
	size_t size = sizeof(copymecd) - 1;

	char copyme[] = "int(** const *(*p)(void*(*)(double, float)))())\n";
	size_t size2 = sizeof(copyme);


	char *dest2 = (char *)((size_t)dest + size);
	cmemcpy2(dest, &copymecd, size);
	cmemcpy(dest2, &copyme, size2);

	printf("%s", dest);
	
	// Check if rdtscp and cpuid are available
	uint32_t *cpuid_ret = cpuid_gcc();
	if (cpuid_ret[1] == 0) {
		printf("No rdtscp\n");
		return 1;
	}

	if (sched_setaffinity(pid, cpuset_size, &cpu_set) == -1) {
		perror("sched_setaffinity");
		return 1;
	}
	
	if (sched_getaffinity(pid, cpuset_size, &cpu_set) == -1) {
		perror("sched_setaffinity");
		return 1;
	}
	printf("Making sure that cpu affinity was set properly:\n");
	for (int i=0; i<cpuset_size; i++) {
		if ( (CPU_ISSET(i, &cpu_set)) ) { 
			printf("1");
		} else { 
			printf("0");
		}
		
		if (!((i+1)%4))
			printf(" ");

		if (!((i+1)%48))
			printf("\n");
	}
	printf("\n");

	// COMPARING MEMCOPY IMPLEMENTATIONS
	void * libmemcpy = dlopen("./mcpc.so", RTLD_NOW);
	void (* lmcp) (void *, void *, unsigned long long) = dlsym(libmemcpy, "so_memcopy");
	
	char *test1 = malloc(666), *test2 = malloc(666), *test3 = malloc(666), *test4 = malloc(666);

	char text1[] = "This is the text I want you to copy for me";
	char text2[] = "This is an other text I also want you to copy. As you can see it's much longer than the previous one so that it will be harder to copy for a less-performant solution. I think this would be a good test for the solution too.";
	long long int text3 = 6666666666;

	uint64_t rdtscp_v[8];
	uint64_t rtest[4];

	// Run tests
	int ctest__ = 3;
	char text__[666];
	size_t tsize__;
	char tname__[666];
	for (int i=0; i<ctest__; i++) {
		
		switch(i) {
			
			case 0:
			strcpy(text__, text1); 
			tsize__ = sizeof(text1);
			strcpy(tname__, "UNO");
			break;
			
			case 1:
			strcpy(text__, text2); 
			tsize__ = sizeof(text2);
			strcpy(tname__, "DOS");
			break;

			case 2: 
			memcpy(text__, &text3, sizeof(long long int)); 
			tsize__ = sizeof(text3);
			strcpy(tname__, "TRES");
			break;

		}
		cpuid();
	
		// Naive
		rdtscp_v[0] = rdtscp();
		cmemcpy(test1, text__, tsize__);
		rdtscp_v[1] = rdtscp();
		
		cpuid();
	
		// Butter
		rdtscp_v[2] = rdtscp();
		cmemcpy2(test2, text__, tsize__);
		rdtscp_v[3] = rdtscp();
		
		cpuid();
	
		// Libc
		rdtscp_v[4] = rdtscp();
		memcpy(test3, text__, tsize__);
		rdtscp_v[5] = rdtscp();
		
		cpuid();
		
		// Linked other
		rdtscp_v[6] = rdtscp();
		lmcp(test4, text__, tsize__);
		rdtscp_v[7] = rdtscp();
		
		cpuid();

		rtest[0] =  rdtscp_v[1] - rdtscp_v[0];
		rtest[1] =  rdtscp_v[3] - rdtscp_v[2];
		rtest[2] =  rdtscp_v[5] - rdtscp_v[4];
		rtest[3] =  rdtscp_v[7] - rdtscp_v[6];

		printf("TEST NUMERO %s:\n", tname__);
		printf("\tcmemcpy: %"	       PRIu64 "\n", rtest[0]);
		printf("\tcmemcpy2: %" 	       PRIu64 "\n", rtest[1]);
		printf("\tmemcpy (libc): %"    PRIu64 "\n", rtest[2]);
		printf("\tmemcopy (linked): %" PRIu64 "\n", rtest[3]);
	}

	free(test1);
	free(test2);
	free(test3);
	free(test4);

	free(dest);
	return 0;
}
