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


#default target.
#
# Since some apps/libs depend on others, an
# app/lib is always build _and_ installed
# for the subsequent build of the next app/lib
# finding all prerequisites.
#    
install:

all: error

# save some host tool values
HOSTCC:=$(CC)
HOSTLD:=$(LD)
CC_FOR_BUILD = $(HOSTCC)
export CC_FOR_BUILD

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
# we want RTEMS_BSP_FAMILY and RTEMS_CPU now
include $(RTEMS_CUSTOM)

export RTEMS_MAKEFILE_PATH

BUILDEXT  = $(RTEMS_CPU)-build

BUILDDIRS = $(filter %.$(BUILDEXT),$(SUBDIRS))

PPCBSPS=motorola_powerpc svgm 

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

# make ours first
# DONT ADD APPLICATION SUBDIRECTORIES HERE
SUBDIRS+=$(subst distclean-recursive,.,$(filter distclean-recursive,$@))
SUBDIRS+=$(subst clean-recursive,.,$(filter clean-recursive,$@))
# ADD APPLICATION SUBDIRECTORIES BELOW HERE
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

# Need this rule prior to including the distclean: distclean-recursive
# dependency, so we can mock up a dummy Makefile just for the recursion
clean-recursive \
distclean-recursive: $(addsuffix /Makefile,$(BUILDDIRS))


all: error

error:
	@echo
	@echo YOU MUST USE THE 'install' TARGET FROM THIS DIRECTORY
	@echo
	exit 1

prep: CC=$(HOSTCC)
prep: LD=$(HOSTLD)

prep:
	for dir in $(AUTOCONFSUBDIRS); do $(MAKE) -C $$dir $@; done

include $(RTEMS_ROOT)/make/directory.cfg
include $(CONFIG.CC)

install: $(INSTDIRS) $(SUBDIRS)

CLOBBER_ADDITIONS+=$(BUILDDIRS)

###preinstall-recursive: $(INSTDIRS) $(SUBDIRS)

%.$(BUILDEXT)/Makefile:
	test -d $(dir $@) || mkdir $(dir $@)
	echo distclean: > $@
	echo clean: >> $@

%.$(BUILDEXT): %
	test -d $@ || $(MKDIR) $@
	cd	$@ ; ../$^/configure --build=`../$^/config.guess` --host=$(RTEMS_CPU)-rtems --disable-nls --prefix=$(RTEMS_SITE_INSTALLDIR) --with-newlib CC=$(word 1,$(CC))

$(INSTDIRS):
	mkdir -p $@
