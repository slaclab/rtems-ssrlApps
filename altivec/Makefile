#
#  Makefile.leaf,v 1.7 2002/07/22 22:56:09 joel Exp
#
# Templates/Makefile.leaf
# 	Template leaf node Makefile
#

# C source names, if any, go here -- minus the .c
C_PIECES=vec_context vec_alloc
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

# Notes about compiler flags:
#
# -mno-altivec
#   Compile *without* altivec to avoid implicit use of
#   Vector registers by the compiler.
#   All access to the vector unit is by means of inline assembly.
#
# -mabi=altivec
#   Ensures proper (16byte) stack alignment. Note that
#   *every* single file linked to the application must
#   be compiled with -mabi=altivec (-mno-eabi should have
#   the same effect as far as stack alignment is concerned.
#   I don't know of any other effects -mabi=altivec has).
#
# -mvrsave=no
#   All code should be compiled with -mvrsave=no. The
#   altivec support doesn't use vrsave, i.e., always
#   switches all VRs -- the performance gained by honouring
#   vrsave seems to be extremely marginal (7450).
#   Since it's not used by the system, the compiler
#   shouldn't generate overhead code to maintain vrsave
#   and this is accomplished by choosing this option).
#
# NOTE: If you DO want the compiler to maintain vrsave
#   then both, -mabi=altivec and -mvsave=yes are needed.
#
VEC_CFLAGS=-mno-altivec -mabi=altivec -mvrsave=no

# C++ source names, if any, go here -- minus the .cc
CC_PIECES=
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES=

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

# If your PGMS target has the '.exe' extension, a statically
# linked application is generated.
# If it has a '.obj' extension, a loadable module is built.

PGMS=${ARCH}/vec.obj

#  List of RTEMS Classic API Managers to be included in the application
#  goes here. Use:
#     MANAGERS=all
# to include all RTEMS Classic API Managers in the application or
# something like this to include a specific set of managers.
#     MANAGERS=io event message rate_monotonic semaphore timer
#
# UNUSED for loadable modules
MANAGERS=

ifndef RTEMS_MAKEFILE_PATH
$(error you need to set the RTEMS_MAKEFILE_PATH environment variable)
endif

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/leaf.cfg

#
# (OPTIONAL) Add local stuff here using +=
#

DEFINES  +=
CPPFLAGS +=
CFLAGS   += $(VEC_CFLAGS)

#
# CFLAGS_DEBUG_V are used when the `make debug' target is built.
# To link your application with the non-optimized RTEMS routines,
# uncomment the following line:
# CFLAGS_DEBUG_V += -qrtems_debug
#

LD_PATHS  +=
LD_LIBS   += 
LDFLAGS   +=

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

CLEAN_ADDITIONS += 
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(PGMS)

#How to make a relocatable object
$(filter %.obj, $(PGMS)): ${OBJS}
	$(make-obj)

#How to make an executable (statically linked)
$(filter %.exe,$(PGMS)): ${LINK_FILES}
	$(make-exe)
ifdef ELFEXT
ifdef XSYMS
	$(XSYMS) $(@:%.exe=%.$(ELFEXT)) $(@:%.exe=%.sym)
endif
endif

ifndef RTEMS_SITE_INSTALLDIR
RTEMS_SITE_INSTALLDIR = $(PROJECT_RELEASE)
endif

${RTEMS_SITE_INSTALLDIR}/include \
${RTEMS_SITE_INSTALLDIR}/lib \
${RTEMS_SITE_INSTALLDIR}/bin:
	test -d $@ || mkdir -p $@

# Install the program(s), appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
#
#  - Some BSPs might generate bootable executables in yet another
#    format (such as .srec) and you might need to extend the rule
#    below so the essential files get installed. YMMV.
install:  all $(RTEMS_SITE_INSTALLDIR)/bin
	$(INSTALL_VARIANT) -m 555 ${PGMS} ${PGMS:%.exe=%.bin} ${PGMS:%.exe=%.sym} ${RTEMS_SITE_INSTALLDIR}/bin

