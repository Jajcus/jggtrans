/* $Id: register.c,v 1.24 2003/04/06 10:36:15 jajcus Exp $ */

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

static struct {
	char *lang_name;
	char *locale;
} locale_mapping[]={
	{ "Polski", "pl_PL"},
	{ "English", "C" },
	{ NULL, NULL}
	};
	
xmlnode register_change_form(User *u){
xmlnode form,tag,field,option,value;
int i;

	form=xmlnode_new_tag("x");
	xmlnode_put_attrib(form,"xmlns","jabber:x:data");
	xmlnode_put_attrib(form,"type","form");
	tag=xmlnode_insert_tag(form,"title");
	xmlnode_insert_cdata(tag,_("Registration change form"),-1);
	tag=xmlnode_insert_tag(form,"instruction");
	xmlnode_insert_cdata(tag,_("You may use this form to change account"
				" information, change personal information in the"
				" public directory or unregister from the"
				" transport."),-1);
	
	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","list-single");
	xmlnode_put_attrib(field,"label",_("Action"));
	xmlnode_put_attrib(field,"var","action");
	xmlnode_insert_tag(field,"required");
	value=xmlnode_insert_tag(field,"value");
	xmlnode_insert_cdata(value,"options",-1);
	option=xmlnode_insert_tag(field,"option");
	xmlnode_put_attrib(option,"label",_("Change account options"));
	value=xmlnode_insert_tag(option,"value");
	xmlnode_insert_cdata(value,"options",-1);
	option=xmlnode_insert_tag(field,"option");
	xmlnode_put_attrib(option,"label",_("Change account"));
	value=xmlnode_insert_tag(option,"value");
	xmlnode_insert_cdata(value,"account",-1);
	option=xmlnode_insert_tag(field,"option");
	xmlnode_put_attrib(option,"label",_("Change public directory information"));
	value=xmlnode_insert_tag(option,"value");
	xmlnode_insert_cdata(value,"pubdir",-1);
	option=xmlnode_insert_tag(field,"option");
	xmlnode_put_attrib(option,"label",_("Unregister"));
	value=xmlnode_insert_tag(option,"value");
	xmlnode_insert_cdata(value,"unregister",-1);
	
	
	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","fixed");
	value=xmlnode_insert_tag(field,"value");
	xmlnode_insert_cdata(value,_("Fill out this part only when changing account options"),-1);

	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","list-single");
	xmlnode_put_attrib(field,"label",_("Language"));
	xmlnode_put_attrib(field,"var","locale");
	value=xmlnode_insert_tag(field,"value");
	if (u->locale && u->locale[0])
		xmlnode_insert_cdata(value,u->locale,-1);
	else 
		xmlnode_insert_cdata(value,"",-1);
	for(i=0;locale_mapping[i].locale!=NULL;i++){
		option=xmlnode_insert_tag(field,"option");
		xmlnode_put_attrib(option,"label",locale_mapping[i].lang_name);
		value=xmlnode_insert_tag(option,"value");
		xmlnode_insert_cdata(value,locale_mapping[i].locale,-1);
	}
	
	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","boolean");
	xmlnode_put_attrib(field,"label",_("Friends only"));
	xmlnode_put_attrib(field,"var","friends_only");
	value=xmlnode_insert_tag(field,"value");
	if (u->friends_only)
		xmlnode_insert_cdata(value,"1",-1);
	else
		xmlnode_insert_cdata(value,"0",-1);

	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","boolean");
	xmlnode_put_attrib(field,"label",_("Invisible"));
	xmlnode_put_attrib(field,"var","invisible");
	value=xmlnode_insert_tag(field,"value");
	if (u->invisible)
		xmlnode_insert_cdata(value,"1",-1);
	else
		xmlnode_insert_cdata(value,"0",-1);

	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","fixed");
	value=xmlnode_insert_tag(field,"value");
	xmlnode_insert_cdata(value,_("Fill out this part only when changing account"),-1);

	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","fixed");
	value=xmlnode_insert_tag(field,"value");
	xmlnode_insert_cdata(value,_("Fill out this part only when changing public directory info"),-1);

	return form;
}

int register_process_options_form(Stream *s,const char *from,const char *to,
					const char *id,User *u,xmlnode form){
xmlnode field,value;
char *locale=NULL,*invisible=NULL,*friends_only=NULL;

	field=xmlnode_get_tag(form,"field?var=locale");
	if (field!=NULL) {
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) locale=xmlnode_get_data(value);
	}
	field=xmlnode_get_tag(form,"field?var=invisible");
	if (field!=NULL) {
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) invisible=xmlnode_get_data(value);
	}
	field=xmlnode_get_tag(form,"field?var=friends_only");
	if (field!=NULL) {
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) friends_only=xmlnode_get_data(value);
	}

	if (u->locale!=NULL) g_free(u->locale);
	u->locale=g_strdup(locale);
	if (invisible && (!strcmp(invisible,"1")||!strcmp(invisible,"yes"))) 
		u->invisible=1;
	else
		u->invisible=0;
	if (friends_only && (!strcmp(friends_only,"1")||!strcmp(friends_only,"yes"))) 
		u->friends_only=1;
	else
		u->friends_only=0;
	user_save(u);
	
	jabber_iq_send_result(s,from,to,id,NULL);
	return 0;
}

int register_process_change_form(Stream *s,const char *from,const char *to,
					const char *id,User *u,xmlnode form){
xmlnode field,value;
char *action;

	field=xmlnode_get_tag(form,"field?var=action");
	if (field==NULL) {
		jabber_iq_send_error(s,from,to,id,406,_("No action field present"));
		return -1;
	}
	value=xmlnode_get_tag(field,"value");
	if (field==NULL) {
		jabber_iq_send_error(s,from,to,id,406,_("No action value defined"));
		return -1;
	}

	action=xmlnode_get_data(value);
	if (action==NULL) {
		jabber_iq_send_error(s,from,to,id,406,_("No action value defined"));
		return -1;
	}
	else if (!strcmp(action,"options")){
		register_process_options_form(s,from,to,id,u,form);
	}
	else if (!strcmp(action,"account")){
		jabber_iq_send_error(s,from,to,id,501,_("Not implemented (yet)"));
		return -1;
	}
	else if (!strcmp(action,"pubdir")){
		jabber_iq_send_error(s,from,to,id,501,_("Not implemented (yet)"));
		return -1;
	}
	else if (!strcmp(action,"unregister")){
		unregister(s,from,to,id,0);
	}
	else{
		jabber_iq_send_error(s,from,to,id,406,_("Bad action given"));
		return -1;
	}
	return 0;
}

void jabber_iq_get_register(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
xmlnode node;
xmlnode iq;
xmlnode query;
xmlnode instr;
User *u;

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

	/* needed to register new user - not supported until DataGathering gets
	 * implemented */
//	xmlnode_insert_tag(query,"email");

	/* public directory info */
	xmlnode_insert_tag(query,"first");
	xmlnode_insert_tag(query,"last");
	xmlnode_insert_tag(query,"nick");
	xmlnode_insert_tag(query,"city");
	xmlnode_insert_tag(query,"born");
	xmlnode_insert_tag(query,"gender");

	instr=xmlnode_insert_tag(query,"instructions");
	xmlnode_insert_cdata(instr,register_instructions,-1);

	u=user_get_by_jid(from);
	if (u!=NULL){
		xmlnode_insert_tag_node(query,register_change_form(u));
	}
	
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
char *username,*password,*first,*last,*nick,*city,*sex,*born,*ftype;
uin_t uin;
User *user;
Session *session=NULL;
gg_pubdir50_t change;
Request *r;

	username=password=first=last=nick=city=sex=born=NULL;

	user=user_get_by_jid(from);

	node=xmlnode_get_firstchild(q);
	if (!node){
		debug("Set query for jabber:iq:register empty: %s",xmlnode2str(q));
		unregister(s,from,to,id,0);
		return;
	}

	node=xmlnode_get_tag(q,"x?xmlns=jabber:x:data");
	if (node) {
		ftype=xmlnode_get_attrib(node,"type");
		if (ftype==NULL) {
			jabber_iq_send_error(s,from,to,id,406,_("Form returned with no type defined"));
		}
		else if (!strcmp(ftype,"submit")){ 
			if (user!=NULL) 
				register_process_change_form(s,from,to,id,user,node);
			else 
				jabber_iq_send_error(s,from,to,id,501,_("Not implemented (yet)"));
		}
		else if (!strcmp(ftype,"cancel")){ 
			jabber_iq_send_error(s,from,to,id,406,_("Cancelled"));
		}
		else jabber_iq_send_error(s,from,to,id,406,_("Bad form type"));
		return;
	}

	node=xmlnode_get_tag(q,"remove");
	if (node){
		debug("<remove/> in jabber:iq:register set: %s",xmlnode2str(q));
		unregister(s,from,to,id,0);
		return;
	}

	node=xmlnode_get_tag(q,"username");
	if (node) username=xmlnode_get_data(node);
	if (!node || !username) uin=0;
	else uin=atoi(username);

	node=xmlnode_get_tag(q,"password");
	if (node) password=xmlnode_get_data(node);

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

	node=xmlnode_get_tag(q,"first");
	if (node){
		first=from_utf8(xmlnode_get_data(node));
	}

	node=xmlnode_get_tag(q,"last");
	if (node){
		last=from_utf8(xmlnode_get_data(node));
	}

	node=xmlnode_get_tag(q,"nick");
	if (node){
		nick=from_utf8(xmlnode_get_data(node));
	}

//	node=xmlnode_get_tag(q,"email");
//	if (node)
//		gg_pubdir50_add(change, GG_PUBDIR50_email=xmlnode_get_data(node);

	node=xmlnode_get_tag(q,"city");
	if (node){
		city=from_utf8(xmlnode_get_data(node));
	}

	node=xmlnode_get_tag(q,"gender");
	if (node){
		sex=xmlnode_get_data(node);
	}

	node=xmlnode_get_tag(q,"born");
	if (node){
		born=xmlnode_get_data(node);
	}

	if (!first && !last && !nick && !city && !born && !sex){
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

	if (!nick){
		debug("nickname not given for registration");
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

	change=gg_pubdir50_new(GG_PUBDIR50_WRITE);
	if (first) gg_pubdir50_add(change, GG_PUBDIR50_FIRSTNAME, (const char *)first);
	if (last) gg_pubdir50_add(change, GG_PUBDIR50_LASTNAME, (const char *)last);
	if (nick) gg_pubdir50_add(change, GG_PUBDIR50_NICKNAME, (const char *)nick);
	if (city) gg_pubdir50_add(change, GG_PUBDIR50_CITY, (const char *)city);
	if (sex[0]=='k' || sex[0]=='f' || sex[0]=='K' || sex[0]=='F')
		gg_pubdir50_add(change, GG_PUBDIR50_GENDER,
				GG_PUBDIR50_GENDER_FEMALE);
	else if (sex!=NULL && sex[0]!='\000')
		gg_pubdir50_add(change, GG_PUBDIR50_GENDER,
				GG_PUBDIR50_GENDER_MALE);
	if (born) gg_pubdir50_add(change, GG_PUBDIR50_BIRTHYEAR, (const char *)born);

	r=add_request(RT_CHANGE,from,to,id,q,(void*)change,s);
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


