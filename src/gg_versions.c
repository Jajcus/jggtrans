/* $Id: gg_versions.c,v 1.4 2004/06/11 07:31:04 smoku Exp $ */

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

#include <stdio.h>

/* Gadu-Gadu protocol=>version mapping (aproximations) */
char *gg_version[]={
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,	/* 0x00 - 0x06 */
	NULL,NULL,NULL,				/* 0x07 - 0x09 */
	"(WPKontakt)",				/* 0x0a */
	"4.0.2x",				/* 0x0b */
	NULL,NULL,NULL,				/* 0x0c - 0x0e */
	"4.5.1x",				/* 0x0f */
	"4.5.2x",				/* 0x10 */
	"4.6.x",				/* 0x11 */
	NULL,NULL,				/* 0x12 - 0x13 */
	"4.8.x",				/* 0x14 */
	"4.8.9",				/* 0x15 */
	"4.9.1",				/* 0x16 */
	"4.9.2",				/* 0x17 */
	"4.9.3-5.0.1",				/* 0x18 */
	"5.0.3",				/* 0x19 */
	"5.0.4",				/* 0x1a */
	"5.0.5",				/* 0x1b */
	"5.7 beta",				/* 0x1c */
	NULL,					/* 0x1d */
	"5.7 beta (build 121)",			/* 0x1e */
	NULL,					/* 0x1f */
	"6.0",					/* 0x20 */
	"6.0 (build 133)",			/* 0x21 */
	"6.0 (build 136,142)",			/* 0x22 */
        NULL,                                   /* 0x23 */
        "6.0 (build 156)",                      /* 0x24 */
        NULL,NULL,NULL                          /* 0x25 - 0x27 */
};
