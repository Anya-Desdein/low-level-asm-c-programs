#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/socket_test"

int main(void) {
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("socket");
		return 1;
	}
	printf("%d\n", fd);
	unlink(SOCKET_PATH);

	struct sockaddr_un server_addr;

	memset(&server_addr, 0,sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path));

	if(bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==  -1) {
		perror("bind");
		close(fd);
		return 1;

	}

	close(fd);
	return 0;
}
