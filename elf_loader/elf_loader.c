#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>


void clfl(int *file_descr) {
	printf("Closing file descriptor number %d\n", *file_descr);
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
	struct E_ident {
		unsigned char el_mag0;	
		unsigned char el_mag1;
		unsigned char el_mag2;
		unsigned char el_mag3;
		unsigned char el_class;
		unsigned char el_data;
		unsigned char el_version;
		unsigned char el_osabi;
		unsigned char el_abiversion;
		unsigned char el_pad;
		unsigned char el_nident;
	} E_ident;
	unsigned char e_magic[] = {0x7f, 0x45, 0x4c, 0x46}; 

	off_t buf_iter = 0;
	off_t f_len = lseek(file_descr, 0, SEEK_END);
	lseek(file_descr, 0, SEEK_SET);
	printf("File len: %lld\n", (long long)f_len);
	int is_magic = 0;
	
	while ((bytes_read = read(file_descr, buffer, BUFFER_SIZE)) > 0) {
		if (buf_iter == 0) {
			for (int i=0; i<(sizeof(e_magic)/sizeof(e_magic[0])); i++) {
				printf("%c", buffer[i]);
				
				if(e_magic[i] != buffer[i]) {
					printf("\n");
					break;
				}
				is_magic = 1;
			}
			
			if (!is_magic) {
				puts("Not an elf");
				break;
			}
	
			if(is_magic) {
				printf("\n");
				puts("It's an elf!");
			}
		}
		
		if (buf_iter >= f_len) {	
			break;
		}

		buf_iter += 1024;
		lseek(file_descr, buf_iter, SEEK_SET);
	}
	return 0;
}
