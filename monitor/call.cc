#include <rtems.h>
#include <rtems/monitor.h>
#include <rtems/cpuuse.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <libtecla.h>

#ifdef __cplusplus
extern "C" {
#endif

static rtems_id monitorMutex=0;

extern rtems_monitor_command_entry_t rtems_monitor_commands[];

int
taskStack(rtems_id thread_id);

int
memUsageDump(void);

static void
fnwrap(int argc, char **argv, unsigned32 arg, boolean verbose)
{
unsigned long	iarg;
char			*endp;
if ( argc > 0 ) {
		fprintf(stderr,"1 arg %s\n",argv[1]);
	if (argv[1]) {
		iarg=strtol(argv[1],&endp,0);
		if (!*argv[1] || *endp) {
			fprintf(stderr,"Invalid Argument\n");
			return;
		}
	} else {
		iarg=0;
	}
	((void (*)(unsigned long))arg)(iarg);
} else {
		fprintf(stderr,"no args\n");
	((void (*)(void))arg)();
}
}

#define NUMBEROF(arr) (sizeof(arr)/sizeof(arr[0]))

static
rtems_monitor_command_entry_t entries[] = {
	{
		"cpuUsageDump",
		"cpuUsageDump",
		0,
		fnwrap,
		(unsigned32)CPU_usage_Dump,
		0
	},
	{
		"cpuUsageReset",
		"cpuUsageReset",
		0,
		fnwrap,
		(unsigned32)CPU_usage_Reset,
		0
	},
	{
		"threadStack",
		"threadStack [taskId]; give stacktrace of a task (0 for self)",
		1,
		fnwrap,
		(unsigned32)taskStack,
		0
	},
	{
		"memUsageDump",
		"memUsageDump; show used/free amount of RTEMS workspace and malloc heap",
		0,
		fnwrap,
		(unsigned32)memUsageDump,
		0
	},
};

int
rtemsMonitor(int verbose)
{
GetLine	*gl=0;
char	*line,*buf=0;

	rtems_semaphore_obtain(monitorMutex, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

	if (rtems_monitor_task_id) {
		rtems_semaphore_release(monitorMutex);
		fprintf(stderr,"RTEMS Monitor already in use, giving up...\n");
		return -1;
	}
	/* acquire the monitor task id and store our id there;
	 * serves as 'busy' mark
	 */
	rtems_task_ident(RTEMS_SELF, 0, &rtems_monitor_task_id);
	rtems_semaphore_release(monitorMutex);

	rtems_monitor_default_node=
	rtems_monitor_node = rtems_get_node(rtems_monitor_task_id);

	/* make sure symbols are loaded */
	rtems_monitor_symbols_loadup();

	/* create a line editor */
	if ( ! (gl = new_GetLine(200,100)) ) {
		fprintf(stderr,"No memory for line editor\n");
		goto cleanup;
	}

	printf("Entering the RTEMS Monitor...\n");

	while ( (line=gl_get_line(gl,"monitor>",NULL,0)) ) {
		int		argc;
		char	*argv[20]; /* no overrun protection */
		rtems_monitor_command_entry_t	*cmd;

		free(buf);
		if ( ! (buf=strdup(line)) ){
			fprintf(stderr,"No memory for strdup()\n");
			goto cleanup;
		}

		assert(rtems_monitor_make_argv(buf,&argc,argv) < (int)(sizeof(argv)/sizeof(argv[0])));

		/* catch exit/quit which call the fatal error handler */
		if (!strcmp("exit",argv[0]) ||
		    !strcmp("quit",argv[0]))
			break;

		if ((cmd=rtems_monitor_command_lookup(
						rtems_monitor_commands,
						argc,
						argv)))
        	cmd->command_function(argc, argv, cmd->command_arg, verbose);
		else {
			fprintf(stderr,"Command '%s' not found\n",argv[0]);
		}
	}

	printf("Leaving the RTEMS Monitor\n");

cleanup:
	/* destroy the line editor */
	if (gl)
		del_GetLine(gl);

	free(buf);

	/* release the monitor */
	rtems_semaphore_obtain(monitorMutex, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	rtems_monitor_task_id=0;
	rtems_semaphore_release(monitorMutex);
	return 0;
}


#ifdef __cplusplus


class CallUtils {
public:
	CallUtils() {
		unsigned i;
		if (nest++) return;
		rtems_semaphore_create( rtems_build_name('c','u','t','m'),
								1,
						        RTEMS_PRIORITY|RTEMS_BINARY_SEMAPHORE|
								RTEMS_INHERIT_PRIORITY|
								RTEMS_NO_PRIORITY_CEILING|RTEMS_LOCAL,
								0,
								&monitorMutex);
		for (i=0; i<NUMBEROF(entries); i++)
			rtems_monitor_insert_cmd(entries+i);
	};
	~CallUtils() {
		unsigned i;
		if (--nest) return;
		for (i=0; i<NUMBEROF(entries); i++)
			rtems_monitor_erase_cmd(entries+i);
		rtems_semaphore_delete(monitorMutex);
	};
static int nest;
};

static CallUtils oneOnly;

int CallUtils::nest=0;

};
#endif
