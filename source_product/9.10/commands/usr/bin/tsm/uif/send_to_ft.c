/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: send_to_ft.c,v 70.1 92/03/09 16:16:19 ssa Exp $ */
/*
 * send_to_ft.c
 *
 * Facet/Term aware calls for the Facet User Interface.
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved.
 *
 */

#include	<ctype.h>
#include	"facetterm.h"
#include	"facetpath.h"
#include	"fmenu.h"

					/* character used in FACET/TERM ioctl
					** string to switch to the keyboard.
					*/

extern char	Window_mode_to_user_char;

ft_init( menu_window )
{
	ft_send_idle_window_popup( menu_window );
	ft_send_invisible_scan_window( menu_window );
	ft_send_notify_window( menu_window );
	ft_get_ft_hot_key( Ft_hot_key );
}
ft_quit( menu_window )
{
	ft_send_not_notify_window( menu_window );
	ft_send_not_invisible_scan_window( menu_window );
	ft_send_no_idle_window();
}


				/* This collection of routines sends commands
				** to FACET/TERM.
				*/

ft_send_idle_window_popup( winno )
int winno;
{
	char buff[ 5 ];

	buff[ 0 ] = 'i';
	if ( winno == 10 )
		buff[ 1 ] = '0';
	else
		buff[ 1 ] = '0' + winno;
	buff[ 2 ] = 'm';
	buff[ 3 ] = '\r';
	buff[ 4 ] = '\0';
	ft_send_command( buff );
}
ft_send_no_idle_window()
{
	ft_send_command( "in\r" );
}
ft_send_notify_window( winno )
int winno;
{
	char buff[ 5 ];

	buff[ 0 ] = 'x';
	buff[ 1 ] = 'n';
	if ( winno == 10 )
		buff[ 2 ] = '0';
	else
		buff[ 2 ] = '0' + winno;
	buff[ 3 ] = 'Y';
	buff[ 4 ] = '\0';
	ft_send_command( buff );
}
ft_send_not_notify_window( winno )
int winno;
{
	char buff[ 5 ];

	buff[ 0 ] = 'x';
	buff[ 1 ] = 'n';
	if ( winno == 10 )
		buff[ 2 ] = '0';
	else
		buff[ 2 ] = '0' + winno;
	buff[ 3 ] = 'N';
	buff[ 4 ] = '\0';
	ft_send_command( buff );
}
ft_send_invisible_scan_window( winno )
int winno;
{
	char buff[ 5 ];

	buff[ 0 ] = 'x';
	buff[ 1 ] = 'i';
	if ( winno == 10 )
		buff[ 2 ] = '0';
	else
		buff[ 2 ] = '0' + winno;
	buff[ 3 ] = 'Y';
	buff[ 4 ] = '\0';
	ft_send_command( buff );
}
ft_send_not_invisible_scan_window( winno )
int winno;
{
	char buff[ 5 ];

	buff[ 0 ] = 'x';
	buff[ 1 ] = 'i';
	if ( winno == 10 )
		buff[ 2 ] = '0';
	else
		buff[ 2 ] = '0' + winno;
	buff[ 3 ] = 'N';
	buff[ 4 ] = '\0';
	ft_send_command( buff );
}
ft_send_select_window( winno )
int winno;
{
	char buff[ 4 ];

	if ( winno == 10 )
		buff[ 0 ] = '0';
	else
		buff[ 0 ] = '0' + winno;
	buff[ 1 ] = '\0';
	ft_send_command( buff );
}
ft_send_popup_start( winno )
int winno;
{
	char	buff[ 3 ];

	buff[ 0 ] = 'z';
	if ( winno == 10 )
		buff[ 1 ] = '0';
	else
		buff[ 1 ] = '0' + winno;
	buff[ 2 ] = '\0';
	ft_send_command( buff );
}
ft_send_push_popup()
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	buffer[ 0 ] = '\0';
	fct_ioctl( 0, FIOC_PUSH_POPUP_SCREEN, buffer );
}
ft_send_pop_popup()
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	buffer[ 0 ] = '\0';
	fct_ioctl( 0, FIOC_POP_POPUP_SCREEN, buffer );
}
ft_paste_to_window( winno, paste_data )
int winno;
char *paste_data;
{
	char buffer[ FIOC_BUFFER_SIZE ];

	if ( winno < 10 )
		buffer[ 0 ] = winno + '0';
	else
		buffer[ 0 ] = '0';
	string_encode( paste_data, buffer + 1 );
	buffer[ FIOC_BUFFER_SIZE - 1 ] = '\0';
	fct_ioctl( 0, FIOC_PASTE, buffer );
}
ft_send_exit_window_mode()
{
	ft_send_command( " " );
}
run_program( title, program, winno )
	char	*title, *program;
	int	winno;
{
	char	command[ 256 ];

	sprintf( command, "%s/bin/%srunwin %d \"%s\" %s", Facettermpath,
		 Facetprefix, winno, title, program );
	winno = system( command );
	if ( winno <= 0 )
	{
		return( -1 );
	}
	else
	{
		winno >>= 8;
		if ( winno < 1 || winno > 10 )
			return( -1 );
		else
			return( winno );
	}
}
				/* The following routines provide a simple
				** interface to the FACET/TERM window command
				** mode ioctl.
				** They understand some of the \ and ^ 
				** conventions such as \r for return and ^C
				** for CONTROL-C.
				*/
ft_send_command_then_user( s )
	char	*s;
{
	char command[ 256 ];
	int len;

	strcpy( command, s );
	len = strlen( command );
						/* Get the rest from the user.
						** Without this code FACET/TERM
						** interprets this as though 
						** the user has typed Control-W,
						** C, and ESCAPE.  This causes
						** it to exit window command
						** mode with no action.
						*/
	command[ len ] = Window_mode_to_user_char;
	command[ len+1 ] = '\0';
	ft_send_command( command );
}
ft_send_command( s )
	char	*s;
{
	char	command[ 256 ];
	char	*d;
	char	c;

	d = command;
	while ( ( c = *s++ ) != '\0' )
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
	ft_send_command_raw( command );
}
ft_send_command_raw( command )
	char	*command;
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strncpy( buffer, command, FIOC_BUFFER_SIZE );
	buffer[ FIOC_BUFFER_SIZE - 1 ] = '\0';
	if ( fct_ioctl( 0, FIOC_WINDOW_MODE, buffer ) == -1 )
	{
		perror( "FIOC_WINDOW_MODE" );
		exit( -1 );
	}
}
ft_get_ft_hot_key( s )
char *s;
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "hotkey" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
	{
		perror( "FIOC_GET_INFORMATION" );
		exit( -1 );
	}
	if ( buffer[ 0 ] == '\0' )
	{
		printf( "Cannot determine hotkey\r\n" );
		exit( -1 );
	}
	strncpy( s, buffer, 2 );
	s[2] = 0;
}
ft_get_current_window()
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "current_window_number" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
	{
		perror( "FIOC_GET_INFORMATION" );
		exit( -1 );
	}
	return( atoi( buffer ) );
}
ft_get_window_number()
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "window_number" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
	{
		perror( "FIOC_GET_INFORMATION" );
		exit( -1 );
	}
	if ( buffer[ 0 ] == '\0' )
	{
		printf( "Cannot determine window number\r\n" );
		exit( -1 );
	}
	return( atoi( buffer ) );
}
#define MAX_ACTIVE_WINDOWS FIOC_BUFFER_SIZE
only_active( manager_window_number )
	int	manager_window_number;
{
	int	active[ 11 ];
	int	i;

	get_active_windows( active );
	for ( i = 1; i <= 10; i++ )
	{
		if ( ( i != manager_window_number ) && ( active[ i ] ) )
			return( 0 );
	}
	return( 1 );
}
get_active_windows( active )
	int	active[];
{
	char    active_windows[ MAX_ACTIVE_WINDOWS + 1 ];
	char	*p;
	int	i;
	char	c;

	ask_active_windows( active_windows, MAX_ACTIVE_WINDOWS );
	for ( i = 1; i <= 10; i++ )
		active[ i ] = 0;
	p = active_windows;
	while ( *p != '\0'  )
	{
		c = *p++;
		if ( ( c >= '2' ) && ( c <= '9' ) )
		{
			active[ c - '0' ] = 1;
		}
		else if ( c == '1' )
		{
			if ( *p == '0' )
			{
				active[ 10 ] = 1;
				p++;
			}
			else
				active[ 1 ] = 1;
		}
	}
}
ask_active_windows( active_windows, max )
	char	*active_windows;
	int	max;		/* active windows is one larger for null */
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "active_window_numbers" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
	{
		perror( "FIOC_GET_INFORMATION" );
		exit( -1 );
	}
	buffer[ FIOC_BUFFER_SIZE - 1 ] = '\0';
	strncpy( active_windows, buffer, max );
	active_windows[ max ] = '\0';
}
ask_window_title( active_window_number, program_name, max )
	int	active_window_number;
	char	*program_name;
	int	max;			/* buffer is one larger for null */
{
	char	buffer[ FIOC_BUFFER_SIZE ];

	sprintf( buffer, "program_name_%d", active_window_number );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
	{
		program_name[ 0 ] = '\0';
		return( -1 );
	}
	buffer[ FIOC_BUFFER_SIZE - 1 ] = '\0';
	strncpy( program_name, buffer, max );
	program_name[ max ] = '\0';
	return( 0 );
}
