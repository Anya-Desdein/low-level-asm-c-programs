#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <stdint.h>
#include <inttypes.h>

#include <limits.h>

int IS_BIG_ENDIAN = 0;
int IS_PIE = 0;

static void clfl(int *file_descr) {
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
	unsigned char e_magic[] = {0x7f, 0x45, 0x4c, 0x46};

	// ELF Header 
	typedef struct {
		unsigned char e_ident[16]; // ELF identification
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
		printf("%c", h.e_ident[i]);
				
		if(e_magic[i] != h.e_ident[i]) {
			puts("");
			printf("Not an Elf: No Magic\n");
			return 1;		
		}
	}
	puts("");
	
	// Class
	if (h.e_ident[4] > 2 || h.e_ident[4] < 1) {
		printf("Unknown ELF Class %02x\n", h.e_ident[4]);
		return 1;
	}

	// Data Encoding
	if (h.e_ident[5] > 2 || h.e_ident[5] < 1 ) {
		printf("Unknown Data Encoding %02x\n", h.e_ident[5]);
		return 1;
	} 
	
	if (h.e_ident[5] == 2)
		IS_BIG_ENDIAN = 1;
			
	// Operating System & ABI
	if (h.e_ident[7] != 0) {
		printf("Operating System & ABI not supported\n");
		return 1;
	}

	// File type
	if (h.e_type != 3 && h.e_type != 2) {
		printf("File type not supported \n");
		return 1;
	}

	if (h.e_type == 3) {
		printf("Shared object file\n");
		// Position-independent executables (PIE)
		// AND
		// Relocatable object files
		
		is_pie();
	}
	
	// Target architecture
	// System V ABI only
	// Based on sco.com's machine table for System V ABI
	if (h.e_machine != 62) {
		printf("Target Architecture not supported.\n");
		return 1;
	}

	if (h.e_version < 1) {
		printf("Invalid ELF version\n");
		return 1;
	}

	// ELF program header entry
	typedef struct {
		uint32_t p_type; // Type of segment
		uint32_t p_flags; // Segment attributes
		uint64_t p_offset; // Offset in file
		uint64_t p_vaddr; // Virtual address in memory
		uint64_t p_paddr; // Reserved
		uint64_t p_filesz; // Size of segment in file
		uint64_t p_memsz; // Size of segment in memory
		uint64_t p_align; // Alignment of segment
	} E_phdr;

	E_phdr ph;
	_Static_assert(sizeof(ph) == 56, "e_ident not 56 bytes");
	
	char *buffer_entry = (char *)&ph;
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
	
		printf("Segment number %jd: ", (entries_read+1));
		// Segment type
		if (ph.p_type < 1 && ph.p_type > 4) {
			printf("Skipping entry\n");
			continue;
		}

		// Offset in file
		if (ph.p_offset < 0) {
			printf("Incorrect offset: %" PRIu64 "\n", ph.p_offset);
			return 1;
		}
	
		unsigned int prot_flags = 0;
		if (ph.p_flags & 0x04)
			prot_flags |= PROT_READ;
		if (ph.p_flags & 0x02)
			prot_flags |= PROT_WRITE;
		if (ph.p_flags & 0x01)
			prot_flags |= PROT_EXEC;
		if (ph.p_flags == 0)
			prot_flags |= PROT_NONE;

		unsigned int page_flags = 0;		
		IS_PIE = 1; // Delete after differentiating shared library from PIE
		if (h.e_type == 3 && IS_PIE == 1) 
			page_flags |= MAP_PRIVATE;
		if (h.e_type == 2) {
			page_flags |= MAP_FIXED;
			page_flags |= MAP_PRIVATE;
		} 

		void* segment_addr = mmap((void *)ph.p_vaddr, ph.p_memsz, prot_flags, page_flags, file_descr, ph.p_offset);
		if (segment_addr == MAP_FAILED) {
			perror("mmap");
			return 1;
		}
	} 
	
	void (*entry)() = (void (*)())h.e_entry;
	entry();
	
	return 0;
}
