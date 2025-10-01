#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/epoll.h>

#include <sys/un.h>
#include <sys/select.h>

#define SOCKET_PATH "/tmp/socket_test"
#define CLIENT_MAX FD_SETSIZE
#define CLIENT_QUEUE_MAX SOMAXCONN

#define BUILD_BUG_ON_ZERO(expr) ((int)(sizeof(struct {int:(-!!(expr)); })))

#define __same_type(a,b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define ARRAY_SIZE(arr) \
	((void)BUILD_BUG_ON_ZERO(\
		__same_type((arr), &(arr)[0])), \
		(sizeof(arr) / sizeof(arr[0])) \
	)

#define MAX_EVENTS 64

int main(void) {

	int clients[CLIENT_MAX];

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

	if(listen(fd, CLIENT_MAX) == -1) {
		printf("Connection queque limit reached.\nConnection refused.\n");
	}

	for(size_t i=0; i<(ARRAY_SIZE(clients)); i++)
		clients[i] = -1;

	struct epoll_event ev, events[MAX_EVENTS];
	int ready, epollfd;
	
	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		return 1;
	}

	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror("epoll_ctl: fd");
		return 1;
	}
	
	uint32_t new_ev;
	int event_fd;
	for (;;) {
		ready = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (ready == -1) {
			perror("epoll_wait");
			return 1;
		}

		for (int n=0; n<ready, n++) {
			new_ev = evs[i].events;
			event_fd = evs[i].data.fd;

			if (new_ev & EPOLLERR) {
				// get error and close stuff
			}

			if (new_ev & EPOLLHUP) {
				// hangup, close
			}


			




		}
	}
	
	close(fd);
	return 0;
}
