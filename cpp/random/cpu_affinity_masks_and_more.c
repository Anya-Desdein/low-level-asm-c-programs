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


int main(void) {
	
	printf("CPU_SETSIZE:\n\ta constant,\n\tmaximum number\n\tof CPUs per CPU_SET\n");
	printf("Current CPU_SETSIZE:\n\t%d bytes\n", (CPU_SETSIZE/8));

	cpu_set_t cpu_set; // this is a mask
	size_t cpuset_size = sizeof(cpu_set);
	printf("CPU_SET size:\n\t%zu bytes\n",cpuset_size);

	int all_cpus = sysconf(_SC_NPROCESSORS_CONF);
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	printf("TOTAL CPU COUNT: %d\nCPUs ONLINE: %d\n", all_cpus, online_cpus);

	CPU_ZERO(&cpu_set); // clear set and inits struct

	pid_t pid = getpid();
	printf("CURRENT PROCESS ID: %d\n", (int)pid);

	if (sched_getaffinity(pid, cpuset_size, &cpu_set) == -1) {
		perror("sched_getaffinity");
		return 1;
	}

	puts("");
	printf("Before Setting Affinity:\n");
	printf("CPU SET MASK:\n");
	for( int i=0; i<cpuset_size; i++){
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
	
	CPU_ZERO(&cpu_set);  // clear set and inits struct
	CPU_SET(2, &cpu_set);
	
	CPU_SET(3, &cpu_set); // Just to practice using this instr
	CPU_CLR(3, &cpu_set);
	
	if (sched_setaffinity(pid, cpuset_size, &cpu_set) == -1) {
		perror("sched_setaffinity");
		return 1;
	}

	if (sched_getaffinity(pid, cpuset_size, &cpu_set) == -1) {
		perror("sched_getaffinity");
		return 1;
	}

	printf("After Setting Affinity:\n");
	printf("CPU SET MASK:\n");
	for( int i=0; i<cpuset_size; i++){
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
	

	return 0;
}
