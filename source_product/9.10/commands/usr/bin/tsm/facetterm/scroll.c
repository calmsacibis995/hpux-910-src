/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: scroll.c,v 70.1 92/03/09 15:46:49 ssa Exp $ */
/**************************************************************************
* scroll.c
*	Handle terminals that can turn auto scroll off.
**************************************************************************/
#include "ftproc.h"
#include <stdio.h>
#include "ftwindow.h"
#include "features.h"

int	M_auto_scroll_on = 1;
char	*T_auto_scroll_on = NULL;
char	*T_auto_scroll_off = NULL;


int	F_auto_scroll_off_wraps_top = 0;
/**************************************************************************
* init_auto_scroll_on
*	Startup default to on.
**************************************************************************/
init_auto_scroll_on()
{
	Outwin->auto_scroll_on = 1;
}
/**************************************************************************
* modes_init_auto_scroll_on
*	Window "win" went idle - default to on.
**************************************************************************/
modes_init_auto_scroll_on( win )
	FT_WIN	*win;
{
	win->auto_scroll_on = 1;
}
/**************************************************************************
* win_se_auto_scroll_on
*	An event has occurred that had the side effect of setting the
*	auto scroll mode of the window to "on".
**************************************************************************/
win_se_auto_scroll_on()
{
	Outwin->auto_scroll_on = 1;
}
/**************************************************************************
* win_se_auto_scroll_off
*	An event has occurred that had the side effect of setting the
*	auto scroll mode of the window to "off".
**************************************************************************/
win_se_auto_scroll_off()
{
	Outwin->auto_scroll_on = 0;
}
/**************************************************************************
* fct_auto_scroll_on
*	ACTION module for 'auto_scroll_on'.
*	Turn auto scroll mode on.
**************************************************************************/
fct_auto_scroll_on()
{
	Outwin->real_xenl = 0;
	Outwin->auto_scroll_on = 1;
	if ( Outwin->onscreen )
		term_auto_scroll_on();
}
/**************************************************************************
* fct_auto_scroll_off
*	ACTION module for 'auto_scroll_off'.
*	Turn auto scroll mode off.
**************************************************************************/
fct_auto_scroll_off()
{
	Outwin->real_xenl = 0;
	Outwin->auto_scroll_on = 0;
	if ( Outwin->onscreen )
		term_auto_scroll_off();
}
/* ftterm.c */
/**************************************************************************
* t_sync_auto_scroll
*	Syncronize the terminal, if necessary, to the auto scroll mode 
*	"auto_scroll_on".
**************************************************************************/
t_sync_auto_scroll( auto_scroll_on )
	int	auto_scroll_on;
{
	if ( auto_scroll_on != M_auto_scroll_on )
	{
		if ( auto_scroll_on )
			term_auto_scroll_on();
		else
			term_auto_scroll_off();
	}
}
/**************************************************************************
* term_auto_scroll_on
*	TERMINAL OUTPUT module for 'auto_scroll_on'.
**************************************************************************/
term_auto_scroll_on()
{
	if ( T_auto_scroll_on != NULL )
	{
		term_tputs( T_auto_scroll_on );
		M_auto_scroll_on = 1;
	}
}
/**************************************************************************
* term_auto_scroll_off
*	TERMINAL OUTPUT module for 'auto_scroll_off'.
**************************************************************************/
term_auto_scroll_off()
{
	if ( T_auto_scroll_off != NULL )
	{
		term_tputs( T_auto_scroll_off );
		M_auto_scroll_on = 0;
	}
}
/**************************************************************************
* term_se_auto_scroll_on
*	An event has occurred that had the side effect of setting the
*	auto scroll mode of the terminal to 'on'.
**************************************************************************/
term_se_auto_scroll_on()
{
	M_auto_scroll_on = 1;
}
/**************************************************************************
* term_se_auto_scroll_off
*	An event has occurred that had the side effect of setting the
*	auto scroll mode of the terminal to 'off'.
**************************************************************************/
term_se_auto_scroll_off()
{
	M_auto_scroll_on = 0;
}
/**************************************************************************
* dec_auto_scroll_on
*	DECODE module for 'auto_scroll_on'.
**************************************************************************/
dec_auto_scroll_on()
{
	fct_auto_scroll_on();
}
/**************************************************************************
* dec_auto_scroll_off
*	DECODE module for 'auto_scroll_off'.
**************************************************************************/
dec_auto_scroll_off()
{
	fct_auto_scroll_off();
}
/**************************************************************************
* inst_auto_scroll_on
*	INSTALL module for 'auto_scroll_on'.
**************************************************************************/
inst_auto_scroll_on( str )
	char	*str;
{
	dec_install( "auto_scroll_on", (UNCHAR *) str, dec_auto_scroll_on, 
			0, 0,
			(char *) 0 );
}
/**************************************************************************
* inst_auto_scroll_off
*	INSTALL module for 'auto_scroll_off'.
**************************************************************************/
inst_auto_scroll_off( str )
	char	*str;
{
	dec_install( "auto_scroll_off", (UNCHAR *) str, dec_auto_scroll_off, 
			0, 0,
			(char *) 0 );
}
/**************************************************************************
* extra_auto_scroll
*	TERMINAL DESCRIPTION PARSER module for 'auto_scroll'.
**************************************************************************/
/*ARGSUSED*/
extra_auto_scroll( name, string, attribute_on_string, attribute_off_string )
	char	*name;
	char	*string;
	char	*attribute_on_string;		/* not used */
	char	*attribute_off_string;		/* not used */
{
	int	match;
	char	*dec_encode();

	match = 1;
	if ( strcmp( name, "auto_scroll_on" ) == 0 )
	{
		T_auto_scroll_on = dec_encode( string );
		inst_auto_scroll_on( T_auto_scroll_on );
	}
	else if ( strcmp( name, "auto_scroll_off" ) == 0 )
	{
		T_auto_scroll_off = dec_encode( string );
		inst_auto_scroll_off( T_auto_scroll_off );
	}
	else if ( strcmp( name, "auto_scroll_off_wraps_top" ) == 0 )
	{
		F_auto_scroll_off_wraps_top = get_optional_value( string, 1 );
	}
	else
		match = 0;
	return( match );
}
