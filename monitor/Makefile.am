#  $Id$
AUTOMAKE_OPTIONS=foreign

include $(top_srcdir)/rtems-pre.am

EXEEXT=$(OBJEXEEXT)

monitor_SOURCES     = stack.c call.cc memusage.c

bin_PROGRAMS        = monitor

monitor_LINK        = $(OBJLINK)

include $(top_srcdir)/rtems.am

