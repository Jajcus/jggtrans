INCLUDES = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libxode.la

libxode_la_SOURCES = \
	xstream.c \
	expat.c \
	genhash.c \
	pool.c \
	sha.c \
	snprintf.c \
	socket.c \
	str.c \
	xmlnode.c

AM_CFLAGS = -Wall -g

EXTRA_DIST = xmltok_impl.c xmltok_ns.c
libxode_la_CFLAGS = $(EXPAT_CFLAGS)
libxode_la_LDFLAGS = -version-info 2:0:1 $(EXPAT_LIBS)
