#ifndef users_h
#define users_h

#include <libgg.h>
#include <glib.h>

typedef struct contact_s{
	uin_t uin;
	int status;
}Contact;

typedef struct user_s{
	uin_t uin;
	char * jid;
	char * name;
	char * password;
	char * email;
	int confirmed;
	GList *contacts;
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

int user_set_contact_status(User *u,int status,unsigned int uin);

int users_init();
int users_done();

#endif
