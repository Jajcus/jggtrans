/* $Id: jabber.h,v 1.8 2002/01/30 16:52:03 jajcus Exp $ */

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

#ifndef jabber_h
#define jabber_h

#include "stream.h"
	
int jabber_init();
int jabber_connect();
int jabber_done();
int jabber_iter();

struct stream_s * jabber_stream();

enum jabber_state_e{
	JS_NONE,
	JS_HANDSHAKE,
	JS_CONNECTED	
};
extern enum jabber_state_e jabber_state;


extern const char *my_name;
extern const char *register_instructions;
extern const char *search_instructions;
extern const char *gateway_desc;
extern const char *gateway_prompt;

#endif
