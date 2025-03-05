#include <unistd.h> 	// System-related functions like getpid()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get avg clockrate
static double getavgclockrate() {

	double clockrate[66];
	FILE *fp = fopen("/proc/cpuinfo", "r");
	
	char *pos = NULL;
	char buffer[4096];
	int iter = 0;
	
	char linecon[] = "cpu MHz"; 
	char colon[] = ":";
	while(fgets(buffer, sizeof(buffer), fp) != NULL) {

		pos = strstr(buffer, linecon);
		if (pos == NULL)
			continue;
	
		pos = strstr(buffer, colon);
		if (pos == NULL)
			continue;

		char * a = pos + 1;
		clockrate[iter] = strtod(a, NULL);	 

		printf("Clockrate for CPU %d: %f\n", iter , clockrate[iter]);
		
		iter ++;
	}
	
	double sum = 0;
	for (int i=0; i<iter; i++) {
		sum += clockrate[i];

	}
	double avg = sum/(double)(long long int)iter;

	return avg;
}


int main() {

	double avgclock = getavgclockrate();	
	printf("Averaged clockrate: %f\n", avgclock);

	return 0;
}
