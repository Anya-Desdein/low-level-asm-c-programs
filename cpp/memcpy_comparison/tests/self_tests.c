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
#include <x86intrin.h> 	// rdtsc and cpuid
#include <cpuid.h> 	// __get_cpuid from gcc extension as an alternative
#include <immintrin.h>  // __rdtsc from intel intrinsics

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

// This evolved over time into a function that checks both rdtsc and invariant_tsc
static inline uint32_t* cpuid_gcc() {
	static uint32_t retvals[3] = {0,0,0};

	uint32_t eax, ebx, ecx, edx, max_leaf, has_rdtsc, has_invariant_tsc;
	__get_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
	max_leaf = eax;
	retvals[0] = max_leaf;
	printf("Max leaf: %" PRIu32 "\n",max_leaf);
	
	if (max_leaf <= 0x80000001)
		return retvals;

	eax = 0, ebx = 0, ecx = 0, edx = 0;
	__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
 	has_rdtsc = edx & (1 << 27);
	retvals[1] = has_rdtsc;
	printf("Has rdtsc: %" PRIu32 "\n",has_rdtsc);
	
	if (max_leaf <= 0x80000007)
		return retvals;

	eax = 0, ebx = 0, ecx = 0, edx = 0;
	__get_cpuid(0x80000007, &eax, &ebx, &ecx, &edx);
	has_invariant_tsc = edx & (1 << 8); 
	retvals[2] = has_invariant_tsc;
	printf("Has invariant tsc: %" PRIu32 "\n", has_invariant_tsc);

	return retvals;
}

static inline uint64_t rdtsc_intel() {
	return __rdtsc();
}
static inline uint64_t rdtsc() {
		
	// edx = higher bits
	// eax = lower bits
	uint32_t eax, edx;

	asm volatile(
		"rdtsc"
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
	
	int all_cpus    = sysconf(_SC_NPROCESSORS_CONF);
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);

	CPU_ZERO( &cpu_set);		      // clear set and inits struct
	CPU_SET ( (online_cpus-1), &cpu_set); // assign last

	pid_t pid = getpid();
	
	printf("Size of cpu_set mask: %zu bytes\n", sizeof(cpu_set));
	printf("Total CPU count: %d, Online: %d\n", all_cpus, online_cpus);
	printf("Current process ID: %d\n",	    (int)pid);
	
	// Check if rdtsc and cpuid are available
	uint32_t *cpuid_ret = cpuid_gcc();
	if (cpuid_ret[1] == 0) {
		printf("No rdtsc\n");
		return 1;
	}

	if (cpuid_ret[2] == 0) {
		printf("No invariant TSC, unused for now\n");
	} 

	if (sched_setaffinity(pid, cpuset_size, &cpu_set) == -1) {
		perror("sched_setaffinity");
		return 1;
	}	
	
	if (sched_getaffinity(pid, cpuset_size, &cpu_set) == -1) {
		perror("sched_setaffinity");
		return 1;
	} printf("Making sure that cpu affinity was set properly:\n");
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
	} printf("\n");

	// Test linkage
	char copyme1[] = "declare p as pointer to function (pointer to function (double, float) returning pointer to void) returning pointer to const pointer to pointer to function() returning int =\n";
	char copyme2[] = "int(** const *(*p)(void*(*)(double, float)))())\n";
	
	size_t size = sizeof(copyme1);
	size_t size2 = sizeof(copyme2);

	void *f1 = dlopen("./cmemcpy.so",  RTLD_NOW);
	void *f2 = dlopen("./cmemcpy2.so", RTLD_NOW);
	if (!f1 || !f2) {
		perror("dlopen");
		return 1;
	}

	void (* cmemcpy)  (void *, void *, unsigned long long) = dlsym(f1, "cmemcpy" );
	void (* cmemcpy2) (void *, void *, unsigned long long) = dlsym(f2, "cmemcpy2");
	if (!cmemcpy || !cmemcpy2) {
		perror("dlsym");
		return 1;
	}


	char *dest = (char *)malloc(2048);
	
	cmemcpy (dest,        &copyme1,  size);
	cmemcpy2(dest+size-1,   &copyme2,  size2 );

	printf("%s", dest);
	free(dest);
	return 0;
}
