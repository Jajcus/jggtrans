/* $Id: jid.h,v 1.6 2004/04/13 17:44:07 jajcus Exp $ */

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

#ifndef jid_h
#define jid_h

int jid_is_my(const char *jid);
int jid_is_me(const char *jid);
int jid_has_uin(const char *jid);
int jid_get_uin(const char *jid);
const char *jid_get_resource(const char *jid);

/* Functions below return strings which must be freed */

/* returns normalized (all parts stringpreped) jid. Bare jid (no resource) is returned if full=0 */
char * jid_normalized(const char *jid,int full);
char * jid_my_registered();
char * jid_build(long unsigned int uin);
char * jid_build_full(long unsigned int uin);

#endif
