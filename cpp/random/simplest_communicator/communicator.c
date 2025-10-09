#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/epoll.h>

#include <sys/un.h>
#include <sys/select.h>

#include <assert.h>

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
#define BUF_SIZE (4*1024)
#define RETURN_BUF_SIZE (2*(BUF_SIZE))

#ifdef __STDC_VERSION__
	#if __STDC_VERSION__ >= 202311L
		#define SOCK_STATIC_ASSERT(expr, msg) \
			static_assert(expr, msg)
	#elif __STDC_VERSION__ >= 201112L
		#include <assert.h>
		#define SOCK_STATIC_ASSERT(expr, msg) \
			_Static_assert(expr, msg)
	#else 
		#error "Minimal supported version is C11"
	#endif
#endif

static ssize_t
sock_write(int fd, char *buf, ssize_t bufsize) {
	assert((fd >= 0) && "int fd missing in sock_write");
	assert((buf != NULL) && "char *buf missing in sock_write");
	assert((bufsize>1) && "ssize_t bufsize missing in sock_write");

	ssize_t n = 0, send = 0;
	while (send < bufsize) {
		n = write(
			fd, 
			buf+send,
			bufsize - send
		);

		if (n < 0) {
			perror("write");
			return -1;
		}

		if (n == 0)
			return 0;

		send += n;
	}
	return 0;
}

static ssize_t
sock_read(int fd, char *buf, ssize_t bufsize) {
	assert((fd >= 0) && "int fd missing in sock_read");
	assert((buf != NULL) && "char *buf missing in sock_read");
	assert((bufsize>1) && "ssize_t bufsize missing in sock_read");

	ssize_t n = 0, filled = 0;
	while (filled < bufsize) {
		n = read(
			fd, 
			buf+filled,
			bufsize - filled
		);

		if (n < 0) {
			perror("read");
			return -1;
		}

		if (n == 0)
			return filled;

		filled += n;
	}
	return filled;
}

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
	int event_fd, new_sock;
	ssize_t read_size, write_err;

	char read_res[BUF_SIZE], return_msg[RETURN_BUF_SIZE];
	memset(read_res, 0, sizeof(read_res));
	for (;;) {
		ready = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (ready == -1) {
			perror("epoll_wait");
			return 1;
		}

		for (int n=0; n<ready; n++) {
			new_ev = events[n].events;
			event_fd = events[n].data.fd;
			
			if (event_fd == fd) {
				new_sock = accept4(event_fd, NULL, SOCK_NONBLOCK);
				if (new_sock == -1) {
					perror("accept");
					close(new_sock);
					continue;
				}
			
				if (epoll_ctl(
					epollfd,
					EPOLL_CTL_ADD,
					new_sock,
					&ev)
					== -1) {
					perror("epoll_ctl_add");
					close(new_sock);
				}

				if (new_ev & EPOLLERR) {
					// should never happen
					printf("LISTENING SOCK: epoll err\n");
					return 1;
				}
				continue;	
			}

			read_size = sock_read(event_fd, read_res, BUF_SIZE);
			if (read_size == -1) {
				epoll_ctl(
					epollfd, 
					EPOLL_CTL_DEL, 
					event_fd,
					0
				);

				close(event_fd);
			}

			if (read_size == 0) {
				// No data to read, sth is wrong
				printf("SOCK: uncaught read error.\n");
				epoll_ctl(
					epollfd, 
					EPOLL_CTL_DEL, 
					event_fd,
					0
				);

				close(event_fd);
			}

			if (
				(new_ev & EPOLLERR) 
				|| (new_ev & EPOLLHUP)
			) {
				epoll_ctl(
					epollfd, 
					EPOLL_CTL_DEL, 
					event_fd,
					0
				);

				close(event_fd);
			}

			memset(
				return_msg, 
				0, 
				sizeof(return_msg)
			);

			snprintf(
				return_msg, 
				sizeof(return_msg),
				"Socket %d on event %d said: %s",
				event_fd, 
				new_ev, 
				read_res
			);

			write_err = sock_write(
				event_fd, 
				return_msg, 
				BUF_SIZE
			);
			if (read_size == -1) {
				epoll_ctl(
					epollfd, 
					EPOLL_CTL_DEL, 
					event_fd,
					0
				);
				close(event_fd);
			}
		}
	}
	
	close(fd);
	return 0;
}
