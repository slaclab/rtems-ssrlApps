#include <bfd.h>
#include <libiberty.h>
#include <stdio.h>
#include "tab.h"

#ifdef USE_MDBG
#include <mdbg.h>
#endif

#define CACHE_LINE_SIZE 32

/* an output segment description */
typedef struct SegmentRec_ {
	PTR				chunk;		/* pointer to memory */
	unsigned long	vmacalc;	/* working counter */
	unsigned long	size;		/* initial 'size' of the segment is its aligment */
	unsigned		attributes; /* such as 'read-only' etc.; currently unused */
} SegmentRec, *Segment;

#define NUM_SEGS 1

typedef struct LinkDataRec_ {
	SegmentRec		segs[NUM_SEGS];
	asymbol			**st;
	asection		*tsill;
	int				errors;
} LinkDataRec, *LinkData;

/* how to decide where a particular section should go */
static Segment
segOf(LinkData ld, asection *sect)
{
	/* multiple sections not supported (yet) */
	return &ld->segs[0];
}

static void
s_count(bfd *abfd, asection *sect, PTR arg)
{
Segment		seg=segOf((LinkData)arg, sect);
	printf("Section %s, flags 0x%08x\n",
			sect->name, sect->flags);
	printf("size: %i, alignment %i\n",
			bfd_section_size(abfd,sect),
			(1<<bfd_section_alignment(abfd,sect)));
	if (SEC_ALLOC & sect->flags) {
		seg->size=align_power(seg->size, bfd_get_section_alignment(abfd,sect));
		seg->size+=bfd_section_size(abfd,sect);
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
asymbol **rval=0,*sp;
int		i,errs=0;
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
	/* resolve undefined and common symbols;
	 * find name clashes
	 */
	for (i=0; sp=rval[i]; i++) {
		asection *sect=bfd_get_section(sp);

		/* we only care about global symbols
		 * (NOTE: undefined symbols are neither local
		 *        nor global)
		 */
		if ( (BSF_LOCAL & sp->flags) )
			continue;

		if (bfd_is_und_section(sect)) {

			TstSym ts=tstSymLookup(sp->name);

			if (ts) {
				sp=rval[i]=bfd_make_empty_symbol(abfd);
				sp->name=ts->name;
				sp->value=ts->value;
				sp->section=bfd_abs_section_ptr;
				sp->flags=BSF_GLOBAL;
			} else {
				fprintf(stderr,"Unresolved symbol: %s\n",sp->name);
				errs++;
			}
		}
		else if (bfd_is_com_section(sp)) {
			;
		}
	}

	if (errs) {
		/* release resources */
		free(rval);
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

	bfd_init();
#ifdef USE_MDBG
	mdbgInit();
#endif
	if ( ! (abfd=bfd_openr(argv[1],0)) ) {
		fprintf(stderr,"Unable to open '%s'\n",argv[1]);
		goto cleanup;
	}
	if (!bfd_check_format(abfd, bfd_object)) {
		fprintf(stderr,"Invalid format\n");
		goto cleanup;
	}

	if (!(ldr.st=slurp_symtab(abfd,&ldr))) {
		fprintf(stderr,"Error reading symbol table\n");
		goto cleanup;
	}

	/* set aligment for our segments; we just have to make sure
	 * the initial aligment is worse than what 'malloc()' gives us.
	 */
	for (i=0; i<NUM_SEGS; i++)
		ldr.segs[i].size=1; /* first section in this segment will align this */

	ldr.errors=0;
	bfd_map_over_sections(abfd, s_count, &ldr);
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


	/* allocate segment space */
	for (i=0; i<NUM_SEGS; i++)
		ldr.segs[i].vmacalc=(unsigned long)ldr.segs[i].chunk=xmalloc(ldr.segs[i].size);

	ldr.errors=0;
	bfd_map_over_sections(abfd, s_setvma, &ldr);

	ldr.errors=0;
memset(ldr.segs[0].chunk,0xee,ldr.segs[0].size); /*TSILL*/
	bfd_map_over_sections(abfd, s_reloc, &ldr);

	flushCache(&ldr);

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
