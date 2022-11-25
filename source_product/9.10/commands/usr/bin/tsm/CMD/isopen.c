/* @(#) $Header: isopen.c,v 70.1 92/03/09 15:38:30 ssa Exp $ */
#include <sys/types.h>
#include <fcntl.h>

main(argc,argv)
int argc;
char *argv[];
{
  int fd;

  if(argc != 2) exit(1);
  fd = open(argv[1], O_WRONLY | O_NDELAY);
  exit (fd < 0);
}
