/* $Id: iq.c,v 1.21 2002/02/02 19:49:54 jajcus Exp $ */

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
#include "jid.h"
#include "search.h"
#include "iq.h"
#include "browse.h"
#include "debug.h"
#include "conf.h"

const char *gateway_desc;
const char *gateway_prompt;

void jabber_iq_get_agent(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_server_vcard(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_gateway(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_set_gateway(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
	
IqNamespace server_iq_ns[]={
	{"jabber:iq:register","query",jabber_iq_get_register,jabber_iq_set_register},
	{"jabber:iq:search","query",jabber_iq_get_search,jabber_iq_set_search},
	{"jabber:iq:agent","query",jabber_iq_get_agent,NULL},
	{"jabber:iq:gateway","query",jabber_iq_get_gateway,jabber_iq_set_gateway},
	{"jabber:iq:browse","item",jabber_iq_get_server_browse,NULL},
	{"jabber:iq:browse","query",jabber_iq_get_server_browse,NULL},/* WinJab bug (?) workaround */
	{"vcard-temp","vCard",jabber_iq_get_server_vcard,NULL},
	{"vcard-temp","VCARD",jabber_iq_get_server_vcard,NULL}, /* WinJab bug workaround */
	{NULL,NULL,NULL,NULL}
};

IqNamespace client_iq_ns[]={
	{"vcard-temp","vCard",jabber_iq_get_user_vcard,NULL},
	{"vcard-temp","VCARD",jabber_iq_get_user_vcard,NULL}, /* WinJab bug workaround */
	{"jabber:iq:browse","item",jabber_iq_get_client_browse,NULL},
	{"jabber:iq:browse","query",jabber_iq_get_client_browse,NULL},/* WinJab bug (?) workaround */
	{NULL,NULL,NULL,NULL}
};

void jabber_iq_send_error(Stream *s,const char *was_from,const char *was_to,const char *id,int code,char *string){
xmlnode iq;
xmlnode error;
char *str;

	if (was_from==NULL) was_from=my_name;
	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","error");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",was_from);
	if (was_to) xmlnode_put_attrib(iq,"from",was_to);
	else xmlnode_put_attrib(iq,"from",my_name);
	error=xmlnode_insert_tag(iq,"error");
	if (code>0) {
		str=g_strdup_printf("%03u",(unsigned)code);
		xmlnode_put_attrib(error,"code",str);
		g_free(str);
	}
	xmlnode_insert_cdata(error,string,-1);
	stream_write(s,iq);
	xmlnode_free(iq);
}

void jabber_iq_send_result(Stream *s,const char *was_from,const char *was_to,const char *id,xmlnode content){
xmlnode iq;

	if (was_from==NULL) was_from=my_name;
	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","result");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",was_from);
	if (was_to) xmlnode_put_attrib(iq,"from",was_to);
	else xmlnode_put_attrib(iq,"from",my_name);
	if (content) xmlnode_insert_tag_node(iq,content);
	stream_write(s,iq);
	xmlnode_free(iq);
}

void jabber_iq_get_agent(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode n;
xmlnode query;

	query=xmlnode_new_tag("query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:agent");
	n=xmlnode_get_tag(config,"vCard/FN");
	if (n) xmlnode_insert_cdata( xmlnode_insert_tag(query,"name"),
					xmlnode_get_data(n),-1);
	n=xmlnode_get_tag(config,"vCard/DESC");
	if (n) xmlnode_insert_cdata( xmlnode_insert_tag(query,"description"),
					xmlnode_get_data(n),-1);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"transport"),gateway_prompt,-1);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"service"),"x-gg",-1); /* until gg is registered */
	xmlnode_insert_tag(query,"register");
	xmlnode_insert_tag(query,"search");
		
	jabber_iq_send_result(s,from,to,id,query);
}

void jabber_iq_get_gateway(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode query;

	query=xmlnode_new_tag("query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:gateway");
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"desc"),gateway_desc,-1);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"prompt"),gateway_prompt,-1);
	jabber_iq_send_result(s,from,to,id,query);
}

void jabber_iq_set_gateway(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode n;
char *str;
int uin;
xmlnode query;

	n=xmlnode_get_tag(q,"prompt");
	if (!n) jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
	str=xmlnode_get_data(n);
	if (!str) jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
	uin=atoi(str);
	if (uin<=0) jabber_iq_send_error(s,from,to,id,406,"Not Acceptable");
	
	query=xmlnode_new_tag("query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:gateway");
	str=jid_build(uin);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"jid"),str,-1);
	g_free(str);
	jabber_iq_send_result(s,from,to,id,query);
}

void jabber_iq_get_query(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
char *ns;

	ns=xmlnode_get_attrib(q,"xmlns");
	if (!ns){
		g_warning("iq_get_query: No xmlns!");
		g_warning("Query: %s!",xmlnode2str(q));
		return;
	}
	if (jid_is_me(to)){
		if (!strcmp(ns,"jabber:iq:register"))
			jabber_iq_get_register(s,from,to,id,q);
		else if (!strcmp(ns,"jabber:iq:search"))
			jabber_iq_get_search(s,from,to,id,q);
		else if (!strcmp(ns,"jabber:iq:agent"))
			jabber_iq_get_agent(s,from,to,id,q);
		else{
			g_warning("Unsupported xmlns=%s in server iq/get!",ns);
			jabber_iq_send_error(s,from,to,id,501,"Not Implemented");
		}
	}
	else{
		g_warning("Unsupported xmlns=%s in user iq/get!",ns);
		jabber_iq_send_error(s,from,to,id,501,"Not Implemented");
	}
}

void jabber_iq_set_query(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
char *ns;

	ns=xmlnode_get_attrib(q,"xmlns");
	if (!ns){
		g_warning("iq_get_query: No xmlns!");
		g_warning("Query: %s!",xmlnode2str(q));
		return;
	}

	if (jid_is_me(to)){ 
		if (!strcmp(ns,"jabber:iq:register"))
			jabber_iq_set_register(s,from,to,id,q);
		else if (!strcmp(ns,"jabber:iq:search"))
			jabber_iq_set_search(s,from,to,id,q);
		else{
			g_warning("Unknown xmlns=%s in server iq/set!",ns);
			jabber_iq_send_error(s,from,to,id,501,"Not Implemented");
		}
	}
	else{
		g_warning("Unsupported xmlns=%s in useriq/set!",ns);
		jabber_iq_send_error(s,from,to,id,501,"Not Implemented");
	}
}

void jabber_iq_get_server_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
char *ns;
xmlnode n;

	n=xmlnode_get_tag(config,"vCard");
	if (!n){
		jabber_iq_send_error(s,from,to,id,503,"Service Unavailable");
		g_warning("No vcard for server defined");
		return;
	}
	jabber_iq_send_result(s,from,to,id,n);
}

static void jabber_iq_getset(Stream *s,xmlnode x,int set){
char *to;
char *id;
char *from;
char *ns,*name;
xmlnode query;
IqNamespace *table;
int i;

	to=xmlnode_get_attrib(x,"to");
	id=xmlnode_get_attrib(x,"id");
	from=xmlnode_get_attrib(x,"from");
	if (!to || !jid_is_my(to) ){
		g_warning("Wrong to=%s (my name is %s)",to?to:"(null)",my_name);
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
		return;
	}
	if (!from){
		g_warning("No from in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
		return;
	}

	for(query=xmlnode_get_firstchild(x);query;query=xmlnode_get_nextsibling(query))
		if (xmlnode_get_type(query)==NTYPE_TAG) break;

	if (!query){
		g_warning("No subelement in <iq type='get'> in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
		return;
	}
	name=xmlnode_get_name(query);
	ns=xmlnode_get_attrib(query,"xmlns");
	if (!ns){
		g_warning("No namespace defined for <iq/> subelement: %s",xmlnode2str(query));
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
		return;
	}

	if (jid_is_me(to)) table=server_iq_ns;
	else table=client_iq_ns;

	for(i=0;table[i].ns;i++)
		if (!strcmp(table[i].ns,ns) && !strcmp(table[i].node_name,name)){
			if (set){
				if (table[i].set_handler) table[i].set_handler(s,from,to,id,query);
				else {
					g_warning("No <iq type='set'/> implemented for %s",ns);
					jabber_iq_send_error(s,from,to,id,501,"Not implemented");
					return;
				}
			}
			else {
				if (table[i].get_handler) table[i].get_handler(s,from,to,id,query);
				else {
					g_warning("No <iq type='get'/> implemented for %s",ns);
					jabber_iq_send_error(s,from,to,id,501,"Not implemented");
					return;
				}
			}
			return;
		}
	
	g_warning("No known content in iq: %s",xmlnode2str(x));
	jabber_iq_send_error(s,from,to,id,501,"Not implemented");
}

void jabber_iq_result(Stream *s,xmlnode x){}

void jabber_iq_error(Stream *s,xmlnode x){

	g_warning("Error iq received: %s",xmlnode2str(x));
}
	
void jabber_iq(Stream *s,xmlnode x){
char *type;

	if (jabber_state!=JS_CONNECTED){
		g_warning("unexpected <iq/> (not connected yet)");	
		return;
	}
	
	type=xmlnode_get_attrib(x,"type");
	if (strcmp(type,"get")==0)
		jabber_iq_getset(s,x,0);
	else if (strcmp(type,"set")==0)
		jabber_iq_getset(s,x,1);
	else if (strcmp(type,"result")==0)
		jabber_iq_result(s,x);
	else if (strcmp(type,"error")==0)
		jabber_iq_error(s,x);
	else g_warning("Unsupported <iq/> type in: %s",xmlnode2str(x));
}

