/* $Id: stream.c,v 1.24 2004/04/14 11:00:41 jajcus Exp $ */

/*
 *  (C) Copyright 2002 Jacek Konieczny <jajcus@pld.org.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#include "ggtrans.h"
#include "stream.h"
#include "jabber.h"
#include "debug.h"

#define MAX_WRITE_BUF 102400
#define MAX_READ_BUF 102400
#define WRITE_BUF_FLUSH_TIMEOUT 10

GList* destroy_handlers;

int stream_io_error(GIOChannel *source,GIOCondition condition,gpointer data);
int stream_io_connect(GIOChannel *source,GIOCondition condition,gpointer data);
int stream_io_read(GIOChannel *source,GIOCondition condition,gpointer data);
int stream_io_write(GIOChannel *source,GIOCondition condition,gpointer data);
int stream_write_hello(Stream *s);

int stream_set_nonblocking(Stream *s,int nonblock){
int oldflags;
int fd;

	fd=g_io_channel_unix_get_fd(s->ioch);
	oldflags=fcntl(fd, F_GETFL, 0);
	if (oldflags==-1){
		g_warning("fcntl(fd,F_GETFL,0): %s",g_strerror(errno));
		return -1;
	}
	if (nonblock)
		oldflags |= O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, oldflags)<0){
		g_warning("fcntl(fd,F_SETFL,0): %s",g_strerror(errno));
		return -1;
	}
	return 0;
}

Stream *stream_connect(char *host,int port,int nonblock,xstream_onNode cb){
Stream *s;
struct hostent *he;
int optval;
int r;
int fd;

	s=g_new0(Stream,1);
	fd=socket(PF_INET,SOCK_STREAM,0);
	if (!fd){
		g_error(L_("socket: %s"),g_strerror(errno));
		g_free(s);
		return NULL;
	}
	s->ioch=g_io_channel_unix_new(fd);
	g_io_channel_set_encoding(s->ioch,NULL,NULL);
	g_io_channel_set_buffered(s->ioch,0);
	s->sa.sin_family=AF_INET;
	s->sa.sin_port=htons(port);
	he=gethostbyname(host);
	if (he == NULL){
		g_warning(N_("Unknown host: %s"),host);
		g_free(s);
		return NULL;
	}
	s->sa.sin_addr = *(struct in_addr *) he->h_addr;
	optval=1;
	r=setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
	if (r<0){
		g_warning("setsockopt(fd,SOL_SOCKET,SO_REUSEADDR...): %s",g_strerror(errno));
		g_free(s);
		return NULL;
	}

	optval=1;
	r=setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
	if (r<0){
		g_warning("setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE...): %s",g_strerror(errno));
		g_free(s);
		return NULL;
	}

	if (nonblock)
		if (stream_set_nonblocking(s,1)){
			g_free(s);
			return NULL;
		}

	r=connect(fd, (struct sockaddr *) &s->sa, sizeof (s->sa));
	if (r<0 && nonblock && errno==EINPROGRESS){
		s->connecting=1;
	}
	else{
		if (r<0){
			g_error(L_("connect: %s"),g_strerror(errno));
			g_free(s);
			return NULL;
		}
		s->connected=1;
	}
	s->dest=g_strdup(host); /* FIXME */
	s->err_watch=g_io_add_watch(s->ioch,G_IO_ERR|G_IO_HUP|G_IO_NVAL,stream_io_error,s);
	if (s->connecting)
		s->write_watch=g_io_add_watch(s->ioch,G_IO_OUT,stream_io_connect,s);
	else
		s->read_watch=g_io_add_watch(s->ioch,G_IO_IN,stream_io_read,s);
	s->xs=xstream_new(pool_new(),cb,s);
	if (s->connected) stream_write_hello(s);
	return s;
}

int stream_connect_cont(Stream *s,int nonblock){
int r;

	if (s->connected) return 0;
	if (!s->connecting) return -1;
	r=connect(g_io_channel_unix_get_fd(s->ioch),
			(struct sockaddr *) &s->sa, sizeof (s->sa));
	if (r==0 || errno==EISCONN){
		s->connected=1;
		s->connecting=0;
		if (nonblock)
			if (stream_set_nonblocking(s,0)){
				return -1;
			}
		return 0;
	}
	if (errno==EALREADY) return 0;
	g_critical(L_("connect: %s"),g_strerror(errno));
	return -1;
}

int stream_io_error(GIOChannel *source,GIOCondition condition,gpointer data){
Stream *s;

	s=(Stream *)data;
	g_assert(s);
	if (s->err_watch) g_source_remove(s->err_watch);
	s->err_watch=0;
	s->xs->f(XSTREAM_CLOSE,NULL,s);
	if (!s->connected)
		g_critical(L_("Couldn't connect to jabber server"));
	else
		g_critical(L_("Connection to jabber server broken"));
	do_restart=TRUE;
	return FALSE;
}

int stream_io_connect(GIOChannel *source,GIOCondition condition,gpointer data){
Stream *s;
int r;

	debug("stream_io_connect()");
	s=(Stream *)data;
	g_assert(s);
	g_assert(s->connecting);
	r=stream_connect_cont(s,0);
	if (r){
		s->write_watch=0;
		s->xs->f(XSTREAM_CLOSE,NULL,s);
		return FALSE;
	}
	s->write_watch=0;
	s->read_watch=g_io_add_watch(s->ioch,G_IO_IN,stream_io_read,s);
	stream_write_hello(s);
	return FALSE;
}

int stream_io_read(GIOChannel *source,GIOCondition condition,gpointer data){
Stream *s;
GIOStatus st;
GError *error=NULL;
gsize br;

	s=(Stream *)data;
	g_assert(s);

	if (!s->connected) g_error(L_("Unconnected stream"));

	if (!s->read_buf){
		s->read_buf=g_new(char,1025);
		g_assert(s->read_buf!=0);
		s->read_buf_len=1024;
	}
	st=g_io_channel_read_chars(source,s->read_buf,s->read_buf_len,&br,&error);
	if (st!=G_IO_STATUS_NORMAL){
		g_warning("read: %i %s",st,error?error->message:"unknown");
		if (error) g_error_free(error);
		error=NULL;
		s->read_watch=0;
		s->xs->f(XSTREAM_CLOSE,NULL,s);
		return FALSE;
	}
	if (error) g_error_free(error);
	error=NULL;
	if (st==G_IO_STATUS_AGAIN) return TRUE;
	s->read_buf[br]=0;
	debug("IN: %s",s->read_buf);
	xstream_eat(s->xs,s->read_buf,br);
	return TRUE;
}

int stream_io_write(GIOChannel *source,GIOCondition condition,gpointer data){
Stream *s;
char * str;
GIOStatus st;
GError *error=NULL;
gsize br;

	s=(Stream *)data;
	g_assert(s);

	if (s->write_buf && s->write_pos>=0 && s->write_len>=0){
		st=g_io_channel_write_chars(source,
					s->write_buf+s->write_pos,
					s->write_len-s->write_pos,
					&br,
					&error);
		if (st==G_IO_STATUS_ERROR){
			g_warning("write error: %s",error?error->message:"unknwown");
			s->write_watch=0;
			s->xs->f(XSTREAM_CLOSE,NULL,s);
			if (error) g_error_free(error);
			return FALSE;
		}
		if (st==G_IO_STATUS_AGAIN) return TRUE;
		if (st!=G_IO_STATUS_NORMAL){
			g_warning("write status: %i (%s)",st,error?error->message:"unknown");
			if (error) g_error_free(error);
			return TRUE;
		}
		if (error) g_error_free(error);
		error=NULL;
		str=g_new(char,br+1);
		g_assert(str!=NULL);
		memcpy(str,s->write_buf+s->write_pos,br);
		str[br]=0;
		debug("OUT: %s",str);
		g_free(str);
		s->write_pos+=br;
		debug("write_pos: %i, write_len: %i, br: %i\n",s->write_pos,s->write_len,(int)br);
		if (s->write_pos==s->write_len){
			s->write_watch=0;
			s->write_len=0;
			s->write_pos=-1;
			return FALSE;
		}
	}
	return TRUE;
}

int stream_write_bytes(Stream *s,const char *buf,int l){
time_t stream_flush_timeout;
GIOStatus st;
GError *error=NULL;

	if (!l) return 0;
	g_assert(buf!=NULL);
	g_assert(l>=0);
	if (s->write_len+l > MAX_WRITE_BUF){
		stream_flush_timeout=time(NULL)+WRITE_BUF_FLUSH_TIMEOUT;
		if (s->write_len){
			debug("Flushing stream write buffer, as it is full (%i characters, %i to be added)",s->write_len,l);
			/* block to flush the buffer */
			while(time(NULL)<stream_flush_timeout){
				if (!g_main_is_running(main_loop)) break;
				stream_io_write(s->ioch,G_IO_OUT,(gpointer)s);
				if (s->write_len==0) break;
			}
		}
		if (!s->write_len && l>MAX_WRITE_BUF){
			int pos=0;
			gsize written;
			char *str;
			debug("Flushing data that don't fit into the write buffer.");
			while(pos<l){
				st=g_io_channel_write_chars(s->ioch,
						(gchar *)buf+pos,
						l-pos,
						&written,
						&error);
				if (st!=G_IO_STATUS_AGAIN && st!=G_IO_STATUS_NORMAL){
					g_warning("write: %i: %s",st,error?error->message:"unknown");
					if (error) g_error_free(error);
					return -2;
				}
				if (error) g_error_free(error);
				error=NULL;
				str=g_new(char,written+1);
				g_assert(str!=NULL);
				memcpy(str,buf+pos,written);
				str[written]=0;
				debug("OUT: %s",str);
				g_free(str);
				pos+=written;
			}
			return 0;
		}
		if (s->write_len+l > MAX_WRITE_BUF){
			debug("Write buffer overflow!");
			return -2;
		}
	}
	if (!s->write_buf){
		s->write_buf=g_new(char,1024);
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
	if (!s->write_watch) s->write_watch=g_io_add_watch(s->ioch,G_IO_OUT,stream_io_write,s);
	return 0;
}

int stream_write_str(Stream *s,const char *str){

	return stream_write_bytes(s,str,strlen(str));
}

int stream_write(Stream *s,xmlnode xn){
char *str;

	str=xmlnode2str(xn);
	if (!str) return -1;
	packets_out++;
	return stream_write_bytes(s,str,strlen(str));
}

int stream_write_hello(Stream *s){
	return stream_write_str(s,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>")
		|| stream_write_str(s,"<stream:stream to='")
		|| stream_write_str(s,my_name)
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
GList *it;

	g_assert(s!=NULL);
	for(it=destroy_handlers;it;it=it->next){
		stream_destroy_handler_t h=
			(stream_destroy_handler_t)it->data;
		h(s);
	}
	if (!s->closing){
		char goodbye[]="</stream:stream>";
		guint i,l;
		gsize br;
		GIOStatus st;
		GError *error=NULL;

		i=0;
		l=sizeof(goodbye);
		do{
			st=g_io_channel_write_chars(s->ioch,goodbye+i,l,&br,&error);
			if (error) g_error_free(error);
			error=NULL;
			l-=br;
			i+=br;
		}while(st==G_IO_STATUS_AGAIN||(st=G_IO_STATUS_NORMAL && l>0));
		s->closing=1;
	}
	if (s->err_watch) g_source_remove(s->err_watch);
	if (s->read_watch) g_source_remove(s->read_watch);
	if (s->write_watch) g_source_remove(s->write_watch);
	if (s->read_buf) free(s->read_buf);
	if (s->write_buf) free(s->write_buf);
	pool_free(s->xs->p);
	g_io_channel_close(s->ioch);
	free(s);
	return 0;
}

void stream_add_destroy_handler(stream_destroy_handler_t h){

	destroy_handlers=g_list_append(destroy_handlers,h);
}

void stream_del_destroy_handler(stream_destroy_handler_t h){

	destroy_handlers=g_list_remove(destroy_handlers,h);
}
