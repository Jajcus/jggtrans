/* $Id: search.c,v 1.10 2002/02/06 17:23:37 jajcus Exp $ */

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

#include <libgadu.h>
#include "ggtrans.h"
#include "search.h"
#include "requests.h"
#include "stream.h"
#include "iq.h"
#include "jid.h"
#include "encoding.h"
#include "debug.h"

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
}

void jabber_iq_set_search(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
struct gg_search_request sr;
xmlnode n;
struct gg_http *gghttp;
Request *r;
char *p,*str;

	q=xmlnode_dup(q);
	memset(&sr,0,sizeof(sr));
	n=xmlnode_get_tag(q,"active");
	if (n){
		str=xmlnode_get_data(n);
		if (str && (str[0]=='y' || str[0]=='Y' || str[0]=='t' || str[0]=='T'))
				sr.active=1;
	}
	n=xmlnode_get_tag(q,"nick");
	if (n) sr.nickname=g_strdup(from_utf8(xmlnode_get_data(n)));
	else sr.nickname=NULL;
	n=xmlnode_get_tag(q,"first");
	if (n) sr.first_name=g_strdup(from_utf8(xmlnode_get_data(n)));
	else sr.first_name=NULL;
	n=xmlnode_get_tag(q,"last");
	if (n) sr.last_name=g_strdup(from_utf8(xmlnode_get_data(n)));
	else sr.last_name=NULL;
	n=xmlnode_get_tag(q,"city");
	if (n) sr.city=g_strdup(from_utf8(xmlnode_get_data(n)));
	else sr.city=NULL;
	n=xmlnode_get_tag(q,"gender");
	if (n){
		str=xmlnode_get_data(n);
		if (!str || !str[0]) sr.gender=GG_GENDER_NONE;
		else if (str[0]=='k' || str[0]=='f' || str[0]=='K' || str[0]=='F') sr.gender=GG_GENDER_FEMALE;
		else sr.gender=GG_GENDER_MALE;
	}
	else sr.gender=GG_GENDER_NONE;
	n=xmlnode_get_tag(q,"born");
	if (n){
		str=xmlnode_get_data(n);
		if (str) sscanf(str,"%u-%u",&sr.min_birth,&sr.max_birth);
	}
	n=xmlnode_get_tag(q,"email");
	if (n) sr.email=g_strdup(from_utf8(xmlnode_get_data(n)));
	else sr.email=NULL;
	n=xmlnode_get_tag(q,"phone");
	if (n) sr.phone=g_strdup(from_utf8(xmlnode_get_data(n)));
	else sr.phone=NULL;
	n=xmlnode_get_tag(q,"username");
	if (n) sr.uin=atoi(xmlnode_get_data(n));
	else sr.uin=0;

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
char *jid,*name,*str;
	
	results=(struct gg_search *)r->gghttp->data;
	vc=xmlnode_new_tag("vCard");
	xmlnode_put_attrib(vc,"xmlns","vcard-temp");
	xmlnode_put_attrib(vc,"version","2.0");
	
	name=g_strdup_printf("%s %s",results->results[0].first_name,results->results[0].last_name);	
	n=xmlnode_insert_tag(vc,"FN");
	xmlnode_insert_cdata(n,to_utf8(name),-1);
	g_free(name);
	
	n1=xmlnode_insert_tag(vc,"N");
	n=xmlnode_insert_tag(n1,"GIVEN");
	xmlnode_insert_cdata(n,to_utf8(results->results[0].first_name),-1);
	n=xmlnode_insert_tag(n1,"FAMILY");
	xmlnode_insert_cdata(n,to_utf8(results->results[0].last_name),-1);
	
	n=xmlnode_insert_tag(vc,"NICKNAME");
	xmlnode_insert_cdata(n,to_utf8(results->results[0].nickname),-1);

	if (results->results[0].born>0){
		n=xmlnode_insert_tag(vc,"BDAY");
		str=g_strdup_printf("%04i-00-00",results->results[0].born);
		xmlnode_insert_cdata(n,str,-1);
		g_free(str);	
	}
		
	n1=xmlnode_insert_tag(vc,"ADR");
	n=xmlnode_insert_tag(n1,"LOCALITY");
	xmlnode_insert_cdata(n,to_utf8(results->results[0].city),-1);
	
	jid=jid_build(results->results[0].uin);
	n=xmlnode_insert_tag(vc,"JABBERID");
	xmlnode_insert_cdata(n,jid,-1);
	g_free(jid);
	
	jabber_iq_send_result(r->stream,r->from,r->to,r->id,vc);
	return 0;
}

int vcard_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->to,r->id,502,"Remote Server Error");
	return 0;
}
