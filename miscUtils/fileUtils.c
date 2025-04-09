
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

  char buf[512];

  ssize_t l;
  while ((l = read(fd, buf, sizeof (buf)-1)) > 0) {
    buf[l] = 0;
    puts(buf);
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
