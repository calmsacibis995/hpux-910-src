/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tixer.c,v 70.1 92/03/09 16:14:03 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/*
 * File: tixer.c
 * Creator: G.Clark.Brown
 *
 * Main module for the Terminfo Exerciser Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>

#include	"tixermacro.h"
#include	"tixer.h"

#include	<sys/stat.h>

#ifndef lint
	static char	Sccsid[]="%W% %E% %G%";
#endif

/*
 * Global data.
 */


struct	stat		Statbuf;
extern FILE		*u_fopen();
static struct option_tab	options;

/* = = = = = = = = = = = = = = = = = main = = = = = = = = = = = = = = = = = =*/
/*
 * Main routine.
 */

main(argc, argv)

int	argc;
char	*argv[];

{


/*
 * Parse options.
 */
 	init_options(&options);

	if (parse_options(argc, argv, &options))
	{
		bad_usage("", argv[0]);
		u_term(1);
	}

/*
 * Call loop through selected sections.
 */

	selector(&options);

	return(0);
}

/* = = = = = = = = = = = = = = = bad_usage = = = = = = = = = = = = = = = =*/
/*
 * Print error message for incorrect command line.
 */

bad_usage(string, argv0)

char	*string;
char	*argv0;

{
	
	if (string[0])
		ERR2("%s: %s.\r\n", argv0, string);
		
	ERR1(
"Usage: %s [-t<keyletter>] [-i<TERM-id>] [-r] [-c<command>]\r\n",
	argv0);
}

/* = = = = = = = = = = = = = = = tc_putchar = = = = = = = = = = = = = = = =*/
/*
 * Put one character out on the terminal that is being tested.
 */

tc_putchar(c)

int	c;

{
	
	putchar( c );
	return 0;
}

