#include "ggtrans.h"
#include "jabber.h"
#include "register.h"
#include "jid.h"
#include "search.h"
#include "iq.h"
#include "debug.h"

void jabber_iq_send_error(Stream *s,const char *from,const char *id,int code,char *string){
xmlnode iq;
xmlnode error;
char *str;

	if (from==NULL) from=my_name;
	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","error");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",from);
	xmlnode_put_attrib(iq,"from",my_name);
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

void jabber_iq_send_result(Stream *s,const char *from,const char *id,xmlnode content){
xmlnode iq;

	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","result");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",from);
	xmlnode_put_attrib(iq,"from",my_name);
	if (content) xmlnode_insert_tag_node(iq,content);
	stream_write(s,iq);
	xmlnode_free(iq);
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
		if (!g_strcasecmp(ns,"jabber:iq:register"))
			jabber_iq_get_register(s,from,id,q);
		else if (!g_strcasecmp(ns,"jabber:iq:search"))
			jabber_iq_get_search(s,from,id,q);
		else{
			g_warning("Unsupported xmlns=%s in server iq/get!",ns);
			jabber_iq_send_error(s,from,id,501,"Not Implemented");
		}
	}
	else{
		g_warning("Unsupported xmlns=%s in user iq/get!",ns);
		jabber_iq_send_error(s,from,id,501,"Not Implemented");
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
		if (!g_strcasecmp(ns,"jabber:iq:register"))
			jabber_iq_set_register(s,from,id,q);
		else if (!g_strcasecmp(ns,"jabber:iq:search"))
			jabber_iq_set_search(s,from,id,q);
		else{
			g_warning("Unknown xmlns=%s in server iq/set!",ns);
			jabber_iq_send_error(s,from,id,501,"Not Implemented");
		}
	}
	else{
		g_warning("Unsupported xmlns=%s in useriq/set!",ns);
		jabber_iq_send_error(s,from,id,501,"Not Implemented");
	}
}

void jabber_iq_get_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
char *ns;
xmlnode n;

	debug("vCard request");
	ns=xmlnode_get_attrib(q,"xmlns");
	if (ns && g_strcasecmp(ns,"vcard-temp")){
		g_warning("Unsupported vcard namespace: %s!",ns);
		jabber_iq_send_error(s,from,id,501,"Not Implemented");
		return;
	}
	if (jid_is_me(to)){
		n=xmlnode_get_tag(config,"vCard");
		if (!n){
			jabber_iq_send_error(s,from,id,503,"Service Unavailable");
			g_warning("No vcard for server defined");
			return;
		}
		jabber_iq_send_result(s,from,id,n);
	}
	else jabber_iq_get_user_vcard(s,from,to,id,q);
}

void jabber_iq_set_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
char *ns;

	ns=xmlnode_get_attrib(q,"xmlns");
	if (ns && g_strcasecmp(ns,"vcard-temp")){
		g_warning("Unsupported vcard namespace: %s!",ns);
		jabber_iq_send_error(s,from,id,501,"Not Implemented");
		return;
	}
	g_warning("vcard setting unsupported");
	jabber_iq_send_error(s,from,id,501,"Not Implemented");
}
	
void jabber_iq_get(Stream *s,xmlnode x){
char *to;
char *id;
char *from;
xmlnode query;

	to=xmlnode_get_attrib(x,"to");
	if (!to || !jid_is_my(to) ){
		g_warning("Wrong to=%s (my name is %s)",to?to:"(null)",my_name);
		jabber_iq_send_error(s,from,id,400,"Bad Request");
		return;
	}
	id=xmlnode_get_attrib(x,"id");
	from=xmlnode_get_attrib(x,"from");
	if (!from){
		g_warning("No from in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,id,400,"Bad Request");
		return;
	}
	query=xmlnode_get_tag(x,"query");
	if (query){
		jabber_iq_get_query(s,from,to,id,query);
		return;
	}
	query=xmlnode_get_tag(x,"vcard");
	if (!query) query=xmlnode_get_tag(x,"vCard");
	if (query){
		jabber_iq_get_vcard(s,from,to,id,query);
		return;
	}
	g_warning("No known content in iq: %s",xmlnode2str(x));
	jabber_iq_send_error(s,from,id,400,"Bad Request");
}

void jabber_iq_set(Stream *s,xmlnode x){
char *to;
char *id;
char *from;
xmlnode query;

	to=xmlnode_get_attrib(x,"to");
	if (!to || g_strcasecmp(to,my_name)){
		g_warning("Wrong 'to' (my name is %s) in query:  %s",to?to:"(null)",xmlnode2str(x));
		jabber_iq_send_error(s,from,id,400,"Bad Request");
		return;
	}
	id=xmlnode_get_attrib(x,"id");
	from=xmlnode_get_attrib(x,"from");
	if (!from){
		g_warning("No from in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,id,400,"Bad Request");
		return;
	}
	query=xmlnode_get_tag(x,"query");
	if (query){
		jabber_iq_set_query(s,from,to,id,query);
		return;
	}
	query=xmlnode_get_tag(x,"vcard");
	if (!query) query=xmlnode_get_tag(x,"vCard");
	if (query){
		jabber_iq_set_vcard(s,from,to,id,query);
		return;
	}
	g_warning("No known content in iq: %s",xmlnode2str(x));
	jabber_iq_send_error(s,from,id,400,"Bad Request");
}

void jabber_iq_result(Stream *s,xmlnode x){}
void jabber_iq_error(Stream *s,xmlnode x){}
	
void jabber_iq(Stream *s,xmlnode x){
char *type;

	if (jabber_state!=JS_CONNECTED){
		g_warning("unexpected <iq/> (not connected yet)");	
		return;
	}
	
	type=xmlnode_get_attrib(x,"type");
	if (g_strcasecmp(type,"get")==0)
		jabber_iq_get(s,x);
	else if (g_strcasecmp(type,"set")==0)
		jabber_iq_set(s,x);
	else if (g_strcasecmp(type,"result")==0)
		jabber_iq_result(s,x);
	else if (g_strcasecmp(type,"error")==0)
		jabber_iq_error(s,x);
	else g_warning("Unsupported <iq/> type in: %s",xmlnode2str(x));
}

