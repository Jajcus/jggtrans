#ifndef users_h
#define users_h

#include <libgg.h>

typedef struct contact_s{
	uin_t uin;
	int status;
	GTime last_update;
}Contact;

typedef struct user_s{
	uin_t uin;
	char * jid;
	char * password;

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

int user_set_contact_status(User *u,int status,unsigned int uin);

int users_probe_all();
int users_init();
int users_done();

#endif
