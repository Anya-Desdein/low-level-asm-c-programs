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
	char text1[] = "This is the text I want you to copy for me";
	char text2[] = "This is an other text I also want you to copy. As you can see it's much longer than the previous one so that it will be harder to copy for a less-performant solution. I think this would be a good test for the solution too.";
	long long int text3 = 6666666666;

	char pattern1[] = "as6gn%z#d668";
	size_t pattern1_size = sizeof(pattern1) - 1;

	int reps300 = 300;
	int reps10000 = 10000;
	
	char *text4 = (char *)malloc(pattern1_size * reps300 + 1);
	char *text5 = (char *)malloc(pattern1_size * reps10000 + 1);

	for (int i=0; i < reps300; i++) {
		memcpy( text4 + (pattern1_size * i), pattern1, pattern1_size);
	} text4[pattern1_size * reps300] = '\0';
	

	for (int i=0; i < reps10000; i++) {
		memcpy( text5 + (pattern1_size * i), pattern1, pattern1_size);
	} text5[pattern1_size * reps10000] = '\0';

	// INIT
	uint64_t rdtsc_v[8];
	uint64_t rtest[4];

	char * test_dest = (char *)malloc(32768);
	char * text;
	int ctest__ = 5;
	size_t tsize__;
	char tname__[128];
	for (int i=0; i<ctest__; i++) {
		
		switch(i) {
			
			case 0:
			text = text1;
			tsize__ = sizeof(text1);
			strcpy(tname__, "UNO");
			break;
			
			case 1:
			text = text2;
			tsize__ = sizeof(text2);
			strcpy(tname__, "DOS");
			break;

			case 2: 
			text = (char *)&text3;
			tsize__ = sizeof(text3);
			strcpy(tname__, "TRES");
			break;
		
			case 3: 
			text = text4;
			tsize__ = sizeof(text4);
			strcpy(tname__, "CUATRO");
			break;
	
			case 4: 
			text = text5;
			tsize__ = sizeof(text5);
			strcpy(tname__, "CINCO");
			break;
		}
	
		cpuid();
		asm volatile("":::"memory");
	
		// Naive
		rdtsc_v[0] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy(test_dest, text, tsize__);
		}
		cpuid();
		asm volatile("":::"memory");

		// Butter
		rdtsc_v[1] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy2(test_dest, text, tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		
		// Libc
		rdtsc_v[2] = rdtsc();
		for(int j=0; j<1000; j++) {
			memcpy(test_dest, text, tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		
		// TODO: Replace
		rdtsc_v[3] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy2(test_dest, text, tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[4] = rdtsc();	

		rtest[0] =  (rdtsc_v[1] - rdtsc_v[0])/1000;
		rtest[1] =  (rdtsc_v[2] - rdtsc_v[1])/1000;
		rtest[2] =  (rdtsc_v[3] - rdtsc_v[2])/1000;
		rtest[3] =  (rdtsc_v[4] - rdtsc_v[3])/1000;

		printf("TEST NUMERO %s:\n",    tname__);
		printf("\tcmemcpy: %"	       PRIu64 "\n", rtest[0]);
		printf("\tcmemcpy2: %" 	       PRIu64 "\n", rtest[1]);
		printf("\tmemcpy (libc): %"    PRIu64 "\n", rtest[2]);
		printf("\tmemcopy (linked): %" PRIu64 "\n", rtest[3]);
	}

	
	free(test_dest);
	
	free(text4);
	free(text5);
	return 0;
}
