#include "fds.h"
#include <glib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static int max_fd;
static GList* handlers;

int fd_init(){
	max_fd=0;
	handlers=NULL;
	return 0;
}

int fd_register_handler(fd_handler *h){
	
	assert(h!=NULL);
	if (h->fd>max_fd) max_fd=h->fd;
	handlers=g_list_append(handlers,(gpointer)h);
	assert(handlers!=NULL);
	return 0;
}

int fd_unregister_handler(fd_handler *h){

	handlers=g_list_remove(handlers,(gpointer)h);
	g_free(h);
	return 0;
}

int fd_watch(int timeout){
struct timeval tv;
fd_set rd, wr, ex;
GList *it;
fd_handler *h;
int event;
int except;

	tv.tv_sec=timeout;
	tv.tv_usec=0;
	
	FD_ZERO(&rd);	
	FD_ZERO(&wr);	
	FD_ZERO(&ex);	

	for(it=g_list_first(handlers);it;it=it->next){
		h=(fd_handler *)it->data;
		assert(h!=NULL);
		if (h->read)
			FD_SET(h->fd,&rd);
		if (h->write)
			FD_SET(h->fd,&wr);
		FD_SET(h->fd, &ex);
	}
	if (select(max_fd + 1, &rd, &wr, &ex, &tv) == -1){
		return -1;
	}
	for(it=g_list_first(handlers);it;it=it->next){
		h=(fd_handler *)it->data;
		assert(h!=NULL);
		event=0;
		if (FD_ISSET(h->fd,&rd)){
			event=1;
			h->read_ready=1;
		}
		else h->read_ready=0;
		if (FD_ISSET(h->fd,&wr)){
			event=1;
			h->write_ready=1;
		}
		else h->write_ready=0;
		if (FD_ISSET(h->fd,&ex)){
			fprintf(stderr,"Exception on fd=%i\n",h->fd);
			except=1;
		}	
		else except=0;
		if (event){
			h->func(h);
		}
	}
	if (except) return 1;
	return 0;
}

