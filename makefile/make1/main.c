#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 1024

void clfl(FILE **ptr) {
	fclose(*ptr);
}

int is_ascii(char c) {
	return c == '\n' || (c > 31 && c < 127);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s FILE_PATH TEXT_TO_APPEND\n", argv[0]);
		return 1;
	}

	FILE *file __attribute__((cleanup(clfl))) = fopen(argv[1], "r+");
	if (file == NULL) {
		perror("file open");
		return 1;
	}

	char buffer[BUFFER_SIZE];
	int position = 0;

	while (1) {
		size_t buf_len = fread(buffer, 1, BUFFER_SIZE-1, file);
		if (buf_len == 0) {
			break;
		}

		for (size_t i=0; i<buf_len; i++) {
			if ( !is_ascii(buffer[i]) ) {
				printf("Incorrect file contents: non-ascii %c symbol number %lu detected\n", buffer[i], (position+i));
				return 1;
			}
		}

		buffer[buf_len] = '\0';
		printf(buffer);
		
		if (buf_len < BUFFER_SIZE) {
			position += buf_len;
			break;
		}
		position += BUFFER_SIZE;
	}	
	
	fseek(file, 0, SEEK_END);

	fputs(argv[2], file);
	printf("\n");	
	printf("Position: %d\n", position);
	
	return 0;
}
