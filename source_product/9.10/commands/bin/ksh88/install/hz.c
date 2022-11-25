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
/* prints out define for HZ from sys/param.h */
#   include	<sys/param.h>

main()
{
#ifdef HZ
	printf("#define HZ\t%d\n",HZ);
#endif
}

