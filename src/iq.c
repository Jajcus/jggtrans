#include <stdio.h>
#include "jabber.h"
#include "register.h"
#include "jid.h"
#include "search.h"
#include "ggtrans.h"
#include "iq.h"
#include <glib.h>

void jabber_iq_send_error(Stream *s,const char *from,const char *id,char *string){
xmlnode iq;
xmlnode error;

	if (from==NULL) from=my_name;
	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","error");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",from);
	xmlnode_put_attrib(iq,"from",my_name);
	error=xmlnode_insert_tag(iq,"error");
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
		fprintf(stderr,"\nNo xmlns!\n");
		return;
	}
	if (jid_is_me(to)){
		if (!g_strcasecmp(ns,"jabber:iq:register"))
			jabber_iq_get_register(s,from,id,q);
		if (!g_strcasecmp(ns,"jabber:iq:search"))
			jabber_iq_get_search(s,from,id,q);
		else
			fprintf(stderr,"\nUnsupported xmlns=%s in server iq/get!\n",ns);
	}
	else {
		fprintf(stderr,"\nUnsupported xmlns=%s in user iq/get!\n",ns);
	}
}

void jabber_iq_set_query(Stream *s,const char *from,const char * to,const char *id,xmlnode q){
char *ns;

	ns=xmlnode_get_attrib(q,"xmlns");
	if (!ns){
		fprintf(stderr,"\nNo xmlns!\n");
		return;
	}

	if (jid_is_me(to)){ 
		if (!g_strcasecmp(ns,"jabber:iq:register"))
			jabber_iq_set_register(s,from,id,q);
		if (!g_strcasecmp(ns,"jabber:iq:search"))
			jabber_iq_set_search(s,from,id,q);
		else
			fprintf(stderr,"\nUnknown xmlns=%s in server iq/set!\n",ns);
	}
	else fprintf(stderr,"\nUnsupported xmlns=%s in useriq/set!\n",ns);
}

void jabber_iq_get_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
char *ns;
xmlnode n;

	fprintf(stderr,"vCard request\n");
	ns=xmlnode_get_attrib(q,"xmlns");
	if (ns && g_strcasecmp(ns,"vcard-temp")){
		fprintf(stderr,"\nUnsupported vcard namespace: %s!\n",ns);
		return;
	}
	if (jid_is_me(to)){
		n=xmlnode_get_tag(config,"vcard");
		if (!n){
			fprintf(stderr,"\nNo vcard for server defined\n");
			return;
		}
		jabber_iq_send_result(s,from,id,n);
	}
	jabber_iq_get_user_vcard(s,from,to,id,q);
}

void jabber_iq_set_vcard(Stream *s,const char *from,const char *to,const char *id,xmlnode q){
char *ns;

	ns=xmlnode_get_attrib(q,"xmlns");
	if (ns && g_strcasecmp(ns,"vcard-temp")){
		fprintf(stderr,"\nUnsupported vcard namespace: %s!\n",ns);
		return;
	}
	fprintf(stderr,"\nvcard setting unsupported\n");
}
	
void jabber_iq_get(Stream *s,xmlnode x){
char *to;
char *id;
char *from;
xmlnode query;

	to=xmlnode_get_attrib(x,"to");
	if (!to || !jid_is_my(to) ){
		fprintf(stderr,"\nWrong to=%s (my name is %s)\n",to?to:"(null)",my_name);
		return;
	}
	id=xmlnode_get_attrib(x,"id");
	if (!id){
		fprintf(stderr,"\nNo id\n");
		return;
	}
	from=xmlnode_get_attrib(x,"from");
	if (!from){
		fprintf(stderr,"\nNo from\n");
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
	fprintf(stderr,"No known content in iq\n");
}

void jabber_iq_set(Stream *s,xmlnode x){
char *to;
char *id;
char *from;
xmlnode query;

	to=xmlnode_get_attrib(x,"to");
	if (!to || g_strcasecmp(to,my_name)){
		fprintf(stderr,"\nWrong to=%s (my name is %s)\n",to?to:"(null)",my_name);
		return;
	}
	id=xmlnode_get_attrib(x,"id");
	if (!id){
		fprintf(stderr,"\nNo id\n");
		return;
	}
	from=xmlnode_get_attrib(x,"from");
	if (!from){
		fprintf(stderr,"\nNo from\n");
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
	fprintf(stderr,"No known content in iq\n");
}


void jabber_iq_result(Stream *s,xmlnode x){}
void jabber_iq_error(Stream *s,xmlnode x){}
	
void jabber_iq(Stream *s,xmlnode x){
char *type;

	if (jabber_state!=JS_CONNECTED){
		fprintf(stderr,"\nunexpected <iq/>\n");	
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
	else fprintf(stderr,"\nUnsupported <iq/> type: '%s'\n",type);
}

