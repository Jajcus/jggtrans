/* $Id: search.h,v 1.5 2003/02/03 20:28:19 mmazur Exp $ */

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

#ifndef search_h
#define search_h

struct request_s;
struct stream_s;

int search_done(struct request_s *r, gg_pubdir50_t results);
void jabber_iq_get_search(struct stream_s *s,const char *from,const char *to,const char *id,xmlnode q);
void jabber_iq_set_search(struct stream_s *s,const char *from,const char *to,const char *id,xmlnode q);

int vcard_done(struct request_s *r, gg_pubdir50_t results);
void jabber_iq_get_user_vcard(struct stream_s *s,const char *from,const char * to,const char *id,xmlnode q);

#endif
