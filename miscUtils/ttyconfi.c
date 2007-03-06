#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>


int
sttyspeed(int speed, char *ttynam)
{
int fd;
int rval = -1;
struct termios ta;

	if ( !ttynam )
		ttynam = "/dev/console";

	switch ( speed ) {
		default:
			fprintf(stderr,"Error: invalid speed %i\n", speed);
			return -1;

		case 0:			speed = B0;     break;
		case 50:		speed = B50;     break;
		case 75:		speed = B75;     break;
		case 110:		speed = B110;    break;
		case 134:		speed = B134;    break;
		case 150:		speed = B150;    break;
		case 200:		speed = B200;    break;
		case 300:		speed = B300;    break;
		case 600:		speed = B600;    break;
		case 1200:		speed = B1200;   break;
		case 1800:		speed = B1800;   break;
		case 2400:		speed = B2400;   break;
		case 4800:		speed = B4800;   break;
		case 9600:		speed = B9600;   break;
		case 19200:		speed = B19200;  break;
		case 38400:		speed = B38400;  break;
		case 57600:		speed = B57600;  break;
		case 115200:	speed = B115200; break;
		case 230400:	speed = B230400; break;
	}

	if ( (fd = open(ttynam, O_RDWR)) < 0 ) {
		fprintf(stderr,"error opening %s: %s\n", ttynam, strerror(errno));
		return -1;
	}

	if ( !isatty(fd) ) {
		fprintf(stderr,"error: %s is not a terminal\n", ttynam);
		goto bail;
	}

	if ( tcgetattr(fd, &ta) ) {
		perror("tcgetattr");
		goto bail;
	}

	if ( cfsetispeed(&ta, speed) || cfsetospeed(&ta, speed) ) {
		perror("setting speed");
		goto bail;
	}

	if ( tcsetattr(fd, TCSANOW, &ta) ) {
		perror("tcsetattr");
		goto bail;
	}

	rval = 0;

bail:
	close(fd);
	return rval;
}
