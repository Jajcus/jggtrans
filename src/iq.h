#ifndef iq_h
#define iq_h

struct stream_s;
void jabber_iq(struct stream_s *s,xmlnode x);
void jabber_iq_send_error(struct stream_s *s,const char *from,const char *id,char *string);
void jabber_iq_send_result(struct stream_s *s,const char *from,const char *id,xmlnode content);

#endif
