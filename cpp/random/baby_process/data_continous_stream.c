#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int strcmp_cs(char *a, char *b) {
	while(1) {
		int diff = *a - *b;
		if (diff)
			return diff;

		if (*a == '\0')
			return 0;

		++a;
		++b;
	}
}

#define MODE_NONE 0
#define MODE_BUFFERED 1
#define MODE_UNBUFFERED 2

int main(int argc, char *argv[]) {

	int mode = MODE_NONE;

	if (argc != 3) {
		printf("Usage: data_continous_stream [un]buffered stream_count\n");
		return 1;
	}

	if (strcmp_cs(argv[1], "buffered") == 0)
		mode = MODE_BUFFERED;

	if (strcmp_cs(argv[1], "unbuffered") == 0)
		mode = MODE_UNBUFFERED;
	
	if (mode == MODE_NONE) { 
		printf("Usage: data_continous_stream [un]buffered stream_count\n");
		return 1;
	}

	int count = atoi(argv[2]);

	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = 300000000;
	
	int bufsize = 256;
	char buf[bufsize];
	for (int i=0; i < count; i++) {
		if (mode == MODE_UNBUFFERED) {
			int size = snprintf(
				buf, 
				bufsize, 
				"data stream number %d\n",
				1+i);

			// descriptor 1 is stdout
			// macro => STDOUT_FILENO
			write(STDOUT_FILENO, buf, size+1);
		}

		if (mode == MODE_BUFFERED) {
			printf(
				"data stream number %d\n",
				1+i);
		}	
		
		nanosleep(&time, NULL);
	}
	return 0;
}
