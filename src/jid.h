#ifndef jid_h
#define jid_h

int jid_is_my(const char *jid);
int jid_is_me(const char *jid);
int jid_has_uin(const char *jid);
int jid_get_uin(const char *jid);


/* Functions below return strings which must be freed */

/* returns uncapitalized user@host part of given jid */
char * jid_normalized(const char *jid);
char * jid_my_registered();
char * jid_build(long unsigned int uin);

#endif
