#ifndef fds_h
#define fds_h

struct fd_handler_s;
typedef int (*fd_handler_func)(struct fd_handler_s *h);

typedef struct fd_handler_s{
	int fd;
	fd_handler_func func;
	int read;
	int write;
	int read_ready;
	int write_ready;
	void *extra;
}fd_handler;

int fd_init();

int fd_register_handler(fd_handler *h);

int fd_unregister_handler(fd_handler *h);

int fd_watch(int timeout);

#endif
