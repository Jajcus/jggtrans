#include <stdio.h>
#include <unistd.h>
#include "fds.h"
#include "jabber.h"
#include "ggtrans.h"
#include "sessions.h"
#include <glib.h>
#include <signal.h>

static int the_end=0;

xmlnode config;

int stdin_handler(fd_handler *h){
char t;

	if (h->read_ready){
		read(0,&t,1);
		if (t=='q') the_end=1;
	}
	return 0;
}

int main(int argc,char *argv[]){
fd_handler *stdin_h;

	config=xmlnode_file("/etc/jabber/ggtrans.xml");
	if (!config){
		fprintf(stderr,"Couldn't load config!\n");
		return 1;
	}

	fd_init();
	
	stdin_h=(fd_handler *)g_malloc(sizeof(fd_handler));
	stdin_h->fd=0;
	stdin_h->func=stdin_handler;
	stdin_h->read=1;
	stdin_h->write=0;

	
	if (fd_register_handler(stdin_h)) return 1;

	if (jabber_init()) return 1;
	if (users_init()) return 1;
	if (sessions_init()) return 1;
		
	signal(SIGPIPE,SIG_IGN);		

	while(!the_end){
		fd_watch(10);
		if (jabber_iter()) break;
	}

	jabber_done();

	return 0;
}

