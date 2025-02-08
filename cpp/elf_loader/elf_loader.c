#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void clfl(int *file_descr) {
	printf("Closing file descriptor number %d\n", *file_descr);
	close(*file_descr);
}


#define BUFFER_SIZE 64

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

	typedef struct {
		unsigned char el_mag0;	
		unsigned char el_mag1;
		unsigned char el_mag2;
		unsigned char el_mag3;
		unsigned char el_class;
		unsigned char el_data;
		unsigned char el_version;
		unsigned char el_osabi;
		unsigned char el_abiversion;
		char el_pad[7];
	} E_ident_t;

	union E_ident_u {
		E_ident_t as_struct;
		unsigned char as_bytes[sizeof(E_ident_t)];
	};
	
	_Static_assert(sizeof(E_ident_t) == 16, "e_ident not 16 bytes");

	union E_ident_u E_ident_i;
	unsigned char e_magic[] = {0x7f, 0x45, 0x4c, 0x46}; 

	struct stat file_status;
	int fstat_chck = fstat(file_descr, &file_status);
	if (fstat_chck == -1) {
		perror("Fstat failure");
	}
	printf("File size: %lld\n", (long long)file_status.st_size);
	
	off_t curr_br;
	for (off_t bytes_read=0; bytes_read < 64; ) {
		curr_br = read(file_descr, buffer + bytes_read, (BUFFER_SIZE - bytes_read));
		if (curr_br == -1) {
			printf("Error reading file\n");
			return 1;
		}

		if (curr_br == 0) {
			printf("Unexpected EoF\n");
			return 1;
		}

		bytes_read += curr_br;
	}
	
	memcpy(&E_ident_i.as_bytes[0], &buffer, sizeof(E_ident_i));
	memcpy(&E_ident_i.as_struct, &buffer, sizeof(E_ident_i));
		
	for (int i=0; i<(sizeof(e_magic)/sizeof(e_magic[0])); i++) {
		printf("%c", E_ident_i.as_bytes[i]);
				
		if(e_magic[i] != E_ident_i.as_bytes[i]) {
			puts("");
			printf("Not an Elf: No Magic\n");
			return 1;		
		}
	}
	puts("");
	
	// Class
	if (E_ident_i.as_bytes[4] > 2 || E_ident_i.as_bytes[4] < 1 ) {
		printf("Unknown ELF Class %02x\n", E_ident_i.as_bytes[4]);
		return 1;
	} else if (E_ident_i.as_bytes[4] == 2) {
		printf("ELF 64-bit\n");
	} else if (E_ident_i.as_bytes[4] == 1) {
		printf("ELF 32-bit\n");
	}

	// Data Encoding
	if (E_ident_i.as_bytes[5] > 2 || E_ident_i.as_bytes[5] < 1 ) {
		printf("Unknown Data Encoding %02x\n", E_ident_i.as_bytes[5]);
		return 1;
	} else if (E_ident_i.as_bytes[5] == 2) {
		printf("Big-endian\n");
	} else if (E_ident_i.as_bytes[5] == 1) {
		printf("Little-endian\n");
	}
			
	// Version
	printf("File version: %02x\n", E_ident_i.as_bytes[6]);


	// Operating System & ABI
	if (E_ident_i.as_bytes[7] == 0) {
		printf("System V ABI\n");
	} else if (E_ident_i.as_bytes[7] == 1) {
		printf("HP-UX OS\n");
	} else if (E_ident_i.as_bytes[7] == 255) {
		printf("Standalone (embedded) application\n");
	} else {
		printf("Unknown OS & ABI %02x\n", E_ident_i.as_bytes[7]);
		return 1;
	}

	// ABI Version
	printf("ABI Version: %02x\n", E_ident_i.as_bytes[8]);

	return 0;
}
