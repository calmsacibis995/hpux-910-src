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
/*
 * see whether there is a select() that can be used for fine timing
 * This defines _SECECT_ if select() exists and delays
 */

#include	<sys/types.h>
#include	<sys/time.h>

main()
{
	time_t t1,t2;
	struct timeval timeloc;
	timeloc.tv_sec = 2;
	timeloc.tv_usec = 0;
	time(&t1);
	select(0,(fd_set*)0,(fd_set*)0,(fd_set*)0,&timeloc);
	time(&t2);
	if(t2 > t1)
	{
		printf("#define _SELECT_	1\n");
		exit(0);
	}
	exit(1);
}
