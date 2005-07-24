/* $Id: browse.c,v 1.18 2004/04/13 17:44:07 jajcus Exp $ */

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
#include "iq.h"
#include "browse.h"
#include "conf.h"
#include "jid.h"
#include "sessions.h"
#include "debug.h"

void browse_session(gpointer key,gpointer value,gpointer data){
const char *jid=(char *)key;
const Session *sess=(Session *)value;
xmlnode result=(xmlnode)data;
xmlnode n;
char *str;

	n=xmlnode_insert_tag(result,"item");
	xmlnode_put_attrib(n,"category","user");
	xmlnode_put_attrib(n,"type","client");
	xmlnode_put_attrib(n,"jid",jid);
	str=session_get_info_string(sess);
	xmlnode_put_attrib(n,"name",str);
	g_free(str);
}

void browse_admin(Stream *s,const char *from,const char * to, const char *id,xmlnode q){
xmlnode result;
char *jid;

	jid=jid_normalized(from,0);
	if (jid==NULL){
		debug(L_("Bad 'from' address"));
		return;
	}
	if (g_list_find_custom(admins,jid,(GCompareFunc)strcmp)==NULL){
		g_free(jid);
		jabber_iq_send_error(s,from,to,id,405,_("You are not allowed to browse users"));
		return;
	}
	g_free(jid);

	result=xmlnode_new_tag("item");
	xmlnode_put_attrib(result,"xmlns","jabber:iq:browse");
	xmlnode_put_attrib(result,"jid",to);
	xmlnode_put_attrib(result,"name",_("Online users"));

	g_hash_table_foreach(sessions_jid,browse_session,result);

	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

void jabber_iq_get_server_browse(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n;
char *str,*jid,*resource;
int i;

	jid=jid_normalized(from,0);
	if (jid==NULL){
		debug(L_("Bad 'from' address"));
		return;
	}
	resource=strchr(to,'/');
	if (resource){
		resource++;
		if (!strcmp(resource,"admin"))
			browse_admin(s,from,to,id,q);
		else jabber_iq_send_error(s,from,to,id,404,_("Browse request for unknown resource"));
		return;
	}
	result=xmlnode_new_tag("item");
	xmlnode_put_attrib(result,"xmlns","jabber:iq:browse");
	xmlnode_put_attrib(result,"category","service");
	xmlnode_put_attrib(result,"type","x-gadugadu");
	xmlnode_put_attrib(result,"jid",my_name);
	xmlnode_put_attrib(result,"version",VERSION);
	n=xmlnode_get_tag(config,"vCard/FN");
	if (n){
		str=xmlnode_get_data(n);
		if (str) xmlnode_put_attrib(result,"name",str);
	}

	for(i=0;server_iq_ns[i].ns;i++){
		if (i && !strcmp(server_iq_ns[i-1].ns,server_iq_ns[i].ns)){
			/* workaround for the WinJab bug :-) */
			continue;
		}
		n=xmlnode_insert_tag(result,"ns");
		xmlnode_insert_cdata(n,server_iq_ns[i].ns,-1);
	}
	if (g_list_find_custom(admins,jid,(GCompareFunc)strcmp)){
		n=xmlnode_insert_tag(result,"item");
		str=g_strdup_printf("%s/admin",my_name);
		xmlnode_put_attrib(n,"jid",str);
		xmlnode_put_attrib(n,"name",_("Online users"));
	}
	g_free(jid);
	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

void jabber_iq_get_client_browse(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n;
int i;

	result=xmlnode_new_tag("item");
	xmlnode_put_attrib(result,"xmlns","jabber:iq:browse");
	xmlnode_put_attrib(result,"category","user");
	xmlnode_put_attrib(result,"type","client");
	xmlnode_put_attrib(result,"jid",to);
	for(i=0;client_iq_ns[i].ns;i++){
		if (i && !strcmp(client_iq_ns[i-1].ns,client_iq_ns[i].ns)){
			/* workaround for the WinJab bug workaround :-) */
			continue;
		}
		n=xmlnode_insert_tag(result,"ns");
		xmlnode_insert_cdata(n,client_iq_ns[i].ns,-1);
	}
	jabber_iq_send_result(s,from,to,id,result);
	xmlnode_free(result);
}

