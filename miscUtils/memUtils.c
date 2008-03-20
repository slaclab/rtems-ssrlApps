#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <rtems.h>

#if ! defined( __RTEMS_MAJOR__ ) || ! defined( __RTEMS_MINOR__ ) || ! defined( __RTEMS_REVISION__ )
#error "missing __RTEMS_MAJOR__ & friends -- did you include <rtems.h>" ?
#endif
#define ISMINVERSION(ma,mi,re) \
	(    __RTEMS_MAJOR__  > (ma)	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__  > (mi))	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__ == (mi) && __RTEMS_REVISION__ >= (re)) \
    )

/* miscellaneous memory utilities (dump, modify, ... )
 * as a CEXP loadable module.
 * Author: Till Straumann 2005/4
 */

#if 0
#define PREF "0x"
#else
#define PREF ""
#endif

#define DEFSIZE 4

static void dumpchars(char **pstr, char *end)
{
	printf("  ");
	while ( *pstr < end ) {
		fputc(isprint(**pstr) ? **pstr : '.', stdout);
		(*pstr)++;
	}
}


static void dumpval(unsigned address, int size)
{
	switch(abs(size)) {
		case 4: printf(PREF"%08x", *(unsigned*)address); 		break;
		case 2: printf(PREF"%04x", *(unsigned short*)address);	break;
		case 1: printf(PREF"%02x", *(unsigned char *)address);	break;
		default:
		break;
	}
}

int
md(unsigned address, int count, int size)
{
int i = 0;
char *oadd = (char*)address;
	switch (size) {
		default:	size=DEFSIZE;
		case 1: case 2: case 4:
		break;
	}
	while ( i < count) {
		if ( i%16 == 0 ) {
			if (i) {
				dumpchars(&oadd, (char*)address);
			}
			printf("\n0x%08x:", address);
		}
		printf("  ");
		dumpval(address,size);
		i += size;
		address += size;
	}

	if (count > 0) {
		printf("%*s",(((i+15)/16)*16 - i)/size * (2 + (int)strlen(PREF) + 2*size),"");
		dumpchars(&oadd,(char*)address);
	}
	fputc('\n',stdout);
	return 0;
}

#define LBSZ 40

int
mm(unsigned address, int size)
{
char     buf[LBSZ];
char     *endp;
unsigned v;

	switch (size) {
		default: size = DEFSIZE;
		case 1: case 2: case 4:
		break;
	}
	do {
		printf("0x%08x:  ", address);
		dumpval(address,size);
		printf(" >");
		fflush(stdout);
		fgets(buf,LBSZ,stdin);
		v = strtoul(buf,&endp,0);
		if (endp>buf) {
			switch (abs(size)) {
				case 4: *(unsigned*)address = v; 		break;
				case 2: *(unsigned short*)address = v;	break;
				case 1: *(unsigned char*)address = v;	break;
				default: return -1; /* should never get here */
			}
			address += size;
		} else {
			switch (toupper(buf[0])) {
				case '^': size = -abs(size);
					break;
				case 'V': size = abs(size);
					break;
				case '\n':
				case '\r':
					break;
				default  : address-= size;
					break;
			}
			address += size;
		}
	} while ( strlen(buf) && '.'!=buf[0] );
	return 0;
}

static int
doWrite(int fd, unsigned start, unsigned len)
{
int written;
unsigned here = start;
    while ( len > 0 ) {
        written = write(fd, (void*)here, len);
        if ( written < 0 ) {
            perror("Unable to write; try a different server/file");
            return -1;
        }
		len  -= written;
        here += written;
    }
    return here-start;
}

unsigned __BSP_mem_size_dummy = 0;

extern unsigned BSP_mem_size __attribute__((weak, alias("__BSP_mem_size_dummy")));
 
unsigned
coredump(char *fn, unsigned long start, unsigned long size, int forceWrite)
{
int fd;
unsigned valAtZero;
unsigned mem_size;
extern unsigned BSP_mem_size;
unsigned rval = -1;
unsigned flags = O_WRONLY|O_CREAT|O_EXCL|O_TRUNC;

	if ( 0 == size )
		size = BSP_mem_size;

	if ( !size ) {
		fprintf(stderr,"Unable to guess memory size; use 'coredump(file,mem_start,mem_size)'\n");
		return -1;
	}

	if ( forceWrite )
		flags &= ~O_EXCL;

	if ( (fd = open(fn,flags,0644)) < 0 ) {
		perror("Opening image file on host failed");
		return -1;
	}

	/* round stop to the next power of two; size of the page tables
	 * might have been subtracted
	 */
	for ( mem_size=1; mem_size<size; mem_size<<=1)
		;

	fprintf(stderr,"Dumping %i (0x%08x) bytes @0x%08lx, patience might be needed...",
			mem_size, mem_size, start);

	/* address 0 is special; we must not pass 0 to 'write()' */
	valAtZero = *(unsigned *)start;
	if ( doWrite(fd, (unsigned)&valAtZero, sizeof(valAtZero)) < 0 ) {
		goto cleanup;
	}
	if ( doWrite(fd, start + (unsigned)sizeof(valAtZero), mem_size - sizeof(valAtZero)) < 0 ) {
		goto cleanup;
	}
	fprintf(stderr,"done.\nMemory image successfully written");
	rval = mem_size;

cleanup:
	fputc('\n',stderr);
	if ( 0 <= fd ) {
		close(fd);
	}
	return rval;
}

#ifdef __PPC__

#define MFBATS(bat,idx,l,u) __asm__ __volatile__("mf"bat"l %0,%2; mf"bat"u %1,%2":"=r"(l),"=r"(u):"i"(idx),"0"(l),"1"(u))
static int
pbat(unsigned bat, int dbat)
{
unsigned u=0,l=0; /* initialize to silence compiler warnings */
unsigned bl, bepi, brpn;
char *szstr;
	if ( dbat ) {
		switch ( bat ) {
			case 0: MFBATS("dbat", 0, l, u); break;
			case 1: MFBATS("dbat", 1, l, u); break;
			case 2: MFBATS("dbat", 2, l, u); break;
			case 3: MFBATS("dbat", 3, l, u); break;
			default:
				fprintf(stderr,"invalid index -- use 0..3\n");
				return -1;
		}
	} else {
		switch ( bat ) {
			case 0: MFBATS("ibat", 0, l, u); break;
			case 1: MFBATS("ibat", 1, l, u); break;
			case 2: MFBATS("ibat", 2, l, u); break;
			case 3: MFBATS("ibat", 3, l, u); break;
			default:
				fprintf(stderr,"invalid index -- use 0..3\n");
				return -1;
		}
	}
	bepi = u & ~ ((1<<(31-14)) -1 );
	bl   = (u & ((1<<(31-18))-1)) >> (31-29);
	switch (bl) {
		case (1<<0)-1: szstr = "128k"; break;
		case (1<<1)-1: szstr = "256k"; break;
		case (1<<2)-1: szstr = "512k"; break;
		case (1<<3)-1: szstr = "1M"; break;
		case (1<<4)-1: szstr = "2M"; break;
		case (1<<5)-1: szstr = "4M"; break;
		case (1<<6)-1: szstr = "8M"; break;
		case (1<<7)-1: szstr = "16M"; break;
		case (1<<8)-1: szstr = "32M"; break;
		case (1<<9)-1: szstr = "64M"; break;
		case (1<<10)-1: szstr = "128M"; break;
		case (1<<11)-1: szstr = "256M"; break;
		default: szstr = "INVALID"; break;
	}
	bl   = ~bl << (31-14);
	brpn = l & ~ ((1<<(31-14)) -1 );

	printf("Raw upper 0x%08x lower 0x%08x\n\n", u,l);
	printf("Effective Address: 0x%08x\n", bepi);
	printf("Block Length/Mask: 0x%08x (%s)\n", bl, szstr);
	printf("Physical  Address: 0x%08x\n\n", brpn);
	printf("WIMG: 0x%1x, PP: ", (l>>3) & 0xf);
	switch ( l & 3 ) {
		case 0: printf("NO"); break;
		case 2: printf("RW"); break;
		default: printf("RO"); break;
	}
	printf(" access, Vs %i Vp %i\n", u&2, u&1);
	return 0;
}

#if !ISMINVERSION(4,7,0)
int getdbat(unsigned i)
{
return pbat(i,1);
}
#endif

#if !ISMINVERSION(4,8,99)
int getibat(unsigned i)
{
return pbat(i,0);
}
#endif

#endif /* __PPC__ */

#if !defined(MAIN)
#ifdef HAVE_CEXP
#include <cexpHelp.h>
CEXP_HELP_TAB_BEGIN(memutils)
	HELP(
"Inspect and modify memory. Type '.' to exit, '^' / 'v' to change directions.\n"
"'word_size' can be '4', '2', or '1' bytes - defaults to '4'\n",
		int, mm, (unsigned address, int word_size)
	),
	HELP(
"Dump memory. Dump 'count' bytes starting at 'address' using a word length of\n"
"'word_size' bytes ('4', '2' or '1' - defaults to '4').\n",
		int, md, (unsigned address, int count, int word_size)
	),
	HELP(
"Write memory image file for use with GDB on host (needs 'gencore' utility)\n"
"if 'size' is 0, BSP_mem_size is used (PPC BSP only)\n"
"otherwise, you must know your board's memory boundaries...\n"
"The 'forceWrite' flags allows you to overwrite existing files\n",
		int, coredump, (char *filename, unsigned start_addr, unsigned size, int forceWrite)
	),
#ifdef __PPC__
#if !ISMINVERSION(4,7,0)
	HELP(
"Display Data BAT register i (0..3)\n",
		int, getdbat, (unsigned batno)
	),
#endif
#if !ISMINVERSION(4,8,99)
	HELP(
"Display Instruction BAT register i (0..3)\n",
		int, getibat, (unsigned batno)
	),
#endif
#endif
CEXP_HELP_TAB_END
#endif
#endif


#ifdef MAIN
static int valdig(int *pc, int base)
{
int rval = *pc - '0';
	if ( 0<=rval && 7>=rval ) {
	} else if ( 8<=rval && 9>=rval ) {
		if ( base <= 8 )
			rval = -1;
	} else if ( 16==base ) {
		rval = toupper(*pc) - 'A';
		if ( 0 <= rval && 5 >= rval ) {
			rval += 10;
		} else 
			rval = -1;
	} else {
		rval = -1;
	}
	if ( rval >= 0 )
		*pc = getchar();
	return rval;
}

static int getn(int *pn, int *pc)
{
int dig;
int base = 10;

	*pn = 0;

	while ( ' '==*pc || '\t' == *pc )
		*pc = getchar();

	if ( '0'<=*pc && '9'>=*pc ) {
		if ( '0' == *pc ) {
			*pc = getchar();
			if ( 'x' == *pc || 'X' == *pc ) {
				base = 16;
				*pc=getchar();
			} else if ( '0' <= *pc && '7' >= *pc ) {
				base = 8;
			}
		}
		if ( (dig = valdig(pc,base)) >= 0 ) {
			do {
			  *pn = (*pn) * base + dig;
			} while ( (dig = valdig(pc,base)) >= 0 );
			return 0;
		}
	}
	return -1;
}

int
MAIN(int argc, char *argv)
{
int ch;
int		 a,c,s;
#if 0
unsigned a;
int dummy[200];
	for ( s=0; s<200; s++)
		dummy[s]=s;
	a = (unsigned)dummy;
	mm(a,4);
	mm(a,2);
	mm(a,1);
	for (s=0,c=8; scanf("%i %i",&c,&s) > 0; s=0,c=8 ) {
		md(a,c,s);
	}
#else
	while ( (ch = getchar()) >= 0 ) {
		switch ( ch ) {
			case 'm':	ch = getchar();
						if ( getn(&a, &ch) ) {
							fprintf(stderr,"\nusage: m address [size]\n");
						} else {
							if ( getn(&s,&ch) )
								s = 4;
							else switch(s) {
								default: s = 4;
								case 1: case 2: case 4: break;
							}
							mm(a,s);
						}
			break;

			case 'd':	ch = getchar();
						if ( getn(&a, &ch) ) {
							fprintf(stderr,"\nusage: d address [count [size]]\n");
						} else {
							if ( getn(&c,&ch) )
								c = 16;
							else if ( getn(&s,&ch) )
								s = 4;
							else switch(s) {
								default: s = 4;
								case 1: case 2: case 4: break;
							}
							md(a,c,s);
						}
			break;

			case 'q':
			return 0;

			default: 
				fprintf(stderr,"\ninvalid command; use 'd', 'm' or 'q'\n");
			break;
		}
		while ( '\n' != ch )
			ch = getchar();
	}
#endif
	return 0;
}
#endif
