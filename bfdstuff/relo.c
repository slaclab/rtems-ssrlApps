#include <bfd.h>
#include <stdio.h>
#include "tab.h"

#ifdef USE_MDBG
#include <mdbg.h>
#endif

typedef struct LinkDataRec_ {
	 void		*chunk;
	 asymbol	**st;
	 asection	*tsill;
} LinkDataRec, *LinkData;

static void
s_count(bfd *abfd, asection *sect, PTR arg)
{
long *sizep=(long*)arg;
	printf("Section %s, flags 0x%08x\n",
			sect->name, sect->flags);
	printf("size: %i\n",
			bfd_section_size(abfd,sect));
	if (SEC_ALLOC & sect->flags) {
		/* TODO align */
		*sizep+=bfd_section_size(abfd,sect);
	}
}

static void
s_setvma(bfd *abfd, asection *sect, PTR arg)
{
char **addr=(char**)arg;

	if (SEC_ALLOC & sect->flags) {
		/* TODO align */
		printf("%s allocated at 0x%08x\n",
				bfd_get_section_name(abfd,sect),
				*addr);
		bfd_set_section_vma(abfd,sect,*addr);
		*addr+=bfd_section_size(abfd,sect);
		sect->output_section = sect;
	}
}


static void
s_reloc(bfd *abfd, asection *sect, PTR arg)
{
LinkData	ld=(LinkData)arg;
int			i;
long		err;
char		buf[1000];

	if ((SEC_RELOC | SEC_ALLOC) == ((SEC_RELOC | SEC_ALLOC) & sect->flags)) {
		arelent **cr=0,r;
		long	sz;
		sz=bfd_get_reloc_upper_bound(abfd,sect);
		if (sz<=0) {
			fprintf(stderr,"No relocs for section %s???\n",
					bfd_get_section_name(abfd,sect));
			return;
		}
		cr=(arelent**)xmalloc(sz);
		sz=bfd_canonicalize_reloc(abfd,sect,cr,ld->st);
		if (sz<=0) {
			fprintf(stderr,"ERROR: unable to canonicalize relocs\n");
			free(cr);
			return;
		}
		bfd_get_section_contents(abfd,sect,bfd_get_section_vma(abfd,sect),0,bfd_section_size(abfd,sect));
		for (i=0; i<sect->reloc_count; i++) {
			arelent *r=cr[i];
			printf("relocating (%s=",
					bfd_asymbol_name(*(r->sym_ptr_ptr))
					);
			if (bfd_is_und_section(bfd_get_section(*r->sym_ptr_ptr))) {
				printf("UNDEFINED), skipping...\n");
			} else {
				printf("0x%08x)->0x%08x\n",
				bfd_asymbol_value(*(r->sym_ptr_ptr)),
				r->address);

				if ((err=bfd_perform_relocation(
					abfd,
					r,
					bfd_get_section_vma(abfd,sect),
					sect,
					0 /* output bfd */,
					0)))
				fprintf(stderr,"Relocation failed (err %i)\n",err);
			}
		}
		free(cr);
	}
}

static asymbol **
slurp_symtab(bfd *abfd, LinkData ld)
{
asymbol **rval=0,*sp;
int		i;
long	ss;

	if (!(HAS_SYMS & bfd_get_file_flags(abfd))) {
		fprintf(stderr,"No symbols found\n");
		return 0;
	}
	if ((ss=bfd_get_symtab_upper_bound(abfd))<0) {
		fprintf(stderr,"Fatal error: illegal symtab size\n");
		return 0;
	}
	if (ss) {
		rval=(asymbol**)xmalloc(ss);
	}
	if (bfd_canonicalize_symtab(abfd,rval) <= 0) {
		fprintf(stderr,"Canonicalizing symtab failed\n");
		free(rval);
		return 0;
	}
	/* resolve undefined symbols */
	for (i=0; sp=rval[i]; i++) {
		if (bfd_is_und_section(bfd_get_section(sp))) {
			TstSym ts;
			for (ts=tstTab; ts->name && strcmp(ts->name, sp->name); ts++)
					/* do nothing else */;
			if (ts->name) {
				sp=rval[i]=bfd_make_empty_symbol(abfd);
				sp->name=ts->name;
				sp->value=ts->value;
				sp->section=ld->tsill;
				sp->flags=BSF_GLOBAL;
			}
		}
	}
	return rval;
}


int
main(int argc, char **argv)
{
bfd 			*abfd=0;
long			total;
LinkDataRec		ldr={0};
unsigned char	*addr;
int				rval=1;
TstSym			sm;

	if (argc<2) {
		fprintf(stderr,"Need filename arg\n");
		goto cleanup;
	}
	bfd_init();
#ifdef USE_MDBG
	mdbgInit();
#endif
	abfd=bfd_openr(argv[1],0);
	if (!bfd_check_format(abfd, bfd_object)) {
		fprintf(stderr,"Invalid format\n");
		goto cleanup;
	}
	total=0;
	bfd_map_over_sections(abfd, s_count, &total);
	ldr.tsill=bfd_make_section_old_way(abfd,".tsillsym");
	ldr.tsill->output_section=ldr.tsill;

	for (sm=tstTab; sm->name; sm++) {
		asymbol			*s;
		s=bfd_make_empty_symbol(abfd);
		s->name=sm->name;
		s->value=sm->value;
		s->section=ldr.tsill;
		s->flags=BSF_GLOBAL;
	}


	addr=ldr.chunk=xmalloc(total);
	bfd_map_over_sections(abfd, s_setvma, &addr);
	if (!(ldr.st=slurp_symtab(abfd,&ldr))) {
		fprintf(stderr,"Error reading symbol table\n");
		goto cleanup;
	}
	bfd_map_over_sections(abfd, s_reloc, &ldr);
	rval=0;
cleanup:
	if (ldr.st) free(ldr.st);
	if (abfd) bfd_close_all_done(abfd);
	if (ldr.chunk) free(ldr.chunk);
#ifdef USE_MDBG
	printf("Memory leaks found: %i\n",mdbgPrint(0,0));
#endif
	return rval;
}
