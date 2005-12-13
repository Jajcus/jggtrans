/* $Id: users.h,v 1.20 2003/06/27 14:06:51 jajcus Exp $ */

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

#ifndef users_h
#define users_h

#include <libgadu.h>

#define USER_FILE_FORMAT_VERSION 0x02020001U   /* first change after 2.2.0 */

typedef enum subscription_type_e {
	SUB_UNDEFINED=0,
	SUB_NONE,
	SUB_FROM,
	SUB_TO,
	SUB_BOTH
}SubscriptionType;

typedef struct contact_s{
	uin_t uin;
	gboolean ignored;
	gboolean blocked;
	SubscriptionType subscribe;

	gboolean got_online;
	gboolean got_probe;

	int gg_notify_type;
	
	int status;
	GTime last_update;
	char *status_desc;
	uint32_t ip;
	uint16_t port;
	uint32_t version;
}Contact;

typedef struct user_s{
	uin_t uin;
	char * jid;
	char * password;
	int last_sys_msg;
	gboolean friends_only;
	gboolean invisible;
	gboolean ignore_unknown;
	char *locale;
	char * status;

	int confirmed;
	int refcount;
	gboolean deleted;
	GList *contacts;
}User;

extern char *default_user_locale;
extern GHashTable *users_jid;

User *user_create(const char *jid,uin_t uin,const char * password);
int user_ref(User *u);
int user_unref(User *u);
int user_delete(User *u);

int user_save(User *u);

User *user_get_by_jid(const char *jid);

Contact * user_get_contact(User *u, uin_t uin, gboolean create);

/* check if the contact is still needed, and if not, then remove it */
int user_check_contact(User *u, Contact *c); 

int user_sys_msg_received(User *u,int nr);

int user_set_contact_status(User *u,int status,unsigned int uin,char *desc,
				int more,uint32_t ip,uint16_t port,uint32_t version);
int user_get_contact_status(User *u,unsigned int uin);

void user_load_locale(User *u);

void user_print(User *u,int indent);
int users_probe_all();
int users_count();
int users_init();
int users_done();

#endif
