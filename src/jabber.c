#include <stdio.h>
#include <assert.h>
#include "jabber.h"
#include "stream.h"
#include "ggtrans.h"
#include "iq.h"
#include "presence.h"
#include "message.h"
#include <glib.h>

Stream *stream;
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
		fprintf(stderr,"\nunexpected <stream:stream/>\n");	
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
		fprintf(stderr,"\nunexpected <hanshake/>\n");	
		return;
	}
		
	fprintf(stderr,"\nhandshake OK\n");	
	jabber_state=JS_CONNECTED;
}

void jabber_stream_error(Stream *s,xmlnode x){

	fprintf(stderr,"\n Stream error: %s\n",xmlnode_get_data(x));
	stream_close(s);
	stop_it=1;
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
	else fprintf(stderr,"\nUnsupported tag: '%s'\n",name);
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
			fprintf(stderr,"\nStream Error\n");
			if (x){
				fprintf(stderr,"\t%s\n",xmlnode_get_data(x));
			}
			break;	
		default:
			fprintf(stderr,"\nUnknown node type\n");
			stop_it=1;
			stream_close(s);
			break;
	}
}

int jabber_init(){
char *server;
int port;
xmlnode node;

	node=xmlnode_get_tag(config,"service");
	if (!node){
		fprintf(stderr,"No <service/> found in config file\n");
		return -1;
	}
	my_name=xmlnode_get_attrib(node,"jid");
	if (!my_name){
		fprintf(stderr,"<service/> without \"jid\" in config file\n");
		return -1;
	}
	
	node=xmlnode_get_tag(config,"connect/ip");
	if (!node){
		fprintf(stderr,"Jabberd server not found in config file\n");
		return -1;
	}
	server=xmlnode_get_data(node);
	node=xmlnode_get_tag(config,"connect/port");
	if (!node){
		fprintf(stderr,"Connect port not found in config file\n");
		return -1;
	}
	port=atoi(xmlnode_get_data(node));
	node=xmlnode_get_tag(config,"connect/secret");
	if (!node){
		fprintf(stderr,"Connect secret not found in config file\n");
		return -1;
	}
	secret=xmlnode_get_data(node);
	
	node=xmlnode_get_tag(config,"instructions");
	if (!node){
		fprintf(stderr,"Connect secret not found in config file\n");
		return -1;
	}
	instructions=xmlnode_get_data(node);

	jabber_state=JS_NONE;
	stream=stream_connect(server,port,1,jabber_event_cb);
	assert(stream!=NULL);
	return 0;
}

int jabber_done(){

	if (stream){
		stream_destroy(stream);
		stream=NULL;
	}
	return 0;
}

int jabber_iter(){

	if (stop_it && stream){
			if (stop_it>1){
				stream_destroy(stream);
				stream=NULL;
				return 1;
			}
			stop_it++;
	}
	else if (stream==NULL) return 1;
	return 0;
}
