/* $Id: iq.h,v 1.8 2003/01/15 08:04:56 jajcus Exp $ */

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

#ifndef iq_h
#define iq_h

struct stream_s;
void jabber_iq(struct stream_s *s,xmlnode x);
void jabber_iq_send_error(struct stream_s *s,const char *was_from,const char *was_to,const char *id,int code,char *string);
void jabber_iq_send_result(struct stream_s *s,const char *was_from,const char *was_to,const char *id,xmlnode content);

typedef void (*IqHandler)(struct stream_s *s,const char *from, const char *to,const char *id,xmlnode n);
typedef struct iq_namespace_s{
	const char *ns;
	const char *node_name;
	IqHandler get_handler;
	IqHandler set_handler;
}IqNamespace;
extern IqNamespace server_iq_ns[];
extern IqNamespace client_iq_ns[];

#endif
