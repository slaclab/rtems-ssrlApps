#include <cexp.h>

int startTelnetd(void (*cmd)(char *, void *), void *arg, int dontSpawn, int stack, int priority);

static void
cexpWrap(char *dev, void *arg)
{
char	*args[]={"Cexp-telnet",0};
	fprintf(stderr,"[Telnet:] starting cexp on %s\n",dev);
	cexp_main(1,args);
}

void _cexpModuleInitialize(void *m)
{
	startTelnetd(cexpWrap,0,0,32000,100);
}

/* telnetd can't be stopped */
