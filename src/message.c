/* $Id: message.c,v 1.46 2004/10/27 20:50:14 jajcus Exp $ */

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
#include "acl.h"
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
void message_friends_only(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_invisible(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_locale(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_ignore_unknown(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_ignore(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_unignore(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);
void message_status(struct stream_s *s,const char *from, const char *to,
				const char *args, xmlnode msg);

MsgCommand msg_commands[]={
	{"get roster","gr",N_("Download user list from server"),message_get_roster,0},
	{"friends only","fo",N_("\"Only for friends\" mode"),message_friends_only,0},
	{"invisible","iv",N_("\"Invisible\" mode"),message_invisible,0},
	{"locale","loc",N_("Set user locale (language)"),message_locale,0},
	{"ignore_unknown","iu",N_("Ignore messages from unknown users"),message_ignore_unknown,0},
	{"ignore","ig",N_("Add a user to, or view the ignore list"),message_ignore,0},
	{"unignore","ui",N_("Remove a user from, or view the ignore list"),message_unignore,0},
	{"status","st",N_("Status message to show to GG users. Use 'off' to use Jabber status."),message_status,0},
	{NULL,NULL,NULL,0},
};

int message_send_subject(struct stream_s *stream,const char *from,
		const char *to,const char *subject,const char *message,time_t timestamp){
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
	n=xmlnode_insert_tag(msg,"subject");
	xmlnode_insert_cdata(n,subject,-1);
	n=xmlnode_insert_tag(msg,"body");
	xmlnode_insert_cdata(n,message,-1);
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
	xmlnode_insert_cdata(n,message,-1);
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

void message_get_roster(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *s;

	s=session_get_by_jid(from,stream,0);
	if(!s || !s->ggs){
		message_send(stream,to,from,1,_("Log in first..."),0);
		return;
	}

	message_send(stream,to,from,1,_("Receiving roster..."),0);

	gg_userlist_request(s->ggs, GG_USERLIST_GET, NULL);
}

void get_roster_done(Session *s,struct gg_event *e){
char **results;
char *body=NULL;
char *jid;
xmlnode roster;
xmlnode msg;
xmlnode n;
int i;

	if(!e->event.userlist.reply){
		message_send(s->s,NULL,s->user->jid,1,_("Roster empty."),0);
		return;
	}

	message_send(s->s,NULL,s->user->jid,0,_("Roster received."),0);

	msg=xmlnode_new_tag("message");
	jid=jid_my_registered();
	xmlnode_put_attrib(msg,"from",jid);
	g_free(jid);

	xmlnode_put_attrib(msg,"to",s->user->jid);
	n=xmlnode_insert_tag(msg,"body");
	roster=xmlnode_insert_tag(msg,"x");
	xmlnode_put_attrib(roster,"xmlns","jabber:x:roster");

	body=g_strdup("");
	results=g_strsplit(e->event.userlist.reply,"\r\n",0);
	for(i=0;results[i];i++){
		char **cinfo;
		char *t,*jid;
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

		t=g_strdup_printf("%sUin: %u\n",body,uin);
		g_free(body);
		body=t;

		if (cinfo[2] && cinfo[2][0]){
			t=g_strdup_printf("%sNick: %s\n",body,cinfo[2]);
			g_free(body);
			body=t;
			if (name==NULL) name=g_strdup(cinfo[2]);
		}
		if (cinfo[0] && cinfo[0][0]){
			t=g_strdup_printf("%sFirst name: %s\n",body,cinfo[0]);
			g_free(body);
			body=t;
			if (name==NULL) name=g_strdup(cinfo[0]);
		}
		if (cinfo[1] && cinfo[1][0]){
			t=g_strdup_printf("%sLast name: %s\n",body,cinfo[1]);
			g_free(body);
			body=t;
			if (name==NULL) name=g_strdup(cinfo[1]);
		}
		if (cinfo[3] && cinfo[3][0]){
			t=g_strdup_printf("%sDisplay: %s\n",body,cinfo[3]);
			g_free(body);
			body=t;
			if (name) g_free(name);
			name=g_strdup(cinfo[3]);
		}
		if (cinfo[4] && cinfo[4][0]){
			t=g_strdup_printf("%sPhone: %s\n",body,cinfo[4]);
			g_free(body);
			body=t;
		}
		if (cinfo[5] && cinfo[5][0]){
			t=g_strdup_printf("%sGroup: %s\n",body,cinfo[5]);
			g_free(body);
			body=t;
			tag=xmlnode_insert_tag(item,"group");
			xmlnode_insert_cdata(n,to_utf8(cinfo[5]),-1);
		}
		if (cinfo[7] && cinfo[7][0]){
			t=g_strdup_printf("%sE-mail: %s\n",body,cinfo[7]);
			g_free(body);
			body=t;
		}

		jid=jid_build(uin);
		xmlnode_put_attrib(item,"jid",jid);
		g_free(jid);
		if (name==NULL) name=g_strdup_printf("%u",uin);
		xmlnode_put_attrib(item,"name",to_utf8(name));
		g_free(name);
		g_strfreev(cinfo);
	}
	g_strfreev(results);
	xmlnode_insert_cdata(n,to_utf8(body),-1);

	stream_write(s->s,msg);
	xmlnode_free(msg);
}

void message_friends_only(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
gboolean on;

	session=session_get_by_jid(from,stream,0);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);
	if (args && g_strcasecmp(args,"on")==0) on=TRUE;
	else if (args && g_strcasecmp(args,"off")==0) on=FALSE;
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

	if (session!=NULL) session_send_status(session);

	user_save(user);
}

void message_invisible(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
Resource *r;
gboolean on;

	session=session_get_by_jid(from,stream,0);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);
	if (args && g_strcasecmp(args,"on")==0) on=TRUE;
	else if (args && g_strcasecmp(args,"off")==0) on=FALSE;
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

	if (session!=NULL) session_send_status(session);
	r=session_get_cur_resource(session);
	if ( r )
		presence_send(stream,NULL,user->jid,user->invisible?-1:r->available,r->show,session->gg_status_descr,0);

	user_save(user);
}

void message_status(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
char *m;

	session=session_get_by_jid(from,stream,0);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);

	g_free(user->status);
	if (args) {
		if (!g_strcasecmp(args,"off")) user->status=NULL;
		else user->status=g_strndup(from_utf8(args),GG_STATUS_DESCR_MAXSIZE);
	}
	else user->status=NULL;

	m=g_strdup_printf(_("status: %s%s%s"),
			(user->status?"`":""),
			(user->status?to_utf8(user->status):_("not set")),
			(user->status?"'":""));
	message_send(stream,to,from,1,m,0);
	g_free(m);

	if (session!=NULL) session_send_status(session);
	user_save(user);
}

void message_locale(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
char *m;

	session=session_get_by_jid(from,stream,0);
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

void message_ignore_unknown(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
gboolean on;

	session=session_get_by_jid(from,stream,0);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);
	if (args && g_strcasecmp(args,"on")==0) on=TRUE;
	else if (args && g_strcasecmp(args,"off")==0) on=FALSE;
	else on=!user->ignore_unknown;

	if (user->ignore_unknown==on){
		message_send(stream,to,from,1,_("No change."),0);
		return;
	}
	user->ignore_unknown=on;

	if (on)
		message_send(stream,to,from,1,_("ignore unknown: on"),0);
	else
		message_send(stream,to,from,1,_("ignore unknown: off"),0);

	if (session!=NULL) session_send_status(session);

	user_save(user);
}


void message_ignore(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
uin_t uin;
Contact *c;
gchar *m;

	session=session_get_by_jid(from,stream,0);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);

	if (args && *args) uin=atoi(args);
	else uin=0;

	if (uin<=0) {
		GList *it;
		m=g_strdup(_("\nMessages from the following GG numbers will be ignored:"));
		for(it=user->contacts;it;it=it->next){
			c=(Contact *)it->data;
			if (!c->ignored) continue;
			m=g_strdup_printf(_("%s\n  %li"),m,(long)c->uin);
		}
		message_send(stream,to,from,1,m,0);
		g_free(m);
		return;
	}

	c=user_get_contact(user,uin,TRUE);
	c->ignored=TRUE;
	if (session) session_update_contact(session,c);
		
	m=g_strdup_printf(_("\nMessages from GG number %li will be ignored."),(long)uin);
	message_send(stream,to,from,1,m,0);
	g_free(m);

	user_save(user);
}

void message_unignore(struct stream_s *stream,const char *from, const char *to,
				const char *args, xmlnode msg){
Session *session;
User *user;
uin_t uin;
Contact *c;
gchar *m;

	session=session_get_by_jid(from,stream,0);
	if (session!=NULL)
		user=session->user;
	else
		user=user_get_by_jid(from);

	if (args && *args) uin=atoi(args);
	else uin=0;

	if (uin<=0) {
		message_ignore(stream,from,to,NULL,msg);
		return;
	}

	c=user_get_contact(user,uin,FALSE);
	if (c) {
		c->ignored=FALSE;
		user_check_contact(user,c);
		if (session) session_update_contact(session,c);
	}
		
	m=g_strdup_printf(_("\nMessages from GG number %li will NOT be ignored."),(long)uin);
	message_send(stream,to,from,1,m,0);
	g_free(m);
	
	user_save(user);
}

int message_to_me(struct stream_s *stream,const char *from,
		const char *to,const char *body,xmlnode tag){
int i,ignored;
const char *ce;
char *args,*p;
User *user;
Session *sess;
char *msg, *t;
GList *it;

	sess=session_get_by_jid(from,stream,1);
	if (sess)
		user=sess->user;
	else
		user=NULL;
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
	msg=g_strdup(_("\nAvailable commands and abbreviations:"));
	for(i=0;msg_commands[i].command;i++){
		t=g_strdup_printf("%s\n  %-14s %-4s - %s%s",msg,
				msg_commands[i].command,
				msg_commands[i].abr,
				_(msg_commands[i].description),
				msg_commands[i].experimental?_(" EXPERIMENTAL!"):"");
		g_free(msg); msg=t;
	}
	t=g_strdup_printf(_("%s\n\nCurrent settings:"),msg);
	g_free(msg); msg=t;
	t=g_strdup_printf(_("%s\n  status: %s%s%s"),msg,
			(user->status?"`":""),
			(user->status?to_utf8(user->status):_("not set")),
			(user->status?"'":""));
	g_free(msg); msg=t;
	t=g_strdup_printf(_("%s\n  friends only: %s"),msg,user->friends_only?_("on"):_("off"));
	g_free(msg); msg=t;
	t=g_strdup_printf(_("%s\n  invisible: %s"),msg,user->invisible?_("on"):_("off"));
	g_free(msg); msg=t;
	t=g_strdup_printf(_("%s\n  ignore unknown: %s"),msg,user->ignore_unknown?_("on"):_("off"));
	g_free(msg); msg=t;
	t=g_strdup_printf(_("%s\n  locale: %s"),msg,user->locale?user->locale:_("_default_"));
	g_free(msg); msg=t;
	ignored=0;
	for(it=user->contacts;it;it=it->next){
		Contact * c=(Contact *)it->data;
		if (c->ignored) ignored++;
	}
	t=g_strdup_printf(_("%s\n  number of ignored users: %i"),msg,ignored);
	g_free(msg); msg=t;
	t=g_strdup_printf(_("%s\n\nRegistered as: %u"),msg,user->uin);
	g_free(msg); msg=t;
	if (sess->ggs){
		char *t1=session_get_info_string(sess);
		t=g_strdup_printf("%s\n  %s",msg,t1);
		g_free(t1);
		g_free(msg); msg=t;
	}
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
	type=xmlnode_get_attrib(tag,"type");

	if (!acl_is_allowed(from,tag)){
		if (type && !strcmp(type,"error")){
			debug("Ignoring forbidden message error");
			return -1;
		}
		if (!from) return -1;
		message_send_error(stream,to,from,NULL,405,_("Not allowed"));
		return -1;
	}

	if (from) u=user_get_by_jid(from);
	else u=NULL;
	user_load_locale(u);

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

	s=session_get_by_jid(from,NULL,0);
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

