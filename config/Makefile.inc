#
# Settings to use a specific version of RTEMS
# along with its toolchain and locally installed
# stuff.
#
#################################################################

# this may be overridden e.g. on the command line
ifndef RTEMS_BSP
RTEMS_BSP = svgm
endif

# support 'production' and development environments
RTEMS_WHICH = prod

RTEMS_TOP = /afs/slac/package/rtems/$(RTEMS_WHICH)

#Where to install, where to grab local packages
RTEMS_SITE_DIR = /afs/slac/g/spear/rtemsApplications/$(RTEMS_WHICH)

RTEMS_MAKEFILE_PATH = /afs/slac/g/spear/rtemsApplications/config

#################################################################
# There should be no need to change anything below here

#for EPICS
RTEMS_BASE = $(RTEMS_TOP)/tools/@sys

#RTEMS 
prefix = $(RTEMS_TOP)/rtems
#RTEMS (target specific) libraries
exec_prefix = $(prefix)/powerpc-rtems
#Toolchaing
tools_prefix = $(RTEMS_BASE)/bin

# continue with generic/common settings

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc.generic
