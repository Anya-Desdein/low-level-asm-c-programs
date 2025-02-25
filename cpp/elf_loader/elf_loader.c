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

unsigned long PAGE_SIZE = 0x1000;

int IS_BIG_ENDIAN = 0;
int IS_PIE = 0;

static void clfl(int *file_descr) {
	close(*file_descr);
}

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

// Virtual size
void virsz_calc(uint64_t *paddr, uint64_t *pmemsz, uint64_t *pvirsz) {

	const uint64_t addr_begin = page_align_d(*paddr);
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

	uint64_t highest_addr=0, lowest_addr=0, alloc_len=0;
	void* total_addr;

	if (IS_PIE == 1){
		char *buffer_entry = (char *)&ph;
		off_t desc_loc;
		for (off_t entries_read=0; entries_read < h.e_phnum; entries_read++) {
			desc_loc = entries_read * h.e_phentsize + h.e_phoff;

			lseek(file_descr, desc_loc, SEEK_SET);
		
			for (off_t bytes_read=0; bytes_read < h.e_phentsize; ) {
				curr_br = read(file_descr, buffer_entry + bytes_read, h.e_phentsize - bytes_read);
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
		// logic to count together and malloc
		if (lowest_addr > ph.p_vaddr)
			lowest_addr = ph.p_vaddr;
 		
		unsigned long seg_highest_addr = ph.p_vaddr + ph.p_memsz;
		if (highest_addr < seg_highest_addr)
			highest_addr = seg_highest_addr;
		}	
		
		highest_addr = page_align(highest_addr);
		lowest_addr = page_align_d(lowest_addr);
		alloc_len = highest_addr - lowest_addr;

		unsigned int page_flags = 0 | MAP_PRIVATE | MAP_ANONYMOUS;
		unsigned int temp_flags = 0 | PROT_READ | PROT_WRITE;
		
		// Debug!!!!!!!!!!!!!
		total_addr = mmap("0x5a6b3c11c000", alloc_len, temp_flags, page_flags | MAP_FIXED, -1, 0);
		// total_addr = mmap(NULL, alloc_len, temp_flags, page_flags, -1, 0);
		
		if (total_addr == MAP_FAILED) {
			perror("mmap");
			printf("Start addr: %" PRIu64 "\n",(unsigned long)total_addr);
			printf("Allocated size: %" PRIu64 "\n",alloc_len);
			return 1;
		}
		printf("PIE alloc\n");
		printf("Start addr: %" PRIu64 "\n",(unsigned long)total_addr);
		printf("Allocated size: %" PRIu64 "\n",alloc_len);
			
	}

	char *buffer_entry = (char *)&ph;
	off_t desc_loc;
	for (off_t entries_read=0; entries_read < h.e_phnum; entries_read++) {
		desc_loc = entries_read * h.e_phentsize + h.e_phoff;

		lseek(file_descr, desc_loc, SEEK_SET);
	
		for (off_t bytes_read=0; bytes_read < h.e_phentsize; ) {
			curr_br = read(file_descr, buffer_entry + bytes_read, h.e_phentsize - bytes_read);
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
	
		// For debug only
		printf("Processing %jd segment\n", (entries_read+1));
		// Segment type
		//if (ph.p_type < 1 && ph.p_type > 4) {
		if (ph.p_type != 1) {
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
			prot_flags = PROT_NONE;

		unsigned int page_flags = 0;		
		if ((h.e_type == 3 && IS_PIE == 1) || h.e_type == 2) { 
			page_flags |= MAP_FIXED;
			page_flags |= MAP_PRIVATE;
		}

		// TODO: If a segment is of size 0, check flags and handle 
		if (ph.p_memsz == 0) { 
			printf("Skipping segment of size 0\n");
			continue;
		}

		
		unsigned long adjusted_vaddr;
		// Adjust vaddr for PIE
		if (h.e_type == 3 && IS_PIE == 1) {
			adjusted_vaddr =  page_align_d((ph.p_vaddr + (uint64_t)total_addr));
			printf("adjusted_vaddr: %lu\n", adjusted_vaddr);
		} else {
			adjusted_vaddr = page_align_d(ph.p_vaddr);
		}

		// Adjust offset, calc diff
		unsigned long adjusted_offset = page_align_d(ph.p_offset);
		unsigned long diff = ph.p_offset - adjusted_offset;
		
		if (diff != 0)
			printf("Segment offset moved by %lu bytes, from %lu to %" PRIu64 "\n", diff, ph.p_offset, adjusted_offset);
		
		
		// Adjust vaddr alignment
		uint64_t virsz;
		virsz_calc(&adjusted_vaddr, &ph.p_memsz, &virsz); 

		
		unsigned int temp_flags = 0 | PROT_READ | PROT_WRITE;
		void* segment_addr = mmap((void *)adjusted_vaddr, virsz, temp_flags, page_flags | MAP_ANONYMOUS, -1, 0);

		if (segment_addr == MAP_FAILED) {
			perror("mmap");
			printf("Size of the segment: %" PRIu64 "\n",ph.p_memsz);
			printf("Virtual addr: %" PRIu64 "\n",ph.p_vaddr);
			printf("Offset in file: %" PRIu64 " \n",ph.p_offset);
			printf("Allocated size: %" PRIu64 "\n", virsz);
			return 1;
		}
		
	
		uint64_t segment_end = ph.p_offset + ph.p_filesz;
		struct stat file_stat;
		if (fstat(file_descr, &file_stat) == -1) {
			perror("fstat");
			return 1;
		}

		if ((ph.p_offset + ph.p_filesz) > file_stat.st_size) {
			printf("Exceeding EoF");
			printf("File size %lld, EoSegment %" PRIu64 "\n", (long long)file_stat.st_size, segment_end);
			return 1;
		}
		
		printf("Filesz %" PRIu64 "\n", ph.p_filesz);

		char *seg_buf = malloc(ph.p_filesz);

		off_t cu_br;
		for (off_t br=0; br < ph.p_filesz; ) {
			cu_br = pread(file_descr, seg_buf + br, ph.p_filesz - br, ph.p_offset + br);
			if (cu_br == -1) {
				printf("Error reading file\n");
				free(seg_buf);
				seg_buf = NULL;
			
				return 1;
			}
	
			if (cu_br == 0) {
				printf("Unexpected EoF\n");
				free(seg_buf);
				seg_buf = NULL;

				return 1;
			}
			br += cu_br;
		}
		memcpy((void *)ph.p_vaddr, seg_buf, ph.p_filesz);

		if (mprotect(segment_addr, virsz, prot_flags ) == -1) {
			perror("mprotect");
			return 1;
		}

		free(seg_buf);
		seg_buf = NULL;
	} 
	
	if (IS_PIE == 1 || ph.p_type == 2) { 
		void (*entry)() = (void (*)())h.e_entry;
		entry();
	
	} else {
		printf("Shared object loaded \n");
	}

	return 0;
}
