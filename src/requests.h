/* $Id: requests.h,v 1.6 2002/01/30 16:52:03 jajcus Exp $ */

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

struct request_s;

typedef enum request_type_e{
	RT_NONE=0,
	RT_SEARCH,
	RT_VCARD,
	RT_CHANGE
}RequestType;

typedef struct request_s{
	char *from; /* jid of requesting user */
	char *to; /* target of user request */
	char *id;  /* ID if user request (<iq/>) */
	xmlnode query; /* The query */
	RequestType type;
	
	struct gg_http* gghttp; 
	GIOChannel *ioch;
	guint io_watch;

	struct stream_s *stream;
}Request;

Request * add_request(RequestType type,const char *from,const char *to,
			const char *id,xmlnode query,struct gg_http *gghttp,
			struct stream_s *stream);
int remove_request(Request *r);

#endif
