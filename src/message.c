/* $Id: message.c,v 1.13 2003/01/14 11:03:03 jajcus Exp $ */

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

#include "ggtrans.h"
#include <stdio.h>
#include "presence.h"
#include "jabber.h"
#include "jid.h"
#include "users.h"
#include "sessions.h"
#include "encoding.h"

typedef void (*MsgHandler)(struct stream_s *s,const char *from, const char *to,const char *args); 
typedef struct iq_namespace_s{
	const char *command;
	const char *abr;
	MsgHandler handler;
}MsgCommand;

void message_get_roster(struct stream_s *s,const char *from, const char *to,const char *args);
void message_put_roster(struct stream_s *s,const char *from, const char *to,const char *args);
void message_friends_only(struct stream_s *s,const char *from, const char *to,const char *args);
void message_invisible(struct stream_s *s,const char *from, const char *to,const char *args);

MsgCommand msg_commands[]={
	{"get roster","gr",message_get_roster},
	{"put roster","pr",message_put_roster},
	{"friends only","fo",message_friends_only},
	{"invisible","iv",message_invisible},
	{NULL,NULL,NULL},
};

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


void message_get_roster(struct stream_s *stream,const char *from, const char *to,const char *args){
User *user;

	user=user_get_by_jid(from);
	message_send(stream,to,from,1,"Receiving roster...");
}

void message_put_roster(struct stream_s *stream,const char *from, const char *to,const char *args){
User *user;

	user=user_get_by_jid(from);
	message_send(stream,to,from,1,"Sending roster...");
}

void message_friends_only(struct stream_s *stream,const char *from, const char *to,const char *args){
Session *session;
User *user;
int on;

	session=session_get_by_jid(from,stream);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);
	if (args && g_strcasecmp(args,"on")==0) on=1;
	else if (args && g_strcasecmp(args,"off")==0) on=0;
	else on=!user->friends_only;

	if (user->friends_only==on){
		message_send(stream,to,from,1,"No change.");
		return;
	}
	user->friends_only=on;

	if (on)
		message_send(stream,to,from,1,"friends only: on");
	else
		message_send(stream,to,from,1,"friends only: off");

	session_send_status(session);
}
	
void message_invisible(struct stream_s *stream,const char *from, const char *to,const char *args){
Session *session;
User *user;
int on;

	session=session_get_by_jid(from,stream);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);
	if (args && g_strcasecmp(args,"on")==0) on=1;
	else if (args && g_strcasecmp(args,"off")==0) on=0;
	else on=!user->invisible;

	if (user->invisible==on){
		message_send(stream,to,from,1,"No change.");
		return;
	}
	user->invisible=on;

	if (on)
		message_send(stream,to,from,1,"invisible: on");
	else
		message_send(stream,to,from,1,"invisible: off");

	session_send_status(session);
}
	
int message_to_me(struct stream_s *stream,const char *from,
		const char *to,const char *body,xmlnode tag){
int i;
const char *ce;
char *args,*p;
User *user;

	user=user_get_by_jid(from);
	if (user==NULL){
		message_send(stream,to,from,1,"I don't know you. Register first.");
		return;	
	}
	
	if (body!=NULL){
		for(i=0;msg_commands[i].command;i++){
			if (strncmp(body,msg_commands[i].command,
					strlen(msg_commands[i].command))==0)
				ce=body+strlen(msg_commands[i].command);
			else if (strncmp(body,msg_commands[i].abr,
					strlen(msg_commands[i].abr))==0)
				ce=body+strlen(msg_commands[i].abr);
			else continue;
			if (ce[0]!='\000' && !isspace(ce[0])) continue;
			p=g_strdup(ce);
			args=g_strstrip(p);
			msg_commands[i].handler(stream,from,to,args);
			g_free(p);
			return;
		}
	}
	message_send(stream,to,from,1,"Available commands (and abbreviations):");
	for(i=0;msg_commands[i].command;i++){
		p=g_strdup_printf("  %s (%s)", msg_commands[i].command, msg_commands[i].abr);
		message_send(stream,to,from,1,p);
		g_free(p);
	}
	message_send(stream,to,from,1,"Current settings:");
	p=g_strdup_printf("  friends only: %s", user->friends_only?"on":"off");
	message_send(stream,to,from,1,p);
	g_free(p);
	p=g_strdup_printf("  invisible: %s", user->invisible?"on":"off");
	message_send(stream,to,from,1,p);
	g_free(p);
	
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
	if (!to || !jid_is_my(to)){
		g_warning("Bad 'to' in: %s",xmlnode2str(tag));
		message_send_error(stream,to,from,body,400,"Bad Request"); 
		return -1;
	}
	
	if (!jid_has_uin(to)){
		return message_to_me(stream,from,to,body,tag);
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

