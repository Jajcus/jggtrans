#ifndef presence_h
#define presence_h

#include "jabber.h"

struct stream_s;

int presence_send_subscribe(struct stream_s *stream,const char *from,
		const char *to);
int presence_send_unsubscribe(struct stream_s *stream,const char *from,
		const char *to);
int presence_send_subscribed(struct stream_s *stream,const char *from,
		const char *to);
int presence_send_unsubscribed(struct stream_s *stream,const char *from,
		const char *to);
int presence_send(struct stream_s *stream,const char *from,
		const char *to,int available,const char *show,
		const char *status);
int presence_send_probe(struct stream_s *stream,const char *to);

int jabber_presence(struct stream_s *stream,xmlnode tag);


#endif
