#include <stdio.h>
#include <unistd.h>
#include "fds.h"
#include "jabber.h"
#include "ggtrans.h"
#include "sessions.h"
#include "encoding.h"
#include <glib.h>
#include <signal.h>
#include <sys/wait.h>


static int the_end=0;
static int signal_received=0;

void sigchld_handler (int signum) {
int pid, status, serrno;
	serrno = errno;
	while (1) {
		   pid = waitpid (WAIT_ANY, &status, WNOHANG);
		   if (pid<=0)
		     break;
	}
	errno = serrno;
}


void signal_handler(int sig){

	switch(sig){
		case SIGINT:
		case SIGTERM:
			the_end=1;
			break;
		case SIGHUP:
		case SIGPIPE:
			signal(sig,signal_handler);
			break;
		default:
			break;
	}
	signal_received=sig;
}

xmlnode config;

int main(int argc,char *argv[]){

	config=xmlnode_file("/etc/jabber/ggtrans.xml");
	if (!config){
		fprintf(stderr,"Couldn't load config!\n");
		return 1;
	}

	fd_init();
	
	if (jabber_init()) return 1;
	if (sessions_init()) return 1;
	if (users_init()) return 1;
	if (encoding_init()) return 1;
		
	signal(SIGPIPE,signal_handler);		
	signal(SIGHUP,signal_handler);		
	signal(SIGINT,signal_handler);		
	signal(SIGTERM,signal_handler);		
	signal(SIGCHLD,sigchld_handler);		

	while(!the_end){
		fd_watch(10);
		if (signal_received){
			psignal(signal_received,"signal received");
			if (the_end) break;
			signal_received=0;
		}
		if (jabber_iter()) break;
		if (sessions_iter()) break;
	}
	
	sessions_done();
	users_done();
	jabber_done();

	return 0;
}

