/* $Id: iq.h,v 1.5 2002/01/30 16:52:03 jajcus Exp $ */

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

#ifndef iq_h
#define iq_h

struct stream_s;
void jabber_iq(struct stream_s *s,xmlnode x);
void jabber_iq_send_error(struct stream_s *s,const char *from,const char *to,const char *id,int code,char *string);
void jabber_iq_send_result(struct stream_s *s,const char *from,const char *to,const char *id,xmlnode content);

#endif
