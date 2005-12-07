/* $Id$ */

#ifndef __PPC__
#error "This code is PowerPC specific"
#endif

/* Code to probe the compiler's stack alignment (PowerPC);
 * The routine determines at run-time if the compiler generated
 * 8 or 16-byte aligned code.
 *
 * Till Straumann <strauman@slac.stanford.edu>, 2005
 */

#include <stdio.h>

#ifndef STATIC
#define STATIC
#endif

static void dummy() __attribute__((noinline));
/* add (empty) asm statement to make sure this isn't optimized away */
static void dummy() { asm volatile(""); }

static unsigned probe_r1() __attribute__((noinline));
static unsigned probe_r1()
{
unsigned r1;
	/* call something to enforce creation of a minimal stack frame;
     * (8 bytes: r1 and lr space for 'dummy' callee). If compiled
     * with -meabi -mno-altivec gcc allocates 8 bytes, if -mno-eabi
     * or -maltivec / -mabi=altivec then gcc allocates 16 bytes
     * according to the sysv / altivec ABI specs.
     */
	dummy();
	/* return stack pointer */
	asm volatile("mr %0,1":"=r"(r1));
	return r1;
}

STATIC unsigned
probe_ppc_stack_alignment()
{
unsigned r1;
	asm volatile("mr %0,1":"=r"(r1));
	return (r1 - probe_r1()) & ~ 0xf;
}

#ifdef DEBUG

typedef struct lr {
	struct lr *up;
	void      *lr;
} lr;

STATIC void
sdump()
{
lr   *r1;
void *lr;
	asm volatile("mr %0,1; mflr %1":"=r"(r1),"=r"(lr));
	while ( r1->up ) {
		printf("R1: 0x%08x; return addr 0x%08x\n",(uint32_t)r1,(uint32_t)lr);
		r1=r1->up;
		lr=r1->lr;
	}
}
#endif
