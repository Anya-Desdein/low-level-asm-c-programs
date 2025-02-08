#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "extern_append.h"
#include "extern_cleanup.h"

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
	
	int position = file_append(file, argc, argv);
	
	printf("\n");	
	printf("Position: %d\n", position);

	return 0;
}
