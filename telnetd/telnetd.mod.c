/* $Id$ */

/* cexpsh loadable module support */

/* T. Straumann <strauman@slac.stanford.edu>, 2003-2007 */

#ifdef HAVE_BUNDLED_TELNETD
#include <rtems.h>
#include <rtems/telnetd.h>
#else
int startTelnetd(void (*cmd)(char *, void *), void *arg, int dontSpawn, int stack, int priority);
#define rtems_telnetd_initialize(cmd,arg,dsp,stk,pri,ask) \
	startTelnetd(cmd,arg,dsp,stk,pri)
#endif

#include <cexp.h>

static void
cexpWrap(char *dev, void *arg)
{
char	*args[]={"Cexp-telnet",0};
	fprintf(stderr,"[Telnet:] starting cexp on %s\n",dev);
	cexp_main(1,args);
}

void _cexpModuleInitialize(void *m)
{
	rtems_telnetd_initialize(cexpWrap,0,0,32000,0,1);
}

/* telnetd can't be stopped */
