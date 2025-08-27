#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

__attribute__((noreturn)) void child_labour(void) {
	for (int i = 0; 1; ++i) {
		printf("CHILD: %d\n", i);
	}
	exit(0);
}

int main(void) {
	
	int baby_ret;

	setvbuf(stdout, NULL, _IONBF, 0);

	printf("Race start!\n");
	
	pid_t baby_pid = fork();
	if (baby_pid == -1) {
		perror("fork failed");
		return 1;
	}
	

	if (baby_pid == 0)
		child_labour();

	for (int i = 0; i < 524288; ++i) {
		if (i % 512 && i != 0)
			continue;

		printf("PARENT: %d\n", i);
	}
	kill(baby_pid, 9);

	waitpid(baby_pid, &baby_ret, 0);
	printf("Child exited with %d\n", baby_ret);

	return 0;
}
