#
# Settings to use a specific version of RTEMS
# along with its toolchain and locally installed
# stuff.
#
#################################################################

#### CD specific settings
#set version
RTEMS_WHICH        = 4.9
# define a variable for the CD topdir
RTEMS_CD_TOP       = /afs/slac/package/rtems/$(RTEMS_WHICH)
#where to pick up RTEMS (installation)
RTEMS_TOP          = $(RTEMS_CD_TOP)/target
#where to pick up the toolchain; RTEMS_BASE is also used by EPICS
RTEMS_BASE         = $(RTEMS_CD_TOP)/host/$(RTEMS_HOST_ARCH)
#NOTE: for the CD, we add yet another area, the RTEMS_SSRLAPPS_DIR
#      which is where the ssrl applications are installed.
#      Since this may be read-only (CD), the user can then
#      use RTEMS_SITE_DIR for installing the examples / other
#      applications.
#      Hence, there are three areas:
#      RTEMS_TOP          -- rtems proper
#      RTEMS_SSRLAPPS_DIR -- ssrl applications, cexp, nfs etc.
#      RTEMS_SITE_DIR     -- user installation top
#
#where to install ssrl applications (into the 'target' area)
#RTEMS_SSRLAPPS_DIR = $(RTEMS_TOP)/ssrlApps-test
RTEMS_SSRLAPPS_DIR = $(RTEMS_TOP)/ssrlApps

#where to install documentation 
RTEMS_SITE_DOCDIR  = $(RTEMS_CD_TOP)/doc

# this may be overridden e.g. on the command line
ifndef RTEMS_BSP
$(error RTEMS_BSP not set -- use either environment variable or pass on the command line)
endif

# support 'production' and development environments
# (RTEMS and toolchain)
ifndef RTEMS_WHICH
RTEMS_WHICH =
endif

# support different install locations (prod/devel)
# (SSRL rtemsApplications)
ifndef RTEMS_SITE_WHICH
RTEMS_SITE_WHICH = $(RTEMS_WHICH)
endif

ifndef RTEMS_TOP
$(error RTEMS_TOP not set -- needs to point to your RTEMS installation top directory)
endif

ifndef RTEMS_BASE
$(error RTEMS_BASE not set -- needs to point to your RTEMS toolchain installation top directory)
endif

#Where to install/grab local packages
ifndef RTEMS_SITE_DIR
RTEMS_SITE_DIR = $(RTEMS_SSRLAPPS_DIR)
endif

ifndef RTEMS_SITE_DOCDIR
RTEMS_SITE_DOCDIR = $(RTEMS_SITE_DIR)
endif

#AFS:
#RTEMS_HOST_ARCH=@sys

ifndef RTEMS_HOST_ARCH
  RTEMS_HOST_ARCH:=$(shell if sys > /dev/null 2>&1 ; then sys; fi)
  ifeq ($(RTEMS_HOST_ARCH)xx,xx)
    ifdef EPICS_HOST_ARCH
    RTEMS_HOST_ARCH:=$(EPICS_HOST_ARCH)
    else
    RTEMS_HOST_ARCH:=i386_linux24
    endif
  endif
endif

#################################################################
# There should be no need to change anything below here

# An ugly hack. In fact, the <top>/make/custom/<bsp>.cfg
# files need to be patched to define RTEMS_CPU _prior_
# to including 'default.cfg'. Until that goes into
# the distribution, we define our default value here
ifndef RTEMS_CPU
RTEMS_CPU = powerpc
endif

#RTEMS 
prefix       = $(RTEMS_TOP)
#RTEMS (target specific) libraries
exec_prefix  = $(prefix)/$(RTEMS_CPU)-rtems
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
        tar cfz $(REVISION).tgz $(if $(wildcard tarexcl),-X tarexcl) --exclude $(REVISION).tgz -C .. $(shell basename `pwd`) ;\
    fi
endef

# continue with generic/common settings

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc.generic
