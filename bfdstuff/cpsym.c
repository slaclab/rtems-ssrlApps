#include <bfd.h>

#include <stdio.h>

static char *targ="elf32-i386";
static char *tarch="i386";

int
main(int argc, char **argv)
{
bfd							*obfd,*ibfd;
const bfd_arch_info_type	*arch;
int							i,nsyms;
asymbol						**isyms=0, **osyms=0;
int							rval=-1;

		bfd_init();

		arch = bfd_scan_arch(tarch);

		i = 1;
		if ( i>=argc || !(ibfd=bfd_openr(argv[i],0)) ||
			! bfd_check_format(ibfd,bfd_object)) {
			fprintf(stderr,"Unable to open input file\n");
			goto cleanup;
		}

		i++;

		arch=bfd_get_arch_info(ibfd);
		
		if (!arch) {
			fprintf(stderr,"Unable to determine architecture\n");
			goto cleanup;
		}

		if (!(obfd=bfd_openw("foo",0)) ||
			! bfd_set_format(obfd,bfd_object) ||
			! bfd_set_arch_mach(obfd, arch->arch, arch->mach)
		   )  {
			fprintf(stderr,"Unable to create output BFD\n");
			goto cleanup;
		}

		if (!(HAS_SYMS & bfd_get_file_flags(ibfd))) {
			fprintf(stderr,"No symbols found\n");
			goto cleanup;
		}
		if ((i=bfd_get_symtab_upper_bound(ibfd))<0) {
			fprintf(stderr,"Fatal error: illegal symtab size\n");
			goto cleanup;
		}

		/* Allocate space for the symbol table  */
		if (i) {
			isyms=(asymbol**)xmalloc((i));
			osyms=(asymbol**)xmalloc((i));
		}
		nsyms= i ? i/sizeof(asymbol*) - 1 : 0;

		if (bfd_canonicalize_symtab(ibfd,isyms) <= 0) {
			bfd_perror("Canonicalizing symtab");
			goto cleanup;
		}

		for (i=0; i<nsyms; i++) {
			osyms[i]          = bfd_make_empty_symbol(obfd);
			if (bfd_is_und_section(isyms[i]->section)) {
				osyms[i]->section = bfd_und_section_ptr;
			} else {
				osyms[i]->section = bfd_abs_section_ptr;
			}
			osyms[i]->value   = bfd_asymbol_value(isyms[i]) - bfd_get_section_vma(obfd,bfd_abs_section_ptr);
			osyms[i]->flags   = isyms[i]->flags;
			osyms[i]->name    = isyms[i]->name;
			bfd_copy_private_symbol_data(ibfd,isyms[i],obfd,osyms[i]);
		}

		bfd_set_symtab(obfd,osyms,nsyms);

		rval = 0;
cleanup:

		if (obfd)
			bfd_close(obfd);

		free(isyms);
		free(osyms);

		if (ibfd)
			bfd_close_all_done(ibfd);

		return rval;
}
