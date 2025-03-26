#define _GNU_SOURCE 	// GNU-specific extensions like
			// sched_setaffinity() sched_getaffinity() 
			// these are not part of standard POSIX API

#include <sched.h>      // Set thread's CPU affinity
#include <unistd.h> 	// System-related functions like getpid()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdint.h>
#include <inttypes.h>

#include <dlfcn.h> 	// dynamic linking library 

#include "perf_utils.h"
#include "memcpy.h"

#define TEXT_MAX_SIZE  (1 << 19)
#define TITLE_MAX_SIZE (1 << 9 )

#define SINGLE_TEST_COUNT	3

#define PATTERN_COUNT		2
#define PATTERN_REPEAT_COUNT	4

#define MEMCPY_COUNT		3
#define WARMUP_COUNT		666
#define RUN_COUNT		1024	 

#define UNIQUE_TEST_COUNT	((SINGLE_TEST_COUNT) + (PATTERN_COUNT))
#define TEST_COUNT		((SINGLE_TEST_COUNT) + ((PATTERN_COUNT) * (PATTERN_REPEAT_COUNT)))
#define FULL_TEST_COUNT		(TEST_COUNT * MEMCPY_COUNT)

#define COLOR_MAX_SIZE		512

// positive value ? -1 : 0
// -1 is invalid size
#define BUILD_BUG_ON_ZERO(expr) ((int)(sizeof(struct { int:(-!!(expr)); })))

// sametype ? 1 : 0;
#define __same_type(a,b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define ARRAY_SIZE(arr) \
	(BUILD_BUG_ON_ZERO( \
		__same_type((arr), &(arr)[0])), \
		(sizeof(arr) / sizeof((arr)[0])) \
	)

// Converts argument into string
#define STRINGIFY__(x) #x
#define STRINGIFY(x)  STRINGIFY__(x)

struct {
	rdtsc_t 	rdtsc;
	rdtsc_intel_t	rdtsc_intel;
	cpuid_t		cpuid;
	cpuid_gcc_t     cpuid_gcc;
} utils;

typedef struct {
	memcpy_t func; 
	char	 name[TITLE_MAX_SIZE];
} Memcpy;

struct {
	Memcpy arr[MEMCPY_COUNT];
} tested_memcpy;

typedef struct {
	char 	 name[TITLE_MAX_SIZE];
	char 	 text[TEXT_MAX_SIZE];
	size_t 	 reps;
	size_t 	 size;
} Entry;

typedef struct {
	Entry arr[TEST_COUNT];
} Entries;
Entries entries;

typedef struct {
	char 	test_name  [TITLE_MAX_SIZE];
	char 	memcpy_name[TITLE_MAX_SIZE];
	size_t 	size;
	size_t 	difftime;
} Result;

struct {
	Result	arr[FULL_TEST_COUNT];
} results;

size_t generate_pattern_reps (	
	Entry *restrict const pattern_el 
) {

	static char   *prev_pattern_name;
	
	static size_t  reps	 = 0;
	static size_t  size	 = 0;
	       size_t  base_reps = pow(2,8);
	       size_t  increment = pow(2,2);

	if (!pattern_el->text || !pattern_el->name || !pattern_el->size) {
		printf("Entry pattern_txt incorrectly passed to generate_pattern_reps function\n");
		exit(1);
	}
	char *pattern_name = pattern_el->name;

	if (prev_pattern_name && strcmp(prev_pattern_name, pattern_name) == 0) {

		if ((reps * increment * size) > TEXT_MAX_SIZE) {
			reps = TEXT_MAX_SIZE;
		} else {
			reps = reps * increment;
		}

	} else {
		
		size = pattern_el->size;
		reps = base_reps;
		prev_pattern_name = pattern_name;
	}

	return reps;
}

void fill(
	char *restrict const buffer, 
	char *restrict const pattern,
	size_t 		     bufsize
) {
	
	const size_t divisor        = strlen(pattern);
	const size_t numberofcopies = (bufsize) / divisor;
	      size_t reminder	    = (bufsize) % divisor;
	      char  *dst_chunk      = buffer; 

	for (size_t i=0; i < numberofcopies; i++) { 
		memcpy(dst_chunk, pattern, divisor);
		
		dst_chunk += divisor;
	}

	for (size_t i=0; i < reminder; i++) {
		memcpy(dst_chunk, pattern+i, 1);
	
		dst_chunk ++;
		reminder --;
	} buffer[bufsize] = '\0';
}

size_t measure_time( 
	char  	*dst_txt,
	char	*src_txt,
	size_t 	 size,

	size_t 	 warmup_count,
	size_t 	 run_count,
	memcpy_t tested_memcpyi

) {
		size_t starttime, endtime, difftime;

		utils.cpuid();
		asm volatile("":::"memory");
		
		for(size_t i=0; i < warmup_count; i++) {
			tested_memcpyi(
				dst_txt,	
				src_txt,
				size);
		}
		
		starttime = utils.rdtsc();

		for(size_t i=0; i < warmup_count; i++) {
			tested_memcpyi(
				dst_txt,	
				src_txt,
				size);
		}
		
		utils.cpuid();
		asm volatile("":::"memory");
		
		endtime = utils.rdtsc();

		difftime = (endtime - starttime)/run_count;
		return difftime;
}

void test_memcpy_set(){
	
	size_t  marr = ARRAY_SIZE(tested_memcpy.arr),
		tarr = ARRAY_SIZE(entries.arr),
		rarr = ARRAY_SIZE(results.arr);

	if (marr != MEMCPY_COUNT) { 
		printf("tested_memcpy.arr not %d elements long\n", MEMCPY_COUNT);
		exit(1);
	}

	if (tarr != TEST_COUNT) {
		printf("entries.arr not %d elements long\n",       TEST_COUNT);
		exit(1);
	}

	if (rarr != FULL_TEST_COUNT) {
		printf("results.arr not %d elements long\n",       FULL_TEST_COUNT);
		exit(1);
	}

	printf("marr: %zu tarr: %zu, rarr: %zu\n", marr, tarr, rarr);

	int   idx 	 = 0;
	char *src_txt    = (char *)malloc(TEXT_MAX_SIZE);
	char *dst_txt 	 = (char *)malloc(TEXT_MAX_SIZE);

	for (unsigned int i=0; i < marr; i++) {
		for (unsigned int j=0; j < tarr; j++) {
			
			if (idx > rarr) {
				printf("Overflowing results.arr index\n");
				exit(1);
			}
	
			Entry  *ent = &entries.arr[j];
			Result *res = &results.arr[idx];

			res->size = ent->size * ent->reps;
			if (res->size > TEXT_MAX_SIZE) {
				printf("Overflowing results.arr.size\n");
				printf("res->size: %zu, TEXT_MAX_SIZE: %d\n", res->size, TEXT_MAX_SIZE);
				exit(1);
			}

			fill(
				src_txt,
				ent->text,
				res->size);
			
			strcpy(
				res->memcpy_name,
				tested_memcpy.arr[i].name);
			strcpy(
				res->test_name,
				ent->name);
		
			res->difftime = measure_time(
				dst_txt,
				src_txt,
				res->size,
				(size_t)(WARMUP_COUNT),
				(size_t)(RUN_COUNT),
				tested_memcpy.arr[i].func
			);
			idx ++;
		}
	}
}

char *generate_line(size_t character_count, char symbol) {

	char *line_pt = malloc(character_count+1);
	
	for (size_t i=0; i < character_count; i++) {
		line_pt[i] = symbol;
	} line_pt[character_count] = '\0';
	
	return line_pt;
}

int test_type_comp(const void *lhs_, const void *rhs_) {
	const Result *lhs = (const Result *)lhs_;
	const Result *rhs = (const Result *)rhs_;

	if (lhs->difftime == rhs->difftime)
		return 0;

	if (lhs->difftime >= rhs->difftime)
		return 1;

	return -1;
}

size_t count_digits(size_t num) {

	if (num < 0)
		return -1;
	if (num == 0)
		return  1;
	
	return 	(size_t)
		(
		ceil(
		log10(num) + 1)
		);
}

void generate_result_table() {

	size_t res_size = ARRAY_SIZE(results.arr);

	size_t max_memcpy	= 0; 
	size_t max_test		= 0; 
	size_t max_size 	= 0; 
	size_t max_difftime	= 0; 

	for (int i=0; i < res_size; i++) {
	
		const Result *res    = &results.arr[i];
		      size_t  test__ =  strlen(res->test_name);
		      size_t  mmcp__ =  strlen(res->memcpy_name);

		if (res->difftime > max_difftime)
			max_difftime = res->difftime;
		if (res->size > max_size) 
			max_size     = res->size;
		if (test__ > max_test)
			max_test     = test__;
		if (mmcp__ > max_memcpy)
			max_memcpy   = mmcp__;
	}
	
	max_size     = count_digits(max_size);
	max_difftime = count_digits(max_difftime);
	
	printf("MEM: %zu, TST: %zu, SIZ: %zu, DIF: %zu\n", max_memcpy, max_test, max_size, max_difftime);
	//size_t line_width = max_memcpy + max_test + max_size + max_difftime;
	//char *line = generate_line(line_width, '-');

	qsort(
		results.arr,
		ARRAY_SIZE(results.arr),
		sizeof(results.arr[0]),	
		&test_type_comp);
	
	printf("RESULTS\n");

	//printf("%s\n", line);
	printf("TIME: \t\tSIZE:\t\tMEMCPY_NAME:\t\tTEST_NAME:\n");

	char color[COLOR_MAX_SIZE];
	char color_end[] = "\x1b[0m";


	for (int i=0; i < res_size; i++) {

		const Result *res = &results.arr[i];
	
		int c_type = 3;
		sprintf(color, "\x1b[3%dm", c_type);

		printf("%zu  \t\t", 	res->difftime);
		printf("%zu  \t\t", 	res->size);
		printf("%s   \t\t",	res->memcpy_name);
		printf("%s   \n", 		res->test_name);
	}
}

int main(void) {

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
	
	utils.cpuid_gcc = dlsym(pu, "cpuid_gcc");
	if (!utils.cpuid_gcc) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	Cpustat cpuid_ret = utils.cpuid_gcc();
	if (cpuid_ret.has_rdtsc == 0) {
		printf("No rdtsc\n");
		return 1;
	}

	if (cpuid_ret.has_invariant_tsc == 0) {
		printf("No invariant TSC, TODO: finish this\n");
	}


	utils.cpuid = dlsym(pu, "cpuid");
	if (!utils.cpuid) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	utils.cpuid_gcc = dlsym(pu, "cpuid_gcc");
	if (!utils.cpuid_gcc) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	utils.rdtsc = dlsym(pu, "rdtsc");
	if (!utils.rdtsc) {
		printf("dlopen error: %s\n", dlerror());
		return 1;
	}

	utils.rdtsc_intel = dlsym(pu, "rdtsc_intel");
	if (!utils.rdtsc_intel) {
		printf("dlopen error: %s\n", dlerror());
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

	tested_memcpy.arr[0].func = memcpy;
	strcpy(	tested_memcpy.arr[0].name,
		"memcpy");

	// LOAD MEMCOPY IMPLEMENTATIONS
	void * f1 = dlopen("./cmemcpy.so",  RTLD_NOW);
	void * f2 = dlopen("./cmemcpy2.so", RTLD_NOW);
	
	tested_memcpy.arr[1].func = dlsym(f1, "cmemcpy");
	strcpy( tested_memcpy.arr[1].name,
		"cmemcpy");

	tested_memcpy.arr[2].func = dlsym(f2, "cmemcpy2");
	strcpy( tested_memcpy.arr[2].name,
		"cmemcpy2");

	if (ARRAY_SIZE(entries.arr) != TEST_COUNT) {
		printf("entries.arr not %d elements long\n", TEST_COUNT);
		return 1;
	}

	strcpy(
		entries.arr[0].name,
		"SHORT_STR");
	strcpy(
		entries.arr[0].text,
		"This is the text I want you to copy for me");
	
	entries.arr[0].size = strlen(entries.arr[0].text);
	entries.arr[0].reps = 1;	

	strcpy(
		entries.arr[1].name,
		"LONGER_STR");
	strcpy(
		entries.arr[1].text,
		"This is an other text I also want you to copy. "
		"As you can see it's much longer than the previous "
		"one so that it will be harder to copy for a "
		"less-performant solution. I think this would be a "
		"good test for the solution too.");

	entries.arr[1].size = strlen(entries.arr[1].text);
	entries.arr[1].reps = 1;	

	strcpy(
		entries.arr[2].name,
		"66666666");
	strcpy(
		entries.arr[2].text,
		"6");

	entries.arr[2].size = strlen(entries.arr[2].text);
	entries.arr[2].reps = 8;	

	size_t rarr = ARRAY_SIZE(results.arr), \ 
	tarr = ARRAY_SIZE(entries.arr), \
	result_size;

	_Static_assert( PATTERN_COUNT        == 2,
			"Incorrect pattern count, not equal " STRINGIFY(PATTERN_COUNT));	
	_Static_assert( PATTERN_REPEAT_COUNT == 4,
			"Incorrect pattern count, not equal " STRINGIFY(PATTERN_REPEAT_COUNT));	

	int i = SINGLE_TEST_COUNT;
	for(i; i < (PATTERN_REPEAT_COUNT + SINGLE_TEST_COUNT); i++) {
	
		strcpy(
			entries.arr[i].name,
			"PATTERN 0");
		strcpy(
			entries.arr[i].text,
			"śg!@$%^63^fb");
	
		entries.arr[i].size = strlen(entries.arr[i].text);

	
		strcpy(
			entries.arr[i].text,
			"as6gn%z#d668");

		entries.arr[i].size = strlen(entries.arr[i].text);
		entries.arr[i].reps = generate_pattern_reps(&entries.arr[i]);
	}

	int newmax = i + PATTERN_REPEAT_COUNT;
	for(i; i < newmax; i++) {
	
		strcpy(
			entries.arr[i].name,
			"PATTERN 1");
		
		strcpy(
			entries.arr[i].text,
			"as6gn%z#d668");

		entries.arr[i].size = strlen(entries.arr[i].text);
		entries.arr[i].reps = generate_pattern_reps(&entries.arr[i]);
	}
	
	if (rarr != FULL_TEST_COUNT) {
		printf("results.arr not %d elements long\n", FULL_TEST_COUNT);
		return 1;
	}

	// Base structs generated, proceeding to test memcpy set 
	test_memcpy_set();
	
	printf("Test count: %d\nResults struct size: %zu\n", TEST_COUNT, sizeof(results));

	// Print results
	generate_result_table();

	return 0;
}
