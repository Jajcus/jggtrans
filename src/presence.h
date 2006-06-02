/* $Id: presence.h,v 1.6 2003/04/13 14:36:47 jajcus Exp $ */

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

#ifndef presence_h
#define presence_h

#include "jabber.h"

struct stream_s;

int presence_send_error(struct stream_s *stream,const char *from,
		const char *to,int code,const char *descr);
int presence_send_subscribe(struct stream_s *stream,const char *from,
		const char *to);
int presence_send_unsubscribe(struct stream_s *stream,const char *from,
		const char *to);
int presence_send_subscribed(struct stream_s *stream,const char *from,
		const char *to);
int presence_send_unsubscribed(struct stream_s *stream,const char *from,
		const char *to);
int presence_send(struct stream_s *stream,const char *from,
		const char *to,int available,const char *show,
		const char *status,GTime timestamp);
int presence_send_probe(struct stream_s *stream, const char *from, const char *to);

int jabber_presence(struct stream_s *stream,xmlnode tag);


#endif
