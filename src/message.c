/* $Id: message.c,v 1.10 2002/02/26 08:58:24 jajcus Exp $ */

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

#include <stdio.h>
#include "ggtrans.h"
#include "presence.h"
#include "jabber.h"
#include "jid.h"
#include "users.h"
#include "sessions.h"
#include "encoding.h"

int message_send(struct stream_s *stream,const char *from,
		const char *to,int chat,const char *message){
xmlnode msg;
xmlnode n;

	msg=xmlnode_new_tag("message");
	if (from!=NULL) 
		xmlnode_put_attrib(msg,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(msg,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(msg,"to",to);
	if (chat) xmlnode_put_attrib(msg,"type","chat");
	n=xmlnode_insert_tag(msg,"body");
	xmlnode_insert_cdata(n,to_utf8(message),-1);
	stream_write(stream,msg);
	xmlnode_free(msg);
	return 0;
}

int message_send_error(struct stream_s *stream,const char *from,
		const char *to,const char *body,int code,const char *str){
xmlnode msg;
xmlnode n;
char *s;

	msg=xmlnode_new_tag("message");
	if (from!=NULL) 
		xmlnode_put_attrib(msg,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(msg,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(msg,"to",to);
	xmlnode_put_attrib(msg,"type","error");
	s=g_strdup_printf("%03u",code);
	xmlnode_put_attrib(msg,"code",s);
	g_free(s);
	n=xmlnode_insert_tag(msg,"error");
	xmlnode_insert_cdata(n,str,-1);
	if (body) {
		n=xmlnode_insert_tag(msg,"body");
		xmlnode_insert_cdata(n,body,-1);
	}
	stream_write(stream,msg);
	xmlnode_free(msg);
	return 0;
}


int jabber_message(struct stream_s *stream,xmlnode tag){
char *type;
char *from;
char *to;
char *subject;
char *body;
int chat;
xmlnode subject_n;
xmlnode body_n;
Session *s;

	body_n=xmlnode_get_tag(tag,"body");
	if (body_n) body=xmlnode_get_data(body_n);
	else body=NULL;
	
	subject_n=xmlnode_get_tag(tag,"subject");
	if (subject_n) subject=xmlnode_get_data(subject_n);
	else subject=NULL;
	
	type=xmlnode_get_attrib(tag,"type");
	if (!type || !strcmp(type,"normal")) chat=0;
	else if (!strcmp(type,"chat")) chat=1;
	else if (!strcmp(type,"error")) {
		g_warning("Error message received: %s",xmlnode2str(tag));
		return 0;
	}
	else {
		g_warning("Unsupported message type");
		message_send_error(stream,to,from,body,500,"Internal Server Error"); 
		return -1;
	}

	from=xmlnode_get_attrib(tag,"from");
	to=xmlnode_get_attrib(tag,"to");
	if (!to || !jid_is_my(to) || !jid_has_uin(to)){
		g_warning("Bad 'to' in: %s",xmlnode2str(tag));
		message_send_error(stream,to,from,body,400,"Bad Request"); 
		return -1;
	}
	
	if (!jid_has_uin(to)){
		g_warning("Bad 'to' in: %s",xmlnode2str(tag));
		message_send_error(stream,to,from,body,404,"Not Found"); 
		return -1;
	}

	if (!from){
		g_warning("Anonymous message? %s",xmlnode2str(tag));
		message_send_error(stream,to,from,body,400,"Bad Request"); 
		return -1;
	}
	
	s=session_get_by_jid(from,NULL);
	if (!s || !s->connected){
		g_warning("%s not logged in. While processing %s",from,xmlnode2str(tag));
		message_send_error(stream,to,from,body,407,"Not logged in"); 
		return -1;
	}	
	
	if (subject)
		body=g_strdup_printf("Subject: %s\n%s",subject,body);
	session_send_message(s,jid_get_uin(to),chat,from_utf8(body));
	if (subject)
		g_free(body);
	
	return 0;
}

