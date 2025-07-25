/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: File utils
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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * Read a file and print it to stdout
 */
int
cat(const char* file)
{
  int fd = open(file, O_RDONLY);
  if (fd < 0) {
    perror("cat");
    return -1;
  }

  ssize_t l;
  char buf[4096];
  while ((l = read(fd, buf, sizeof (buf)-1)) > 0) {
    buf[l] = 0;
    fwrite(buf, l, 1, stdout);
  }

  close(fd);
  return 0;
}

#ifdef HAVE_CEXP
#include <cexpHelp.h>
CEXP_HELP_TAB_BEGIN(sockstats)
	HELP(
"Read a file and print it to stdout\n",
	int, cat,  (const char* file)
	),
CEXP_HELP_TAB_END
#endif
