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
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems.h>
#include <rtems/error.h>
#include <rtems/pty.h>
#include <rtems/shell.h>
#include <rtems/telnetd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <cexp.h>
#include <rtems/userenv.h>
#include <rtems/error.h>

static void
cexpWrap(char *dev, void *arg)
{
char	*args[]={"Cexp-telnet",0};
	fprintf(stderr,"[Telnet:] starting cexp on %s\n",dev);
	cexp_main(1,args);
}
/***********************************************************/
rtems_id            telnetd_task_id      =0;
rtems_unsigned32    telnetd_stack_size   =32000;
rtems_task_priority telnetd_task_priority=100;
void				(*telnetd_shell)(char *, void*)=cexpWrap;
void				*telnetd_shell_arg	 =0;
int					telnetd_dont_spawn	 =0;


char * (*do_get_pty)(int)=0;


static rtems_task
spawned_shell(rtems_task_argument arg);

/***********************************************************/
rtems_task
rtems_task_telnetd(rtems_task_argument task_argument)
{
int					des_socket, acp_socket;
struct sockaddr_in	srv;
char				*devname;
int					i=1;
int					size_adr;
rtems_id 			task_id;	/* unused */
rtems_status_code	sc;

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
	  acp_socket=accept(des_socket,(struct sockaddr*)&srv,&size_adr);
	  if (acp_socket<0) {
		perror("telnetd:accept");
		break;
	  };
	  if (do_get_pty && (devname = do_get_pty(acp_socket)) ) {
#if 0
			  FILE *f;
			  if (!(f=fopen(devname,"rw"))) {
					  perror("opening PTY");
			  } else {
				int ch;
				while (EOF!=(ch=fgetc(f)) && 4!=ch) {
						printk("%c",ch);
				}
				printk("\nEOF on PTY\n");
			    fclose(f);
				printk("closed PTY\n");
			  }
			  close(acp_socket);
#else
			  if (telnetd_dont_spawn) {
					telnetd_shell(devname, telnetd_shell_arg);
			  } else if ((sc=rtems_task_create(
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
						(rtems_task_argument)devname))) {
				rtems_error(sc,"Telnetd: spawning child task");
				close(acp_socket);
			  }
#if 0
	   shell_init(&devname[5],
		      telnetd_stack_size,
		      telnetd_task_priority,
		      devname,B9600|CS8,FALSE);
#endif
#endif
	  } else {
			if (!do_get_pty) {
				printk("PTY driver probably not registered\n");
			}
           close(acp_socket);
	  };
	} while(1);
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
int startTelnetd(void (*cmd)(char *, void *), void *arg, int dontSpawn, int stack, int priority)
{
	rtems_status_code	sc;

	if (telnetd_task_id) {
		fprintf(stderr,"ERROR:telnetd already started\n");
		return 1;
	};

	if (cmd)
		telnetd_shell=cmd;
	telnetd_shell_arg=arg;
	telnetd_dont_spawn=dontSpawn;
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
spawned_shell(rtems_task_argument arg)
{
rtems_status_code	sc;
FILE				*std[3]={0};
int					i;
char				*devname=(char*)arg;

	sc=rtems_libio_set_private_env();

	if (RTEMS_SUCCESSFUL != sc) {
		rtems_error(sc,"rtems_libio_set_private_env");
		goto cleanup;
	}

	/* redirect stdio */
	for (i=0; i<3; i++) {
		if ( !(std[i]=fopen(devname,"r+")) ) {
			perror("unable to open stdio");
			goto cleanup;
		}
	}
	stdin  = std[0];
	stdout = std[1];
	stderr = std[2];

	/* call their routine */
	telnetd_shell(devname, telnetd_shell_arg);

cleanup:
	for (i=0; i<3; i++) {
		if (std[i])
			fclose(std[i]);
	}
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
