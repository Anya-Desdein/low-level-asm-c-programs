#define _GNU_SOURCE

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif

#define MEMFD_SIZE (2*1024*1024)

int main(void) {
	int fd = syscall(SYS_memfd_create, "inmemory_file", MFD_CLOEXEC | MFD_ALLOW_SEALING);

	if (fd == -1) { 
		perror("memfd_create"); 
		return 1;
	}

	if (ftruncate(fd, MEMFD_SIZE)) { 
		perror("ftruncate"); 
		return 1;
	}

	char *p = mmap(NULL, MEMFD_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (p == MAP_FAILED) { 
		perror("mmap"); 
		return 1;
	}

	if (fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW) == -1) {
		perror("fcntl(F_ADD_SEALS)");
		return 1;
	}

	int seal_mask = fcntl(fd, F_GET_SEALS);
	if (seal_mask == -1) {
		perror("fcntl(F_GET_SEALS)");
		return 1;
	}

	printf("In-memory file created:\nFile Descriptor: %d\nSize: %d\nSeals:", fd, MEMFD_SIZE);

	if (seal_mask == 0) {
		printf(" NONE");
	}
	if (seal_mask & F_SEAL_SEAL) {
		printf(" SEAL_SEAL");
	}
	if (seal_mask & F_SEAL_SHRINK) {
		printf(" SEAL_SHRINK");
	}
	if (seal_mask & F_SEAL_GROW) {
		printf(" SEAL_GROW");
	}
	if (seal_mask & F_SEAL_WRITE) {
		printf(" SEAL_WRITE");
	}
	if (seal_mask & F_SEAL_FUTURE_WRITE) {
		printf(" SEAL_FUTURE_WRITE");
	}
	printf("\n");
	
	int child_ret;
	pid_t child_pid = fork();

	if (child_pid == -1) {
		perror("fork");
		exit(1);
	}

	if (child_pid == 0) {
		// This will fail due to SEAL_SHRINK
		int trunc = ftruncate(fd, 1024 * 1024);
		if (trunc != 0) {
			perror("ftruncate");
		}
		
		// Add a seal from child
		if (fcntl(fd, 
			F_ADD_SEALS, 
			F_SEAL_SEAL) == -1) {
			perror("fcntl(F_ADD_SEALS)");
			return 1;
		}

		// This will fail, as F_SEAL_SEAL was added
		if (fcntl(
			fd, 
			F_ADD_SEALS, 
			F_SEAL_FUTURE_WRITE)
			== -1) {
			perror("fcntl(F_ADD_SEALS)");
		}
	}
}	
