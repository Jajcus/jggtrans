/* $Id: disco.h,v 1.3 2003/05/27 09:07:40 jajcus Exp $ */

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

#ifndef disco_h
#define disco_h

void jabber_iq_get_server_disco_items(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_server_disco_info(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_client_disco_items(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_client_disco_info(Stream *s,const char *from,const char * to,const char *id,xmlnode q);

#endif
