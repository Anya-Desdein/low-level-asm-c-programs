#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
	if (argc != 3) {
		perror("Incorrect number of arguments. 1 - path to a file, 2 - text to append");
		return 1;
	}

	FILE *file = fopen(argv[1], "r+");
	if (file == NULL) {
		perror("Incorrect file path.");
		return 1;
	}
	
	char buffer[BUFFER_SIZE];
	int position = 0;
	
	while (1) {
		size_t buf_len = fread(buffer, 1, BUFFER_SIZE, file);
		if (buf_len == 0) {
			break;
		}

		printf(buffer);
		
		if (buf_len < BUFFER_SIZE) {
			position += buf_len;
			break;
		}
		position += BUFFER_SIZE;
	}	
	
	fseek(file, position, SEEK_SET);
	fputs(argv[2], file);

	printf("\n");	
	printf("Position: %d\n", position);
	
	fclose(file);
	return 0;
}
