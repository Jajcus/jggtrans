
sbin_PROGRAMS=jggtrans

jggtrans_SOURCES= \
	browse.c \
	browse.h \
	conf.c \
	conf.h \
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
	status.c \
	status.h \
	stream.c \
	stream.h \
	users.c \
	users.h 

INCLUDES=$(GLIB_CFLAGS) -I../libxode/include

jggtrans_CFLAGS=-DVERSION="\"$(VERSION)\"" -DSYSCONFDIR=\"$(sysconfdir)\"
jggtrans_LDADD=$(GLIB_LIBS) ../libxode/src/libxode.la

tags: 
	ctags -R . ../libxode/
