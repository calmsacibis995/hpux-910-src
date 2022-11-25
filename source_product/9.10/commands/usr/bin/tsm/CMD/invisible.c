/*****************************************************************************
** Copyright (c) 1989 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: invisible.c,v 70.1 92/03/09 15:38:26 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

char copy[] = 
"Copyright (c) 1989 Structured Software Solutions, Inc. All rights reserved.";

#include <stdio.h>
main( argc, argv )
	int	argc;
	char	**argv;
{
	FILE	*in;
	int	c;
	int	value;

	if ( argc > 1 )
	{
		if ( ( in = fopen( argv[ 1 ], "r" ) ) == NULL )
		{
			perror( argv[ 1 ] );
			exit( 1 );
		}
	}
	else
		in = stdin;
	while ( ( c = getc( in ) ) != EOF )
	{
		if ( c == '\n' )
		{
		}
		else if ( ( c < ' ' ) || ( c >= 0x7F ) )
		{
			fprintf( stderr, "Invalid char 0x%x\n", c );
			exit( 1 );
		}
		else if ( c == '^' )
		{
			if ( ( c = getc( in ) ) == EOF )
			{
				fprintf( stderr, "Unexpected EOF on ^" );
				exit( 1 );
			}
			if ( c == '?' )
			{
				putchar( 0x7F );
			}
			else if ( ( c >= '@' ) && ( c <= '_' ) )
			{
				putchar( c - '@' );
			}
			else
			{
				fprintf( stderr, "Invalid char ^ 0x%x\n", c );
				exit( 1 );
			}
		}
		else if ( c == '\\' )
		{
			if ( ( c = getc( in ) ) == EOF )
			{
				fprintf( stderr, "Unexpected EOF on \\" );
				exit( 1 );
			}
			if ( c == '\\' )
				putchar( '\\' );
			else if ( c == '^' )
				putchar( '^' );
			else if ( c == 'E' )
			{
				putchar( 0x1B );
			}
			else if ( c == 'b' )
			{
				putchar( '\b' );
				
			}
			else if ( c == 'r' )
			{
				putchar( '\r' );
			}
			else if ( c == 'n' )
			{
				putchar( '\n' );
			}
			else if ( c == 't' )
			{
				putchar( '\t' );
			}
			else if ( c == 's' )
			{
				putchar( ' ' );
			}
			else if ( ( c >= '0' ) && ( c <= '9' ) )
			{
				value = ( c - '0' ) << 6;
				if ( ( c = getc( in ) ) == EOF )
				{
					fprintf( stderr,
						"Unexpected EOF on \\nn" );
					exit( 1 );
				}
				if ( ( c < '0' ) || ( c > '9' ) )
				{
					fprintf( stderr,
						"Invalid char \\nn 0x%x\n", c );
					exit( 1 );
				}
				value += ( c - '0' ) << 3;
				if ( ( c = getc( in ) ) == EOF )
				{
				 fprintf( stderr, "Unexpected EOF on \\nnn" );
				 exit( 1 );
				}
				if ( ( c < '0' ) || ( c > '9' ) )
				{
					fprintf( stderr,
						"Invalid char \\nnn 0x%x\n",
						c );
					exit( 1 );
				}
				value += ( c - '0' );
				putchar( value );
			}
			else
			{
				fprintf( stderr, "Invalid char \\ 0x%x\n", c );
				exit( 1 );
			}
		}
		else
		{
			putchar( c );
		}
	}
}
