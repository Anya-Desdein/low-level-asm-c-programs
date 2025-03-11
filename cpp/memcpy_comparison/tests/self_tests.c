#define _GNU_SOURCE 	// GNU-specific extensions like
			// sched_setaffinity() sched_getaffinity() 
			// these are not part of standard POSIX API

#include <sched.h>      // Set thread's CPU affinity
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <inttypes.h>

#include <dlfcn.h> 	// dynamic linking library 
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
	
	printf("CPU_SET MASK SIZE: %zu BYTES\n",    cpuset_size);
	printf("TOTAL CPU COUNT: %d, ONLINE: %d\n", all_cpus, online_cpus);
	printf("CURRENT PROCESS ID: %d\n",	    (int)pid);
	

	void *pu = dlopen("./perf_utils.so",  RTLD_NOW);
	if (!pu) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	uint32_t* (*cpuid_gcc)(void) = dlsym(pu, "cpuid_gcc" );
	if (!cpuid_gcc) {
		printf("dlsym error: %s\n", dlerror());
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
	
	size_t size  = sizeof(copyme1);
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
	
	cmemcpy (dest,          &copyme1,  size);
	cmemcpy2(dest+size-1,   &copyme2,  size2 );

	printf("%s", dest);
	free(dest);
	return 0;
}
