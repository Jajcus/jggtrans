#include "ggtrans.h"
#include "jabber.h"
#include "iq.h"
#include "browse.h"
#include "debug.h"
#include "conf.h"
#include "jid.h"
#include "sessions.h"


void browse_session(gpointer key,gpointer value,gpointer data){
const char *jid=(char *)key;
const Session *sess=(Session *)value;
xmlnode result=(xmlnode)data;
xmlnode n;
char *str;

	n=xmlnode_insert_tag(result,"user");
	xmlnode_put_attrib(n,"type","client");
	xmlnode_put_attrib(n,"jid",jid);

	str=g_strdup_printf("%s (%s)",jid,sess->connected?_("Connected"):_("Disconnected"));
	xmlnode_put_attrib(n,"name",str);
	g_free(str);
}

void browse_admin(Stream *s,const char *from,const char * to, const char *id,xmlnode q){
xmlnode result;
char *jid;
	
	jid=jid_normalized(from);
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
}

void jabber_iq_get_server_browse(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n;
char *str,*jid,*resource;
int i;

	resource=strchr(to,'/');
	if (resource) {
		resource++;
		if (!strcmp(resource,"admin"))
			browse_admin(s,from,to,id,q);
		else jabber_iq_send_error(s,from,to,id,404,_("Browse request for unknown resource"));
		return;
	}
	result=xmlnode_new_tag("service");
	xmlnode_put_attrib(result,"xmlns","jabber:iq:browse");
	xmlnode_put_attrib(result,"type","x-gadugadu");
	xmlnode_put_attrib(result,"jid",my_name);
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
	jid=jid_normalized(from);
	if (g_list_find_custom(admins,jid,(GCompareFunc)strcmp)){
		n=xmlnode_insert_tag(result,"item");
		str=g_strdup_printf("%s/admin",my_name);
		xmlnode_put_attrib(n,"jid",str);
		xmlnode_put_attrib(n,"name",_("Online users"));
	}
	g_free(jid);
	jabber_iq_send_result(s,from,to,id,result);
}

void jabber_iq_get_client_browse(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n;
int i;

	result=xmlnode_new_tag("user");
	xmlnode_put_attrib(result,"xmlns","jabber:iq:browse");
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
}

