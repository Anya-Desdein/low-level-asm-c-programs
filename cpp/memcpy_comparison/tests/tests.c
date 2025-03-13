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

#include <dlfcn.h> 	// dynamic linking library 

char *mallocnfill(char *pattern, size_t psize, int reps) {	
	char *text__ = (char *)malloc(psize * reps + 1);
	for (int i=0; i < reps; i++) {
		memcpy(text__ + (psize * i), pattern, psize);
	} 
	text__[psize * reps] = '\0';

	return text__;
}

int main() {
	cpu_set_t cpu_set; 
	size_t cpuset_size = sizeof(cpu_set);
	
	int all_cpus = sysconf(_SC_NPROCESSORS_CONF);
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);

	CPU_ZERO(&cpu_set);		    // clear set and inits struct
	CPU_SET((online_cpus-1), &cpu_set); // assign last

	pid_t pid = getpid();
	
	printf("CPU_SET MASK SIZE: %zu BYTES\n",    cpuset_size);
	printf("TOTAL CPU COUNT: %d, ONLINE: %d\n", all_cpus, online_cpus);
	printf("CURRENT PROCESS ID: %d\n",          (int)pid);

	void *pu = dlopen("./perf_utils.so", RTLD_NOW);
	if (!pu) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}
	
	void (*cpuid)(void) = dlsym(pu, "cpuid");
	if (!cpuid) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	uint32_t* (*cpuid_gcc)(void) = dlsym(pu, "cpuid_gcc");
	if (!cpuid_gcc) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	uint64_t (*rdtsc)(void) = dlsym(pu, "rdtsc");
	if (!rdtsc) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	uint64_t (*rdtsc_intel)(void) = dlsym(pu, "rdtsc_intel");
	if (!rdtsc_intel) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

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
	}

	// LOAD MEMCOPY IMPLEMENTATIONS
	void * f1 = dlopen("./cmemcpy.so",  RTLD_NOW);
	void * f2 = dlopen("./cmemcpy2.so", RTLD_NOW);

	void (* cmemcpy)  (void *, void *, unsigned long long) = dlsym(f1, "cmemcpy" );
	void (* cmemcpy2) (void *, void *, unsigned long long) = dlsym(f2, "cmemcpy2");

	// TEST STRINGS
	int ptests = 8;	
	char *text[ptests+3];

	text[8] = "This is the text I want you to copy for me";
	text[9] = "This is an other text I also want you to copy. As you can see it's much longer than the previous one so that it will be harder to copy for a less-performant solution. I think this would be a good test for the solution too.";
	text[10] = "6666666666";

	char pattern1[] = "as6gn%z#d668";
	char pattern2[] = "śg!@$%^63^fb";
	
	size_t pattern1_size = sizeof(pattern1) - 1;
	size_t pattern2_size = sizeof(pattern1) - 1;

	int reps;

	char *pattern = pattern1;
	size_t pattern_size = pattern1_size;
	for (int i=1; i <= ptests; i++) {
		if (i % 1) 
			reps = 300;
		if (i % 2)
			reps = 1100;
		if (i % 3)
			reps = 6000;
		if (i % 4)
			reps = 1000;
		if (i > 4) { 
			pattern = pattern2;
			pattern_size = pattern2_size;
		}

		text[i-1] = mallocnfill(pattern, pattern_size, reps);	
	}

	// INIT
	uint64_t rdtsc_v[8];
	uint64_t rtest[4];

	char * test_dest = (char *)malloc(131072);

	int ctest__ = 11;
	size_t tsize__;
	char tname__[128];
	char *tnum [] = {
		"UNO",
		"DOS",
		"TRES",
		"QUATRO",
		"CINCO",
		"SEIS",
		"SIETE",
		"OCHO",
		"NUEVE",
		"DIEZ",
		"ONCE"
	};

	for (int i=0; i<ctest__; i++) {
		
		char wut_cp[64] = "PATTERN1";
		if (i > 3) { 
			strcpy(wut_cp, "PATTERN2");
		}

		if (i == 8) { 
			strcpy(wut_cp, "SHORT STR");
		} else if (i == 9) { 
			strcpy(wut_cp, "SEMI-SHORT STR");
		} else if (i == 10) { 
			strcpy(wut_cp, "NUMBER");
		}

		tsize__ = strlen(text[i]) + 1;
		sprintf(tname__, "NUMERO %s, MEMCPY %s", tnum[i], wut_cp);
	
		cpuid();
		asm volatile("":::"memory");
	
		// Naive
		for(int j=0; j<666; j++) {
			cmemcpy(test_dest, text[i], tsize__);
		}
		
		rdtsc_v[0] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy(test_dest, text[i], tsize__);
		}
		
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[1] = rdtsc();
		
		// Butter
		for(int j=0; j<666; j++) {
			cmemcpy2(test_dest, text[i], tsize__);
		}

		rdtsc_v[2] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy2(test_dest, text[i], tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[3] = rdtsc();
		
		// Libc
		for(int j=0; j<666; j++) {
			memcpy(test_dest, text[i], tsize__);
		}

		rdtsc_v[4] = rdtsc();
		for(int j=0; j<1000; j++) {
			memcpy(test_dest, text[i], tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[5] = rdtsc();
		

		// TODO: Replace
		for(int j=0; j<666; j++) {
			cmemcpy2(test_dest, text[i], tsize__);
		}

		rdtsc_v[6] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy2(test_dest, text[i], tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[7] = rdtsc();	

		rtest[0] =  (rdtsc_v[1] - rdtsc_v[0])/1000;
		rtest[1] =  (rdtsc_v[3] - rdtsc_v[2])/1000;
		rtest[2] =  (rdtsc_v[5] - rdtsc_v[4])/1000;
		rtest[3] =  (rdtsc_v[7] - rdtsc_v[6])/1000;

		printf("%s:\n",		         tname__);
		printf("LENGHT: %zu BYTES\n",    tsize__);
		printf("\tcmemcpy: %"	       PRIu64 "\n", rtest[0]);
		printf("\tcmemcpy2: %" 	       PRIu64 "\n", rtest[1]);
		printf("\tmemcpy (libc): %"    PRIu64 "\n", rtest[2]);
		printf("\tcmemcpy3: %" PRIu64 "\n", rtest[3]);
	}

	
	free(test_dest);
	for (int i=0; i < ptests; i++) {
		free(text[i]);
	}
	return 0;
}
