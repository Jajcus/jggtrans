INCLUDES = -I$(top_builddir)/include

noinst_LTLIBRARIES = libxode.la

libxode_la_SOURCES = \
	xstream.c \
	expat.c \
	genhash.c \
	hashtable.c \
	pool.c \
	sha.c \
	snprintf.c \
	socket.c \
	str.c \
	xmlnode.c \
	xmlparse.c \
	xmlrole.c \
	xmltok.c

CFLAGS = -Wall -g

EXTRA_DIST = xmltok_impl.c xmltok_ns.c
libxode_la_LDFLAGS = -version-info 2:0:1
