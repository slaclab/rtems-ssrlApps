/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Standalone traceroute utility for RTEMS and Linux
 * ----------------------------------------------------------------------------
 * This file is part of 'ssrlApps'. It is subject to the license terms in the
 * LICENSE.txt file found in the top-level directory of this distribution,
 * and at:
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of 't9p', including this file, may be copied, modified,
 * propagated, or distributed except according to the terms contained in the
 * LICENSE.txt file.
 * ----------------------------------------------------------------------------
 **/

#pragma once
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

enum TracerouteLog
{
  TR_LOG_NONE,
  TR_LOG_FULL,
  TR_LOG_VERBOSE
};

struct traceroute_opts
{
  struct sockaddr_in ip;
  int max_hops; /* Max number of hops */
  int log_type;
};

struct traceroute_node
{
  char addr[64];
  in_addr_t in_addr;
  struct traceroute_node* next;
};

struct traceroute_result
{
  int hops;
  struct traceroute_node* first;
};

void traceroute_opts_init(struct traceroute_opts* opts);

bool do_traceroute(const struct traceroute_opts* opts, struct traceroute_result** result);

void traceroute_result_free(struct traceroute_result* result);

#ifdef __cplusplus
}
#endif