/* $Id: jid.c,v 1.14 2004/04/13 17:44:07 jajcus Exp $ */

/*
 *  (C) Copyright 2002-2005 Jacek Konieczny [jajcus(a)jajcus,net]
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ggtrans.h"
#include <stdio.h>
#include <stringprep.h>
#include <idna.h>
#include "jid.h"
#include "jabber.h"
#include "ctype.h"
#include "debug.h"

int jid_is_my(const char *jid){
int i,at,slash;
int just_digits;
int ret;
char *inbuf,*outbuf;

	if (!jid) return 0;

	just_digits=1;
	at=-1;
	slash=-1;
	/* split into parts, check for numeric username */
	for(i=0;jid[i];i++){
		if (jid[i]=='/' && slash<0) slash=i;
		else if (jid[i]=='@')
			if (!just_digits){
				debug(L_("Non-digits before '@' in jid: %s"),jid);
				return 0;
			}
			else at=i;
		else if (!isdigit(jid[i]) && just_digits) just_digits=0;
	}

	if (slash>=0 && slash<at){
		g_warning(N_("slash<at (%i<%i) in %s"),slash,at,jid);
		return 0;
	}

	if ((slash<0 && strlen(jid+at+1)>1023) || slash-at-1>1023){
		g_warning(N_("JID domain too long"));
		return 0;
	}

	if (slash<0)
		inbuf=g_strdup(jid+at+1);
	else
		inbuf=g_strndup(jid+at+1,slash-at-1);

	ret=idna_to_unicode_8z8z(inbuf,&outbuf,IDNA_ALLOW_UNASSIGNED);

	if (ret!=IDNA_SUCCESS){
		g_warning(N_("JID domain doesn't pass ToUnicode"));
		ret=0;
	}
	else{
		ret=!strcmp(outbuf,my_name);
		if (!ret){
			debug(L_("Bad hostname (%s) in JID: %s"),outbuf,jid);
		}
	}

	g_free(inbuf);
	g_free(outbuf);

	return ret;
}

int jid_is_me(const char *jid){
int i,slash;
int ret;
char *inbuf,*outbuf;

	if (!jid) return 0;

	slash=-1;
	for(i=0;jid[i];i++){
		if (jid[i]=='/' && slash<0) slash=i;
		else if (jid[i]=='@') return 0;
	}

	if ((slash<0 && strlen(jid)>1023) || slash>1023){
		g_warning(N_("JID domain too long"));
		return 0;
	}

	if (slash<0)
		inbuf=g_strdup(jid);
	else
		inbuf=g_strndup(jid,slash);

	ret=idna_to_unicode_8z8z(inbuf,&outbuf,IDNA_ALLOW_UNASSIGNED);

	if (ret!=IDNA_SUCCESS){
		g_warning(N_("JID domain doesn't pass ToUnicode"));
		ret=0;
	}
	else{
		ret=!strcmp(outbuf,my_name);
	}

	g_free(inbuf);
	g_free(outbuf);

	return ret;
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

const char *jid_get_resource(const char *jid){
const char *p;

	p=strchr(jid,'/');
	if (p) p++;
	return p;
}

char * jid_my_registered(){

	return g_strdup_printf("%s/registered",my_name);
}

char * jid_build(long unsigned int uin){
	return g_strdup_printf("%lu@%s",uin,my_name);
}

char * jid_build_full(long unsigned int uin){
	return g_strdup_printf("%lu@%s/GG",uin,my_name);
}

char * jid_normalized(const char *jid,int full){
int i,at,slash,ret;
char node[1024],domain[1024],resource[1024];
char *domainbuf;

	if (!jid) return NULL;

	slash=-1;
	at=-1;
	/* split into parts */
	for(i=0;jid[i];i++){
		if (jid[i]=='@' && at<0) at=i;
		if (jid[i]=='/' && slash<0) slash=i;
	}

	if (slash>=0 && slash<at){
		g_warning(N_("slash<at (%i<%i) in %s"),slash,at,jid);
		return NULL;
	}
	if (slash==at+1){
		g_warning(N_("empty domain in %s"),jid);
		return NULL;
	}

	/* node */
	if (at>0){
		if (at>1023){
			g_warning(N_("node too long in %s"),jid);
			return NULL;
		}
		memcpy(node,jid,at);
		node[at]='\000';
	}
	else node[0]='\000';

	/* domain */
	if (slash>0){
		if (slash-at>1024){
			g_warning(N_("domain too long in %s"),jid);
			return NULL;
		}
		memcpy(domain,jid+at+1,slash-at-1);
		domain[slash-at-1]='\000';
	}
	else{
		if (strlen(jid+at+1)>1023){
			g_warning(N_("domain too long in %s"),jid);
			return NULL;
		}
		strcpy(domain,jid+at+1);
	}

	/* resource */
	if (slash>0){
		if (strlen(jid+slash+1)>1023){
			g_warning(N_("resource too long in %s"),jid);
			return NULL;
		}
		strcpy(resource,jid+slash+1);
	}
	else resource[0]='\000';

	if (node[0]){
		ret=stringprep_xmpp_nodeprep(node,1024);
		if (ret!=0){
			g_warning(N_("bad node: %s"),node);
			return NULL;
		}
	}
	if (resource[0]){
		ret=stringprep_xmpp_resourceprep(resource,1024);
		if (ret!=0){
			g_warning(N_("bad node: %s"),resource);
			return NULL;
		}
	}
	ret=idna_to_unicode_8z8z(domain,&domainbuf,IDNA_ALLOW_UNASSIGNED);
	if (ret!=IDNA_SUCCESS){
		g_warning(N_("bad domain: %s"),domain);
		return NULL;
	}
	strcpy(domain,domainbuf);
	g_free(domainbuf);

	if (!full || !resource[0]){
		if (node[0])
			return g_strconcat(node,"@",domain,NULL);
		else
			return g_strdup(domain);
	}
	else{
		if (node[0])
			return g_strconcat(node,"@",domain,'/',resource,NULL);
		else
			return g_strconcat(domain,'/',resource,NULL);
	}
}
