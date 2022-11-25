/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: command.c,v 70.1 92/03/09 15:37:30 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/*
 * fct_command
 *
 * program:	send window commands to facet
 *
 * notes:	Requires that stdin be associated with a facet window.
 *
 */

#include <stdio.h>

char	*Argv0 = (char *) 0;

main( argc, argv )
	int	argc;
	char	**argv;
{
	char	input[ 256 ];
	int	i;

	Argv0 = argv[ 0 ];
	if ( argc < 2 )
	{
		printf( "Usage: %s command ...\n", Argv0 );
		exit( 1 );
	}
	for ( i = 1; i < argc; i++ )
	{
		send_command( argv[ i ] );
	}
	exit( 0 );
}
send_command( input )
	char	*input;
{
	char	command[ 256 ];
	char	*s;
	char	*d;
	char	c;

	s = input;
	d = command;
	while ( ( c = *s++ ) != 0 )
	{
		if ( c == '\\' )
		{
			c = *s++;
			if ( c == '\0' )
			{
				*d++ = '\\';
				break;
			}
			else if ( c == 'r' )
			{
				*d++ = '\r';
			}
			else if ( c == 'E' )
			{
				*d++ = '\033';
			}
			else if ( c == '\\' )
			{
				*d++ = c;
			}
			else
			{
				*d++ = c;
			}
		}
		else if ( c == '^' )
		{
			c = *s++;
			if ( c == '\0' )
			{
				*d++ = '^';
				break;
			}
			else if ( ( c >= 0x40 ) && ( c <= 0x5F ) )
			{
				*d++ = c - 0x40;
			}
			else
			{
				*d++ = '^';
				*d++ = c;
			}
		}
		else
		{
			*d++ = c;
		}
	}
	*d++ = '\0';
	send_command_raw( command );
}
#include "facetterm.h"
send_command_raw( command )
	char	*command;
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strncpy( buffer, command, FIOC_BUFFER_SIZE );
	buffer[ FIOC_BUFFER_SIZE - 1 ] = '\0';
	if ( fct_ioctl( 0, FIOC_WINDOW_MODE, buffer ) == -1 )
	{
		perror( Argv0 );
		exit( -1 );
	}
}
