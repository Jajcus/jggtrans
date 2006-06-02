/* $Id: disco.c,v 1.13 2004/04/13 17:44:07 jajcus Exp $ */

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
#include "jabber.h"
#include "iq.h"
#include "browse.h"
#include "conf.h"
#include "jid.h"
#include "sessions.h"
#include "debug.h"

static const char *extra_features[]={NULL};

static void disco_session(gpointer key,gpointer value,gpointer data){
const char *jid=(char *)key;
const Session *sess=(Session *)value;
xmlnode result=(xmlnode)data;
xmlnode n;
char *str;

	n=xmlnode_insert_tag(result,"item");
	xmlnode_put_attrib(n,"jid",jid);
	str=session_get_info_string(sess);
	xmlnode_put_attrib(n,"name",str);
	g_free(str);
}

static void disco_online_users(Stream *s,const char *from,const char * to, const char *id,xmlnode q){
xmlnode result;
char *jid;

	jid=jid_normalized(from,0);
	if (jid==NULL){
		debug(N_("Bad 'from' address"));
		return;
	}

	if (g_list_find_custom(admins,jid,(GCompareFunc)strcmp)==NULL){
		g_free(jid);
		jabber_iq_send_error(s,from,to,id,405,_("You are not allowed to browse users"));
		return;
	}
	g_free(jid);

	result=xmlnode_new_tag("query");
	xmlnode_put_attrib(result,"xmlns","http://jabber.org/protocol/disco#items");
	xmlnode_put_attrib(result,"node","online_users");

	g_hash_table_foreach(sessions_jid,disco_session,result);

	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

void jabber_iq_get_server_disco_items(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n;
char *jid,*node;

	jid=jid_normalized(from,0);
	if (jid==NULL){
		debug(N_("Bad 'from' address"));
		return;
	}

	node=xmlnode_get_attrib(q,"node");
	if (node && node[0]){
		if (!strcmp(node,"online_users"))
			disco_online_users(s,from,to,id,q);
		else jabber_iq_send_error(s,from,to,id,404,_("Disco request for unknown node"));
		return;
	}
	result=xmlnode_new_tag("query");
	xmlnode_put_attrib(result,"xmlns","http://jabber.org/protocol/disco#items");
	if (g_list_find_custom(admins,jid,(GCompareFunc)strcmp)!=NULL){
		n=xmlnode_insert_tag(result,"item");
		xmlnode_put_attrib(n,"jid",my_name);
		xmlnode_put_attrib(n,"node","online_users");
		xmlnode_put_attrib(n,"name",_("Online users"));
	}
	else debug("%s not admin",jid);
	g_free(jid);
	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

void jabber_iq_get_server_disco_info(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n,n1;
char *str,*node;
int i;

	node=xmlnode_get_attrib(q,"node");
	if (node && node[0]){
		if (!strcmp(node,"online_users")){
			result=xmlnode_new_tag("query");
			xmlnode_put_attrib(result,"xmlns","http://jabber.org/protocol/disco#info");
			xmlnode_put_attrib(result,"node","online_users");
			n=xmlnode_insert_tag(result,"identity");
			xmlnode_put_attrib(n,"category","hierarchy");
			xmlnode_put_attrib(n,"type","branch");
			xmlnode_put_attrib(n,"name",_("Online users"));
			jabber_iq_send_result(s,from,to,id,result);
			xmlnode_free(result);
		}
		jabber_iq_send_error(s,from,to,id,404,_("Disco request for unknown node"));
		return;
	}
	result=xmlnode_new_tag("query");
	xmlnode_put_attrib(result,"xmlns","http://jabber.org/protocol/disco#info");
	n=xmlnode_insert_tag(result,"identity");
	xmlnode_put_attrib(n,"category","gateway");
	xmlnode_put_attrib(n,"type","gadu-gadu");
	n1=xmlnode_get_tag(config,"vCard/FN");
	if (n1){
		str=xmlnode_get_data(n1);
		if (str) xmlnode_put_attrib(n,"name",str);
	}

	for(i=0;server_iq_ns[i].ns;i++){
		if (i && !strcmp(server_iq_ns[i-1].ns,server_iq_ns[i].ns)){
			/* workaround for the WinJab bug :-) */
			continue;
		}
		n=xmlnode_insert_tag(result,"feature");
		xmlnode_put_attrib(n,"var",server_iq_ns[i].ns);
	}
	for(i=0;extra_features[i];i++){
		n=xmlnode_insert_tag(result,"feature");
		xmlnode_put_attrib(n,"var",extra_features[i]);
	}

	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

void jabber_iq_get_client_disco_items(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
char *node;

	node=xmlnode_get_attrib(q,"node");
	if (node && node[0]){
		jabber_iq_send_error(s,from,to,id,404,_("Disco request for unknown node"));
		return;
	}

	result=xmlnode_new_tag("query");
	xmlnode_put_attrib(result,"xmlns","http://jabber.org/protocol/disco#items");
	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

void jabber_iq_get_client_disco_info(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n;
char *str, *node;
int i;

	node=xmlnode_get_attrib(q,"node");
	if (node && node[0]){
		jabber_iq_send_error(s,from,to,id,404,_("Disco request for unknown node"));
		return;
	}

	result=xmlnode_new_tag("query");
	xmlnode_put_attrib(result,"xmlns","http://jabber.org/protocol/disco#info");
	n=xmlnode_insert_tag(result,"identity");
	xmlnode_put_attrib(n,"category","client");
	xmlnode_put_attrib(n,"type","pc");
	str=g_strdup_printf("GG user #%u",jid_get_uin(to));
	xmlnode_put_attrib(n,"name",str);
	g_free(str);

	for(i=0;client_iq_ns[i].ns;i++){
		if (i && !strcmp(client_iq_ns[i-1].ns,client_iq_ns[i].ns)){
			/* workaround for the WinJab bug workaround :-) */
			continue;
		}
		n=xmlnode_insert_tag(result,"feature");
		xmlnode_put_attrib(n,"var",client_iq_ns[i].ns);
	}
	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

