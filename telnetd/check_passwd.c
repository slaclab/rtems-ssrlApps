/* $Id$ */

/* Read a password, encrypt it and compare to the encrypted 
 * password in the TELNETD_PASSWD environment variable.
 * No password is required if TELNETD_PASSWD is unset
 */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2003 */

#if !defined(INSIDE_TELNETD)
#include <crypt.h>
#endif
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/* rtems has global filedescriptors but per-thread stdio streams... */
#define STDI_FD fileno(stdin)
#define MAXPASSRETRY	3

extern char *__des_crypt_r(char *, char*, char*);

#if !defined(INSIDE_TELNETD)
#define sockpeername(s,b,sz) (-1)
#endif

#if defined(INSIDE_TELNETD)
static
#endif
int check_passwd(char *peername)
{
char			*pw;
int				rval = -1, tmp, retries;
struct termios	t,told;
int				restore_flags = 0;
char			buf[30], cryptbuf[21];
char			salt[3];

	if ( !(pw=getenv("TELNETD_PASSWD")) || 0 == strlen(pw) )
		return 0;

	if ( tcgetattr(STDI_FD, &t) ) {
		perror("check_passwd(): tcgetattr");
		goto done;	
	}
	told = t;
	t.c_lflag &= ~ECHO;
	t.c_lflag &= ~ICANON;
	t.c_cc[VTIME] = 255;
	t.c_cc[VMIN]  = 0;

	strncpy(salt,pw,2);
	salt[2]=0;

	if ( tcsetattr(STDI_FD, TCSADRAIN, &t) ) {
		perror("check_passwd(): tcsetattr");
		goto done;	
	}
	restore_flags = 1;

	/* Here we ask for the password... */
	for ( retries = MAXPASSRETRY; retries > 0; retries-- ) {
		fflush(stdin);
		fprintf(stderr,"Password:");
		fflush(stderr);
		if ( 0 == fgets(buf,sizeof(buf),stdin) ) {
			/* Here comes an ugly hack:
			 * The termios driver's 'read()' handler
			 * returns 0 to the c library's fgets if
			 * it times out. 'fgets' interprets this
			 * (correctly) as EOF, a condition we want
			 * to undo since it's not really true since
			 * we really have a read error (termios bug??)
			 *
			 * As a workaround we push something back and
			 * read it again. This should simply reset the
			 * EOF condition.
			 */
			if (ungetc('?',stdin) >= 0)
				fgetc(stdin);
			goto done;
		}
		tmp = strlen(buf);
		while ( tmp > 0 && ('\n' == buf[tmp-1] || '\r' == buf[tmp-1]) ) {
			buf[--tmp]=0;
		}
		if ( !strcmp(__des_crypt_r(buf, salt, cryptbuf), pw) ) {
			rval = 0;
			break;
		}
		fprintf(stderr,"\nIncorrect Password.\n");
		sleep(2);
	}

	if ( 0 == retries ) {
		syslog( LOG_AUTHPRIV | LOG_WARNING,
			"telnetd: %i wrong passwords entered from %s",
			MAXPASSRETRY,
			peername ? peername : "<UNKNOWN>");
	}

done:
	/* what to do if restoring the flags fails?? */
	if (restore_flags)
		tcsetattr(STDI_FD, TCSADRAIN, &told);
	
	if (rval) {
		sleep(2);
	}
	return rval;
}

#if !defined(INSIDE_TELNETD)
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
if (check_passwd(-1)) {
	fprintf(stderr,"check_passwd() failed\n");
}
return 0;
}

#endif
