#
#  $Id$
#

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
export RTEMS_MAKEFILE_PATH

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/directory.cfg

SUBDIRS=$(filter-out CVS bfdstuff powerpc-rtems config cexp bin img system $(wildcard *.src) $(wildcard Makefile*) $(wildcard README*), $(wildcard *))
# make system last
SUBDIRS+=system
