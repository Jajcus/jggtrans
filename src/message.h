/* $Id: message.h,v 1.5 2003/03/24 13:46:49 jajcus Exp $ */

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

#ifndef message_h
#define message_h

#include "jabber.h"

struct stream_s;

int message_send(struct stream_s *stream,const char *from,
		const char *to,int chat,const char *message, time_t timestamp);

int message_error(struct stream_s *stream,const char *from,
		const char *to,const char *body,int code, time_t timestamp);

int jabber_message(struct stream_s *stream,xmlnode tag);

#ifdef REMOTE_USERLIST
struct request_s;

void get_roster_error(struct request_s *r);
void get_roster_done(struct request_s *r);
void put_roster_error(struct request_s *r);
void put_roster_done(struct request_s *r);
#endif

#endif
