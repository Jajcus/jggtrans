#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#include "stream.h"
#include "fds.h"
#include "debug.h"
#include <glib.h>

#define MAX_WRITE_BUF 102400
#define MAX_READ_BUF 102400

int stream_fd_handler(FdHandler *h);
int stream_write_hello(Stream *s);
	
int stream_set_nonblocking(Stream *s,int nonblock){
int oldflags;

	oldflags=fcntl(s->fd, F_GETFL, 0);
	if (oldflags==-1){
		perror("fcntl(s->fd,F_GETFL,0)");
		return -1;
	}
	if (nonblock) 
		oldflags |= O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;
	if (fcntl(s->fd, F_SETFL, oldflags)<0){
		perror("fcntl(s->fd,F_SETFL,0)");
		return -1;
	}
	return 0;
}

Stream *stream_connect(char *host,int port,int nonblock,xstream_onNode cb){
Stream *s;
struct hostent *he;
int optval;
int r;
FdHandler *h;

	s=(Stream *)g_malloc(sizeof(Stream));
	memset(s,0,sizeof(Stream));
	s->fd=socket(PF_INET,SOCK_STREAM,0);
	if (!s->fd){
		perror("socket");
		g_free(s);
		return NULL;
	}
	s->sa.sin_family=AF_INET;	
	s->sa.sin_port=htons(port);	
	he=gethostbyname(host);
	if (he == NULL){
		g_warning("Unknown host: %s",host);
		g_free(s);
		return NULL;
	}
	s->sa.sin_addr = *(struct in_addr *) he->h_addr;
	optval=1;
	r=setsockopt(s->fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
	if (r<0){
		g_warning("setsockopt(s->fd,SOL_SOCKET,SO_REUSEADDR...): %s",g_strerror(errno));
		g_free(s);
		return NULL;
	}
	
	optval=1;
	r=setsockopt(s->fd,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
	if (r<0){
		g_warning("setsockopt(s->fd,SOL_SOCKET,SO_KEEPALIVE...): %s",g_strerror(errno));
		g_free(s);
		return NULL;
	}
	
	if (nonblock)
		if (stream_set_nonblocking(s,1)){
			g_free(s);
			return NULL;
		}
	
	r=connect(s->fd, (struct sockaddr *) &s->sa, sizeof (s->sa));
	if (r<0 && nonblock && errno==EINPROGRESS){
		s->connecting=1;
	}
	else {
		if (r<0){
			perror("connect");
			g_free(s);
			return NULL;
		}
		s->connected=1;
	}
	s->dest=g_strdup(host); /* FIXME */
	h=(FdHandler *)g_malloc(sizeof(FdHandler));
	g_assert(h!=NULL);
	memset(h,0,sizeof(*h));
	s->h=h;
	h->fd=s->fd;
	h->func=stream_fd_handler;
	h->extra=s;
	h->read=1;
	if (s->connecting) h->write=1;
	s->xs=xstream_new(pool_new(),cb,s);
	fd_register_handler(h);
	if (s->connected) stream_write_hello(s);
	return s;
}

int stream_connect_cont(Stream *s,int nonblock){
int r;

	if (s->connected) return 0;
	if (!s->connecting) return -1;
	r=connect(s->fd, (struct sockaddr *) &s->sa, sizeof (s->sa));
	if (r==0){
		s->connected=1;
		s->connecting=0;
		if (!nonblock)
			if (stream_set_nonblocking(s,0)){
				return -1;
			}
		return 0;
	}
	if (errno==EALREADY) return 0;
	perror("connect");
	return -1;
}

int stream_fd_handler(FdHandler *h){
Stream *s;
char * str;
int r;

	s=(Stream *)h->extra;
	if (s->connecting){
		if (!h->write_ready) return 0;	
		r=stream_connect_cont(s,0);
		if (r){ 
			s->xs->f(XSTREAM_CLOSE,NULL,s);
			return r;
		}
		h->write=0;
		stream_write_hello(s);
		return 0;
	}

	if (!s->connected) return -1;
	
	if (h->read_ready){
		if (!s->read_buf){
			s->read_buf=(char *)g_malloc(1025);
			g_assert(s->read_buf!=0);
			s->read_buf_len=1024;
		}
		r=read(h->fd,s->read_buf,s->read_buf_len);
		if (r==0){
			s->xs->f(XSTREAM_CLOSE,NULL,s);
			return 1;
		}
		if (r==-1 && errno==EINTR) return 0;
		if (r==-1){
			perror("read");
			return 1;
		}
		s->read_buf[r]=0;
		debug("IN: %s",s->read_buf);
		r=xstream_eat(s->xs,s->read_buf,r);
	}
	if (h->write_ready){
		if (s->write_buf && s->write_pos>=0 && s->write_len>=0){
			r=write(s->fd,s->write_buf+s->write_pos,s->write_len-s->write_pos);
			if (!r){
				s->xs->f(XSTREAM_CLOSE,NULL,s);
				return 1;
			}
			if (r==-1 && errno==EINTR) return 0;
			if (r==-1){
				g_warning("write: %s", g_strerror(errno));
				return 1;
			}
			str=(char *)g_malloc(r+1);
			g_assert(str!=NULL);
			memcpy(str,s->write_buf+s->write_pos,r);
			str[r]=0;
			debug("OUT: %s",str);
			s->write_pos+=r;
			if (s->write_pos==s->write_len){
				s->write_len=0;
				s->write_pos=-1;
				s->h->write=0;
			}
		}
	}
	return 0;
}

int stream_write_bytes(Stream *s,const char *buf,int l){

	if (!l) return 0;
	g_assert(buf!=NULL);
	g_assert(l>=0);
	if (s->write_buf_len+l > MAX_WRITE_BUF){
		return -2;
	}
	if (!s->write_buf){
		s->write_buf=(char *)g_malloc(1024);
		g_assert(s->write_buf!=NULL);
		s->write_buf_len=1024;
	}
	else if (s->write_len+l > s->write_buf_len){
		s->write_buf_len+=1024*((l+1023)/1024);
		s->write_buf=(char *)g_realloc(s->write_buf,s->write_buf_len);
		g_assert(s->write_buf!=NULL);
	}
	memcpy(s->write_buf+s->write_len,buf,l);
	s->write_len+=l;
	if (s->write_pos<0) s->write_pos=0;
	s->h->write=1;
	return 0;
}

int stream_write_str(Stream *s,const char *str){

	return stream_write_bytes(s,str,strlen(str));
}

int stream_write(Stream *s,xmlnode xn){
char *str;

	str=xmlnode2str(xn);
	if (!str) return -1;
	return stream_write_bytes(s,str,strlen(str));
}

int stream_write_hello(Stream *s){
	return stream_write_str(s,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>")
		|| stream_write_str(s,"<stream:stream to='")
		|| stream_write_str(s,s->dest)
		|| stream_write_str(s,"' xmlns='jabber:component:accept' xmlns:stream='http://etherx.jabber.org/streams'>");
};

int stream_close(Stream *s){

	if (!s->closing){ 
		s->closing=1;
		return stream_write_str(s,"</stream:stream>");
	}
	return 0;
}

int stream_destroy(Stream *s){

	g_assert(s!=NULL);
	if (!s->closing){
		write(s->fd,"</stream:stream>",sizeof("</stream:stream>"));
		s->closing=1;
	}
	fd_unregister_handler(s->h);
	close(s->fd);
	if (s->read_buf) free(s->read_buf);
	if (s->write_buf) free(s->write_buf);
	pool_free(s->xs->p);
	free(s);
	return 0;
}

