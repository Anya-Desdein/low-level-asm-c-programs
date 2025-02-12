#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include <inttypes.h>

int IS_PIE = 0;

static void clfl(int *file_descr) {
	printf("Closing file descriptor number %d\n", *file_descr);
	close(*file_descr);
}

static void is_pie() {
	// TODO: find how to differentiate PIE from shared object
	IS_PIE = 1;
}

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

	ssize_t bytes_read;

	// E_ident
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

	unsigned char e_magic[] = {0x7f, 0x45, 0x4c, 0x46}; 

	// Full ELF Header 
	typedef struct {
		union E_ident_u e_ident; // ELF identification
		uint16_t e_type; // Object file type
		uint16_t e_machine; // Machine type
		uint32_t e_version; // Obj file version
		uint64_t e_entry; // Entry point addr
		uint64_t e_phoff; // Program header offstet
		uint64_t e_shoff; // Section header offset
		uint32_t e_flags; // Processor-specific flags
		uint16_t e_ehsize; // ELF header size
		uint16_t e_phentsize; // Size of program header entry
		uint16_t e_phnum; // Number of program header entries
		uint16_t e_shentsize; // Size of section header entry
		uint16_t e_shnum; // Number of section header entries
		uint16_t e_shstmdx; // Section name string table index 
	} Elf64_Ehdr; 
	
	Elf64_Ehdr h;
	_Static_assert(sizeof(Elf64_Ehdr) == 64, "e_header not 64 bytes");

	struct stat file_status;
	int fstat_chck = fstat(file_descr, &file_status);
	if (fstat_chck == -1) {
		perror("Fstat failure");
	}
	printf("File size: %lld\n", (long long)file_status.st_size);
	
	char *buffer = (char *)&h;
	off_t curr_br;
	for (off_t bytes_read=0; bytes_read < 64; ) {
		curr_br = read(file_descr, buffer + bytes_read, (sizeof(Elf64_Ehdr) - bytes_read));
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
	
	for (int i=0; i<(sizeof(e_magic)/sizeof(e_magic[0])); i++) {
		printf("%c", h.e_ident.as_bytes[i]);
				
		if(e_magic[i] != h.e_ident.as_bytes[i]) {
			puts("");
			printf("Not an Elf: No Magic\n");
			return 1;		
		}
	}
	puts("");
	
	// Class
	if (h.e_ident.as_bytes[4] > 2 || h.e_ident.as_bytes[4] < 1 ) {
		printf("Unknown ELF Class %02x\n", h.e_ident.as_bytes[4]);
		return 1;
	} else if (h.e_ident.as_bytes[4] == 2) {
		printf("ELF 64-bit\n");
	} else if (h.e_ident.as_bytes[4] == 1) {
		printf("ELF 32-bit\n");
	}

	// Data Encoding
	if (h.e_ident.as_bytes[5] > 2 || h.e_ident.as_bytes[5] < 1 ) {
		printf("Unknown Data Encoding %02x\n", h.e_ident.as_bytes[5]);
		return 1;
	} else if (h.e_ident.as_bytes[5] == 2) {
		printf("Big-endian\n");
	} else if (h.e_ident.as_bytes[5] == 1) {
		printf("Little-endian\n");
	}
			
	// Version
	printf("File version: %02x\n", h.e_ident.as_bytes[6]);


	// Operating System & ABI
	if (h.e_ident.as_bytes[7] == 0) {
		printf("System V ABI\n");
		// Only this 
	} else if (h.e_ident.as_bytes[7] == 1) {
		printf("HP-UX OS\n");
		return 1;
	} else if (h.e_ident.as_bytes[7] == 255) {
		printf("Standalone (embedded) application\n");
		return 1;
	} else {
		printf("Unknown OS & ABI %02x\n", h.e_ident.as_bytes[7]);
		return 1;
	}

	// ABI Version
	printf("ABI Version: %02x\n", h.e_ident.as_bytes[8]);

	printf("h.e_type: %hu\n", h.e_type);

	// File type
	if (h.e_type == 0) {
		printf("No file type\n");
		return 1;
	} else if (h.e_type == 1) {
		printf("Relocatable object file\n");
		return 1;
	} else if (h.e_type == 2) {
		printf("Executable file\n");
		// Executable file
	} else if (h.e_type == 3) {
		printf("Shared object file\n");
		// Position-independent executables (PIE)
		// AND
		// Relocatable object files
		
		is_pie();

		if (IS_PIE == 1) {
			printf("This is PIE\n");	
		}
	} else if (h.e_type == 4) {
		printf("Core file\n");
		return 1;
	} else if (h.e_type == 0xFEFF || h.e_type == 0xFE00) {
		printf("Environment-specific use\n");
		return 1;
	} else if (h.e_type == 0xFFFF || h.e_type == 0xFF00) {
		printf("Processor-specific use\n");
		return 1;
	} else {
		printf("Unknown file type\n");
		return 1;
	};

	// Target architecture
	// For System V ABI only
	// Based on sco.com's machine table for System V ABI
	// Only x86 for now
	if (h.e_machine == 62) {
		printf("AMD x86-64 architecture\n");

	} else {
		printf("Machine type: %hu\nThis architecture type is currently not being supported", h.e_machine);
		return 1;
	}

	if (h.e_version == 1) {
		printf("Valid ELF version == 1\n");
	} else {
		printf("Invalid ELF version\n");
	}
	
	// Entry point
	printf("Entry point addr: %" PRIu64 "\n", h.e_entry);

	// Program header table offset
	printf("Program header table offset: %" PRIu64 "\n", h.e_phoff);
	
	// Section header table offset
	printf("Program header table offset: %" PRIu64 "\n", h.e_shoff);

	// Flags
	printf("Flags: %" PRIu32 "\n", h.e_flags);
	
	// ELF header size
	printf("ELF header size: %" PRIu16 "\n", h.e_ehsize);
	
	// Program header table entry size
	printf("Program header entry size: %" PRIu16 "\n", h.e_phentsize);

	// Program header table entry count
	printf("Number of entries in program header: %" PRIu16 "\n", h.e_phnum);
	
	// Section header table entry size 
	printf("Section header entry size: %" PRIu16 "\n", h.e_shentsize);
	
	// Section header table entry count
	printf("Number of entries in section section: %" PRIu16 "\n", h.e_shnum);
	
	// Index of section containing section name string table inside section header 
	if (h.e_shstmdx == 0) {
		printf("SHN_UNDEF\n");
	} else {
		printf("Shstmdx: %" PRIu16 "\n", h.e_shstmdx);
	}
	
	char *buffer_entry = malloc(h.e_phentsize);
	off_t curr_entry;
	off_t desc_loc;
	for (off_t entries_read=0; entries_read < h.e_phnum; entries_read++) {
		desc_loc = entries_read * h.e_phentsize + h.e_phoff;

		lseek(file_descr, desc_loc, SEEK_SET);
	
		for (off_t bytes_read=0; bytes_read < h.e_phentsize; ) {
			curr_entry = read(file_descr, buffer_entry + bytes_read, h.e_phentsize - bytes_read);
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
	
		printf("Header entry bytes:\n");
			uint64_t print_count = h.e_phentsize/2;
			for (uint64_t i=0; i<print_count; i++) {
				printf("%02x ", (unsigned char)buffer_entry[i]);
			}
		printf("\n");


	} free(buffer_entry);
	buffer_entry = NULL;

	return 0;
}
