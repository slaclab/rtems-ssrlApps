#  $Id$
AUTOMAKE_OPTIONS=foreign

include $(top_srcdir)/rtems-pre.am

EXEEXT=$(OBJEXEEXT)

miscUtils_SOURCES       = memUtils.c loop.c ifconfi.c
miscUtils_SOURCES      += ttyconfi.c icmpping.c sockstats.c
miscUtils_SOURCES      += task_uptime.c
miscUtils_SOURCES      += compat.h
miscUtils_SOURCES      += exectime.c
miscUtils_SOURCES      += traceroute.c
miscUtils_SOURCES      += fileUtils.c
miscUtils_SOURCES      += traceroute.c

include_HEADERS         = icmpping.h
include_HEADERS        += ssrlAppsMiscUtils.h

bin_PROGRAMS            = miscUtils

include $(top_srcdir)/rtems.am

miscUtils_LINK          = $(OBJLINK)
