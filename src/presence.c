/* $Id: presence.c,v 1.26 2003/04/06 15:42:42 mmazur Exp $ */

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
#include "debug.h"
#include "register.h"
#include "status.h"
#include "encoding.h"
#include <time.h>

int presence_send_error(struct stream_s *stream,const char *from,const char *to,
				int code,const char *string){
xmlnode pres;
xmlnode error;
char *jid;
char *str;

	pres=xmlnode_new_tag("presence");
	jid=jid_my_registered();
	if (from!=NULL)
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	g_free(jid);
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","error");
	error=xmlnode_insert_tag(pres,"error");
	if (code>0){
		str=g_strdup_printf("%03u",(unsigned)code);
		xmlnode_put_attrib(error,"code",str);
		g_free(str);
	}
	xmlnode_insert_cdata(error,string,-1);

	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}


int presence_send_probe(struct stream_s *stream,const char *to){
xmlnode pres;
char *jid;

	pres=xmlnode_new_tag("presence");
	jid=jid_my_registered();
	xmlnode_put_attrib(pres,"from",jid);
	g_free(jid);
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","probe");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_subscribe(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL)
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","subscribe");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_subscribed(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL)
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","subscribed");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_unsubscribed(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL)
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","unsubscribed");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_unsubscribe(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL)
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","unsubscribe");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send(struct stream_s *stream,const char *from,
		const char *to,int available,const char *show,
		const char *status,GTime timestamp){
xmlnode pres;
xmlnode n;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL)
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	if (!available) xmlnode_put_attrib(pres,"type","unavailable");
	if (show){
		n=xmlnode_insert_tag(pres,"show");
		xmlnode_insert_cdata(n,show,-1);
	}
	if (status){
		n=xmlnode_insert_tag(pres,"status");
		xmlnode_insert_cdata(n,to_utf8(status),-1);
	}
	if (timestamp){
		struct tm *t;
		char str[21];

		t=localtime((time_t *)&timestamp);
		strftime(str,20,"%Y%m%dT%T",t);
		n=xmlnode_insert_tag(pres,"x");
		xmlnode_put_attrib(n,"xmlns","jabber:x:delay");
		xmlnode_put_attrib(n,"stamp",str);
	}
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence(struct stream_s *stream,const char *from,const char *to,
		int available,const char *show,const char *status,int priority){
Session *s;
int r;
char *jid;
const char *resource;
Resource *res;

	s=session_get_by_jid(from,available?stream:NULL);
	if (!s){
		debug(N_("presence: No such session: %s"),from);
		presence_send_error(stream,NULL,from,407,_("Not logged in"));
		return -1;
	}
	jid=g_strdup(s->user->jid); /* session may be removed in the next step */
	resource=jid_get_resource(from);
	r=session_set_status(s,resource,available,show,from_utf8(status),priority);
	if (!r && s->connected){
		res=session_get_cur_resource(s);
		if (res) presence_send(stream,NULL,jid,res->available,res->show,res->status,0);
	}
	g_free(jid);
	return 0;
}

int presence_subscribe(struct stream_s *stream,const char *from,const char *to){
User *u;
Session *s;
int r;

	u=user_get_by_jid(from);
	if (jid_is_me(to)){
		debug(N_("Presence subscribe request sent to me"));
		if (!u) presence_send_unsubscribed(stream,to,from);
		else presence_send_subscribed(stream,to,from);
		return 0;
	}
	if (!u){
		g_warning(N_("Presence subscription from unknown user (%s)"),from);
		return -1;
	}
	if (!jid_has_uin(to) || !jid_is_my(to)){
		g_warning(N_("Bad 'to': %s"),to);
		return -1;
	}
	s=session_get_by_jid(from,stream);
	if (!s){
		g_warning(N_("Couldn't find or open session for '%s'"),from);
		return -1;
	}
	debug(N_("Subscribing %s to %s..."),from,to);
	r=session_subscribe(s,jid_get_uin(to));
	if (!r){
		debug(N_("Subscribed."));
		presence_send_subscribed(stream,to,from);
	}
	else presence_send_subscribed(stream,to,from);

	return 0;
}

int presence_subscribed(struct stream_s *stream,const char *from,const char *to){

	return 0;
}

int presence_unsubscribed(struct stream_s *stream,const char *from,const char *to){

	if (!jid_is_me(to)) return 0;

	debug(N_("Presence unsubscribed sent to me."));

	unregister(stream,from,NULL,NULL,1);

	return 0;
}

int presence_probe(struct stream_s *stream,const char *from,const char *to){
Session *s;
User *u;
uin_t uin;
int status;
int available;
char *show,*stat;
GList *it;
GTime timestamp;

	s=session_get_by_jid(from,NULL);
	if (jid_is_me(to)){
		if (s){
			if (!s->connected){
				presence_send(stream,to,from,0,NULL,"Disconnected",0);
			}
			else{
				Resource *r=session_get_cur_resource(s);
				if (r) presence_send(stream,NULL,s->user->jid,r->available,
							r->show,r->status,0);
			}
		}
		else presence_send(stream,to,from,0,NULL,"Not logged in",0);
		return 0;
	}

	if (!jid_is_my(to)){
		presence_send_error(stream,to,from,404,_("Not Found"));
		return -1;
	}

	if (s) u=s->user;
	else u=user_get_by_jid(from);

	if (!u){
		presence_send_error(stream,to,from,407,_("Not logged in"));
		return -1;
	}

	uin=jid_get_uin(to);
	status=0;
	stat="";
	timestamp=0;
	for(it=u->contacts;it;it=it->next){
		Contact *c=(Contact *)it->data;

		if (c && c->uin==uin){
			status=c->status;
			timestamp=c->last_update;
			stat=c->status_desc;
		}
	}
	if (!status){
		presence_send_error(stream,to,from,404,_("Not Found"));
		return -1;
	}

	available=status_gg_to_jabber(status,&show,&stat);

	presence_send(stream,to,u->jid,available,show,stat,timestamp);
	if (s) session_send_notify(s);
	return 0;
}

int presence_unsubscribe(struct stream_s *stream,const char *from,const char *to){
User *u;
Session *s;
int r;

	if (jid_is_me(to)){
		debug(N_("Presence unsubscribe request sent to me"));
		presence_send_unsubscribed(stream,to,from);
		return 0;
	}
	u=user_get_by_jid(from);
	if (!u){
		g_warning(N_("Presence subscription from unknown user (%s)"),from);
		return -1;
	}
	if (!jid_has_uin(to) || !jid_is_my(to)){
		g_warning(N_("Bad 'to': %s"),to);
		return -1;
	}
	s=session_get_by_jid(from,stream);
	if (!s){
		g_warning(N_("Couldn't find or open session for '%s'"),from);
		return -1;
	}
	debug(N_("Unsubscribing %s to %s..."),from,to);
	r=session_unsubscribe(s,jid_get_uin(to));
	if (!r){
		debug(N_("Unsubscribed."));
		presence_send_unsubscribed(stream,to,from);
	}

	return 0;
}

int jabber_presence(struct stream_s *stream,xmlnode tag){
char *type;
char *from;
char *to;
xmlnode prio_n;
xmlnode show_n;
xmlnode status_n;
char *show,*status;
int priority;
User *u;

	type=xmlnode_get_attrib(tag,"type");
	from=xmlnode_get_attrib(tag,"from");
	to=xmlnode_get_attrib(tag,"to");

	if (from) u=user_get_by_jid(from);
	else u=NULL;
	user_load_locale(u);

	show_n=xmlnode_get_tag(tag,"show");
	if (show_n) show=xmlnode_get_data(show_n);
	else show=NULL;

	status_n=xmlnode_get_tag(tag,"status");
	if (status_n) status=xmlnode_get_data(status_n);
	else status=NULL;

	prio_n=xmlnode_get_tag(tag,"priority");
	if (prio_n) priority=atoi(xmlnode_get_data(prio_n));
	else priority=-1;

	if (!type) type="available";

	if (!from || !to){
		presence_send_error(stream,to,from,406,_("Not Acceptable"));
		g_warning(N_("Bad <presence/>: %s"),xmlnode2str(tag));
		return -1;
	}

	if (!jid_is_my(to)){
		presence_send_error(stream,to,from,406,_("Not Acceptable"));
		g_warning(N_("Wrong 'to' in %s"),xmlnode2str(tag));
		return -1;
	}

	if (!strcmp(type,"available"))
		return presence(stream,from,to,1,show,status,priority);
	else if (!strcmp(type,"unavailable"))
		return presence(stream,from,to,0,show,status,priority);
	else if (!strcmp(type,"subscribe"))
		return presence_subscribe(stream,from,to);
	else if (!strcmp(type,"unsubscribe"))
		return presence_unsubscribe(stream,from,to);
	else if (!strcmp(type,"subscribed"))
		return presence_subscribed(stream,from,to);
	else if (!strcmp(type,"unsubscribed"))
		return presence_unsubscribed(stream,from,to);
	else if (!strcmp(type,"probe"))
		return presence_probe(stream,from,to);
	else if (!strcmp(type,"error")){
		g_warning(N_("Error presence received: %s"),xmlnode2str(tag));
		return 0;
	}

	g_warning(N_("Unsupported type in %s"),xmlnode2str(tag));
	presence_send_error(stream,to,from,501,_("Not Implemented"));
	return -1;
}

