/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: putprog.c,v 70.1 92/03/09 15:38:49 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

/*
 * putprog
 *
 * program:	record title on window
 *
 * notes:	Requires that stdin be associated with a facet window.
 *
 */

#include	"facetterm.h"
#include	"facetwin.h"

main( argc, argv )
	int	argc;
	char	**argv;
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	if ( argc < 2 )
	{
		exit( 2 );
	}
	strncpy( buffer, argv[ 1 ], FIOC_BUFFER_SIZE );
	buffer[ FIOC_BUFFER_SIZE - 1 ] = '\0';
	if ( fct_ioctl( 0, EXEC_LIST, buffer ) == -1 )
	{
		perror( argv[ 0 ] );
		exit( 1 );
	}
	exit( 0 );
}
