#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <stdint.h>
#include <inttypes.h>

#include <linux/limits.h>

int main(int argc, char *argv[]) {
	
	if (argc < 2 || argc > 3) {
		printf("Usage:\n\toocpu list,\n\toocpu on CPU_NUM,\n\toocpu off CPU_NUM\n");
		return 1;
	}

	int all_cpus = sysconf(_SC_NPROCESSORS_CONF);		
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
		
	if (strcmp(argv[1],"list") == 0) {

		printf("TOTAL CPU COUNT %d\nONLINE CPU COUNT %d\n", all_cpus, online_cpus);
		return 0;
	}

	if (argc < 3) {
		printf("Usage:\n\toocpu list,\n\toocpu on CPU_NUM,\n\toocpu off CPU_NUM\n");
		return 1;
	}

	// Create correct path to a file managing core state (on/off)
	int cpu_num = atoi(argv[2]);
	char str[] = "/sys/devices/system/cpu/cpu%d/online";
	char buffer[PATH_MAX];

	int cx = sprintf(buffer, str, cpu_num);
	// printf("Path to file being edited: %s\n", buffer);

	if (strcmp(argv[1],"on") == 0) {

		if (cpu_num > all_cpus) {

			printf("Argument cannot exceed TOTAL CPU COUNT(%d)\n", all_cpus);
			return 1;
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

			printf("Argument cannot exceed ONLINE CPU COUNT(%d)\n", online_cpus);
			return 1;
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

	printf("Usage:\n\toocpu list,\n\toocpu on CPU_NUM,\n\toocpu off CPU_NUM\n");
	return 1;
}
