#include <libgg.h>
#include "ggtrans.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "sessions.h"
#include "iq.h"
#include "presence.h"
#include "users.h"
#include "jid.h"
#include "message.h"
#include "debug.h"

static int conn_timeout=30;
static int pong_timeout=30;
static int ping_interval=10;
GHashTable *sessions_jid;

int sessions_init(){
char *proxy_ip;
char *proxy_port;
char *data;
int port;
xmlnode node;

#ifdef DEBUG
	if (isatty(2)) gg_debug_level=255;
#endif

	sessions_jid=g_hash_table_new(g_str_hash,g_str_equal);
	if (!sessions_jid) return -1;
	
	node=xmlnode_get_tag(config,"conn_timeout");
	if (node){
		data=xmlnode_get_data(node);
		if (data) conn_timeout=atoi(data);
	}
	node=xmlnode_get_tag(config,"ping_interval");
	if (node){
		data=xmlnode_get_data(node);
		if (data) ping_interval=atoi(data);
	}
	
	node=xmlnode_get_tag(config,"proxy/ip");
	if (node) proxy_ip=xmlnode_get_data(node);
	if (!node || !proxy_ip) return 0;
	node=xmlnode_get_tag(config,"proxy/port");
	if (node) proxy_port=xmlnode_get_data(node);
	
	if (!node || !proxy_port) return 0;
	port=atoi(proxy_port);
	g_message("Using proxy: http://%s:%i",proxy_ip,port);
	gg_http_use_proxy=1;
	gg_http_proxy_host=proxy_ip;
	gg_http_proxy_port=port;
	return 0;
}

static int session_destroy(Session *s);

static gboolean sessions_hash_remove_func(gpointer key,gpointer value,gpointer udata){

	session_destroy((Session *)value);
	g_free(key);
	return TRUE;
}

int sessions_done(){

	g_hash_table_foreach_remove(sessions_jid,sessions_hash_remove_func,NULL);
	return 0;
}

gboolean session_timeout(gpointer data){
Session *s;

	g_assert(data!=NULL);
	s=(Session *)data;
	s->timeout_func=0;
	g_warning("Session timeout for %s",s->jid);
	
	if (s->req_id){
		jabber_iq_send_error(s->s,s->jid,s->req_id,504,"Remote Server Timeout");
	}
	else{
		presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection Timeout",0);
	}

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
		return TRUE;
	}
	
	if (!s->ping_timer) s->ping_timer=g_timer_new();
	else g_timer_reset(s->ping_timer);
	g_timer_start(s->ping_timer);
	gg_ping(s->ggs);
	s->waiting_for_pong=TRUE;
	return TRUE;
}

int session_event_status(Session *s,int status,uin_t uin){
int available;
char *ujid;
char *show,*stat;

	user_set_contact_status(s->user,status,uin);

	ujid=jid_build(uin);
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
			stat="Available";
			break;
	}
	presence_send(s->s,ujid,s->user->jid,available,show,stat,0);
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
				event->event.notify[i].uin);
	return 0;
}

int session_io_handler(GIOChannel *source,GIOCondition condition,gpointer data){
Session *s;
struct gg_event *event;
char *jid;
int chat;
GIOCondition cond;
gdouble t;

	if (condition&(G_IO_ERR|G_IO_NVAL)){
		s=(Session *)data;
		if (condition&G_IO_ERR) g_warning("Error on connection for %s",s->jid);
		if (condition&G_IO_HUP) g_warning("Hangup on connection for %s",s->jid);
		if (condition&G_IO_NVAL) g_warning("Invalid channel on connection for %s",s->jid);
		if (s->req_id){
			jabber_iq_send_error(s->s,s->jid,s->req_id,502,"Remote Server Error");
		}
		else{
			presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection broken",0);
		}
		s->connected=0;
		s->io_watch=0;
		session_remove(s);
		return FALSE;
	}
	
	s=(Session *)data;
	event=gg_watch_fd(s->ggs);
	if (!event){
		g_warning("Connection broken. Session of %s",s->jid);
		if (s->req_id){
			jabber_iq_send_error(s->s,s->jid,s->req_id,502,"Remote Server Error");
		}
		else{
			presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection broken",0);
		}
		s->connected=0;
		s->io_watch=0;
		session_remove(s);
		return FALSE;
	}
	
	switch(event->type){
		case GG_EVENT_CONN_FAILED:
			g_warning("Login failed for %s",s->jid);
			if (s->req_id)
				jabber_iq_send_error(s->s,s->jid,s->req_id,401,"Unauthorized");
			else presence_send(s->s,NULL,s->user->jid,0,NULL,"Login failed",0);
			s->io_watch=0;
			session_remove(s);
			return FALSE;
		case GG_EVENT_CONN_SUCCESS:
			g_message("Login succeed for %s",s->jid);
			if (s->req_id)
				jabber_iq_send_result(s->s,s->jid,s->req_id,NULL);
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
			s->available=1;
			session_send_status(s);
			if (s->user->contacts) session_send_notify(s);
			presence_send(s->s,NULL,s->user->jid,1,NULL,"Online",0);
			s->ping_timeout_func=
				g_timeout_add(ping_interval*1000,session_ping,s);
			break;	
		case GG_EVENT_NOTIFY:
			session_event_notify(s,event);
			break;
		case GG_EVENT_STATUS:
			session_event_status(s,event->event.status.status,event->event.status.uin);
			break;
		case GG_EVENT_MSG:
			jid=jid_build(event->event.msg.sender);
			if (event->event.msg.msgclass==GG_CLASS_CHAT) chat=1;
			else chat=0;
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
			s->timeout_func=g_timeout_add(pong_timeout*1000,session_timeout,s);
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
	
	gg_free_event(event);
	
	return FALSE;
}

/* destroys Session object */
static int session_destroy(Session *s){
GList *it;

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
		if (s->connected) {
			debug("gg_logoof(%p)",s->ggs);
			gg_logoff(s->ggs);
		}
		gg_free_session(s->ggs);
	}
	if (s->query) xmlnode_free(s->query);
	if (s->user) user_remove(s->user);
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

Session *session_create(User *user,const char *jid,const char *req_id,const xmlnode query,struct stream_s *stream){
Session *s;
int t;
char *njid;
GIOCondition cond;

	g_message("Creating session for '%s'",jid);
	g_assert(user!=NULL);
	s=g_new0(Session,1);
	s->user=user;
	s->jid=g_strdup(jid);
	if (req_id) s->req_id=g_strdup(req_id);
	s->query=xmlnode_dup(query);
	s->ggs=gg_login(user->uin,user->password,1);
	if (!s->ggs) {
		g_free(s);
		return NULL;
	}
	s->s=stream;
	
	s->ioch=g_io_channel_unix_new(s->ggs->fd);
	cond=G_IO_ERR|G_IO_HUP|G_IO_NVAL;
	if (s->ggs->check&GG_CHECK_READ) cond|=G_IO_IN;
	if (s->ggs->check&GG_CHECK_WRITE) cond|=G_IO_OUT;
	s->io_watch=g_io_add_watch(s->ioch,cond,session_io_handler,s);

	s->timeout_func=g_timeout_add(conn_timeout*1000,session_timeout,s);
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

int session_send_status(Session *s){
int status;

	g_assert(s!=NULL && s->ggs!=NULL);
	if (!s->available) status=GG_STATUS_NOT_AVAIL;
	else if (!s->show) status=GG_STATUS_AVAIL;
	else if (!g_strcasecmp(s->show,"away")) status=GG_STATUS_BUSY;
	else if (!g_strcasecmp(s->show,"dnd")) status=GG_STATUS_BUSY;
	else if (!g_strcasecmp(s->show,"xa")) status=GG_STATUS_BUSY;
	else status=GG_STATUS_AVAIL;
	debug("Changing gg status to %i",status);
	gg_change_status(s->ggs,status);
	return 0;
}

int session_set_status(Session *s,int available,const char *show,const char *status){

	s->available=available;
	if (s->show) g_free(s->show);
	if (show) s->show=g_strdup(show);
	else s->show=NULL;
	if (s->status) g_free(s->status);
	if (status) s->status=g_strdup(status);
	else s->status=NULL;
	if (!available) return session_remove(s);
	if (s->connected) return session_send_status(s);
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

int session_send_message(Session *s,uin_t uin,int chat,const char *body){

	g_assert(s!=NULL);
	if (!s->connected || !s->ggs) return -1;
	gg_send_message(s->ggs,chat?GG_CLASS_CHAT:GG_CLASS_MSG,uin,(char *)body);
	return 0; /* FIXME: check for errors */
}
