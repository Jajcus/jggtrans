# $Id: Makefile.in,v 1.54 2005/01/21 16:25:21 wojtekka Exp $

EXTRA_DIST = libgadu-config.h.in

noinst_LTLIBRARIES = libgadu.la
libgadu_la_SOURCES = common.c libgadu.c pubdir.c http.c dcc.c events.c pubdir50.c obsolete.c
noinst_HEADERS = compat.h libgadu.h

libgadu_la_CFLAGS = $(CFLAGS_LIBGADU)
libgadu_la_LDFLAGS = $(LIBS_LIBGADU)

DISTCLEANFILES = libgadu-config.h

