#ifndef sessions_h
#define sessions_h

#include "libxode.h"
#include "users.h"

struct request_s;
typedef int (*request_cb)(struct request_s *req,void *event);
				/* event: struct gg_event or int z wynikiem gg_search_watchfd */

typedef struct request_s{
	int id;  /* ID if user request (<iq/>) */
	xmlnode query; /* The query */
	struct session_s *ses;
	int gg_session_type;
	struct gg_http* gghttp; 
	int event_type;
	struct fd_handler_s *fdh;
	request_cb callback;
}Request;

typedef struct sesion_s{
	char * jid; /* users JID, with resource */
	User *user;
	GList *requests;
	struct gg_session *ggs;
	struct fd_handler_s *fdh;
	struct stream_s *s; /* server stream */

	int connected;
	int available;
	char *show;
	char *status;
	
	char *req_id;  /* ID if user registration request (<iq/>) */
	xmlnode query; /* The query */
}Session;

GHashTable *sessions_jid;
GHashTable *sessions_uin;

Session *session_create(User *user,const char *jid,const char *req_id,const xmlnode query,struct stream_s *stream);
int session_remove(Session *s);
	
int session_set_status(Session *s,int available,const char *show,const char *status);
int session_send_status(Session *s);

int session_subscribe(Session *s,uin_t uin);

/*
 * Finds session associated with JID. 
 * If none exists and stream is given, new session is created
 */
Session *session_get_by_jid(const char *jid,struct stream_s *stream);

int sessions_init();

#endif
