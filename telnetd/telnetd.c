/***********************************************************/
/*
 *
 *  The telnet DAEMON
 *
 *  Author: 17,may 2001
 *
 *   WORK: fernando.ruiz@ctv.es 
 *   HOME: correo@fernando-ruiz.com
 *
 * After start the net you can start this daemon.
 * It uses the previously inited pseudo-terminales (pty.c)
 * getting a new terminal with getpty(). This function
 * gives a terminal name passing a opened socket like parameter.
 *
 * With register_telnetd() you add a new command in the shell to start
 * this daemon interactively. (Login in /dev/console of course)
 * 
 * Sorry but OOB is not still implemented. (This is the first version)
 *
 * Till Straumann <strauman@slac.stanford.edu>
 *  - made the 'shell' interface more generic, i.e. it is now
 *    possible to have 'telnetd' run an arbitrary 'shell'
 *    program. The default, however, is CEXP.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems.h>
#include <rtems/error.h>
#include <rtems/pty.h>
#include <rtems/shell.h>
#include <rtems/telnetd.h>
#include <rtems/bspIo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <syslog.h>

#include <cexp.h>
#include <rtems/userenv.h>
#include <rtems/error.h>

#define PARANOIA

struct shell_args {
	char	*devname;
	void	*arg;
	char	peername[16];
};

static void cexpWrap(char *dev, void *arg);
static int sockpeername(int sock, char *buf, int bufsz);

/***********************************************************/
rtems_id            telnetd_task_id      =0;
rtems_unsigned32    telnetd_stack_size   =32000;
rtems_task_priority telnetd_task_priority=100;
void				(*telnetd_shell)(char *, void*)=cexpWrap;
void				*telnetd_shell_arg	 =0;

static rtems_id		connLimit			 =0;

char * (*do_get_pty)(int)=0;

static void
cexpWrap(char *dev, void *arg)
{
char	*args[]={"Cexp-telnet",0};
	fprintf(stderr,"[Telnet:] starting cexp on %s\n",dev);
	cexp_main(1,args);
}

static char *grab_a_Connection(int des_socket, struct sockaddr_in *srv, char *peername, int sz)
{
char	*rval = 0;
int		size_adr = sizeof(*srv);
int		acp_sock;

	/* wait until the number of active connections drops */
	if (connLimit)
		rtems_semaphore_obtain(connLimit, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

	acp_sock = accept(des_socket,(struct sockaddr*)srv,&size_adr);

	if (acp_sock<0) {
		perror("telnetd:accept");
		goto bailout;
	};

	if ( !(rval=do_get_pty(acp_sock)) ) {
		syslog( LOG_DAEMON | LOG_ERR, "telnetd: unable to obtain PTY");
		/* NOTE: failing 'do_get_pty()' closed the socket */
		goto bailout;
	}

	if (sockpeername(acp_sock, peername, sz))
		strncpy(peername, "<UNKNOWN>", sz);

#ifdef PARANOIA
	syslog(LOG_DAEMON | LOG_INFO,
			"telnetd: accepted connection from %s on %s",
			peername,
			rval);
#endif

bailout:

	if (!rval && connLimit) {
		rtems_semaphore_release(connLimit);
	}
			
	return rval;
}


static void release_a_Connection(char *devname, char *peername, FILE **std, int n)
{

#ifdef PARANOIA
	syslog( LOG_DAEMON | LOG_INFO,
			"telnetd: releasing connection from %s on %s",
			peername,
			devname );
#endif

	while (--n>=0)
		if (std[n]) fclose(std[n]);

	if (connLimit)
			rtems_semaphore_release(connLimit);
}

static int sockpeername(int sock, char *buf, int bufsz)
{
struct sockaddr_in peer;
int len  = sizeof(peer);

int rval = sock < 0;

	if ( !rval)
		rval = getpeername(sock, (struct sockaddr*)&peer, &len);

	if ( !rval )
		rval = !inet_ntop( AF_INET, &peer.sin_addr, buf, bufsz );

	return rval;
}

#if 1
#define INSIDE_TELNETD
#include "check_passwd.c"
#else
#define check_passwd(arg) 0
#endif


static rtems_task
spawned_shell(rtems_task_argument arg);

/***********************************************************/
rtems_task
rtems_task_telnetd(rtems_task_argument task_argument)
{
int					des_socket;
struct sockaddr_in	srv;
char				*devname;
char				peername[16];
int					i=1;
int					size_adr;
rtems_id 			task_id;	/* unused */
rtems_status_code	sc;
struct shell_args	*arg;

	if ((des_socket=socket(PF_INET,SOCK_STREAM,0))<0) {
		perror("telnetd:socket");
		telnetd_task_id=0;
		rtems_task_delete(RTEMS_SELF);
	};
	setsockopt(des_socket,SOL_SOCKET,0,&i,sizeof(i));
	memset(&srv,0,sizeof(srv));
	srv.sin_family=AF_INET;
	srv.sin_port=htons(23);
	size_adr=sizeof(srv);
	if ((bind(des_socket,(struct sockaddr *)&srv,size_adr))<0) {
		perror("telnetd:bind");
	        close(des_socket);
		telnetd_task_id=0;
		rtems_task_delete(RTEMS_SELF);
	};
	if ((listen(des_socket,5))<0) {
		perror("telnetd:listen");
	        close(des_socket);
		telnetd_task_id=0;
		rtems_task_delete(RTEMS_SELF);
	};

	/* we don't redirect stdio as this probably
	 * was started from the console anyways..
	 */
	do {
	  devname = grab_a_Connection(des_socket, &srv, peername, sizeof(peername));

	  if ( !devname ) {
		/* if something went wrong, sleep for some time */
		sleep(10);
		continue;
	  }
	  if ( ! connLimit ) {
		/* no limit was set; this means we should execute
		 * the shell in 'telnetd' context...
		 */
		if ( 0 == check_passwd(peername) )
			telnetd_shell(devname, telnetd_shell_arg);
	  } else {
		arg = malloc( sizeof(*arg) );

		arg->devname = devname;
		arg->arg = telnetd_shell_arg;
		strncpy(arg->peername, peername, sizeof(arg->peername));

		if ((sc=rtems_task_create(
				rtems_build_name(
						devname[5],
						devname[6],
						devname[7],
						devname[8]),
				telnetd_task_priority,
				telnetd_stack_size,
				RTEMS_DEFAULT_MODES,
				RTEMS_LOCAL | RTEMS_FLOATING_POINT,
				&task_id)) ||
			(sc=rtems_task_start(
				task_id,
				spawned_shell,
				(rtems_task_argument)arg))) {

			FILE *dummy;

			rtems_error(sc,"Telnetd: spawning child task");
			/* hmm - the pty driver slot can only be
			 * released by opening and subsequently
			 * closing the PTY - this also closes
			 * the underlying socket. So we mock up
			 * a stream...
			 */

			if ( !(dummy=fopen(devname,"r+")) )
				perror("Unable to dummy open the pty, losing a slot :-(");
			release_a_Connection(devname, peername, &dummy, 1);
			free(arg);
			sleep(2); /* don't accept connections too fast */
  		}
	  }
	} while(1);
	/* TODO: how to free the connection semaphore? But then - 
	 *       stopping the daemon is probably only needed during
	 *       development/debugging.
	 *       Finalizer code should collect all the connection semaphore
	 *       counts and eventually clean up...
	 */
	close(des_socket);
	telnetd_task_id=0;
	rtems_task_delete(RTEMS_SELF);
}
/***********************************************************/
int rtems_initialize_telnetd(void) {
	rtems_status_code sc;
	
#if 0
	void register_icmds(void);
	register_icmds(); /* stats for tcp/ip */
#endif
	
	if (telnetd_task_id         ) return RTEMS_RESOURCE_IN_USE;
	if (telnetd_stack_size<=0   ) telnetd_stack_size   =32000;
	if (telnetd_task_priority<=2) telnetd_task_priority=100;
	sc=rtems_task_create(rtems_build_name('t','n','t','d'),
			     100,RTEMS_MINIMUM_STACK_SIZE,	
			     RTEMS_DEFAULT_MODES,
			     RTEMS_DEFAULT_ATTRIBUTES,
			     &telnetd_task_id);
        if (sc!=RTEMS_SUCCESSFUL) {
		rtems_error(sc,"creating task telnetd");
		return (int)sc;
	};
	sc=rtems_task_start(telnetd_task_id,
			    rtems_task_telnetd,
			    (rtems_task_argument)NULL);
        if (sc!=RTEMS_SUCCESSFUL) {
		rtems_error(sc,"starting task telnetd");
	};
	return (int)sc;
}
/***********************************************************/
int startTelnetd(void (*cmd)(char *, void *), void *arg, int maxNumConnections, int stack, int priority)
{
	rtems_status_code	sc;

	if (telnetd_task_id) {
		fprintf(stderr,"ERROR:telnetd already started\n");
		return 1;
	};

	if ( !do_get_pty ) {
		fprintf(stderr,"PTY driver probably not registered\n");
		return 1;
	}

	if (cmd)
		telnetd_shell=cmd;
	telnetd_shell_arg=arg;
	/* Set the default maximal number of simultaneous connections
     * This parameter means:
	 *  maxNumConnections == 0 --> select default.
	 *  maxNumConnections > 0  set limit of simultanously open connections.
	 *  maxNumConnections < 0  dont spawn the shell but execute it in telnetd's
	 *                         context.
     */
	if (0 == maxNumConnections)
		maxNumConnections = 3;
	if (maxNumConnections > 0) {
		if (maxNumConnections > MAX_PTYS)
			maxNumConnections = MAX_PTYS;
		assert( RTEMS_SUCCESSFUL ==
				rtems_semaphore_create(
					rtems_build_name('t','n','t','d'),
					maxNumConnections,
					RTEMS_FIFO | RTEMS_COUNTING_SEMAPHORE | 
					RTEMS_NO_INHERIT_PRIORITY | RTEMS_LOCAL |
					RTEMS_NO_PRIORITY_CEILING,
					0,
					&connLimit) );
	}
	telnetd_stack_size=stack;
	telnetd_task_priority=priority;

	sc=rtems_initialize_telnetd();
        if (sc!=RTEMS_SUCCESSFUL) return sc;
	printf("rtems_telnetd() started with stacksize=%u,priority=%d\n",
                        telnetd_stack_size,telnetd_task_priority);
	return 0;
}
/***********************************************************/
#if 0
int register_telnetd(void) {
	shell_add_cmd("telnetd","telnet","telnetd [stacksize [tsk_priority]]",main_telnetd);
	return 0;
}
#endif
/***********************************************************/

/* utility wrapper */
static rtems_task
spawned_shell(rtems_task_argument targ)
{
rtems_status_code	sc;
FILE				*std[3]={0};
int					i;
struct shell_args	*arg = (struct shell_args *)targ;

	sc=rtems_libio_set_private_env();

	if (RTEMS_SUCCESSFUL != sc) {
		rtems_error(sc,"rtems_libio_set_private_env");
		goto cleanup;
	}

	/* redirect stdio */
	for (i=0; i<3; i++) {
		if ( !(std[i]=fopen(arg->devname,"r+")) ) {
			perror("unable to open stdio");
			goto cleanup;
		}
	}
	stdin  = std[0];
	stdout = std[1];
	stderr = std[2];

	/* call their routine */
	if ( 0 == check_passwd(arg->peername) )
		telnetd_shell(arg->devname, arg->arg);

cleanup:
	release_a_Connection(arg->devname, arg->peername, std, 3);
	free(arg);
	rtems_task_delete(RTEMS_SELF);
}

/* convenience routines for CEXP (retrieve stdio descriptors
 * from reent structure)
 *
 */
#ifdef stdin
static __inline__ FILE *
_stdin(void)  { return stdin; }
#undef stdin
FILE *stdin(void)  { return _stdin(); }
#endif
#ifdef stdout
static __inline__ FILE *
_stdout(void) { return stdout; }
#undef stdout
FILE *stdout(void) { return _stdout(); }
#endif
#ifdef stderr
static __inline__ FILE *
_stderr(void) { return stderr; }
#undef stderr
FILE *stderr(void) { return _stderr(); }
#endif

/* MUST NOT USE stdin & friends below here !!!!!!!!!!!!! */
