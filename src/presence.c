#include <stdio.h>
#include "ggtrans.h"
#include "presence.h"
#include "jabber.h"
#include "jid.h"
#include "users.h"
#include "sessions.h"
#include "debug.h"
#include "register.h"


int presence_send_probe(struct stream_s *stream,const char *to){
xmlnode pres;
char *jid;

	pres=xmlnode_new_tag("presence");
	jid=jid_my_registered();
	xmlnode_put_attrib(pres,"from",jid);
	g_free(jid);
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","probe");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_subscribe(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL) 
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","subscribe");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_subscribed(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL) 
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","subscribed");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_unsubscribed(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL) 
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","unsubscribed");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send_unsubscribe(struct stream_s *stream,const char *from,const char *to){
xmlnode pres;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL) 
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","unsubscribe");
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence_send(struct stream_s *stream,const char *from,
		const char *to,int available,const char *show,
		const char *status){
xmlnode pres;
xmlnode n;

	pres=xmlnode_new_tag("presence");
	if (from!=NULL) 
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(pres,"to",to);
	if (!available) xmlnode_put_attrib(pres,"type","unavailable");
	if (show){
		n=xmlnode_insert_tag(pres,"show");
		xmlnode_insert_cdata(n,show,-1);
	}
	if (status){
		n=xmlnode_insert_tag(pres,"status");
		xmlnode_insert_cdata(n,status,-1);
	}
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence(struct stream_s *stream,const char *from,const char *to,
		int available,const char *show,const char *status){
Session *s;

	s=session_get_by_jid(from,available?stream:NULL);
	if (!s){
		debug("presence: No such session: %s",from);
		presence_send(stream,NULL,from,0,NULL,"Not logged in");
		return -1;
	}
	return session_set_status(s,available,show,status);
}

int presence_subscribe(struct stream_s *stream,const char *from,const char *to){
User *u;
Session *s;
int r;
	
	u=user_get_by_jid(from);
	if (jid_is_me(to)){
		debug("Presence subscribe request sent to me");
		if (!u) presence_send_unsubscribed(stream,to,from);
		else presence_send_subscribed(stream,to,from);
		return 0;
	}
	if (!u)	{
		g_warning("Presence subscription from unknown user (%s)",from);
		return -1;
	}
	if (!jid_has_uin(to) || !jid_is_my(to)){
		g_warning("Bad 'to': %s",to);
		return -1;
	}
	s=session_get_by_jid(from,stream);
	if (!s){
		g_warning("Couldn't find or open session for '%s'",from);
		return -1;
	}
	debug("Subscribing %s to %s...",from,to);
	r=session_subscribe(s,jid_get_uin(to));
	if (!r){
		debug("Subscribed.");
		presence_send_subscribed(stream,to,from);
	}
	else presence_send_subscribed(stream,to,from);
	
	return 0;
}

int presence_subscribed(struct stream_s *stream,const char *from,const char *to){

	return 0;
}

int presence_unsubscribed(struct stream_s *stream,const char *from,const char *to){
	
	if (!jid_is_me(to)) return 0;

	debug("Presence unsubscribed sent to me.");

	unregister(stream,from,NULL,1);

	return 0;
}

int presence_unsubscribe(struct stream_s *stream,const char *from,const char *to){
User *u;
Session *s;
int r;
	
	if (jid_is_me(to)){
		debug("Presence unsubscribe request sent to me");
		presence_send_unsubscribed(stream,to,from);
		return 0;
	}
	u=user_get_by_jid(from);
	if (!u)	{
		g_warning("Presence subscription from unknown user (%s)",from);
		return -1;
	}
	if (!jid_has_uin(to) || !jid_is_my(to)){
		g_warning("Bad 'to': %s",to);
		return -1;
	}
	s=session_get_by_jid(from,stream);
	if (!s){
		g_warning("Couldn't find or open session for '%s'",from);
		return -1;
	}
	debug("Unsubscribing %s to %s...",from,to);
	r=session_unsubscribe(s,jid_get_uin(to));
	if (!r){
		debug("Unsubscribed.");
		presence_send_unsubscribed(stream,to,from);
	}
	
	return 0;
}

int jabber_presence(struct stream_s *stream,xmlnode tag){
char *type;
char *from;
char *to;
xmlnode show_n;
xmlnode status_n;
char *show,*status;

	type=xmlnode_get_attrib(tag,"type");
	from=xmlnode_get_attrib(tag,"from");
	to=xmlnode_get_attrib(tag,"to");

	show_n=xmlnode_get_tag(tag,"show");
	if (show_n) show=xmlnode_get_data(show_n);
	else show=NULL;
	
	status_n=xmlnode_get_tag(tag,"status");
	if (status_n) show=xmlnode_get_data(status_n);
	else status=NULL;

	if (!type) type="available";
	
	if (!from || !to){
		g_warning("Bad <presence/>: %s",xmlnode2str(tag));
		return -1;
	}

	if (!jid_is_my(to)){
		g_warning("Wrong 'to' in %s",xmlnode2str(tag));
		return -1;
	}
	
	if (!g_strcasecmp(type,"available"))
		return presence(stream,from,to,1,show,status);
	else if (!g_strcasecmp(type,"unavailable"))
		return presence(stream,from,to,0,show,status);
	else if (!g_strcasecmp(type,"subscribe"))
		return presence_subscribe(stream,from,to);
	else if (!g_strcasecmp(type,"unsubscribe"))
		return presence_unsubscribe(stream,from,to);
	else if (!g_strcasecmp(type,"subscribed"))
		return presence_subscribed(stream,from,to);
	else if (!g_strcasecmp(type,"unsubscribed"))
		return presence_unsubscribed(stream,from,to);
	
	g_warning("Unsupported type in %s",xmlnode2str(tag));
	return -1;
}

