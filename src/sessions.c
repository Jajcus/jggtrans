/* $Id: sessions.c,v 1.105 2004/06/11 07:19:11 smoku Exp $ */

/*
 *  (C) Copyright 2002-2005 Jacek Konieczny [jajcus(a)jajcus,net]
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
#include "requests.h"
#include "encoding.h"
#include "debug.h"


static int conn_timeout=30;
static int pong_timeout=30;
static int disconnect_delay=2;
static int ping_interval=10;
static int reconnect=0;
static GList *gg_servers=NULL;
GHashTable *sessions_jid;

static int session_try_login(Session *s);
static int session_destroy(Session *s);
static void resource_remove(Resource *r,int kill_session);
static int session_io_handler(Session *s);

static void session_stream_destroyed(gpointer key,gpointer value,gpointer user_data){
Session *s=(Session *)value;
Stream *stream=(Stream *)user_data;

	if (s->s==stream) s->s=NULL;
}

static void sessions_stream_destroyed(struct stream_s *stream){

	g_hash_table_foreach(sessions_jid,session_stream_destroyed,stream);
}

typedef struct {
	GSource g_src;
	Session *session;
}Source;

static gboolean session_g_source_prepare(GSource *source, gint* timeout_){

	*timeout_=-1;

	return FALSE;
}
static gboolean session_g_source_check(GSource *source){
Source *src=(Source *)(gpointer)source;

	return src->session->g_pollfd.revents != 0;
}
static gboolean session_g_source_dispatch(GSource *source,GSourceFunc callback,gpointer user_data){
Source *src=(Source *)(gpointer)source;

	session_io_handler(src->session);
	return TRUE;
}

static GSourceFuncs source_funcs = {
	session_g_source_prepare,
	session_g_source_check,
	session_g_source_dispatch,
	NULL
};

static void session_setup_g_source(Session *s) {

	if (!s->ggs) return;
	
	if (s->g_source && s->g_pollfd.fd != s->ggs->fd) {
		g_source_remove_poll(s->g_source, &s->g_pollfd);
	}

	if (!s->g_source) {
		s->g_source = g_source_new(&source_funcs, sizeof(Source));
		((Source *)(gpointer)s->g_source)->session=s;
		g_source_attach(s->g_source,g_main_loop_get_context(main_loop));
	}
	
	s->g_pollfd.events=G_IO_ERR|G_IO_HUP|G_IO_NVAL;
	if (s->ggs->check&GG_CHECK_READ) s->g_pollfd.events|=G_IO_IN;
	if (s->ggs->check&GG_CHECK_WRITE) s->g_pollfd.events|=G_IO_OUT;

	if (s->g_pollfd.fd != s->ggs->fd) {
		s->g_pollfd.fd=s->ggs->fd;
		if (s->g_pollfd.fd!=-1) g_source_add_poll(s->g_source, &s->g_pollfd);
	}
}

static void session_remove_g_source(Session *s) {

	if (!s->g_source) return;

	if (s->g_pollfd.fd!=-1) g_source_remove_poll(s->g_source, &s->g_pollfd);
	g_source_destroy(s->g_source);
	s->g_source=NULL;
}


int sessions_init(){
char *proxy_ip,*proxy_username,*proxy_password,*proxy_http_only;
char *p,*r;
int port;
int i;
xmlnode parent,tag;
GgServer *server;

	stream_add_destroy_handler(sessions_stream_destroyed);

	sessions_jid=g_hash_table_new(g_str_hash,g_str_equal);
	if (!sessions_jid) return -1;

	i=config_load_int("conn_timeout",0);
	if (i>0) conn_timeout=i;
	i=config_load_int("pong_timeout",0);
	if (i>0) pong_timeout=i;
	i=config_load_int("disconnect_delay",0);
	if (i>0) disconnect_delay=i;
	i=config_load_int("ping_interval",0);
	if (i>0) ping_interval=i;
	i=config_load_int("reconnect",0);
	if (i>0) reconnect=i;


	parent=xmlnode_get_tag(config,"servers");
	if (parent && xmlnode_has_children(parent)){
		gg_servers=NULL;
		for(tag=xmlnode_get_firstchild(parent); tag!=NULL;
				tag=xmlnode_get_nextsibling(tag)){
			if(xmlnode_get_type(tag) != NTYPE_TAG) continue;
			p=xmlnode_get_name(tag);
			if (strcmp(p, "hub")==0){
				server=g_new(GgServer, 1);
				server->port=1;
				gg_servers=g_list_append(gg_servers, server);
			}
			else if (strcmp(p, "server")==0){
				server=g_new(GgServer, 1);
				r=xmlnode_get_attrib(tag, "port");
				if (r)
					server->port=atoi(r);
				else
					server->port=8074;
				r=xmlnode_get_data(tag);
				if(inet_aton(r, &server->addr))
					gg_servers=g_list_append(gg_servers, server);
			}
			else continue;

			r=xmlnode_get_attrib(tag, "tls");
			if (r && !g_strcasecmp(r,"no"))
				server->tls=0;
			else
				server->tls=1;
		}


	}
	else{
		server=g_new(GgServer, 1);
		server->port=1;
		server->tls=0;
		gg_servers=g_list_append(gg_servers, server);

		server=g_new(GgServer, 1);
		inet_aton("217.17.41.85", &server->addr);
		server->port=8074;
		server->tls=0;
		gg_servers=g_list_append(gg_servers, server);
	}

	proxy_ip=config_load_string("proxy/ip");
	if (!proxy_ip) return 0;
	port=config_load_int("proxy/port",0);
	if (port<=0) return 0;
	proxy_username=config_load_string("proxy/username");
	proxy_password=config_load_string("proxy/password");

	tag=xmlnode_get_tag(config,"proxy");
	proxy_http_only=xmlnode_get_attrib(tag,"http_only");

	g_message(L_("Using proxy: http://%s:%i"),proxy_ip,port);
	gg_proxy_enabled=1;
	gg_proxy_host=proxy_ip;
	gg_proxy_port=port;
	if (proxy_username && proxy_password){
		gg_proxy_username=proxy_username;
		gg_proxy_password=proxy_password;
	}
	if (proxy_http_only && strcmp(proxy_http_only,"no")){
		gg_proxy_http_only=1;
	}
	return 0;
}

static gboolean sessions_hash_remove_func(gpointer key,gpointer value,gpointer udata){

	session_destroy((Session *)value);
	g_free(key);
	return TRUE;
}

int sessions_done(){
guint s;
GList *it;

	s=g_hash_table_size(sessions_jid);
	debug(L_("%u sessions in hash table"),s);

	g_hash_table_foreach_remove(sessions_jid,sessions_hash_remove_func,NULL);
	g_hash_table_destroy(sessions_jid);

	for(it=g_list_first(gg_servers);it;it=g_list_next(it))
		g_free(it->data);
	g_list_free(gg_servers);

	stream_del_destroy_handler(sessions_stream_destroyed);
	return 0;
}

gboolean sessions_reconnect(gpointer data){
char *jid;

	jid=(char *)data;
	presence_send_probe(jabber_stream(),NULL,jid);
	g_free(jid);
	return FALSE;
}

void session_schedule_reconnect(Session *s){
int t;

	s->current_server=g_list_first(gg_servers);
	if (!reconnect) return;
	t=(int)((reconnect*9.0/10.0)+(2.0*reconnect/10.0*rand()/(RAND_MAX+1.0)));
	debug(L_("Sheduling reconnect in %u seconds"),t);
	g_timeout_add(t*1000,sessions_reconnect,g_strdup(s->jid));
}

gboolean session_timeout(gpointer data){
Session *s;

	g_assert(data!=NULL);
	s=(Session *)data;
	user_load_locale(s->user);

	g_warning(N_("Timeout for server %u"),
			g_list_position(gg_servers, s->current_server));

	s->current_server=g_list_next(s->current_server);
	if(s->current_server!=NULL)
		if(!session_try_login(s))
			return FALSE;

	s->timeout_func=0;
	g_warning(N_("Session timeout for %s"),s->jid);

	if (s->req_id){
		jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,504,_("Remote Server Timeout"));
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
int oldstat;
int available;
char *ujid;
char *show;
Contact *c;

	c=user_get_contact(s->user,uin,FALSE);
	if (c==NULL) {
		debug(L_("%s got notification from unknown contact %i, ignoring.."),s->user->jid,uin);
	       	return 0;
	}
	if (!c->got_probe && c->subscribe!=SUB_TO && c->subscribe!=SUB_BOTH) {
		debug(L_("%s got notification from contact %i whose presence was not requested, ignoring.."),s->user->jid,uin);
	       	return 0;
	}

	available=status_gg_to_jabber(status,&show,&desc);
	oldstat=user_get_contact_status(s->user,uin);
	user_set_contact_status(s->user,status,uin,desc,more,ip,port,version);

	if (!available && oldstat==-1)
		ujid=jid_build(uin);
	else
		ujid=jid_build_full(uin);

	presence_send(s->s,ujid,s->user->jid,available,show,desc,0);
	g_free(ujid);
	return 0;
}


int session_event_notify(Session *s,struct gg_event *event){
int i;

	for(i=0;event->event.notify[i].uin;i++)
		session_event_status(s,event->event.notify[i].status,
				event->event.notify[i].uin,
				NULL,
				1,
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
				event->event.notify_descr.descr,
				1,
				event->event.notify[i].remote_ip,
				event->event.notify[i].remote_port,
				event->event.notify[i].version);
	return 0;
}

int session_event_notify60(Session *s,struct gg_event *event){
int i;

	for(i=0;event->event.notify60[i].uin;i++)
		session_event_status(s,event->event.notify60[i].status,
				event->event.notify60[i].uin,
				event->event.notify60[i].descr,
				1,
				event->event.notify60[i].remote_ip,
				event->event.notify60[i].remote_port,
				event->event.notify60[i].version);
	return 0;
}

void session_broken(Session *s){

	if (s->req_id){
		jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,502,_("Remote Server Error"));
	}
	else{
		GList *it;
		presence_send(s->s,NULL,s->user->jid,0,NULL,"Connection broken",0);
		for(it=s->user->contacts;it;it=it->next){
			Contact *c=(Contact *)it->data;

			if (!GG_S_NA(c->status) && c->status!=-1){
				char *ujid;
				ujid=jid_build_full(c->uin);
				presence_send(s->s,ujid,s->user->jid,0,NULL,"Transport disconnected",0);
				g_free(ujid);
			}
		}
	}
	s->connected=0;
	session_schedule_reconnect(s);
	session_remove(s);
}


int session_io_handler(Session *s){
struct gg_event *event;
char *jid,*str;
int chat;
GIOCondition condition=s->g_pollfd.revents;
time_t timestamp;

	user_load_locale(s->user);
	debug(L_("Checking error conditions..."));
	if (condition&(G_IO_ERR|G_IO_NVAL)){
		if (condition&G_IO_ERR) g_warning(N_("Error on connection for %s"),s->jid);
		if (condition&G_IO_HUP){
			g_warning(N_("Hangup on connection for %s"),s->jid);
			s->current_server=g_list_next(s->current_server);
			if(!s->connected && s->current_server!=NULL){
				session_try_login(s);
				return FALSE;
			}
		}
		if (condition&G_IO_NVAL) g_warning(N_("Invalid channel on connection for %s"),s->jid);

		session_broken(s);
		return FALSE;
	}

	debug(L_("watching fd (gg_debug_level=%i)..."),gg_debug_level);
	event=gg_watch_fd(s->ggs);
	if (!event){
		g_warning(N_("Connection broken. Session of %s"),s->jid);
		session_broken(s);
		return FALSE;
	}

	switch(event->type){
		case GG_EVENT_DISCONNECT:
			g_warning(N_("Server closed connection of %s"),s->jid);
			session_broken(s);
			gg_event_free(event);
			return FALSE;
		case GG_EVENT_CONN_FAILED:
			g_warning(N_("Login failed for %s"),s->jid);
			if (s->req_id)
				jabber_iq_send_error(s->s,s->jid,NULL,s->req_id,401,_("Unauthorized"));
			else presence_send(s->s,NULL,s->user->jid,0,NULL,"Login failed",0);
			if (!s->req_id)
				session_schedule_reconnect(s);
			session_remove(s);
			gg_event_free(event);
			return FALSE;
		case GG_EVENT_CONN_SUCCESS:
			g_message(L_("Login succeed for %s"),s->jid);
			if (s->req_id)
				jabber_iq_send_result(s->s,s->jid,NULL,s->req_id,NULL);
			if (s->req_id){
				free(s->req_id);
				s->req_id=NULL;
			}
			if (s->query){
				xmlnode_free(s->query);
				s->query=NULL;
			}
			if (!s->user->confirmed){
				s->user->confirmed=1;
				user_save(s->user);
			}
			s->connected=1;
			session_send_status(s);
			if (s->user->contacts) session_send_notify(s);
			presence_send(s->s,NULL,s->user->jid,1,NULL,s->gg_status_descr,0);

			if (s->timeout_func) g_source_remove(s->timeout_func);
			s->ping_timeout_func=
				g_timeout_add(ping_interval*1000,session_ping,s);
			if (s->pubdir_change){
				add_request(RT_CHANGE,s->jid,NULL,s->req_id,
							NULL,s->pubdir_change,s->s);
				gg_pubdir50_free(s->pubdir_change);
				s->pubdir_change=NULL;
			}
			if (s->get_roster){
				gg_userlist_request(s->ggs, GG_USERLIST_GET, NULL);
			}
			break;
		case GG_EVENT_NOTIFY:
			session_event_notify(s,event);
			break;
		case GG_EVENT_NOTIFY_DESCR:
			session_event_notify_descr(s,event);
			break;
		case GG_EVENT_NOTIFY60:
			session_event_notify60(s,event);
			break;
		case GG_EVENT_STATUS:
			session_event_status(s,
					event->event.status.status,
					event->event.status.uin,
					event->event.status.descr,
					0,0,0,0);
			break;
		case GG_EVENT_STATUS60:
			session_event_status(s,
					event->event.status60.status,
					event->event.status60.uin,
					event->event.status60.descr,
					1,
					event->event.status60.remote_ip,
					event->event.status60.remote_port,
					event->event.status60.version);
			break;
		case GG_EVENT_MSG:
			if (event->event.msg.recipients_count>1){
				debug(L_("Dropped conference message: sender: %i class: %i time: %lu"),
							event->event.msg.sender,
							event->event.msg.msgclass,
							(unsigned long)event->event.msg.time);
				break;
			}
			gg_messages_in++;
			debug(L_("Message: sender: %i class: %i time: %lu"),
							event->event.msg.sender,
							event->event.msg.msgclass,
							(unsigned long)event->event.msg.time);
			
			if (event->event.msg.sender==0){
				if (!user_sys_msg_received(s->user,event->event.msg.msgclass)) break;
				jid=jid_my_registered();
				timestamp=event->event.msg.time;
				str=g_strdup_printf(_("GG System message #%i"),
							event->event.msg.msgclass);
				message_send_subject(s->s,jid,s->user->jid,str,
						to_utf8(event->event.msg.message),timestamp);
				g_free(str);
				jid=jid_my_registered();
				break;
			}
			else{
				Contact *c=user_get_contact(s->user,
						event->event.msg.sender,0);
				if ((!c && s->user->ignore_unknown) 
						|| (c && c->ignored)) {
					debug(L_("Ignoring the message."));
				       	break;
				}
				jid=jid_build_full(event->event.msg.sender);
				if ((event->event.msg.msgclass&GG_CLASS_CHAT)!=0) chat=1;
				else chat=0;
			}
			if ((event->event.msg.msgclass&GG_CLASS_QUEUED)!=0){
				timestamp=event->event.msg.time;
			}
			else timestamp=0;
			message_send(s->s,jid,s->user->jid,chat,
					to_utf8(event->event.msg.message),timestamp);
			g_free(jid);
			break;
		case GG_EVENT_PONG:
			s->waiting_for_pong=FALSE;
			if (s->ping_timer){
				g_timer_stop(s->ping_timer);
				debug(L_("Pong! ping time: %fs"),
						g_timer_elapsed(s->ping_timer,NULL));
			}
			if (s->timeout_func) g_source_remove(s->timeout_func);
			break;
		case GG_EVENT_PUBDIR50_SEARCH_REPLY:
			request_response_search(event);
			break;
		case GG_EVENT_PUBDIR50_WRITE:
			request_response_write(event);
			break;
		case GG_EVENT_ACK:
			debug("GG_EVENT_ACK");
			break;
		case GG_EVENT_NONE:
			debug("GG_EVENT_NONE");
			break;
		case GG_EVENT_USERLIST:
			if(event->event.userlist.type==GG_USERLIST_GET_REPLY)
				get_roster_done(s,event);
			else
				g_warning(N_("Wrong gg userlist type: %i"),event->event.userlist.type);
			break;
		default:
			g_warning(N_("Unknown GG event: %i"),event->type);
			break;
	}

	session_setup_g_source(s);

	gg_event_free(event);
	debug(L_("io handler done..."));

	return FALSE;
}

/* destroys Session object */
static int session_destroy(Session *s){
GList *it;

	g_message(L_("Deleting session for '%s'"),s->jid);
	if (s->ping_timeout_func) g_source_remove(s->ping_timeout_func);
	if (s->timeout_func) g_source_remove(s->timeout_func);
	if (s->ping_timer) g_timer_destroy(s->ping_timer);
	session_remove_g_source(s);
	if (s->ggs){
		if (s->connected){
			debug("gg_logoff(%p)",s->ggs);
			gg_logoff(s->ggs);
		}
		gg_free_session(s->ggs);
	}
	while(s->resources) resource_remove((Resource *)s->resources->data,0);
	if (s->connected && s->s && s->jid){
		presence_send(s->s,NULL,s->user->jid,0,NULL,"Offline",0);
		for(it=s->user->contacts;it;it=it->next){
			Contact *c=(Contact *)it->data;

			if (!GG_S_NA(c->status) && c->status!=-1){
				char *ujid;
				ujid=jid_build_full(c->uin);
				presence_send(s->s,ujid,s->user->jid,0,NULL,"Transport disconnected",0);
				g_free(ujid);
			}
		}
	}
	if (s->query) xmlnode_free(s->query);
	if (s->user) user_unref(s->user);
	if (s->gg_status_descr) g_free(s->gg_status_descr);
	g_free(s);
	return 0;
}

int session_remove(Session *s){
gpointer key,value;
char *njid;

	if (s==NULL) return 1;
	g_assert(sessions_jid!=NULL);
	njid=jid_normalized(s->jid,0);
	g_assert(njid!=NULL);
	if (g_hash_table_lookup_extended(sessions_jid,(gpointer)njid,&key,&value)){
		g_hash_table_remove(sessions_jid,(gpointer)njid);
		g_free(key);
	}
	g_free(njid);
	session_destroy(s);

	return 0;
}

/* returns: -1 on error, 1 on status change, 0 on no change */
static int session_do_make_status(Session *s, Resource *r, gboolean send_presence){
int status;
char *status_descr;

	if (!r) {
		if (send_presence) presence_send(s->s,NULL,s->user->jid,FALSE,NULL,s->gg_status_descr,0);
		return -1;
	}
	status=status_jabber_to_gg(r->available,r->show,r->status);
	status_descr=r->status;
	if (!r->available && s->user->offline_status) status_descr=s->user->offline_status;
	else if (s->user->status) status_descr=s->user->status;
	if (s->user->invisible || r->available==-1){
	       	status=GG_STATUS_INVISIBLE;
		if (s->user->invisible_status) status_descr=s->user->invisible_status;
	}
	else if (s->user->friends_only) status|=GG_STATUS_FRIENDS_MASK;

	if (status==s->gg_status){
		if (r->status!=NULL && s->gg_status_descr!=NULL
				&& !strcmp(status_descr,s->gg_status_descr)) return 0;
		if (r->status==NULL && s->gg_status_descr==NULL) return 0;
	}
	g_free(s->gg_status_descr);
	s->gg_status_descr=g_strdup(status_descr);
	s->gg_status=status;
	if (send_presence) {
		presence_send(s->s,NULL,s->user->jid,r->available,r->show,s->gg_status_descr,0);
	}
	return 1;
}

Resource *session_get_cur_resource(Session *s){
GList *it;
Resource *r=NULL;
int maxprio;

	maxprio=-129;
	for(it=g_list_last(s->resources);it;it=it->prev){
		Resource *r1=(Resource *)it->data;
		if (r1->priority>maxprio){
			r=r1;
			maxprio=r1->priority;
		}
	}
	return r;
}

/* returns: -1 on error, 1 on status change, 0 on no change */
int session_make_status(Session *s, gboolean send_presence){
Resource *r;

	r=session_get_cur_resource(s);
	return session_do_make_status(s,r,send_presence);
}


static int session_try_login(Session *s){
struct gg_login_params login_params;
GgServer *serv;
int r;

	g_warning(N_("Trying to log in on server %u"),
			g_list_position(gg_servers, s->current_server));

	if (s->timeout_func) g_source_remove(s->timeout_func);
	if (s->ggs) {
		gg_free_session(s->ggs);
		s->ggs=NULL;
	}
	session_remove_g_source(s);

	memset(&login_params,0,sizeof(login_params));
	login_params.uin=s->user->uin;
	login_params.password=from_utf8(s->user->password);
	login_params.async=1;
	login_params.last_sysmsg=s->user->last_sys_msg;
	login_params.protocol_version=GG_DEFAULT_PROTOCOL_VERSION;

	r=session_make_status(s, FALSE);
	if (r!=-1){
		debug(L_("Setting gg status to %i (%s)"),s->gg_status,s->gg_status_descr?s->gg_status_descr:"");
		login_params.status=s->gg_status;
		if (s->gg_status_descr) login_params.status_descr=s->gg_status_descr;
	}

	serv=(GgServer*)s->current_server->data;
	if(serv->port!=1){
		login_params.server_addr=serv->addr.s_addr;
		login_params.server_port=serv->port;
	}
#ifdef __GG_LIBGADU_HAVE_OPENSSL
	debug(N_("Turning TLS %s"), serv->tls?"on":"off");
	login_params.tls=serv->tls;
#endif

	s->ggs=gg_login(&login_params);
	if (!s->ggs){
		g_free(s);
		return 1;
	}
	session_setup_g_source(s);

	s->timeout_func=g_timeout_add(conn_timeout*1000,session_timeout,s);
	return FALSE;
}

Session * session_create(User *user,const char *jid,const char *req_id,
		const xmlnode query,struct stream_s *stream,int delay_login){
Session *s;
char *njid;

	g_message(L_("Creating session for '%s'"),jid);
	njid=jid_normalized(jid,0);
	if (njid==NULL){
		g_message(L_("Bad JID: '%s'"),jid);
		return NULL;
	}

	g_assert(user!=NULL);

	if (user->deleted){
		g_message(L_("User deleted: '%s'"),user->jid);
		return NULL;
	}
	
	s=g_new0(Session,1);
	s->user=user;
	user_ref(user);
	s->gg_status=-1;
	s->jid=g_strdup(jid);
	if (req_id) s->req_id=g_strdup(req_id);
	s->query=xmlnode_dup(query);
	s->current_server=g_list_first(gg_servers);
	s->g_pollfd.fd=-1;

	if (!delay_login && session_try_login(s)) return NULL;

	s->s=stream;

	g_assert(sessions_jid!=NULL);
	g_hash_table_insert(sessions_jid,(gpointer)njid,(gpointer)s);
	return s;
}

Session *session_get_by_jid(const char *jid,Stream *stream,int delay_login){
Session *s;
User *u;
char *njid;
GList *it;

	g_assert(sessions_jid!=NULL);
	debug(L_("Looking up session for '%s'"),jid);
	njid=jid_normalized(jid,0);
	if (njid==NULL){
		g_message(L_("Bad JID: '%s'"),jid);
		return NULL;
	}

	debug(L_("Using '%s' as key"),njid);
	s=(Session *)g_hash_table_lookup(sessions_jid,(gpointer)njid);
	g_free(njid);
	if (s) return s;
	debug(L_("Session not found"));
	if (!stream) return NULL;
	u=user_get_by_jid(jid);
	if (!u) return NULL;

	debug(L_("User loaded, processing his subscriptions."));
	for(it=u->contacts;it;it=it->next){
		Contact *c;
		char *c_jid;
		c=(Contact *)it->data;
		if (c->subscribe == SUB_UNDEFINED) {
			c_jid=jid_build(c->uin);
			presence_send_subscribe(stream,c_jid,u->jid);
			g_free(c_jid);
		}
		else if (c->subscribe == SUB_FROM || c->subscribe == SUB_BOTH){
			c_jid=jid_build(c->uin);
			presence_send_probe(stream,c_jid,u->jid);
			g_free(c_jid);
		}
	}
	debug(L_("Creating new session"));
	return session_create(u,jid,NULL,NULL,stream,delay_login);
}

int session_send_status(Session *s){
int r;

	g_assert(s!=NULL);
	if (s->ggs==NULL) return -1;
	r=session_make_status(s, s->connected);
	if (r==-1) return -1;
	if (r==0) return 0;
	debug(L_("Changing gg status to %i"),s->gg_status);
	if (s->gg_status_descr!=NULL)
		gg_change_status_descr(s->ggs,s->gg_status,s->gg_status_descr);
	else
		gg_change_status(s->ggs,s->gg_status);
	return 0;
}

static void resource_remove(Resource *r,int kill_session){
Session *s=r->session;

	debug(L_("Removing resource %s of %s"),r->name,s->jid);
	if (r->name) g_free(r->name);
	if (r->show) g_free(r->show);
	if (r->status) g_free(r->status);
	s->resources=g_list_remove(s->resources,r);
	if (r->disconnect_delay_func){
		g_source_remove(r->disconnect_delay_func);
		r->disconnect_delay_func=0;
	}
	g_free(r);
	if (!s->resources && kill_session){
		session_remove(s);
		return;
	}
	session_send_status(s);
}

gboolean delayed_disconnect(gpointer data){
Resource *r=(Resource *)data;

	g_assert(r!=NULL);

	debug(L_("Delayed removal of resource %s of %s"),r->name,r->session->jid);
	r->disconnect_delay_func=0;
	resource_remove(r,1);
	return FALSE;
}


int session_set_status(Session *s,const char *resource,int available,
			const char *show,const char *status,int priority){
Resource *r;
GList *it;
int num_resources=0;

	r=NULL;
	for(it=g_list_first(s->resources);it;it=it->next){
		Resource *r1=(Resource *)it->data;
		if ( (!r1->name && !resource) || (r1->name && resource && !strcmp(r1->name,resource)) ){
			r=r1;
		}
		num_resources++;
	}

	if (!available){
		if (!r){
			g_warning(N_("Unknown resource %s of %s"),resource?resource:"NULL",s->jid);
			return 0;
		}
		if (disconnect_delay>0 && r->available && num_resources==1){
			r->available=0;
			debug(L_("Delaying removal of resource %s of %s"),resource?resource:"NULL",s->jid);
			r->disconnect_delay_func=g_timeout_add(disconnect_delay*1000,delayed_disconnect,r);
		}
		else resource_remove(r,1);
		return 0;
	}
	else{
		if ( r ){
			if (r->disconnect_delay_func) g_source_remove(r->disconnect_delay_func);
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
			debug(L_("New resource %s of %s"),resource?resource:"NULL",s->jid);
			r=g_new0(Resource,1);
			if (resource) r->name=g_strdup(resource);
			r->session=s;
			s->resources=g_list_append(s->resources,r);
		}
		r->available=available;
		if (show) r->show=g_strdup(show);
		if (status) r->status=g_strndup(status, 70);
		if (priority>=0) r->priority=priority;
		session_send_status(s);
	}

	if (s->connected) session_send_status(s);
	else return session_try_login(s);

	return 0;
}

static int compute_notify_type(Contact *c){
int notify_type=0;

	if (c->blocked) return GG_USER_BLOCKED;
	if (c->got_online) notify_type|=GG_USER_NORMAL-GG_USER_OFFLINE;
	if (c->got_probe || c->subscribe==SUB_TO || c->subscribe==SUB_BOTH) notify_type|=GG_USER_OFFLINE;

	return notify_type;
}

int session_update_contact(Session *s,Contact *c){
int new_notify_type=0;
int del,add;

	if (!s->connected) return 0;

	new_notify_type=compute_notify_type(c);

	del=c->gg_notify_type&(~new_notify_type);
	add=new_notify_type&(~c->gg_notify_type);
	
	if (del) gg_remove_notify_ex(s->ggs,c->uin,del);
	if (add) gg_add_notify_ex(s->ggs,c->uin,add);

	c->gg_notify_type=new_notify_type;
	
	return 0;
}

int session_send_notify(Session *s){
GList *it;
uin_t *userlist;
char *types;
int userlist_len;
int i;

	g_assert(s!=NULL);
	userlist_len=g_list_length(s->user->contacts);
	userlist=g_new(uin_t,userlist_len+1);
	types=g_new(char,userlist_len+1);

	i=0;
	for(it=g_list_first(s->user->contacts);it;it=it->next){
		Contact *c=(Contact *)it->data;
		userlist[i]=c->uin;
		c->gg_notify_type=compute_notify_type(c);
		types[i]=c->gg_notify_type;
		i++;	
	}

	userlist[i]=0;
	types[i]=0;

	debug("gg_notify_ex(%p,%p,%p,%i)",s->ggs,userlist,types,userlist_len);
	gg_notify_ex(s->ggs,userlist,types,userlist_len);

	g_free(userlist);
	g_free(types);
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
			gg_messages_out++;
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
	njid=jid_normalized(s->jid,0);
	g_message(L_("%sSession: %p"),space,s);
	g_message("%sJID: %s",space,s->jid);
	g_message(L_("%sUser:"),space);
	user_print(s->user,indent+1);
	g_message(L_("%sResources:"),space);
	cr=session_get_cur_resource(s);
	for(it=g_list_first(s->resources);it;it=it->next){
		r=(Resource *)it->data;
		g_message(L_("%sResource: %p%s"),space1,r,(r==cr)?N_(" (current)"):"");
		if (njid && r->name) g_message("%sJID: %s/%s",space1,njid,r->name);
		else g_message("%sJID: %s",space1,s->jid);
		g_message(L_("%sPriority: %i"),space1,r->priority);
		g_message(L_("%sAvailable: %i"),space1,r->available);
		if (r->show)
			g_message(L_("%sShow: %s"),space1,r->show);
		if (r->status)
			g_message(L_("%sStatus: %s"),space1,r->status);
	}
	g_message(L_("%sGG session: %p"),space,s->ggs);
	g_message(L_("%sGSource: %p"),space,s->g_source);
	g_message(L_("%sStream: %p"),space,s->s);
	g_message(L_("%sConnected: %i"),space,s->connected);
	g_message(L_("%sRequest id: %s"),space,s->req_id?s->req_id:"(null)");
	g_message(L_("%sRequest query: %s"),space,s->query?xmlnode2str(s->query):"(null)");
	g_message(L_("%sWaiting for ping: %i"),space,(int)s->waiting_for_pong);
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

char * session_get_info_string(const Session *sess){
char *str;
GgServer *server;
char *using_tls="";

	if (sess->current_server){
		server=(GgServer *)sess->current_server->data;
#ifdef __GG_LIBGADU_HAVE_OPENSSL
		if (server->tls) using_tls=_(" with TLS");
#endif
		if (!server || server->port==1){
			if (sess->connected){
				str=g_strdup_printf(_("%s (Connected via hub to %s:%i%s)"),
						sess->jid,
						inet_ntoa(*(struct in_addr*)&sess->ggs->server_addr),
						sess->ggs->port,
						using_tls);
			}
			else{
				str=g_strdup_printf(_("%s (Connecting via hub%s)"),
						sess->jid,
						using_tls);
			}
		}
		else
			str=g_strdup_printf(_("%s (%s to %s:%u%s)"),
					sess->jid,
					sess->connected?_("Connected"):_("Connecting"),
					inet_ntoa(server->addr),
					server->port,
					using_tls);
	}
	else
		str=g_strdup_printf("%s (%s)",
				sess->jid,
				sess->connected?_("Connected"):_("Connecting"));

	return str;
}
