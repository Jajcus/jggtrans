/* $Id: users.c,v 1.12 2002/02/02 19:49:54 jajcus Exp $ */

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

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "ggtrans.h"
#include "jabber.h"
#include "users.h"
#include "jid.h"
#include "presence.h"
#include "debug.h"
#include "conf.h"

GHashTable *users_jid=NULL;
static char *spool_dir;

int users_init(){
xmlnode node;
int r;

	spool_dir=config_load_string("spool");
	if (!spool_dir)
		g_error("No <spool/> defined in config file");

	r=chdir(spool_dir);
	if (r) g_error("Couldn't enter %s: %s",spool_dir,g_strerror(errno));
	
	users_jid=g_hash_table_new(g_str_hash,g_str_equal);
	if (!users_jid) return -1;
	return 0;
}

static int user_destroy(User *s);

static gboolean users_hash_func(gpointer key,gpointer value,gpointer udata){

	user_destroy((User *)value);
	g_free(key);
	return TRUE;
}

int users_done(){

	g_hash_table_foreach_remove(users_jid,users_hash_func,NULL);
	g_hash_table_destroy(users_jid);
	return 0;
}

int user_save(User *u){
FILE *f;
char *fn;
char *str;
char *njid;
int r;	
xmlnode xml,tag,userlist;

	g_assert(u!=NULL);
	str=strchr(u->jid,'/');
	g_assert(str==NULL);

	g_message("Saving user '%s'",u->jid);
	njid=jid_normalized(u->jid);
	fn=g_strdup_printf("%s.new",njid);
	f=fopen(fn,"w");
	if (!f){
		g_warning("Couldn't open '%s': %s",fn,g_strerror(errno));
		g_free(fn);
		g_free(njid);
		return -1;
	}
	xml=xmlnode_new_tag("user");
	tag=xmlnode_insert_tag(xml,"jid");
	xmlnode_insert_cdata(tag,u->jid,-1);
	tag=xmlnode_insert_tag(xml,"uin");
	str=g_strdup_printf("%lu",(unsigned long)u->uin);
	xmlnode_insert_cdata(tag,str,-1);
	g_free(str);
	tag=xmlnode_insert_tag(xml,"password");
	xmlnode_insert_cdata(tag,u->password,-1);

	if (u->last_sys_msg>0){
		tag=xmlnode_insert_tag(xml,"last_sys_msg");
		str=g_strdup_printf("%i",u->last_sys_msg);
		xmlnode_insert_cdata(tag,str,-1);
		g_free(str);
	}

	if (u->contacts){
		GList *it;
		Contact *c;
		
		userlist=xmlnode_insert_tag(xml,"userlist");
		for(it=g_list_first(u->contacts);it;it=it->next){
			c=(Contact *)it->data;
			tag=xmlnode_insert_tag(userlist,"uin");
			str=g_strdup_printf("%lu",(unsigned long)c->uin);
			xmlnode_insert_cdata(tag,str,-1);
			g_free(str);
		}
	}

	str=xmlnode2str(xml);
	r=fputs(str,f);
	if (r<0){
		g_warning("Couldn't save '%s': %s",u->jid,g_strerror(errno));
		fclose(f);
		unlink(fn);
		xmlnode_free(xml);
		g_free(fn);
		g_free(njid);
		return -1;
	}
	fclose(f);
	r=unlink(njid);
	if (r && errno!=ENOENT){
		g_warning("Couldn't unlink '%s': %s",njid,g_strerror(errno));
		xmlnode_free(xml);
		g_free(fn);
		g_free(njid);
		return -1;
	}
	
	r=rename(fn,njid);
	if (r){
		g_warning("Couldn't rename '%s' to '%s': %s",fn,u->jid,g_strerror(errno));
		xmlnode_free(xml);
		g_free(fn);
		g_free(njid);
		return -1;
	}
	
	xmlnode_free(xml);
	g_free(fn);
	g_free(njid);
	return 0;
}

User *user_load(const char *jid){
char *fn,*njid;
xmlnode xml,tag,t;
char *uin,*ujid,*name,*password,*email;
int last_sys_msg=0;
User *u;
GList *contacts;
char *p;

	uin=ujid=name=password=email=NULL;
	g_message("Loading user '%s'",jid);
	fn=jid_normalized(jid);
	errno=0;
	xml=xmlnode_file(fn);
	if (!xml){
		g_warning("Couldn't read or parse '%s': %s",fn,errno?g_strerror(errno):"XML parse error");
		g_free(fn);
		return NULL;
	}
	g_free(fn);
	tag=xmlnode_get_tag(xml,"jid");
	if (tag) ujid=xmlnode_get_data(tag);
	if (!ujid){
		g_warning("Couldn't find JID in %s's file",jid);
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"uin");
	if (tag) uin=xmlnode_get_data(tag);
	if (!uin){
		g_warning("Couldn't find UIN in %s's file",jid);
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"password");
	if (tag) password=xmlnode_get_data(tag);
	if (!password){
		g_warning("Couldn't find password in %s's file",jid);
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"email");
	if (tag) email=xmlnode_get_data(tag);
	tag=xmlnode_get_tag(xml,"name");
	if (tag) name=xmlnode_get_data(tag);
	tag=xmlnode_get_tag(xml,"last_sys_msg");
	if (tag) last_sys_msg=atoi(xmlnode_get_data(tag));
	tag=xmlnode_get_tag(xml,"userlist");
	contacts=NULL;
	if (tag){
		Contact *c;
		
		for(t=xmlnode_get_firstchild(tag);t;t=xmlnode_get_nextsibling(t))
			if (!strcmp(xmlnode_get_name(t),"uin")
					&& xmlnode_get_data(t)
					&& atoi(xmlnode_get_data(t)) ) {

					c=g_new0(Contact,1);
					c->uin=atoi(xmlnode_get_data(t));	
					contacts=g_list_append(contacts,c);
			}
	}
	u=g_new0(User,1);
	u->uin=atoi(uin);
	u->jid=g_strdup(jid);
	p=strchr(u->jid,'/');
	if (p) *p=0;
	u->password=g_strdup(password);
	u->last_sys_msg=last_sys_msg;
	u->contacts=contacts;
	xmlnode_free(xml);
	g_assert(users_jid!=NULL);
	njid=jid_normalized(u->jid);
	g_hash_table_insert(users_jid,(gpointer)njid,(gpointer)u);
	u->confirmed=1;
	return u;
}

User *user_get_by_jid(const char *jid){
User *u;
char *str,*p;
	
	str=g_strdup(jid);
	p=strchr(str,'/');
	if (p) *p=0;
	g_assert(users_jid!=NULL);
	u=(User *)g_hash_table_lookup(users_jid,(gpointer)str);
	g_free(str);
	if (u) return u;
	return user_load(jid);	
}

static int user_destroy(User *u){

	g_message("Removing user '%s'",u->jid);
	if (u->jid) g_free(u->jid);
	if (u->password) g_free(u->password);
	g_free(u);
	return 0;
}


int user_remove(User *u){
gpointer key,value;
char *njid;

	g_assert(users_jid!=NULL);
	
	njid=jid_normalized(u->jid);
	if (g_hash_table_lookup_extended(users_jid,(gpointer)u->jid,&key,&value)){
		g_assert(u==value);
		g_hash_table_remove(users_jid,(gpointer)u->jid);
		g_free(key);
	}
	g_free(njid);
	return user_destroy(u);
}

User *user_create(const char *jid,uin_t uin,const char * password){
User *u;
char *p,*njid;

	g_message("Creating user '%s'",jid);

	njid=jid_normalized(jid);
	u=(User *)g_hash_table_lookup(users_jid,(gpointer)njid);
	if (u){
		g_warning("User '%s' already exists",jid);
		g_free(njid);
		return NULL;
	}
	
	if (uin<1){
		g_warning("Bad UIN");
		g_free(njid);
		return NULL;
	}
	if (!password){
		g_warning("Password not given");
		g_free(njid);
		return NULL;
	}
	if (!jid){
		g_warning("JID not given");
		g_free(njid);
		return NULL;
	}

	u=g_new0(User,1);
	u->uin=uin;
	u->jid=g_strdup(jid);
	p=strchr(u->jid,'/');
	if (p) *p=0;
	u->password=g_strdup(password);
	u->confirmed=0;
	g_assert(users_jid!=NULL);
	g_hash_table_insert(users_jid,(gpointer)njid,(gpointer)u);
	return u;
}

int user_subscribe(User *u,uin_t uin){
Contact *c;	
GList *it;

	g_assert(u!=NULL);
	for(it=g_list_first(u->contacts);it;it=it->next){
		c=(Contact *)it->data;
		if (c->uin==uin) return -1;
	}

	c=g_new0(Contact,1);

	c->uin=uin;

	u->contacts=g_list_append(u->contacts,c);
	if (u->confirmed) user_save(u);
	return 0;	
}

int user_unsubscribe(User *u,uin_t uin){
Contact *c;
GList *it;

	for(it=g_list_first(u->contacts);it;it=it->next){
		c=(Contact *)it->data;
		if (c->uin==uin){
			u->contacts=g_list_remove(u->contacts,c);
			g_free(c);
			if (u->confirmed) user_save(u);
			return 0;
		}
	}
	return -1;
}

int user_set_contact_status(User *u,int status,unsigned int uin){
GList *it;
Contact *c;

	g_assert(u!=NULL);
	for(it=u->contacts;it;it=it->next){
		c=(Contact *)it->data;
		if (c->uin==uin){
			c->status=status;
			c->last_update=time(NULL);
			return 0;
		}
	}
		
	return -1;	
}

int users_probe_all(){
DIR *dir;
Stream *s;
struct dirent *de;
struct stat st;
int r;

	s=jabber_stream();
	if (!s) return -1;
	dir=opendir(".");
	if (!dir){
		g_warning("Couldn't open '%s' directory: %s",spool_dir,g_strerror(errno));
		return -1;
	}
	while((de=readdir(dir))){
		r=stat(de->d_name,&st);
		if (r){
			g_warning("Couldn't stat '%s': %s",de->d_name,g_strerror(errno));
			continue;
		}
		if (S_ISREG(st.st_mode)) presence_send_probe(s,de->d_name);
	}
	closedir(dir);
	return 0;
}

int user_sys_msg_received(User *u,int nr){

	if (nr<=u->last_sys_msg) return 0;
	u->last_sys_msg=nr;
	if (u->confirmed) user_save(u);
	return 1;
}

int user_delete(User *u){
int r;
char *njid;

	g_assert(u!=NULL);

	njid=jid_normalized(u->jid);
	r=user_remove(u);
	if (r){
		g_free(njid);
		return r;
	}

	r=unlink(njid);
	if (r && errno!=ENOENT){
		g_warning("Couldn't unlink '%s': %s",njid,g_strerror(errno));
		r=-1;
	}
	else r=0;
		
	g_free(njid);

	return r;	
}
