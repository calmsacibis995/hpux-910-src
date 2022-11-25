/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: protect.c,v 70.1 92/03/09 15:46:30 ssa Exp $ */
/**************************************************************************
* protect.c
*	Support write protect on terminals.
*	This is done to support clear unprotected only - not operation
*	in protect mode.
**************************************************************************/
#include "ftproc.h"
#include <stdio.h>
#include "ftwindow.h"
#include "features.h"

int	M_write_protect_on = 0;
char	*T_write_protect_on = NULL;
char	*T_write_protect_off = NULL;


int	F_write_protect_tab_wraps_top = 0;

int	F_write_protect_on_sets_auto_scroll_off = 0;
int	F_write_protect_off_sets_auto_scroll_on = 0;
int	F_write_protect_on_moves_cursor_to_unprotected = 0;
/**************************************************************************
* init_write_protect_on
*	Startup - initially off.
**************************************************************************/
init_write_protect_on()
{
	Outwin->write_protect_on = 0;
}
/**************************************************************************
* modes_init_write_protect_on
*	Window "win" went idle - force off.
**************************************************************************/
modes_init_write_protect_on( win )
	FT_WIN	*win;
{
	win->write_protect_on = 0;
}
/**************************************************************************
* fct_write_protect_on
*	ACTION module for 'write_protect_on'.
*	Turn write protect mode on.
**************************************************************************/
fct_write_protect_on()
{
	Outwin->real_xenl = 0;
	Outwin->write_protect_on = 1;
	if ( F_write_protect_on_sets_auto_scroll_off )
		win_se_auto_scroll_off();
	if ( F_write_protect_on_moves_cursor_to_unprotected )
		d_cursor_next_unprotected();
	if ( Outwin->onscreen )
		term_write_protect_on();
}
/**************************************************************************
* fct_write_protect_off
*	ACTION module for 'write_protect off'.
*	Turn write protect mode off.
**************************************************************************/
fct_write_protect_off()
{
	Outwin->real_xenl = 0;
	Outwin->write_protect_on = 0;
	if ( F_write_protect_off_sets_auto_scroll_on )
		win_se_auto_scroll_on();
	if ( Outwin->onscreen )
		term_write_protect_off();
}
/* ftterm.c */
/**************************************************************************
* t_sync_write_protect
*	Syncronize the terminal, if necessary, to the write protect mode
*	"write_protect_on".
**************************************************************************/
t_sync_write_protect( write_protect_on )
	int	write_protect_on;
{
	if ( write_protect_on != M_write_protect_on )
	{
		if ( write_protect_on )
			term_write_protect_on();
		else
			term_write_protect_off();
	}
}
/**************************************************************************
* term_write_protect_on
*	TERMINAL OUTPUT module for 'write_protect_on'.
**************************************************************************/
term_write_protect_on()
{
	if ( T_write_protect_on != NULL )
	{
		term_tputs( T_write_protect_on );
		M_write_protect_on = 1;
		if ( F_write_protect_on_sets_auto_scroll_off )
			term_se_auto_scroll_off();
	}
}
/**************************************************************************
* term_write_protect_off
*	TERMINAL OUTPUT module for 'write_protect_off'.
**************************************************************************/
term_write_protect_off()
{
	if ( T_write_protect_off != NULL )
	{
		term_tputs( T_write_protect_off );
		M_write_protect_on = 0;
		if ( F_write_protect_off_sets_auto_scroll_on )
			term_se_auto_scroll_on();
	}
}
/**************************************************************************
* dec_write_protect_on
*	DECODE module for 'write_protect_on'.
**************************************************************************/
dec_write_protect_on()
{
	fct_write_protect_on();
}
/**************************************************************************
* dec_write_protect_off
*	DECODE module for 'write_protect_off'.
**************************************************************************/
dec_write_protect_off()
{
	fct_write_protect_off();
}
/**************************************************************************
* inst_write_protect_on
*	INSTALL module for 'write_protect_on'.
**************************************************************************/
inst_write_protect_on( str )
	char	*str;
{
	dec_install( "write_protect_on", (UNCHAR *) str, dec_write_protect_on, 
			0, 0,
			(char *) 0 );
}
/**************************************************************************
* inst_write_protect_off
*	INSTALL module for 'write_protect_off'.
**************************************************************************/
inst_write_protect_off( str )
	char	*str;
{
	dec_install( "write_protect_off", (UNCHAR *) str,
			dec_write_protect_off, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* extra_write_protect
*	TERMINAL DESCRIPTION PARSER module for 'write_protect'.
**************************************************************************/
/*ARGSUSED*/
extra_write_protect( name, string, attribute_on_string, attribute_off_string )
	char	*name;
	char	*string;
	char	*attribute_on_string;		/* not used */
	char	*attribute_off_string;		/* not used */
{
	int	match;
	char	*dec_encode();

	match = 1;
	if ( strcmp( name, "write_protect_on" ) == 0 )
	{
		T_write_protect_on = dec_encode( string );
		inst_write_protect_on( T_write_protect_on );
	}
	else if ( strcmp( name, "write_protect_off" ) == 0 )
	{
		T_write_protect_off = dec_encode( string );
		inst_write_protect_off( T_write_protect_off );
	}
	else if ( strcmp( name, "write_protect_tab_wraps_top" ) == 0 )
	{
		F_write_protect_tab_wraps_top = get_optional_value( string, 1 );
	}
	else if ( strcmp( name, "write_protect_on_sets_auto_scroll_off" ) == 0 )
	{
		F_write_protect_on_sets_auto_scroll_off =
						get_optional_value( string, 1 );
	}
	else if ( strcmp( name, "write_protect_off_sets_auto_scroll_on" ) == 0 )
	{
		F_write_protect_off_sets_auto_scroll_on =
						get_optional_value( string, 1 );
	}
	else if ( strcmp( name, "write_protect_on_moves_cursor_to_unprotected" )
		  == 0 )
	{
		F_write_protect_on_moves_cursor_to_unprotected =
						get_optional_value( string, 1 );
	}
	else
		match = 0;
	return( match );
}
