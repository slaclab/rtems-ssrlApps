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
 *  $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include <rtems/pty.h>
#include <errno.h>
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


int ptys_initted=FALSE;
pty_t ptys[MAX_PTYS];

/* This procedure returns the devname for a pty slot free.
 * If not slot availiable (field socket>=0) 
 *  then the socket argument is closed
 */

char *  get_pty(int socket) {
	int ndx;
	if (!ptys_initted) return NULL;
	for (ndx=0;ndx<MAX_PTYS;ndx++) {
		if (ptys[ndx].socket<0) {
			ptys[ndx].socket=socket;
			return ptys[ndx].devname;
		};
	};
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
const char IAC_AYT_RSP[]="\r\nAYT? Yes, RTEMS-SHELL is here\r\n";
const char IAC_BRK_RSP[]="<*Break*>";
const char IAC_IP_RSP []="<*Interrupt*>";


static
int send_iac(int minor,unsigned char mode,unsigned char option) {
	unsigned char buf[3];
	buf[0]=IAC_ESC;
	buf[1]=mode;
	buf[2]=option;
	return write(ptys[minor].socket,buf,sizeof(buf));
}

static int
handleSB(pty_t *pty)
{
	switch (pty->sb_buf[0]) {
		case 31:	/* NAWS */
			pty->width  = (pty->sb_buf[1]<<8) + pty->sb_buf[2];
			pty->height = (pty->sb_buf[3]<<8) + pty->sb_buf[4];
			break;
		default:
			break;
	}
	return 0;
}

int read_pty(int minor) { /* Characters writed in the client side*/
	 unsigned char	value;
	 unsigned int	omod;
	 int			count;
	 int			result;
	 pty_t			*pty=ptys+minor;

	 count=read(pty->socket,&value,sizeof(value));
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
#ifdef DEBUG
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
#ifdef DEBUG
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
#ifdef DEBUG
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
							 if ((value=='\n') && (pty->last_cr)) result=-1;
							 pty->last_cr=(value=='\r');
							 return result;
					 };
	 };
	
}

/*-----------------------------------------------------------*/
static int ptySetAttributes(int minor,const struct termios *t);
static int ptyPollInitialize(int major,int minor,void * arg) ;
static int ptyShutdown(int major,int minor,void * arg) ;
static int ptyPollWrite(int minor, const char * buf,int len) ;
static int ptyPollRead(int minor) ;
const rtems_termios_callbacks * pty_get_termios_handlers(int polled) ;
/*-----------------------------------------------------------*/
/* Set the 'Hardware'                                        */ 
/*-----------------------------------------------------------*/
static int
ptySetAttributes(int minor,const struct termios *t) {
	if (minor<MAX_PTYS) {	
	 ptys[minor].c_cflag=t->c_cflag;	
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
         if (ptys[minor].socket<0) return -1;		
	 ptys[minor].opened=TRUE;
	 ptys[minor].ttyp= (struct rtems_termios_tty *) args->iop->data1;
	 ptys[minor].iac_mode=0;
	 ptys[minor].sb_ind=0;
	 ptys[minor].width=0;
	 ptys[minor].height=0;
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
	 ptys[minor].opened=FALSE;
         if (ptys[minor].socket>=0) close(ptys[minor].socket);
	 ptys[minor].socket=-1;
	 chown(ptys[minor].devname,2,0);
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
         if (ptys[minor].socket<0) return -1;		
	 count=write(ptys[minor].socket,buf,len);
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
         if (ptys[minor].socket<0) return -1;		
	 result=read_pty(minor);
	 return result;
	};
	return -1;
}
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
const rtems_termios_callbacks * pty_get_termios_handlers(int polled) {
	return &pty_poll_callbacks;
}
/*-----------------------------------------------------------*/
void init_ptys(void) {
	int ndx;
	for (ndx=0;ndx<MAX_PTYS;ndx++) {
		ptys[ndx].devname=(char*)malloc(strlen("/dev/ptyXX")+1);
		sprintf(ptys[ndx].devname,"/dev/pty%X",ndx);
		ptys[ndx].ttyp=NULL;
		ptys[ndx].c_cflag=CS8|B9600;
		ptys[ndx].socket=-1;
		ptys[ndx].opened=FALSE;
		ptys[ndx].sb_ind=0;
		ptys[ndx].width=0;
		ptys[ndx].height=0;

	};
	ptys_initted=TRUE;
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
rtems_device_driver pty_initialize(
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

		init_ptys();

		/*
		 * Register the devices
		 */
		for (ndx=0;ndx<MAX_PTYS;ndx++) {
				status = rtems_io_register_name(ptys[ndx].devname, major, ndx);
				if (status != RTEMS_SUCCESSFUL)
						rtems_fatal_error_occurred(status);
				chmod(ptys[ndx].devname,0660);
				chown(ptys[ndx].devname,2,0); /* tty,root*/
		};
		printk("Device: /dev/pty%X../dev/pty%X (%d)pseudo-terminals registered.\n",0,MAX_PTYS-1,MAX_PTYS);

		return RTEMS_SUCCESSFUL;
}

int pty_finalize(
	rtems_device_major_number major
)
{
int ndx;
rtems_status_code status;
		for (ndx=0;ndx<MAX_PTYS;ndx++) {
			if (ptys[ndx].opened) {
					fprintf(stderr,"There are still opened PTY devices, unable to proceed\n");
					return -1;
			}
		}
		if (RTEMS_SUCCESSFUL != rtems_io_unregister_driver(major)) {
				fprintf(stderr,"Unable to remove this driver\n");
				return -1;
		}
		for (ndx=0;ndx<MAX_PTYS;ndx++) {
				/* rtems_io_register_name() actually creates a node in the filesystem
				 * (mknod())
				 */
				status = (rtems_status_code)unlink(ptys[ndx].devname);
				if (status != RTEMS_SUCCESSFUL)
					perror("removing pty device node from file system");
				else
					free(ptys[ndx].devname);
		};
		fprintf(stderr,"PTY driver unloaded successfully\n");
		return 0;
}

/*
 *  Open entry point
 */

static
rtems_device_driver pty_open(
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
rtems_device_driver pty_close(
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
rtems_device_driver pty_read(
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
rtems_device_driver pty_write(
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
rtems_device_driver pty_control(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void                    * arg
)
{
rtems_libio_ioctl_args_t *args = (rtems_libio_ioctl_args_t*)arg;
struct winsize			 *wp = (struct winsize*)args->buffer;
pty_t					 *p=&ptys[minor];

	switch (args->command) {

		case TIOCGWINSZ:

			wp->ws_row = p->height;
			wp->ws_col = p->width;
			args->ioctl_return=0;

			return RTEMS_SUCCESSFUL;

		case TIOCSWINSZ:

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
		pty_initialize,
		pty_open,
		pty_close,
		pty_read,
		pty_write,
		pty_control
};

extern char * (*do_get_pty)(int);

#ifdef __cplusplus

class PtyIni {
public:
	PtyIni() { if (!nest++) {
					if (RTEMS_SUCCESSFUL==rtems_io_register_driver(0, &drvPty, &major))
							do_get_pty=get_pty;
				}
			 };
	~PtyIni(){ if (!--nest) {
					if (0==pty_finalize(major))
						do_get_pty=0;
				}
			 };
private:
	rtems_device_major_number major;
static int nest;
};

static PtyIni onlyInst;
int PtyIni::nest=0;

};
#endif

static rtems_device_major_number pty_major;

void
_cexpModuleInitialize(void* unused)
{
	if (RTEMS_SUCCESSFUL==rtems_io_register_driver(0, &drvPty, &pty_major))
		do_get_pty=get_pty;
	else
		fprintf(stderr,"WARNING: registering the PTY driver FAILED\n");
}

int
_cexpModuleFinalize(void *unused)
{
int rval=0;

	if (do_get_pty)
		rval=pty_finalize(pty_major);
	if (0==rval)
		do_get_pty=0;
	return rval;
}
