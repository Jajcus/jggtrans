#ifndef sessions_h
#define sessions_h

#include "users.h"

typedef struct sesion_s{
	User *user;
	
	struct gg_session *ggs; /* GG session */
	
	GIOChannel *ioch; /* GG IO Channel */
	guint io_watch;

	
	int connected;
	
	char *jid; 		/* users JID, with resource */
	struct stream_s *s; 	/* Jabber stream */
	int available;
	char *show;
	char *status;
	
	char *req_id;  /* ID if user registration request (<iq/>) */
	xmlnode query; /* The query */

	guint ping_timeout_func;
	guint timeout_func;
	GTimer *ping_timer;
}Session;

extern GHashTable *sessions_jid;

Session *session_create(User *user,const char *jid,const char *req_id,const xmlnode query,struct stream_s *stream);
int session_remove(Session *s);
	
int session_set_status(Session *s,int available,const char *show,const char *status);
int session_send_status(Session *s);

int session_subscribe(Session *s,uin_t uin);
int session_unsubscribe(Session *s,uin_t uin);

int session_send_message(Session *s,uin_t uin,int chat,const char *body);

/*
 * Finds session associated with JID. 
 * If none exists and stream is given, new session is created
 */
Session *session_get_by_jid(const char *jid,struct stream_s *stream);

int sessions_init();
int sessions_done();
int sessions_iter();

#endif
