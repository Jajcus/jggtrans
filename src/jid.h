/* $Id: jid.h,v 1.5 2003/05/09 10:32:59 jajcus Exp $ */

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

#ifndef jid_h
#define jid_h

int jid_is_my(const char *jid);
int jid_is_me(const char *jid);
int jid_has_uin(const char *jid);
int jid_get_uin(const char *jid);
const char *jid_get_resource(const char *jid);

/* Functions below return strings which must be freed */

/* returns uncapitalized user@host part of given jid */
char * jid_normalized(const char *jid);
char * jid_my_registered();
char * jid_build(long unsigned int uin);
char * jid_build_full(long unsigned int uin);

#endif
