/* $Id: debug.h,v 1.3 2002/01/30 16:52:03 jajcus Exp $ */

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

#ifndef debug_h
#define debug_h

#ifdef DEBUG
# if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define debug(...)    g_log (G_LOG_DOMAIN,         \
                               G_LOG_LEVEL_DEBUG,    \
                               __VA_ARGS__)
# elif defined (__GNUC__)
#  define debug(format...)      g_log (G_LOG_DOMAIN,         \
                                       G_LOG_LEVEL_DEBUG,    \
                                       format)
# endif
#else
# define debug
#endif

#endif
