/* $Id: search.c,v 1.16 2003/01/22 07:53:01 jajcus Exp $ */

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
#include <libgadu.h>
#include "search.h"
#include "requests.h"
#include "stream.h"
#include "iq.h"
#include "jid.h"
#include "encoding.h"
#include "debug.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


const char *search_instructions;

int search_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->to,r->id,502,"Remote Server Error");
	return 0;
}

int search_done(struct request_s *r){
xmlnode q,item,n;
struct gg_search * results;
char *jid,*name,*str;
int i;

	results=(struct gg_search *)r->gghttp->data;
	q=xmlnode_new_tag("query");
	xmlnode_put_attrib(q,"xmlns","jabber:iq:search");
	for(i=0;results && i<results->count;i++){
		item=xmlnode_insert_tag(q,"item");
		jid=jid_build(results->results[i].uin);
		xmlnode_put_attrib(item,"jid",jid);
		g_free(jid);
		name=g_strdup_printf("%s %s",results->results[i].first_name,results->results[i].last_name);
		n=xmlnode_insert_tag(item,"name");
		xmlnode_insert_cdata(n,to_utf8(name),-1);
		g_free(name);
		n=xmlnode_insert_tag(item,"first");
		xmlnode_insert_cdata(n,to_utf8(results->results[i].first_name),-1);
		n=xmlnode_insert_tag(item,"last");
		xmlnode_insert_cdata(n,to_utf8(results->results[i].last_name),-1);
		n=xmlnode_insert_tag(item,"nick");
		xmlnode_insert_cdata(n,to_utf8(results->results[i].nickname),-1);
		if (results->results[i].born>0){
			str=g_strdup_printf("%i",results->results[i].born);
			n=xmlnode_insert_tag(item,"born");
			xmlnode_insert_cdata(n,str,-1);
			g_free(str);
		}
		if (results->results[i].gender==GG_GENDER_FEMALE)
			xmlnode_insert_cdata(xmlnode_insert_tag(item,"gender"),"f",-1);
		else if (results->results[i].gender==GG_GENDER_MALE)
			xmlnode_insert_cdata(xmlnode_insert_tag(item,"gender"),"m",-1);
		n=xmlnode_insert_tag(item,"city");
		xmlnode_insert_cdata(n,to_utf8(results->results[i].city),-1);
		if (results->results[i].active)
			xmlnode_insert_cdata(xmlnode_insert_tag(item,"active"),"yes",-1);
	}
	jabber_iq_send_result(r->stream,r->from,r->to,r->id,q);
	xmlnode_free(q);
	gg_free_search(r->gghttp);
	r->gghttp=NULL;
	return 0;
}

void jabber_iq_get_search(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
xmlnode iq,n;

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
	xmlnode_insert_tag(iq,"email");
	xmlnode_insert_tag(iq,"phone");
	xmlnode_insert_tag(iq,"username");

	jabber_iq_send_result(s,from,to,id,iq);
	xmlnode_free(iq);
}

void jabber_iq_set_search(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
struct gg_search_request sr;
xmlnode n;
struct gg_http *gghttp;
Request *r;
char *data;

	q=xmlnode_dup(q);
	memset(&sr,0,sizeof(sr));
	n=xmlnode_get_tag(q,"active");
	if (n){
		data=xmlnode_get_data(n);
		if (data!=NULL && (data[0]=='y' || data[0]=='Y' || data[0]=='t' || data[0]=='T'))
				sr.active=1;
	}
	n=xmlnode_get_tag(q,"nick");
	sr.nickname=NULL;
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			sr.nickname=g_strdup(from_utf8(data));
	}
	n=xmlnode_get_tag(q,"first");
	sr.first_name=NULL;
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			sr.first_name=g_strdup(from_utf8(data));
	}
	n=xmlnode_get_tag(q,"last");
	sr.last_name=NULL;
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			sr.last_name=g_strdup(from_utf8(data));
	}
	n=xmlnode_get_tag(q,"city");
	sr.city=NULL;
	if (n){
		data=xmlnode_get_data(n);
		sr.city=g_strdup(from_utf8(data));
	}
	n=xmlnode_get_tag(q,"gender");
	if (n){
		data=xmlnode_get_data(n);
		if (!data || !data[0]) sr.gender=GG_GENDER_NONE;
		else if (data[0]=='k' || data[0]=='f' || data[0]=='K' || data[0]=='F') sr.gender=GG_GENDER_FEMALE;
		else sr.gender=GG_GENDER_MALE;
	}
	else sr.gender=GG_GENDER_NONE;
	n=xmlnode_get_tag(q,"born");
	if (n){
		data=xmlnode_get_data(n);
		if (data) sscanf(data,"%u-%u",&sr.min_birth,&sr.max_birth);
	}
	n=xmlnode_get_tag(q,"email");
	sr.email=NULL;
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			sr.email=g_strdup(from_utf8(data));
	}
	n=xmlnode_get_tag(q,"phone");
	sr.phone=NULL;
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			sr.phone=g_strdup(from_utf8(data));
	}
	n=xmlnode_get_tag(q,"username");
	sr.uin=0;
	if (n){
		data=xmlnode_get_data(n);
		if (data)
			sr.uin=atoi(data);
	}

	debug("gg_search()");
	gghttp=gg_search(&sr,1);

	if (sr.nickname) g_free(sr.nickname);
	if (sr.first_name) g_free(sr.first_name);
	if (sr.last_name) g_free(sr.last_name);
	if (sr.city) g_free(sr.city);
	if (sr.email) g_free(sr.email);
	if (sr.phone) g_free(sr.phone);

	if (!gghttp){
		jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
		return;
	}

	r=add_request(RT_SEARCH,from,to,id,q,gghttp,s);
	if (!r) jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
}

void jabber_iq_get_user_vcard(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
struct gg_search_request sr;
struct gg_http *gghttp;
Request *r;

	q=xmlnode_dup(q);
	memset(&sr,0,sizeof(sr));
	sr.uin=jid_get_uin(to);

	gghttp=gg_search(&sr,1);
	if (!gghttp){
		jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
		return;
	}

	r=add_request(RT_VCARD,from,to,id,q,gghttp,s);
	if (!r) jabber_iq_send_error(s,from,to,id,500,"Internal Server Error");
}

int vcard_done(struct request_s *r){
xmlnode vc,n,n1;
struct gg_search * results;
char *jid,*name=NULL,*str;
GList *it;
Contact *c;
User *u;


	results=(struct gg_search *)r->gghttp->data;
	if (!results || !results->count){
		jabber_iq_send_error(r->stream,r->from,r->to,r->id,404,"Not Found");
		gg_free_search(r->gghttp);
		r->gghttp=NULL;
		return 1;
	}

	vc=xmlnode_new_tag("vCard");
	xmlnode_put_attrib(vc,"xmlns","vcard-temp");
	xmlnode_put_attrib(vc,"version","2.0");

	n=xmlnode_insert_tag(vc,"FN");
	if (results->results[0].first_name && results->results[0].last_name){
		name=g_strdup_printf("%s %s",results->results[0].first_name,
							results->results[0].last_name);
		xmlnode_insert_cdata(n,to_utf8(name),-1);
	}
	else if (results->results[0].first_name){
		xmlnode_insert_cdata(n,to_utf8(results->results[0].first_name),-1);
	}
	else if (results->results[0].last_name){
		xmlnode_insert_cdata(n,to_utf8(results->results[0].last_name),-1);
	}
	g_free(name);

	n1=xmlnode_insert_tag(vc,"N");
	n=xmlnode_insert_tag(n1,"GIVEN");
	if (results->results[0].first_name){
		xmlnode_insert_cdata(n,to_utf8(results->results[0].first_name),-1);
	}
	n=xmlnode_insert_tag(n1,"FAMILY");
	if (results->results[0].last_name){
		xmlnode_insert_cdata(n,to_utf8(results->results[0].last_name),-1);
	}

	n=xmlnode_insert_tag(vc,"NICKNAME");
	if (results->results[0].nickname){
		xmlnode_insert_cdata(n,to_utf8(results->results[0].nickname),-1);
	}

	if (results->results[0].born>0){
		n=xmlnode_insert_tag(vc,"BDAY");
		str=g_strdup_printf("%04i-00-00",results->results[0].born);
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);
	}

	n1=xmlnode_insert_tag(vc,"ADR");
	xmlnode_insert_tag(n1,"HOME");
	n=xmlnode_insert_tag(n1,"LOCALITY");
	xmlnode_insert_cdata(n,to_utf8(results->results[0].city),-1);

	jid=jid_build(results->results[0].uin);
	n=xmlnode_insert_tag(vc,"JABBERID");
	xmlnode_insert_cdata(n,jid,-1);
	g_free(jid);

	c=NULL;
	u=user_get_by_jid(r->from);
	if (u){
		for(it=g_list_first(u->contacts);it;it=it->next){
			c=(Contact *)it->data;
			if (c->uin==results->results[0].uin) break;
		}
		if (it==NULL) c=NULL;
	}

	n=xmlnode_insert_tag(vc,"DESC");
	xmlnode_insert_cdata(n,"GG user\n",-1);
	if (c!=NULL){
		struct in_addr a;
		a.s_addr=c->ip;
		str=g_strdup_printf("Client version: 0x%x\n",c->version);
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);
		str=g_strdup_printf("User IP: %s\n",inet_ntoa(a));
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);
		str=g_strdup_printf("User port: %u\n",(unsigned)c->port);
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);
	}

	jabber_iq_send_result(r->stream,r->from,r->to,r->id,vc);
	xmlnode_free(vc);
	gg_free_search(r->gghttp);
	r->gghttp=NULL;
	return 0;
}

int vcard_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->to,r->id,502,"Remote Server Error");
	return 0;
}
