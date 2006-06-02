/* $Id: jabber.c,v 1.24 2004/04/13 17:44:07 jajcus Exp $ */

/*
 *  (C) Copyright 2002-2006 Jacek Konieczny [jajcus(a)jajcus,net]
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
#include "stream.h"
#include "iq.h"
#include "presence.h"
#include "message.h"
#include "users.h"
#include "conf.h"

GSource * jabber_source=NULL;
Stream *stream=NULL;
char *stream_id=NULL;
const char *secret;
const char *my_name;
int bare_domain = 0;
int stop_it;

enum jabber_state_e jabber_state;

void jabber_stream_start(Stream *s,xmlnode x){
char hashbuf[50];
char *str;
xmlnode tag;

	if (jabber_state!=JS_NONE){
		g_warning(N_("unexpected <stream:stream/>"));
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
		g_warning(N_("unexpected <hanshake/>"));
		return;
	}

	g_message(L_("handshake OK"));
	jabber_state=JS_CONNECTED;
	users_probe_all();
}

void jabber_stream_error(Stream *s,xmlnode x){
char *data;

	data=xmlnode_get_data(x);
	if (data==NULL) data="-unknown-";
	g_critical(L_("Stream error: %s"),data);
	stream_close(s);
	stop_it=1;
}

struct stream_s * jabber_stream(){
	return stream;
}

void jabber_node(Stream *s,xmlnode x){
char *name;

	packets_in++;
	name=xmlnode_get_name(x);
	if (strcmp(name,"stream:stream")==0)
		jabber_stream_start(s,x);
	else if (strcmp(name,"handshake")==0)
		jabber_handshake(s,x);
	else if (strcmp(name,"stream:error")==0)
		jabber_stream_error(s,x);
	else if (strcmp(name,"iq")==0)
		jabber_iq(s,x);
	else if (strcmp(name,"presence")==0)
		jabber_presence(s,x);
	else if (strcmp(name,"message")==0)
		jabber_message(s,x);
	else g_warning(N_("Unsupported tag: %s"),xmlnode2str(x));
	xmlnode_free(x);
}

void jabber_event_cb(int type,xmlnode x,void *arg){
Stream *s;
char *str;
char *data;

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
			g_warning(N_("Stream Error"));
			if (x){
				data=xmlnode_get_data(x);
				if (data==NULL) data="-unknown-";
				g_warning("    %s",data);
			}
			break;
		default:
			g_critical(L_("Unknown node type: %i"),type);
			stop_it=1;
			stream_close(s);
			break;
	}
}

gboolean jabber_source_prepare(GSource *source,
				gint	 *timeout){

	*timeout=1000;
	if (stop_it || stream==NULL) return TRUE;
	return FALSE;
}

gboolean jabber_source_check(GSource *source){

	if (stop_it || stream==NULL) return TRUE;
	return FALSE;
}

gboolean jabber_source_dispatch(GSource *source,
			GSourceFunc callback,
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

void jabber_source_finalize(GSource *source){
}

static GSourceFuncs jabber_source_funcs={
		jabber_source_prepare,
		jabber_source_check,
		jabber_source_dispatch,
		jabber_source_finalize,
		NULL,
		NULL
		};

void jabber_stream_destroyed(struct stream_s *s){

	if (s==stream) stream=NULL;
}

int jabber_done(){

	if (stream){
		stream_destroy(stream);
		stream=NULL;
	}
	if (jabber_source){
		g_source_destroy(jabber_source);
	}
	g_free(register_instructions);
	g_free(search_instructions);
	g_free(gateway_desc);
	g_free(gateway_prompt);
	stream_del_destroy_handler(jabber_stream_destroyed);
	return 0;
}

static char *server;
static int port;

int jabber_init(){
xmlnode node;

	stream_add_destroy_handler(jabber_stream_destroyed);
	node=xmlnode_get_tag(config,"service");
	if (!node)
		g_error(L_("No <service/> found in config file"));

	my_name=xmlnode_get_attrib(node,"jid");
	if (!my_name)
		g_error(L_("<service/> without \"jid\" in config file"));

	node=xmlnode_get_tag(config, "bare_domain");
	if (node) bare_domain=1;

	server=config_load_string("connect/ip");
	if (!server)
		g_error(L_("Jabberd server not found in config file"));

	port=config_load_int("connect/port",0);
	if (port<=0)
		g_error(L_("Connect port not found in config file"));

	node=xmlnode_get_tag(config,"connect/secret");
	if (node) secret=xmlnode_get_data(node);
	if (!node || !secret)
		g_error(L_("Connect secret not found in config file"));

	register_instructions=config_load_formatted_string("register/instructions");
	if (!register_instructions)
		g_error(L_("Registration instructions not not found in config file"));

	search_instructions=config_load_formatted_string("search/instructions");
	if (!search_instructions)
		g_error(L_("Search instructions not found in config file"));

	gateway_desc=config_load_formatted_string("gateway/desc");
	if (!gateway_desc)
		g_error(L_("Gateway instructions not found in config file"));

	gateway_prompt=config_load_formatted_string("gateway/prompt");
	if (!gateway_prompt)
		g_error(L_("Gateway prompt not found in config file"));

	jabber_state=JS_NONE;
	return 0;
}

int jabber_connect(){

	stream=stream_connect(server,port,1,jabber_event_cb);
	g_assert(stream!=NULL);
	jabber_source=g_source_new(&jabber_source_funcs,sizeof(GSource));
	g_source_attach(jabber_source,g_main_loop_get_context(main_loop));
	return 0;
}

