/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_paste.c,v 70.1 92/03/09 15:37:48 ssa Exp $ */
/*****************************************************************************
     NAME
	  fct_paste - paste_characters to a facet window.

     SYNOPSIS
	  fct_paste window characters

     DESCRIPTION
	  The program "fct_paste" sends "characters" to the FACET/TERM
	  window "window" as if they were typed on the keyboard.

	  Stdin must be a FACET window.

	  If window is -1, then the window associated with stdin is pasted to.
	  If window is 0 or 10 then window 10 is pasted to.
	  If window is 1 through 9 then the respective window is pasted to.

	  Characters may be of the form:
		\r	return
		\n	return
		\E	escape
		\e	escape
		^X	Control-X
		^?	delete
		\\	\
		\^	^
*****************************************************************************/
#include <stdio.h>

char	*Argv0 = (char *) 0;

main( argc, argv )
	int	argc;
	char	**argv;
{
	char	w;
	int	winno;

	Argv0 = argv[ 0 ];
	if ( argc < 3 )
	{
		printf( "Usage: %s window characters\n", argv[ 0 ] );
		exit( 1 );
	}
	winno = atoi( argv[ 1 ] );
	if ( winno == -1 )
	{
		w = '*';
	}
	else if ( ( winno >= 1 ) && ( winno <= 9 ) )
	{
		w = '0' + winno;
	}
	else if ( winno == 10 )
	{
		w = '0';
	}
	else if ( ( winno == 0 ) && ( *argv[ 1 ] == '0' ) )
	{
		w = '0';
	}
	else
	{
		printf( "Usage: %s window characters\n", argv[ 0 ] );
		exit( 1 );
	}
	send_command( w, argv[ 2 ] );
	exit( 0 );
}
send_command( winchar, input )
	char	winchar;
	char	*input;
{
	char	command[ 256 ];
	char	*s;
	char	*d;
	char	c;

	s = input;
	d = command;
	*d++ = winchar;
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
				*d++ = '\r';
			else if ( c == 'n' )
				*d++ = '\n';
			else if ( c == 'E' )
				*d++ = '\033';
			else if ( c == 'e' )
				*d++ = '\033';
			else if ( c == '\\' )
			{
				*d++ = c;
			}
			else if ( c == '^' )
			{
				*d++ = c;
			}
			else
			{
				*d++ = '\\';
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
				*d++ = c - 0x40;
			else if ( ( c >= 0x60 ) && ( c <= 0x7F ) )
				*d++ = c - 0x60;
			else if ( c == '?' )
				*d++ = 0x7F;
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
	if ( fct_ioctl( 0, FIOC_PASTE, buffer ) == -1 )
	{
		perror( Argv0 );
		exit( 1 );
	}
}
