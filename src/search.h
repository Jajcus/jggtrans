#ifndef search_h
#define search_h

struct request_s;
struct stream_s;

int search_error(struct request_s *r);
int search_done(struct request_s *r);
void jabber_iq_get_search(struct stream_s *s,const char *from,const char *to,const char *id,xmlnode q);
void jabber_iq_set_search(struct stream_s *s,const char *from,const char *to,const char *id,xmlnode q);

int vcard_error(struct request_s *r);
int vcard_done(struct request_s *r);
void jabber_iq_get_user_vcard(struct stream_s *s,const char *from,const char * to,const char *id,xmlnode q);

#endif
