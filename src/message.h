/* $Id: message.h,v 1.9 2004/03/16 19:30:25 mmazur Exp $ */

/*
 *  (C) Copyright 2002-2005 Jacek Konieczny [jajcus(a)jajcus,net]
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
#include "sessions.h"

struct stream_s;

int message_send(struct stream_s *stream,const char *from,
		const char *to,int chat,const char *message, time_t timestamp);

int message_send_subject(struct stream_s *stream,const char *from,
		const char *to,const char *subject,const char *message, time_t timestamp);

int message_error(struct stream_s *stream,const char *from,
		const char *to,const char *body,int code, time_t timestamp);

int jabber_message(struct stream_s *stream,xmlnode tag);

struct request_s;

void get_roster_done(Session *s,struct gg_event *event);

#endif
