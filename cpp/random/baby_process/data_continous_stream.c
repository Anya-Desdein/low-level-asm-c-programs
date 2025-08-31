#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("Usage: data_continous_stream stream_count\n");
		return 1;
	}
	
	int count = atoi(argv[1]);

	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = 300000000;

	for (int i=0; i < count; i++) {
		printf("data stream number %d\n", 1+i);
		nanosleep(&time, NULL);
	}
	return 0;
}
