#include <bfd.h>
#include <bfdlink.h>

#include <stdio.h>

static char *targ="elf32-i386";
static char *tarch="i386";

int
main(int argc, char **argv)
{
bfd							*obfd,*ibfd;
struct bfd_link_info		info;
const bfd_arch_info_type	*arch;
int							i;

		info.callbacks=0;
		info.relocateable=false;
		info.emitrelocations=false;
		info.task_link=false;
		info.shared=false;
		info.symbolic=false;
		info.static_link=true;
		info.traditional_format=true;
		info.optimize=true;
		info.no_undefined=true;
		info.allow_shlib_undefined=false;
		info.strip=strip_debugger;
		info.discard=discard_none;
		info.keep_memory=false;
		info.input_bfds=0;
		info.create_object_symbols_section=0;
		info.hash=0;
		info.keep_hash=0;
		info.notice_all=false;
		info.notice_hash=0;
		info.wrap_hash=0;
		info.base_file=0;
		info.mpc860c0=0;
		info.init_function=0; /* char* */
		info.fini_function=0; /* char* */
		info.new_dtags=false;
		info.flags=0;
		info.flags_1=0;

		bfd_init();

		arch = bfd_scan_arch(tarch);
#if 0
		obfd=bfd_openw("foo",0);
		bfd_set_format(obfd,bfd_object);
		printf("0x%08x\n",arch);
		bfd_set_arch_mach(obfd,arch->arch,arch->mach);
		printf("arch: %s\n",bfd_printable_name(obfd));
#else
		if (!(obfd=bfd_openw("foo",0)) ||
			! bfd_set_format(obfd,bfd_object) ||
			! bfd_set_arch_mach(obfd,arch->arch,arch->mach)
		   ) 
			fprintf(stderr,"Unable to create output BFD\n");
#endif
		info.hash=bfd_link_hash_table_create(obfd);
		bfd_set_gp_size(obfd,8);

		for (i=0; i<argc; i++) {
		if (!(ibfd=bfd_openr(argv[i],0)) ||
			! bfd_check_format(ibfd,bfd_object)) {
			fprintf(stderr,"Unable to open input file\n");
		}
		if (ibfd && !bfd_link_add_symbols(ibfd,&info))
			fprintf(stderr,"Unable to add symbols\n");
		/*
		if (ibfd)
			bfd_close_all_done(ibfd);
				*/
		}
		bfd_final_link(obfd,&info);



		if (obfd)
			bfd_close_all_done(obfd);

		return 0;
}
