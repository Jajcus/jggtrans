#include <stdio.h>
#include "presence.h"
#include "jabber.h"
#include "jid.h"
#include "users.h"
#include "sessions.h"
#include "encoding.h"
#include <glib.h>

int message_send(struct stream_s *stream,const char *from,
		const char *to,int chat,const char *message){
xmlnode msg;
xmlnode n;

	msg=xmlnode_new_tag("message");
	if (from!=NULL) 
		xmlnode_put_attrib(msg,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(msg,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(msg,"to",to);
	if (chat) xmlnode_put_attrib(msg,"type","chat");
	n=xmlnode_insert_tag(msg,"body");
	xmlnode_insert_cdata(n,to_utf8(message),-1);
	stream_write(stream,msg);
	xmlnode_free(msg);
	return 0;
}

int message_send_error(struct stream_s *stream,const char *from,
		const char *to,const char *body,int code){
xmlnode msg;
xmlnode n;
char *s;

	msg=xmlnode_new_tag("message");
	if (from!=NULL) 
		xmlnode_put_attrib(msg,"from",from);
	else{
		char *jid;
		jid=jid_my_registered();
		xmlnode_put_attrib(msg,"from",jid);
		g_free(jid);
	}
	xmlnode_put_attrib(msg,"to",to);
	xmlnode_put_attrib(msg,"type","error");
	s=g_strdup_printf("%03u",code);
	xmlnode_put_attrib(msg,"code",s);
	g_free(s);
	s=g_strdup_printf("Error %03u",code);
	n=xmlnode_insert_tag(msg,"error");
	xmlnode_insert_cdata(n,s,-1);
	g_free(s);
	if (body) {
		n=xmlnode_insert_tag(msg,"body");
		xmlnode_insert_cdata(n,body,-1);
	}
	stream_write(stream,msg);
	xmlnode_free(msg);
	return 0;
}


int jabber_message(struct stream_s *stream,xmlnode tag){
char *type;
char *from;
char *to;
char *subject;
char *body;
int chat;
xmlnode subject_n;
xmlnode body_n;
Session *s;

	from=xmlnode_get_attrib(tag,"from");
	to=xmlnode_get_attrib(tag,"to");
	if (!to || !jid_is_my(to) || !jid_has_uin(to)){
		fprintf(stderr,"Bad 'to'\n");
		return -1;
	}

	if (!from){
		fprintf(stderr,"Anonymous message?\n");
		return -1;
	}
	
	body_n=xmlnode_get_tag(tag,"body");
	if (body_n) body=xmlnode_get_data(body_n);
	else body=NULL;
	
	subject_n=xmlnode_get_tag(tag,"subject");
	if (subject_n) subject=xmlnode_get_data(subject_n);
	else subject=NULL;
	
	type=xmlnode_get_attrib(tag,"type");
	if (!type || !g_strcasecmp(type,"normal")) chat=0;
	else if (!g_strcasecmp(type,"chat")) chat=1;
	else {
		fprintf(stderr,"Unsupported message type\n");
		message_send_error(stream,to,from,body,499); /* FIXME */
		return -1;
	}
	
	s=session_get_by_jid(from,NULL);
	if (!s || !s->connected){
		fprintf(stderr,"Not logged in\n");
		message_send_error(stream,to,from,body,499); /* FIXME */
		return -1;
	}	
	
	if (subject)
		body=g_strdup_printf("Subject: %s\n%s",subject,body);
	session_send_message(s,jid_get_uin(to),chat,to_utf8(body));
	if (subject)
		g_free(body);
	
	return 0;
}

