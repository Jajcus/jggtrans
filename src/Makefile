CC=gcc
CFLAGS:=$(CFLAGS) -Wall $(OPTFLAGS) -g2 -ggdb 
INCLUDES=-I/usr/include/jabberd/ `glib-config --cflags` -I./libxode/include
LIBS= ./libxode/src/.libs/libxode.a -lgg `glib-config --libs` -lefence

ggtrans_OBJECTS= \
	jabber.o \
	fds.o \
	main.o \
	stream.o \
	iq.o \
	register.o \
	users.o \
	sessions.o \
	presence.o \
	jid.o \
	message.o \
	requests.o \
	search.o \
	encoding.o


ggtrans_HEADERS= \
	jabber.h \
	fds.h \
	ggtrans.h \
	stream.h \
	iq.h \
	register.h \
	users.h \
	sessions.h \
	presence.h \
	jid.h \
	message.h \
	requests.h \
	search.h \
	encoding.h

all: jggtrans

dep:
	$(CC) $(INCLUDES) -MM *.c > .deps

jggtrans: $(ggtrans_OBJECTS) $(ggtrans_HEADERS)
	$(CC) $(CFLAGS) -o jggtrans $(ggtrans_OBJECTS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(ggtrans_OBJECTS) jggtrans

.c.o:
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $< 

include .deps
