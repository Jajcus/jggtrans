#ifndef jabber_h
#define jabber_h

#include "stream.h"
	
int jabber_init();
int jabber_done();
int jabber_iter();

enum jabber_state_e{
	JS_NONE,
	JS_HANDSHAKE,
	JS_CONNECTED	
};
extern enum jabber_state_e jabber_state;


extern const char *my_name;
extern const char *instructions;

#endif
