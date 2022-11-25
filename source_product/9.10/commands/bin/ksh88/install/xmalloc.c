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
 * This program test whether a program can have its own malloc
 * and free routines
 */

char *malloc();
int free();

main()
{
	char *cp = malloc(4);
	free(cp);
	exit(0);
}

char *malloc(n)
{
	static char buff[100];
	return(buff);
}

int free(){}
