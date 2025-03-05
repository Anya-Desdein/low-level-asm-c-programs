#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <stdint.h>
#include <inttypes.h>

#include <linux/limits.h>

/* This is complicated... and stolen from Linux
In C, bit-field size must be non-negative
On positive, we're turning it to -1
On 0, it stays 0 */
#define BUILD_BUG_ON_ZERO(expr) ((int)(sizeof(struct { int:(-!!(expr)); })))

/* Are two types/vars the same (ignoring qualifiers)? 
^ Stolen from linux compiler_types.h :P */

/* If two values are of the same type, return 1, else 0  */
#define __same_type(a,b) __builtin_types_compatible_p(typeof(a), typeof(b))

/* Using macros above, 
If arr[x], it will end up as
0, ARR_EL_COUNT
But if it's a pointer, it will end up as
1...
and fail at compile time.

Due to comma operator, it will ignore the first arg and
pass the second one as the result.

Without the first argument it would be
, ARR_EL_COUNT
Which, on the other hand, is incorrect syntax and wouldn't work.
*/
#define ARRAY_SIZE(arr) \
	(BUILD_BUG_ON_ZERO(\
		__same_type((arr), &(arr)[0])), \
		(sizeof(arr) / sizeof((arr)[0])) \
	)

int pleaseleave() {	
	printf("Usage:\n\toocpu list,\n\toocpu on CPU_NUM,\n\toocpu off CPU_NUM\n");
	return 1;
}

int plsplspls(int cpus) {
	printf("Argument cannot exceed TOTAL CPU COUNT(%d)\n", cpus);
	return 1;
}

int main(int argc, char *argv[]) {
	
	// Testing ARRAY_SIZE
	// int tab[12];
	// printf("%d\n", ARRAY_SIZE(tab));

	int all_cpus = sysconf(_SC_NPROCESSORS_CONF);	
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
		
	if (argc == 2 && strcmp(argv[1],"list") == 0) {
		printf("\tREMINDER: NO ONLINE FILE FOR CPU 0\n........................................................\n");
		printf("\tTOTAL CPU COUNT %d\n\tONLINE CPU COUNT %d\n", all_cpus, online_cpus);
		return 0;
	}

	if (argc != 3) {
		return pleaseleave();
	}

	int cpu_num = atoi(argv[2]);
	// Create correct path to a file managing core state (on/off)
	char str[] = "/sys/devices/system/cpu/cpu%d/online";
	char buffer[PATH_MAX];
	sprintf(buffer, str, cpu_num);

	char digit = 9;
	if (strcmp(argv[1],"on") == 0) {
		digit = '1';
	}

	if (strcmp(argv[1],"off") == 0) {
		digit = '0';

	}

	if (digit == 9) {
		return pleaseleave();
	}

	FILE *desc = fopen(buffer,"w");
	if (desc == NULL) {
		perror("fopen");
		return 1;
	}
		
	if (cpu_num == -1) {
		printf("Buffer: %s\n",buffer);
		printf("Num to write: %c\n", digit);
		printf("Pointer to file: %lld\n", (long long int)&desc);
		return 0;
	}	
	
	if (cpu_num > (all_cpus-1) || cpu_num < 1) {
		return plsplspls(all_cpus);
	}
	
	fwrite(&digit, sizeof(char), 1, desc);	
	fclose(desc);

	return 0;	
}
