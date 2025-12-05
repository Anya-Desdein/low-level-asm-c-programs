#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/epoll.h>

#include <sys/un.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sys/resource.h>

#include <ctype.h>

#define SOCKET_PORT "/tmp/socket_test"
#define CLIENT_QUEUE_MAX SOMAXCONN

#define CLIENT_MAX 128
#define CLIENT_MAX_ALIAS 512

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

typedef struct {
	int  clients[CLIENT_MAX];
	char aliases[CLIENT_MAX][CLIENT_MAX_ALIAS];
} Users;

static int 
remove_client(int fd, int *clients, int client_size, int *client_count) {
	for(int i=0; i < client_size; i++) {
		if (clients[i] == -1)
			continue;

		if (clients[i] == fd) {
			clients[i] = -1;
			client_count++; 
			return 0;
		}
	}
	
	return -1;
}

static int 
add_client(int fd, int *clients, int client_size, int *client_count) {
	for(int i=0; i < client_size; i++) {
		if (clients[i] != -1)
			continue;

		clients[i] = fd;
		client_count--;
		return 0;
	}
	
	return -1;
}

static ssize_t
sock_send(int fd, char *buf, ssize_t bufsize) {
	assert((fd >= 0) && "int fd missing in sock_send");
	assert((buf != NULL) && "char *buf missing in sock_send");
	assert((bufsize>1) && "ssize_t bufsize missing in sock_send");

	int e;
	ssize_t n = 0, sent = 0;
	while (sent < bufsize) {
		n = send(
			fd, 
			buf+sent,
			bufsize - sent,
			MSG_NOSIGNAL
		);

		if (n < 0) {
			e = errno;	
	
			switch (e) {
				case EINTR:
					continue;
				case EAGAIN:
				#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
					case EWOULDBLOCK:
				#endif
					printf("Unhandled EAGAIN\n");
					break;
				case EBADF:
				case EFAULT:
				case EINVAL:
				case ENOTSOCK:
				case EOPNOTSUPP:
					printf("Caller error\n");
					break;
				case EPIPE:
				case ECONNRESET:
				case ENOTCONN:
				case ETIMEDOUT:
				case EIO:
				case ENOBUFS:
					printf("Peer error\n");
					break;
				default:
					printf("Unexpected error type\n");
					break;
			}

			perror("send");
			return -1;
		}

		if (n == 0)
			return 0;

		sent += n;
	}
	return 0;
}	

typedef int(*command_handler)(const char *msg, const ssize_t msg_size, Users *users, int user_id);

// TODO: printfs are send to users instead
int change_name(const char *msg, const ssize_t msg_size, Users *users, int user_id) {
	if (msg_size < 2 || isblank((int)msg[0]) == 0) {
		printf("/change_name: missing name or incorrect formatting\n");
		return 1;
	}
	
	const ssize_t msg_size_no_space = msg_size-1;
	if ((msg_size_no_space) >= CLIENT_MAX_ALIAS) {
		printf("/change_name: name too long\n");
		return 1;
	}	
	
	char msg_cp[CLIENT_MAX_ALIAS];
	int cp_err = snprintf(msg_cp, ARRAY_SIZE(msg_cp), "%s", msg+1);
	if (cp_err < 0) {
		printf("/change_name: internal error\n");
		return 1;
	}

	ssize_t msg_cp_size = msg_size_no_space;
	if (msg_cp[msg_cp_size-1] == '\n') {
		msg_cp[msg_cp_size-1] = '\0';
		msg_cp_size--;
	}

	for (ssize_t i=0; i < msg_cp_size; i++) {
		int curr_char = (int)msg_cp[i];

		if (curr_char == 0) 
			break;

		if (isalpha(curr_char) == 0 && isalnum(curr_char) == 0) {
			printf("/change_name: name must consist of only letters and numbers\n");
			printf("user input: %s\n", msg_cp+1);

			return 1;
		}	
	}

	memset(users->aliases[user_id], '\0', CLIENT_MAX_ALIAS); 
	int err = snprintf(users->aliases[user_id], CLIENT_MAX_ALIAS, "%s", msg_cp);  
	if (err < 0) {
		printf("/change_name: name change fail\n");
		return 1;
	}
	
	return 0;
}

// TODO: show the name to the user
int show_name(const char *msg, const ssize_t msg_size, Users *users, int user_id) { 
	(void)msg;
	(void)msg_size;

	char *alias = users->aliases[user_id];
	
	if (alias[0] == '\0') {
		const char msg_fail[] = "/show_name: name not set\n";
		printf("%s", msg_fail);
		ssize_t send_err = sock_send(users->clients[user_id], (char *)msg_fail, ARRAY_SIZE(msg_fail));
		if (send_err == -1)
			return 1; 
		// differentiation will make more sense after I'll change the way the function exits
		return 1;
	}

	const char *msg_header = "/show_name: ";
	ssize_t msg_header_size = strlen(msg_header);
	
	ssize_t msg_succ_size = msg_header_size + CLIENT_MAX_ALIAS + 1;
	char msg_succ[msg_succ_size];
	memset(msg_succ, 0, sizeof(msg_succ));

	snprintf(msg_succ, msg_succ_size, "%s%s\n", msg_header, alias);
	sock_send(users->clients[user_id], msg_succ, msg_succ_size);
	printf("%s", msg_succ);

	return 0;
}

int remove_name(const char *msg, const ssize_t msg_size, Users *users, int user_id) {
	(void)msg;
	(void)msg_size;

	memset(users->aliases[user_id], '\0', CLIENT_MAX_ALIAS); 
		
	if(users->aliases[user_id][0] != '\0') {
		printf("/change_name: name deletion fail\n");
		return 1;
	}
	
	return 0;
}

typedef struct {
	char *name;
	command_handler handler;
} Command;

Command commands[] = {
	{ .name = "/change_name", .handler = change_name},
	{ .name = "/show_name",   .handler = show_name},
	{ .name = "/remove_name", .handler = remove_name},
};

/*
 * Returns whether the message should be further processed.
 */
static int
maybe_process_command(const char *msg, const ssize_t msg_size, Users *users, int user_id) {
	if (user_id < 0 || user_id >= CLIENT_MAX)
		return 1;

	if ((msg_size < 1) || (msg[0] == '\0'))
		return 0;

	for (size_t i=0; i<ARRAY_SIZE(commands); i++) {
		const char *cmd_name = commands[i].name;
		size_t name_len = strlen(cmd_name);

		if (cmd_name[0] == '/' && (size_t)msg_size >= name_len && (strncmp(msg, cmd_name, name_len) == 0)) {
			return commands[i].handler(
				msg + name_len, 
				msg_size - name_len, 
				users, 
				user_id
			);
		}	
	}
	
	return 0;
}

static ssize_t
sock_read(int fd, char *buf, ssize_t bufsize) {
	assert((fd >= 0) && "int fd missing in sock_read");
	assert((buf != NULL) && "char *buf missing in sock_read");
	assert((bufsize>1) && "ssize_t bufsize missing in sock_read");

	int e;
	ssize_t n = 0, filled = 0;
	while (filled < bufsize) {
		n = read(
			fd, 
			buf+filled,
			bufsize - filled
		);

		if (n < 0) {
			e = errno;	
			if (e == EINTR) 
				continue;

			if (e == EAGAIN) {
				return filled;
			}
			
			#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
				if (e == EWOULDBLOCK) {
					return filled;
				}
			#endif

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

	// TODO: use it
	struct rlimit limit;
	if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
		perror("getrlimit");
	}

	Users users = {0};
	int   client_count = 0;

	int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd == -1) {
		perror("socket");
		return 1;
	}
	printf("%d\n", fd);
	unlink(SOCKET_PORT);

	struct sockaddr_un server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, SOCKET_PORT, sizeof(server_addr.sun_path));

	if(bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==  -1) {
		perror("bind");
		close(fd);
		return 1;
	}

	if(listen(fd, CLIENT_MAX) == -1) {
		printf("Connection queque limit reached.\nConnection refused.\n");
	}

	for(size_t i=0; i<(ARRAY_SIZE(users.clients)); i++)
		users.clients[i] = -1;

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
	ssize_t read_size, send_err;

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

			printf("EVENT: fd=%02x ev=%04x\n", event_fd, new_ev);

			if (event_fd == 0 || new_ev == 0)
				continue;
			
			if (event_fd == fd) {
				new_sock = accept4(event_fd, NULL, NULL, SOCK_NONBLOCK);
				if (new_sock == -1) {
					perror("accept");
					close(new_sock);
					continue;
				}
			
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLIN;
				ev.data.fd = new_sock;
				if (epoll_ctl(
					epollfd,
					EPOLL_CTL_ADD,
					new_sock,
					&ev)
					== -1) {
					perror("epoll_ctl_add");
					close(new_sock);
				}

				if (add_client(
					new_sock, 
					users.clients, 
					ARRAY_SIZE(users.clients),
					&client_count
					) != 0) {
					printf("Add_client: failed to add a new client\n");	
				}

				if (new_ev & EPOLLERR) {
					// should never happen
					printf("LISTENING SOCK: epoll err\n");
					return 1;
				}
				continue;	
			}

			memset(
				read_res, 
				0, 
				sizeof(read_res)
			);

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
			
			int user_id = -1;
			for(int i=0; i < CLIENT_MAX; i++) {
				if (users.clients[i] != event_fd) 
					continue;
				
				user_id = i;
			}

			if (maybe_process_command(read_res, read_size, &users, user_id) != 0) {
				return 1;
			}

			memset(
				return_msg, 
				0, 
				sizeof(return_msg)
			);

			snprintf(
				return_msg, 
				sizeof(return_msg),
				"%d: %s",
				event_fd, 
				read_res
			);
		
			for(size_t i=0; i<(ARRAY_SIZE(users.clients)); i++) {
				if (users.clients[i] == -1)
					continue;

				if (users.clients[i] == event_fd)
					continue;

				send_err = sock_send(
					users.clients[i], 
					return_msg, 
					BUF_SIZE
				);
				if (send_err == -1) {
					epoll_ctl(
						epollfd, 
						EPOLL_CTL_DEL, 
						users.clients[i],
						0
					);
					
					if (remove_client(
						users.clients[i], 
						users.clients, 
						ARRAY_SIZE(users.clients),
						&client_count
						) != 0) {
						printf("Remove_client: client not removed\n");
						return 1;
					}
						
					close(event_fd);
				}
			}
		}
	}
	
	close(fd);
	return 0;
}
