#define __RTEMS_VIOLATE_KERNEL_VISIBILITY__
#include <rtems.h>
//#include <rtems/score/cpu.h>
//#include <rtems/score/thread.h>
#ifdef PPC
#include <libcpu/stackTrace.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <cexp.h>


#define NumberOf(arr) (sizeof(arr)/sizeof((arr)[0]))

int
taskStack(rtems_id id)
{
Objects_Locations	loc;
Thread_Control		*tcb;
void				*stackbuf[30];
int					i;
	tcb = _Thread_Get(id, &loc);
	if (OBJECTS_LOCAL!=loc || !tcb) {
		fprintf(stderr,"Id %x not found on local node\n",id);
		return -1;
	}
	stackbuf[0]=0;
	if (_Thread_Executing==tcb)
		tcb=0;
#ifdef PPC
	CPU_stack_take_snapshot(
					stackbuf,
					NumberOf(stackbuf),
					(void*)0,
					(void*)tcb ? tcb->Registers.pc : 0,
					(void*)tcb ? tcb->Registers.gpr1 : 0);
#else
#warning CPU_stack_take_snapshot() needs to be implemented for your architecture
#endif
	_Thread_Enable_dispatch();

	for (i=0; stackbuf[i] && i<NumberOf(stackbuf); i++) {
		void		*symaddr=stackbuf[i];
		unsigned	diff=(unsigned)stackbuf[i];
		char		buf[250];
		if (0==cexpAddrFind(&symaddr,buf,sizeof(buf))) {
			diff=(unsigned)stackbuf[i]-(unsigned)symaddr;
			printf("<%s",buf);
			if (diff) {
				printf(" + 0x%x",diff);
			}
			printf(">\n");
		} else {
			printf("0x%08x\n",stackbuf[i]);
		}
	}

	return 0;
}
