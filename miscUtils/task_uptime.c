/* $Id$ lightweight CPU usage */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(RTEMS_VERSION_ATLEAST) && RTEMS_VERSION_ATLEAST(4,8,99)
#define HAVE_HIGHRES_TIME
#endif

#include <rtems/system.h>
#include <rtems/rtems/tasks.h>
#ifdef HAVE_HIGHRES_TIME
#include <rtems/score/timespec.h>
#endif

#include <ssrlAppsMiscUtils.h>

rtems_status_code
miscu_get_idle_uptime(struct timespec *pts)
{
#if defined(HAVE_HIGHRES_TIME)
rtems_status_code sc = RTEMS_SUCCESSFUL;
int               key;

	rtems_interrupt_disable(key);
		if ( ! _Thread_Idle ) {
			sc   = RTEMS_NOT_DEFINED;
		} else {
			*pts = _Thread_Idle->cpu_time_used;
		}
	rtems_interrupt_enable(key);

	return sc;
#else
	return RTEMS_NOT_IMPLEMENTED;
#endif
}

rtems_status_code
miscu_get_task_uptime(rtems_id tid, struct timespec *pts)
{
#if defined(HAVE_HIGHRES_TIME)
Thread_Control    *tcb;
Objects_Locations loc;

	tcb = _Thread_Get (tid, &loc);

	switch ( loc ) {
		case OBJECTS_LOCAL:
			*pts = tcb->cpu_time_used;
			_Thread_Enable_dispatch();
		return RTEMS_SUCCESSFUL;

#if defined(RTEMS_MULTIPROCESSING)
		case OBJECTS_REMOTE:
			_Thread_Dispatch();
		/* not implemented for remote tasks */
		return RTEMS_ILLEGAL_ON_REMOTE_OBJECT;
#endif
		
		case OBJECTS_ERROR:
		default:
		break;
	}
	return RTEMS_INVALID_ID;
#else
	return RTEMS_NOT_IMPLEMENTED;
#endif
}

/* Returns percentage or NAN (if difference to last uptime == 0) */
double
miscu_cpu_load_percentage(struct timespec *lst_uptime, struct timespec *lst_idletime)
{
#if defined(HAVE_HIGHRES_TIME)
static struct timespec internal_up   = { 0., 0. };
static struct timespec internal_idle = { 0., 0. };

struct timespec diff, now_uptime, now_idletime;

double res;

	if ( !lst_uptime )
		lst_uptime   = &internal_up;
	if ( !lst_idletime )
		lst_idletime = &internal_idle;

	if ( RTEMS_SUCCESSFUL != rtems_clock_get_uptime( &now_uptime ) )
		return nan("");

	_Timespec_Subtract(lst_uptime, &now_uptime, &diff);

	res = diff.tv_sec + diff.tv_nsec * 1.0E-9;

	if ( 0.0 == res )
		return nan("");

	if ( RTEMS_SUCCESSFUL != miscu_get_idle_uptime(&now_idletime) )
		return nan("");

	_Timespec_Subtract(lst_idletime, &now_idletime, &diff);

	res = 100.0 * ( 1.0 - (diff.tv_sec + diff.tv_nsec * 1.0E-9) / res );

	*lst_uptime   = now_uptime;
	*lst_idletime = now_idletime;

	return res;
#else
	return nan("");
#endif
}

void
miscu_cpu_load_percentage_init(struct timespec *lst_uptime, struct timespec *lst_idletime)
{
	if ( lst_uptime ) {
		lst_uptime->tv_sec  = 0;
		lst_uptime->tv_nsec = 0;
	}
	if ( lst_idletime ) {
		lst_idletime->tv_sec  = 0;
		lst_idletime->tv_nsec = 0;
	}
}
