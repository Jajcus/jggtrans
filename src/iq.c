/* $Id: iq.c,v 1.37 2003/04/06 10:50:58 mmazur Exp $ */

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
#include "users.h"
#include "search.h"
#include "iq.h"
#include "browse.h"
#include "debug.h"
#include "conf.h"
#include "gg_versions.h"
#include <sys/utsname.h>

char *gateway_desc;
char *gateway_prompt;

void jabber_iq_get_agent(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_server_vcard(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_gateway(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_set_gateway(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_server_version(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_client_version(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_not_implemented(Stream *s,const char *from,const char * to,const char *id,xmlnode q);

IqNamespace server_iq_ns[]={
	{"jabber:iq:register","query",jabber_iq_get_register,jabber_iq_set_register},
	{"jabber:iq:search","query",jabber_iq_get_search,jabber_iq_set_search},
	{"jabber:iq:agent","query",jabber_iq_get_agent,NULL},
	{"jabber:iq:gateway","query",jabber_iq_get_gateway,jabber_iq_set_gateway},
	{"jabber:iq:browse","item",jabber_iq_get_server_browse,NULL},
	{"jabber:iq:browse","query",jabber_iq_get_server_browse,NULL},/* WinJab bug (?) workaround */
	{"jabber:iq:version","query",jabber_iq_get_server_version,NULL},
	{"vcard-temp","vCard",jabber_iq_get_server_vcard,NULL},
	{"vcard-temp","VCARD",jabber_iq_get_server_vcard,NULL}, /* WinJab bug workaround */
	{NULL,NULL,NULL,NULL}
};

IqNamespace client_iq_ns[]={
	{"vcard-temp","vCard",jabber_iq_get_user_vcard,NULL},
	{"vcard-temp","VCARD",jabber_iq_get_user_vcard,NULL}, /* WinJab bug workaround */
	{"jabber:iq:browse","item",jabber_iq_get_client_browse,NULL},
	{"jabber:iq:browse","query",jabber_iq_get_client_browse,NULL},/* WinJab bug (?) workaround */
	{"jabber:iq:version","query",jabber_iq_get_client_version,NULL},
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
	if (code>0){
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
char *data;

	query=xmlnode_new_tag("query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:agent");
	n=xmlnode_get_tag(config,"vCard/FN");
	if (n!=NULL){
		data=xmlnode_get_data(n);
		if (data!=NULL)
			xmlnode_insert_cdata( xmlnode_insert_tag(query,"name"),data,-1);
	}
	n=xmlnode_get_tag(config,"vCard/DESC");
	if (n!=NULL){
		data=xmlnode_get_data(n);
		if (data!=NULL)
			xmlnode_insert_cdata(xmlnode_insert_tag(query,"description"),data,-1);
	}
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"transport"),gateway_prompt,-1);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"service"),"x-gadugadu",-1); /* until gg is registered */
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

void jabber_iq_get_server_version(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode query;
xmlnode os;
struct utsname un;

	query=xmlnode_new_tag("query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:version");
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"name"),"Gadu-Gadu Transport",-1);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"version"),VERSION,-1);
	uname(&un);
	os = xmlnode_insert_tag(query,"os");
	xmlnode_insert_cdata(os,un.sysname,-1);
	xmlnode_insert_cdata(os," ",1);
	xmlnode_insert_cdata(os,un.release,-1);
	jabber_iq_send_result(s,from,to,id,query);
}

void jabber_iq_get_client_version(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode query;
int uin;
User *u;
GList *it;
Contact *c;
char verstring[20] = _("- unknown -"); /* let it be a bit longer */
char *ver;
int version;

	query=xmlnode_new_tag("query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:version");
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"name"),"Gadu-Gadu(tm)",-1);
	if (!jid_has_uin(to)){
		g_warning("No UIN given in 'to': %s",to);
		jabber_iq_send_error(s,from,to,id,400,_("Bad Request"));
		return;
	}
	uin=jid_get_uin(to);
	u=user_get_by_jid(from);
	ver=verstring;
	g_assert(u!=NULL);
	for(it=u->contacts;it;it=it->next){
		c=(Contact *)it->data;
		version=c->version & 0xff;
		if (c->uin==uin && version){
			if (version < GG_VERSION_ELEMENTS   /* < elements in gg_version[] */
			   && gg_version[version]){
				ver=gg_version[version];
			} else{
				sprintf(verstring, "(prot.0x%02X)", version);
			}
		}
	}
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"version"),ver,-1);
	jabber_iq_send_result(s,from,to,id,query);
}

void jabber_iq_set_gateway(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode n;
char *str;
int uin;
xmlnode query;

	n=xmlnode_get_tag(q,"prompt");
	if (n==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("Not Acceptable"));
		return;
	}
	str=xmlnode_get_data(n);
	if (str==NULL){
		jabber_iq_send_error(s,from,to,id,406,_("Not Acceptable"));
		return;
	}
	uin=atoi(str);
	if (uin<=0){
		jabber_iq_send_error(s,from,to,id,406,_("Not Acceptable"));
		return;
	}

	query=xmlnode_new_tag("query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:gateway");
	str=jid_build(uin);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"prompt"),str,-1);
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"jid"),str,-1);
	g_free(str);
	jabber_iq_send_result(s,from,to,id,query);
}

void jabber_iq_get_server_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
xmlnode n;

	n=xmlnode_get_tag(config,"vCard");
	if (!n){
		jabber_iq_send_error(s,from,to,id,503,_("Service Unavailable"));
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
		jabber_iq_send_error(s,from,to,id,400,_("Bad Request"));
		return;
	}
	if (!from){
		g_warning("No from in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,to,id,400,_("Bad Request"));
		return;
	}

	for(query=xmlnode_get_firstchild(x);query;query=xmlnode_get_nextsibling(query))
		if (xmlnode_get_type(query)==NTYPE_TAG) break;

	if (!query){
		g_warning("No subelement in <iq type='get'> in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,to,id,400,_("Bad Request"));
		return;
	}
	name=xmlnode_get_name(query);
	ns=xmlnode_get_attrib(query,"xmlns");
	if (!ns){
		g_warning("No namespace defined for <iq/> subelement: %s",xmlnode2str(query));
		jabber_iq_send_error(s,from,to,id,400,_("Bad Request"));
		return;
	}

	if (jid_is_me(to)) table=server_iq_ns;
	else table=client_iq_ns;

	for(i=0;table[i].ns;i++)
		if (!strcmp(table[i].ns,ns) && !strcmp(table[i].node_name,name)){
			if (set){
				if (table[i].set_handler) table[i].set_handler(s,from,to,id,query);
				else{
					g_warning("No <iq type='set'/> implemented for %s",ns);
					jabber_iq_send_error(s,from,to,id,501,_("Not implemented"));
					return;
				}
			}
			else{
				if (table[i].get_handler) table[i].get_handler(s,from,to,id,query);
				else{
					g_warning("No <iq type='get'/> implemented for %s",ns);
					jabber_iq_send_error(s,from,to,id,501,_("Not implemented"));
					return;
				}
			}
			return;
		}

	g_warning("No known content in iq: %s",xmlnode2str(x));
	jabber_iq_send_error(s,from,to,id,501,_("Not implemented"));
}

void jabber_iq_not_implemented(Stream *s,const char *from,const char * to,const char *id,xmlnode q){

	jabber_iq_send_error(s,from,to,id,501,_("Not implemented"));
}


void jabber_iq_result(Stream *s,xmlnode x){}

void jabber_iq_error(Stream *s,xmlnode x){

	g_warning("Error iq received: %s",xmlnode2str(x));
}

void jabber_iq(Stream *s,xmlnode x){
char *type;
char *from;
User *u;

	if (jabber_state!=JS_CONNECTED){
		g_warning("unexpected <iq/> (not connected yet)");
		return;
	}

	from=xmlnode_get_attrib(x,"from");
	if (from) u=user_get_by_jid(from);
	else u=NULL;
	user_load_locale(u);

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

