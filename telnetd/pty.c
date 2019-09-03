/*
 * /dev/ptyXX  (A first version for pseudo-terminals)
 *
 *  Author: Fernando RUIZ CASAS (fernando.ruiz@ctv.es)
 *  May 2001
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Till Straumann <strauman@slac.stanford.edu>
 *
 *   - converted into a loadable module
 *   - NAWS support / ioctls for querying/setting the window
 *     size added.
 *   - don't delete the running task when the connection
 *     is closed. Rather let 'read()' return a 0 count so
 *     they may cleanup. Some magic hack works around termios
 *     limitation.
 * 
 */

/*
                      LICENSE INFORMATION

RTEMS is free software; you can redistribute it and/or modify it under
terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.  RTEMS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details. You should have received
a copy of the GNU General Public License along with RTEMS; see
file COPYING. If not, write to the Free Software Foundation, 675
Mass Ave, Cambridge, MA 02139, USA.

As a special exception, including RTEMS header files in a file,
instantiating RTEMS generics or templates, or linking other files
with RTEMS objects to produce an executable application, does not
by itself cause the resulting executable application to be covered
by the GNU General Public License. This exception does not
however invalidate any other reasons why the executable file might be
covered by the GNU Public License.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define DEBUG_WH		(1<<0)
#define DEBUG_DETAIL	(1<<1)

/* #define DEBUG DEBUG_WH */

#ifdef __cplusplus
extern "C" {
#endif
/*-----------------------------------------*/
#include <termios.h>
#include <rtems/termiostypes.h>
#include <sys/ttycom.h>
#include <rtems.h>
#include <rtems/libio.h>
#include <bsp.h>
#include <rtems/bspIo.h>
#if 0
#include <rtems/pty.h>
#else
#define MAX_PTYS 8
#endif
#include <errno.h>
#include <sys/socket.h>
#ifdef __cplusplus
};
#endif
/*-----------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/*-----------------------------------------*/
#define IAC_ESC    255
#define IAC_DONT   254
#define IAC_DO     253
#define IAC_WONT   252
#define IAC_WILL   251
#define IAC_SB     250
#define IAC_GA     249
#define IAC_EL     248
#define IAC_EC     247
#define IAC_AYT    246
#define IAC_AO     245
#define IAC_IP     244
#define IAC_BRK    243
#define IAC_DMARK  242
#define IAC_NOP    241
#define IAC_SE     240
#define IAC_EOR    239

#define SB_MAX		16

struct pty_tt;
typedef struct pty_tt pty_t;

struct pty_tt {
 char                     *devname;
 struct rtems_termios_tty *ttyp;
 tcflag_t                  c_cflag;
 int                       opened;
 int                       socket;
 int                       last_cr;
 unsigned                  iac_mode;   
 unsigned char             sb_buf[SB_MAX];	
 int                       sb_ind;
 int                       width;
 int                       height;
};


#ifdef __cplusplus

extern "C" {
int		printk(char*,...);

#endif

#if     MAX_PTYS > 5
#undef  MAX_PTYS
#define MAX_PTYS 5
#endif


static int   telnet_pty_inited=FALSE;
static pty_t telnet_ptys[MAX_PTYS];

static rtems_device_major_number pty_major;


/* This procedure returns the devname for a pty slot free.
 * If not slot availiable (field socket>=0) 
 *  then the socket argument is closed
 */

char *  telnet_get_pty(int socket) {
	int ndx;
	if (telnet_pty_inited) {
		for (ndx=0;ndx<MAX_PTYS;ndx++) {
			if (telnet_ptys[ndx].socket<0) {
				struct timeval t;
				/* set a long polling interval to save CPU time */
				t.tv_sec=2;
				t.tv_usec=00000;
				setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
				telnet_ptys[ndx].socket=socket;
				return telnet_ptys[ndx].devname;
			};
		};
	}
	close(socket);
	return NULL;
}


/*-----------------------------------------------------------*/
/*
 * The NVT terminal is negociated in PollRead and PollWrite
 * with every BYTE sendded or received. 
 * A litle status machine in the pty_read_byte(int minor) 
 * 
 */
static const char IAC_AYT_RSP[]="\r\nAYT? Yes, RTEMS-SHELL is here\r\n";
static const char IAC_BRK_RSP[]="<*Break*>";
static const char IAC_IP_RSP []="<*Interrupt*>";


static
int send_iac(int minor,unsigned char mode,unsigned char option) {
	unsigned char buf[3];
	buf[0]=IAC_ESC;
	buf[1]=mode;
	buf[2]=option;
	return write(telnet_ptys[minor].socket,buf,sizeof(buf));
}

static int
handleSB(pty_t *pty)
{
	switch (pty->sb_buf[0]) {
		case 31:	/* NAWS */
			pty->width  = (pty->sb_buf[1]<<8) + pty->sb_buf[2];
			pty->height = (pty->sb_buf[3]<<8) + pty->sb_buf[4];
#if DEBUG & DEBUG_WH
			fprintf(stderr,
					"Setting width/height to %ix%i\n",
					pty->width,
					pty->height);
#endif
			break;
		default:
			break;
	}
	return 0;
}

static int read_pty(int minor) { /* Characters written to the client side*/
	 unsigned char	value;
	 unsigned int	omod;
	 int			count;
	 int			result;
	 pty_t			*pty=telnet_ptys+minor;

	 count=read(pty->socket,&value,sizeof(value));
	 if (count<0)
		return -1;

	 if (count<1) {
			/* Unfortunately, there is no way of passing an EOF
			 * condition through the termios driver. Hence, we
			 * resort to an ugly hack. Setting cindex>ccount
			 * causes the termios driver to return a read count
			 * of '0' which is what we want here. We leave
			 * 'errno' untouched.
			 */
			pty->ttyp->cindex=pty->ttyp->ccount+1;
			return pty->ttyp->termios.c_cc[VEOF];
	 };

	 omod=pty->iac_mode;
	 pty->iac_mode=0;
	 switch(omod & 0xff) {
			 case IAC_ESC:
					 switch(value) {
							 case IAC_ESC :
									 /* in case this is an ESC ESC sequence in SB mode */
									 pty->iac_mode = omod>>8;
									 return IAC_ESC;
							 case IAC_DONT:
							 case IAC_DO  :
							 case IAC_WONT:
							 case IAC_WILL:
									 pty->iac_mode=value;
									 return -1;
							 case IAC_SB  :
#if DEBUG & DEBUG_DETAIL
									 printk("SB\n");
#endif
									 pty->iac_mode=value;
									 pty->sb_ind=0;
									 return -100;
							 case IAC_GA  :
									 return -1;
							 case IAC_EL  :
									 return 0x03; /* Ctrl-C*/
							 case IAC_EC  :
									 return '\b';
							 case IAC_AYT :
									 write(pty->socket,IAC_AYT_RSP,strlen(IAC_AYT_RSP));
									 return -1;
							 case IAC_AO  :
									 return -1;
							 case IAC_IP  :
									 write(pty->socket,IAC_IP_RSP,strlen(IAC_IP_RSP));
									 return -1;
							 case IAC_BRK :
									 write(pty->socket,IAC_BRK_RSP,strlen(IAC_BRK_RSP));
									 return -1;
							 case IAC_DMARK:
									 return -2;
							 case IAC_NOP :
									 return -1;
							 case IAC_SE  :
#if DEBUG & DEBUG_DETAIL
									{
									int i;
									printk("SE");
									for (i=0; i<pty->sb_ind; i++)
										printk(" %02x",pty->sb_buf[i]);
									printk("\n");
									}
#endif
									handleSB(pty);
							 return -101;
							 case IAC_EOR :
									 return -102;
							 default      :
									 return -1;
					 };
					 break;

			 case IAC_SB:
					 pty->iac_mode=omod;
					 if (IAC_ESC==value) {
						pty->iac_mode=(omod<<8)|value;
					 } else {
						if (pty->sb_ind < SB_MAX)
							pty->sb_buf[pty->sb_ind++]=value;
					 }
					 return -1;

			 case IAC_WILL:
					 if (value==34){
							send_iac(minor,IAC_DONT,   34);	/*LINEMODE*/ 
							send_iac(minor,IAC_DO  ,    1);	/*ECHO    */
					 } else if (value==31) {
							send_iac(minor,IAC_DO  ,   31);	/*NAWS	  */
#if DEBUG & DEBUG_DETAIL
							printk("replied DO NAWS\n");
#endif
					 } else {
							send_iac(minor,IAC_DONT,value);
					 }
					 return -1;
			 case IAC_DONT:
					 return -1;
			 case IAC_DO  :
					 if (value==3) {
							send_iac(minor,IAC_WILL,    3);	/* GO AHEAD*/
					 } else	if (value==1) {                         
							/* ECHO */
					 } else {
							send_iac(minor,IAC_WONT,value);
					 };
					 return -1;
			 case IAC_WONT:
					 if (value==1) {send_iac(minor,IAC_WILL,    1);} else /* ECHO */
					 {send_iac(minor,IAC_WONT,value);};
					 return -1;
			 default:
					 if (value==IAC_ESC) {
							pty->iac_mode=value;
							return -1;
					 } else {
							result=value;  
							if ( 0
#if 0							 /* pass CRLF through - they should use termios to handle it */
								 ||	((value=='\n') && (pty->last_cr))
#endif
								/* but map telnet CRNUL to CR down here */
								 || ((value==0) && pty->last_cr)
								) result=-1;
							 pty->last_cr=(value=='\r');
							 return result;
					 };
	 };
	/* should never get here but keep compiler happy */
	return -1;
}

/*-----------------------------------------------------------*/
static int ptySetAttributes(int minor,const struct termios *t);
static int ptyPollInitialize(int major,int minor,void * arg) ;
static int ptyShutdown(int major,int minor,void * arg) ;
static int ptyPollWrite(int minor, const char * buf,int len) ;
static int ptyPollRead(int minor) ;
static const rtems_termios_callbacks * pty_get_termios_handlers(int polled) ;
/*-----------------------------------------------------------*/
/* Set the 'Hardware'                                        */ 
/*-----------------------------------------------------------*/
static int
ptySetAttributes(int minor,const struct termios *t) {
	if (minor<MAX_PTYS) {	
	 telnet_ptys[minor].c_cflag=t->c_cflag;	
	} else {
	 return -1;
	};
	return 0;
}
/*-----------------------------------------------------------*/
static int 
ptyPollInitialize(int major,int minor,void * arg) {
	rtems_libio_open_close_args_t * args = (rtems_libio_open_close_args_t*)arg;
	struct termios t;
        if (minor<MAX_PTYS) {	
         if (telnet_ptys[minor].socket<0) return -1;		
	 telnet_ptys[minor].opened=TRUE;
	 telnet_ptys[minor].ttyp= (struct rtems_termios_tty *) args->iop->data1;
	 telnet_ptys[minor].iac_mode=0;
	 telnet_ptys[minor].sb_ind=0;
	 telnet_ptys[minor].width=0;
	 telnet_ptys[minor].height=0;
	 t.c_cflag=B9600|CS8;/* termios default */
	 return ptySetAttributes(minor,&t);
	} else {
	 return -1;
	};
}
/*-----------------------------------------------------------*/
static int 
ptyShutdown(int major,int minor,void * arg) {
        if (minor<MAX_PTYS) {	
	 telnet_ptys[minor].opened=FALSE;
         if (telnet_ptys[minor].socket>=0) close(telnet_ptys[minor].socket);
	 telnet_ptys[minor].socket=-1;
	 chown(telnet_ptys[minor].devname,2,0);
	} else {
	 return -1;
	};
	return 0;
}
/*-----------------------------------------------------------*/
/* Write Characters into pty device                          */ 
/*-----------------------------------------------------------*/
static int
ptyPollWrite(int minor, const char * buf,int len) {
	int count;
        if (minor<MAX_PTYS) {	
         if (telnet_ptys[minor].socket<0) return -1;		
	 count=write(telnet_ptys[minor].socket,buf,len);
	} else {
	 count=-1;
	};
	return count;
}
/*-----------------------------------------------------------*/
static int
ptyPollRead(int minor) {
	int result;
        if (minor<MAX_PTYS) {	
         if (telnet_ptys[minor].socket<0) return -1;		
	 result=read_pty(minor);
	 return result;
	};
	return -1;
}
/*-----------------------------------------------------------*/
/*  pty_initialize
 *
 *  This routine initializes the pty IO driver.
 *
 *  Input parameters: NONE
 *
 *  Output parameters:  NONE
 *
 *  Return values:
 */
/*-----------------------------------------------------------*/
static
rtems_device_driver my_pty_initialize(
  rtems_device_major_number  major,
  rtems_device_minor_number  minor,
  void                      *arg
)
{
int ndx;	
rtems_status_code status ;

	/* 
	 * Set up ptys
	 */

	for (ndx=0;ndx<MAX_PTYS;ndx++) {
		telnet_ptys[ndx].devname=(char*)malloc(strlen("/dev/ptyXX")+1);
		sprintf(telnet_ptys[ndx].devname,"/dev/pty%X",ndx);
		telnet_ptys[ndx].ttyp=NULL;
		telnet_ptys[ndx].c_cflag=CS8|B9600;
		telnet_ptys[ndx].socket=-1;
		telnet_ptys[ndx].opened=FALSE;
		telnet_ptys[ndx].sb_ind=0;
		telnet_ptys[ndx].width=0;
		telnet_ptys[ndx].height=0;

	};

	/*
	 * Register the devices
	 */
	for (ndx=0;ndx<MAX_PTYS;ndx++) {
		status = rtems_io_register_name(telnet_ptys[ndx].devname, major, ndx);
		if (status != RTEMS_SUCCESSFUL)
				rtems_fatal_error_occurred(status);
		chmod(telnet_ptys[ndx].devname,0660);
		chown(telnet_ptys[ndx].devname,2,0); /* tty,root*/
	};
	printk("Device: /dev/pty%X../dev/pty%X (%d)pseudo-terminals registered.\n",0,MAX_PTYS-1,MAX_PTYS);

	return RTEMS_SUCCESSFUL;
}

static int pty_do_finalize()
{
int ndx;
rtems_status_code status;

		if ( !telnet_pty_inited )
			return 0;

		for (ndx=0;ndx<MAX_PTYS;ndx++) {
			if (telnet_ptys[ndx].opened) {
					fprintf(stderr,"There are still opened PTY devices, unable to proceed\n");
					return -1;
			}
		}
		if (RTEMS_SUCCESSFUL != rtems_io_unregister_driver(pty_major)) {
				fprintf(stderr,"Unable to remove this driver\n");
				return -1;
		}
		for (ndx=0;ndx<MAX_PTYS;ndx++) {
				/* rtems_io_register_name() actually creates a node in the filesystem
				 * (mknod())
				 */
				status = (rtems_status_code)unlink(telnet_ptys[ndx].devname);
				if (status != RTEMS_SUCCESSFUL)
					perror("removing pty device node from file system");
				else
					free(telnet_ptys[ndx].devname);
		};
		fprintf(stderr,"PTY driver unloaded successfully\n");
		telnet_pty_inited=FALSE;
		return 0;
}

/*
 *  Open entry point
 */

static
rtems_device_driver my_pty_open(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void                    * arg
)
{
  rtems_status_code sc ;
  sc = rtems_termios_open(major,minor,arg,pty_get_termios_handlers(FALSE));
  return sc;
}
 
/*
 *  Close entry point
 */

static
rtems_device_driver my_pty_close(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void                    * arg
)
{
  return rtems_termios_close(arg);
}

/*
 * read bytes from the pty
 */

static
rtems_device_driver my_pty_read(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void                    * arg
)
{
  return rtems_termios_read(arg);
}

/*
 * write bytes to the pty
 */

static
rtems_device_driver my_pty_write(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void                    * arg
)
{
  return rtems_termios_write(arg);
}

/*
 *  IO Control entry point
 */

static
rtems_device_driver my_pty_control(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void                    * arg
)
{
rtems_libio_ioctl_args_t *args = (rtems_libio_ioctl_args_t*)arg;
struct winsize			 *wp = (struct winsize*)args->buffer;
pty_t					 *p=&telnet_ptys[minor];

	switch (args->command) {

		case TIOCGWINSZ:

			wp->ws_row = p->height;
			wp->ws_col = p->width;
			args->ioctl_return=0;
#if DEBUG & DEBUG_WH
			fprintf(stderr,
					"ioctl(TIOCGWINSZ), returning %ix%i\n",
					wp->ws_col,
					wp->ws_row);
#endif

			return RTEMS_SUCCESSFUL;

		case TIOCSWINSZ:
#if DEBUG & DEBUG_WH
			fprintf(stderr,
					"ioctl(TIOCGWINSZ), setting %ix%i\n",
					wp->ws_col,
					wp->ws_row);
#endif

			p->height = wp->ws_row;
			p->width  = wp->ws_col;
			args->ioctl_return=0;

			return RTEMS_SUCCESSFUL;

		default:

	   	break;
  }

  return rtems_termios_ioctl(arg);
}

static rtems_driver_address_table drvPty = {
		my_pty_initialize,
		my_pty_open,
		my_pty_close,
		my_pty_read,
		my_pty_write,
		my_pty_control
};

/*-----------------------------------------------------------*/
static const rtems_termios_callbacks pty_poll_callbacks = {
	ptyPollInitialize,	/* FirstOpen*/
	ptyShutdown,		/* LastClose*/
	ptyPollRead,		/* PollRead  */
	ptyPollWrite,		/* Write */
	ptySetAttributes,	/* setAttributes */
	NULL,			/* stopRemoteTX */
	NULL,			/* StartRemoteTX */
	0 			/* outputUsesInterrupts */
};
/*-----------------------------------------------------------*/
static const rtems_termios_callbacks * pty_get_termios_handlers(int polled) {
	return &pty_poll_callbacks;
}
/*-----------------------------------------------------------*/

static int pty_do_initialize()
{
	if ( !telnet_pty_inited ) {
		if (RTEMS_SUCCESSFUL==rtems_io_register_driver(0, &drvPty, &pty_major))
			telnet_pty_inited=TRUE;
		else
			fprintf(stderr,"WARNING: registering the PTY driver FAILED\n");
	}
	return telnet_pty_inited;
}

#ifdef __cplusplus

class TelnetPtyIni {
public:
	TelnetPtyIni() { if (!nest++) {
						pty_do_initialize();
				}
			 };
	~TelnetPtyIni(){ if (!--nest) {
						pty_do_finalize();
				}
			 };
private:
static int nest;
};

static TelnetPtyIni onlyInst;
int TelnetPtyIni::nest=0;

int telnet_pty_initialize()
{
	return telnet_pty_inited;
}

int telnet_pty_finalize()
{
	return telnet_pty_inited;
}
};
#else
int telnet_pty_initialize()
{
	return pty_do_initialize();
}

int telnet_pty_finalize()
{
	return pty_do_finalize();
}
#endif
