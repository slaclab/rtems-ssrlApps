/* $Id$ */

/* Network configuration utilities */

/* T.S, 2005,2006,2007 */

#include <rtems.h>
#include <rtems/rtems_bsdnet.h>
#include <rtems/rtems_bsdnet_internal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <bsp.h>

/* configure interface 'name' to use IP address 'addr' and netmask 'msk'
 * (both strings in IP 'dot' notation). Bring IF up.
 *
 * 'addr' may be NULL to bring the IF down.
 */
int
ifconf(char *nam, char *addr, char *msk)
{
struct sockaddr_in sin;
int	               flags;
int                rval = -1;

	if ( !nam ) {
		fprintf(stderr,"usage: ifconf(char *if_name, char *ip_addr, char *ip_mask)\n");
		return -1;
	}

	memset( &sin, 0, sizeof(sin) );
	sin.sin_len     = sizeof(sin);
	sin.sin_family  = AF_INET;
	sin.sin_port    = htons(0);

	flags = addr ? IFF_UP : 0;

	if ( rtems_bsdnet_ifconfig(nam, SIOCSIFFLAGS, &flags ) ) {
		fprintf(stderr,"Unable to bring '%s' %s\n", nam, addr ? "UP" : "DOWN");
		return -1;
	}
	if ( !addr )
		return 0;

	if ( !inet_aton(addr, &sin.sin_addr) ) {
		fprintf(stderr,"Invalid IP address '%s'\n", addr);
		goto cleanup;
	}
	if ( rtems_bsdnet_ifconfig(nam,SIOCSIFADDR,&sin) ) {
		fprintf(stderr,"Unable to set IP address on '%s'\n",nam);
		goto cleanup;
	}		
	if ( msk ) {
		if ( !inet_aton(msk, &sin.sin_addr) ) {
			fprintf(stderr,"Invalid IP MASK '%s'\n", msk);
			goto cleanup;
		}
		if ( rtems_bsdnet_ifconfig(nam,SIOCSIFNETMASK,&sin) ) {
			fprintf(stderr,"Unable to set NETMASK address on '%s'\n",nam);
			goto cleanup;
		}
	}
	rval = 0;
cleanup:
	if ( rval ) {
		flags = 0;
		if ( rtems_bsdnet_ifconfig(nam, SIOCSIFFLAGS, &flags ) ) {
			fprintf(stderr,"Unable to bring '%s' DOWN\n", nam);
		}
	}
	return rval;
}

/* Attach interface, optionally using given ethernet address
 * (if supported by driver -- some drivers might require the address)
 *
 * enet_addr must be a pointer to an array of 6 octets defining
 * the ethernet address (or a NULL pointer to let the driver use
 * the predefined address programmed into the hardware).
 *
 * RETURNS: 0 on success, nonzero on error.
 */
int
ifattach(char *name, uint8_t *enet_addr)
{
int                          rval;
struct rtems_bsdnet_ifconfig cfg;

	memset(&cfg, 0, sizeof(cfg));

	cfg.name   = name ? name : RTEMS_BSP_NETWORK_DRIVER_NAME;
	cfg.attach = RTEMS_BSP_NETWORK_DRIVER_ATTACH;
	cfg.next   = 0;
	/* configure the interface later */
	cfg.ip_address = 0;
	cfg.ip_netmask = 0;
	cfg.hardware_address = enet_addr;

	cfg.ignore_broadcast = 0;
	cfg.mtu              = 0;
	cfg.rbuf_count       = 0;
	cfg.xbuf_count       = 0;

	cfg.port             = 0;
	cfg.irno             = 0;
	cfg.bpar             = 0;

	cfg.drv_ctrl         = 0;

	rtems_bsdnet_semaphore_obtain();
	rval = cfg.attach(&cfg, 1);
	rtems_bsdnet_semaphore_release();

	return rval;
}
