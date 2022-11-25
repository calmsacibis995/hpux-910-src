/*
 * A program that acts as the front end of a script for vhe.
 * The script needs to be called by root so it can do a mount and this
 * is a safer way to do it then with a setuid script.
 *
 *  $Header: vhe_u_mnt.c,v 1.5.109.1 91/11/19 14:20:20 kcs Exp $
 *
 * (c) Copyright 1988 Hewlett-Packard Company
 *
 */

#ifndef lint
static char rcsid[] = "@(#)vhe_u_mnt:	$Revision: 1.5.109.1 $	$Date: 91/11/19 14:20:20 $";
#endif

#include <sys/errno.h>
#include <stdio.h>
extern  int errno;
extern  char **environ;

main(argc, argv)
int	argc;
char	*argv[];
{
char	*name;
int	num;

	/*
	 * Clean up the old environment and then execute 
	 * /usr/etc/vhe/vhe_script
	 */

	cleanenv(&environ, 0);

	system("/usr/etc/vhe/vhe_script");
}
