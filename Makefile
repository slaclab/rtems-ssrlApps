#
#  $Id$
#
#  Till Straumann <strauman@slac.stanford.edu>, 4/2003
#
#  To build the rtemsApplications for a different
#  BSP and/or RTEMS release and/or RTEMS_SITE_DIR,
#  you must first issue
# 
#      make distclean
#
#  then
#
#      make
#
#  RTEMS_BSP and/or RTEMS_WHICH and/or RTEMS_SITE_DIR
#  can also be specified on the commandline:
#
#      make distclean; make RTEMS_BSP=pc586
#
#  (Changing any of those variables requires the
#  'configure' scripts in the xxx.build subdirs to
#  be re-run, which is achieved by the 'distclean'
#  cycle.)

########################################################
# STUFF THAT MUST BE AT THE TOP
#
# save some host tool values before including
# the cross definitions
HOSTCC:=$(CC)
HOSTLD:=$(LD)
# GNU configure wants this
CC_FOR_BUILD = $(HOSTCC)
export CC_FOR_BUILD
#
include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
# we want RTEMS_BSP_FAMILY and RTEMS_CPU now
include $(RTEMS_CUSTOM)
#
export RTEMS_MAKEFILE_PATH
#
# BSP families we know they are PPC
PPCBSPS=motorola_powerpc svgm 
#
#
# END OF STUFF THAT MUST BE AT THE TOP
########################################################

# Default target.
#
# Since some apps/libs depend on others, an
# app/lib is always build _and_ installed
# so the subsequent build of the next app/lib
# finds all prerequisites.
#    
# refuse the 'all' target
all: error

#
# Applications which need to be 'autoconf'ed
# should come with a "Makefile" in their source
# directory which knows how to 'autoconf' the
# particular application. The source directory
# should be added to AUTOCONFSUBDIRS.
# E.g. if your application 'xxx' has a 'xxx/Makefile'
# calling 'autoconf' & friends, add
#
# AUTOCONFSUBDIRS+=xxx
#
# below. The 'real' makefile used to build the
# application will be generated in a separate
# "xxx.<arch>.build" subdirectory, see below
# (you must still/also add
#
# SUBDIRS+=xxx.$(BUILDEXT)
#
# see below)

AUTOCONFSUBDIRS+=cexp

#
# Add applications subdirectories at the
# appropriate place to the 'SUBDIRS'
# (I.e. if one application or library depends
# on another one, make sure the prerequisites
# appear first in the list)
#
# Applications which need to be 'configure'd,
# i.e. which need a 'configure' script to be run
# are usually configured and built in a separate
# directory 'xxx.<arch>.build' which is automatically
# created and configured.
# E.g. if 'xxx' was such an application, the line
#
# SUBDIRS+=xxx.$(BUILDEXT)
#
# should be added below
#

#SUBDIRS+=$(subst clean-recursive,.,$(filter clean-recursive,$@))

SUBDIRS+=rtemsNfs

# libbspExt supported only on our PPC BSPs
ifneq ($(filter $(RTEMS_BSP_FAMILY),$(PPCBSPS))xx,xx)
SUBDIRS+=libbspExt
endif

SUBDIRS+=cexp.$(BUILDEXT)

# apps below here depend on cexp and/or tecla and hence
# are made later
ifeq ("$(RTEMS_BSP)","svgm")
SUBDIRS+=svgmWatchdog
endif

SUBDIRS+=monitor
SUBDIRS+=telnetd
SUBDIRS+=system

ifeq ("$(RTEMS_BSP)","svgm")
SUBDIRS+=netboot
endif

INSTSUBDIRS+=/bin
INSTSUBDIRS+=/include
INSTSUBDIRS+=/lib
INSTSUBDIRS+=/$(RTEMS_BSP)/bin
INSTSUBDIRS+=/$(RTEMS_BSP)/lib
INSTSUBDIRS+=/$(RTEMS_BSP)/img

INSTDIRS = $(addprefix $(RTEMS_SITE_INSTALLDIR),$(INSTSUBDIRS))

BUILDEXT  = $(RTEMS_CPU)-build

BUILDDIRS = $(filter %.$(BUILDEXT),$(SUBDIRS))

# we simply delete these, no need to recurse into the build
# directories
distclean: SUBDIRS=$(filter-out %.$(BUILDEXT),$(SUBDIRS))

# only cleanup existing build directories
clean: SUBDIRS=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)))

error:
	@echo
	@echo 'YOU MUST USE AN EXPLICIT TARGET FROM THIS DIRECTORY'
	@echo 
	@echo 'Valid targets are:'
	@echo 
	@echo '    prep      - (re)create "configure" & friends after CVS checkout'
	@echo '    install   - configure, build and install everything to'
	@echo '                "$(RTEMS_SITE_INSTALLDIR)"'
	@echo '    clean     - remove target objects'
	@echo '    distclean - remove target objects AND host configuration'
	@echo
	@echo 'NOTES: - you must "gmake distclean" when switching to a different host machine'
	@echo '       - need for "prep" ONLY after CVS checkout'
	@echo 
	exit 1

prep: CC=$(HOSTCC)
prep: LD=$(HOSTLD)

prep:
	for dir in $(AUTOCONFSUBDIRS); do $(MAKE) -C $$dir tools_prefix=$(tools_prefix) $@; done

include $(RTEMS_ROOT)/make/directory.cfg
include $(CONFIG.CC)

install: $(INSTDIRS) $(BUILDDIRS:%=%/Makefile)

CLOBBER_ADDITIONS+=$(BUILDDIRS)

%.$(BUILDEXT)/Makefile:
	test -d $(dir $@) || $(MKDIR) $(dir $@)
	cd	$(dir $@) ; ../$(@:%.$(BUILDEXT)/Makefile=%)/configure --build=`../$(@:%.$(BUILDEXT)/Makefile=%)/config.guess` --host=$(RTEMS_CPU)-rtems --disable-nls --prefix=$(RTEMS_SITE_INSTALLDIR) --with-newlib CC=$(word 1,$(CC))

$(INSTDIRS):
	$(MKDIR) -p $@
