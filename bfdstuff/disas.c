#include <bfd.h>
#include "dis-asm.h"
#include <stdio.h>

#ifdef USE_MDBG
#include <mdbg.h>
#endif

const char	*target="i686-pc-linux-gnu";
static int	dynsym=0;

char  halla;
short balla;
int   kralla;

static disassembler_ftype da;

static void
merf(int status, bfd_vma addr, disassemble_info *inf)
{
fprintf(stderr,"INFO was %x\n",inf);
}

static int
mrf(bfd_vma addr, bfd_byte *myaddr, unsigned int len, disassemble_info *inf)
{
		char *a=(char*)addr;
#if 0
		memcpy(myaddr,(void*)addr,len);
#else
		myaddr[0]=a[3];
		myaddr[1]=a[2];
		myaddr[2]=a[1];
		myaddr[3]=a[0];
#endif
		fprintf(stderr,"MRF: (a %08x) 0x%08x\n",addr,*(unsigned long*)myaddr);
		return 0;
}

int
main(int argc, char **argv)
{
bfd			*abfd;
char		*fnam=argc > 1 ? argv[1] : argv[0];
long		nsyms;
PTR			minisyms=0;
char		*msp;
unsigned	sz,i;
asymbol		*store_s, *as;
const char	**matching;
disassemble_info dinf;
int			len;
bfd_vma		addr;

	bfd_init();
	if (0 && !bfd_set_default_target(target)) {
		fprintf(stderr,"Unable to set default BFD target\n");
		return 1;
	}
#ifdef USE_MDBG
	mdbgInit();
#endif
	target = argc>2 ? argv[2] : 0;
	abfd=bfd_openr(fnam, target);
	if (!abfd) {
		bfd_perror("opening file");
		return 1;
	}
	if ( !bfd_check_format(abfd, bfd_object)) {
		fprintf(stderr,"unrecognized format\n");
		return 1;
		
	}
	matching=bfd_target_list();
	if (matching) {
		char **mp=matching;
		while (*mp)
				printf("%s\n",*mp++);
		free(matching);
	}
	printf("Architecture: '%s'\n",bfd_printable_name(abfd));
	if (!(bfd_get_file_flags(abfd) & HAS_SYMS)) {
		fprintf(stderr,"File has no symbols\n");
		return 1;
	}
	if ((nsyms=bfd_read_minisymbols(abfd, dynsym, &minisyms, &sz)) <0) {
		bfd_perror("reading minisyms");
		return 1;
	}	
	store_s=bfd_make_empty_symbol(abfd);
//	nsyms=0;/*TSILL */
	for (i=0,msp=minisyms; i<nsyms; i++,msp+=sz) {
		as=bfd_minisymbol_to_symbol(abfd, dynsym, msp, store_s);
		if (as) {
			symbol_info inf;
			bfd_get_symbol_info(abfd, as, &inf);
			if (bfd_is_undefined_symclass(inf.type)) {
				printf("  %8s","");
			} else {
				printf("0x%08x",inf.value);
			}
			printf(" %c %s flgs 0x%08x\n",
						inf.type,inf.name, as->flags);
		} else {
			fprintf(stderr,"error reading symbol\n");
		}
	}
	if (minisyms) {
		free(minisyms);
		minisyms=0;
	}

	if (!(da=disassembler(abfd))) {
		fprintf(stderr,"no disassembler\n");
		return 1;
	}
	INIT_DISASSEMBLE_INFO(dinf, stderr, fprintf);
	fprintf(stderr,"ORIG INF %x\n",&dinf);
//	dinf.read_memory_func=mrf;
	dinf.buffer=(bfd_byte *)main;
	dinf.buffer_vma=(bfd_vma)main;
	dinf.buffer_length=100;
	if (bfd_big_endian(abfd)) {
		dinf.display_endian = dinf.endian=BFD_ENDIAN_BIG;
	} else if (bfd_little_endian(abfd)) {
		dinf.display_endian = dinf.endian=BFD_ENDIAN_LITTLE;
	} else {
		fprintf(stderr,"UNKNOWN ENDIANNESS\n");
		return 1;
	}
	len=10;
	addr=dinf.buffer_vma;
	while(len--) {
		dinf.fprintf_func(dinf.stream,"0x%08x: ",addr);
		dinf.fprintf_func(dinf.stream,"(0x%08x) ",*(unsigned long*)addr);
		dinf.fprintf_func(dinf.stream,"%2i ",dinf.bytes_per_line);
		addr+=da(addr,&dinf);
		fprintf(stderr,"\n");
	}
	bfd_close_all_done(abfd);
#ifdef USE_MDBG
	printf("Memory leaks found: %i\n",mdbgPrint(0,0));
#endif

return 0;
}
