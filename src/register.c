#include "ggtrans.h"
#include "jabber.h"
#include "register.h"
#include "iq.h"
#include "users.h"
#include "sessions.h"
#include "presence.h"
#include "users.h"
#include "debug.h"
#include "requests.h"
#include "encoding.h"

const char *register_instructions;

void jabber_iq_get_register(Stream *s,const char *from,const char *id,xmlnode q){
xmlnode node;
xmlnode iq;
xmlnode query;
xmlnode instr;

	node=xmlnode_get_firstchild(q);
	if (node){
		g_warning("Get for jabber:iq:register not empty!: %s",xmlnode2str(q));
		jabber_iq_send_error(s,from,id,406,"Not Acceptable");
		return;
	}		
	iq=xmlnode_new_tag("iq");
	xmlnode_put_attrib(iq,"type","result");
	if (id) xmlnode_put_attrib(iq,"id",id);
	xmlnode_put_attrib(iq,"to",from);
	xmlnode_put_attrib(iq,"from",my_name);
	query=xmlnode_insert_tag(iq,"query");
	xmlnode_put_attrib(query,"xmlns","jabber:iq:register");

	/* needed to register existing user */
	xmlnode_insert_tag(query,"username");
	xmlnode_insert_tag(query,"password");

	/* needed to register new user */
	xmlnode_insert_tag(query,"email");
	
	/* public directory info */
	xmlnode_insert_tag(query,"first");
	xmlnode_insert_tag(query,"last");
	xmlnode_insert_tag(query,"nick");
	xmlnode_insert_tag(query,"city");
	xmlnode_insert_tag(query,"born");
	xmlnode_insert_tag(query,"gender");
	
	instr=xmlnode_insert_tag(query,"instructions");
	xmlnode_insert_cdata(instr,register_instructions,-1);
	stream_write(s,iq);
	xmlnode_free(iq);
}
		
void unregister(Stream *s,const char *from,const char *id,int presence_used){
Session *ses;
User *u;
char *jid;

	debug("Unregistering '%s'",from);
	ses=session_get_by_jid(from,NULL);
	if (ses) 
		if (session_remove(ses)){
			g_warning("'%s' unregistration failed",from);
			jabber_iq_send_error(s,from,id,500,"Internal Server Error");
			return;
		}
			
	u=user_get_by_jid(from);
	if (u){
		jid=g_strdup(u->jid);
		if (user_delete(u)){
			g_warning("'%s' unregistration failed",from);
			jabber_iq_send_error(s,from,id,500,"Internal Server Error");
			return;
		}
	}
	
	if (!u){
		g_warning("Tried to unregister '%s' who was never registered",from);
		jabber_iq_send_error(s,from,id,404,"Not Found");
		return;
	}

	if (!presence_used){
		jabber_iq_send_result(s,from,id,NULL);
		presence_send_unsubscribe(s,NULL,jid);	
	}
	presence_send_unsubscribed(s,NULL,jid);
	g_message("User '%s' unregistered",from);
	g_free(jid);
}

void jabber_iq_set_register(Stream *s,const char *from,const char *id,xmlnode q){
xmlnode node;
char *str,*password;
struct gg_modify modify;
uin_t uin;
User *user;
Session *session;
struct gg_http *gghttp;
Request *r;
	
	node=xmlnode_get_firstchild(q);
	if (!node){
		debug("Set query for jabber:iq:register empty: %s",xmlnode2str(q));
		unregister(s,from,id,0);
		return;
	}
	
	node=xmlnode_get_tag(q,"username");
	if (node) str=xmlnode_get_data(node);
	if (!node || !str) uin=0;
	else uin=atoi(str);
	
	node=xmlnode_get_tag(q,"password");
	if (node) password=xmlnode_get_data(node);
	else password=NULL;

	user=user_get_by_jid(from);

	if (!user && (!uin || !password)){
		g_warning("User '%s' doesn't exist and not enough info to add him",from);
		jabber_iq_send_error(s,from,id,406,"Not Acceptable");
		return;
	}

	if (!user){
		user=user_create(from,uin,password);
		if (!user){
			g_warning("Couldn't create user %s",from);
			jabber_iq_send_error(s,from,id,500,"Internal Server Error");
			return;
		}
	
		session=session_create(user,from,id,q,s);
		if (!session){
			g_warning("Couldn't create session for %s",from);
			jabber_iq_send_error(s,from,id,500,"Internal Server Error");
			return;
		}	
	}

	memset(&modify,0,sizeof(modify));	

	node=xmlnode_get_tag(q,"first");
	if (node) modify.first_name=xmlnode_get_data(node);
	else modify.first_name=NULL;
	
	node=xmlnode_get_tag(q,"last");
	if (node) modify.last_name=xmlnode_get_data(node);
	else modify.last_name=NULL;
	
	node=xmlnode_get_tag(q,"nick");
	if (node) modify.nickname=xmlnode_get_data(node);
	else modify.nickname=NULL;
	
	node=xmlnode_get_tag(q,"email");
	if (node) modify.email=xmlnode_get_data(node);
	else modify.email=NULL;
	
	node=xmlnode_get_tag(q,"city");
	if (node) modify.city=xmlnode_get_data(node);
	else modify.city=NULL;
	
	node=xmlnode_get_tag(q,"gender");
	if (node) str=xmlnode_get_data(node);
	if (!node || !str || !str[0]) modify.gender=GG_GENDER_NONE;
	else if (str[0]=='k' || str[0]=='f' || str[0]=='K' || str[0]=='F') modify.gender=GG_GENDER_FEMALE;
	else modify.gender=GG_GENDER_MALE;
	
	node=xmlnode_get_tag(q,"born");
	if (node) str=xmlnode_get_data(node);
	if (!node || !str || !str[0]) modify.born=0;
	else modify.born=atoi(str);
		
	if (!modify.first_name && !modify.last_name && !modify.nickname
		&& !modify.city && !modify.email && !modify.born && !modify.gender){ 
			if (!uin && !password){
				debug("Set query for jabber:iq:register empty: %s",xmlnode2str(q));
				unregister(s,from,id,0);
				return;
			}
			if (!uin || !password){
				g_warning("Nothing to change");
				jabber_iq_send_error(s,from,id,406,"Not Acceptable");
			}
			return;
	}

	if (!modify.email || !modify.nickname){
		debug("email or nickname not given for registration");
		jabber_iq_send_error(s,from,id,406,"Not Acceptable");
		return;
	}
	
	if (!user && (!password ||!uin)){
		g_warning("Not registered, and no password gived for public directory change.");
		jabber_iq_send_error(s,from,id,406,"Not Acceptable");
		return;
	}
	if (!password) password=user->password;
	if (!uin) uin=user->uin;

	debug("gg_change_pubdir()");
	
	if (modify.first_name) modify.first_name=g_strdup(from_utf8(modify.first_name));
	if (modify.last_name) modify.last_name=g_strdup(from_utf8(modify.last_name));
	if (modify.nickname) modify.nickname=g_strdup(from_utf8(modify.nickname));
	if (modify.email) modify.email=g_strdup(from_utf8(modify.email));
	if (modify.city) modify.city=g_strdup(from_utf8(modify.city));
	
	gghttp=gg_change_pubdir(uin,password,&modify,1);
	
	if (modify.first_name) g_free(modify.first_name);
	if (modify.last_name) g_free(modify.last_name);
	if (modify.nickname) g_free(modify.nickname);
	if (modify.email) g_free(modify.email);
	if (modify.city) g_free(modify.city);
	
	if (!gghttp){
		jabber_iq_send_error(s,from,id,502,"Remote Server Error");
		return;
	}

	r=add_request(RT_CHANGE,from,id,q,gghttp,s);
	if (!r) jabber_iq_send_error(s,from,id,500,"Internal Server Error");
}

int register_error(Request *r){

	jabber_iq_send_error(r->stream,r->from,r->id,502,"Remote Server Error");
	return 0;
}

int register_done(struct request_s *r){
	
	jabber_iq_send_result(r->stream,r->from,r->id,NULL);
	return 0;
}


