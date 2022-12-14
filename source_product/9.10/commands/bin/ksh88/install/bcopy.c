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
 * make sure that there is a bcopy in the library and that
 * it works
 */

extern bcopy();

char foo[] = "abcdefg";
char bar[] = "xxxxxxx";

main()
{
	bcopy(foo,bar,sizeof(foo));
	exit(strcmp(foo,bar));
}
