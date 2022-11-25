/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: info.c,v 70.1 92/03/09 15:38:19 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

/*
 * info
 *
 * program:	prints requested facetterm information on standard out
 *
 * notes:	Requires that stdin be associated with a facet window.
 *
 */

#include	<stdio.h>
#include	"facetterm.h"

main( argc, argv )
	int	argc;
	char	**argv;
{
	char	original[ FIOC_BUFFER_SIZE ];
	char	buffer[ FIOC_BUFFER_SIZE ];
	int	window_test;
	int	is_a_window_answer;
	int	not_a_window_answer;
	char	*strrchr();
	char	*p;

	window_test = 0;
	     if ( argc < 2 )
	{
		exit( 2 );
	}
	else if (  ( strcmp( argv[ 1 ], "NOT_A_WINDOW" ) == 0 )
		|| ( strcmp( argv[ 1 ], "not_a_window" ) == 0 ) )
	{
		window_test = 1;
		is_a_window_answer = 1;		/* fail */
		not_a_window_answer = 0;	/* succeed */
	}
	else if (  ( strcmp( argv[ 1 ], "IS_A_WINDOW" ) == 0 )
	        || ( strcmp( argv[ 1 ], "is_a_window" ) == 0 ) )
	{
		window_test = 1;
		is_a_window_answer = 0;		/* succeed */
		not_a_window_answer = 1;	/* fail */
	}

	if ( window_test )
	{
		strcpy( buffer, "facet_type" );
		if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
			exit( not_a_window_answer );
		else if ( strcmp( buffer, "FACET/TERM" ) == 0 )
			exit( is_a_window_answer );
		else if ( strcmp( buffer, "FACET/PC" ) == 0 )
			exit( is_a_window_answer );
		else
			exit( not_a_window_answer );
	}
		
	strncpy( original, argv[ 1 ], FIOC_BUFFER_SIZE );
	original[ FIOC_BUFFER_SIZE - 1 ] = '\0';
	strcpy( buffer, original );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
	{
		perror( argv[ 0 ] );
		exit( 1 );
	}
	else if ( strcmp( buffer, original ) == 0 )
	{
		fprintf ( stderr, "%s: Invalid Argument\n", argv[ 0 ] );
		exit( 3 );
	}
	else
	{
		printf ( "%s\n", buffer );
		exit( 0 );
	}
}
