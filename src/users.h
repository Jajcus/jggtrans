#ifndef users_h
#define users_h

#include <libgg.h>
#include <glib.h>

typedef struct user_s{
	uin_t uin;
	char * jid;
	char * name;
	char * password;
	char * email;
	int confirmed;
	uin_t *userlist;
	int userlist_len;
}User;

extern GHashTable *users_jid;
extern GHashTable *users_uin;

User *user_add(const char *jid,uin_t uin,const char *name,const char * password,const char *email);
int user_delete(User *u);
int user_save(User *u);
User *user_get_by_uin(uin_t uin);
User *user_get_by_jid(const char *jid);

int user_subscribe(User *u,uin_t uin);
int user_unsubscribe(User *u,uin_t uin);

int users_init();

#endif
