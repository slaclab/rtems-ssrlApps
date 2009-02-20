/*
 *  This file contains a simple ICMP ping client interface.
 *
 *  COPYRIGHT (c) 2005.
 *  LOYTEC electronics GmbH, 2005.
 *
 *  Thomas Rauscher <trauscher@loytec.com>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 */

#ifndef SSRLAPPS_MISCUTILS_PING_H
#define SSRLAPPS_MISCUTILS_PING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtems_ping
{
  int                 socket;
  int                 seq;
  /* stupid alias rule */
  union {
	  struct sockaddr_in  dest_in;
	  struct sockaddr     dest_sa;
  }                  dest_u;
  void               *payload;
  size_t              payload_size;
  struct timeval      timeout;
  size_t              req_size;
  size_t              resp_size;
  union {  
    unsigned char *raw_req;
    struct icmp   *icmp_req;
  };

  struct ip   *raw_resp;
  struct icmp *icmp_resp;

} rtems_ping_t;

typedef struct rtems_ping_cfg
{
  struct timeval timeout;
  size_t         payload_size;
} rtems_ping_cfg_t;

extern rtems_ping_cfg_t rtems_ping_standard;

rtems_ping_t *rtems_ping_open(uint32_t              ip_addr,
			      const rtems_ping_cfg_t *cfg);

/* trip_time is in microseconds */
int rtems_ping_send(rtems_ping_t *ping, rtems_interval *trip_time);

int rtems_ping_close(rtems_ping_t *ping);

/* Convenience Wrapper; if 'quiet' is set nothing is printed to stdout
 *
 * RETURNS: 
 *
 *   roundtrip-time (us) on success. Always >= 0
 *
 *   - number of dropped pings (if at least one dropped ping)
 *   
 *   value <= -(retries+2) on networking/system error
 *
 */
int rtems_ping(char *ip_dot_addr, int retries, int quiet);

#ifdef __cplusplus
}
#endif

#endif
