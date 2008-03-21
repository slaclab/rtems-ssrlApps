#ifdef __rtems__
#include <rtems.h>
#include <rtems/libio_.h>
#else
#define rtems_libio_number_iops 2000
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/stat.h>

#if ! defined(__rtems__) 
#define LOCK() do {} while (0)
#define UNLOCK() do {} while (0)
#else
#define UNLOCK() do { rtems_semaphore_release( rtems_libio_semaphore ); } while (0)
#define LOCK()   do { rtems_semaphore_obtain( rtems_libio_semaphore, RTEMS_WAIT, RTEMS_NO_TIMEOUT ); } while (0)
#endif

int
sockstats(FILE *f)
{
int             i,j,e;

union {
struct sockaddr sa;
struct sockaddr_in sin;
} ss,sp;
socklen_t       l;
char            buf[100];
struct stat     sb;

	if ( ! f )
		f = stdout;
	for ( i=j=0; i< rtems_libio_number_iops; i++ ) {
		l = sizeof(ss);
		LOCK();
		if ( fstat(i, &sb) || ! S_ISSOCK(sb.st_mode) || getsockname(i, &ss.sa, &l) ) {
			UNLOCK();
			continue;
		}
		if ( AF_INET != ss.sin.sin_family ) {
			UNLOCK();
			fprintf(f, "%i: <NOT INET>\n", i);
		} else {
			l = sizeof(sp);
			e = getpeername(i, &sp.sa, &l);
			UNLOCK();

			j++;
			inet_ntop(AF_INET, &ss.sin.sin_addr, buf, sizeof(buf));
			fprintf(f, "%i: %16s:%5u <-> ", i, buf, ntohs(ss.sin.sin_port));


			if ( e ) {
				fprintf(f,"<UNKNOWN>\n");
			} else {
				inet_ntop(AF_INET, &sp.sin.sin_addr, buf, sizeof(buf));
				fprintf(f, "%16s:%5u\n", buf, ntohs(sp.sin.sin_port));
			}
		}
	}
	return j;
}

#ifdef HAVE_CEXP
#include <cexpHelp.h>
CEXP_HELP_TAB_BEGIN(sockstats)
	HELP(
"Dump local and peer (if applicable) address of all AF_INET\n"
"sockets in the system.\n"
"RETURNS: number of live (used) sockets. The max. number of\n"
"         file descriptors can be obtained from the (READ-ONLY)\n"
"         variable 'rtems_libio_number_iops'.\n",
	int, sockstats,  (FILE *f)
	),
CEXP_HELP_TAB_END
#endif
#ifndef __rtems__
int main()
{
int sd;
union {
struct sockaddr_in sin;
struct sockaddr    sa;
} sa;
	sa.sin.sin_family = AF_INET;
	sa.sin.sin_addr.s_addr   = inet_addr("127.0.10.10");
	sa.sin.sin_port   = htons(5432);
	sd = socket(PF_INET, SOCK_STREAM, 0);
	bind(sd, &sa.sa, sizeof(sa));
	sockstats(0);
}
#endif
