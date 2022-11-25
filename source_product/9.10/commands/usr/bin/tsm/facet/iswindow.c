/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: iswindow.c,v 66.4 90/09/20 12:01:50 kb Exp $ */
/* COMMON module in facetterm facet and CMD */
#include	"facetterm.h"
iswindow()           /* return non-zero for true - zero for false */
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "facet_type" );
	if ( ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
		return( 0 );			/* not window */
	else if ( strcmp( buffer, "FACET/TERM" ) == 0 )
		return( 1 );			/* window */
	else if ( strcmp( buffer, "FACET/PC" ) == 0 )
		return( 1 );			/* window */
	else
		return( 0 );			/* not window */
}
