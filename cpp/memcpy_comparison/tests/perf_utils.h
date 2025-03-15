#pragma once
#include <stdint.h>

typedef struct{
	uint32_t max_leaf;
	uint32_t has_rdtsc;
	uint32_t has_invariant_tsc;	
} Cpustat;

typedef void     (*cpuid_t)      (void);
typedef uint64_t (*rdtsc_t)      (void);
typedef uint64_t (*rdtsc_intel_t)(void);
typedef Cpustat  (*cpuid_gcc_t)  (void);
