/* $Id: acl.c,v 1.2 2003/04/16 09:41:25 jajcus Exp $ */

/*
 *  (C) Copyright 2002 Jacek Konieczny <jajcus@pld.org.pl>
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
#include "conf.h"
#include "jid.h"
#include "debug.h"
#include <fnmatch.h>

struct acl_s {
	int allow;
	char *what;
	char *who;
};

static GList *acl=NULL;

int acl_init() {
xmlnode node;
char *str,*what,*who;
int allow=0;
struct acl_s *acl_e;
	

	node=xmlnode_get_tag(config,"acl");
	if (!node) return 0;
	
	for(node=xmlnode_get_firstchild(node);node;node=xmlnode_get_nextsibling(node)){
		str=xmlnode_get_name(node);
		if (!str) continue;
		if (!strcmp(str,"allow")) allow=1;
		else if (!strcmp(str,"deny")) allow=0;
		else g_error(N_("Invalid tag <%s/> in config section <acl/>"),str);
		what=xmlnode_get_attrib(node,"what");
		if (!what || what[0]=='\000' || !strcmp(what,"*")) what=NULL;
		who=xmlnode_get_attrib(node,"who");
		if (!who || who[0]=='\000' || !strcmp(who,"*")) who=NULL;
		acl_e=g_new0(struct acl_s,1);
		if (what) acl_e->what=g_strdup(what);
		if (who) acl_e->who=g_strdup(who);
		acl_e->allow=allow;
		acl=g_list_append(acl,acl_e);
	}
	return 0;
}

void acl_done() {
GList *it;
struct acl_s *acl_e;
	
	for(it=g_list_first(acl);it;it=g_list_next(it)){
		acl_e=(struct acl_s*)it->data;
		g_free(acl_e->who);
		g_free(acl_e->what);
		g_free(acl_e);
	}
	g_list_free(acl);
}

int acl_is_allowed(const char *from,xmlnode node){
GList *it;
struct acl_s *acl_e;
char *jid;
xmlnode x;
int result=0;

	x=xmlnode_new_tag("x");
	xmlnode_insert_tag_node(x,node);
	if (from) jid=jid_normalized(from);
	else jid=NULL;
	for(it=g_list_first(acl);it;it=g_list_next(it)){
		acl_e=(struct acl_s*)it->data;
		if (acl_e->who && jid) {
			if (fnmatch(acl_e->who,jid,0)) continue; /* no match */
		}
		if (acl_e && !xmlnode_get_tag(x,acl_e->what)) continue; /* no match */
		result=acl_e->allow;
		break;
	}
	if (it==NULL) result=1;
	xmlnode_free(x);
	g_free(jid);
	if (result) debug("Allowed");
	else debug("Denied");
	return result;
}
