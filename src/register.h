#ifndef register_h
#define register_h

void jabber_iq_get_register(Stream *s,const char *from,const char *id,xmlnode q);
void jabber_iq_set_register(Stream *s,const char *from,const char *id,xmlnode q);

#endif
