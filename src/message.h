#ifndef message_h
#define message_h

#include "jabber.h"

struct stream_s;

int message_send(struct stream_s *stream,const char *from,
		const char *to,int chat,const char *message);

int message_error(struct stream_s *stream,const char *from,
		const char *to,const char *body,int code);

int jabber_message(struct stream_s *stream,xmlnode tag);


#endif
