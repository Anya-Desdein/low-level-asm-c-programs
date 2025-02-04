#include  <stdio.h>
#include <stdlib.h>
#include  <errno.h>

#include <fcntl.h>
#include <unistd.h>


void clfl(int *file_descr) {
	printf("Closing elf file, descriptor number %d\n", *file_descr);
	close(*file_descr);
}


#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
	if (argc !=2) {
		printf("Usage: %s FILE_PATH\n", argv[0]);
		return 1;
	}

	int file_descr  __attribute__((cleanup(clfl))) = open(argv[1], O_RDONLY);
	if (file_descr == -1) {
		perror("file open");
		return 1;
	}

	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;
	int iter = 0;
	int starts_with_elf = 0;
	while ((bytes_read = read(file_descr, buffer, BUFFER_SIZE)) > 0) {
		if (iter == 0) {
			unsigned char elf_hex[] = {0x7f, 0x45, 0x4c, 0x46};
			for (int i=0; i<(sizeof(elf_hex)/sizeof(elf_hex[0])); i++) {
				printf("%c", buffer[i]);
				if(elf_hex[i] != buffer[i]) {
					printf("\n");
					break;
				}
				printf("\n");
				starts_with_elf = 1;
			}
		}
			
		if (starts_with_elf == 0) {
			puts("Not an elf");
		} else {
			puts("It's an elf!");
		}

		iter += 1024;
		lseek(file_descr, iter, SEEK_SET );
	}
	printf("File end\n");



	return 0;
}
