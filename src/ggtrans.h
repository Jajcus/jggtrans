/* $Id: ggtrans.h,v 1.8 2003/04/05 11:02:57 jajcus Exp $ */

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

/* dummy gettext shortcut for translating error/debug messages after log handler is set */
#define N_(String) gettext (String)

extern GMainLoop *main_loop;
extern gboolean do_restart;

#endif
