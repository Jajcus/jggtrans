#include <stdio.h>
#include "sessions.h"
#include "iq.h"
#include "fds.h"
#include "ggtrans.h"
#include "presence.h"
#include "users.h"
#include "jid.h"
#include "message.h"
#include <libgg.h>
#include <assert.h>

static int conn_timeout=30;
static int ping_interval=10;
GHashTable *sessions_jid;

int sessions_init(){
char *proxy_ip;
char *proxy_port;
char *data;
int port;
xmlnode node;

	/*gg_debug_level=255;*/

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
	fprintf(stderr,"Using proxy: http://%s:%i\n",proxy_ip,port);
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
	presence_send(s->s,ujid,s->user->jid,available,show,stat);
	g_free(ujid);
	return 0;
}
			
int session_send_notify(Session *s){
GList *it;
uin_t *userlist;
int userlist_len;
int i;

	assert(s!=NULL);
	userlist_len=g_list_length(s->user->contacts);
	userlist=(uin_t *)g_malloc(sizeof(uin_t)*(userlist_len+1));

	i=0;
	for(it=g_list_first(s->user->contacts);it;it=it->next)
		userlist[i++]=((Contact *)it->data)->uin;

	userlist[i]=0;
	
	fprintf(stderr,"\ngg_notify(%p,%p,%i)\n",s->ggs,userlist,userlist_len);
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

int session_fd_handler(FdHandler *h){
Session *s;
struct gg_event *event;
char *jid;
int chat;

	s=(Session *)h->extra;
	event=gg_watch_fd(s->ggs);
	if (!event){
		fprintf(stderr,"\nConnection broken\n");
		if (s->req_id){
			jabber_iq_send_error(s->s,s->jid,s->req_id,"Server connection broken");
		}
		else{
			presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection broken");
		}
		s->connected=0;
		session_remove(s);
		return -1;
	}
	switch(event->type){
		case GG_EVENT_CONN_FAILED:
			fprintf(stderr,"\nLogin failed\n");
			if (s->req_id)
				jabber_iq_send_error(s->s,s->jid,s->req_id,"Login failed");
			else presence_send(s->s,NULL,s->user->jid,0,NULL,"Login failed");
			session_remove(s);
			return -1;
		case GG_EVENT_CONN_SUCCESS:
			fprintf(stderr,"\nLogin succeed\n");
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
			presence_send(s->s,NULL,s->user->jid,1,NULL,"Online");
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
		default:
			fprintf(stderr,"\nGG event: %i\n",event->type);
			break;
	}

	h->read=(s->ggs->check&GG_CHECK_READ);
	h->write=(s->ggs->check&GG_CHECK_WRITE);
	gg_free_event(event);
	
	return 0;
}

/* destroys Session object */
static int session_destroy(Session *s){

	fprintf(stderr,"\nDeleting session for '%s'\n",s->jid);
	if (s->connected && s->s && s->jid)
		presence_send(s->s,NULL,s->user->jid,0,NULL,"Offline");
	if (s->fdh) fd_unregister_handler(s->fdh);
	if (s->ggs){
		if (s->connected) {
			fprintf(stderr,"\ngg_logoof(%p)\n",s->ggs);
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

	assert(sessions_jid!=NULL);
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

	fprintf(stderr,"\nCreating session for '%s'\n",jid);
	assert(user!=NULL);
	s=(Session *)g_malloc(sizeof(Session));
	assert(s!=NULL);
	memset(s,0,sizeof(Session));
	s->user=user;
	s->jid=g_strdup(jid);
	s->req_id=g_strdup(req_id);
	s->query=xmlnode_dup(query);
	s->ggs=gg_login(user->uin,user->password,1);
	if (!s->ggs) {
		g_free(s);
		return NULL;
	}
	s->fdh=(FdHandler *)g_malloc(sizeof(FdHandler));
	memset(s->fdh,0,sizeof(FdHandler));
	s->fdh->fd=s->ggs->fd;
	s->fdh->read=(s->ggs->check & GG_CHECK_READ);
	s->fdh->write=(s->ggs->check & GG_CHECK_WRITE);
	s->fdh->extra=s;
	s->fdh->func=session_fd_handler;
	s->s=stream;
	t=fd_register_handler(s->fdh);
	s->last_ping=time(NULL);
	assert(t==0);
	assert(sessions_jid!=NULL);
	njid=jid_normalized(s->jid);
	g_hash_table_insert(sessions_jid,(gpointer)njid,(gpointer)s);
	return s;	
}

Session *session_get_by_jid(const char *jid,Stream *stream){
Session *s;
User *u;
char *njid;
	
	assert(sessions_jid!=NULL);
	fprintf(stderr,"\nLooking up session for '%s'\n",jid);
	njid=jid_normalized(jid);
	fprintf(stderr,"Using '%s' as key\n",njid);
	s=(Session *)g_hash_table_lookup(sessions_jid,(gpointer)njid);
	g_free(njid);
	if (s) return s;
	fprintf(stderr,"Not found\n");
	if (!stream) return NULL;
	u=user_get_by_jid(jid);
	if (!u) return NULL;
	fprintf(stderr,"Creating new session\n");
	return session_create(u,jid,NULL,NULL,stream);
}

int session_send_status(Session *s){
int status;

	assert(s!=NULL && s->ggs!=NULL);
	if (!s->available) status=GG_STATUS_NOT_AVAIL;
	else if (!s->show) status=GG_STATUS_AVAIL;
	else if (!g_strcasecmp(s->show,"away")) status=GG_STATUS_BUSY;
	else if (!g_strcasecmp(s->show,"dnd")) status=GG_STATUS_BUSY;
	else if (!g_strcasecmp(s->show,"xa")) status=GG_STATUS_BUSY;
	else status=GG_STATUS_AVAIL;
	fprintf(stderr,"\nChanging gg status to %i\n",status);
	gg_change_status(s->ggs,status);
	return 0;
}

int session_set_status(Session *s,int available,const char *show,const char *status){

	s->available=available;
	if (s->show) g_free(s->show);
	if (show) s->show=g_strdup(show);
	if (s->status) g_free(s->status);
	if (status) s->status=g_strdup(status);
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

static void sessions_iter_func(gpointer key,gpointer value,gpointer udata){
Session *s;

	s=(Session *)value;
	if (s->connected && s->ggs && ping_interval)
		if (time(NULL)>s->last_ping+ping_interval){
			gg_ping(s->ggs);
			s->last_ping=time(NULL);
		}
}

int sessions_iter(){

	g_hash_table_foreach(sessions_jid,sessions_iter_func,NULL);
	return 0;
}

int session_send_message(Session *s,uin_t uin,int chat,const char *body){

	assert(s!=NULL);
	if (!s->connected || !s->ggs) return -1;
	gg_send_message(s->ggs,chat?GG_CLASS_CHAT:GG_CLASS_MSG,uin,(char *)body);
	return 0; /* FIXME: check for errors */
}
