/* $Id$ */

/* Read a password, encrypt it and compare to the encrypted 
 * password in the TELNETD_PASSWD environment variable.
 * No password is required if TELNETD_PASSWD is unset
 */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2003 */

#include <crypt.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

extern char *__des_crypt_r(char *, char*, char*);

static int check_passwd()
{
char			*pw, *try;
int				rval = -1, tmp, retries;
struct termios	t;
tcflag_t		lflag;
int				restore_flags = 0;
char			buf[30], cryptbuf[21];
char			salt[3];

	if ( !(pw=getenv("TELNETD_PASSWD")) )
		return 0;

	if ( tcgetattr(0, &t) ) {
		perror("check_passwd(): tcgetattr");
		goto done;	
	}
	lflag = t.c_lflag;
	t.c_lflag &= ~ECHO;

	strncpy(salt,pw,2);
	salt[2]=0;

	if ( tcsetattr(0, TCSANOW, &t) ) {
		perror("check_passwd(): tcsetattr");
		goto done;	
	}
	t.c_lflag = lflag;
	restore_flags = 1;

	/* Here we ask for the password... */
	for ( retries = 3; retries > 0; retries-- ) {
		fprintf(stderr,"Password:");
		fflush(stderr);
		fgets(buf,sizeof(buf),stdin);
		tmp = strlen(buf);
		if ( tmp > 0 && '\n' == buf[tmp-1] ) {
			buf[--tmp]=0;
		}
	
		if ( !strcmp(__des_crypt_r(buf, salt, cryptbuf), pw) ) {
			rval = 0;
			break;
		}
		fprintf(stderr,"\nIncorrect Password.\n");
		sleep(2);
	}


done:
	/* what to do if restoring the flags fails?? */
	if (restore_flags)
		tcsetattr(0, TCSANOW, &t);
	
	if (rval) {
		sleep(2);
	}
	return rval;
}

#ifdef DEBUG

int
main(int argc, char **argv)
{
char *str, *enc=0;
int   ch;

while ( (ch=getopt(argc, argv, "g:")) > 0 ) {
	switch (ch) {
		default:
			fprintf(stderr,"Unknown option\n");
		return(1);

		case 'g':
			printf("Generated encrypted password: '%s'\n", (enc=crypt(optarg,"td")));
		break;
			
	}
}
if (argc>optind && !enc) {
	enc=argv[optind];
}
if (enc) {
	str = malloc(strlen(enc) + 30);
	sprintf(str,"TELNETD_PASSWD=%s",enc);
	putenv(str);
}
if (check_passwd()) {
	fprintf(stderr,"check_passwd() failed\n");
}
return 0;
}

#endif
