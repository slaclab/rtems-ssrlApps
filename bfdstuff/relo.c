#include <bfd.h>
#include <libiberty.h>
#include <stdio.h>
#include "tab.h"

#ifdef USE_MDBG
#include <mdbg.h>
#endif

#ifdef USE_ELF_STUFF
#include "elf-bfd.h"
#endif

#define CACHE_LINE_SIZE 32

/* an output segment description */
typedef struct SegmentRec_ {
	PTR				chunk;		/* pointer to memory */
	unsigned long	vmacalc;	/* working counter */
	unsigned long	size;
	unsigned		attributes; /* such as 'read-only' etc.; currently unused */
} SegmentRec, *Segment;

#define NUM_SEGS 1

typedef struct LinkDataRec_ {
	SegmentRec		segs[NUM_SEGS];
	asymbol			**st;
	asection		*csect;		/* new common symbols' section */
	int				errors;
	unsigned long	nSyms;		/* number of symbols defined by the loaded file */
	unsigned long	nSymChars;	/* size we need for the string table */
} LinkDataRec, *LinkData;

/* how to decide where a particular section should go */
static Segment
segOf(LinkData ld, asection *sect)
{
	/* multiple sections not supported (yet) */
	return &ld->segs[0];
}

/* determine the alignment power of a common symbol
 * (currently only works for ELF)
 */
#ifdef USE_ELF_STUFF
static __inline__ int
get_align_pwr(bfd *abfd, asymbol *sp)
{
register unsigned long rval=0,tst;
elf_symbol_type *esp;
	if (esp=elf_symbol_from(abfd, sp))
		for (tst=1; tst<esp->internal_elf_sym.st_size; rval++)
			tst<<=1;
	return rval;
}
#else
#define get_align_pwr(abfd,sp) (0)
#endif

static void
s_count(bfd *abfd, asection *sect, PTR arg)
{
Segment		seg=segOf((LinkData)arg, sect);
	printf("Section %s, flags 0x%08x\n",
			bfd_get_section_name(abfd,sect), sect->flags);
	printf("size: %i, alignment %i\n",
			bfd_section_size(abfd,sect),
			(1<<bfd_section_alignment(abfd,sect)));
	if (SEC_ALLOC & sect->flags) {
		seg->size+=bfd_section_size(abfd,sect);
		seg->size+=(1<<bfd_get_section_alignment(abfd,sect));
	}
}

static void
s_setvma(bfd *abfd, asection *sect, PTR arg)
{
Segment		seg=segOf((LinkData)arg, sect);

	if (SEC_ALLOC & sect->flags) {
		seg->vmacalc=align_power(seg->vmacalc, bfd_get_section_alignment(abfd,sect));
		printf("%s allocated at 0x%08x\n",
				bfd_get_section_name(abfd,sect),
				seg->vmacalc);
		bfd_set_section_vma(abfd,sect,seg->vmacalc);
		seg->vmacalc+=bfd_section_size(abfd,sect);
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

	if ( ! (SEC_ALLOC & sect->flags) )
		return;

	/* read section contents to its memory segment
	 * NOTE: this automatically clears section with
	 *       ALLOC set but with no CONTENTS (such as
	 *       bss)
	 */
	bfd_get_section_contents(
		abfd,
		sect,
		(PTR)bfd_get_section_vma(abfd,sect),
		0,
		bfd_section_size(abfd,sect)
	);

	/* if there are relocations, resolve them */
	if ((SEC_RELOC & sect->flags)) {
		arelent **cr=0,r;
		long	sz;
		sz=bfd_get_reloc_upper_bound(abfd,sect);
		if (sz<=0) {
			fprintf(stderr,"No relocs for section %s???\n",
					bfd_get_section_name(abfd,sect));
			return;
		}
		/* slurp the relocation records; build a list */
		cr=(arelent**)xmalloc(sz);
		sz=bfd_canonicalize_reloc(abfd,sect,cr,ld->st);
		if (sz<=0) {
			fprintf(stderr,"ERROR: unable to canonicalize relocs\n");
			free(cr);
			return;
		}
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
					(PTR)bfd_get_section_vma(abfd,sect),
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
asymbol 		**rval=0,*sp;
int				i,errs=0;
long			ss;
unsigned long	csect_size=0;
unsigned		num_new_commons=0;

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

	/* resolve undefined and common symbols;
	 * find name clashes
	 */
	for (i=0; sp=rval[i]; i++) {
		asection *sect=bfd_get_section(sp);
		TstSym		ts;

		/* we only care about global symbols
		 * (NOTE: undefined symbols are neither local
		 *        nor global)
		 */
		if ( (BSF_LOCAL & sp->flags) )
			continue;

		ts=tstSymLookup(bfd_asymbol_name(sp));

		if (bfd_is_und_section(sect)) {

			if (ts) {
				sp=rval[i]=bfd_make_empty_symbol(abfd);
				bfd_asymbol_name(sp) = bfd_asymbol_name(ts);
				sp->value=ts->value;
				sp->section=bfd_abs_section_ptr;
				sp->flags=BSF_GLOBAL;
			} else {
				fprintf(stderr,"Unresolved symbol: %s\n",bfd_asymbol_name(sp));
				errs++;
			}
		}
		else if (bfd_is_com_section(sp)) {
			if (ts) {
				/* use existing value */
				sp = bfd_make_empty_symbol(abfd);
				/* TODO: check size and alignment */
				/* copy pointer to old name */
				bfd_asymbol_name(sp) = bfd_asymbol_name(rval[i]);
				sp->value=ts->value;
				sp->section=bfd_abs_section_ptr;
				sp->flags=BSF_GLOBAL;
			} else {
				/* it's the first definition of this common symbol */
				asymbol *swap;

				/* we'll have to add it to our internal symbol table */
				ld->nSyms++;
				ld->nSymChars+=strlen(bfd_asymbol_name(sp))+1;
				sp->flags |= BSF_KEEP;

				/* increase section size */
				csect_size += bfd_asymbol_value(sp);

				/* this is a new common symbol; we move all of these
				 * to the beginning of the 'st' list
				 */
				swap=rval[num_new_commons];
				rval[num_new_commons++]=sp;
				sp=swap;
			}
			rval[i]=sp; /* use new instance */
		} else {
			if (ts) {
				fprintf(stderr,"Symbol '%s' already exists\n",bfd_asymbol_name(sp));

				errs++;
				/* TODO: check size and alignment; allow multiple
				 *       definitions???
				 */
			} else {
				/* new symbol defined by the loaded object; account for it */
				ld->nSyms++;
				ld->nSymChars+=strlen(bfd_asymbol_name(sp))+1;
				/* mark for second pass */
				sp->flags |= BSF_KEEP;
			}
		}
	}

	/* TODO
		sort st[0]..st[num_new_commons-1] by alignment
		 * (MUST be powers of two)
	 */

	/* TODO buildCexpSymtab(); */

	if (num_new_commons) {
		unsigned long tmp,val;

		/* our common section alignment is the maximal alignment
		 * found during the sorting process which is the alignment
		 * of the first element...
		 */
		bfd_section_alignment(abfd,ld->csect) = get_align_pwr(abfd,rval[0]);

		/* set new common symbol values */
		for (val=0,i=0; i<num_new_commons; i++) {
			asymbol *sp;

			sp = bfd_make_empty_symbol(abfd);

			val=align_power(val,get_align_pwr(abfd,rval[i]));
			/* copy pointer to old name */
			bfd_asymbol_name(sp) = bfd_asymbol_name(rval[i]);
			sp->value=val;
			sp->section=ld->csect;
			sp->flags=rval[i]->flags;
			val+=rval[i]->value;
			rval[i] = sp;
		}
		
		bfd_set_section_size(abfd, ld->csect, csect_size);
	}

	if (errs) {
		/* release resources */
		free(rval);
		/* TODO destroyCexpSymtab(); */
		return 0;
	}
	return rval;
}

static int
flushCache(LinkData ld)
{
int i,j;
	for (i=0; i<NUM_SEGS; i++) {
		for (j=0; j<= ld->segs[i].size; j+=CACHE_LINE_SIZE)
			__asm__ __volatile__(
				"dcbf %0, %1\n"	/* flush out one data cache line */
				"icbi %0, %1\n" /* invalidate cached instructions for this line */
				::"b"(ld->segs[i].chunk),"r"(j));
	}
	/* enforce flush completion and discard preloaded instructions */
	__asm__ __volatile__("sync; isync");
}


int
main(int argc, char **argv)
{
bfd 			*abfd=0;
LinkDataRec		ldr;
int			rval=1,i,j;
TstSym			sm;

	memset(&ldr,0,sizeof(ldr));

	if (argc<2) {
		fprintf(stderr,"Need filename arg\n");
		goto cleanup;
	}
printf("TSILL: bfd_log2(1025)=%i\n",bfd_log2(1025));

	bfd_init();
#ifdef USE_MDBG
	mdbgInit();
#endif
	if ( ! (abfd=bfd_openr(argv[1],0)) ) {
		bfd_perror("Opening object file");
		goto cleanup;
	}
	if (!bfd_check_format(abfd, bfd_object)) {
		bfd_perror("Checking format");
		goto cleanup;
	}

	/* make a dummy section for new common symbols */
	ldr.csect=bfd_make_section(abfd,bfd_get_unique_section_name(abfd,".dummy",0));
	if (!ldr.csect) {
		bfd_perror("Creating dummy section");
		goto cleanup;
	}
	ldr.csect->flags |= SEC_ALLOC;

	if (!(ldr.st=slurp_symtab(abfd,&ldr))) {
		fprintf(stderr,"Error creating symbol table\n");
		goto cleanup;
	}

	ldr.errors=0;
	bfd_map_over_sections(abfd, s_count, &ldr);

	/* allocate segment space */
	for (i=0; i<NUM_SEGS; i++)
		ldr.segs[i].vmacalc=(unsigned long)ldr.segs[i].chunk=xmalloc(ldr.segs[i].size);

	ldr.errors=0;
	bfd_map_over_sections(abfd, s_setvma, &ldr);

	ldr.errors=0;
memset(ldr.segs[0].chunk,0xee,ldr.segs[0].size); /*TSILL*/
	bfd_map_over_sections(abfd, s_reloc, &ldr);

	/* TODO: setCexpSymtabValues() */

	flushCache(&ldr);

	/* TODO: call constructors */

	for (i=0; ldr.st[i]; i++) {
		if (0==strcmp(bfd_asymbol_name(ldr.st[i]),"blah")) {
			printf("FOUND blah; is 0x%08x\n",bfd_asymbol_value(ldr.st[i]));
			((void (*)(int))bfd_asymbol_value(ldr.st[i]))(0xfeedcafe);
		}
	}
	rval=0;

cleanup:
	if (ldr.st) free(ldr.st);
	if (abfd) bfd_close_all_done(abfd);
	for (i=0; i<NUM_SEGS; i++)
		if (ldr.segs[i].chunk) free(ldr.segs[i].chunk);
#ifdef USE_MDBG
	printf("Memory leaks found: %i\n",mdbgPrint(0,0));
#endif
	return rval;
}
