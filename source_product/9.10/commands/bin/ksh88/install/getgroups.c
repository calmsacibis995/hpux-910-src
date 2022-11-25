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
 * Check getgroups
 */

#include	<signal.h>

extern int getgroups();
int sigsys();

main()
{
	int groups[100];
	int n;
	signal(SIGSYS,sigsys);
	n = getgroups(100,groups);
	if(n>0)
		exit(0);
	exit(1);
}

sigsys()
{
	exit(1);
}
