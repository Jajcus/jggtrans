/* $Id: message.c,v 1.25 2003/04/06 15:42:42 mmazur Exp $ */

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
#include <ctype.h>
#include <time.h>
#include "message.h"
#include "presence.h"
#include "jabber.h"
#include "jid.h"
#include "users.h"
#include "sessions.h"
#include "requests.h"
#include "encoding.h"
#include "debug.h"

typedef void (*MsgHandler)(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
typedef struct iq_namespace_s{
	const char *command;
	const char *abr;
	const char *description;
	MsgHandler handler;
	int experimental;
}MsgCommand;

void message_get_roster(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_put_roster(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_friends_only(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_invisible(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_locale(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);

MsgCommand msg_commands[]={
#ifdef REMOTE_USERLIST
	{"get roster","gr",N_("Download user list from server"),message_get_roster,1},
	{"put roster","pr",N_("Upload user list to server"),message_put_roster,1},
#endif
	{"friends only","fo",N_("\"Only for friends\" mode"),message_friends_only,1},
	{"invisible","iv",N_("\"Invisible\" mode"),message_invisible,1},
	{"locale","loc",N_("Set user locale (language)"),message_locale,1},
	{NULL,NULL,NULL,0},
};

int message_send(struct stream_s *stream,const char *from,
		const char *to,int chat,const char *message,time_t timestamp){
xmlnode msg;
xmlnode n;
struct tm *tm;
char buf[101];

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
	if (timestamp){
		n=xmlnode_insert_tag(msg,"x");
		xmlnode_put_attrib(n,"xmlns","jabber:x:delay");
		tm=gmtime(&timestamp);
		strftime(buf,100,"%Y%m%dT%H:%M:%S",tm);
		xmlnode_put_attrib(n,"stamp",buf);
		xmlnode_insert_cdata(n,"Delayed message",-1);
	}
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
	if (body){
		n=xmlnode_insert_tag(msg,"body");
		xmlnode_insert_cdata(n,body,-1);
	}
	stream_write(stream,msg);
	xmlnode_free(msg);
	return 0;
}

#ifdef REMOTE_USERLIST
void message_get_roster(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
struct gg_http *gghttp;
User *user;
Request *r;

	user=user_get_by_jid(from);

	message_send(stream,to,from,1,_("Receiving roster..."));

	gghttp=gg_userlist_get(user->uin,user->password,1);
	r=add_request(RT_USERLIST_GET,from,to,"",msg,(void*)gghttp,stream);
	if (!r){
		message_send_error(stream,to,from,NULL,
					500,_("Internal Server Error"));
	}
}

void get_roster_error(struct request_s *r){
	message_send_error(r->stream,r->to,r->from,NULL,
				500,_("Internal Server Error"));
	gg_userlist_get_free(r->gghttp);
	r->gghttp=NULL;
}

void get_roster_done(struct request_s *r){
char **results;
char *body=NULL;
xmlnode roster;
xmlnode msg;
xmlnode n;
int i;


	message_send(r->stream,r->to,r->from,1,_("Roster received."));

	msg=xmlnode_new_tag("message");
	if (r->to!=NULL)
		xmlnode_put_attrib(msg,"from",r->to);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(msg,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(msg,"to",r->from);
	xmlnode_put_attrib(msg,"type","chat");
	n=xmlnode_insert_tag(msg,"body");
	roster=xmlnode_insert_tag(msg,"x");
	xmlnode_put_attrib(roster,"xmlns","jabber:x:roster");

	results=g_strsplit(r->gghttp->data,"\r\n",0);
	for(i=0;results[i];i++){
		char **cinfo;
		char *line,*t,*jid;
		char *name=NULL;
		int j,uin;
		xmlnode item,tag;

		cinfo=g_strsplit(results[i],";",0);
		for(j=0;cinfo[j];j++);
		if (j<7){
			g_strfreev(cinfo);
			continue;
		}

		uin=atoi(cinfo[6]);
		item=xmlnode_insert_tag(roster,"item");

		t=g_strconcat(body,"\n",NULL);
		g_free(body);
		body=t;

		line=g_strdup_printf("Uin: %u\n",uin);
		t=g_strconcat(body,line,NULL);
		g_free(body);
		body=t;

		if (cinfo[2] && cinfo[2][0]){
			line=g_strdup_printf("Nick: %s\n",cinfo[2]);
			t=g_strconcat(body,line,NULL);
			g_free(body);
			body=t;
			if (name==NULL) name=g_strdup(cinfo[2]);
		}
		if (cinfo[0] && cinfo[0][0]){
			line=g_strdup_printf("First name: %s\n",cinfo[0]);
			t=g_strconcat(body,line,NULL);
			g_free(body);
			body=t;
			if (name==NULL) name=g_strdup(cinfo[0]);
		}
		if (cinfo[1] && cinfo[1][0]){
			line=g_strdup_printf("Last name: %s\n",cinfo[1]);
			t=g_strconcat(body,line,NULL);
			g_free(body);
			body=t;
			if (name==NULL) name=g_strdup(cinfo[1]);
		}
		if (cinfo[3] && cinfo[3][0]){
			line=g_strdup_printf("Display: %s\n",cinfo[3]);
			t=g_strconcat(body,line,NULL);
			g_free(body);
			body=t;
			/* if (name==NULL) name=g_strdup(cinfo[3]); */
		}
		if (cinfo[4] && cinfo[4][0]){
			line=g_strdup_printf("Phone: %s\n",cinfo[4]);
			t=g_strconcat(body,line,NULL);
			g_free(body);
			body=t;
		}
		if (cinfo[5] && cinfo[5][0]){
			line=g_strdup_printf("Group: %s\n",cinfo[5]);
			t=g_strconcat(body,line,NULL);
			g_free(body);
			body=t;
			tag=xmlnode_insert_tag(item,"group");
			xmlnode_insert_cdata(n,to_utf8(cinfo[5]),-1);
		}
		if (cinfo[7] && cinfo[7][0]){
			line=g_strdup_printf("E-mail: %s\n",cinfo[7]);
			t=g_strconcat(body,line,NULL);
			g_free(body);
			body=t;
		}

		jid=jid_build(uin);
		xmlnode_put_attrib(item,"jid",jid);
		g_free(jid);
		if (name==NULL) name=g_strdup_printf("%u",uin);
		xmlnode_put_attrib(item,"name",name);
		g_free(name);
		g_strfreev(cinfo);
	}
	g_strfreev(results);
	xmlnode_insert_cdata(n,to_utf8(body),-1);

	gg_userlist_put_free(r->gghttp);
	r->gghttp=NULL;
	stream_write(r->stream,msg);
	xmlnode_free(msg);
}

void message_put_roster(struct stream_s *stream,const char *from, const char *to,
			const char *args, xmlnode msg){
User *user;
struct gg_http *gghttp;
char *contacts=NULL,*cinfo,*t;
Contact *c;
GList *it;
Request *r;


	user=user_get_by_jid(from);

	for(it=user->contacts;it;it=it->next){
		c=(Contact *)it->data;
		cinfo=g_strdup_printf("%s;%s;%s;%s;%s;%s;%u;%s;%s;%s;%s;\r\n",
				c->first?c->first:"",
				c->last?c->last:"",
				c->nick?c->nick:"",
				c->display?c->display:"",
				c->phone?c->phone:"",
				c->group?c->group:"",
				c->uin,
				c->email?c->email:"",
				c->x1?c->x1:"0",
				c->x2?c->x2:"",
				c->x3?c->x3:"0");
		if (contacts==NULL){
			contacts=cinfo;
		}
		else{
			t=g_strconcat(contacts,cinfo,NULL);
			g_free(contacts);
			contacts=t;
			g_free(cinfo);
		}
	}

	if (contacts==NULL){
		message_send(stream,to,from,1,_("No contacts defined."));
		return;
	}

	message_send(stream,to,from,1,_("Sending roster..."));
	gghttp=gg_userlist_put(user->uin,user->password,contacts,1);
	g_free(contacts);
	r=add_request(RT_USERLIST_PUT,from,to,"",msg,(void*)gghttp,stream);
	if (!r){
		message_send_error(stream,to,from,NULL,
					500,_("Internal Server Error"));
	}
}

void put_roster_error(struct request_s *r){

	message_send_error(r->stream,r->to,r->from,NULL,
				500,_("Internal Server Error"));
	gg_userlist_put_free(r->gghttp);
	r->gghttp=NULL;
}

void put_roster_done(struct request_s *r){

	message_send(r->stream,r->to,r->from,1,_("Roster sent."));
	gg_userlist_put_free(r->gghttp);
	r->gghttp=NULL;
}
#endif /* REMOTE USERLIST */

void message_friends_only(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
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
		message_send(stream,to,from,1,_("No change."),0);
		return;
	}
	user->friends_only=on;

	if (on)
		message_send(stream,to,from,1,_("friends only: on"),0);
	else
		message_send(stream,to,from,1,_("friends only: off"),0);

	session_send_status(session);

	user_save(user);
}

void message_invisible(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
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
		message_send(stream,to,from,1,_("No change."),0);
		return;
	}
	user->invisible=on;

	if (on)
		message_send(stream,to,from,1,_("invisible: on"),0);
	else
		message_send(stream,to,from,1,_("invisible: off"),0);

	session_send_status(session);

	user_save(user);
}

void message_locale(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
char *m;

	session=session_get_by_jid(from,stream);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);

	if (args && args[0]){
		if (user->locale) g_free(user->locale);
		user->locale=g_strdup(args);
	}

	m=g_strdup_printf(_("Locale set to: %s"),user->locale?user->locale:_("_default_"));
	message_send(stream,to,from,1,m,0);
	g_free(m);
	user_save(user);
}


int message_to_me(struct stream_s *stream,const char *from,
		const char *to,const char *body,xmlnode tag){
int i;
const char *ce;
char *args,*p;
User *user;
char *msg;

	user=user_get_by_jid(from);
	if (user==NULL){
		message_send(stream,to,from,1,_("I don't know you. Register first."),0);
		return -1;
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
			msg_commands[i].handler(stream,from,to,args,tag);
			g_free(p);
			return 0;
		}
	}
	msg=_("\nAvailable commands and abbreviations:");
	for(i=0;msg_commands[i].command;i++){
		msg=g_strdup_printf("%s\n  %-12s %-4s  - %s%s",msg,
				msg_commands[i].command,
				msg_commands[i].abr,
				_(msg_commands[i].description),
				msg_commands[i].experimental?_(" EXPERIMENTAL!"):"");
	}
	msg=g_strdup_printf(_("%s\nCurrent settings:"),msg);
	msg=g_strdup_printf(_("%s\n  friends only: %s"),msg,user->friends_only?_("on"):_("off"));
	msg=g_strdup_printf(_("%s\n  invisible: %s"),msg,user->invisible?_("on"):_("off"));
	msg=g_strdup_printf(_("%s\n  locale: %s"),msg,user->locale?user->locale:_("_default_"));
	message_send(stream,to,from,1,msg,0);
	g_free(msg);

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
User *u;

	body_n=xmlnode_get_tag(tag,"body");
	if (body_n!=NULL) body=xmlnode_get_data(body_n);
	else body=NULL;

	subject_n=xmlnode_get_tag(tag,"subject");
	if (subject_n!=NULL) subject=xmlnode_get_data(subject_n);
	else subject=NULL;

	from=xmlnode_get_attrib(tag,"from");
	to=xmlnode_get_attrib(tag,"to");

	if (from) u=user_get_by_jid(from);
	else u=NULL;
	user_load_locale(u);

	type=xmlnode_get_attrib(tag,"type");
	if (!type || !strcmp(type,"normal")) chat=0;
	else if (!strcmp(type,"chat")) chat=1;
	else if (!strcmp(type,"error")){
		g_warning(N_("Error message received: %s"),xmlnode2str(tag));
		return 0;
	}
	else{
		g_warning(N_("Unsupported message type"));
		message_send_error(stream,to,from,body,500,_("Internal Server Error"));
		return -1;
	}

	if (!to || !jid_is_my(to)){
		g_warning(N_("Bad 'to' in: %s"),xmlnode2str(tag));
		message_send_error(stream,to,from,body,400,_("Bad Request"));
		return -1;
	}

	if (!jid_has_uin(to)){
		return message_to_me(stream,from,to,body,tag);
	}

	if (!from){
		g_warning(N_("Anonymous message? %s"),xmlnode2str(tag));
		message_send_error(stream,to,from,body,400,_("Bad Request"));
		return -1;
	}

	s=session_get_by_jid(from,NULL);
	if (!s || !s->connected){
		g_warning(N_("%s not logged in. While processing %s"),from,xmlnode2str(tag));
		message_send_error(stream,to,from,body,407,_("Not logged in"));
		return -1;
	}

	if (subject)
		body=g_strdup_printf("Subject: %s\n%s",subject,body);
	session_send_message(s,jid_get_uin(to),chat,from_utf8(body));
	if (subject)
		g_free(body);

	return 0;
}

