/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: info.c,v 70.1 92/03/09 15:39:52 ssa Exp $ */
#include "wins.h"
#include "ptypath.h"
#ifndef FACET
#include "printer.h"
#include "options.h"
#endif

/**************************************************************************
* get_information
*	Answer request for information from fct_info.
*	The request is in the string "in".
*	The reply is to be put in the string "out"
*	which can hold a maximum of "max" characeters.
*	The default window number for the requests is "winno".
**************************************************************************/
get_information( winno, in, out, max )
	int	winno;
	char	*in;
	char	*out;
	int	max;
{
	char	*ttyn;
	char	*ttyname();

	blank_trim( in );
	if ( strcmp( in, "ttyname" ) == 0 )
	{
		ttyn = ttyname( 0 );
		if ( ttyn != (char *) 0 )
			strncpy( out, ttyn, max );
		else
			strncpy( out, "ttyname failed", max );
	}
	else if (  ( strcmp( in, "window_number" ) == 0 )
	        || ( strcmp( in, "session_number" ) == 0 )
		)
	{
		sprintf( out, "%d", winno + 1 );
	}
	else if (  ( strcmp( in, "number_of_windows" ) == 0 )
		|| ( strcmp( in, "number_of_sessions" ) == 0 )
		)
	{
		sprintf( out, "%d", Wins );
	}
	else if ( strncmp( in, "ptyname_", 8 ) == 0 )
	{
		int	w;
		char	*p;

		p = &in[ 8 ];
		w = atoi( p ) - 1;
		if ( ( w >= 0 ) && ( w < Wins ) )
			strncpy( out, Slave_pty_path[ w ], max );
		else if ( ( w == -1 ) && ( *p == '0' ) )
			strncpy( out, Slave_pty_path[ 9 ], max );
		else
			out[ 0 ] = '\0';
	}
#ifndef FACET
	else if ( strcmp( in, "local_printer" ) == 0 )
	{
		int	i;

		for ( i = 0; i < Wins; i++ )
		{
			if ( Window_printer[ i ] )
			{
				strncpy( out, Slave_pty_path[ i ], max );
				break;
			}
		}
	}
#endif
	else if ( strcmp( in, "facet_type" ) == 0 )
	{
#ifndef FACET
		strcpy( out, "FACET/TERM" );
#else
		strcpy( out, "FACET/PC" );
#endif
	}
	else if (  ( strcmp( in, "current_window_number" ) == 0 )
		|| ( strcmp( in, "current_session_number" ) == 0 )
		)
		get_curwin_string( out );
	else if (  ( strcmp( in, "active_window_numbers" ) == 0 )
		|| ( strcmp( in, "active_session_numbers" ) == 0 )
		)
		build_active_list( out );
	else if (  ( strcmp( in, "idle_window_numbers" ) == 0 )
		|| ( strcmp( in, "idle_session_numbers" ) == 0 )
		)
		build_idle_list( out );
	else if ( strcmp( in, "program_name" ) == 0 )
		get_program_name( winno, "", out );
	else if ( strncmp( in, "program_name_", 13 ) == 0 )
		get_program_name( winno, &in[ 13 ], out );
#ifndef FACET
	else if ( strcmp( in, "hotkey" ) == 0 )
	{
		if ( Opt_hotkey < ' ' )
			sprintf( out, "^%c", Opt_hotkey + 0x40 );
		else
			sprintf( out, "%c", Opt_hotkey );
	}
	else if ( strcmp( in, "termio" ) == 0 )
	{
		window_termio_in_hex( out );
	}
#endif
	else
	{
		out[ 0 ] = '\0';
	}
	out[ max - 1 ] = '\0';
}
