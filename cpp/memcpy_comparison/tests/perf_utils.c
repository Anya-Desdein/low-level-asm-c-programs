#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>

#include <x86intrin.h> 	// rdtsc and cpuid
#include <cpuid.h> 	// __get_cpuid from gcc extension as an alternative
/*

IMPORTANT! Changed __get_cpuid to __cpuid
__get_cpuid is static and can't be used in DYN .so
 
They differ in that 
	__get_cpuid takes &eax ...
and 
	__cpuid takes eax ...
*/

#include <immintrin.h>  // __rdtsc from intel intrinsics

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
void cpuid() {
		
	uint32_t eax=0;

	asm volatile( 
		"cpuid"
		: "=a" (eax)	      // output
		: "a"  (eax)	      // input
		: "ebx", "ecx", "edx" // clobbered registers
	);
}


// This evolved over time into a function that checks both rdtsc and invariant_tsc
uint32_t * cpuid_gcc() {
	static uint32_t retvals[3] = {0,0,0};
	
	uint32_t eax=0, ebx=0, ecx=0, edx=0, max_leaf, has_rdtsc, has_invariant_tsc;
	__cpuid(0x80000000, eax, ebx, ecx, edx);
	max_leaf = eax;
	retvals[0] = max_leaf;
	printf("Max leaf: %" PRIu32 "\n",max_leaf);
	
	if (max_leaf <= 0x80000001)
		return retvals;

	eax = 0, ebx = 0, ecx = 0, edx = 0;
	__cpuid(0x80000001, eax, ebx, ecx, edx);
 	has_rdtsc = edx & (1 << 27);
	retvals[1] = has_rdtsc;
	printf("Has rdtsc: %" PRIu32 "\n",has_rdtsc);
	
	if (max_leaf <= 0x80000007)
		return retvals;

	eax = 0, ebx = 0, ecx = 0, edx = 0;
	__cpuid(0x80000007, eax, ebx, ecx, edx);
	has_invariant_tsc = edx & (1 << 8); 
	retvals[2] = has_invariant_tsc;
	printf("Has invariant tsc: %" PRIu32 "\n", has_invariant_tsc);

	return retvals;
}

uint64_t rdtsc_intel() {
	return __rdtsc();
}

uint64_t rdtsc() {
		
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
