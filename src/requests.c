/* $Id: requests.c,v 1.12 2002/12/09 09:55:52 jajcus Exp $ */

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
#include <stdio.h>
#include <libxode.h>
#include <libgadu.h>
#include "requests.h"
#include "search.h"
#include "stream.h"
#include "register.h"

static GList *requests=NULL;

int request_io_handler(GIOChannel *source,GIOCondition condition,gpointer data){
Request *r;
int t;
GIOCondition cond;

	r=(Request *)data;
	g_assert(r!=NULL);
	switch(r->type){
		case RT_SEARCH:
			t=gg_search_watch_fd(r->gghttp);
			if (t || r->gghttp->state==GG_STATE_ERROR) search_error(r);
			else if (!t && r->gghttp->state==GG_STATE_DONE) search_done(r);
			else break;
			r->io_watch=0;
			remove_request(r);
			return FALSE;
		case RT_VCARD:
			t=gg_search_watch_fd(r->gghttp);
			if (t || r->gghttp->state==GG_STATE_ERROR) vcard_error(r);
			else if (!t && r->gghttp->state==GG_STATE_DONE) vcard_done(r);
			else break;
			r->io_watch=0;
			remove_request(r);
			return FALSE;
		case RT_CHANGE:
			t=gg_search_watch_fd(r->gghttp);
			if (t || r->gghttp->state==GG_STATE_ERROR) register_error(r);
			else if (!t && r->gghttp->state==GG_STATE_DONE) register_done(r);
			else break;
			r->io_watch=0;
			remove_request(r);
			return FALSE;
		default:
			g_warning("Unknow gg_http session type: %i",r->gghttp->type);
			gg_http_watch_fd(r->gghttp);
			break;
	}		

	cond=G_IO_ERR|G_IO_HUP|G_IO_NVAL;
	if (r->gghttp->check&GG_CHECK_READ) cond|=G_IO_IN;
	if (r->gghttp->check&GG_CHECK_WRITE) cond|=G_IO_OUT;
	r->io_watch=g_io_add_watch(r->ioch,cond,request_io_handler,r);
	
	return FALSE;
}

Request * add_request(RequestType type,const char *from,const char *to,
			const char *id,xmlnode query,struct gg_http *gghttp,
			Stream *stream){
Request *r;
GIOCondition cond;	
	
	g_assert(gghttp!=NULL);
	r=g_new0(Request,1);
	r->type=type;
	r->id=g_strdup(id);
	r->from=g_strdup(from);
	if (to) r->to=g_strdup(to);
	else r->to=NULL;
	r->query=query;
	r->gghttp=gghttp;
	
	r->ioch=g_io_channel_unix_new(gghttp->fd);
	cond=G_IO_ERR|G_IO_HUP|G_IO_NVAL;
	if (r->gghttp->check&GG_CHECK_READ) cond|=G_IO_IN;
	if (r->gghttp->check&GG_CHECK_WRITE) cond|=G_IO_OUT;
	r->io_watch=g_io_add_watch(r->ioch,cond,request_io_handler,r);
	
	r->stream=stream;
	requests=g_list_append(requests,r);
	return r;
}

int remove_request(Request *r){

	if (!r) return -1;
	if (r->io_watch) g_source_remove(r->io_watch);
	requests=g_list_remove(requests,r);
	g_io_channel_close(r->ioch);
	if (r->from) g_free(r->from);	
	if (r->id) g_free(r->id);	
	if (r->query) xmlnode_free(r->query);
	if (r->gghttp) gg_http_free(r->gghttp);
	g_free(r);
	return 0;
}
