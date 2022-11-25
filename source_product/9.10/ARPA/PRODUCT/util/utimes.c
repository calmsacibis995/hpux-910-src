/* includes for utime() */
#include <sys/types.h>
#include <unistd.h>

/* include for utimes() */
#include <sys/time.h>

/* Emulation of 4.3 BSD utimes(2) function */
/* This should be a pretty good emulation, since the utime() errnos */
/* are a subset of those legal for utimes() and the return values   */
/* are identical. */

int utimes(file, tvp)
     char *file;
     struct timeval tvp[2];
{
  struct utimbuf times;

  times.actime = tvp[0].tv_sec;
  times.modtime = tvp[1].tv_sec;

  return utime(file, &times);
}
