#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <stdint.h>
#include <inttypes.h>

#include <linux/limits.h>

int pleaseleave(void) {	
	printf("Usage:\n\toocpu list,\n\toocpu on CPU_NUM,\n\toocpu off CPU_NUM\n");
	return 1;
}

int plsplspls(int cpus) {
	printf("Argument cannot exceed TOTAL CPU COUNT(%d)\n", cpus);
	return 1;
}

int main(int argc, char *argv[]) {

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
