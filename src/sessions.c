#include <stdio.h>
#include "sessions.h"
#include "iq.h"
#include "fds.h"
#include "ggtrans.h"
#include "presence.h"
#include "users.h"
#include <libgg.h>
#include <assert.h>

int sessions_init(){
char *proxy_ip;
char *proxy_port;
int port;
xmlnode node;

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
	/*gg_debug_level=255;*/
	sessions_jid=g_hash_table_new(g_str_hash,g_str_equal);
	if (!sessions_jid) return -1;
	sessions_uin=g_hash_table_new(g_int_hash,g_int_equal);
	if (!sessions_uin) return -1;
	return 0;
}

int session_event_status(Session *s,int status,uin_t uin){
int available;
char *ujid;
char *show,*stat;

	ujid=g_strdup_printf("%lu@%s",uin,my_name);
	switch(status){
		case GG_STATUS_NOT_AVAIL:
			available=0;
			show=NULL;
			stat="Not available";
			break;
		case GG_STATUS_AVAIL:
			available=0;
			show=NULL;
			stat="Available";
			break;
		case GG_STATUS_BUSY:
			available=0;
			show="dnd";
			stat="Busy";
			break;
		default:
			available=0;
			show=NULL;
			stat="Available";
			break;
	}
	presence_send(s->s,ujid,s->jid,available,show,stat);
	g_free(ujid);
	return 0;
}


int session_event_notify(Session *s,struct gg_event *event){
int i;

	for(i=0;event->event.notify[i].uin;i++)
		session_event_status(s,event->event.notify[i].uin,
				event->event.notify[i].status);
	return 0;
}

int session_fd_handler(fd_handler *h){
Session *s;
struct gg_event *event;

	s=(Session *)h->extra;
	event=gg_watch_fd(s->ggs);
	if (!event){
		fprintf(stderr,"\nConnection broken\n");
		if (s->req_id){
			jabber_iq_send_error(s->s,s->user->jid,s->req_id,"Server connection broken");
		}
		else presence_send(s->s,my_name,s->jid,0,NULL,"Connection broken");
		s->connected=0;
		session_remove(s);
		return -1;
	}
	if (event->type==GG_EVENT_CONN_FAILED){
		fprintf(stderr,"\nLogin failed\n");
		if (s->req_id){
			jabber_iq_send_error(s->s,s->user->jid,s->req_id,"Login failed");
		}
		else presence_send(s->s,my_name,s->jid,0,NULL,"Login failed");
		session_remove(s);
		return -1;
	}
	else if (event->type==GG_EVENT_CONN_SUCCESS){
		fprintf(stderr,"\nLogin succeed\n");
		presence_send_subscribe(s->s,NULL,s->user->jid);
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
		if (s->user->userlist && s->user->userlist_len){
			fprintf(stderr,"\ngg_notify(%p,%p,%i)\n",s->ggs,s->user->userlist,s->user->userlist_len);
			gg_notify(s->ggs,s->user->userlist,s->user->userlist_len);
		}
		presence_send(s->s,my_name,s->jid,1,NULL,"Online");
	}	
	else if (event->type==GG_EVENT_NOTIFY)
		session_event_notify(s,event);
	else if (event->type==GG_EVENT_STATUS)
		session_event_status(s,event->event.status.uin,event->event.status.status);
	else fprintf(stderr,"\nGG event: %i\n",event->type);

	h->read=(s->ggs->check&GG_CHECK_READ);
	h->write=(s->ggs->check&GG_CHECK_WRITE);
	gg_free_event(event);
	
	return 0;
}

int session_remove(Session *s){

	g_hash_table_remove(sessions_jid,(gpointer)s->jid);
	g_hash_table_remove(sessions_uin,GINT_TO_POINTER(s->user->uin));
	if (s->fdh) fd_unregister_handler(s->fdh);
	if (s->ggs){
		if (s->connected) {
			fprintf(stderr,"\ngg_logoof(%p)\n",s->ggs);
			gg_logoff(s->ggs);
		}
		gg_free_session(s->ggs);
	}
	if (s->query) xmlnode_free(s->query);
	if (s->user) user_delete(s->user);
	g_free(s);
	return 0;
}

Session *session_create(User *user,const char *jid,const char *req_id,const xmlnode query,struct stream_s *stream){
Session *s;
int t;

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
	s->fdh=(fd_handler *)g_malloc(sizeof(fd_handler));
	memset(s->fdh,0,sizeof(fd_handler));
	s->fdh->fd=s->ggs->fd;
	s->fdh->read=(s->ggs->check & GG_CHECK_READ);
	s->fdh->write=(s->ggs->check & GG_CHECK_WRITE);
	s->fdh->extra=s;
	s->fdh->func=session_fd_handler;
	s->s=stream;
	t=fd_register_handler(s->fdh);
	assert(t==0);
	g_hash_table_insert(sessions_jid,(gpointer)s->jid,(gpointer)s);
	g_hash_table_insert(sessions_uin,GINT_TO_POINTER(s->user->uin),(gpointer)s);
	return s;	
}

Session *session_get_by_jid(const char *jid,Stream *stream){
Session *s;
User *u;
	
	s=(Session *)g_hash_table_lookup(sessions_jid,(gpointer)jid);
	if (s) return s;
	if (!stream) return NULL;
	u=user_get_by_jid(jid);
	if (!u) return NULL;
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
	if (r) return r;
	if (s->connected){
	     /*	gg_add_notify(s->ggs,uin); */
	     	fprintf(stderr,"\ngg_notify(%p,%p,%i)\n",s->ggs,s->user->userlist,s->user->userlist_len);
	     	gg_notify(s->ggs,s->user->userlist,s->user->userlist_len);
	}
	return 0;
}

int session_unsubscribe(Session *s,uin_t uin){

	user_unsubscribe(s->user,uin);
	if (s->connected){
			fprintf(stderr,"\ngg_notify(%p,%p,%i)\n",s->ggs,s->user->userlist,s->user->userlist_len);
			gg_notify(s->ggs,s->user->userlist,s->user->userlist_len);
	}
	return 0;
}

