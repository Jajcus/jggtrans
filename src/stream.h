#ifndef stream_h
#define stream_h

#include <libxode.h>
#include <glib.h>

typedef struct stream_s{
	GIOChannel *ioch;	
	guint err_watch;
	guint read_watch;
	guint write_watch;
	
	struct sockaddr_in sa;
	char *dest;
	int listening;
	int connecting;
	int connected;
	int closing;
	char *read_buf;
	int read_buf_len;
	char *write_buf;
	int write_pos;
	int write_buf_len;
	int write_len;
	xstream xs;
}Stream;

Stream *stream_connect(char *host,int port,int nonblock,xstream_onNode cb);
int stream_close(Stream *s);
int stream_write_str(Stream *s,const char *str);
int stream_write(Stream *s,xmlnode xn);
int stream_destroy(Stream *s);

#endif
