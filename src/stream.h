/* $Id: stream.h,v 1.7 2003/01/22 07:53:01 jajcus Exp $ */

/*
 *  (C) Copyright 2002-2006 Jacek Konieczny [jajcus(a)jajcus,net]
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

#ifndef stream_h
#define stream_h

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
	xstream xs;
}Stream;

Stream *stream_connect(char *host,int port,int nonblock,xstream_onNode cb);
int stream_close(Stream *s);
int stream_write_str(Stream *s,const char *str);
int stream_write(Stream *s,xmlnode xn);
int stream_destroy(Stream *s);

typedef void (*stream_destroy_handler_t)(Stream *s);
void stream_add_destroy_handler(stream_destroy_handler_t h);
void stream_del_destroy_handler(stream_destroy_handler_t h);

#endif
