/* $Id: users.h,v 1.10 2002/02/06 17:23:37 jajcus Exp $ */

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

#ifndef users_h
#define users_h

#include <libgadu.h>

typedef struct contact_s{
	uin_t uin;
	int status;
	GTime last_update;
}Contact;

typedef struct user_s{
	uin_t uin;
	char * jid;
	char * password;
	int last_sys_msg;

	int confirmed;
	GList *contacts;
}User;

extern GHashTable *users_jid;

User *user_create(const char *jid,uin_t uin,const char * password);
int user_remove(User *u);
int user_delete(User *u);

int user_save(User *u);

User *user_get_by_jid(const char *jid);

int user_subscribe(User *u,uin_t uin);
int user_unsubscribe(User *u,uin_t uin);

int user_sys_msg_received(User *u,int nr);

int user_set_contact_status(User *u,int status,unsigned int uin);

void user_print(User *u,int indent);
int users_probe_all();
int users_init();
int users_done();

#endif
