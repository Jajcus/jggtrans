#ifndef jabber_h
#define jabber_h

#include "stream.h"
#include <glib.h>
	
int jabber_init();
int jabber_done();
int jabber_iter();

struct stream_s * jabber_stream();

enum jabber_state_e{
	JS_NONE,
	JS_HANDSHAKE,
	JS_CONNECTED	
};
extern enum jabber_state_e jabber_state;


extern const char *my_name;
extern const char *register_instructions;
extern const char *search_instructions;

#endif
