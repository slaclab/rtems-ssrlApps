#include <crypt.h>
#include <stdio.h>
#include <unistd.h>

static void
usage(char *nm)
{
	fprintf(stderr,"Usage: %s [-h] [-s salt] cleartext_password\n", nm);
}

int
main(int argc, char **argv)
{
int ch;
char *salt="td";
	while ( (ch=getopt(argc, argv, "hs:")) >=0 ) {
		switch (ch) {
			default:  fprintf(stderr,"Unknown Option '%c'\n",ch);
			case 'h': usage(argv[0]);
			return 0;
			case 's': salt=optarg;
			break;
		}
	}
	if ( optind >= argc ) {
		usage(argv[0]);
		return 1;
	}
	printf("#define TELNETD_DEFAULT_PASSWD \"%s\"\n",crypt(argv[optind],salt));
}
