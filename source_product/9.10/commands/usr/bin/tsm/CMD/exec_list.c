/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: exec_list.c,v 70.1 92/03/09 15:37:34 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/*
 * exec_list	%W% %U% %G%
 *
 * sends packet containing commandline to facetterm or PC
 */

#include        "facetwin.h"

#define EXECLIST_LEN    80

exec_list (argc, argv)
	int	argc;
	char	*argv[];
{
	int	i;
	char	exec_list_buff[EXECLIST_LEN + 10 ];
	int	left;

	exec_list_buff[0] = 0;
	for( i = 0; i < argc; i++ )
	{
		if ( i > 0 )
			strcat( exec_list_buff, " " );
		if ( strlen( exec_list_buff ) + strlen( argv[i] ) >= 
							EXECLIST_LEN )
		{
			left = EXECLIST_LEN - strlen( exec_list_buff );
			if ( left > 0 )
				strncat( exec_list_buff, argv[ i ], left );
			break;
		}
		strcat( exec_list_buff, argv[i] );
	}
	exec_list_buff[ strlen( exec_list_buff ) ] = '\0';
	fct_ioctl( 0, EXEC_LIST, exec_list_buff );
}
