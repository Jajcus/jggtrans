#include "ggtrans.h"
#include "jabber.h"
#include "stream.h"
#include "iq.h"
#include "presence.h"
#include "message.h"
#include "users.h"

Stream *stream=NULL;
char *stream_id=NULL;
const char *secret;
const char *my_name;
int stop_it;

enum jabber_state_e jabber_state;

void jabber_stream_start(Stream *s,xmlnode x){
char hashbuf[50];
char *str;
xmlnode tag;
	
	if (jabber_state!=JS_NONE){
		g_warning("unexpected <stream:stream/>");	
		return;
	}

	stream_id=xmlnode_get_attrib(x,"id");
	str=g_strdup_printf("%s%s",stream_id,secret);
	shahash_r(str,hashbuf);
	g_free(str);
	tag=xmlnode_new_tag("handshake");
	xmlnode_insert_cdata(tag,hashbuf,-1);
	stream_write(s,tag);
	xmlnode_free(tag);
	jabber_state=JS_HANDSHAKE;
}

void jabber_handshake(Stream *s,xmlnode x){

	if (jabber_state!=JS_HANDSHAKE){
		g_warning("unexpected <hanshake/>");	
		return;
	}
		
	g_message("handshake OK");
	jabber_state=JS_CONNECTED;
	users_probe_all();
}

void jabber_stream_error(Stream *s,xmlnode x){

	g_critical("Stream error: %s",xmlnode_get_data(x));
	stream_close(s);
	stop_it=1;
}

struct stream_s * jabber_stream(){
	return stream;
}

void jabber_node(Stream *s,xmlnode x){
char *name;

	name=xmlnode_get_name(x);
	if (g_strcasecmp(name,"stream:stream")==0)
		jabber_stream_start(s,x);
	else if (g_strcasecmp(name,"handshake")==0)
		jabber_handshake(s,x);
	else if (g_strcasecmp(name,"stream:error")==0)
		jabber_stream_error(s,x);
	else if (g_strcasecmp(name,"iq")==0)
		jabber_iq(s,x);
	else if (g_strcasecmp(name,"presence")==0)
		jabber_presence(s,x);
	else if (g_strcasecmp(name,"message")==0)
		jabber_message(s,x);
	else g_warning("Unsupported tag: %s",xmlnode2str(x));
}

void jabber_event_cb(int type,xmlnode x,void *arg){
Stream *s;
char *str;

	if (stop_it || !stream) return;

	s=(Stream *)arg;
	if (x){
		str=xmlnode2str(x);
	}
	switch(type){
		case XSTREAM_ROOT:
			jabber_node(s,x);
			break;
		case XSTREAM_CLOSE:
			if (x || !stop_it){
				stop_it=1;
				stream_close(s);
			}
			else stop_it=2;
			break;
		case XSTREAM_NODE:
			jabber_node(s,x);
			break;
		case XSTREAM_ERR:
			g_warning("Stream Error");
			if (x){
				g_warning("    %s",xmlnode_get_data(x));
			}
			break;	
		default:
			g_critical("Unknown node type: %i",type);
			stop_it=1;
			stream_close(s);
			break;
	}
}

gboolean jabber_source_prepare(gpointer  source_data,
				GTimeVal *current_time,
				gint     *timeout,
				gpointer  user_data){

	*timeout=1000;
	if (stop_it || stream==NULL) return TRUE;
	return FALSE;
}

gboolean jabber_source_check(gpointer  source_data,
                        	GTimeVal *current_time,
                        	gpointer  user_data){

	if (stop_it || stream==NULL) return TRUE;
	return FALSE;
}

gboolean jabber_source_dispatch(gpointer  source_data,
                        GTimeVal *current_time,
                        gpointer  user_data){
	
	if (stop_it && stream){
			if (stop_it>1){
				stream_destroy(stream);
				stream=NULL;
				g_main_quit(main_loop);
			}
			stop_it++;
	}
	else if (stream==NULL) g_main_quit(main_loop);
	
	return TRUE;
}

void jabber_source_destroy(gpointer user_data){
}

static GSourceFuncs jabber_source_funcs={
		jabber_source_prepare,
		jabber_source_check,
		jabber_source_dispatch,
		jabber_source_destroy
		};

int jabber_done(){

	if (stream){
		stream_destroy(stream);
		stream=NULL;
	}
	return 0;
}

static char *load_config_string(const char *tag){
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
					if (out[0]) add="\n";
				}
				else if (!g_strcasecmp(name,"br"))
					add="\n";
				else g_warning("Unknown formatting '%s' in '%s'",
						xmlnode2str(child),xmlnode2str(node));
				break;
			case NTYPE_CDATA:
				tmp=xmlnode_get_data(child);
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
		}
		child=xmlnode_get_nextsibling(child);
	}
	
	return out;
}

int jabber_init(){
char *server;
char *str;
int port;
xmlnode node;

	node=xmlnode_get_tag(config,"service");
	if (!node)
		g_error("No <service/> found in config file");
	
	my_name=xmlnode_get_attrib(node,"jid");
	if (!my_name) 
		g_error("<service/> without \"jid\" in config file");
	
	node=xmlnode_get_tag(config,"connect/ip");
	if (node) server=xmlnode_get_data(node);
	if (!node || !server)
		g_error("Jabberd server not found in config file");
	
	node=xmlnode_get_tag(config,"connect/port");
	if (node) str=xmlnode_get_data(node);
	if (!node || !str) 
		g_error("Connect port not found in config file");
	port=atoi(str);
	
	node=xmlnode_get_tag(config,"connect/secret");
	if (node) secret=xmlnode_get_data(node);
	if (!node || !secret) 
		g_error("Connect secret not found in config file");
	
	register_instructions=load_config_string("register/instructions");
	if (!register_instructions) 
		g_error("Registration instructions not not found in config file");

	search_instructions=load_config_string("search/instructions");
	if (!search_instructions)
		g_error("Search instructions not found in config file");

	gateway_desc=load_config_string("gateway/desc");
	if (!gateway_desc)
		g_error("Gateway instructions not found in config file");

	gateway_prompt=load_config_string("gateway/prompt");
	if (!gateway_prompt)
		g_error("Gateway prompt not found in config file");

	jabber_state=JS_NONE;
	stream=stream_connect(server,port,1,jabber_event_cb);
	g_assert(stream!=NULL);
	g_source_add(G_PRIORITY_DEFAULT,0,&jabber_source_funcs,NULL,NULL,NULL);
	return 0;
}

