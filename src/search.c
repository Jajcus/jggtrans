#include <libgg.h>
#include "search.h"
#include "requests.h"
#include "stream.h"
#include "iq.h"
#include "jid.h"
#include "encoding.h"
#include <glib.h>

const char *search_instructions;

int search_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->id,"Search error");
	return 0;
}

int search_done(struct request_s *r){
xmlnode q,item,n;
struct gg_search * results;
char *jid,*name;
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
		n=xmlnode_insert_tag(item,"email");
		xmlnode_insert_cdata(n,"",-1);
		n=xmlnode_insert_tag(item,"city");
		xmlnode_insert_cdata(n,to_utf8(results->results[i].city),-1);
	}
	jabber_iq_send_result(r->stream,r->from,r->id,q);
	return 0;
}

void jabber_iq_get_search(Stream *s,const char *from,const char *id,xmlnode q){
xmlnode iq,n;

	iq=xmlnode_new_tag("query");
	xmlnode_put_attrib(iq,"xmlns","jabber:iq:search");
	n=xmlnode_insert_tag(iq,"instructions");
	xmlnode_insert_cdata(n,search_instructions,-1);
	xmlnode_insert_tag(iq,"first");
	xmlnode_insert_tag(iq,"last");
	xmlnode_insert_tag(iq,"nick");
	xmlnode_insert_tag(iq,"city");
	xmlnode_insert_tag(iq,"email");
	xmlnode_insert_tag(iq,"phone");
	xmlnode_insert_tag(iq,"uin");

	jabber_iq_send_result(s,from,id,iq);
}

void jabber_iq_set_search(Stream *s,const char *from,const char *id,xmlnode q){
struct gg_search_request sr;
xmlnode n;
struct gg_http *gghttp;
Request *r;

	q=xmlnode_dup(q);
	memset(&sr,0,sizeof(sr));
	n=xmlnode_get_tag(q,"nick");
	if (n) sr.nickname=from_utf8(xmlnode_get_data(n));
	else sr.nickname=NULL;
	n=xmlnode_get_tag(q,"first");
	if (n) sr.first_name=from_utf8(xmlnode_get_data(n));
	else sr.first_name=NULL;
	n=xmlnode_get_tag(q,"last");
	if (n) sr.last_name=from_utf8(xmlnode_get_data(n));
	else sr.last_name=NULL;
	n=xmlnode_get_tag(q,"city");
	if (n) sr.city=from_utf8(xmlnode_get_data(n));
	else sr.city=NULL;
	n=xmlnode_get_tag(q,"email");
	if (n) sr.email=from_utf8(xmlnode_get_data(n));
	else sr.email=NULL;
	n=xmlnode_get_tag(q,"phone");
	if (n) sr.phone=from_utf8(xmlnode_get_data(n));
	else sr.phone=NULL;
	n=xmlnode_get_tag(q,"uin");
	if (n) sr.uin=atoi(xmlnode_get_data(n));
	else sr.uin=0;

	gghttp=gg_search(&sr,1);
	if (!gghttp) jabber_iq_send_error(s,from,id,"Search error");

	r=add_request(RT_SEARCH,from,id,q,gghttp,s);
	if (!r) jabber_iq_send_error(s,from,id,"Search error");
}

void jabber_iq_get_user_vcard(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
struct gg_search_request sr;
struct gg_http *gghttp;
Request *r;
	
	q=xmlnode_dup(q);
	memset(&sr,0,sizeof(sr));
	sr.uin=jid_get_uin(to);

	gghttp=gg_search(&sr,1);
	if (!gghttp) jabber_iq_send_error(s,from,id,"vCard error");

	r=add_request(RT_VCARD,from,id,q,gghttp,s);
	if (!r) jabber_iq_send_error(s,from,id,"Search error");
}

int vcard_done(struct request_s *r){
xmlnode vc,n,n1;
struct gg_search * results;
char *jid,*name;
	
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
	
	n1=xmlnode_insert_tag(vc,"ADR");
	n=xmlnode_insert_tag(n1,"LOCALITY");
	xmlnode_insert_cdata(n,to_utf8(results->results[0].city),-1);
	
	jid=jid_build(results->results[0].uin);
	n=xmlnode_insert_tag(vc,"JABBERID");
	xmlnode_insert_cdata(n,jid,-1);
	g_free(jid);
	
	jabber_iq_send_result(r->stream,r->from,r->id,vc);
	return 0;
}

int vcard_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->id,"vCard error");
	return 0;
}
