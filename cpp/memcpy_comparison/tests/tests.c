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
#include <assert.h>

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

typedef struct {
	int r;
	int g;
	int b;
} RGB;

typedef struct {
	int h;
	int s;
	int v;
} HSV;

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

int wrap(int value, int min, int max) {
	
	int range  =   max   - min,
	    result = ((value - min) % range + range) % range + min;

	return result;
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

	//printf("marr: %zu tarr: %zu, rarr: %zu\n", marr, tarr, rarr);

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
				
				printf("res->size: %zu, TEXT_MAX_SIZE: %d\n",
				res->size,
				TEXT_MAX_SIZE);
				
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

char *generate_symbols(size_t character_count, char symbol) {

	char *line_pt = malloc(character_count+1);
	
	for (size_t i=0; i < character_count; i++) {
		line_pt[i] = symbol;
	} line_pt[character_count] = '\0';
	
	return line_pt;
}

RGB hsv_to_rgb(HSV hsv) {
	
	assert((hsv.h >= 0 && hsv.h <= 360 && hsv.h == floor(hsv.h)) && "Error in hsv_to_rgb: invalid h");	
	assert((hsv.s >= 0 && hsv.s <= 100 && hsv.s == floor(hsv.s)) && "Error in hsv_to_rgb: invalid s");	
	assert((hsv.v >= 0 && hsv.v <= 100 && hsv.v == floor(hsv.v)) && "Error in hsv_to_rgb: invalid v");	

	RGB rgb = {0};
	
	float h = (float)hsv.h/360,
	      s = (float)hsv.s/100, 
	      v = (float)hsv.v/100,
	      r = 0,
	      g = 0, 
	      b = 0;
	
	float i = floor(h *  6),
	      f =       h *  6 - i,
	      p =       v * (1 - f * s),
	      q =       v * (1 - f * s),
	      t =       v * (1 - (1 - f) * s);

	switch((int)i % 6) {
		case 0: r = v; g = t; b = p; break;
		case 1:	r = q; g = v; b = p; break;
		case 2:	r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		case 5: r = v; g = p; b = q; break;
	}

	rgb.r = (int)(r * 255);
	rgb.g = (int)(g * 255);
	rgb.b = (int)(b * 255);

	assert((rgb.r >= 0 && rgb.r < 256) && "Error in hsv_to_rgb: invalid r");	
	assert((rgb.g >= 0 && rgb.g < 256) && "Error in hsv_to_rgb: invalid g");	
	assert((rgb.b >= 0 && rgb.b < 256) && "Error in hsv_to_rgb: invalid b");	

	return rgb;
}

/* 
Fn to print element of a column
ARGS: 
	column length,
	pointer to string you want to print,
	alignment (center, left);
This will NOT automatically include NEWLINE 
So that you can have multi-column out
*/
void print_column_el(size_t column_len, char *str, char *align, HSV *hsv) {

	assert(column_len && "Error in print_column_el: missing argument column_len");	
	assert(str	  && "Error in print_column_el: missing argument str");	
	assert(align	  && "Error in print_column_el: missing argument align");	

	assert((align != "center" && align != "left")
		&& "Error in print_column_el: incorrect argument align");		
	
	size_t padding_size  = (column_len - strlen(str));
	if (strcmp(align, "center") == 0) 
		padding_size = padding_size/2;

	char  *padding  = generate_symbols(padding_size, ' ');	

	char color_str[COLOR_MAX_SIZE], 
	     color_reset[] = "\033[38;2;255;255;255m";

	hsv->h = wrap(hsv->h - 5, 0, 360); 
	RGB rgb = hsv_to_rgb(*hsv);
	
	sprintf(color_str, 
		"\033[38;2;%d;%d;%dm", 
		rgb.r, 
		rgb.g, 
		rgb.b);

	if (strcmp(align, "center") == 0) 
		printf("%s%s%s%s%s", 
			color_str, 
			padding, 
			str, 
			padding, 
			color_reset);
		
	if (strcmp(align, "left") == 0) 
		printf("%s%s%s%s", 
			color_str, 
			str, 
			padding, 
			color_reset);
}

int type_comp(const void *lhs_, const void *rhs_) {
	const Result *lhs = (const Result *)lhs_;
	const Result *rhs = (const Result *)rhs_;

	if (strcmp(lhs->test_name, rhs->test_name) == 0) {

		if (lhs->difftime == rhs->difftime)
			return 0;

		if (lhs->difftime >= rhs->difftime)
			return 1;

		return -1;
	}

	if (strcmp(lhs->test_name, rhs->test_name) > 0)
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

	size_t table_len  = 0, column_len = 0;
	struct {
		size_t memcpy;	
		size_t test;	
		size_t size;	
		size_t diff;	
	} max = {0};
	assert( sizeof(max) == sizeof(size_t) * 4 && "Error in max struct: incorrect size");
	
	size_t column_count = sizeof(max)/ sizeof(size_t);

	size_t res_size = ARRAY_SIZE(results.arr);
	assert(res_size == FULL_TEST_COUNT && "Error in results.arr: incorrect size");	

	for (int i=0; i < res_size; i++) {
	
		const Result *res = &results.arr[i];
			
		if (!res->memcpy_name || !res->test_name || !res->difftime || !res->size)
			exit(1);

		size_t  test__ =  strlen(res->test_name);
		size_t  mmcp__ =  strlen(res->memcpy_name);

		if (res->difftime > max.diff)
			max.diff     = res->difftime;
		if (res->size	  > max.size) 
			max.size     = res->size;
		if (test__ 	  > max.test)
			max.test     = test__;
		if (mmcp__ 	  > max.memcpy)
			max.memcpy   = mmcp__;
	}

	max.size = count_digits(max.size);
	max.diff = count_digits(max.diff);

	if (column_len < max.memcpy) 
		column_len = max.memcpy;
	if (column_len < max.test) 
		column_len = max.test;
	if (column_len < max.size) 
		column_len = max.size;
	if (column_len < max.diff)  
		column_len = max.diff;
	
	       column_len += 2; // set padding between columns
	       table_len   = column_len * column_count;
	char  *line_arr    = generate_symbols(table_len, '-');

	qsort(
		results.arr,
		ARRAY_SIZE(results.arr),
		sizeof(results.arr[0]),	
		&type_comp);
	
	char header[]       = "RESULTS";
	char header_align[] = "center";
	
	// h.max = 360, s.max = 100, v.max = 100
	HSV hsv = {.h = 360, .s = 100, .v = 100};
	print_column_el(table_len, header, header_align, &hsv);
	puts("");

	if (!column_len || column_len == 0)
		return;
	
	char subh_align[] = "left";
	char *subh[] = {
		"TIME:",
		"SIZE:",
		"MEMCPY:",
		"TEST:"
	};
	size_t subh_size = ARRAY_SIZE(subh);

	for (size_t i=0; i < subh_size; i++)
		print_column_el(column_len, subh[i], subh_align, &hsv);
	puts("");
	print_column_el(table_len, line_arr, header_align, &hsv);
	puts("");

	for (int i=0; i < res_size; i++) {

		const Result *res = &results.arr[i];
	
		char diff[TITLE_MAX_SIZE], size[TITLE_MAX_SIZE], memcpy[TITLE_MAX_SIZE], test[TITLE_MAX_SIZE];

		strcpy(memcpy, res->memcpy_name);
		strcpy(test,   res->test_name);

		sprintf(diff, "%zu", res->difftime);
		sprintf(size, "%zu", res->size);
		
		char disp_align[] = "left";
		print_column_el(column_len, diff,   disp_align, &hsv);
		print_column_el(column_len, size,   disp_align, &hsv);
		print_column_el(column_len, memcpy, disp_align, &hsv);
		print_column_el(column_len, test,   disp_align, &hsv);
		puts("");
	
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
	
	//printf("CPU_SET MASK SIZE: %zu BYTES\n",    cpuset_size);
	//printf("TOTAL CPU COUNT: %d, ONLINE: %d\n", all_cpus, online_cpus);
	//printf("CURRENT PROCESS ID: %d\n",          (int)pid);

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
	
	// Print results
	generate_result_table();

	return 0;
}
