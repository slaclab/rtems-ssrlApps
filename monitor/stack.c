#define __RTEMS_VIOLATE_KERNEL_VISIBILITY__
#include <rtems.h>
//#include <rtems/score/cpu.h>
//#include <rtems/score/thread.h>
#include <stdio.h>
#ifdef PPC
#include <libcpu/stackTrace.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <cexp.h>
#endif


#define NumberOf(arr) (sizeof(arr)/sizeof((arr)[0]))

#define GETREG(var, nr)	__asm__ __volatile__("mr %0, %1":"=r"(var##nr):"i"(nr))

#ifdef PPC
int
taskStack(rtems_id id)
{
Objects_Locations	loc;
Thread_Control		*tcb;
void				*stackbuf[30];
int					i;
Context_Control		regs;

	tcb = _Thread_Get(id, &loc);
	if (OBJECTS_LOCAL!=loc || !tcb) {
		if (tcb)
			_Thread_Enable_dispatch();
		fprintf(stderr,"Id %x not found on local node\n",id);
		return -1;
	}
	stackbuf[0]=0;
	if (_Thread_Executing==tcb)
		tcb=0;
	CPU_stack_take_snapshot(
					stackbuf,
					NumberOf(stackbuf),
					(void*)0,
					(void*)(tcb ? tcb->Registers.pc : 0),
					(void*)(tcb ? tcb->Registers.gpr1 : 0));
	if (tcb) {
		regs = tcb->Registers;
	}

	_Thread_Enable_dispatch();

	if (!tcb) {
		GETREG(regs.gpr,1);
		GETREG(regs.gpr,2);
		GETREG(regs.gpr,13);
		GETREG(regs.gpr,14);
		GETREG(regs.gpr,15);
		GETREG(regs.gpr,16);
		GETREG(regs.gpr,17);
		GETREG(regs.gpr,18);
		GETREG(regs.gpr,19);
		GETREG(regs.gpr,20);
		GETREG(regs.gpr,21);
		GETREG(regs.gpr,22);
		GETREG(regs.gpr,23);
		GETREG(regs.gpr,24);
		GETREG(regs.gpr,25);
		GETREG(regs.gpr,26);
		GETREG(regs.gpr,27);
		GETREG(regs.gpr,28);
		GETREG(regs.gpr,29);
		GETREG(regs.gpr,30);
		GETREG(regs.gpr,31);
		__asm__ __volatile__("mfcr %0":"=r"(regs.cr));
		__asm__ __volatile__("mfmsr %0":"=r"(regs.msr));
	}
	printf("\nRegisters:\n");
	printf("GPR1:  0x%08x\n",
			regs.gpr1);
	printf("GPR2:  0x%08x, GPR13: 0x%08x, GPR14: 0x%08x, GPR15: 0x%08x\n",
			regs.gpr2, regs.gpr13, regs.gpr14, regs.gpr15);
	printf("GPR16: 0x%08x, GPR17: 0x%08x, GPR18: 0x%08x, GPR19: 0x%08x\n",
			regs.gpr16, regs.gpr17, regs.gpr18, regs.gpr19);
	printf("GPR20: 0x%08x, GPR21: 0x%08x, GPR22: 0x%08x, GPR23: 0x%08x\n",
			regs.gpr20, regs.gpr21, regs.gpr22, regs.gpr23);
	printf("GPR24: 0x%08x, GPR25: 0x%08x, GPR26: 0x%08x, GPR27: 0x%08x\n",
			regs.gpr24, regs.gpr25, regs.gpr26, regs.gpr27);
	printf("GPR28: 0x%08x, GPR29: 0x%08x, GPR30: 0x%08x, GPR31: 0x%08x\n\n",
			regs.gpr28, regs.gpr29, regs.gpr30, regs.gpr31);
	printf("CR:    0x%08x\n",
			regs.cr);
	printf("MSR:   0x%08x\n",
			regs.msr);

	printf("\nStack Trace:\n");

	for (i=0; stackbuf[i] && i<NumberOf(stackbuf); i++) {
		void		*symaddr=stackbuf[i];
		unsigned	diff=(unsigned)stackbuf[i];
		char		buf[250];
		printf("0x%08x",(unsigned)stackbuf[i]);
		if (0==cexpAddrFind(&symaddr,buf,sizeof(buf))) {
			diff=(unsigned)stackbuf[i]-(unsigned)symaddr;
			printf(" == <%s",buf);
			if (diff) {
				printf(" + 0x%x",diff);
			}
			fputc('>',stdout);
		}
		fputc('\n',stdout);
	}

	return 0;
}
#else
#warning CPU_stack_take_snapshot() needs to be implemented for your architecture
#warning register access/printing not implemented for this CPU architecture
#warning task stack dumping not implemented.

/* dummy on architectures where we don't have GETREG and CPU_stack_take_snapshot */
int
taskStack(rtems_id id)
{
	fprintf(stderr,"Dumping a task's stack is not implemented for this architecture\n");
	fprintf(stderr,"You need to implement CPU_stack_take_snapshot() and more, see\n");
	fprintf(stderr,"    %s\n",__FILE__);
	return -1;
}

#endif
