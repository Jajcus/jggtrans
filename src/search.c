/* $Id: search.c,v 1.37 2003/06/27 17:30:51 jajcus Exp $ */

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

#include "ggtrans.h"
#include <libgadu.h>
#include "search.h"
#include "requests.h"
#include "sessions.h"
#include "stream.h"
#include "iq.h"
#include "jid.h"
#include "encoding.h"
#include "gg_versions.h"
#include "forms.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "debug.h"


const char *search_instructions;

void jabber_iq_set_search_byform(Stream *s,const char *from,const char *to,
				const char *id,xmlnode q);

xmlnode search_form(xmlnode parent){
xmlnode form,field;

	form=form_new(parent,_("GG public directory search"),_("Enter your search filter"));

	form_add_field(form,"text-single","uin",_("GG number"),NULL,0);
	form_add_field(form,"text-single","nick",_("Nick"),NULL,0);
	form_add_field(form,"text-single","firstname",_("First name"),NULL,0);
	form_add_field(form,"text-single","lastname",_("Last name"),NULL,0);
	form_add_field(form,"text-single","birthyear",_("Birth year"),NULL,0);
	form_add_field(form,"text-single","city",_("City"),NULL,0);
	field=form_add_field(form,"list-single","gender",_("Sex"),"_any_",0);
	form_add_option(field,_("Any"),"_any_");
	form_add_option(field,_("Female"),GG_PUBDIR50_GENDER_FEMALE);
	form_add_option(field,_("Male"),GG_PUBDIR50_GENDER_MALE);
	form_add_field(form,"boolean","active",_("Active only"),"0",0);
	form_add_field(form,"text-single","familyname",_("Family name"),NULL,0);
	form_add_field(form,"text-single","familycity",_("Family city"),NULL,0);
	return form;
}

#define GG_SEARCH_FRIENDS_MASK	0x0080

int search_byform_done(struct request_s *r, gg_pubdir50_t results){
int i;
xmlnode q,form,item;
char *jid;
const char *val;

	q=xmlnode_new_tag("query");
	xmlnode_put_attrib(q,"xmlns","jabber:iq:search");
	form=form_new_result(_("GG public directory search results"));

	form_add_result_field(form,"jid",_("JID"),"jid-single");
	form_add_result_field(form,"nick",_("Nick"),"text-single");
	form_add_result_field(form,"status",_("Status"),"text-single");
	form_add_result_field(form,"firstname",_("First name"),"text-single");
	form_add_result_field(form,"lastname",_("Last name"),"text-single");
	form_add_result_field(form,"city",_("City"),"text-single");
	form_add_result_field(form,"gender",_("Sex"),"text-single");
	form_add_result_field(form,"birthyear","Birth year","text-single");
	form_add_result_field(form,"familyname",_("Family name"),"text-single");
	form_add_result_field(form,"familycity",_("Family city"),"text-single");

	for(i=0;i<gg_pubdir50_count(results);i++){
		item=form_add_result_item(form);
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_UIN);
		if (val==NULL) continue;
		jid=jid_build(atoi(val));
		form_add_result_value(item,"jid",jid);
		g_free(jid);
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_NICKNAME);
		form_add_result_value(item,"nick",to_utf8(val));
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_STATUS);
		switch((val)?(atoi(val) & ~GG_SEARCH_FRIENDS_MASK):-1){
			case GG_STATUS_AVAIL:
				form_add_result_value(item,"status",_("Available"));
				break;
			case GG_STATUS_BUSY:
				form_add_result_value(item,"status",_("Away"));
				break;
			default:
				form_add_result_value(item,"status",_("Unavailable"));
				break;
		}
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_FIRSTNAME);
		form_add_result_value(item,"firstname",to_utf8(val));
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_LASTNAME);
		form_add_result_value(item,"lastname",to_utf8(val));
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_CITY);
		form_add_result_value(item,"city",to_utf8(val));
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_GENDER);
		if (val && !strcmp(val,GG_PUBDIR50_GENDER_FEMALE))
			form_add_result_value(item,"gender",_("female"));
		else if (val && !strcmp(val,GG_PUBDIR50_GENDER_MALE))
			form_add_result_value(item,"gender",_("male"));
		else form_add_result_value(item,"gender","");
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_BIRTHYEAR);
		form_add_result_value(item,"birthyear",to_utf8(val));
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_FAMILYNAME);
		form_add_result_value(item,"familyname",to_utf8(val));
		val=gg_pubdir50_get(results, i, GG_PUBDIR50_FAMILYCITY);
		form_add_result_value(item,"familycity",to_utf8(val));
	}
	xmlnode_insert_tag_node(q,form);
	jabber_iq_send_result(r->stream,r->from,r->to,r->id,q);
	xmlnode_free(q);
	xmlnode_free(form);
	return 0;
}

int search_done(struct request_s *r, gg_pubdir50_t results){
xmlnode q,item,n;
char *jid,*name;
int i;
const char *uin, *first_name, *last_name, *nickname, *born, *city, *gender, *active;

	n=xmlnode_get_tag(r->query,"x?xmlns=jabber:x:data");
	if (n){
		return search_byform_done(r,results);
	}
	q=xmlnode_new_tag("query");
	xmlnode_put_attrib(q,"xmlns","jabber:iq:search");
	for(i=0;i<gg_pubdir50_count(results);i++){
		item=xmlnode_insert_tag(q,"item");
		uin=gg_pubdir50_get(results, i, GG_PUBDIR50_UIN);
		if (uin==NULL) continue;
		jid=jid_build(atoi(uin));
		xmlnode_put_attrib(item,"jid",jid);
		g_free(jid);
		first_name=gg_pubdir50_get(results, i, GG_PUBDIR50_FIRSTNAME);
		last_name=gg_pubdir50_get(results, i, GG_PUBDIR50_LASTNAME);
		name=g_strdup_printf("%s %s",first_name,last_name);
		n=xmlnode_insert_tag(item,"name");
		xmlnode_insert_cdata(n,to_utf8(name),-1);
		g_free(name);
		n=xmlnode_insert_tag(item,"first");
		xmlnode_insert_cdata(n,to_utf8(first_name),-1);
		n=xmlnode_insert_tag(item,"last");
		xmlnode_insert_cdata(n,to_utf8(last_name),-1);
		nickname=gg_pubdir50_get(results, i, GG_PUBDIR50_NICKNAME);
		n=xmlnode_insert_tag(item,"nick");
		xmlnode_insert_cdata(n,to_utf8(nickname),-1);
		born=gg_pubdir50_get(results, i, GG_PUBDIR50_BIRTHYEAR);
		if (born){
			n=xmlnode_insert_tag(item,"born");
			xmlnode_insert_cdata(n,born,-1);
		}
		gender=gg_pubdir50_get(results, i, GG_PUBDIR50_GENDER);
		if (gender && strcmp(gender, GG_PUBDIR50_GENDER_FEMALE)==0)
			xmlnode_insert_cdata(xmlnode_insert_tag(item,"gender"),"f",-1);
		else if (gender && strcmp(gender, GG_PUBDIR50_GENDER_MALE)==0)
			xmlnode_insert_cdata(xmlnode_insert_tag(item,"gender"),"m",-1);
		city=gg_pubdir50_get(results, i, GG_PUBDIR50_CITY);
		n=xmlnode_insert_tag(item,"city");
		xmlnode_insert_cdata(n,to_utf8(city),-1);
		active=gg_pubdir50_get(results, i, GG_PUBDIR50_ACTIVE);
		if (active && strcmp(active, GG_PUBDIR50_ACTIVE_TRUE)==0)
			xmlnode_insert_cdata(xmlnode_insert_tag(item,"active"),"yes",-1);
	}
	jabber_iq_send_result(r->stream,r->from,r->to,r->id,q);
	xmlnode_free(q);
	return 0;
}

void jabber_iq_get_search(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
xmlnode iq,n;
Session *sess;

	sess=session_get_by_jid(from,NULL,0);
	if (!sess || !sess->connected){
		jabber_iq_send_error(s,from,to,id,407,_("Not logged in"));
		return;
	}

	iq=xmlnode_new_tag("query");
	xmlnode_put_attrib(iq,"xmlns","jabber:iq:search");
	n=xmlnode_insert_tag(iq,"instructions");
	xmlnode_insert_cdata(n,search_instructions,-1);
	xmlnode_insert_tag(iq,"active");
	xmlnode_insert_tag(iq,"first");
	xmlnode_insert_tag(iq,"last");
	xmlnode_insert_tag(iq,"nick");
	xmlnode_insert_tag(iq,"city");
	xmlnode_insert_tag(iq,"gender");
	xmlnode_insert_tag(iq,"born");
	xmlnode_insert_tag(iq,"phone");
	xmlnode_insert_tag(iq,"username");
	search_form(iq);

	jabber_iq_send_result(s,from,to,id,iq);
	xmlnode_free(iq);
}

#define FIELD_TO_PUBDIR(fieldname,symbol) \
	val=NULL; \
	field=xmlnode_get_tag(form,"field?var=" fieldname); \
	if (field!=NULL){ \
		value=xmlnode_get_tag(field,"value"); \
		if (value!=NULL) val=xmlnode_get_data(value); \
	} \
	if (val!=NULL && val[0]) \
		gg_pubdir50_add(search, symbol, from_utf8(val));

void jabber_iq_set_search_byform(Stream *s,const char *from,const char *to,
		const char *id,xmlnode q){
xmlnode form,field,value;
char *val;
gg_pubdir50_t search;
Session *sess;
Request *r;

	form=xmlnode_get_tag(q,"x?xmlns=jabber:x:data");

	sess=session_get_by_jid(from,NULL,0);
	if (!sess || !sess->connected){
		jabber_iq_send_error(s,from,to,id,407,_("Not logged in"));
		return;
	}

	search=gg_pubdir50_new(GG_PUBDIR50_SEARCH);
	FIELD_TO_PUBDIR("uin",GG_PUBDIR50_UIN);
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
		gg_pubdir50_add(search, GG_PUBDIR50_GENDER, val);

	val=NULL;
	field=xmlnode_get_tag(form,"field?var=active");
	if (field!=NULL){
		value=xmlnode_get_tag(field,"value");
		if (value!=NULL) val=xmlnode_get_data(value);
	}
	if (val!=NULL && strcmp(val,"1")==0 )
		gg_pubdir50_add(search, GG_PUBDIR50_ACTIVE,GG_PUBDIR50_ACTIVE_TRUE);

	FIELD_TO_PUBDIR("birthyear",GG_PUBDIR50_BIRTHYEAR);
	FIELD_TO_PUBDIR("familyname",GG_PUBDIR50_FAMILYNAME);
	FIELD_TO_PUBDIR("familycity",GG_PUBDIR50_FAMILYCITY);

	gg_pubdir50_add(search, GG_PUBDIR50_START, "0");

	r=add_request(RT_SEARCH,from,to,id,q,search,s);
	gg_pubdir50_free(search);
}

void jabber_iq_set_search(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
gg_pubdir50_t sr;
xmlnode n;
Session *sess;
char *data;

	n=xmlnode_get_tag(q,"x?xmlns=jabber:x:data");
	if (n){
		return jabber_iq_set_search_byform(s,from,to,id,q);
	}
	sess=session_get_by_jid(from,NULL,0);
	if (!sess || !sess->connected){
		jabber_iq_send_error(s,from,to,id,407,_("Not logged in"));
		return;
	}

	q=xmlnode_dup(q);
	sr=gg_pubdir50_new(GG_PUBDIR50_SEARCH);
	n=xmlnode_get_tag(q,"active");
	if (n){
		data=xmlnode_get_data(n);
		if (data!=NULL && (data[0]=='y' || data[0]=='Y' || data[0]=='t' || data[0]=='T'))
			gg_pubdir50_add(sr, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);
	}
	n=xmlnode_get_tag(q,"nick");
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			gg_pubdir50_add(sr, GG_PUBDIR50_NICKNAME, from_utf8(data));
	}
	n=xmlnode_get_tag(q,"first");
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			gg_pubdir50_add(sr, GG_PUBDIR50_FIRSTNAME, from_utf8(data));
	}
	n=xmlnode_get_tag(q,"last");
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			gg_pubdir50_add(sr, GG_PUBDIR50_LASTNAME, from_utf8(data));
	}
	n=xmlnode_get_tag(q,"city");
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			gg_pubdir50_add(sr, GG_PUBDIR50_CITY, from_utf8(data));
	}
	n=xmlnode_get_tag(q,"gender");
	if (n){
		data=xmlnode_get_data(n);
		if (data!=NULL){
			if (data[0]=='k' || data[0]=='f' || data[0]=='K' || data[0]=='F')
				gg_pubdir50_add(sr, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_FEMALE);
			else if (data[0]=='m' || data[0]=='M')
				gg_pubdir50_add(sr, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_MALE);
		}
	}
	n=xmlnode_get_tag(q,"born");
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			gg_pubdir50_add(sr, GG_PUBDIR50_BIRTHYEAR, data);
	}
	n=xmlnode_get_tag(q,"username");
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			gg_pubdir50_add(sr, GG_PUBDIR50_UIN, data);
	}

	add_request(RT_SEARCH,from,to,id,q,(void*)sr,s);
	gg_pubdir50_free(sr);
}

void jabber_iq_get_user_vcard(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
int i=0;
char *uin;
gg_pubdir50_t sr;

	sr=gg_pubdir50_new(GG_PUBDIR50_SEARCH);

	while(to[i]!='@')
		i++;

	uin=g_strndup(to, i);

	gg_pubdir50_add(sr, GG_PUBDIR50_UIN, uin);

	add_request(RT_VCARD,from,to,id,q,(void*)sr,s);

	gg_pubdir50_free(sr);
	g_free(uin);
}

int vcard_done(struct request_s *r, gg_pubdir50_t results){
xmlnode vc,n,n1;
char *jid,*name=NULL,*str;
GList *it;
Contact *c;
User *u;
const char *uin, *first_name, *last_name, *nickname, *born, *city;


	if (gg_pubdir50_count(results)<1){
		jabber_iq_send_error(r->stream,r->from,r->to,r->id,404,_("Not Found"));
		return 1;
	}

	uin=gg_pubdir50_get(results, 0, GG_PUBDIR50_UIN);
	first_name=gg_pubdir50_get(results, 0, GG_PUBDIR50_FIRSTNAME);
	last_name=gg_pubdir50_get(results, 0, GG_PUBDIR50_LASTNAME);
	nickname=gg_pubdir50_get(results, 0, GG_PUBDIR50_NICKNAME);
	born=gg_pubdir50_get(results, 0, GG_PUBDIR50_BIRTHYEAR);
	city=gg_pubdir50_get(results, 0, GG_PUBDIR50_CITY);

	vc=xmlnode_new_tag("vCard");
	xmlnode_put_attrib(vc,"xmlns","vcard-temp");
	xmlnode_put_attrib(vc,"version","2.0");

	n=xmlnode_insert_tag(vc,"FN");
	if (first_name && last_name){
		name=g_strdup_printf("%s %s",first_name,
							last_name);
		xmlnode_insert_cdata(n,to_utf8(name),-1);
	}
	else if (first_name){
		xmlnode_insert_cdata(n,to_utf8(first_name),-1);
	}
	else if (last_name){
		xmlnode_insert_cdata(n,to_utf8(last_name),-1);
	}
	g_free(name);

	n1=xmlnode_insert_tag(vc,"N");
	n=xmlnode_insert_tag(n1,"GIVEN");
	if (first_name){
		xmlnode_insert_cdata(n,to_utf8(first_name),-1);
	}
	n=xmlnode_insert_tag(n1,"FAMILY");
	if (last_name){
		xmlnode_insert_cdata(n,to_utf8(last_name),-1);
	}

	n=xmlnode_insert_tag(vc,"NICKNAME");
	if (nickname){
		xmlnode_insert_cdata(n,to_utf8(nickname),-1);
	}

	if (born){
		n=xmlnode_insert_tag(vc,"BDAY");
		xmlnode_insert_cdata(n,born,-1);
	}

	n1=xmlnode_insert_tag(vc,"ADR");
	xmlnode_insert_tag(n1,"HOME");
	n=xmlnode_insert_tag(n1,"LOCALITY");
	xmlnode_insert_cdata(n,to_utf8(city),-1);

	if (uin){
		jid=jid_build(atoi(uin));
		n=xmlnode_insert_tag(vc,"JABBERID");
		xmlnode_insert_cdata(n,jid,-1);
		g_free(jid);
	}

	c=NULL;
	u=user_get_by_jid(r->from);
	if (u){
		for(it=g_list_first(u->contacts);it;it=it->next){
			c=(Contact *)it->data;
			if (c->uin==atoi(uin)) break;
		}
		if (it==NULL) c=NULL;
	}

	n=xmlnode_insert_tag(vc,"DESC");
	xmlnode_insert_cdata(n,_("GG user\n"),-1);
	if (c!=NULL){
		struct in_addr a;
		a.s_addr=c->ip;
		str=g_strdup_printf(_("Client version: %s (prot.0x%02X)\n"),
					((c->version & 0xff) < GG_VERSION_ELEMENTS) && gg_version[c->version & 0xff]
						? gg_version[c->version & 0xff]
						: "?.?.?",
					c->version & 0xff);
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);
		str=g_strdup_printf(_("User IP: %s\n"),inet_ntoa(a));
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);
		str=g_strdup_printf(_("User port: %u\n"),(unsigned)c->port);
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);
	}

	jabber_iq_send_result(r->stream,r->from,r->to,r->id,vc);
	xmlnode_free(vc);
	return 0;
}

