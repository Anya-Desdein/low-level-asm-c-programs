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
	int tab[12];
	printf("%d\n", ARRAY_SIZE(tab));

	if (argc < 2 || argc > 3) {
		return pleaseleave();
	}

	int all_cpus = sysconf(_SC_NPROCESSORS_CONF);		
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
		
	if (strcmp(argv[1],"list") == 0) {
		printf("\tTOTAL CPU COUNT %d\n\tONLINE CPU COUNT %d\n", all_cpus, online_cpus);
		return 0;
	}

	if (argc < 3) {
		return pleaseleave();
	}

	// Create correct path to a file managing core state (on/off)
	int cpu_num = atoi(argv[2]) - 1;

	char str[] = "/sys/devices/system/cpu/cpu%d/online";
	char buffer[PATH_MAX];

	sprintf(buffer, str, cpu_num);
	// printf("Path to file being edited: %s\n", buffer);

	if (strcmp(argv[1],"on") == 0) {

		if (cpu_num > all_cpus) {
			return plsplspls(all_cpus);
		}
	
		if (cpu_num != 0) { 
	
			FILE *desc = fopen(buffer,"w");
			if (desc == NULL) {
				perror("fopen");
				return 1;
			}
			
			char digit = '1';
			fwrite(&digit, sizeof(char), 1, desc);	
			fclose(desc);
		}	
		return 0;
	}
	
	if (strcmp(argv[1],"off") == 0) {

		if (cpu_num > online_cpus) {
			return plsplspls(online_cpus);
		}
		
		if (cpu_num != 0) { 
	
			FILE *desc = fopen(buffer,"w");
			if (desc == NULL) {
				perror("fopen");
				return 1;
			}
			
			char digit = '0';
			fwrite(&digit, sizeof(char), 1, desc);	
			fclose(desc);
		}		
	
		return 0;
	}

	return pleaseleave();
}
