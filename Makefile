#
#  $Id$
#

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
export RTEMS_MAKEFILE_PATH

BUILDDIRS = cexp.build

# make ours first
SUBDIRS+=$(subst distclean-recursive,.,$(filter distclean-recursive,$@))
SUBDIRS+=$(subst clean-recursive,.,$(filter clean-recursive,$@))
SUBDIRS+=rtemsNfs
SUBDIRS+=libbspExt

SUBDIRS+=cexp.build

# these depend on cexp and/or tecla
ifeq ($(RTEMS_BSP),"svgm")
SUBDIRS+=svgmWatchdog
endif
SUBDIRS+=monitor
SUBDIRS+=telnetd
SUBDIRS+=system
ifeq ($(RTEMS_BSP),"svgm")
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


#default target
install:

all: error

error:
	@echo
	@echo YOU MUST USE THE 'install' TARGET FROM THIS DIRECTORY
	@echo
	exit 1

clean-recursive \
distclean-recursive: $(addsuffix /Makefile,$(BUILDDIRS))

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/directory.cfg
include ${CONFIG.CC}

install: $(INSTDIRS) $(SUBDIRS)

CLOBBER_ADDITIONS+=$(BUILDDIRS) $(RTEMS_SITE_INSTALLDIR)

###preinstall-recursive: $(INSTDIRS) $(SUBDIRS)

%.build/Makefile:
	test -d $(dir $@) || $(MKDIR) $(dir $@)
	echo distclean: > $@
	echo clean: >> $@

%.build: %
	test -d $@ || $(MKDIR) $@
	cd	$@ ; ../$^/configure --host=$(RTEMS_CPU)-rtems --disable-nls --prefix=$(RTEMS_SITE_INSTALLDIR) --with-newlib

$(INSTDIRS):
	$(MKDIR) -p $@
