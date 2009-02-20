#ifndef SSRLAPPS_MISC_RTEMS_UTILS_H
#define SSRLAPPS_MISC_RTEMS_UTILS_H

#include <rtems.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Read uptime of the IDLE task;
 * returns RTEMS_SUCCESSFUL on
 * success and timespec in 'pts'.
 *
 * May return RTEMS_NOT_DEFINED if
 * idle task cannot be found or
 * RTEMS_NOT_IMPLEMENTED if RTEMS
 * is older than 4.9
 */
rtems_status_code
miscu_get_idle_uptime(struct timespec *pts);

/*
 * Return uptime of an RTEMS API task
 * For return values see
 *
 * rtems_get_idle_uptime()
 *
 * but additional error codes are
 * possible (RTEMS_INVALID_ID or
 * RTEMS_ILLEGAL_ON_REMOTE_OBJECT).
 */
rtems_status_code
miscu_get_task_uptime(rtems_id tid, struct timespec *pts);

/*
 * Initialize two 'timespec' structs for use
 * by miscu_cpu_load_percentage()
 */

void
miscu_cpu_load_percentage_init(struct timespec *lst_uptime, struct timespec *lst_idletime);

/* 
 * Determine percentage of CPU load
 * since the last time this routine
 * was called (using the same timespec
 * buffers).
 *
 * NOTES: If an error was found or the
 *        system clock does not proceed
 *        then NAN is returned.
 *
 *        As a side-effect, the routine
 *        updates the timespec structs,
 *        i.e., data held there must
 *        remain valid between consecutive
 *        calls of this routine.
 *
 *        After the first call following
 *
 *        miscu_cpu_load_percentage_init()
 *
 *        the CPU load since the system
 *        has been booted is returned.
 */
double
miscu_cpu_load_percentage(struct timespec *lst_uptime, struct timespec *lst_idletime);

#ifdef __cplusplus
};
#endif

#endif
