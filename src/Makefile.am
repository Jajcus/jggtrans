
sbin_PROGRAMS=jggtrans

jggtrans_SOURCES= \
	acl.c \
	acl.h \
	browse.c \
	browse.h \
	conf.c \
	conf.h \
	debug.h \
	disco.c \
	disco.h \
	enc_uni2win.h \
	enc_win2uni.h \
	encoding.c \
	encoding.h \
	forms.c \
	forms.h \
	gg_versions.c \
	gg_versions.h \
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
	stats.c \
	stats.h \
	status.c \
	status.h \
	stream.c \
	stream.h \
	users.c \
	users.h 

check_PROGRAMS = encodingtest
encodingtest_SOURCES = \
	enc_uni2win.h \
	enc_win2uni.h \
	encoding.c \
	encoding.h 
encodingtest_CFLAGS=-DENCODINGTEST
encodingtest_LDADD=$(GLIB_LIBS)

INCLUDES=$(GLIB_CFLAGS) $(JK_LIBGADU_CFLAGS) -I$(top_srcdir)/libxode/include

jggtrans_CFLAGS=-DVERSION="\"$(VERSION)\"" -DSYSCONFDIR=\"$(sysconfdir)\"
jggtrans_LDADD=$(GLIB_LIBS) $(JK_LIBGADU_CFLAGS) $(JK_LIBGADU_LIBS) $(top_builddir)/libxode/src/libxode.la


tags: 
	ctags -R . ../libxode/

.PHONY: valgrind

valgrind:
	valgrind --num-callers=20 --leak-check=yes --show-reachable=yes --logfile-fd=3 -v \
		--trace-children=no ./jggtrans -f -d 1 -D 1 3>jggtrans.valgrind
