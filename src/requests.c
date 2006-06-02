/* $Id: requests.c,v 1.37 2004/03/16 19:30:25 mmazur Exp $ */

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
#include <stdio.h>
#include <libxode.h>
#include <libgadu.h>
#include <time.h>
#include <message.h>
#include "requests.h"
#include "search.h"
#include "sessions.h"
#include "stream.h"
#include "register.h"
#include "iq.h"
#include "debug.h"

static GList *requests=NULL;
static GHashTable *lookups=NULL;
static int id_counter=0;

int requests_init(){

	id_counter=time(NULL);
	lookups=g_hash_table_new(g_int_hash, g_int_equal);
	if(!lookups)
		return 1;
	return 0;
}

void request_response_search(struct gg_event *data){
int hash;
Request *r;

	hash=gg_pubdir50_seq(data->event.pubdir50);
	r=g_hash_table_lookup(lookups, &hash);
	g_hash_table_remove(lookups, &hash);

	if (r){
		switch(r->type){
			case RT_VCARD:
				vcard_done(r, data->event.pubdir50);
				remove_request(r);
				return;
			case RT_SEARCH:
				search_done(r, data->event.pubdir50);
				remove_request(r);
				return;
			default:
				break;
		}
	}

}

void request_response_write(struct gg_event *data){
int hash;
Request *r;

	hash=gg_pubdir50_seq(data->event.pubdir50);
	debug("got public directory write response (id: %i)",hash);
	r=g_hash_table_lookup(lookups, &hash);
	if (r==NULL){
		debug("no associated request found");
		return;
	}
	g_hash_table_remove(lookups, &hash);

	if (r->type!=RT_CHANGE){
		if (r->from!=NULL && r->to!=NULL && r->id!=NULL)
			jabber_iq_send_error(r->stream,r->from,r->to,r->id,
				500,_("Got public directory change result for unknown request type."));
		return;
	}
	if (r->from!=NULL && r->to!=NULL && r->id!=NULL && r->query!=NULL){
		debug("Query defined in request - sending result.");
		jabber_iq_send_result(r->stream,r->from,r->to,r->id,NULL);
	}
	else
		debug("Query not defined in request - not sending result.");
}


Request * add_request(RequestType type,const char *from,const char *to,
			const char *id,xmlnode query, void *data,
			Stream *stream){
Request *r;
Session *s;

	r=g_new0(Request,1);
	r->type=type;
	r->id=g_strdup(id);
	r->from=g_strdup(from);
	if (to) r->to=g_strdup(to);
	else r->to=NULL;
	if (query) r->query=xmlnode_dup(query);
	else r->query=NULL;
	r->stream=stream;

	if (type==RT_VCARD || type==RT_SEARCH || type==RT_CHANGE) {
		s=session_get_by_jid(from, stream,0);
		if (s==NULL) return NULL;

		r->hash=id_counter++;
		gg_pubdir50_seq_set((gg_pubdir50_t)data, r->hash);
		gg_pubdir50(s->ggs, (gg_pubdir50_t)data);
		g_hash_table_insert(lookups, &r->hash, r);
	}
	requests=g_list_append(requests,r);
	return r;
}

int remove_request(Request *r){

	if (!r) return -1;
	requests=g_list_remove(requests,r);
	if (r->from) g_free(r->from);
	if (r->to) g_free(r->to);
	if (r->id) g_free(r->id);
	if (r->query) xmlnode_free(r->query);
	if (r->gghttp){
		if (r->gghttp->destroy)
			r->gghttp->destroy(r->gghttp);
		else
			gg_http_free(r->gghttp);
	}

	g_free(r);
	return 0;
}

void requests_done(){

	while(requests) remove_request((Request *)requests->data);
	g_hash_table_destroy(lookups);
}
