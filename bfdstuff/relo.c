#include <bfd.h>
#include <stdio.h>

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

static asymbol	**st=0;

static void
s_reloc(bfd *abfd, asection *sect, PTR arg)
{
char	**addr=(char**)arg;
int		i;
long	err;
char	buf[1000];

	if (SEC_RELOC & sect->flags) {
		arelent **cr=0,r;
		long	sz;
		sz=bfd_get_reloc_upper_bound(abfd,sect);
		if (sz<=0) {
			fprintf(stderr,"No relocs for section %s???\n",
					bfd_get_section_name(abfd,sect));
			return;
		}
		cr=(arelent**)xmalloc(sz);
		sz=bfd_canonicalize_reloc(abfd,sect,cr,bfd_get_outsymbols(abfd));
		if (sz<=0) {
			fprintf(stderr,"ERROR: unable to canonicalize relocs\n");
			return;
		}
		bfd_get_section_contents(abfd,sect,bfd_get_section_vma(abfd,sect),0,bfd_section_size(abfd,sect));
		for (i=0; i<sect->reloc_count; i++) {
			arelent *r=cr[i];
			printf("relocating (%s=0x%08x)->0x%08x\n",
				bfd_asymbol_name(*(r->sym_ptr_ptr)),
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
}

static asymbol **
slurp_symtab(bfd *abfd)
{
asymbol **rval=0;
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
	return rval;
}


int
main(int argc, char **argv)
{
bfd 			*abfd;
long			total;
void			*chunk;
unsigned char	*addr;
int				rval=1;

	if (argc<2) {
		fprintf(stderr,"Need filename arg\n");
		goto cleanup;
	}
	bfd_init();
	abfd=bfd_openr(argv[1],0);
	if (!bfd_check_format(abfd, bfd_object)) {
		fprintf(stderr,"Invalid format\n");
		goto cleanup;
	}
	total=0;
	bfd_map_over_sections(abfd, s_count, &total);
	addr=chunk=xmalloc(total);
	bfd_map_over_sections(abfd, s_setvma, &addr);
	if (!(st=slurp_symtab(abfd))) {
		fprintf(stderr,"Error reading symbol table\n");
		goto cleanup;
	}
	bfd_map_over_sections(abfd, s_reloc, chunk);
	rval=0;
cleanup:
	if (st) free(st);
	bfd_close_all_done(abfd);
	return rval;
}
