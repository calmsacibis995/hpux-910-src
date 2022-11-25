/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: notwin.c,v 70.1 92/03/09 15:38:40 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

/*
 * notwin
 *
 * program:	return exit code of 0 if not running on a FACET/PC or
 *							  FACET/TERM window.
 *
 */

#include	"facetterm.h"

main()
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "facet_type" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
		exit( 0 );
	else if ( strcmp( buffer, "FACET/TERM" ) == 0 )
		exit( 1 );
	else if ( strcmp( buffer, "FACET/PC" ) == 0 )
		exit( 1 );
	else
		exit( 0 );
}
