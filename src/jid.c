#include <stdio.h>
#include "jid.h"
#include "jabber.h"
#include "ctype.h"
#include <glib.h>

int jid_ok(const char *jid){
const char *p;
	
	if (!jid) return 0;
	if (!g_strcasecmp(jid,my_name)) return 1;
	
	for(p=jid;*p!='@';p++){
		if (!isdigit(*p)) return 0;
	}

	if (g_strcasecmp(p+1,my_name)) return 0;
	
	return 1;
}

int jid_has_uin(const char *jid){
char *p;

	p=strchr(jid,'@');
	if (p) return 1;
	return 0;
}

int jid_get_uin(const char *jid){

	return atoi(jid);
}

