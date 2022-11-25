/*****************************************************************************
** Copyright (c) 1989 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: visible.c,v 70.1 92/03/09 15:39:08 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

char copy[] = 
  "Copyright (c) 1989 Structured Software Solutions, Inc. All rights reserved.";
#include <stdio.h>
int	Cut_at_escape = 0;
main( argc, argv )
	int	argc;
	char	**argv;
{
	FILE	*in;
	int	c;

	if ( argc > 1 )
	{
		if ( strcmp( argv[ 1 ], "-e" ) == 0 )
		{
			Cut_at_escape = 1;
			argc--;
			argv++;
		}
	}
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
		if ( c == ' ' )
		{
			out( '\\' );
			out( 's' );
		}
		else if ( c < ' ' )
		{
			if ( c == 0x1B )
			{
				if ( Cut_at_escape )
					new_line();
				else 
					check_escape();
				out( '\\' );
				out( 'E' );
			}
			else if ( c == '\b' )
			{
				out( '\\' );
				out( 'b' );
				
			}
			else if ( c == '\r' )
			{
				out( '\\' );
				out( 'r' );
			}
			else if ( c == '\n' )
			{
				out( '\\' );
				out( 'n' );
			}
			else if ( c == '\t' )
			{
				out( '\\' );
				out( 't' );
			}
			else
			{
				out( '^' );
				out( '@' + c );
			}
		}
		else if ( c < 0x7F )
		{
			if ( c == '\\' )
			{
				out( '\\' );
				out( c );
			}
			else if ( c == '^' )
			{
				out( '\\' );
				out( c );
			}
			else
			{
				out( c );
			}
		}
		else if ( c == 0x7F )
		{
			out( '^' );
			out( '?' );
		}
		else
		{
			out( '\\' );
			out( '0' + ( ( c >> 6 ) & 0x3 ) );
			out( '0' + ( ( c >> 3 ) & 0x7 ) );
			out( '0' + (   c        & 0x7 ) );
		}
		check();
	}
	putchar( '\n' );
}
int	Col = 0;
out( c )
	int	 c;
{
	putchar( c );
	Col++;
}
check_escape()
{
	if ( Col > 60 )
	{
		new_line();
	}
}
check()
{
	if ( Col > 70 )
	{
		new_line();
	}
}
new_line()
{
	putchar( '\n' );
	Col = 0;
}
