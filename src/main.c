#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "ggtrans.h"
#include "jabber.h"
#include "sessions.h"
#include "encoding.h"

xmlnode config;
GMainLoop *main_loop;

static int signal_received=0;
static int the_end=0;

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

gboolean signal_source_prepare(gpointer  source_data,
				GTimeVal *current_time,
				gint     *timeout,
				gpointer  user_data){

	*timeout=1000;
	if (signal_received) return TRUE;
	return FALSE;
}

gboolean signal_source_check(gpointer  source_data,
                        	GTimeVal *current_time,
                        	gpointer  user_data){

	if (signal_received) return TRUE;
	return FALSE;
}

gboolean signal_source_dispatch(gpointer  source_data,
                        GTimeVal *current_time,
                        gpointer  user_data){
	
	psignal(signal_received,"signal received");
	g_warning("Signal received: %s",g_strsignal(signal_received));
	if (the_end) g_main_quit(main_loop);
	signal_received=0;
	return TRUE;
}

void signal_source_destroy(gpointer user_data){
}

static GSourceFuncs signal_source_funcs={
		signal_source_prepare,
		signal_source_check,
		signal_source_dispatch,
		signal_source_destroy
		};


int main(int argc,char *argv[]){

	config=xmlnode_file("/etc/jabber/ggtrans.xml");
	if (!config){
		g_error("Couldn't load config!");
		return 1;
	}

	main_loop=g_main_new(0);
	g_source_add(G_PRIORITY_HIGH,0,&signal_source_funcs,NULL,NULL,NULL);
	
	if (jabber_init()) return 1;
	if (sessions_init()) return 1;
	if (users_init()) return 1;
	if (encoding_init()) return 1;
		
	signal(SIGPIPE,signal_handler);		
	signal(SIGHUP,signal_handler);		
	signal(SIGINT,signal_handler);		
	signal(SIGTERM,signal_handler);		
	signal(SIGCHLD,sigchld_handler);		

	g_main_run(main_loop);
	
	sessions_done();
	users_done();
	jabber_done();

	g_main_destroy(main_loop);

	return 0;
}

