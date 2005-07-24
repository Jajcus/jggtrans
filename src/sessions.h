/* $Id: sessions.h,v 1.29 2004/03/16 19:30:25 mmazur Exp $ */

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

#ifndef sessions_h
#define sessions_h

#include "users.h"

typedef struct resource_s{
	char *name;
	int priority;
	int available;
	char *show;
	char *status;
	struct session_s *session;
	guint disconnect_delay_func;
}Resource;

typedef struct session_s{
	User *user;

	struct gg_session *ggs; /* GG session */
	int gg_status; /* status set at GG server */
	char *gg_status_descr;

	GSource *g_source;
	GPollFD g_pollfd;

	int connected;
	GList *current_server;

	char *jid;		/* users JID, with resource */
	struct stream_s *s;	/* Jabber stream */
	GList *resources;
	
	char *req_id;  /* ID if user registration request (<iq/>) */

	xmlnode query; /* The query */
	gg_pubdir50_t pubdir_change; /* Info for public directory change reqested on registration */
	int get_roster;
	guint ping_timeout_func;
	guint timeout_func;
	GTimer *ping_timer;
	gboolean waiting_for_pong;
}Session;

typedef struct gg_server_s {
	struct in_addr addr;
	int port;
	int tls;
}GgServer;

extern GHashTable *sessions_jid;

Session *session_create(User *user,const char *jid,const char *req_id,
		const xmlnode query,struct stream_s *stream,int delay_login);
int session_remove(Session *s);

/* available==-1 means invisible */
int session_set_status(Session *s,const char *resource,int available,
			const char *show,const char *status,int priority);
int session_send_status(Session *s);

int session_update_contact(Session *s, Contact *c);

int session_send_message(Session *s,uin_t uin,int chat,const char *body);
int session_send_notify(Session *s);

/* Finds resource representing current state,
 * probably the one with highest priority */
Resource *session_get_cur_resource(Session *s);

/*
 * Finds session associated with JID.
 * If none exists and stream is given, new session is created
 * If delay_login != 0, than the session is not logged in, when created, but on
 * the first status change
 */
Session *session_get_by_jid(const char *jid,struct stream_s *stream,int delay_login);

void session_print(Session *s,int indent);
void sessions_print_all(int indent);
char * session_get_info_string(const Session *sess);

int sessions_init();
int sessions_done();

#endif
