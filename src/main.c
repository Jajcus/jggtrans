#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>
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

static FILE *log_file=NULL;
static gboolean use_syslog=FALSE;
static char *pid_filename=NULL;

static struct {
	const char *name;
	int code;
}facilitynames[] =
  {
    { "auth", LOG_AUTH },
    { "authpriv", LOG_AUTHPRIV },
    { "cron", LOG_CRON },
    { "daemon", LOG_DAEMON },
    { "ftp", LOG_FTP },
    { "kern", LOG_KERN },
    { "lpr", LOG_LPR },
    { "mail", LOG_MAIL },
    { "news", LOG_NEWS },
    { "syslog", LOG_SYSLOG },
    { "user", LOG_USER },
    { "uucp", LOG_UUCP },
    { "local0", LOG_LOCAL0 },
    { "local1", LOG_LOCAL1 },
    { "local2", LOG_LOCAL2 },
    { "local3", LOG_LOCAL3 },
    { "local4", LOG_LOCAL4 },
    { "local5", LOG_LOCAL5 },
    { "local6", LOG_LOCAL6 },
    { "local7", LOG_LOCAL7 },
    { NULL, -1 }
  };


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

void log_handler_file(FILE *f,const gchar *log_domain, GLogLevelFlags log_level, 
			const gchar *message){

	if (log_domain && log_domain[0]) fprintf(f,"%s: ",log_domain);
	switch(log_level){
		case G_LOG_LEVEL_ERROR:
			fprintf(f,"Fatal error: %s\n",message);
			break;
		case G_LOG_LEVEL_CRITICAL:
			fprintf(f,"Error: %s\n",message);
			break;
		case G_LOG_LEVEL_WARNING:
			fprintf(f,"Warning: %s\n",message);
			break;
		case G_LOG_LEVEL_MESSAGE:
			if (debug_level<-1) break;
		case G_LOG_LEVEL_INFO:
			if (debug_level<0) break;
			fprintf(f,"%s\n",message);
			break;
		case G_LOG_LEVEL_DEBUG:
			if (debug_level>0)
				fprintf(f,"Debug: %s\n",message);
			break;
		default:
			fprintf(f,"Unknown: %s\n",message);
			break;
	}
}

void log_handler_syslog(const gchar *log_domain, GLogLevelFlags log_level, 
			const gchar *message){

	switch(log_level){
		case G_LOG_LEVEL_ERROR:
			syslog(LOG_ERR,"Fatal error: %s",message);
			break;
		case G_LOG_LEVEL_CRITICAL:
			syslog(LOG_ERR,"Error: %s",message);
			break;
		case G_LOG_LEVEL_WARNING:
			syslog(LOG_WARNING,"Warning: %s",message);
			break;
		case G_LOG_LEVEL_MESSAGE:
			if (debug_level<-1) break;
			syslog(LOG_NOTICE,"%s",message);
			break;
		case G_LOG_LEVEL_INFO:
			if (debug_level<0) break;
			syslog(LOG_NOTICE,"%s",message);
			break;
		case G_LOG_LEVEL_DEBUG:
			if (debug_level>0)
				syslog(LOG_DEBUG,"Debug: %s\n",message);
			break;
		default:
			syslog(LOG_NOTICE,"Unknown: %s\n",message);
			break;
	}
}

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, 
			const gchar *message, gpointer user_data){

	log_level&=G_LOG_LEVEL_MASK;
	if (foreground) log_handler_file(stderr,log_domain,log_level,message);
	if (log_file) log_handler_file(log_file,log_domain,log_level,message);
	if (use_syslog) log_handler_syslog(log_domain,log_level,message);
}

void daemonize(){
pid_t pid;
pid_t sid;
int fd;

	debug("Daeminizing...");
	pid=fork();
	if (pid==-1) g_error("Failed to fork(): %s",g_strerror(errno));
	if (pid){
		if (pid_filename){
			FILE *f;
			f=fopen(pid_filename,"w");
			if (!f){
				g_critical("Couldn't write pid file, killing child: %s",g_strerror(errno));
				kill(pid,SIGTERM);
				exit(1);
			}
			fprintf(f,"%u",pid);
			fclose(f);
		}
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
	printf("\tUsage: %s [OPTIONS]... [<config file>]\n",name);
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
xmlnode tag;
char *log_type=NULL;
char *log_filename=NULL;
char *str;
char *config_file;
int log_facility=-1;
FILE *f;

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
	
	if (optind<argc-1){
		fprintf(stderr,"Unexpected argument: %s\n",argv[optind]);
		usage(argv[0]);
		return 1;
	}
	if (optind==argc-1) config_file=argv[optind];
	else config_file=g_strdup_printf("%s/%s",SYSCONFDIR,"ggtrans.xml");

	g_log_set_handler(NULL,G_LOG_FLAG_FATAL | G_LOG_LEVEL_ERROR
				| G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING 
				| G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO
				| G_LOG_LEVEL_DEBUG,log_handler,NULL);

	config=xmlnode_file(config_file);
	if (!config){
		g_error("Couldn't load config!");
		return 1;
	}
	str=xmlnode_get_name(config);
	if (!str || strcmp(str,"ggtrans")){
		g_error("/etc/jabber/ggtrans.xml doesn't look like ggtrans config file.");
		return 1;
	}
	
	for(tag=xmlnode_get_firstchild(config);tag;tag=xmlnode_get_nextsibling(tag)){
		str=xmlnode_get_name(tag);
		if (!str || strcmp(str,"log")) continue;
		log_type=xmlnode_get_attrib(tag,"type");
		if (!strcmp(log_type,"syslog")){
			if (log_facility!=-1){
				g_warning("Multiple syslog configs specified. Using only one.");
				continue;
			}
			str=xmlnode_get_attrib(tag,"facility");
			if (!str){
				log_facility=LOG_USER;
				continue;
			}
			for(log_facility=0;facilitynames[log_facility].name;log_facility++)
				if (!strcmp(facilitynames[log_facility].name,str)) break;
			if (!facilitynames[log_facility].name)
				 g_error("Unknown syslog facility: %s",str);
		}
		else if (!strcmp(log_type,"file")){
			if (log_filename) g_warning("Multiple log files specified. Using only one.");
			else log_filename=xmlnode_get_data(tag);
		}
		else g_warning("Ignoring unknown log type: %s",xmlnode2str(tag));
	}

	tag=xmlnode_get_tag(config,"pidfile");
	if (tag) pid_filename=xmlnode_get_data(tag);

	if (pid_filename){
		f=fopen(pid_filename,"r");
		if (f){
			pid_t pid;
			int r;
				
			r=fscanf(f,"%u",&pid);
			fclose(f);
			if (r==1 && pid>0){
				r=kill(pid,0);
				if (!r || (r && errno!=ESRCH)) g_error("ggtrans already running");
				if (r){
					g_warning("Stale pid file. Removing.");
					unlink(pid_filename);
				}
			}
			else g_error("Invalid pid file.");
		}
	}

	main_loop=g_main_new(0);
	
	if (jabber_init()) return 1;
	if (sessions_init()) return 1;
	if (users_init()) return 1;
	if (encoding_init()) return 1;
	
	if (!fg) daemonize();
	
	if (log_filename){
		log_file=fopen(log_filename,"a");
		if (!log_file) g_critical("Couldn't open log file '%s': %s",
						log_filename,g_strerror(errno));
		setvbuf(log_file,NULL,_IOLBF,0);
	}
	
	if (log_facility!=-1){
		openlog("ggtrans",0,log_facility);
		use_syslog=1;
	}
	
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

