#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

int main(void) {
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("socket");
		return 1;
	}

	



	close(fd);
	return 0;
}
