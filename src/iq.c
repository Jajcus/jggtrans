#include "ggtrans.h"
#include "jabber.h"
#include "register.h"
#include "jid.h"
#include "search.h"
#include "iq.h"
#include "debug.h"

const char *gateway_desc;
const char *gateway_prompt;

void jabber_iq_send_error(Stream *s,const char *from,const char *to,const char *id,int code,char *string){
xmlnode iq;
xmlnode error;
char *str;

	if (from==NULL) from=my_name;
	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","error");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",from);
	if (to) xmlnode_put_attrib(iq,"from",to);
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

void jabber_iq_send_result(Stream *s,const char *from,const char *to,const char *id,xmlnode content){
xmlnode iq;

	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","result");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",from);
	if (to) xmlnode_put_attrib(iq,"from",to);
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
	xmlnode_insert_cdata(xmlnode_insert_tag(query,"service"),"gg",-1);
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

void jabber_iq_get_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
char *ns;
xmlnode n;

	debug("vCard request");
	ns=xmlnode_get_attrib(q,"xmlns");
	if (ns && strcmp(ns,"vcard-temp")){
		g_warning("Unsupported vcard namespace: %s!",ns);
		jabber_iq_send_error(s,from,to,id,501,"Not Implemented");
		return;
	}
	if (jid_is_me(to)){
		n=xmlnode_get_tag(config,"vCard");
		if (!n){
			jabber_iq_send_error(s,from,to,id,503,"Service Unavailable");
			g_warning("No vcard for server defined");
			return;
		}
		jabber_iq_send_result(s,from,to,id,n);
	}
	else jabber_iq_get_user_vcard(s,from,to,id,q);
}

void jabber_iq_set_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
char *ns;

	ns=xmlnode_get_attrib(q,"xmlns");
	if (ns && strcmp(ns,"vcard-temp")){
		g_warning("Unsupported vcard namespace: %s!",ns);
		jabber_iq_send_error(s,from,to,id,501,"Not Implemented");
		return;
	}
	g_warning("vcard setting unsupported");
	jabber_iq_send_error(s,from,to,id,501,"Not Implemented");
}
	
void jabber_iq_get(Stream *s,xmlnode x){
char *to;
char *id;
char *from;
xmlnode query;

	to=xmlnode_get_attrib(x,"to");
	if (!to || !jid_is_my(to) ){
		g_warning("Wrong to=%s (my name is %s)",to?to:"(null)",my_name);
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
		return;
	}
	id=xmlnode_get_attrib(x,"id");
	from=xmlnode_get_attrib(x,"from");
	if (!from){
		g_warning("No from in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
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
	jabber_iq_send_error(s,from,to,id,400,"Bad Request");
}

void jabber_iq_set(Stream *s,xmlnode x){
char *to;
char *id;
char *from;
xmlnode query;

	to=xmlnode_get_attrib(x,"to");
	if (!to || strcmp(to,my_name)){
		g_warning("Wrong to=%s (my name is %s) in query:  %s",to?to:"(null)",my_name,xmlnode2str(x));
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
		return;
	}
	id=xmlnode_get_attrib(x,"id");
	from=xmlnode_get_attrib(x,"from");
	if (!from){
		g_warning("No from in query: %s",xmlnode2str(x));
		jabber_iq_send_error(s,from,to,id,400,"Bad Request");
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
	jabber_iq_send_error(s,from,to,id,400,"Bad Request");
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
	if (strcmp(type,"get")==0)
		jabber_iq_get(s,x);
	else if (strcmp(type,"set")==0)
		jabber_iq_set(s,x);
	else if (strcmp(type,"result")==0)
		jabber_iq_result(s,x);
	else if (strcmp(type,"error")==0)
		jabber_iq_error(s,x);
	else g_warning("Unsupported <iq/> type in: %s",xmlnode2str(x));
}

