#include <bfd.h>
#include <libiberty.h>
#include <stdio.h>
#include "tab.h"

#ifdef USE_MDBG
#include <mdbg.h>
#endif

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
		bfd_get_section_contents(abfd,sect,(PTR)bfd_get_section_vma(abfd,sect),0,bfd_section_size(abfd,sect));
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
LinkDataRec		ldr;
int				rval=1,i;
TstSym			sm;

	if (argc<2) {
		fprintf(stderr,"Need filename arg\n");
		goto cleanup;
	}
	memset(&ldr,0,sizeof(ldr));

	bfd_init();
#ifdef USE_MDBG
	mdbgInit();
#endif
	abfd=bfd_openr(argv[1],0);
	if (!bfd_check_format(abfd, bfd_object)) {
		fprintf(stderr,"Invalid format\n");
		goto cleanup;
	}
	/* set aligment for our segments; we just have to make sure
	 * the initial aligment is worse than what 'malloc()' gives us.
	 */
	for (i=0; i<NUM_SEGS; i++)
		ldr.segs[i].size=1; /* first section in this segment will align this */

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
	bfd_map_over_sections(abfd, s_setvma, &ldr);
	if (!(ldr.st=slurp_symtab(abfd,&ldr))) {
		fprintf(stderr,"Error reading symbol table\n");
		goto cleanup;
	}
	bfd_map_over_sections(abfd, s_reloc, &ldr);
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
