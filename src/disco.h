/* $Id: disco.h,v 1.2 2003/05/27 07:52:36 jajcus Exp $ */

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

#ifndef disco_h
#define disco_h

void jabber_iq_get_server_disco_items(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_server_disco_info(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_client_disco_items(Stream *s,const char *from,const char * to,const char *id,xmlnode q);
void jabber_iq_get_client_disco_info(Stream *s,const char *from,const char * to,const char *id,xmlnode q);

struct session_s *sess;
char * get_user_disco_string(const struct session_s *sess);

#endif
