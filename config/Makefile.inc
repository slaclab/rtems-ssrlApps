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
# (RTEMS/tools)
ifndef RTEMS_WHICH
RTEMS_WHICH = 4.6.0pre4
endif

# support different install locations (prod/devel)
# (rtemsApplications)
ifndef RTEMS_SITE_WHICH
RTEMS_SITE_WHICH = $(RTEMS_WHICH)
endif

ifndef RTEMS_TOP
RTEMS_TOP = /afs/slac/package/rtems/$(RTEMS_WHICH)
endif

#Where to install, where to grab local packages
ifndef RTEMS_SITE_DIR
RTEMS_SITE_DIR = /afs/slac/g/spear/rtemsApplications/$(RTEMS_SITE_WHICH)
endif

#################################################################
# There should be no need to change anything below here

#for EPICS
RTEMS_BASE = $(RTEMS_TOP)/tools/@sys

# An ugly hack. In fact, the <top>/make/custom/<bsp>.cfg
# files need to be patched to define RTEMS_CPU _prior_
# to including 'default.cfg'. Until that goes into
# the distribution, we define our default value here
ifndef RTEMS_CPU
RTEMS_CPU = powerpc
endif

#RTEMS 
prefix = $(RTEMS_TOP)/rtems
#RTEMS (target specific) libraries
exec_prefix = $(prefix)/$(RTEMS_CPU)-rtems
#Toolchain
tools_prefix = $(RTEMS_BASE)/bin

define make-obj
	$(make-rel) $(LINK_LIBS)
#	$(LD) $(AM_LDFLAGS) $(LDFLAGS) -r -o $@ $(OBJS) $(LINK_LIBS)
endef

#Tarball support
define make-tar
	if [ -z $(REVISION) ] ; then \
        echo "I need a version checked out with a revision tag to make a tarball";\
        exit 1;\
    else \
        echo tarring revision $(REVISION);\
        tar cfz $(if $(wildcard tarexcl),x tarexcl) $(REVISION).tgz --exclude $(REVISION).tgz -C .. $(shell basename `pwd`) ;\
    fi
endef

# continue with generic/common settings

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc.generic
