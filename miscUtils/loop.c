/* $Id$ */
#include <rtems.h>
#include <rtems/rtems/ratemon.h>
#include <stdio.h>
#include <stdlib.h>

/* Task spawning utilities as a CEXP loadable module
 * Author: Till Straumann, 2004/5
 */

/*
 * Cheap way of passing any arguments - works only
 * with the PPC SVR5 ABI.
 */

#if defined(__PPC__) && defined(_CALL_SYSV)
#define PPCSTR \
"Due to a pecularity of the PPC ABI, 5 floating point arguments\n" \
"may also be passed (in any order)\n"
#define FPARGS , double f1, double f2, double f3, double f4, double f5
#else
#define PPCSTR ""
#define FPARGS
#endif

typedef struct ArgsRec_ {
		int (*fn)();
		int period_ticks;
		int times;
		int a1, a2, a3, a4, a5;
		int a6, a7, a8, a9, a10;
#if defined(__PPC__) && defined(_CALL_SYSV)
		double f1, f2, f3, f4, f5;
#endif
} ArgsRec, *Args;

static void
wrap(void *a)
{
Args				arg    = (Args)a;
rtems_id			period = 0;
int		 			times  = arg->times;
rtems_status_code	rc;

	if (times > 1 
		&& RTEMS_SUCCESSFUL != (rc=rtems_rate_monotonic_create(
										rtems_build_name('L','O','O','P'),
										&period))) {
		fprintf(stderr,"Creating period failed (error 0x%x)\n",rc);
		goto cleanup;
	}

	while (times-- > 0) {
		if (period &&
			RTEMS_TIMEOUT == rtems_rate_monotonic_period(
									period, arg->period_ticks)) {
				fprintf(stderr,"Missed deadline after %i loops\n",
								arg->times-times-1);
				break;
		}
		arg->fn(
			arg->a1, arg->a2, arg->a3, arg->a4, arg->a5,
			arg->a6, arg->a7, arg->a8, arg->a9, arg->a10
#if defined(__PPC__) && defined(_CALL_SYSV)
			, arg->f1, arg->f2, arg->f3, arg->f4, arg->f5
#endif
			);
	}

cleanup:
	if (period)
		rtems_rate_monotonic_delete(period);
	free(arg);
	rtems_task_delete(RTEMS_SELF);
}

static rtems_id
spawn(int priority, int stack, Args args)
{
rtems_id tid;

	if (!priority)
			priority=60;
	if (!stack)
			stack=20000;
	if (RTEMS_SUCCESSFUL != rtems_task_create(
								rtems_build_name('S','P','W','N'),
								priority,
								stack,
								RTEMS_DEFAULT_MODES,
								RTEMS_FLOATING_POINT | RTEMS_LOCAL,
								&tid)) {
		fprintf(stderr,"Creating task failed\n");
		return 0;
	}
	if (RTEMS_SUCCESSFUL != rtems_task_start(
								tid,
								(rtems_task_entry)   wrap,
								(rtems_task_argument)args)) {
		fprintf(stderr,"Unable to start task\n");
		return 0;
	}
	return tid;
}


rtems_id
spawnUtil(int priority, int stack, int (*f)(),
		int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10
		FPARGS
		)
{
rtems_id rval;
Args args=calloc(1,sizeof(*args));
	args->fn=f;
	args->a1=a1;
	args->a2=a2;
	args->a3=a3;
	args->a4=a4;
	args->a5=a5;
	args->a6=a6;
	args->a7=a7;
	args->a8=a8;
	args->a9=a9;
	args->a10=a10;
#if defined(__PPC__) && defined(_CALL_SYSV)
	args->f1=f1;
	args->f2=f2;
	args->f3=f3;
	args->f4=f4;
	args->f5=f5;
#endif
	args->times=1;
	rval = spawn(priority, stack, args);
	if ( 0 == rval)
		free(args);
	return rval;
}

rtems_id
loopUtil(int period_ticks, int n_times, int (*f)(),
		int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10
		FPARGS
		)
{
rtems_id	rval;
Args	args=malloc(sizeof(*args));
	args->fn=f;
	args->a1=a1;
	args->a2=a2;
	args->a3=a3;
	args->a4=a4;
	args->a5=a5;
	args->a6=a6;
	args->a7=a7;
	args->a8=a8;
	args->a9=a9;
	args->a10=a10;
#if defined(__PPC__) && defined(_CALL_SYSV)
	args->f1=f1;
	args->f2=f2;
	args->f3=f3;
	args->f4=f4;
	args->f5=f5;
#endif
	args->period_ticks = period_ticks;
	args->times = n_times;
	rval = spawn(0, 0, args);
	if ( 0 == rval )
		free(args);
	return rval;
}

#ifdef HAVE_CEXP
#include <cexpHelp.h>
CEXP_HELP_TAB_BEGIN(loopUtil)
	HELP(
"Spawn a task which executes the user function 'f' for n_times\n"
"with at intervals of 'period_ticks'. Up to 10 integer arguments\n"
"may be passed.\n"
PPCSTR
"RETURNS: task_id on success, zero if task couldn't be spawned\n",
	rtems_id, loopUtil, (int period_ticks, int n_times, int (*f)(), 
		int a1, int a2, int a3, int a4, int a5, 
		int a6, int a7, int a8, int a9, int a10
		)
		),
	HELP(
"Spawn a task to execute the user function 'f'. Up to 10 integer\n"
"arguments may be passed.\n"
PPCSTR
"RETURNS: task_id on success, zero if task couldn't be spawned\n",
	rtems_id, spawnUtil, (int priority, int stack, int (*f)(),
		int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10
		)
		),
CEXP_HELP_TAB_END
#endif


