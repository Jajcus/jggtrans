#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "jabber.h"
#include "ggtrans.h"
#include "users.h"

GHashTable *users_uin=NULL;
GHashTable *users_jid=NULL;
static char *spool_dir;

int users_init(){
xmlnode node;
int r;

	node=xmlnode_get_tag(config,"spool");
	if (node) spool_dir=xmlnode_get_data(node);
	if (!spool_dir){
		fprintf(stderr,"No <spool/> defined in config file\n");
		return -1;
	}

	r=chdir(spool_dir);
	if (r){
		fprintf(stderr,"Couldn't enter %s\n",spool_dir);
		perror("chdir");
		return -1;
	}
	
	users_jid=g_hash_table_new(g_str_hash,g_str_equal);
	if (!users_jid) return -1;
	users_uin=g_hash_table_new(g_int_hash,g_int_equal);
	if (!users_uin) return -1;
	return 0;
}

int user_save(User *u){
FILE *f;
char *fn;
char *str;
int r;	
xmlnode xml,tag,userlist;
int i;

	assert(u!=NULL);
	str=strchr(u->jid,'/');
	assert(str==NULL);
	
	fn=g_strdup_printf("%s.new",u->jid);
	f=fopen(fn,"w");
	if (!f){
		fprintf(stderr,"Couldn't open '%s'\n",fn);
		g_free(fn);
		perror("fopen");
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
	if (u->email){
		tag=xmlnode_insert_tag(xml,"email");
		xmlnode_insert_cdata(tag,u->email,-1);
	}
	if (u->name){
		tag=xmlnode_insert_tag(xml,"name");
		xmlnode_insert_cdata(tag,u->name,-1);
	}

	if (u->userlist){
		userlist=xmlnode_insert_tag(xml,"userlist");
		for(i=0;i<u->userlist_len;i++){
			tag=xmlnode_insert_tag(userlist,"uin");
			str=g_strdup_printf("%lu",(unsigned long)u->userlist[i]);
			xmlnode_insert_cdata(tag,str,-1);
			g_free(str);
		}
	}

	str=xmlnode2str(xml);
	r=fputs(str,f);
	if (r<0){
		fprintf(stderr,"Couldn't save '%s'\n",u->jid);
		perror("fputs");
		fclose(f);
		unlink(fn);
		xmlnode_free(xml);
		g_free(fn);
		return -1;
	}
	fclose(f);
	r=unlink(u->jid);
	if (r && errno!=ENOENT){
		fprintf(stderr,"Couldn't unlink '%s'\n",u->jid);
		perror("unlink");
		xmlnode_free(xml);
		g_free(fn);
		return -1;
	}
	
	r=rename(fn,u->jid);
	if (r){
		fprintf(stderr,"Couldn't rename '%s' to '%s'\n",fn,u->jid);
		perror("rename");
		xmlnode_free(xml);
		g_free(fn);
		return -1;
	}
	
	xmlnode_free(xml);
	g_free(fn);
	return 0;
}

User *user_load(const char *jid){
char *fn,*p;
xmlnode xml,tag,t;
char *uin,*ujid,*name,*password,*email;
User *u;
uin_t *userlist;
int userlist_len;

	uin=ujid=name=password=email=NULL;
	fn=g_strdup(jid);
	p=strchr(fn,'/');
	if (p) *p=0;
	xml=xmlnode_file(fn);
	if (!xml){
		fprintf(stderr,"Couldn't read or parse '%s'\n",fn);
		g_free(fn);
		perror("fopen");
		return NULL;
	}
	g_free(fn);
	tag=xmlnode_get_tag(xml,"jid");
	if (tag) ujid=xmlnode_get_data(tag);
	if (!ujid){
		fprintf(stderr,"Couldn't find JID in users file\n");
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"uin");
	if (tag) uin=xmlnode_get_data(tag);
	if (!uin){
		fprintf(stderr,"Couldn't find UIN in users file\n");
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"password");
	if (tag) password=xmlnode_get_data(tag);
	if (!password){
		fprintf(stderr,"Couldn't find password in users file\n");
		return NULL;
	}
	tag=xmlnode_get_tag(xml,"email");
	if (tag) email=xmlnode_get_data(tag);
	tag=xmlnode_get_tag(xml,"name");
	if (tag) name=xmlnode_get_data(tag);
	tag=xmlnode_get_tag(xml,"userlist");
	userlist_len=0;
	if (tag){
		for(t=xmlnode_get_firstchild(tag);t;t=xmlnode_get_nextsibling(t))
			if (!g_strcasecmp(xmlnode_get_name(t),"uin")
					&& xmlnode_get_data(t)
					&& atoi(xmlnode_get_data(t)) ) 
				userlist_len++;
		if (userlist_len){
			int i;
			userlist=(uin_t *)g_malloc(sizeof(uin_t)*userlist_len);
			i=0;
			for(t=xmlnode_get_firstchild(tag);t;
					t=xmlnode_get_nextsibling(t))
				if (!g_strcasecmp(xmlnode_get_name(t),"uin")
				    && xmlnode_get_data(t)
				    && atoi(xmlnode_get_data(t)) ) 
					userlist[i++]=atoi(xmlnode_get_data(t));	
		}
		else userlist=NULL;
	}
	else userlist=NULL;
	u=(User *)g_malloc(sizeof(User));
	memset(u,0,sizeof(User));
	u->uin=atoi(uin);
	u->jid=g_strdup(jid);
	u->password=g_strdup(password);
	u->name=g_strdup(name);
	u->email=g_strdup(email);
	u->userlist=userlist;
	u->userlist_len=userlist_len;
	xmlnode_free(xml);
	g_hash_table_insert(users_jid,(gpointer)u->jid,(gpointer)u);
	g_hash_table_insert(users_uin,GINT_TO_POINTER(u->uin),(gpointer)u);
	u->confirmed=1;
	return u;
}

User *user_get_by_uin(uin_t uin){
User *u;
	
	u=(User *)g_hash_table_lookup(users_uin,GINT_TO_POINTER(uin));
	return u;
}

User *user_get_by_jid(const char *jid){
User *u;
char *str,*p;
	
	str=g_strdup(jid);
	p=strchr(str,'/');
	if (p) *p=0;
	u=(User *)g_hash_table_lookup(users_jid,(gpointer)str);
	g_free(str);
	if (u) return u;
	return user_load(jid);	
}

int user_delete(User *u){

	if (u->uin) g_hash_table_remove(users_uin,GINT_TO_POINTER(u->uin));
	if (u->jid){
		g_hash_table_remove(users_jid,(gpointer)u->jid);
		g_free(u->jid);
	}
	if (u->name) g_free(u->name);
	if (u->password) g_free(u->password);
	if (u->email) g_free(u->email);
	g_free(u);
	return 0;
}

User *user_add(const char *jid,uin_t uin,const char *name,const char * password,const char *email){
User *u;
char *p;

	if (uin<1){
		fprintf(stderr,"Bad UIN\n");
		return NULL;
	}
	if (!password){
		fprintf(stderr,"Password not given\n");
		return NULL;
	}
	if (!jid){
		fprintf(stderr,"JID not given\n");
		return NULL;
	}

	u=(User *)g_malloc(sizeof(User));
	memset(u,0,sizeof(User));
	u->uin=uin;
	u->jid=g_strdup(jid);
	p=strchr(u->jid,'/');
	if (p) *p=0;
	u->password=g_strdup(password);
	if (name) u->name=g_strdup(name);
	if (email) u->email=g_strdup(email);
	u->confirmed=0;
	g_hash_table_insert(users_jid,(gpointer)u->jid,(gpointer)u);
	g_hash_table_insert(users_uin,GINT_TO_POINTER(u->uin),(gpointer)u);
	return u;
}

int user_subscribe(User *u,uin_t uin){
int i;

	assert(u!=NULL);
	if (!u->userlist){
		u->userlist=(uin_t *)g_malloc(sizeof(uin_t));
		u->userlist[0]=uin;
		u->userlist_len=1;
		return 0;
	}
	
	for(i=0;i<u->userlist_len;i++)
		if (u->userlist[i]==uin) return 1;
	
	u->userlist=(uin_t *)g_realloc(u->userlist,sizeof(uin_t)*(u->userlist_len+1));
	assert(u->userlist!=NULL);
	u->userlist[u->userlist_len++]=uin;
	if (u->confirmed) user_save(u);
	return 0;	
}

int user_unsubscribe(User *u,uin_t uin){
int i;

	assert(u!=NULL);
	if (!u->userlist) return 0;
	
	for(i=0;i<u->userlist_len;i++)
		if (u->userlist[i]==uin) break;

	if (i==u->userlist_len) return 0;
	
	u->userlist_len--;
	if (u->userlist_len<1){
		g_free(u->userlist);
		u->userlist=NULL;
	}
	u->userlist=(uin_t *)g_realloc(u->userlist,sizeof(uin_t)*u->userlist_len);
	assert(u->userlist!=NULL);
	if (u->confirmed) user_save(u);
	return 0;	
}
