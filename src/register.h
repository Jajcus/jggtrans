/* $Id: register.h,v 1.7 2003/04/08 17:45:28 jajcus Exp $ */

/*
 *  (C) Copyright 2002-2005 Jacek Konieczny [jajcus(a)jajcus,net]
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

#ifndef register_h
#define register_h

void jabber_iq_get_register(Stream *s,const char *from,const char *to,const char *id,xmlnode q);
void jabber_iq_set_register(Stream *s,const char *from,const char *to,const char *id,xmlnode q);
int unregister(Stream *s,const char *from,const char *to,const char *id,int presence_used);

struct request_s;
int register_error(struct request_s *r);
int register_done(struct request_s *r);

int change_password_error(struct request_s *r);
int change_password_done(struct request_s *r);

#endif
