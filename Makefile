#
#  $Id$
#

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/directory.cfg

SUBDIRS=$(filter-out powerpc-rtems config cexp bin img $(wildcard *.src) $(wildcard Makefile*) $(wildcard README*), $(wildcard *))

blah:
	echo $(PROJECT_RELEASE)
