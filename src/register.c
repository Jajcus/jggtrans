/* $Id: register.c,v 1.53 2004/03/28 13:24:35 jajcus Exp $ */

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
#include "jabber.h"
#include "register.h"
#include "iq.h"
#include "users.h"
#include "sessions.h"
#include "presence.h"
#include "users.h"
#include "requests.h"
#include "encoding.h"
#include "forms.h"
#include "jid.h"
#include "debug.h"

char *register_instructions;

static struct {
	char *lang_name;
	char *locale;
} locale_mapping[]={
	{ "Polski", "pl_PL"},
	{ "Nederlands", "nl_NL"},
	{ "English", "C" },
	{ NULL, NULL}
	};

xmlnode register_form(xmlnode parent,User *u){
xmlnode form,field;
int i;
char *tmp;

	form=form_new(parent,_("Jabber GG transport registration form"),
			_("Fill in this form to regiser in the transport.\n"
			"You may use registration later to change your settings,"
			" public directory information or to unregister."));

	form_add_field(form,"text-single","uin",_("GG number"),NULL,1);
	form_add_field(form,"text-private","password",_("Password"),NULL,1);
	/* form_add_field(form,"boolean","new",_("Create new account"),"0",1); */
	field=form_add_field(form,"list-single","userlist",_("Userlist on GG server"),"get",0);
	form_add_option(field,_("ignore"),"ignore");
	form_add_option(field,_("retrieve"),"get");

	if (default_user_locale && default_user_locale[0]) tmp=default_user_locale;
	else tmp="_default_";

	field=form_add_field(form,"list-single","locale",_("Language"),tmp,0);
	form_add_option(field,_("-default-"),"_default_");
	for(i=0;locale_mapping[i].locale!=NULL;i++)
		form_add_option(field,locale_mapping[i].lang_name,locale_mapping[i].locale);

	form_add_field(form,"boolean","friends_only",_("Friends only"),"1",1);
	form_add_field(form,"boolean","invisible",_("Invisible"),"0",1);

	return form;
}


xmlnode register_change_form(xmlnode parent,User *u){
xmlnode form,field;
int i;

	form=form_new(parent,_("Registration change form"),
			_("You may use this form to change account"
			" information, change personal information"
			" in the public directory or unregister from"
			" the transport."));

	field=form_add_field(form,"list-single","action",_("Action"),"options",1);
	form_add_option(field,_("Change account options"),"options");
	/*form_add_option(field,_("Change password"),"passwd"); */
	form_add_option(field,_("Change public directory information"),"pubdir");
	form_add_option(field,_("Unregister"),"unregister");


	form_add_fixed(form,_("Fill out this part only when changing account options:"));
	field=form_add_field(form,"list-single","locale",_("Language"),
			(u->locale&&u->locale[0])?u->locale:"_default_",0);
	form_add_option(field,_("-default-"),"_default_");
	for(i=0;locale_mapping[i].locale!=NULL;i++)
		form_add_option(field,locale_mapping[i].lang_name,locale_mapping[i].locale);
	form_add_field(form,"boolean","friends_only",_("Friends only"),
						u->friends_only?"1":"0",0);
	form_add_field(form,"boolean","invisible",_("Invisible"),
						u->invisible?"1":"0",0);

	/*form_add_fixed(form,_("Fill out this part only when changing password:"));
	form_add_field(form,"text-private","newpassword",_("New password"),NULL,0);
	form_add_field(form,"text-private","newpassword2",_("Confirm new password"),NULL,0);
	form_add_field(form,"text-single","question",_("Question"),NULL,0);
	form_add_field(form,"text-single","answer",_("Answer"),NULL,0);*/

	form_add_fixed(form,_("Fill out this part only when changing public directory info:"));
	form_add_field(form,"text-single","firstname",_("First name"),NULL,0);
	form_add_field(form,"text-single","lastname",_("Last name"),NULL,0);
	form_add_field(form,"text-single","nick",_("Nick"),NULL,0);
	form_add_field(form,"text-single","birthyear",_("Birth year"),NULL,0);
	form_add_field(form,"text-single","city",_("City"),NULL,0);
	field=form_add_field(form,"list-single","gender",_("Sex"),"_none_",0);
	form_add_option(field,"-","_none_");
	form_add_option(field,_("female"),GG_PUBDIR50_GENDER_FEMALE);
	form_add_option(field,_("male"),GG_PUBDIR50_GENDER_MALE);
	form_add_field(form,"text-single","familyname",_("Family name"),NULL,0);
	form_add_field(form,"text-single","familycity",_("Family city"),NULL,0);

	return form;
}

int register_process_options_form(Stream *s,const char *from,const char *to,
					const char *id,User *u,xmlnode form){
xmlnode field,value;
char *locale=NULL,*invisible=NULL,*friends_only=NULL;
Session *session;

	field=xmlnode_get_tag(form,"field?var=locale");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) locale=xmlnode_get_data(value);
	}
	if (locale && !strcmp(locale,"_default_")) locale="";

	field=xmlnode_get_tag(form,"field?var=invisible");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) invisible=xmlnode_get_data(value);
	}
	field=xmlnode_get_tag(form,"field?var=friends_only");
	if (field!=NULL){
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
	session=session_get_by_jid(u->jid,NULL,0);
	if (session!=NULL) session_send_status(session);
	user_save(u);

	return 0;
}

#if 0 
/* Password change dissabled, as it would be hard to support token-based password change */
int register_process_passwd_form(Stream *s,const char *from,const char *to,
					const char *id,User *u,xmlnode form){
xmlnode field,value;
char *newpasswd=NULL,*newpasswdW,*newpasswd2=NULL,*question=NULL,*answer=NULL,*qa,*qaW;
struct gg_http *gghttp;
Request *r;

	field=xmlnode_get_tag(form,"field?var=newpassword");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) newpasswd=xmlnode_get_data(value);
	}
	if (newpasswd==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No new password given"));
		return -1;
	}
	field=xmlnode_get_tag(form,"field?var=newpassword2");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) newpasswd2=xmlnode_get_data(value);
	}
	if (newpasswd2==NULL || strcmp(newpasswd,newpasswd2)!=0){
		jabber_iq_send_error(s,from,to,id,406,_("Passwords do not match"));
		return -1;
	}
	if (strcmp(newpasswd,u->password)==0){
		jabber_iq_send_error(s,from,to,id,406,_("New password is the same as the old one."));
		return -1;
	}
	field=xmlnode_get_tag(form,"field?var=question");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) question=xmlnode_get_data(value);
	}
	if (question==NULL) question="";
	else if (strchr(question,'~')){
		jabber_iq_send_error(s,from,to,id,406,_("Question contains illegal characters."));
		return -1;
	}
	field=xmlnode_get_tag(form,"field?var=answer");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) answer=xmlnode_get_data(value);
	}
	if (answer==NULL) answer="";
	else if (strchr(answer,'~')){
		jabber_iq_send_error(s,from,to,id,406,_("Answer contains illegal characters."));
		return -1;
	}
	qa=g_strdup_printf("%s~%s",question,answer);
	qaW=g_strdup(from_utf8(qa));
	g_free(qa);
	newpasswdW=g_strdup(from_utf8(newpasswd));
	gghttp=gg_change_passwd3(u->uin,from_utf8(u->password),newpasswdW,qaW,1);
	g_free(newpasswdW);
	g_free(qaW);
	if (gghttp!=NULL) {
		r=add_request(RT_PASSWD,from,to,id,form,gghttp,s);
		r->data=g_strdup(newpasswd);
	}
	return 0;
}
#endif


#define FIELD_TO_PUBDIR(fieldname,symbol) \
	val=0; \
	field=xmlnode_get_tag(form,"field?var=" fieldname); \
	if (field!=NULL){ \
		value=xmlnode_get_tag(field,"value"); \
		if (value!=NULL) val=xmlnode_get_data(value); \
	} \
	if (val!=NULL && val[0]) \
		gg_pubdir50_add(change, symbol, from_utf8(val));

int register_process_pubdir_form(Stream *s,const char *from,const char *to,
					const char *id,User *u,xmlnode form,xmlnode q){
xmlnode field,value;
char *val=NULL;
Request *r;
Session *session=NULL;
gg_pubdir50_t change;

	session=session_get_by_jid(from,s,0);
	if (session==NULL){
		jabber_iq_send_error(s,from,to,id,500,_("Not logged in?"));
	}

	change=gg_pubdir50_new(GG_PUBDIR50_WRITE);

	FIELD_TO_PUBDIR("firstname",GG_PUBDIR50_FIRSTNAME);
	FIELD_TO_PUBDIR("lastname",GG_PUBDIR50_LASTNAME);
	FIELD_TO_PUBDIR("nick",GG_PUBDIR50_NICKNAME);
	FIELD_TO_PUBDIR("city",GG_PUBDIR50_CITY);

	field=xmlnode_get_tag(form,"field?var=gender");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) val=xmlnode_get_data(value);
	}
	if (val!=NULL && val[0] && ( !strcmp(val,GG_PUBDIR50_GENDER_FEMALE)
					|| !strcmp(val,GG_PUBDIR50_GENDER_FEMALE)) )
		gg_pubdir50_add(change, GG_PUBDIR50_GENDER, val);

	val=NULL;
	field=xmlnode_get_tag(form,"field?var=birthyear");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) val=xmlnode_get_data(value);
	}
	if (val!=NULL && val[0]){
		val=g_strdup_printf("%i",atoi(val));
		gg_pubdir50_add(change, GG_PUBDIR50_BIRTHYEAR, val);
		g_free(val);
	}
	FIELD_TO_PUBDIR("familyname",GG_PUBDIR50_FAMILYNAME);
	FIELD_TO_PUBDIR("familycity",GG_PUBDIR50_FAMILYCITY);

	r=add_request(RT_CHANGE,from,to,id,q,change,s);
	gg_pubdir50_free(change);
	if (!r){
		session_remove(session);
		jabber_iq_send_error(s,from,to,id,500,_("Internal Server Error"));
		return -1;
	}
	return 0;
}


int register_process_change_form(Stream *s,const char *from,const char *to,
				const char *id,User *u,xmlnode form,xmlnode q){
xmlnode field,value;
char *action;

	field=xmlnode_get_tag(form,"field?var=action");
	if (field==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No action field present"));
		return -1;
	}
	value=xmlnode_get_tag(field,"value");
	if (field==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No action value defined"));
		return -1;
	}

	action=xmlnode_get_data(value);
	if (action==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No action value defined"));
		return -1;
	}
	else if (!strcmp(action,"options")){
		if (register_process_options_form(s,from,to,id,u,form))
			return -1;
	}
#if 0
	else if (!strcmp(action,"passwd")){
		if (register_process_passwd_form(s,from,to,id,u,form))
			return -1;
	}
#endif
	else if (!strcmp(action,"pubdir")){
		if (register_process_pubdir_form(s,from,to,id,u,form,q))
			return -1;
		return 0;
	}
	else if (!strcmp(action,"unregister")){
		if (unregister(s,from,to,id,0))
			return -1;
		return 0;
	}
	else{
		jabber_iq_send_error(s,from,to,id,406,_("Bad action given"));
		return -1;
	}
	jabber_iq_send_result(s,from,to,id,NULL);
	return 0;
}

int register_process_form(Stream *s,const char *from,const char *to,
					const char *id,xmlnode form,xmlnode q){
xmlnode field,value;
char *password,*tmp;
unsigned uin;
int get_roster=0;
User *user;
Session *session;

	field=xmlnode_get_tag(form,"field?var=uin");
	if (field==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No uin field present"));
		return -1;
	}
	value=xmlnode_get_tag(field,"value");
	if (value==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No uin value defined"));
		return -1;
	}
	tmp=xmlnode_get_data(value);
	if (tmp==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No uin value defined"));
		return -1;
	}
	uin=(unsigned)atol(tmp);
	if (uin<=0){
		jabber_iq_send_error(s,from,to,id,406,_("Bad uin value defined"));
		return -1;
	}

	field=xmlnode_get_tag(form,"field?var=password");
	if (field==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No password field present"));
		return -1;
	}
	value=xmlnode_get_tag(field,"value");
	if (value==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No password value defined"));
		return -1;
	}
	password=xmlnode_get_data(value);
	if (password==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("No password value defined"));
		return -1;
	}

	field=xmlnode_get_tag(form,"field?var=userlist");
	if (field!=NULL) value=xmlnode_get_tag(field,"value");
	else value=NULL;
	if (value!=NULL){
		tmp=xmlnode_get_data(value);
		if (!strcmp(tmp,"get")) get_roster=1;
	}

	user=user_create(from,uin,password);
	if (!user){
		g_warning(N_("Couldn't create user %s"),from);
		jabber_iq_send_error(s,from,to,id,500,_("Internal Server Error"));
		return -1;
	}
			
	session=session_create(user,from,id,q,s,0);
	if (!session){
		g_warning(N_("Couldn't create session for %s"),from);
		jabber_iq_send_error(s,from,to,id,500,_("Internal Server Error"));
		return -1;
	}
	
	presence_send_subscribe(s,NULL,from);

	if (get_roster) session->get_roster=get_roster;

	register_process_options_form(s,from,to,id,user,form);
	return 0;
}

int change_password_error(struct request_s *r){

	g_message(L_("Password change error for user '%s'"),r->from);
	jabber_iq_send_error(r->stream,r->from,r->to,r->id,500,_("Internal Server Error"));
	return 0;
}

int change_password_done(struct request_s *r){
User *u;

	u=user_get_by_jid(r->from);
	if (u==NULL){
		jabber_iq_send_error(r->stream,r->from,r->to,r->id,500,_("Couldn't find the user."));
		return -1;
	}

	g_message(L_("Password changed for user '%s'"),r->from);
	if (r->data){
		g_free(u->password);
		u->password=(char *)r->data;
		user_save(u);
	}
	jabber_iq_send_result(r->stream,r->from,r->to,r->id,NULL);
	return 0;
}

void jabber_iq_get_register(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
xmlnode node;
xmlnode iq;
xmlnode query;
xmlnode instr;
User *user;

	node=xmlnode_get_firstchild(q);
	if (node){
		g_warning("Get for jabber:iq:register not empty!: %s",xmlnode2str(q));
		jabber_iq_send_error(s,from,to,id,406,_("Not Acceptable"));
		return;
	}

	user=user_get_by_jid(from);
	if (user && user->deleted) {
		jabber_iq_send_error(s,from,to,id,503,_("User still in use, try later."));
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

	if (user==NULL)
		register_form(query,user);
	else
		register_change_form(query,user);

	stream_write(s,iq);
	xmlnode_free(iq);
}

int unregister(Stream *s,const char *from,const char *to,const char *id,int presence_used){
Session *ses;
User *u;
char *jid;

	debug(L_("Unregistering '%s'"),from);
	ses=session_get_by_jid(from,NULL,0);
	if (ses)
		if (session_remove(ses)){
			g_warning(N_("'%s' unregistration failed"),from);
			jabber_iq_send_error(s,from,to,id,500,_("Internal Server Error"));
			return -1;
		}

	u=user_get_by_jid(from);
	if (!u){
		g_warning(N_("Tried to unregister '%s' who was never registered"),from);
		jabber_iq_send_error(s,from,to,id,404,_("Not Found"));
		return -1;
	}

	if (u->contacts){
		GList *it;
		Contact *c;
		char *ujid;
		for(it=g_list_first(u->contacts);it;it=it->next){
			c=(Contact *)it->data;
			ujid=jid_build(c->uin);
			presence_send_unsubscribed(s,ujid,u->jid);
			presence_send_unsubscribe(s,ujid,u->jid);
			g_free(ujid);
		}
	}

	jid=g_strdup(u->jid);
	if (user_delete(u)){
		g_warning(N_("'%s' unregistration failed"),from);
		jabber_iq_send_error(s,from,to,id,500,_("Internal Server Error"));
		return -1;
	}

	if (!presence_used){
		jabber_iq_send_result(s,from,to,id,NULL);
		presence_send_unsubscribe(s,NULL,jid);
	}
	presence_send_unsubscribed(s,NULL,jid);
	g_message(L_("User '%s' unregistered"),from);
	g_free(jid);
	return 0;
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
	if (user && user->deleted) {
		jabber_iq_send_error(s,from,to,id,503,_("User still in use, try later."));
		return;
	}

	node=xmlnode_get_firstchild(q);
	if (!node){
		debug(L_("Set query for jabber:iq:register empty: %s"),xmlnode2str(q));
		unregister(s,from,to,id,0);
		return;
	}

	node=xmlnode_get_tag(q,"x?xmlns=jabber:x:data");
	if (node){
		ftype=xmlnode_get_attrib(node,"type");
		if (ftype==NULL){
			jabber_iq_send_error(s,from,to,id,406,_("Form returned with no type defined"));
		}
		else if (!strcmp(ftype,"submit")){
			if (user!=NULL)
				register_process_change_form(s,from,to,id,user,node,q);
			else
				register_process_form(s,from,to,id,node,q);
		}
		else if (!strcmp(ftype,"cancel")){
			jabber_iq_send_error(s,from,to,id,406,_("Cancelled"));
		}
		else jabber_iq_send_error(s,from,to,id,406,_("Bad form type"));
		return;
	}

	node=xmlnode_get_tag(q,"remove");
	if (node){
		debug(L_("<remove/> in jabber:iq:register set: %s"),xmlnode2str(q));
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
		g_warning(N_("User '%s' doesn't exist and not enough info to add him"),from);
		jabber_iq_send_error(s,from,to,id,406,_("Not Acceptable"));
		return;
	}

	if (!user){
		user=user_create(from,uin,password);
		if (!user){
			g_warning(N_("Couldn't create user %s"),from);
			jabber_iq_send_error(s,from,to,id,500,_("Internal Server Error"));
			return;
		}

		session=session_create(user,from,id,q,s,0);
		if (!session){
			g_warning(N_("Couldn't create session for %s"),from);
			jabber_iq_send_error(s,from,to,id,500,_("Internal Server Error"));
			return;
		}
	}
	else session=session_get_by_jid(from,s,0);

	node=xmlnode_get_tag(q,"first");
	if (node) first=xmlnode_get_data(node);

	node=xmlnode_get_tag(q,"last");
	if (node) last=xmlnode_get_data(node);

	node=xmlnode_get_tag(q,"nick");
	if (node) nick=xmlnode_get_data(node);

/*	node=xmlnode_get_tag(q,"email");
	if (node) email=xmlnode_get_data(node); */

	node=xmlnode_get_tag(q,"city");
	if (node) city=xmlnode_get_data(node);

	node=xmlnode_get_tag(q,"gender");
	if (node) sex=xmlnode_get_data(node);

	node=xmlnode_get_tag(q,"born");
	if (node) born=xmlnode_get_data(node);

	if (!first && !last && !nick && !city && !born && !sex){
			if (!uin && !password){
				debug(L_("Set query for jabber:iq:register empty: %s"),xmlnode2str(q));
				unregister(s,from,to,id,0);
				return;
			}
			if (!uin || !password){
				g_warning(N_("Nothing to change"));
				session_remove(session);
				jabber_iq_send_error(s,from,to,id,406,_("Not Acceptable"));
				return;
			}
			presence_send_subscribe(s,NULL,from);
			return;
	}

	if (!user && (!password ||!uin)){
		g_warning(N_("Not registered, and no password given for public directory change."));
		session_remove(session);
		jabber_iq_send_error(s,from,to,id,406,_("Not Acceptable"));
		return;
	}
	if (!password) password=user->password;
	if (!uin) uin=user->uin;

	change=gg_pubdir50_new(GG_PUBDIR50_WRITE);
	if (first) gg_pubdir50_add(change, GG_PUBDIR50_FIRSTNAME, from_utf8(first));
	if (last) gg_pubdir50_add(change, GG_PUBDIR50_LASTNAME, from_utf8(last));
	if (nick) gg_pubdir50_add(change, GG_PUBDIR50_NICKNAME, from_utf8(nick));
	if (city) gg_pubdir50_add(change, GG_PUBDIR50_CITY, from_utf8(city));
	if (sex && (sex[0]=='k' || sex[0]=='f' || sex[0]=='K' || sex[0]=='F'))
		gg_pubdir50_add(change, GG_PUBDIR50_GENDER,
				GG_PUBDIR50_GENDER_FEMALE);
	else if (sex!=NULL && sex[0]!='\000')
		gg_pubdir50_add(change, GG_PUBDIR50_GENDER,
				GG_PUBDIR50_GENDER_MALE);
	if (born){
		born=g_strdup_printf("%i",atoi(born));
		gg_pubdir50_add(change, GG_PUBDIR50_BIRTHYEAR, born);
		g_free(born);
	}

	if (session->connected){
		r=add_request(RT_CHANGE,from,to,id,NULL,change,s);
		gg_pubdir50_free(change);
	}
	else{
		session->pubdir_change=change;
	}
}

int register_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->to,r->id,502,_("Remote Server Error"));
	return 0;
}

int register_done(struct request_s *r){

	jabber_iq_send_result(r->stream,r->from,r->to,r->id,NULL);
	return 0;
}


