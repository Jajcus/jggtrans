/* $Id: requests.h,v 1.17 2003/04/14 17:01:18 jajcus Exp $ */

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

#ifndef requests_h
#define requests_h

#include "users.h"
#include "sessions.h"

struct request_s;

typedef enum request_type_e{
	RT_NONE=0,
	RT_SEARCH,
	RT_VCARD,
	RT_CHANGE,
	RT_PASSWD,
	RT_USERLIST_GET
}RequestType;

typedef struct request_s{
	char *from; /* jid of requesting user */
	char *to; /* target of user request */
	char *id;  /* ID if user request (<iq/>) */
	xmlnode query; /* The query */
	RequestType type;

	struct gg_http* gghttp;
	gg_pubdir50_t ggchange;
	void *data;

	GIOChannel *ioch;
	guint io_watch;

	int hash; /* When standard request interface isn't used */

	struct stream_s *stream;
}Request;

Request * add_request(RequestType type,const char *from,const char *to,
			const char *id,xmlnode query, void *data,
			struct stream_s *stream);
int remove_request(Request *r);

void request_response_search(struct gg_event *data);
void request_response_write(struct gg_event *data);
int requests_init();

#endif
