#define _GNU_SOURCE 	// GNU-specific extensions like
			// sched_setaffinity() sched_getaffinity() 
			// these are not part of standard POSIX API

#include <sched.h>      // Set thread's CPU affinity
#include <unistd.h> 	// System-related functions like getpid()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
	((void)BUILD_BUG_ON_ZERO( \
		__same_type((arr), &(arr)[0])), \
		(sizeof(arr) / sizeof((arr)[0])) \
	)

// Converts argument into string
#define STRINGIFY__(x) #x
#define STRINGIFY(x)  STRINGIFY__(x)

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

size_t clock_rate = 0;

typedef struct {
	char   *array_pt;
	size_t  array_len;
	size_t  element_len;
} SlicedArr;

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
	rdtsc_t		rdtsc;
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

void *aligned_malloc(size_t size, size_t alignment) {

	void *ptr = NULL;
	int   res = posix_memalign(&ptr, alignment, size);

	assert(res == 0 && "Posix_memalign failed in aligned_malloc()");

	return ptr;
}

size_t align_to(size_t el, size_t alignment) {

	/* This works better for larger numbers
	const size_t remainder = el % alignment;
	const size_t misalignment = alignment - remainder;

	return el + misalignment;
	*/

	/*
	Faster implementation for powers of 2
	return (el + alignment - 1) & (~(alignment - 1));
	*/
	
	size_t res=0;
	
	while(res < el) {
		res += alignment;
	}
	return res;
}

size_t generate_pattern_reps (	
	Entry *restrict const pattern_el 
) {

	static char   *prev_pattern_name;
	
	static size_t  reps	 = 0;
	static size_t  size	 = 0;
	       size_t  base_reps = pow(2,8);
	       size_t  increment = pow(2,2);

	assert(pattern_el->text  && "pattern_el->text missing in generate_pattern_reps()");
	assert(pattern_el->name  && "pattern_el->name missing in generate_pattern_reps()");
	assert(pattern_el->size  && "pattern_el->size missing in generate_pattern_reps()");
		
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

void test_memcpy_set(int align){
	
	assert( (align == 64 || align == 8)
		&& "Incorrect align value in test_memcpy_set()");		
	
	size_t  marr = ARRAY_SIZE(tested_memcpy.arr),
		tarr = ARRAY_SIZE(entries.arr),
		rarr = ARRAY_SIZE(results.arr);
	
	assert(marr == MEMCPY_COUNT    && "tested_memcpy.arr of incorrect size in test_memcpy_set()");
	assert(tarr == TEST_COUNT      && "entries.arr of incorrect size in test_memcpy_set()");
	assert(rarr == FULL_TEST_COUNT && "results.arr of incorrect size in test_memcpy_set()");

	int idx=0, unalignment=0;
	if (align == 8)
		unalignment = 71; // making sure that the value is not divisible by 64

	char *src_txt = (char *)aligned_malloc(TEXT_MAX_SIZE + unalignment, align);
	char *dst_txt = (char *)aligned_malloc(TEXT_MAX_SIZE + unalignment, align);
	
	if (align == 8) {
		src_txt = src_txt + unalignment;
		dst_txt = dst_txt + unalignment;
	}

	assert(src_txt && "Src_txt malloc failed in test_memcpy_set()");
	assert(dst_txt && "Dst_txt malloc failed in test_memcpy_set()");

	for (unsigned int i=0; i < marr; i++) {
		for (unsigned int j=0; j < tarr; j++) {
		
			assert((uint32_t)idx <= rarr 
				&& "Overflowing results.arr index in test_memcpy_set()");
	
			Entry  *ent = &entries.arr[j];
			Result *res = &results.arr[idx];

			res->size = ent->size * ent->reps;
			if (res->size > TEXT_MAX_SIZE) {
			
				printf("Overflowing results.arr.size in test_memcpy_set()\n");
				
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
	
	assert((hsv.h >= 0 && hsv.h <= 360 && hsv.h == floor(hsv.h)) && "Invalid h in hsv_to_rgb()");	
	assert((hsv.s >= 0 && hsv.s <= 100 && hsv.s == floor(hsv.s)) && "Invalid s in hsv_to_rgb()");	
	assert((hsv.v >= 0 && hsv.v <= 100 && hsv.v == floor(hsv.v)) && "Invalid v in hsv_to_rgb()");	

	RGB rgb = {0};

	float r = 0, g = 0, b = 0,

	      s = (float)hsv.s/100, 
	      v = (float)hsv.v/100,

	      c = v * s,
	      x = c * (1 - 
	      	  fabs(
		  	fmod((hsv.h / 60.0), 2) - 1)
		  ),
	      m = v - c;


	if (hsv.h == 360)
		hsv.h = 0;

	switch (hsv.h) {
		case 0 ... 59:    r = c; g = x; b = 0; break;
		case 60 ... 119:  r = x; g = c; b = 0; break;
		case 120 ... 179: r = 0; g = c; b = x; break;
		case 180 ... 239: r = 0; g = x; b = c; break;
		case 240 ... 299: r = x; g = 0;	b = c; break;
		default:	  r = c; g = 0;	b = x;
	}
	
	float 	r__ = r + m, 
		g__ = g + m, 
		b__ = b + m;
	
	rgb.r = (int)(CLAMP(r__, 0, 1) * 255);
	rgb.g = (int)(CLAMP(g__, 0, 1) * 255);
	rgb.b = (int)(CLAMP(b__, 0, 1) * 255);

	assert((rgb.r >= 0 && rgb.r < 256) && "Invalid r in hsv_to_rgb()");	
	assert((rgb.g >= 0 && rgb.g < 256) && "Invalid g in hsv_to_rgb()");	
	assert((rgb.b >= 0 && rgb.b < 256) && "Invalid b in hsv_to_rgb()");	

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

	assert(column_len && "Missing column_len in print_column_el()");	
	assert(str	  && "Missing str in print_column_el()");	
	assert(align	  && "Missing align in print_column_el()");	

	assert((strcmp(align, "center") == 0 || strcmp(align, "left") == 0)
		&& "Incorrect align value in print_column_el()");		
	
	size_t padding_size  = (column_len - strlen(str));
	if (strcmp(align, "center") == 0) 
		padding_size = padding_size/2;

	char  *padding  = generate_symbols(padding_size, ' ');	

	char color_str[COLOR_MAX_SIZE], 
	     color_reset[] = "\033[38;2;255;255;255m";

	hsv->h = wrap(hsv->h - 1, 0, 360); 
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

	if (num == 0)
		return  1;
	
	return 	(size_t)
		(
		ceil(
		log10(num) + 1)
		);
}

 SlicedArr slice(
	size_t       max_text_size, 
	const char  *str 
) {
	size_t text_size = strlen(str);
	
	assert(text_size     && "Invalid text argument in slice()");
	assert(max_text_size && "Invalid max_text_size argument in slice()");

	/*
	SlicedArr struct elements:
		char   *array_pt;
		size_t  array_len;
		size_t  element_len;
	*/
	SlicedArr sl;
	sl.element_len      = max_text_size + 1; // Include space for null terminator	

	size_t  full_chunks = text_size / max_text_size,
		remainder   = text_size - full_chunks * max_text_size;

	sl.array_len        = align_to(text_size, max_text_size) / max_text_size;
	assert(	sl.array_len
		&& "sl.array_len calculation failed in slice()");

	sl.array_pt         = calloc(sl.array_len, sl.element_len);
	assert( sl.array_pt 
		&& "Calloc failed in slice()");

	for (size_t i=0; i < full_chunks; i++) {

		size_t  curr_el   = sl.element_len * i,
			last_char = curr_el + max_text_size;

		memcpy( sl.array_pt + 
			curr_el,
			
			str + 
			curr_el - i,

			max_text_size
			);

		sl.array_pt[last_char] = '\0'; 
	}
	
	if(remainder) {
	
		size_t  n_chunk   = sl.array_len - 1,
			last_el   = sl.element_len * n_chunk, 
			last_char = last_el + remainder,

			off_len	  = max_text_size * n_chunk; 

		memcpy( sl.array_pt +
			last_el,

			str + off_len,
			
			remainder
			);
	
		sl.array_pt[last_char] = '\0'; 
	}

	return sl;
}

void generate_result_table(const char *title__) {

	assert( title__ && "Incorrect title__ value in generate_result_table()");		
	
	size_t table_len  = 0, column_len = 0;
	struct {
		size_t memcpy;	
		size_t test;	
		size_t size;	
		size_t diff;
		size_t diff_time;
	} max = {0};	

	assert( sizeof(max)  == sizeof(size_t) * 5 
		&& "Incorrect size of struct max in generate_result_table()");
	
	size_t  column_count  = sizeof(max)/ sizeof(size_t),
	        res_size      = ARRAY_SIZE(results.arr);
	assert( res_size     == FULL_TEST_COUNT 
		&& "Incorrect size of results.arr in generate_result_table()");	
	
	if (clock_rate == 0) // Remove diff_time column hack
		column_count--;

	for (uint32_t i=0; i < res_size; i++) {
	
		const Result *res = &results.arr[i];
		
		assert(res->memcpy_name && "Res->memcpy_name missing in generate_result_table()");
		assert(res->test_name	&& "Res->test_name missing in generate_result_table()");
		assert(res->difftime	&& "Res->difftime missing in generate_result_table()");
		assert(res->size	&& "Res->size missing in generate_result_table()");

		size_t  test__ = strlen(res->test_name);
		size_t  mmcp__ = strlen(res->memcpy_name);

		if (res->difftime > max.diff)
			max.diff  = res->difftime;
		if (res->size	  > max.size) 
			max.size  = res->size;
		if (test__ 	  > max.test)
			max.test  = test__;
		if (mmcp__ 	  > max.memcpy)
			max.memcpy = mmcp__;
	}

	max.size = count_digits(max.size);
	max.diff = count_digits(max.diff);

	if (clock_rate != 0) // Remove diff_time column hack
		max.diff_time = count_digits(clock_rate) + max.diff;

	if (column_len < max.memcpy) 
		column_len = max.memcpy;
	if (column_len < max.test) 
		column_len = max.test;
	if (column_len < max.size) 
		column_len = max.size;
	if (column_len < max.diff)  
		column_len = max.diff;
	if (clock_rate != 0 && column_len < max.diff_time) // Remove diff_time column hack
		column_len = max.diff_time;

	assert((column_len && column_len != 0) && "Invalid column_len in generate_result_table()");	
	
	uint64_t column_len_pad = 2,
	         column_len_raw = column_len;
	         column_len    += column_len_pad;
	         table_len      = column_len * column_count;
	char    *line_arr       = generate_symbols(table_len, '-');

	qsort(
		results.arr,
		ARRAY_SIZE(results.arr),
		sizeof(results.arr[0]),	
		&type_comp);	

	char title[TITLE_MAX_SIZE], header[TITLE_MAX_SIZE];
	
	size_t i=0;
	for (; i < strlen(title__); i++) {
		title[i] = toupper(title__[i]);		
	} title[i] = '\0';

	sprintf(header, "%s RESULTS", title);

	const char header_align[] = "center";
	
	// h.max = 360, s.max = 100, v.max = 100
	HSV hsv = {.h = 360, .s = 100, .v = 100};

	print_column_el(table_len, header, header_align, &hsv);
	puts("");
	
	char subh_align[] = "left";
	char *subh[] = {
		"TIME      (CYCLES):",
		"TIME      (NS):",
		"SIZE:",
		"MEMCPY:",
		"TEST:"
	};

	size_t  max_rows    = 0,
		subh_count  = ARRAY_SIZE(subh);
	
	SlicedArr subh_rows[subh_count];

	for (size_t i=0; i < subh_count; i++) {	
		
		if (clock_rate == 0 && i == 1)  // Remove diff_time column hack
			continue;

		subh_rows[i]= slice(column_len_raw, subh[i]);

		assert(subh_rows[i].array_len
			&& "Invalid subh_rows[i].array_len in generate_result_table()");
		assert(subh_rows[i].element_len
			&& "Invalid subh_rows[i].element_len in generate_result_table()");
		assert(subh_rows[i].array_pt 
			&& "Invalid subh_rows[i].array_pt in generate_result_table()");

		if (subh_rows[i].array_len > max_rows) 
			max_rows = subh_rows[i].array_len;
	}
	assert(max_rows > 0 && "Max_rows not updated correctly in generate_result_table()");
	
	char   *empty  = " ";
	size_t  el_len = subh_rows[0].element_len;

	for (size_t i=0; i < max_rows; i++) {	
		for (size_t j=0; j < subh_count; j++) {

			if(clock_rate == 0 && j == 1) // Remove diff_time column hack
				continue;

			if (i < subh_rows[j].array_len) {
			
				print_column_el(
					column_len,
					
					subh_rows[j].array_pt +
					i * el_len, 
					
					subh_align,
					&hsv);
			} else {
				print_column_el(column_len, empty, subh_align, &hsv);
			}
		} puts("");
	}
	print_column_el(table_len, line_arr, header_align, &hsv);
	puts("");

	hsv.v-=20;

	for (uint32_t i=0; i < res_size; i++) {
	
		const Result *res = &results.arr[i];
	
		char diff   [TITLE_MAX_SIZE], 
		     size   [TITLE_MAX_SIZE], 
		     memcpy [TITLE_MAX_SIZE], 
		     test   [TITLE_MAX_SIZE],
		     diff_sc[TITLE_MAX_SIZE];

		strcpy(memcpy, res->memcpy_name);
		strcpy(test,   res->test_name);

		sprintf(diff,    "%zu", res->difftime);
		sprintf(size,    "%zu", res->size);
		if (clock_rate != 0) // Remove diff_time column hack
			sprintf(diff_sc, "%zu", res->difftime * 1000000000 / clock_rate);

		char disp_align[] = "left";
		print_column_el(column_len, diff,    disp_align, &hsv);
		if (clock_rate != 0) // Remove diff_time column hack
			print_column_el(column_len, diff_sc, disp_align, &hsv);
		print_column_el(column_len, size,    disp_align, &hsv);
		print_column_el(column_len, memcpy,  disp_align, &hsv);
		print_column_el(column_len, test,    disp_align, &hsv);
		puts("");
	
	}
	
	for (size_t i=0; i < subh_count; i++) {

		if (clock_rate == 0 && i == 1) // Remove diff_time column hack)
			continue;

		free(subh_rows[i].array_pt);
	}
}

int main(void) {

	cpu_set_t cpu_set; 
	size_t cpuset_size = sizeof(cpu_set);
	
	//int all_cpus = sysconf(_SC_NPROCESSORS_CONF);
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

	if (cpuid_ret.has_invariant_tsc != 0) {
		if (cpuid_ret.clock_rate != 0){
			clock_rate = cpuid_ret.clock_rate;
			printf("Clock rate: %zu, counting time in ns\n", clock_rate);
		} else {
			printf("No Clock frequency (leaf 0x15), counting time in clock cycles\n");
		}
	} else {
		printf("No invariant TSC\n");
	} puts("");

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

	size_t 	rarr = ARRAY_SIZE(results.arr);

	_Static_assert( PATTERN_COUNT        == 2,
			"Incorrect pattern count, not equal " STRINGIFY(PATTERN_COUNT));	
	_Static_assert( PATTERN_REPEAT_COUNT == 4,
			"Incorrect pattern count, not equal " STRINGIFY(PATTERN_REPEAT_COUNT));	

	uint32_t i = SINGLE_TEST_COUNT;
	for(; i < (PATTERN_REPEAT_COUNT + SINGLE_TEST_COUNT); i++) {
	
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

	uint32_t newmax = i + PATTERN_REPEAT_COUNT;
	for(; i < newmax; i++) {
	
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
	test_memcpy_set(8); // Correct alignments are 8 and 64
	
	// Print results
	generate_result_table("Unaligned");

	test_memcpy_set(64); // run all the tests again with aligned data
	
	puts("");
	generate_result_table("Aligned"); 

	return 0;
}
