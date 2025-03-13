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
	char pattern2[] = "śg!@$%^63^fb";
	
	size_t pattern1_size = sizeof(pattern1) - 1;
	size_t pattern2_size = sizeof(pattern1) - 1;

	int reps300 = 300;
	int reps1100 = 1100;
	int reps6000 = 6000;
	int reps10000 = 10000;
	
	char *text4 = (char *)malloc(pattern1_size * reps300 + 1);
	char *text5 = (char *)malloc(pattern1_size * reps1100 + 1);
	char *text6 = (char *)malloc(pattern1_size * reps6000 + 1);
	char *text7 = (char *)malloc(pattern1_size * reps10000 + 1);
	
	char *text4a = (char *)malloc(pattern2_size * reps300 + 1);
	char *text5a = (char *)malloc(pattern2_size * reps1100 + 1);
	char *text6a = (char *)malloc(pattern2_size * reps6000 + 1);
	char *text7a = (char *)malloc(pattern2_size * reps10000 + 1);

	for (int i=0; i < reps300; i++) {
		memcpy( text4 +  (pattern1_size * i), pattern1, pattern1_size);
		memcpy( text4a + (pattern2_size * i), pattern2, pattern2_size);
	} 
	text4[ pattern1_size * reps300] = '\0';
	text4a[pattern2_size * reps300] = '\0';

	for (int i=0; i < reps1100; i++) {
		memcpy( text5 +  (pattern1_size * i), pattern1, pattern1_size);
		memcpy( text5a + (pattern2_size * i), pattern2, pattern2_size);
	} 
	text5[ pattern1_size * reps1100] = '\0';
	text5a[pattern2_size * reps1100] = '\0';
	
	for (int i=0; i < reps6000; i++) {
		memcpy( text6 +  (pattern1_size * i), pattern1, pattern1_size);
		memcpy( text6a + (pattern2_size * i), pattern2, pattern2_size);
	} 
	text6[ pattern1_size * reps6000] = '\0';
	text6a[pattern2_size * reps6000] = '\0';

	for (int i=0; i < reps10000; i++) {
		memcpy( text7 +  (pattern1_size * i), pattern1, pattern1_size);
		memcpy( text7a + (pattern2_size * i), pattern2, pattern2_size);
	} 
	text7[ pattern1_size * reps10000] = '\0';
	text7a[pattern2_size * reps10000] = '\0';

	// INIT
	uint64_t rdtsc_v[8];
	uint64_t rtest[4];

	char * test_dest = (char *)malloc(131072);
	char * text;

	int ctest__ = 11;
	size_t tsize__;
	char tname__[128];
	for (int i=0; i<ctest__; i++) {
		
		switch(i) {
			
			case 0:
			text = text1;
			tsize__ = sizeof(text1);
			strcpy(tname__, "NUMERO UNO, COPY STRING");
			break;
			
			case 1:
			text = text2;
			tsize__ = sizeof(text2);
			strcpy(tname__, "NUMERO DOS, COPY LONGER STRING");
			break;

			case 2: 
			text = (char *)&text3;
			tsize__ = sizeof(long long int);
			strcpy(tname__, "NUMERO TRES, COPY LONG LONG INT");
			break;
		
			case 3: 
			text = text4;
			tsize__ = strlen(text4) + 1;
			strcpy(tname__, "NUMERO CUATRO, COPY PATTERN1");
			break;
	
			case 4: 
			text = text4a;
			tsize__ = strlen(text4a) + 1;
			strcpy(tname__, "NUMERO CINCO, COPY PATTERN2");
			break;

			case 5: 
			text = text5;
			tsize__ = strlen(text5) + 1;
			strcpy(tname__, "NUMERO SEIS, COPY PATTERN1");
			break;
			
			case 6: 
			text = text5a;
			tsize__ = strlen(text5a) + 1;
			strcpy(tname__, "NUMERO SIETE, COPY PATTERN2");
			break;
			
			case 7: 
			text = text6;
			tsize__ = strlen(text6) + 1;
			strcpy(tname__, "NUMERO OCHO, COPY PATTERN1");
			break;
			
			case 8: 
			text = text6a;
			tsize__ = strlen(text6a) + 1;
			strcpy(tname__, "NUMERO NUEVE, COPY PATTERN2");
			break;

			case 9: 
			text = text7;
			tsize__ = strlen(text7) + 1;
			strcpy(tname__, "NUMERO DIEZ, COPY PATTERN1");
			break;

			case 10: 
			text = text7a;
			tsize__ = strlen(text7a) + 1;
			strcpy(tname__, "NUMERO ONCE, COPY PATTERN2");
			break;

		}
	
		cpuid();
		asm volatile("":::"memory");
	
		// Naive
		for(int j=0; j<666; j++) {
			cmemcpy(test_dest, text, tsize__);
		}
		
		rdtsc_v[0] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy(test_dest, text, tsize__);
		}
		
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[1] = rdtsc();
		
		// Butter
		for(int j=0; j<666; j++) {
			cmemcpy2(test_dest, text, tsize__);
		}

		rdtsc_v[2] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy2(test_dest, text, tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[3] = rdtsc();
		
		// Libc
		for(int j=0; j<666; j++) {
			memcpy(test_dest, text, tsize__);
		}

		rdtsc_v[4] = rdtsc();
		for(int j=0; j<1000; j++) {
			memcpy(test_dest, text, tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[5] = rdtsc();
		

		// TODO: Replace
		for(int j=0; j<666; j++) {
			cmemcpy2(test_dest, text, tsize__);
		}

		rdtsc_v[6] = rdtsc();
		for(int j=0; j<1000; j++) {
			cmemcpy2(test_dest, text, tsize__);
		}
		cpuid();
		asm volatile("":::"memory");
		rdtsc_v[7] = rdtsc();	

		rtest[0] =  (rdtsc_v[1] - rdtsc_v[0])/1000;
		rtest[1] =  (rdtsc_v[3] - rdtsc_v[2])/1000;
		rtest[2] =  (rdtsc_v[5] - rdtsc_v[4])/1000;
		rtest[3] =  (rdtsc_v[7] - rdtsc_v[6])/1000;

		printf("%s:\n",		         tname__);
		printf("Of lenght: %zu bytes\n", tsize__);
		printf("\tcmemcpy: %"	       PRIu64 "\n", rtest[0]);
		printf("\tcmemcpy2: %" 	       PRIu64 "\n", rtest[1]);
		printf("\tmemcpy (libc): %"    PRIu64 "\n", rtest[2]);
		printf("\tcmemcpy3: %" PRIu64 "\n", rtest[3]);
	}

	
	free(test_dest);
	
	free(text4);
	free(text5);
	return 0;
}
