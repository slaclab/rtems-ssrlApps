#  $Id$
AUTOMAKE_OPTIONS=foreign

include $(top_srcdir)/rtems-pre.am

EXEEXT=$(OBJEXEEXT)

librtemsAltivec_a_SOURCES = vec_context.c vec_alloc.c
lib_LIBRARIES             = librtemsAltivec.a

EXTRA_DIST                = ppc_stack_probe.c

psim_bsp_DEFINES          = -DPSIM

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
AM_CFLAGS              += -mno-altivec -mabi=altivec -mvrsave=no
AM_CPPFLAGS            += $(@rtems_bsp@_bsp_DEFINES)

bin_PROGRAMS            = vec
vec_SOURCES             =
vec_LDADD               = $(librtemsAltivec_a_OBJECTS)
vec_DEPENDENCIES        = $(librtemsAltivec_a_OBJECTS)
vec_LINK                = $(OBJLINK)

include $(top_srcdir)/rtems.am
