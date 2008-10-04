#  $Id$
AUTOMAKE_OPTIONS=foreign

include $(top_srcdir)/rtems-pre.am

EXEEXT=$(OBJEXEEXT)

miscUtils_SOURCES       = memUtils.c loop.c ifconfi.c ttyconfi.c icmpping.c sockstats.c
miscUtils_SOURCES      += compat.h

include_HEADERS         = icmpping.h

bin_PROGRAMS            = miscUtils

include $(top_srcdir)/rtems.am

miscUtils_LINK          = $(OBJLINK)
