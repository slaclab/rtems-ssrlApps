# dis-asm.h is unfortunately not installed along with the bfd headers.
# However, different versions of libopcodes don't grok different versions'
# header :-(. Therefore, we must provide a consistent set of library / header
# paths...
##### linux 
#BINUTILS=/home/till/gnu/binutils-2.11.1
#ARCH=build-i386-linux
#CC=gcc
##### rtems
BINUTILS=/home/till/rtems/sandbox/rtemsApplications/cexp/binutils-2.13
BINUTILS_BUILD=/home/till/rtems/sandbox/rtemsApplications/cexp.build/binutils-2.13
CC=powerpc-rtems-gcc
LD=powerpc-rtems-ld
#ARCH=build-ppc-linux
DEBUG=DEBUG
CFLAGS+=-DUSE_ELF_STUFF -I $(BINUTILS)/bfd
#MDBG=/home/till/slac/xfm/src/mdbg/mdbg.o


LDFLAGS+=-L $(BINUTILS_BUILD)/bfd/.libs/ -L $(BINUTILS_BUILD)/$(ARCH)/libiberty/ -L $(BINUTILS_BUILD)/$(ARCH)/opcodes/.libs/
CFLAGS+= -I $(BINUTILS)/include  -I$(BINUTILS_BUILD)/bfd
ifdef DEBUG
CFLAGS+=-g -D$(DEBUG)
endif
ifdef MDBG
CFLAGS+=-DUSE_MDBG -I$(dir $(MDBG))
LDFLAGS+= $(MDBG) -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,free
endif

all: disas blah.o relo tab.o

disas:	disas.o
	$(CC) -o $@ $^ $(LDFLAGS) -lbfd -lopcodes -liberty

relo:	relo.o tab.o
	$(CC) -o $@ $^ $(LDFLAGS) -lbfd -liberty

relo.obj: relo.o tab.o
	$(LD) -r -o $@ $^

lnk: lnk.o
	$(CC) -o $@ $^ $(LDFLAGS) -Wl,-static  -lbfd -liberty

clean:
	$(RM) *.o disas relo lnk
