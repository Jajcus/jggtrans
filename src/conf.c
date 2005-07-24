/* $Id: conf.c,v 1.6 2003/01/22 07:53:01 jajcus Exp $ */

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
#include "jabber.h"
#include "conf.h"
#include <ctype.h>

xmlnode config;

char *config_load_string(const char *tag){
xmlnode node;
char *data;

	node=xmlnode_get_tag(config,tag);
	if (node==NULL) return NULL;
	data=xmlnode_get_data(node);
	if (data==NULL) return NULL;
	return g_strstrip(data);
}

char *config_load_formatted_string(const char *tag){
xmlnode node,child;
char *out,*tmp,*add,*name;
int t;
int i,j,sp;

	node=xmlnode_get_tag(config,tag);
	if (!node) return NULL;
	out=g_strdup("");
	child=xmlnode_get_firstchild(node);
	while(child){
		t=xmlnode_get_type(child);
		add=NULL;
		switch(t){
			case NTYPE_TAG:
				name=xmlnode_get_name(child);
				if (!g_strcasecmp(name,"p")){
					if (out[0]) add=g_strdup("\n");
				}
				else if (!g_strcasecmp(name,"br"))
					add=g_strdup("\n");
				else g_warning("Unknown formatting '%s' in '%s'",
						xmlnode2str(child),xmlnode2str(node));
				break;
			case NTYPE_CDATA:
				tmp=xmlnode_get_data(child);
				if (tmp==NULL) break;
				add=g_new(char,strlen(tmp)+1);
				sp=1;
				for(i=j=0;tmp[i];i++){
					if (!isspace(tmp[i])){
						sp=0;
						add[j++]=tmp[i];
					}
					else if (!sp){
						add[j++]=' ';
						sp=1;
					}
				}
				if (j && sp) j--;
				add[j]=0;
				break;
			case NTYPE_ATTRIB:
				g_error("Unexpected attribute");
				break;
			default:
				g_error("Unknown node type: %i",t);
				break;
		}
		if (add){
			tmp=out;
			out=g_strconcat(out,add,NULL);
			g_free(tmp);
			g_free(add);
		}
		child=xmlnode_get_nextsibling(child);
	}

	return out;
}

int config_load_int(const char *tag,int defval){
xmlnode node;
char *data;

	node=xmlnode_get_tag(config,tag);
	if (node==NULL) return defval;
	data=xmlnode_get_data(node);
	if (data==NULL) return defval;
	return atoi(g_strchug(data));
}
