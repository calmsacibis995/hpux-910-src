/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: activecom.c,v 66.7 90/09/20 12:03:17 kb Exp $ */
#include "facetwin.h"
#include "wins.h"
#include "winactive.h"
		/**********************************************************
		* Array tracking whether window is open or not
		**********************************************************/
int	Window_active[ USR_WINDS ] = { 0 };
		/**********************************************************
		* Window titles.
		**********************************************************/
char	Exec_list[ USR_WINDS ][ MAXPROTOARGS + 1 ] = { "" };

#ifndef FACET
#include "printer.h"
char	*Text_printer_program_name =
"(Printer)";
#endif

/**************************************************************************
* set_window_active
*	Window number "winno" 0-9 has opened.
**************************************************************************/
set_window_active( winno )
	int	winno;
{
	Window_active[ winno ] = 1;
}
/**************************************************************************
* clear_window_active
*	Window number "winno" 0-9 has closed.
**************************************************************************/
clear_window_active( winno )
	int	winno;
{
	Window_active[ winno ] = 0;
}
/**************************************************************************
* check_window_active
*	Return 0 if window number "winno" 0-9 is active.
*	Return 1 otherwise.
**************************************************************************/
check_window_active( winno )
	int	winno;
{
	return( Window_active[ winno ] );
}
/**************************************************************************
* remember_exec_list
*	Store the window title "s" for the window number "winno" 0-9.
**************************************************************************/
remember_exec_list( winno, s )
	int	winno;
	char	*s;
{
	strncpy( Exec_list[ winno ], s, MAXPROTOARGS );
	Exec_list[ winno ][ MAXPROTOARGS ] = '\0';
}
/**************************************************************************
* forget_exec_list
*	Erase the window title for window number "winno" 0-9.
**************************************************************************/
forget_exec_list( winno )
	int	winno;
{
	Exec_list[ winno ][ 0 ] = '\0';
}
/**************************************************************************
* get_exec_list_ptr
*	Return a pointer to the title of window number "winno" 0-9.
**************************************************************************/
char *
get_exec_list_ptr( winno )
	int	winno;
{
	return( Exec_list[ winno ] );
}
/**************************************************************************
* get_program_name
*	Return the window title of the specified window in "output_string".
*	The window number is in "input_string" as an ascii string of 
*	digits:
*		"1" for window 1 to
*		"9" for window 9 and 
*		"10" or "0" for window 10.
*	If "input_string" does not contain one of those forms,
*	use the window number "winno" 0-9.
*	If the window is in "print mode" and is closed or does not have
*	a title, return "(Printer)".
**************************************************************************/
get_program_name( winno, input_string, output_string )
	int	winno;
	char	*input_string;
	char	*output_string;
{
	char	c;
	int	winno_req;

	c = *input_string;
	if ( c >= '0' && c <= '9' )
	{
		if (  ( c == '1' )
		   && ( input_string[ 1 ] == '0') )
		{
			winno_req = 9;
		}
		else
			winno_req = c - '0' - 1;
	}
	else
		winno_req = winno;
#ifndef FACET
	if (  Window_printer[ winno_req ] 
	   && (  ( Window_active[ winno_req ] == 0 )
	      || ( *Exec_list[ winno_req ] == '\0'  ) 
	      ) 
	   )
	{
		strcpy( output_string, Text_printer_program_name );
		return;
	}
#endif
	strcpy( output_string, Exec_list[ winno_req ] );
}
/**************************************************************************
* build_active_list
*	 Retrieve a blank separated list of the window numbers of windows
*	 that are active or are in print mode.
**************************************************************************/
build_active_list( string )
	char	*string;
{
	int	winno;
	char	*out;

	out = string;
	for ( winno = 0; winno < Wins; winno++ )
	{
		if( Window_active[ winno ]
#ifndef FACET
		||  Window_printer[ winno ]
#endif
		  )
		{
			if ( winno < 9 )
				*out++ = '1' + winno;
			else
			{
				*out++ = '1';
				*out++ = '0';
			}
			*out++ = ' ';
		}
	}
	if ( out != string )
		out--;
	*out = '\0';
}
/**************************************************************************
* build_idle_list
*	Retrieve a blank separated list of the window numbers of windows
*	that are not open and are not in print mode.
**************************************************************************/
build_idle_list( string )
	char	*string;
{
	int	winno;
	char	*out;

	out = string;
	for ( winno = 0; winno < Wins; winno++ )
	{
		if( Window_active[ winno ] == 0 )
		{
#ifndef FACET
			if ( Window_printer[ winno ] )
				continue;
#endif
			if ( winno < 9 )
				*out++ = '1' + winno;
			else
			{
				*out++ = '1';
				*out++ = '0';
			}
			*out++ = ' ';
		}
	}
	if ( out != string )
		out--;
	*out = '\0';
}
/**************************************************************************
* window_closed_check_all
*	Window number "winno" 0-9 has closed. ( Ignore if less than 0 ).
*	Return 1 if all windows are closed. 0 otherwise.
**************************************************************************/
window_closed_check_all( winno )	/* PSEUDOTTY */
	int	winno;
{
	int	i;

	if ( winno >= 0 )
		Window_active[ winno ] = 0;
	for ( i = 0; i < Wins; i++ )
	{
		if ( Window_active[ i ] )
			return( 0 );		/* no */
	}
	return( 1 );				/* yes */
}
