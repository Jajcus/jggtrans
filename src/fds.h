#ifndef fds_h
#define fds_h

struct fd_handler_s;
typedef int (*FdHandlerFunc)(struct fd_handler_s *h);

typedef struct fd_handler_s{
	int fd;
	FdHandlerFunc func;
	int read;
	int write;
	int read_ready;
	int write_ready;
	void *extra;
}FdHandler;

int fd_init();

int fd_register_handler(FdHandler *h);

int fd_unregister_handler(FdHandler *h);

int fd_watch(int timeout);

#endif
