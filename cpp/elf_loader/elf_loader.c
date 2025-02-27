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

unsigned long long PAGE_SIZE = 0x1000ull;

static void clfl(int *file_descr) {
	close(*file_descr);
}

int IS_PIE = 0;
static void is_pie() {
	// TODO: find how to differentiate PIE from shared object
	IS_PIE = 1;
}

// Adjust alignment up
unsigned long page_align(unsigned long p) {
	p = (p+PAGE_SIZE-1) & (~(PAGE_SIZE-1));
	return p;
}
//Adjust alignment down
unsigned long page_align_d(unsigned long p) {
	p = p & (~(PAGE_SIZE-1));
	return p;
}

int read_usys(int* file_descr, char* buffer, off_t* bytes2read, uint64_t offset) {
	off_t curr_br, bytesread;
	for (bytesread=0; bytesread < *bytes2read; ) {
		if (offset == 0) {
			curr_br = read(*file_descr, buffer + bytesread, *bytes2read - bytesread);
		} else {
			curr_br = pread(*file_descr, buffer + bytesread, *bytes2read - bytesread, offset + bytesread);
		}

		printf("Desc %d, buffer %d, lenght %llu, offset %" PRIu64 "\n", *file_descr, *buffer, (unsigned long long)bytes2read, offset);
		if (curr_br == -1) {
			printf("Error reading file\n");
			return 1;
		}

		if (curr_br == 0) {
			printf("Unexpected EoF\n");
			return 1;
		}

		bytesread += curr_br;
	}

	return 0;
}

// Virtual size
void virsz_calc(uint64_t *paddr, uint64_t *pmemsz, uint64_t *pvirsz) {

	const uint64_t addr_begin =page_align_d(*paddr);
	const uint64_t addr_end   = page_align(*paddr + *pmemsz);
	const uint64_t virsz      = addr_end - addr_begin;

	*paddr = addr_begin;
	*pvirsz = virsz;
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

	char *buf = (char *) &h;
	off_t bsize = sizeof(Elf64_Ehdr);
	// Read Elf Header
	int re = read_usys(&file_descr, buf, &bsize, 0);
	if (re == 1)
		return 1;

	for (int i=0; i<(sizeof(e_magic)/sizeof(e_magic[0])); i++) {
		if(e_magic[i] != h.e_ident[i]) {
			puts("");
			printf("Not an Elf: No Magic\n");
			return 1;		
		}
	}
	
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
	
	// Endianness
	if (h.e_ident[5] == 2) {
		printf("Big Endian not supported for now\n");
		return 1;
	}
			
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
	
	// For e_type == 2 && e_type == 3
	off_t desc_loc, entries_read;

	buf = (char *)&ph;
	bsize = sizeof(E_phdr);

	// Used only for e_type == 3
	uint64_t lowest_addr=0, highest_addr=0, alloc_size=0;
	void* alloc_type2_addr = NULL;

	if (h.e_type == 3){
		// Read Program Headers 
		for (entries_read=0; entries_read < h.e_phnum; entries_read++) {
			desc_loc = entries_read * h.e_phentsize + h.e_phoff;

			lseek(file_descr, desc_loc, SEEK_SET);
		
			re = read_usys(&file_descr, buf, &bsize, 0);
			if (re) 
				return 1;

			// Offset in file
			if (ph.p_offset < 0) {
				printf("Incorrect offset: %" PRIu64 "\n", ph.p_offset);
				return 1;
			}
		
			// Add only Loadable segment sizes
			if (ph.p_type != 1) {
				continue;
			}


			// Count pages to allocate for the whole program
			if (lowest_addr > ph.p_vaddr)
				lowest_addr = ph.p_vaddr;
			
			unsigned long temp_highest_addr = ph.p_vaddr + ph.p_memsz;
			if (highest_addr < temp_highest_addr)
				highest_addr = temp_highest_addr;
		}	
			
		// Align to page
		highest_addr = page_align(highest_addr);
		lowest_addr = page_align_d(lowest_addr);
		alloc_size = highest_addr - lowest_addr;

		unsigned int temp_page_flags = 0 | MAP_PRIVATE;
		if(IS_PIE)
			temp_page_flags |= MAP_ANONYMOUS;

		unsigned int temp_prot_flags = 0 | PROT_READ | PROT_WRITE;
		
		// Let MMU choose the addr, making it random
		alloc_type2_addr = mmap(NULL, alloc_size, temp_page_flags, temp_page_flags, -1, 0);
		
		if (alloc_type2_addr == MAP_FAILED) {
			perror("mmap");
			printf("Start addr: %" PRIu64 "\n",(unsigned long)alloc_type2_addr);
			printf("Allocated size: %" PRIu64 "\n",alloc_size);
			return 1;
		}
	}

	// Read Program headers 
	for (entries_read=0; entries_read < h.e_phnum; entries_read++) {
		desc_loc = entries_read * h.e_phentsize + h.e_phoff;

		lseek(file_descr, desc_loc, SEEK_SET);
	
		re = read_usys(&file_descr, buf, &bsize, 0);
		if (re) 
			return 1;
	
		// Negative offset
		if (ph.p_offset < 0) {
			printf("Incorrect offset: %" PRIu64 "\n", ph.p_offset);
			return 1;
		}
	
		// Segment not loadable
		if (ph.p_type != 1) {
			continue;
		}
		// Segment size 0
		if (ph.p_memsz == 0) 
			continue;

		unsigned int prot_flags = 0;
		if (ph.p_flags & 0x04)
			prot_flags |= PROT_READ;
		if (ph.p_flags & 0x02)
			prot_flags |= PROT_WRITE;
		if (ph.p_flags & 0x01)
			prot_flags |= PROT_EXEC;
		if (ph.p_flags == 0)
			prot_flags = PROT_NONE;
		
		unsigned int page_flags = 0 | MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;
		
		unsigned long adjusted_vaddr;
		// Adjust vaddr for Shared Object && PIE
		if (h.e_type == 3) {
			adjusted_vaddr =  page_align_d(( (unsigned long)ph.p_vaddr + (unsigned long)alloc_type2_addr));
		} else {
			adjusted_vaddr = page_align_d(ph.p_vaddr);
		}

		// Adjust offset, calc diff
		unsigned long adjusted_offset = page_align_d(ph.p_offset);
		unsigned long diff = ph.p_offset - adjusted_offset;
		
		// Adjust vaddr alignment
		uint64_t virsz;
		virsz_calc(&adjusted_vaddr, &ph.p_memsz, &virsz); 
		
		char *buff = malloc(ph.p_filesz); // For memcpy 
		
		void *segment_page_addr = (void*)adjusted_vaddr;
		
		// MMAP regions only for Executable File
		if (h.e_type == 2) {
			unsigned int temp_flags = 0 | PROT_READ | PROT_WRITE;
			segment_page_addr = mmap(
				(void *)adjusted_vaddr, 
				virsz,
				temp_flags, 
				page_flags, 
				-1,
				0
			);

			if (segment_page_addr == MAP_FAILED) {
				perror("mmap");
				fprintf(stderr, "Size of the segment: %" PRIu64 "\n",ph.p_memsz);
				fprintf(stderr, "Virtual addr: %" PRIu64 "\n",ph.p_vaddr);
				fprintf(stderr, "Offset in file: %" PRIu64 " \n",ph.p_offset);
				fprintf(stderr, "Allocated size: %" PRIu64 "\n", virsz);
				return 1;
			}
		
			uint64_t segment_end = ph.p_offset + ph.p_filesz;
		}

		re = read_usys(&file_descr, buff, &bsize, ph.p_offset);
		if (re) 
			return 1;
	
		// Copy segment from ELF into page
		void *segment_addr = (void*)ph.p_vaddr;
		
		if (h.e_type == 3)
			segment_addr = (void*)((uint64_t)segment_addr + alloc_type2_addr);

		memcpy(segment_addr, buff, ph.p_filesz);

		if (mprotect(segment_page_addr, virsz, prot_flags ) == -1) {
			perror("mprotect");
			return 1;
		}

		free(buff);
		buff = NULL;
	}
	
	if ((h.e_type == 3 && IS_PIE == 1) || h.e_type == 2) { 
		void (*entry)() = (void (*)())(h.e_entry + (uint64_t)alloc_type2_addr);
		entry();
	
	} else {
		printf("Loaded %s\n", argv[1]);
	}

	return 0;
}
