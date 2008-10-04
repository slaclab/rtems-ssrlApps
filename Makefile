#
#  Makefile,v 1.15 2004/11/08 23:02:50 till Exp
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
ifndef RTEMS_MAKEFILE_PATH
$(error $@: RTEMS_MAKEFILE_PATH not set - must point to ssrlApplications config dir)
endif
include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
# we want RTEMS_BSP_FAMILY and RTEMS_CPU now
include $(RTEMS_CUSTOM)
#
export RTEMS_MAKEFILE_PATH
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
ALLSUBDIRS     +=cexp

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

ifneq ($(filter $(RTEMS_CPU),powerpc i386)xx,xx)
SUBDIRS+=libbspExt
ifneq ($(RTEMS_BSP),psim)
SUBDIRS+=bsd_eth_drivers
endif
endif
ALLSUBDIRS     +=libbspExt
ALLSUBDIRS     +=bsd_eth_drivers

ifeq ($(RTEMS_CPU),powerpc)
# need a G4
ifneq ($(filter $(RTEMS_BSP),mvme5500 svgm beatnik)xx,xx)
SUBDIRS+=altivec
endif
# need a 604 with hardware page-tables
ifneq ($(filter $(RTEMS_BSP),mvme5500 svgm beatnik mvme2307)xx,xx)
SUBDIRS+=efence
endif
endif
ALLSUBDIRS     +=altivec
ALLSUBDIRS     +=efence

ifneq ($(filter $(RTEMS_CPU),powerpc i386 m68k)xx,xx)
SUBDIRS+=rtems-gdb-stub
endif
ALLSUBDIRS     +=rtems-gdb-stub

ifeq ($(RTEMS_BSP),uC5282)
SUBDIRS+=coldfUtils
endif
ALLSUBDIRS     +=coldfUtils

SUBDIRS+=cexp.$(BUILDEXT)

# apps below here depend on cexp and/or tecla and hence
# are made later
ifneq ($(filter $(RTEMS_BSP_FAMILY),svgm beatnik mvme3100 pc386)xx,xx)
SUBDIRS+=amdeth
endif
ALLSUBDIRS     +=amdeth

ifneq ($(filter $(RTEMS_BSP),beatnik uC5282)xx,xx)
SUBDIRS+=drvLan9118
endif
ALLSUBDIRS     +=drvLan9118

ifneq ($(filter $(RTEMS_BSP),svgm beatnik uC5282 mvme3100)xx,xx)
SUBDIRS+=svgmWatchdog
endif
ALLSUBDIRS     +=svgmWatchdog

SUBDIRS        +=rtemsNfs
ALLSUBDIRS     +=rtemsNfs
SUBDIRS        +=monitor
ALLSUBDIRS     +=monitor
SUBDIRS        +=telnetd
ALLSUBDIRS     +=telnetd
SUBDIRS        +=ntpNanoclock
ALLSUBDIRS     +=ntpNanoclock
SUBDIRS        +=miscUtils
ALLSUBDIRS     +=miscUtils

ifneq ($(filter $(RTEMS_BSP),svgm beatnik uC5282 mvme3100)xx,xx)
SUBDIRS+=netboot
endif
ALLSUBDIRS     +=netboot

SUBDIRS+=system
ALLSUBDIRS     +=system

INSTSUBDIRS+=/bin
INSTSUBDIRS+=/include
INSTSUBDIRS+=/lib

INSTDIRS = $(addprefix $(RTEMS_SITE_INSTALLDIR),$(INSTSUBDIRS))

BUILDEXT  = $(RTEMS_CPU)-$(RTEMS_BSP)-build

BUILDDIRS = $(filter %.$(BUILDEXT),$(SUBDIRS))

# we simply delete these, no need to recurse into the build
# directories
distclean: SUBDIRS:=$(ALLSUBDIRS)

# only cleanup existing build directories
clean: SUBDIRS:=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)))

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

# add tool path so that tools are found by 'configure' and sub-makes
PATH:=$(RTEMS_BASE)/bin:$(PATH)

# install 'config' to the install area
install-config: $(RTEMS_SITE_DIR)
	if [ `pwd` != `(cd $<; pwd)` ] ; then 			\
		$(RM) -r $</config	;						\
		tar cf - ./config | ( cd $< ; tar xfv - )	\
	fi

install: $(INSTDIRS) $(BUILDDIRS:%=%/Makefile) install-config

# How to make a tarball of this package
REVISION=$(filter-out $$%,$$Name$$)
tar:
	@$(make-tar)

CLOBBER_ADDITIONS+=$(wildcard $(AUTOCONFSUBDIRS).*-build)

ifndef RTEMS_SITE_DOCDIR
RTEMS_SITE_DOCDIR=$(RTEMS_SITE_DIR)
endif

ifndef RTEMS_SITE_MANDIR
RTEMS_SITE_MANDIR=$(RTEMS_SITE_DOCDIR)/man
endif

ifndef RTEMS_SITE_INFODIR
RTEMS_SITE_INFODIR=$(RTEMS_SITE_DOCDIR)/info
endif

# BUILDARCH var is evaluated by 'make' CWD is where this
# makefile is executed and unaffected by 'cd' in the
# commands below...
ifndef BUILDARCH
BUILDARCH=$(shell ./$(@:%.$(BUILDEXT)/Makefile=%)/config.guess)
endif

cexp.$(BUILDEXT)/Makefile:
	test -d $(dir $@) || $(MKDIR) $(dir $@)
	cd	$(dir $@) ;  \
	unset CC CFLAGS CXX CXXFLAGS CPPFLAGS LDFLAGS ; \
	../$(@:%.$(BUILDEXT)/Makefile=%)/configure --host=$(RTEMS_CPU)-rtems --disable-nls --prefix=$(RTEMS_SITE_INSTALLDIR) --mandir=$(RTEMS_SITE_MANDIR) --infodir=$(RTEMS_SITE_INFODIR) --with-newlib --disable-multilib --with-rtems-top=$(RTEMS_TOP) --enable-rtemsbsp=$(RTEMS_BSP)


%.$(BUILDEXT)/Makefile:
	test -d $(dir $@) || $(MKDIR) $(dir $@)
	cd	$(dir $@) ; ../$(@:%.$(BUILDEXT)/Makefile=%)/configure --host=$(RTEMS_CPU)-rtems --disable-nls --prefix=$(RTEMS_SITE_INSTALLDIR) --mandir=$(RTEMS_SITE_MANDIR) --infodir=$(RTEMS_SITE_INFODIR) --with-newlib --disable-multilib CC=$(word 1,$(CC)) CFLAGS="$(CPU_CFLAGS) $(CFLAGS)" CFLAGS_FOR_BUILD="" CXXFLAGS="$(CPU_CFLAGS) $(CXXFLAGS)" CXXFLAGS_FOR_BUILD=""

$(INSTDIRS) $(RTEMS_SITE_DIR):
	$(MKDIR) -p $@

test:
	echo $(CC)
