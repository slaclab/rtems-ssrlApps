# dis-asm.h is unfortunately not installed along with the bfd headers.
# However, different versions of libopcodes don't grok different versions'
# header :-(. Therefore, we must provide a consistent set of library / header
# paths...
BINUTILS=/home/till/binutils-2.11.1
#ARCH=build-i386-linux
ARCH=build-ppc-linux
DEBUG=DEBUG
CFLAGS+=-DUSE_ELF_STUFF -I $(BINUTILS)/bfd
#MDBG=/home/till/slac/xfm/src/mdbg/mdbg.o
CC=gcc


LDFLAGS+=-L $(BINUTILS)/$(ARCH)/bfd/.libs/ -L $(BINUTILS)/$(ARCH)/libiberty/ -L $(BINUTILS)/$(ARCH)/opcodes/.libs/
CFLAGS+= -I $(BINUTILS)/include 
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

lnk: lnk.o
	$(CC) -o $@ $^ $(LDFLAGS) -Wl,-static  -lbfd -liberty

clean:
	$(RM) *.o disas relo lnk
