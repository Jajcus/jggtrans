/* $Id: register.c,v 1.18 2003/01/22 07:53:01 jajcus Exp $ */

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
#include "jabber.h"
#include "register.h"
#include "iq.h"
#include "users.h"
#include "sessions.h"
#include "presence.h"
#include "users.h"
#include "debug.h"
#include "requests.h"
#include "encoding.h"

char *register_instructions;

void jabber_iq_get_register(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
xmlnode node;
xmlnode iq;
xmlnode query;
xmlnode instr;

	node=xmlnode_get_firstchild(q);
	if (node){
		g_warning("Get for jabber:iq:register not empty!: %s",xmlnode2str(q));
		jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
		return;
	}
	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","result");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",from);
	xmlnode_put_attrib(iq,"from",my_name);
	query=xmlnode_insert_tag(iq,"query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:register");

	/* needed to register existing user */
	xmlnode_insert_tag(query,"username");
	xmlnode_insert_tag(query,"password");

	/* needed to register new user */
	xmlnode_insert_tag(query,"email");

	/* public directory info */
	xmlnode_insert_tag(query,"first");
	xmlnode_insert_tag(query,"last");
	xmlnode_insert_tag(query,"nick");
	xmlnode_insert_tag(query,"city");
	xmlnode_insert_tag(query,"born");
	xmlnode_insert_tag(query,"gender");

	instr=xmlnode_insert_tag(query,"instructions");
	xmlnode_insert_cdata(instr,register_instructions,-1);
	stream_write(s,iq);
	xmlnode_free(iq);
}

void unregister(Stream *s,const char *from,const char *to,const char *id,int presence_used){
Session *ses;
User *u;
char *jid;

	debug("Unregistering '%s'",from);
	ses=session_get_by_jid(from,NULL);
	if (ses)
		if (session_remove(ses)){
			g_warning("'%s' unregistration failed",from);
			jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
			return;
		}

	u=user_get_by_jid(from);
	if (u){
		jid=g_strdup(u->jid);
		if (user_delete(u)){
			g_warning("'%s' unregistration failed",from);
			jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
			return;
		}
	}

	if (!u){
		g_warning("Tried to unregister '%s' who was never registered",from);
		jabber_iq_send_error(s,from,to,id,404,"Not Found");
		return;
	}

	if (!presence_used){
		jabber_iq_send_result(s,from,to,id,NULL);
		presence_send_unsubscribe(s,NULL,jid);
	}
	presence_send_unsubscribed(s,NULL,jid);
	g_message("User '%s' unregistered",from);
	g_free(jid);
}

void jabber_iq_set_register(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
xmlnode node;
char *str,*password;
struct gg_change_info_request change;
uin_t uin;
User *user;
Session *session=NULL;
struct gg_http *gghttp;
Request *r;

	node=xmlnode_get_firstchild(q);
	if (!node){
		debug("Set query for jabber:iq:register empty: %s",xmlnode2str(q));
		unregister(s,from,to,id,0);
		return;
	}

	node=xmlnode_get_tag(q,"remove");
	if (node){
		debug("<remove/> in jabber:iq:register set: %s",xmlnode2str(q));
		unregister(s,from,to,id,0);
		return;
	}

	node=xmlnode_get_tag(q,"username");
	if (node) str=xmlnode_get_data(node);
	if (!node || !str) uin=0;
	else uin=atoi(str);

	node=xmlnode_get_tag(q,"password");
	if (node) password=xmlnode_get_data(node);
	else password=NULL;

	user=user_get_by_jid(from);

	if (!user && (!uin || !password)){
		g_warning("User '%s' doesn't exist and not enough info to add him",from);
		jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
		return;
	}

	if (!user){
		user=user_create(from,uin,password);
		if (!user){
			g_warning("Couldn't create user %s",from);
			jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
			return;
		}

		session=session_create(user,from,id,q,s);
		if (!session){
			user_remove(user);
			g_warning("Couldn't create session for %s",from);
			jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
			return;
		}
	}

	memset(&change,0,sizeof(change));

	node=xmlnode_get_tag(q,"first");
	if (node) change.first_name=xmlnode_get_data(node);
	else change.first_name=NULL;

	node=xmlnode_get_tag(q,"last");
	if (node) change.last_name=xmlnode_get_data(node);
	else change.last_name=NULL;

	node=xmlnode_get_tag(q,"nick");
	if (node) change.nickname=xmlnode_get_data(node);
	else change.nickname=NULL;

	node=xmlnode_get_tag(q,"email");
	if (node) change.email=xmlnode_get_data(node);
	else change.email=NULL;

	node=xmlnode_get_tag(q,"city");
	if (node) change.city=xmlnode_get_data(node);
	else change.city=NULL;

	node=xmlnode_get_tag(q,"gender");
	if (node) str=xmlnode_get_data(node);
	if (!node || !str || !str[0]) change.gender=GG_GENDER_NONE;
	else if (str[0]=='k' || str[0]=='f' || str[0]=='K' || str[0]=='F') change.gender=GG_GENDER_FEMALE;
	else change.gender=GG_GENDER_MALE;

	node=xmlnode_get_tag(q,"born");
	if (node) str=xmlnode_get_data(node);
	if (!node || !str || !str[0]) change.born=0;
	else change.born=atoi(str);

	if (!change.first_name && !change.last_name && !change.nickname
		&& !change.city && !change.email && !change.born && !change.gender){
			if (!uin && !password){
				debug("Set query for jabber:iq:register empty: %s",xmlnode2str(q));
				unregister(s,from,to,id,0);
				return;
			}
			if (!uin || !password){
				g_warning("Nothing to change");
				session_remove(session);
				jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
			}
			return;
	}

	if (!change.email || !change.nickname){
		debug("email or nickname not given for registration");
		session_remove(session);
		jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
		return;
	}

	if (!user && (!password ||!uin)){
		g_warning("Not registered, and no password gived for public directory change.");
		session_remove(session);
		jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
		return;
	}
	if (!password) password=user->password;
	if (!uin) uin=user->uin;

	debug("gg_change_pubdir()");

	if (change.first_name) change.first_name=g_strdup(from_utf8(change.first_name));
	if (change.last_name) change.last_name=g_strdup(from_utf8(change.last_name));
	if (change.nickname) change.nickname=g_strdup(from_utf8(change.nickname));
	if (change.email) change.email=g_strdup(from_utf8(change.email));
	if (change.city) change.city=g_strdup(from_utf8(change.city));

	gghttp=gg_change_info(uin,password,&change,1);

	if (change.first_name) g_free(change.first_name);
	if (change.last_name) g_free(change.last_name);
	if (change.nickname) g_free(change.nickname);
	if (change.email) g_free(change.email);
	if (change.city) g_free(change.city);

	if (!gghttp){
		jabber_iq_send_error(s,from,to,id,502,"Remote Server Error");
		session_remove(session);
		return;
	}

	r=add_request(RT_CHANGE,from,to,id,q,gghttp,s);
	if (!r){
		session_remove(session);
		jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
	}
}

int register_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->to,r->id,502,"Remote Server Error");
	return 0;
}

int register_done(struct request_s *r){

	jabber_iq_send_result(r->stream,r->from,r->to,r->id,NULL);
	return 0;
}


