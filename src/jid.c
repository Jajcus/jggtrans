#include <stdio.h>
#include "jid.h"
#include "jabber.h"
#include "ctype.h"
#include <glib.h>
#include <assert.h>

int jid_is_my(const char *jid){
int i,at,slash;
int just_digits;

	if (!jid) return 0;
	
	just_digits=1;
	at=-1;
	slash=-1;
	/* split into parts, check for numeric username */
	for(i=0;jid[i];i++){
		if (jid[i]=='/' && slash<0) slash=i;
		else if (jid[i]=='@')
			if (!just_digits){
				fprintf(stderr,"\nNon-digits before '@' in jid\n");
				return 0;
			}
			else at=i;
		else if (!isdigit(jid[i]) && just_digits) just_digits=0; 
	}

	if (slash>=0 && slash<at){
		fprintf(stderr,"\nslash<at (%i<%i)\n",slash,at);
		return 0;
	}
	
	/* check hostname */ 
	if (slash<0){
		if ( g_strcasecmp(jid+at+1,my_name) ){
			fprintf(stderr,"\nBad hostname in JID: %s\n",jid+at+1);
			return 0;
		}
	} else { 
		if ( slash-at-1!=strlen(my_name) ) {
			fprintf(stderr,"\nBad hostname len in JID: %i instead of %i\n",slash-at-1,strlen(my_name));
			return 0;
		}

		if ( g_strncasecmp(jid+at+1,my_name,slash-at-1) )  {
			fprintf(stderr,"\nBad hostname in JID: %s[0:%i]\n",jid+at+1,slash-at-2);
			return 0;
		}
	}
		
	return 1;
}

int jid_is_me(const char *jid){
int i,slash;

	if (!jid) return 0;
	
	slash=-1;
	
	for(i=0;jid[i];i++){
		if (jid[i]=='/' && slash<0) slash=i;
		else if (jid[i]=='@') return 0;
	}

	if (slash<0){
		if ( g_strcasecmp(jid,my_name) ) return 0;
	} else if ( slash!=strlen(my_name) || g_strncasecmp(jid,my_name,slash) ) return 0;

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

char * jid_my_registered(){
	
	return g_strdup_printf("%s/registered",my_name);
}

char * jid_build(long unsigned int uin){
	return g_strdup_printf("%lu@%s",uin,my_name);
}

char * jid_normalized(const char *jid){
int i,slash;
char *r;

	assert(jid!=NULL);
	if (!jid) return 0;
	
	slash=-1;
	/* split into parts */
	for(i=0;jid[i];i++){
		if (jid[i]=='/' && slash<0) slash=i;
	}

	assert(slash!=0);
	
	if (slash<0) r=g_strdup(jid);
	else r=g_strndup(jid,slash);

	g_strdown(r);

	return r; 
}
