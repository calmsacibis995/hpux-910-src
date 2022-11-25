/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: onstatus.c,v 70.1 92/03/09 15:45:45 ssa Exp $ */
/**************************************************************************
* onstatus.c
*	Support terminals whose status line looks like a 25th line of the
*	terminal.  I.E. one uses cursor positioning, clear, etc on it.
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"
#include "ftterm.h"

char	*T_onstatus = NULL;
char	*T_out_onstatus = NULL;
char	*T_onstatus_clear = NULL;

/**************************************************************************
* init_windows_onstatus
*	Startup
**************************************************************************/
init_windows_onstatus()
{
	Outwin->onstatus = 0;
}
/**************************************************************************
* modes_init_onstatus
*	window "win" went idle.
**************************************************************************/
modes_init_onstatus( win )
	FT_WIN	*win;
{
	win->onstatus = 0;
}
/**************************************************************************
* dec_onstatus
*	DECODE module for 'onstatus'.
**************************************************************************/
/*ARGSUSED*/
dec_onstatus( func_parm, parm_ptr, parms_valid, parm, string_parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
{
	int	col;

	if ( parms_valid & 1 )
		col = parm[ 0 ];
	else
		col = 0;
	fct_onstatus( col );
}
/**************************************************************************
* inst_onstatus
*	INSTALL module for 'onstatus'.
**************************************************************************/
inst_onstatus( str )
	char	*str;
{
	dec_install( "onstatus", (UNCHAR *) str, dec_onstatus, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_onstatus
*	ACTION module for 'onstatus'.
*	"col" is the column number on the onstatus line.
**************************************************************************/
fct_onstatus( col )
{
	int	status_line;

	status_line = Outwin->display_row_bottom + 1;
	if ( status_line > Row_bottom_max )
	{
		term_beep();
		return;
	}
	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	if ( col < 0 )
		col = 0;
	else if ( col > Outwin->col_right )
		col = Outwin->col_right;
	Outwin->onstatus = 1;
	Outwin->row = status_line;
	Outwin->col = col;
	if ( Outwin->onscreen )
		term_onstatus( col );
}
/**************************************************************************
* term_out_onstatus
*	Internally generated request.
*	Position to column "col" on the status line.
**************************************************************************/
extern	char	*my_tparm();
term_out_onstatus( col )
	int	col;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];

	if ( T_out_onstatus != NULL )
	{
		parm[ 0 ] = col;
		p = my_tparm( T_out_onstatus, parm, string_parm, -1 );
		term_tputs( p );
	}
	else
		term_onstatus( col );
}
/**************************************************************************
* term_onstatus
*	TERMINAL OUTPUT module for 'onstatus'.
*	Position to column "col".
**************************************************************************/
term_onstatus( col )
	int	col;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];

	if ( T_onstatus != NULL )
	{
		parm[ 0 ] = col;
		p = my_tparm( T_onstatus, parm, string_parm, -1 );
		term_tputs( p );
	}
}
/**************************************************************************
* t_sync_onstatus_quit
*	Return onstatus status line to default on shutdown.
**************************************************************************/
t_sync_onstatus_quit()
{
	if ( F_has_status_line )
	{
		term_out_onstatus( 0 );
		out_term_clear_line();
		term_pos( Row_bottom_terminal, 0 );
	}
}
/**************************************************************************
* onstatus_load
*	Load the string "string" onto the windows numbered "start_winno"
*	to "end_winno" after possibly instantiating the string with
*	the window number.
**************************************************************************/
onstatus_load( start_winno, end_winno, string )
	int	start_winno;
	int	end_winno;
	char	*string;
{
	int	winno;
	FT_WIN	*win;
	int	status_line;
	char	buff[ MAX_STATUS_LINE_CHARS + 1 ];
	char	buff2[ MAX_STATUS_LINE_CHARS + 30 ];

	if ( F_has_status_line == 0 )
		return( -1 );
	if ( T_onstatus == NULL )
		return( -1 );
	status_line = Outwin->display_row_bottom + 1;
	if ( status_line > Row_bottom_max )
		return( -1 );
	for ( winno = start_winno; winno <= end_winno; winno++ )
	{
		strncpy( buff, string, MAX_STATUS_LINE_CHARS );
		buff[ MAX_STATUS_LINE_CHARS ] = '\0';
		sprintf( buff2, buff, winno + 1 );
		buff2[ MAX_STATUS_LINE_CHARS ] = '\0';
		win = Wininfo[ winno ];
		d_onstatus_load( win, status_line, buff2 );
	}
	return( 0 );
}
/**************************************************************************
* win_se_onstatus_clear
*	An event has occurred that had the side effect of
*	clearing the onstatus status line of the terminal.
**************************************************************************/
win_se_onstatus_clear()
{
	int	status_line;

	if ( F_has_status_line )
	{
		status_line = Outwin->display_row_bottom + 1;
		if ( status_line <= Row_bottom_max )
		{
			d_se_onstatus_clear( status_line );
		}
	}
}
/**************************************************************************
* dec_onstatus_clear
*	DECODE module for 'onstatus_clear'.
*	Clears the onstatus status line.
**************************************************************************/
dec_onstatus_clear()
{
	fct_onstatus_clear();
}
/**************************************************************************
* inst_onstatus_clear
*	INSTALL module for 'onstatus_clear'.
**************************************************************************/
inst_onstatus_clear( str )
	char	*str;
{
	dec_install( "onstatus_clear", (UNCHAR *) str, dec_onstatus_clear, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* fct_onstatus_clear
*	ACTION module for 'onstatus_clear'.
**************************************************************************/
fct_onstatus_clear()
{
	win_se_onstatus_clear();
	if ( Outwin->onscreen )
		term_onstatus_clear();
}
/**************************************************************************
* term_onstatus_clear
*	TERMINAL OUTPUT module for 'onstatus_clear'.
**************************************************************************/
term_onstatus_clear()
{
	if ( T_onstatus_clear != NULL )
		term_tputs( T_onstatus_clear );
}

char *dec_encode();
/**************************************************************************
* extra_onstatus
*	TERMINAL DESCRIPTION PARSER module for 'onstatus...'.
**************************************************************************/
/*ARGSUSED*/
extra_onstatus( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "onstatus" ) == 0 )
	{
		T_onstatus = dec_encode( string );
		inst_onstatus( T_onstatus );
		T_out_onstatus = T_onstatus;
		F_has_status_line = 1;
	}
	else if ( strcmp( buff, "out_onstatus" ) == 0 )
	{
		T_out_onstatus = dec_encode( string );
	}
	else if ( strcmp( buff, "onstatus_clear" ) == 0 )
	{
		T_onstatus_clear = dec_encode( string );
		inst_onstatus_clear( T_onstatus_clear );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
