#  $Id$
AUTOMAKE_OPTIONS=foreign

include $(top_srcdir)/rtems-pre.am

EXEEXT=$(OBJEXEEXT)

telnetd_SOURCES   =

if HAVE_BUNDLED_TELNETD
telnetd_LDADD     =-ltelnetd
else
telnetd_SOURCES  +=telnetd.c pty.c des.c
telnetd_SOURCES  +=passwd.h
endif

if HAVE_CEXP
telnetd_SOURCES  +=telnetd.mod.c
else
if HAVE_BUNDLED_TELNETD
AM_LDFLAGS       +=-Wl,-u,rtems_telnetd_initialize
endif
endif

bin_PROGRAMS      =telnetd

telnetd_LINK      =$(OBJLINK)

EXTRA_DIST        = check_passwd.c

include $(top_srcdir)/rtems.am

