#include <stdio.h>
#include "ggtrans.h"
#include "presence.h"
#include "jabber.h"
#include "jid.h"
#include "users.h"
#include "sessions.h"
#include "debug.h"
#include "register.h"
#include <time.h>

int presence_send_error(struct stream_s *stream,const char *from,const char *to,
				int code,const char *string){
xmlnode pres;
xmlnode error;
char *jid;
char *str;

	pres=xmlnode_new_tag("presence");
	jid=jid_my_registered();
	if (from!=NULL) 
		xmlnode_put_attrib(pres,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(pres,"from",jid);
		g_free(jid);
	}
	g_free(jid);
	xmlnode_put_attrib(pres,"to",to);
	xmlnode_put_attrib(pres,"type","error");
	error=xmlnode_insert_tag(pres,"error");
	if (code>0) {
		str=g_strdup_printf("%03u",(unsigned)code);
		xmlnode_put_attrib(error,"code",str);
		g_free(str);
	}
	xmlnode_insert_cdata(error,string,-1);

	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}


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
		const char *status,GTime timestamp){
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
	if (timestamp){
		struct tm *t;
		char str[21];
		
		t=localtime((time_t *)&timestamp);
		strftime(str,20,"%Y%m%dT%T",t);	
		n=xmlnode_insert_tag(pres,"x");
		xmlnode_put_attrib(n,"xmlns","jabber:x:delay");
		xmlnode_put_attrib(n,"stamp",str);
	}
	stream_write(stream,pres);
	xmlnode_free(pres);
	return 0;
}

int presence(struct stream_s *stream,const char *from,const char *to,
		int available,const char *show,const char *status){
Session *s;
int r;
char *jid;

	s=session_get_by_jid(from,available?stream:NULL);
	if (!s){
		debug("presence: No such session: %s",from);
		presence_send_error(stream,NULL,from,407,"Not logged in");
		return -1;
	}
	jid=g_strdup(s->user->jid); /* session may be removed in the next step */
	r=session_set_status(s,available,show,status);
	if (available && s->connected && !r) presence_send(stream,NULL,jid,available,show,status,0);
	g_free(jid);
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

	unregister(stream,from,NULL,NULL,1);

	return 0;
}

int presence_probe(struct stream_s *stream,const char *from,const char *to){
Session *s;
User *u;
uin_t uin;
int status;
int available;
char *show,*stat;
GList *it;
GTime timestamp;

	s=session_get_by_jid(from,NULL);
	if (jid_is_me(to)){
		if (s) presence_send(stream,NULL,s->user->jid,s->available,s->show,s->status,0);
		else presence_send(stream,to,from,0,NULL,"Not logged in",0);
		return 0;
	}
	
	if (!jid_is_my(to)){
		presence_send_error(stream,to,from,404,"Not Found");
		return -1;
	}

	if (s) u=s->user;
	else u=user_get_by_jid(from);	
	
	if (!u){
		presence_send_error(stream,to,from,407,"Not logged in");
		return -1;
	}

	uin=jid_get_uin(to);
	status=0;
	for(it=u->contacts;it;it=it->next){
		Contact *c=(Contact *)it->data;
	
		if (c && c->uin==uin){
			status=c->status;
			timestamp=c->last_update;
		}
	}
	if (!status){
		presence_send_error(stream,to,from,404,"Not Found");
		return -1;
	}
	
	switch(status){
		case GG_STATUS_NOT_AVAIL:
			available=0;
			show=NULL;
			stat="Not available";
			break;
		case GG_STATUS_AVAIL:
			available=1;
			show=NULL;
			stat="Available";
			break;
		case GG_STATUS_BUSY:
			available=1;
			show="dnd";
			stat="Busy";
			break;
		default:
			available=1;
			show=NULL;
			stat="Unknown";
			break;
	}

	presence_send(s->s,to,u->jid,available,show,stat,timestamp);
	if (s) session_send_notify(s);
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
	if (status_n) status=xmlnode_get_data(status_n);
	else status=NULL;

	if (!type) type="available";
	
	if (!from || !to){
		presence_send_error(stream,to,from,406,"Not Acceptable");
		g_warning("Bad <presence/>: %s",xmlnode2str(tag));
		return -1;
	}

	if (!jid_is_my(to)){
		presence_send_error(stream,to,from,406,"Not Acceptable");
		g_warning("Wrong 'to' in %s",xmlnode2str(tag));
		return -1;
	}
	
	if (!strcmp(type,"available"))
		return presence(stream,from,to,1,show,status);
	else if (!strcmp(type,"unavailable"))
		return presence(stream,from,to,0,show,status);
	else if (!strcmp(type,"subscribe"))
		return presence_subscribe(stream,from,to);
	else if (!strcmp(type,"unsubscribe"))
		return presence_unsubscribe(stream,from,to);
	else if (!strcmp(type,"subscribed"))
		return presence_subscribed(stream,from,to);
	else if (!strcmp(type,"unsubscribed"))
		return presence_unsubscribed(stream,from,to);
	else if (!strcmp(type,"probe"))
		return presence_probe(stream,from,to);
	else if (!strcmp(type,"error")){
		g_warning("Error presence received: %s",xmlnode2str(tag));
		return 0;
	}
	
	g_warning("Unsupported type in %s",xmlnode2str(tag));
	presence_send_error(stream,to,from,501,"Not Implemented");
	return -1;
}

