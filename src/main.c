#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <limits.h>
#include "ggtrans.h"
#include "jabber.h"
#include "sessions.h"
#include "encoding.h"
#include "debug.h"

#ifndef OPEN_MAX
#define OPEN_MAX 255
#endif

xmlnode config;
GMainLoop *main_loop;

static int signal_received=FALSE;
static gboolean the_end=FALSE;
static gboolean foreground=TRUE;
static int debug_level=0;

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
			the_end=TRUE;
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

void log_handler_stderr(const gchar *log_domain, GLogLevelFlags log_level, 
			const gchar *message){

	if (log_domain && log_domain[0]) fprintf(stderr,"%s: ",log_domain);
	switch(log_level){
		case G_LOG_LEVEL_ERROR:
			fprintf(stderr,"Fatal error: %s\n",message);
			break;
		case G_LOG_LEVEL_CRITICAL:
			fprintf(stderr,"Error: %s\n",message);
			break;
		case G_LOG_LEVEL_WARNING:
			fprintf(stderr,"Warning: %s\n",message);
			break;
		case G_LOG_LEVEL_MESSAGE:
			if (debug_level<-1) break;
		case G_LOG_LEVEL_INFO:
			if (debug_level<0) break;
			fprintf(stderr,"%s\n",message);
			break;
		case G_LOG_LEVEL_DEBUG:
			if (debug_level>0)
				fprintf(stderr,"Debug: %s\n",message);
			break;
		default:
			fprintf(stderr,"Unknown: %s\n",message);
			break;
	}
}

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, 
			const gchar *message, gpointer user_data){

	log_level&=G_LOG_LEVEL_MASK;
	if (foreground) log_handler_stderr(log_domain,log_level,message);
}

void daemonize(){
pid_t pid;
pid_t sid;
int fd;

	debug("Daeminizing...");
	pid=fork();
	if (pid==-1) g_error("Failed to fork(): %s",g_strerror(errno));
	if (pid){
		debug("Daemon born, pid %i.",pid);
		exit(0);
	}

        for (fd=0; fd < OPEN_MAX; fd++)
                close(fd);

        fd = open("/dev/null", O_RDWR);
        if (fd) {
                if (fd != 0)
                        dup2(fd, 0);
                if (fd != 1)
                        dup2(fd, 1);
                if (fd != 2)
                        dup2(fd, 2);
                if (fd > 2)
                        close(fd);
        }

        sid=setsid();
	if (sid==-1) abort();
	foreground=FALSE;
	debug("I am a daemon, I think.");
	return;
}

void usage(const char *name){
char *p;

	p=strrchr(name,'/');
	if (p) name=p+1;
	printf("\nJabber GaduGadu Transport %s\n",VERSION);
	printf("\n");
	printf("\tUsage: %s [OPTIONS]...\n",name);
	printf("\nOptions:\n");
	printf("\t-h  This message\n");
	printf("\t-f  Run in foreground. Debug/error messages will be sent to stderr\n");
#ifdef DEBUG
	printf("\t-d  Enable debugging messages\n");
#endif
	printf("\t-D  Enable libgg debugging messages (enables also -f)\n");
	printf("\n");
}

int main(int argc,char *argv[]){
int c;
gboolean fg=FALSE;

	opterr=0;

	while ((c = getopt (argc, argv, "hfd:D:")) != -1){
		switch(c){
			case 'h':
				usage(argv[0]);
				return 0;
			case 'f':
				fg=TRUE;
				break;
			case 'd':
				debug_level=atoi(optarg);
				break;
			case 'D':
				gg_debug_level=atoi(optarg);
				fg=TRUE;
				break;
			case '?':
				if (isprint(optopt))
					fprintf(stderr,"Unknown command-line option: -%c.\n",optopt);
				else
					fprintf(stderr,"Unknown command-line option: -\\%03o.\n",optopt);
				usage(argv[0]);
				return 1;
			default:
				g_error("Error while processing command line options");
				break;
		}	
	}
	
	if (optind<argc){
		fprintf(stderr,"Unexpected argument: %s\n",argv[optind]);
		usage(argv[0]);
		return 1;
	}
	
	config=xmlnode_file("/etc/jabber/ggtrans.xml");
	if (!config){
		g_error("Couldn't load config!");
		return 1;
	}

	g_log_set_handler(NULL,G_LOG_FLAG_FATAL | G_LOG_LEVEL_ERROR
				| G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING 
				| G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO
				| G_LOG_LEVEL_DEBUG,log_handler,NULL);

	main_loop=g_main_new(0);
	
	if (jabber_init()) return 1;
	if (sessions_init()) return 1;
	if (users_init()) return 1;
	if (encoding_init()) return 1;
	
	if (!fg) daemonize();
	
	if (jabber_connect()) return 1;

	g_source_add(G_PRIORITY_HIGH,0,&signal_source_funcs,NULL,NULL,NULL);
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

