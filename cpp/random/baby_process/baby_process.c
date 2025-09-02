#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

__attribute__((noreturn)) void child_labour(void) {
	for (int i = 0; 1; ++i) {
		printf("CHILD: %d\n", i);
	}
	exit(0);
}

void race_condition(void) {
	
	int baby_ret;

	setvbuf(stdout, NULL, _IONBF, 0);

	printf("Race start!\n");
	
	pid_t baby_pid = fork();
	if (baby_pid == -1) {
		perror("fork failed");
		exit(1);
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
}

void execute_new_baby(void) {
	int baby_ret;
	pid_t baby_pid = fork();
	if (baby_pid == -1) {
		perror("fork failed");
		exit(1);
	}

	if (baby_pid == 0) {
		const char *pathname = "./new_baby_process";
		int exec = execve(pathname, NULL, NULL);
		if (exec == -1) {
			perror("execve");
			exit(1);
		}
	}

	waitpid(baby_pid, &baby_ret, 0);
}

#define MODE_BUFFERED "buffered"
#define MODE_UNBUFFERED "unbuffered"

void use_pipe(char *mode, int stream_count_) {
	int pipefd[2];
	if (pipe2(pipefd, 0) == -1) {
		perror("pipe2");
		exit(1);
	}

	int baby_ret;
	
	if (mode != MODE_BUFFERED && 
	    mode !=  MODE_UNBUFFERED) {
		perror("use_pipe");
		exit(1);
	    }
	printf("------Using %s data------\n", mode);

	char stream_count[16];
	snprintf(stream_count, sizeof(stream_count), "%d", stream_count_);

	setvbuf(stdout, NULL, _IONBF, 0);
	
	pid_t baby_pid = fork();
	if (baby_pid == -1) {
		perror("fork failed");
		exit(1);
	}

	if (baby_pid == 0) {

		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);

		char *pathname = "./data_continous_stream";
		char *argv[] = {
			pathname, 
			(mode == MODE_BUFFERED)
			? "buffered"
			: "unbuffered", 
			stream_count, 
			NULL};
		
		int exec = execve(pathname, argv, NULL);
		if (exec == -1) {
			perror("execve");
			exit(1);
		}
	}

	if (baby_pid > 0) {
		close(pipefd[1]);

		int bufsize = 256;
		char buf[bufsize];
		ssize_t rsize;

		int line = 0;

		while ((rsize = read(pipefd[0], buf, bufsize-1)) > 0) {
			if (rsize == -1) {
				perror("read from pipe");
			}	
			buf[rsize] = '\0';

			printf("%d: %s", line, buf);
			line++;
		}
	}
	waitpid(baby_pid, &baby_ret, 0);
}

int main(void) {
	
	// Spawn a child and race with it
	// Write to console
	race_condition();

	// Execve to another process from the child
	execute_new_baby();

	// Write to a pipe instead of stdout 
	// and use child's output
	use_pipe(MODE_UNBUFFERED, 4);
	use_pipe(MODE_BUFFERED, 2);

	return 0;
}
