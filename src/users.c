/* $Id: users.c,v 1.38 2003/04/22 08:23:00 jajcus Exp $ */

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
#include <time.h>
#include <locale.h>
#include "ggtrans.h"
#include "jabber.h"
#include "users.h"
#include "jid.h"
#include "presence.h"
#include "debug.h"
#include "conf.h"

GHashTable *users_jid=NULL;
static char *spool_dir;
char *default_user_locale="C";

int users_init(){
int r;

	default_user_locale=config_load_string("default_locale");

	spool_dir=config_load_string("spool");
	if (!spool_dir)
		g_error(L_("No <spool/> defined in config file"));

	r=chdir(spool_dir);
	if (r) g_error(L_("Couldn't enter %s: %s"),spool_dir,g_strerror(errno));

	users_jid=g_hash_table_new(g_str_hash,g_str_equal);
	if (!users_jid) return -1;
	return 0;
}

static int user_destroy(User *s);

#if 0
static gboolean users_hash_func(gpointer key,gpointer value,gpointer udata){

	user_destroy((User *)value);
	g_free(key);
	return TRUE;
}
#endif

int users_done(){
guint s;
	s=g_hash_table_size(users_jid);
	if (s) g_warning(L_("Still %u users in hash table"),s);
	/*g_hash_table_foreach_remove(users_jid,users_hash_func,NULL);*/
	g_hash_table_destroy(users_jid);
	return 0;
}

int user_save(User *u){
FILE *f;
char *fn;
char *str;
char *njid;
int r;
xmlnode xml,tag,ctag,userlist;

	g_assert(u!=NULL);
	str=strchr(u->jid,'/');
	g_assert(str==NULL);

	if (!u->confirmed){
		g_message(L_("Not saving user '%s' - account not confirmed."),u->jid);
		return -1;
	}

	g_message(L_("Saving user '%s'"),u->jid);
	njid=jid_normalized(u->jid);
	fn=g_strdup_printf("%s.new",njid);
	f=fopen(fn,"w");
	if (!f){
		g_warning(L_("Couldn't open '%s': %s"),fn,g_strerror(errno));
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

	if (u->invisible) tag=xmlnode_insert_tag(xml,"invisible");
	if (u->friends_only) tag=xmlnode_insert_tag(xml,"friendsonly");
	if (u->locale){
		tag=xmlnode_insert_tag(xml,"locale");
		xmlnode_insert_cdata(tag,u->locale,-1);
	}

	if (u->contacts){
		GList *it;
		Contact *c;

		userlist=xmlnode_insert_tag(xml,"userlist");
		for(it=g_list_first(u->contacts);it;it=it->next){
			c=(Contact *)it->data;
			ctag=xmlnode_insert_tag(userlist,"contact");
			str=g_strdup_printf("%lu",(unsigned long)c->uin);
			xmlnode_put_attrib(ctag,"uin",str);
			g_free(str);
		}
	}

	str=xmlnode2str(xml);
	r=fputs(str,f);
	if (r<0){
		g_warning(L_("Couldn't save '%s': %s"),u->jid,g_strerror(errno));
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
		g_warning(L_("Couldn't unlink '%s': %s"),njid,g_strerror(errno));
		xmlnode_free(xml);
		g_free(fn);
		g_free(njid);
		return -1;
	}

	r=rename(fn,njid);
	if (r){
		g_warning(L_("Couldn't rename '%s' to '%s': %s"),fn,u->jid,g_strerror(errno));
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
char *uin,*ujid,*name,*password,*email,*locale;
int last_sys_msg=0,invisible=0,friends_only=0;
User *u;
GList *contacts;
char *p;
char *data;

	uin=ujid=name=password=email=NULL;
	debug(L_("Loading user '%s'"),jid);
	fn=jid_normalized(jid);
	errno=0;
	xml=xmlnode_file(fn);
	if (xml==NULL){
		debug(L_("Couldn't read or parse '%s': %s"),fn,errno?g_strerror(errno):N_("XML parse error"));
		g_free(fn);
		return NULL;
	}
	g_free(fn);
	tag=xmlnode_get_tag(xml,"jid");
	if (tag!=NULL) ujid=xmlnode_get_data(tag);
	if (ujid==NULL){
		g_warning(L_("Couldn't find JID in %s's file"),jid);
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"uin");
	if (tag!=NULL) uin=xmlnode_get_data(tag);
	if (uin==NULL){
		g_warning(L_("Couldn't find UIN in %s's file"),jid);
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"password");
	if (tag!=NULL) password=xmlnode_get_data(tag);
	if (password==NULL){
		g_warning(L_("Couldn't find password in %s's file"),jid);
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"email");
	if (tag!=NULL) email=xmlnode_get_data(tag);
	tag=xmlnode_get_tag(xml,"name");
	if (tag!=NULL) name=xmlnode_get_data(tag);
	tag=xmlnode_get_tag(xml,"last_sys_msg");
	if (tag!=NULL){
		data=xmlnode_get_data(tag);
		if (data!=NULL)
			last_sys_msg=atoi(data);
	}
	tag=xmlnode_get_tag(xml,"friendsonly");
	if (tag!=NULL) friends_only=1;
	tag=xmlnode_get_tag(xml,"invisible");
	if (tag!=NULL) invisible=1;
	tag=xmlnode_get_tag(xml,"locale");
	if (tag!=NULL) locale=xmlnode_get_data(tag);
	else locale=NULL;
	tag=xmlnode_get_tag(xml,"userlist");
	contacts=NULL;
	if (tag!=NULL){
		Contact *c;

		for(t=xmlnode_get_firstchild(tag);t;t=xmlnode_get_nextsibling(t)){
			if (!strcmp(xmlnode_get_name(t),"uin")){
				char *d;
				int uin;

				d=xmlnode_get_data(t);
				if (d==NULL) continue;
				uin=atoi(d);
				if (uin<=0) continue;

				c=g_new0(Contact,1);
				c->status=GG_STATUS_NOT_AVAIL;
				c->uin=uin;
				contacts=g_list_append(contacts,c);
				continue;
			}
			if (!strcmp(xmlnode_get_name(t),"contact")){
				char *d;
				int uin;

				d=xmlnode_get_attrib(t,"uin");
				if (d==NULL) continue;
				uin=atoi(d);
				if (uin<=0) continue;

				c=g_new0(Contact,1);
				c->status=GG_STATUS_NOT_AVAIL;
				c->uin=uin;

				contacts=g_list_append(contacts,c);
			}
		}
	}
	u=g_new0(User,1);
	u->uin=atoi(uin);
	u->jid=g_strdup(jid);
	p=strchr(u->jid,'/');
	if (p) *p=0;
	u->password=g_strdup(password);
	u->last_sys_msg=last_sys_msg;
	u->friends_only=friends_only;
	u->invisible=invisible;
	u->locale=g_strdup(locale);
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
char *njid;

	njid=jid_normalized(jid);
	u=(User *)g_hash_table_lookup(users_jid,(gpointer)njid);
	g_free(njid);
	if (u) return u;
	return user_load(jid);
}

static int user_destroy(User *u){
GList *it;
Contact *c;

	g_message(L_("Destroying user '%s'"),u->jid);

	g_assert(u!=NULL);
	for(it=u->contacts;it;it=it->next){
		c=(Contact *)it->data;
		g_free(c->status_desc);
		g_free(c);
	}
	g_list_free(u->contacts);
	u->contacts=NULL;

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
	if (g_hash_table_lookup_extended(users_jid,(gpointer)njid,&key,&value)){
		g_assert(u==value);
		g_hash_table_remove(users_jid,(gpointer)njid);
		g_free(key);
	}
	else debug(L_("user_remove: user '%s' not found in hash table"),njid);
	g_free(njid);
	return user_destroy(u);
}

User *user_create(const char *jid,uin_t uin,const char * password){
User *u;
char *p,*njid;

	g_message(L_("Creating user '%s'"),jid);

	njid=jid_normalized(jid);
	u=(User *)g_hash_table_lookup(users_jid,(gpointer)njid);
	if (u){
		g_warning(L_("User '%s' already exists"),jid);
		g_free(njid);
		return NULL;
	}

	if (uin<1){
		g_warning(L_("Bad UIN"));
		g_free(njid);
		return NULL;
	}
	if (!password){
		g_warning(L_("Password not given"));
		g_free(njid);
		return NULL;
	}
	if (!jid){
		g_warning(L_("JID not given"));
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
	u->invisible=0;
	u->friends_only=1;
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
	user_save(u);
	return 0;
}

int user_unsubscribe(User *u,uin_t uin){
Contact *c;
GList *it;

	for(it=g_list_first(u->contacts);it;it=it->next){
		c=(Contact *)it->data;
		if (c->uin==uin){
			u->contacts=g_list_remove(u->contacts,c);
			if (c->status_desc) free(c->status_desc);
			g_free(c);
			user_save(u);
			return 0;
		}
	}
	return -1;
}

int user_set_contact_status(User *u,int status,unsigned int uin,char *desc,
				int more,uint32_t ip,uint16_t port,uint32_t version){
GList *it;
Contact *c;

	g_assert(u!=NULL);
	for(it=u->contacts;it;it=it->next){
		c=(Contact *)it->data;
		if (c->uin==uin){
			c->status=status;
			c->last_update=time(NULL);
			if (c->status_desc) g_free(c->status_desc);
			if (desc) c->status_desc=g_strdup(desc);
			else c->status_desc=NULL;
			if (more){
				c->ip=ip;
				c->port=port;
				c->version=version;
			}
			return 0;
		}
	}

	return -1;
}

void user_load_locale(User *u){

	if (u && u->locale){
		setlocale(LC_MESSAGES,u->locale);
		setlocale(LC_CTYPE,u->locale);
	}
	else{
		setlocale(LC_MESSAGES,default_user_locale);
		setlocale(LC_CTYPE,default_user_locale);
	}
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
		g_warning(L_("Couldn't open '%s' directory: %s"),spool_dir,g_strerror(errno));
		return -1;
	}
	while((de=readdir(dir))){
		r=stat(de->d_name,&st);
		if (r){
			g_warning(L_("Couldn't stat '%s': %s"),de->d_name,g_strerror(errno));
			continue;
		}
		if (S_ISREG(st.st_mode)) presence_send_probe(s,de->d_name);
	}
	closedir(dir);
	return 0;
}

int users_count(){
DIR *dir;
struct dirent *de;
struct stat st;
int r,count=0;

	dir=opendir(".");
	if (!dir){
		g_warning(L_("Couldn't open '%s' directory: %s"),spool_dir,g_strerror(errno));
		return -1;
	}
	while((de=readdir(dir))){
		r=stat(de->d_name,&st);
		if (r){
			g_warning(L_("Couldn't stat '%s': %s"),de->d_name,g_strerror(errno));
			continue;
		}
		if (S_ISREG(st.st_mode)) count++;
	}
	closedir(dir);
	return count;
}

int user_sys_msg_received(User *u,int nr){

	if (nr<=u->last_sys_msg) return 0;
	u->last_sys_msg=nr;
	user_save(u);
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
		g_warning(L_("Couldn't unlink '%s': %s"),njid,g_strerror(errno));
		r=-1;
	}
	else r=0;

	g_free(njid);

	return r;
}

void user_print(User *u,int indent){
char *space,*space1;
GList *it;
Contact *c;

	space=g_strnfill(indent*2,' ');
	space1=g_strnfill((indent+1)*2,' ');
	g_message(L_("%sUser: %p"),space,u);
	g_message(L_("%sJID: %s"),space,u->jid);
	g_message(L_("%sUIN: %u"),space,(unsigned)u->uin);
	g_message(L_("%sPassword: %p"),space,u->password);
	g_message(L_("%sLast sys message: %i"),space,u->last_sys_msg);
	g_message(L_("%sConfirmed: %i"),space,u->confirmed);
	g_message(L_("%sContacts:"),space);
	for(it=g_list_first(u->contacts);it;it=it->next){
		c=(Contact *)it->data;
		g_message(L_("%sContact: %p"),space1,c);
		g_message(L_("%sUin: %u"),space1,(unsigned)c->uin);
		g_message(L_("%sStatus: %i"),space1,c->status);
		g_message(L_("%sLast update: %s"),space1,ctime((time_t *)&c->last_update));
	}
	g_free(space1);
	g_free(space);
}

