/* $Id: sessions.c,v 1.39 2003/01/27 11:15:32 mmazur Exp $ */

/*
 *  (C) Copyright 2002 Jacek Konieczny <jajcus@pld.org.pl>
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
#include <libgadu.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#include "sessions.h"
#include "iq.h"
#include "presence.h"
#include "users.h"
#include "jid.h"
#include "message.h"
#include "conf.h"
#include "status.h"
#include "debug.h"


static int conn_timeout=30;
static int pong_timeout=30;
static int ping_interval=10;
static int reconnect=0;
static int gg_port=0;
static server_t gg_server[20];	/* whois 217.17.41.80 to see why 20 */
GHashTable *sessions_jid;

static void session_stream_destroyed(gpointer key,gpointer value,gpointer user_data){
Session *s=(Session *)value;
Stream *stream=(Stream *)user_data;

	if (s->s==stream) s->s=NULL;
}

static void sessions_stream_destroyed(struct stream_s *stream){

	g_hash_table_foreach(sessions_jid,session_stream_destroyed,stream);
}


int sessions_init(){
char *proxy_ip;
char *p,*r;
int port;
int i;
xmlnode parent,tag;

	stream_add_destroy_handler(sessions_stream_destroyed);

	sessions_jid=g_hash_table_new(g_str_hash,g_str_equal);
	if (!sessions_jid) return -1;

	i=config_load_int("conn_timeout",0);
	if (i>0) conn_timeout=i;
	i=config_load_int("pong_timeout",0);
	if (i>0) pong_timeout=i;
	i=config_load_int("ping_interval",0);
	if (i>0) ping_interval=i;
	i=config_load_int("reconnect",0);
	if (i>0) reconnect=i;

	gg_server[0].port=1;
	inet_aton("217.17.41.84", &gg_server[1].addr);
	gg_server[1].port=8074;
	gg_server[2].port=0;

	parent=xmlnode_get_tag(config,"servers");
	if (parent && xmlnode_has_children(parent)){
		i=0;
		for(tag=xmlnode_get_firstchild(parent); tag!=NULL;
				tag=xmlnode_get_nextsibling(tag)){
			if(xmlnode_get_type(tag) != NTYPE_TAG) continue;
			p=xmlnode_get_name(tag);
			printf("%s\n", p);
			if (strcmp(p, "hub")==0){
				gg_server[i].port=1;
				gg_server[i+1].port=0;
				i++;
			}
			else if (strcmp(p, "server")==0){
				if((r=xmlnode_get_attrib(tag, "port")))
					gg_server[i].port=atoi(r);
				else
					gg_server[i].port=8074;

				r=xmlnode_get_data(tag);
				if(inet_aton(r, &gg_server[i].addr)){
					gg_server[i+1].port=0;
					i++;
				}
			}
		} 
		

	}	

	proxy_ip=config_load_string("proxy/ip");
	if (!proxy_ip) return 0;
	port=config_load_int("proxy/port",0);
	if (port<=0) return 0;

	g_message("Using proxy: http://%s:%i",proxy_ip,port);
	gg_proxy_enabled=1;
	gg_proxy_host=proxy_ip;
	gg_proxy_port=port;
	return 0;
}

static int session_destroy(Session *s);

static gboolean sessions_hash_remove_func(gpointer key,gpointer value,gpointer udata){

	session_destroy((Session *)value);
	g_free(key);
	return TRUE;
}

int sessions_done(){
guint s;

	s=g_hash_table_size(sessions_jid);
	debug("%u sessions in hash table",s);

	g_hash_table_foreach_remove(sessions_jid,sessions_hash_remove_func,NULL);
	g_hash_table_destroy(sessions_jid);

	stream_del_destroy_handler(sessions_stream_destroyed);

	return 0;
}

gboolean sessions_reconnect(gpointer data){
char *jid;

	jid=(char *)data;
	presence_send_probe(jabber_stream(),jid);
	g_free(jid);
	return FALSE;
}

void session_schedule_reconnect(Session *s){
int t;

	if (!reconnect) return;
	t=(int)((reconnect*9.0/10.0)+(2.0*reconnect/10.0*rand()/(RAND_MAX+1.0)));
	debug("Sheduling reconnect in %u seconds",t);
	g_timeout_add(t*1000,sessions_reconnect,g_strdup(s->jid));
}

gboolean session_timeout(gpointer data){
Session *s;

	g_assert(data!=NULL);
	s=(Session *)data;

	g_warning("Timeout for server %u", s->current_server-1);
	
	if(gg_server[s->current_server].port!=0)
		if(!session_try_login(s))
			return FALSE;
		
	s->timeout_func=0;
	g_warning("Session timeout for %s",s->jid);

	if (s->req_id){
		jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,504,"Remote Server Timeout");
	}
	else{
		presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection Timeout",0);
	}
	
	session_schedule_reconnect(s);
	session_remove(s);
	return FALSE;
}

gboolean session_ping(gpointer data){
Session *s;

	debug("Ping...");
	g_assert(data!=NULL);
	s=(Session *)data;
	if (s->waiting_for_pong){
		debug("Pong still not received :-( ...");
		gg_ping(s->ggs); /* send ping, even if server doesn't respond
				    this one will be not counted for the ping delay*/
		return TRUE;
	}

	if (!s->ping_timer) s->ping_timer=g_timer_new();
	else g_timer_reset(s->ping_timer);
	g_timer_start(s->ping_timer);
	gg_ping(s->ggs);
	s->waiting_for_pong=TRUE;
	if (s->timeout_func) g_source_remove(s->timeout_func);
	s->timeout_func=g_timeout_add(pong_timeout*1000,session_timeout,s);
	return TRUE;
}

int session_event_status(Session *s,int status,uin_t uin,char *desc,
				int more,uint32_t ip,uint16_t port,uint32_t version){
int available;
char *ujid;
char *show;

	available=status_gg_to_jabber(status,&show,&desc);
	user_set_contact_status(s->user,status,uin,desc,more,ip,port,version);

	ujid=jid_build(uin);
	presence_send(s->s,ujid,s->user->jid,available,show,desc,0);
	g_free(ujid);
	return 0;
}

int session_send_notify(Session *s){
GList *it;
uin_t *userlist;
int userlist_len;
int i;

	g_assert(s!=NULL);
	userlist_len=g_list_length(s->user->contacts);
	userlist=g_new(uin_t,userlist_len+1);

	i=0;
	for(it=g_list_first(s->user->contacts);it;it=it->next)
		userlist[i++]=((Contact *)it->data)->uin;

	userlist[i]=0;

	debug("gg_notify(%p,%p,%i)",s->ggs,userlist,userlist_len);
	gg_notify(s->ggs,userlist,userlist_len);

	g_free(userlist);
	return 0;
}


int session_event_notify(Session *s,struct gg_event *event){
int i;

	for(i=0;event->event.notify[i].uin;i++)
		session_event_status(s,event->event.notify[i].status,
				event->event.notify[i].uin,
				NULL,1,
				event->event.notify[i].remote_ip,
				event->event.notify[i].remote_port,
				event->event.notify[i].version);
	return 0;
}

int session_event_notify_descr(Session *s,struct gg_event *event){
int i;

	for(i=0;event->event.notify_descr.notify[i].uin;i++)
		session_event_status(s,event->event.notify_descr.notify[i].status,
				event->event.notify_descr.notify[i].uin,
				event->event.notify_descr.descr,1,
				event->event.notify[i].remote_ip,
				event->event.notify[i].remote_port,
				event->event.notify[i].version);
	return 0;
}


int session_io_handler(GIOChannel *source,GIOCondition condition,gpointer data){
Session *s;
struct gg_event *event;
char *jid;
int chat;
GIOCondition cond;
Resource *r;

	s=(Session *)data;
	debug("Checking error conditions...");
	if (condition&(G_IO_ERR|G_IO_NVAL)){
		if (condition&G_IO_ERR) g_warning("Error on connection for %s",s->jid);
		if (condition&G_IO_HUP){
			g_warning("Hangup on connection for %s",s->jid);
			if(!s->connected && gg_server[s->current_server].port!=0){
				session_try_login(s);
				return FALSE;
			}
		}
		if (condition&G_IO_NVAL) g_warning("Invalid channel on connection for %s",s->jid);

		if (s->req_id){
			jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,502,"Remote Server Error");
		}
		else{
			presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection broken",0);
		}
		s->connected=0;
		s->io_watch=0;
		session_schedule_reconnect(s);
		session_remove(s);
		return FALSE;
	}

	debug("watching fd (gg_debug_level=%i)...",gg_debug_level);
	event=gg_watch_fd(s->ggs);
	if (!event){
		g_warning("Connection broken. Session of %s",s->jid);
		if (s->req_id){
			jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,502,"Remote Server Error");
		}
		else{
			presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection broken",0);
		}
		s->connected=0;
		s->io_watch=0;
		session_schedule_reconnect(s);
		session_remove(s);
		return FALSE;
	}

	switch(event->type){
		case GG_EVENT_DISCONNECT:
			g_warning("Server closed connection of %s",s->jid);
			if (s->req_id){
				jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,502,"Remote Server Error");
			}
			else{
				presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection broken",0);
			}
			s->connected=0;
			s->io_watch=0;
			session_schedule_reconnect(s);
			session_remove(s);
			return FALSE;
		case GG_EVENT_CONN_FAILED:
			g_warning("Login failed for %s",s->jid);
			if (s->req_id)
				jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,401,"Unauthorized");
			else presence_send(s->s,NULL,s->user->jid,0,NULL,"Login failed",0);
			s->io_watch=0;
			if (!s->req_id)
				session_schedule_reconnect(s);
			session_remove(s);
			return FALSE;
		case GG_EVENT_CONN_SUCCESS:
			g_message("Login succeed for %s",s->jid);
			s->current_server=0;
			if (s->req_id)
				jabber_iq_send_result(s->s,s->jid,NULL,s->req_id,NULL);
			presence_send_subscribe(s->s,NULL,s->user->jid);
			/*presence_send_subscribed(s->s,NULL,s->user->jid);*/
			if (s->req_id){
				free(s->req_id);
				s->req_id=NULL;
			}
			if (s->query){
				xmlnode_free(s->query);
				s->query=NULL;
			}
			if (!s->user->confirmed){
				user_save(s->user);
				s->user->confirmed=1;
			}
			s->connected=1;
			session_send_status(s);
			if (s->user->contacts) session_send_notify(s);

			r=session_get_cur_resource(s);
			if (r)
				presence_send(s->s,NULL,s->user->jid,r->available,r->show,r->status,0);
			else
				presence_send(s->s,NULL,s->user->jid,1,NULL,"Online",0);
			if (s->timeout_func) g_source_remove(s->timeout_func);
			s->ping_timeout_func=
				g_timeout_add(ping_interval*1000,session_ping,s);
			break;
		case GG_EVENT_NOTIFY:
			session_event_notify(s,event);
			break;
		case GG_EVENT_NOTIFY_DESCR:
			session_event_notify_descr(s,event);
			break;
		case GG_EVENT_STATUS:
			session_event_status(s,
					event->event.status.status,
					event->event.status.uin,
					event->event.status.descr,
					0,0,0,0);
			break;
		case GG_EVENT_MSG:
			if (event->event.msg.sender==0){
				if (!user_sys_msg_received(s->user,event->event.msg.msgclass)) break;
				jid=jid_my_registered();
				chat=0;
			}
			else{
				jid=jid_build(event->event.msg.sender);
				if (event->event.msg.msgclass==GG_CLASS_CHAT) chat=1;
				else chat=0;
			}
			message_send(s->s,jid,s->user->jid,chat,event->event.msg.message);
			g_free(jid);
			break;
		case GG_EVENT_PONG:
			s->waiting_for_pong=FALSE;
			if (s->ping_timer){
				g_timer_stop(s->ping_timer);
				debug("Pong! ping time: %fs",
						g_timer_elapsed(s->ping_timer,NULL));
			}
			if (s->timeout_func) g_source_remove(s->timeout_func);
			break;
		case GG_EVENT_ACK:
			debug("GG_EVENT_ACK");
			break;
		case GG_EVENT_NONE:
			debug("GG_EVENT_NONE");
			break;
		default:
			g_warning("Unknown GG event: %i",event->type);
			break;
	}

	cond=G_IO_ERR|G_IO_HUP|G_IO_NVAL;
	if (s->ggs->check&GG_CHECK_READ) cond|=G_IO_IN;
	if (s->ggs->check&GG_CHECK_WRITE) cond|=G_IO_OUT;
	s->io_watch=g_io_add_watch(s->ioch,cond,session_io_handler,s);

	gg_event_free(event);
	debug("io handler done...");

	return FALSE;
}

/* destroys Session object */
static int session_destroy(Session *s){
GList *it;
Resource *r=NULL;

	g_message("Deleting session for '%s'",s->jid);
	if (s->ping_timeout_func) g_source_remove(s->ping_timeout_func);
	if (s->timeout_func) g_source_remove(s->timeout_func);
	if (s->ping_timer) g_timer_destroy(s->ping_timer);
	if (s->connected && s->s && s->jid){
		presence_send(s->s,NULL,s->user->jid,0,NULL,"Offline",0);
		for(it=s->user->contacts;it;it=it->next){
			Contact *c=(Contact *)it->data;

			if (c->status!=GG_STATUS_NOT_AVAIL){
				char *ujid;
				ujid=jid_build(c->uin);
				presence_send(s->s,ujid,s->user->jid,0,NULL,"Transport disconnected",0);
				g_free(ujid);
			}
		}
	}
	if (s->ioch) g_io_channel_close(s->ioch);
	if (s->ggs){
		if (s->connected){
			debug("gg_logoff(%p)",s->ggs);
			gg_logoff(s->ggs);
		}
		gg_free_session(s->ggs);
	}
	if (s->query) xmlnode_free(s->query);
	if (s->user) user_remove(s->user);
	for(it=g_list_first(s->resources);it;){
		r=(Resource *)it->data;
		if (r->name) g_free(r->name);
		if (r->show) g_free(r->show);
		if (r->status) g_free(r->status);
		it=it->next;
		g_free(r);
	}
	g_list_free(s->resources);
	s->resources=NULL;

	g_free(s);
	return 0;
}

int session_remove(Session *s){
gpointer key,value;
char *njid;

	g_assert(sessions_jid!=NULL);
	if (s->io_watch) g_source_remove(s->io_watch);
	njid=jid_normalized(s->jid);
	if (g_hash_table_lookup_extended(sessions_jid,(gpointer)njid,&key,&value)){
		g_hash_table_remove(sessions_jid,(gpointer)njid);
		g_free(key);
	}
	g_free(njid);
	session_destroy(s);

	return 0;
}

int session_try_login(Session *s){
struct gg_login_params login_params;
GIOCondition cond;

	g_warning("Trying to log in on server %u", s->current_server);

	if (s->timeout_func) g_source_remove(s->timeout_func);
	if (s->io_watch) g_source_remove(s->io_watch);
	if (s->ioch) g_io_channel_close(s->ioch);

	memset(&login_params,0,sizeof(login_params));
	login_params.uin=s->user->uin;
	login_params.password=s->user->password;
	login_params.async=1;
	if(login_params.server_port=gg_server[s->current_server].port!=1){
		login_params.server_addr=gg_server[s->current_server].addr.s_addr;
		login_params.server_port=gg_server[s->current_server].port;
	}

	s->ggs=gg_login(&login_params);
	if (!s->ggs){
		g_free(s);
		return 1;
	}
	
	s->ioch=g_io_channel_unix_new(s->ggs->fd);
	cond=G_IO_ERR|G_IO_HUP|G_IO_NVAL;
	if (s->ggs->check&GG_CHECK_READ) cond|=G_IO_IN;
	if (s->ggs->check&GG_CHECK_WRITE) cond|=G_IO_OUT;
	s->io_watch=g_io_add_watch(s->ioch,cond,session_io_handler,s);

	s->timeout_func=g_timeout_add(conn_timeout*1000,session_timeout,s);

	s->current_server++;
	
	return 0;
}

Session *session_create(User *user,const char *jid,const char *req_id,const xmlnode query,struct stream_s *stream){
Session *s;
char *njid;

	g_message("Creating session for '%s'",jid);
	g_assert(user!=NULL);
	s=g_new0(Session,1);
	s->user=user;
	s->jid=g_strdup(jid);
	if (req_id) s->req_id=g_strdup(req_id);
	s->query=xmlnode_dup(query);
	s->current_server=0;

	if(session_try_login(s))
		return NULL;

	s->s=stream;

	g_assert(sessions_jid!=NULL);
	njid=jid_normalized(s->jid);
	g_hash_table_insert(sessions_jid,(gpointer)njid,(gpointer)s);
	return s;
}

Session *session_get_by_jid(const char *jid,Stream *stream){
Session *s;
User *u;
char *njid;

	g_assert(sessions_jid!=NULL);
	debug("Looking up session for '%s'",jid);
	njid=jid_normalized(jid);
	debug("Using '%s' as key",njid);
	s=(Session *)g_hash_table_lookup(sessions_jid,(gpointer)njid);
	g_free(njid);
	if (s) return s;
	debug("Session not found");
	if (!stream) return NULL;
	u=user_get_by_jid(jid);
	if (!u) return NULL;
	debug("Creating new session");
	return session_create(u,jid,NULL,NULL,stream);
}

Resource *session_get_cur_resource(Session *s){
GList *it;
Resource *r=NULL;
int maxprio;

	maxprio=-1;
	for(it=g_list_last(s->resources);it;it=it->prev){
		Resource *r1=(Resource *)it->data;
		if (r1->priority>maxprio){
			r=r1;
			maxprio=r1->priority;
		}
	}
	return r;
}

int session_send_status(Session *s){
int status;
Resource *r;

	g_assert(s!=NULL && s->ggs!=NULL);
	r=session_get_cur_resource(s);
	if (!r) return -1;
	status=status_jabber_to_gg(r->available,r->show,r->status);
	if (s->user->invisible) status=GG_STATUS_INVISIBLE;
	else if (s->user->friends_only) status|=GG_STATUS_FRIENDS_MASK;
	debug("Changing gg status to %i",status);
	if (r->status!=NULL)
		gg_change_status_descr(s->ggs,status,r->status);
	else
		gg_change_status(s->ggs,status);
	return 0;
}

int session_set_status(Session *s,const char *resource,int available,
			const char *show,const char *status,int priority){
Resource *r;
GList *it;

	r=NULL;
	for(it=g_list_first(s->resources);it;it=it->next){
		Resource *r1=(Resource *)it->data;
		if ( (!r1->name && !resource) || !strcmp(r1->name,resource) ){
			r=r1;
			break;
		}
	}

	if (!available){
		if (r){
			debug("Removing resource %s of %s",resource?resource:"NULL",s->jid);
			if (r->name) g_free(r->name);
			if (r->show) g_free(r->show);
			if (r->status) g_free(r->status);
			s->resources=g_list_remove(s->resources,r);
			g_free(r);
			if (!s->resources){
				session_remove(s);
				return -1;
			}
		}
		else{
			g_warning("Unknown resource %s of %s",resource?resource:"NULL",s->jid);
			return 0;
		}
	}
	else{

		if ( r ){
			if (r->show){
				g_free(r->show);
				r->show=NULL;
			}
			if (r->status){
				g_free(r->status);
				r->status=NULL;
			}
		}
		else{
			debug("New resource %s of %s",resource?resource:"NULL",s->jid);
			r=g_new0(Resource,1);
			if (resource) r->name=g_strdup(resource);
			s->resources=g_list_append(s->resources,r);
		}
		r->available=available;
		if (show) r->show=g_strdup(show);
		if (status) r->status=g_strdup(status);
		if (priority>=0) r->priority=priority;
	}

	if (s->connected) session_send_status(s);
	return 0;
}

int session_subscribe(Session *s,uin_t uin){
int r;

	r=user_subscribe(s->user,uin);
	if (r) return 0; /* already subscribed, that is not an error */
	if (s->connected) gg_add_notify(s->ggs,uin);
	return 0;
}

int session_unsubscribe(Session *s,uin_t uin){

	user_unsubscribe(s->user,uin);
	if (s->connected)
		gg_remove_notify(s->ggs,uin);
	return 0;
}

char * session_split_message(const char **msg){
const char *m;
int i;

	m=*msg;
	if (strlen(*msg)<=2000){
		*msg=NULL;
		return g_strdup(m);
	}
	for(i=2000;i>1000;i++){
		if (isspace(m[i])){
			*msg=m+i+1;
			return g_strndup(m,i);
		}
	}
	*msg=m+i;
	return g_strndup(m,i);
}

int session_send_message(Session *s,uin_t uin,int chat,const char *body){
const char *m;
char *mp;

	g_assert(s!=NULL);
	if (!s->connected || !s->ggs) return -1;
	m=body;
	while(m){
		mp=session_split_message(&m);
		if (mp){
			gg_send_message(s->ggs,chat?GG_CLASS_CHAT:GG_CLASS_MSG,uin,mp);
			g_free(mp);
		}
	}
	return 0; /* FIXME: check for errors */
}

void session_print(Session *s,int indent){
char *space,*space1;
GList *it;
Resource *r,*cr;
char *njid;

	space=g_strnfill(indent*2,' ');
	space1=g_strnfill((indent+1)*2,' ');
	njid=jid_normalized(s->jid);
	g_message("%sSession: %p",space,s);
	g_message("%sJID: %s",space,s->jid);
	g_message("%sUser:",space);
	user_print(s->user,indent+1);
	g_message("%sResources:",space);
	cr=session_get_cur_resource(s);
	for(it=g_list_first(s->resources);it;it=it->next){
		r=(Resource *)it->data;
		g_message("%sResource: %p%s",space1,r,(r==cr)?" (current)":"");
		if (r->name) g_message("%sJID: %s/%s",space1,njid,r->name);
		else g_message("%sJID: %s",space1,s->jid);
		g_message("%sPriority: %i",space1,r->priority);
		g_message("%sAvailable: %i",space1,r->available);
		if (r->show)
			g_message("%sShow: %s",space1,r->show);
		if (r->status)
			g_message("%sStatus: %s",space1,r->status);
	}
	g_message("%sGG session: %p",space,s->ggs);
	g_message("%sIO Channel: %p",space,s->ioch);
	g_message("%sStream: %p",space,s->s);
	g_message("%sConnected: %i",space,s->connected);
	g_message("%sRequest id: %s",space,s->req_id?s->req_id:"(null)");
	g_message("%sRequest query: %s",space,s->query?xmlnode2str(s->query):"(null)");
	g_message("%sWaiting for ping: %i",space,(int)s->waiting_for_pong);
	g_free(njid);
	g_free(space1);
	g_free(space);
}

static void sessions_print_func(gpointer key,gpointer value,gpointer user_data){
Session *s=(Session *)value;

	session_print(s,GPOINTER_TO_INT(user_data));
}

void sessions_print_all(int indent){
	g_hash_table_foreach(sessions_jid,sessions_print_func,GINT_TO_POINTER(indent));
}


