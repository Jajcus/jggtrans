#include "ggtrans.h"
#include "jabber.h"
#include "iq.h"
#include "browse.h"
#include "debug.h"
#include "conf.h"

void jabber_iq_get_server_browse(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
xmlnode result;
xmlnode n;
char *str;
int i;

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
			/* workaround for the WinJab bug workaround :-) */
			continue;
		}
		n=xmlnode_insert_tag(result,"ns");
		xmlnode_insert_cdata(n,server_iq_ns[i].ns,-1);
	}
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

