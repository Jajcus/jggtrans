/* $Id: ggtrans.h,v 1.13 2004/03/05 08:39:41 jajcus Exp $ */

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

#ifndef ggtrans_h
#define ggtrans_h

#include <libxode.h>
#include <libintl.h>
#include <glib.h>
#include "../config.h"

/* older glib versions don't define g_log() */
#ifndef g_debug
#define g_debug(format...)      g_log (G_LOG_DOMAIN,         \
                                       G_LOG_LEVEL_DEBUG,    \
                                       format)
#endif

#ifdef ENABLE_NLS
/* gettext shortcut for translating messages */
#  define _(String) gettext (String)

/* dummy gettext shortcut for translations out-of-place */
#  define N_(String) (String)

/* gettext shortcut for translating error/debug messages (locale/encoding must be switched) */
#  define L_(String) local_translate(String)
const char *local_translate(const char *str);
#else
#  define _(String) String
#  define N_(String) String
#  define L_(String) String
#endif


extern GMainLoop *main_loop;
extern gboolean do_restart;
extern GList *admins;

extern time_t start_time;
extern unsigned long packets_in;
extern unsigned long packets_out;
extern unsigned long gg_messages_in;
extern unsigned long gg_messages_out;

#endif
