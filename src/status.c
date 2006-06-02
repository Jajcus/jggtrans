/* $Id: status.c,v 1.8 2003/01/15 08:04:56 jajcus Exp $ */

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
#include <libgadu.h>
#include "status.h"

int status_jabber_to_gg(int available,const char *show,const char *status){

	if (status){
		if (available==-1) return GG_STATUS_INVISIBLE_DESCR;
		if (!available) return GG_STATUS_NOT_AVAIL_DESCR;
		else if (!show) return GG_STATUS_AVAIL_DESCR;
		else if (!strcmp(show,"away")) return GG_STATUS_BUSY_DESCR;
		else if (!strcmp(show,"dnd")) return GG_STATUS_BUSY_DESCR;
		else if (!strcmp(show,"xa")) return GG_STATUS_BUSY_DESCR;
		return GG_STATUS_AVAIL_DESCR;
	}
	else{
		if (available==-1) return GG_STATUS_INVISIBLE;
		if (!available) return GG_STATUS_NOT_AVAIL;
		else if (!show) return GG_STATUS_AVAIL;
		else if (!strcmp(show,"away")) return GG_STATUS_BUSY;
		else if (!strcmp(show,"dnd")) return GG_STATUS_BUSY;
		else if (!strcmp(show,"xa")) return GG_STATUS_BUSY;
	}

	return GG_STATUS_AVAIL;
}

int status_gg_to_jabber(int ggstatus,char **show,char **status){
int available;

	switch(ggstatus){
		case GG_STATUS_NOT_AVAIL:
		case GG_STATUS_NOT_AVAIL_DESCR:
			available=0;
			*show=NULL;
			break;
		case GG_STATUS_AVAIL:
		case GG_STATUS_AVAIL_DESCR:
			available=1;
			*show=NULL;
			break;
		case GG_STATUS_BUSY:
		case GG_STATUS_BUSY_DESCR:
			available=1;
			*show="xa";
			break;
		default:
			available=1;
			*show=NULL;
			break;
	}
	return available;
}


