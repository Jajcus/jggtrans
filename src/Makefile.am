
bin_PROGRAMS=jggtrans

jggtrans_SOURCES= \
	debug.h \
	encoding.c \
	encoding.h \
	ggtrans.h \
	iq.c \
	iq.h \
	jabber.c \
	jabber.h \
	jid.c \
	jid.h \
	main.c \
	message.c \
	message.h \
	presence.c \
	presence.h \
	register.c \
	register.h \
	requests.c \
	requests.h \
	search.c \
	search.h \
	sessions.c \
	sessions.h \
	stream.c \
	stream.h \
	users.c \
	users.h 

INCLUDES=$(GLIB_CFLAGS) -I../libxode/include

jggtrans_CFLAGS=-DVERSION="\"$(VERSION)\""
jggtrans_LDADD=$(GLIB_LIBS) ../libxode/src/libxode.la
