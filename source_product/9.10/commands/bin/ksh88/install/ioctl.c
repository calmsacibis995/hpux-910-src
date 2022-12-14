/* HPUX_ID: @(#) $Revision: 66.1 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
#include	<termio.h>

main()
{
	int fd;
	struct termio term;
	if ((fd=open("/dev/tty",0)) < 0)
		exit(1);
	if(ioctl(fd,TCGETA,&term) < 0)
	{
		printf("#undef _termio_\n");
		printf("#undef _sys_termio_\n");
		exit(0);
	}
	exit(0);
}
	
