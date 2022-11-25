/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: iswindow.c,v 66.6 90/10/18 09:23:49 kb Exp $ */
/* COMMON module in facetterm facet and CMD */
#include	"facetterm.h"
/**************************************************************************
* iswindow
*	Determine if stdin is a FACET window or not.
**************************************************************************/
iswindow()           /* return non-zero for true - zero for false */
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "facet_type" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
		return( 0 );			/* not window */
	else if ( strcmp( buffer, "FACET/TERM" ) == 0 )
		return( 1 );			/* window */
	else if ( strcmp( buffer, "FACET/PC" ) == 0 )
		return( 1 );			/* window */
	else
		return( 0 );			/* not window */
}
