#include <stdio.h>
#include <libxode.h>
#include "fds.h"
#include "requests.h"
#include "search.h"
#include "stream.h"
#include "register.h"
#include <libgg.h>
#include <glib.h>

static GList *requests=NULL;

int request_fd_handler(FdHandler *h){
Request *r;
int t;

	r=(Request *)h->extra;
	g_assert(r!=NULL);
	switch(r->type){
		case RT_SEARCH:
			t=gg_search_watch_fd(r->gghttp);
			if (t || r->gghttp->state==GG_STATE_ERROR) search_error(r);
			else if (!t && r->gghttp->state==GG_STATE_DONE) search_done(r);
			else break;
			remove_request(r);
			return 0;
		case RT_VCARD:
			t=gg_search_watch_fd(r->gghttp);
			if (t || r->gghttp->state==GG_STATE_ERROR) vcard_error(r);
			else if (!t && r->gghttp->state==GG_STATE_DONE) vcard_done(r);
			else break;
			remove_request(r);
			return 0;
		case RT_CHANGE:
			t=gg_search_watch_fd(r->gghttp);
			if (t || r->gghttp->state==GG_STATE_ERROR) register_error(r);
			else if (!t && r->gghttp->state==GG_STATE_DONE) register_done(r);
			else break;
			remove_request(r);
			return 0;
		default:
			g_warning("Unknow gg_http session type: %i",r->gghttp->type);
			gg_http_watch_fd(r->gghttp);
			break;
	}		

	h->read=(r->gghttp->check&GG_CHECK_READ);
	h->write=(r->gghttp->check&GG_CHECK_WRITE);
	return 0;
}

Request * add_request(RequestType type,const char *from,const char *id,xmlnode query,struct gg_http *gghttp,Stream *stream){
Request *r;
FdHandler *h;
	
	g_assert(gghttp!=NULL);
	r=(Request *)g_malloc(sizeof(Request));
	memset(r,0,sizeof(*r));
	r->type=type;
	r->id=g_strdup(id);
	r->from=g_strdup(from);
	r->query=query;
	r->gghttp=gghttp;
	h=(FdHandler *)g_malloc(sizeof(FdHandler));
	memset(h,0,sizeof(*h));
	h->fd=gghttp->fd;
	h->read=(gghttp->check&GG_CHECK_READ);
	h->write=(gghttp->check&GG_CHECK_WRITE);
	h->extra=r;
	h->func=request_fd_handler;
	fd_register_handler(h);
	r->fdh=h;
	r->stream=stream;
	requests=g_list_append(requests,r);
	return r;
}

int remove_request(Request *r){

	if (!r) return -1;
	g_list_remove(requests,r);
	fd_unregister_handler(r->fdh);
	if (r->from) g_free(r->from);	
	if (r->id) g_free(r->id);	
	if (r->query) xmlnode_free(r->query);
	g_free(r);
	return 0;
}
