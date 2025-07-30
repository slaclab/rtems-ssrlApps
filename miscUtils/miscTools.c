/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Misc tools for Cexpsh
 * ----------------------------------------------------------------------------
 * This file is part of 'ssrlApps'. It is subject to the license terms in the
 * LICENSE.txt file found in the top-level directory of this distribution,
 * and at:
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of 't9p', including this file, may be copied, modified,
 * propagated, or distributed except according to the terms contained in the
 * LICENSE.txt file.
 * ----------------------------------------------------------------------------
 **/

#include <stdio.h>
#include <sched.h>
#include <stdint.h>
#include <rtems.h>
#include <rtems/shell.h>
#include <rtems/cpuuse.h>
#include <rtems/posix/pthread.h>

#ifdef HAVE_CEXP
#include <cexpHelp.h>
#endif

static void per_thread(Thread_Control* tcb)
{
  char name[32];
  rtems_object_get_name(tcb->Object.id, sizeof(name), name);
  printf("%6s initial_prio=%lld, current_prio=%lld, real_prio=%lld\n",
    name,
    (long long)tcb->Start.initial_priority,
    (long long)tcb->current_priority,
    (long long)tcb->real_priority
  );
}

void taskList()
{
    rtems_iterate_over_all_threads(per_thread);
}

#ifdef HAVE_CEXP
CEXP_HELP_TAB_BEGIN(taskList)
	HELP(
"List all active tasks and their associated priorities\n",
	void, taskList,  (void)
	),
CEXP_HELP_TAB_END
#endif

void cpuuse()
{
    rtems_cpu_usage_report();
}

#ifdef HAVE_CEXP
CEXP_HELP_TAB_BEGIN(cpuuse)
	HELP(
"Display CPU usage for each active task\n",
	void, cpuuse,  (void)
	),
CEXP_HELP_TAB_END
#endif

void posixSchedInfo()
{
	int i;
	const int prios[] = {SCHED_RR, SCHED_FIFO, SCHED_OTHER};
	const char* prio_names[] = {"SCHED_RR", "SCHED_FIFO", "SCHED_OTHER"};
	for (i = 0; i < sizeof(prios)/sizeof(*prios); ++i) {
		printf("%s: min=%d, max=%d\n", prio_names[i], sched_get_priority_min(prios[i]), sched_get_priority_max(prios[i]));
	}
}

#ifdef HAVE_CEXP
CEXP_HELP_TAB_BEGIN(posixSchedInfo)
	HELP(
"Display min/max priorities for each scheduler type\n",
	void, posixSchedInfo,  (void)
	),
CEXP_HELP_TAB_END
#endif
