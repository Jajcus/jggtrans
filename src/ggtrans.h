/* $Id: ggtrans.h,v 1.12 2003/04/22 08:44:29 jajcus Exp $ */

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

#ifndef ggtrans_h
#define ggtrans_h

#include <libxode.h>
#include <libintl.h>
#include <glib.h>
#include "../config.h"

/* gettext shortcut for translating messages */
#define _(String) gettext (String)

/* dummy gettext shortcut for translations out-of-place */
#define N_(String) (String)

/* gettext shortcut for translating error/debug messages (locale/encoding must be switched) */
#define L_(String) local_translate(String)

const char *local_translate(const char *str);

extern GMainLoop *main_loop;
extern gboolean do_restart;
extern GList *admins;

extern time_t start_time;
extern unsigned long packets_in;
extern unsigned long packets_out;
extern unsigned long gg_messages_in;
extern unsigned long gg_messages_out;

#endif
