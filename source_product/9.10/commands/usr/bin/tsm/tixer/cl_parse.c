/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: cl_parse.c,v 70.1 92/03/09 16:12:47 ssa Exp $ */
/*
 * File:		cl_parse.c
 * Creator:		G.Clark.Brown
 * Creation Date:	9/6/88
 *
 * Command line parser for the Terminfo Exerciser Program.
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
#include	<pwd.h>
#include	<ctype.h>

#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

#ifndef lint
	static char	Sccsid[80]="%W% %E% %G%";
#endif

/* = = = = = = = = = = = = = = = parse_options = = = = = = = = = = = = = = = =*/
/*
 * Main parsing routine.
 */

parse_options(argc, argv, p_opts)

int	argc;
char	*argv[];
struct option_tab	*p_opts;

{
	int		c;
	extern int	optind, opterr;
	extern char	*optarg;
	


/*
 * Cycle through command line arguments and set options.
 */
 
	while ((c = getopt(argc, argv, "t:i:rc:z:")) != EOF)
	{
		switch(c)
		{
		case 't':
			if ((p_opts->test=parse_tp(optarg)) < 0)
			{
				ERR1("Unknown test selection: %s.\n", optarg);
				return -1;
			}
			break;

		case 'i':
			U_strcpy(p_opts->type, optarg);

			break;

		case 'c':
			U_strcpy(p_opts->command, optarg);

			break;

		case 'r':
			p_opts->runaway=1;

			break;

		case 'z':
			break;

		case '?':
			return -1;

		default:
			ERR1("Unknown option: %c.\n", c);
			return -1;
		}		
	}

	return 0;

}

/* = = = = = = = = = = = = = = = parse_debug = = = = = = = = = = = = = = = =*/
/*
 * Takes a string of keyletters and sets flags in section array.
 */

parse_debug(sec, string)

char	*string;
int	sec[];

{
	int	index=0, c;


	while (c=string[index++])
	{
		if(c >= 'A' && c <= 'Z')
			sec[c-'A'] = DETAIL;
		else if(c >= 'a' && c <= 'z')
			sec[c-'a'] = SUMMARY;
		else
		{
			ERR1("Invalid debug keyletter: %c.\n", c);
			return -1;
		}
	}
	return 0;
}

/* = = = = = = = = = = = = = = = parse_tp = = = = = = = = = = = = = = = = = =*/
/*
 * Match the selected keyletter with the requested test.
 */

parse_tp(string)

char	*string;

{
	char	*ip;
	char	*list;
	int	ret;


	list = "1sa";

	if (strlen(string) > 1 && string[1] != '\n' )
		ret = -1;

	else if (strlen(string) < 1)
		ret = -2;

	else if ((ip = strchr(list, string[0])) == NULL)
		ret = -3;

	else
		ret = ip-list;
		


	return ret;
}

/* = = = = = = = = = = = = = = = init_options = = = = = = = = = = = = = = = =*/
/*
 * Initialize options to unset state.
 */

init_options(p_opts)

struct option_tab	*p_opts;

{
	p_opts->type[0] = '\0';
	p_opts->test = -1;
	p_opts->captab = -1;
	p_opts->runaway = 0;
	p_opts->command[0] = '\0';
}



