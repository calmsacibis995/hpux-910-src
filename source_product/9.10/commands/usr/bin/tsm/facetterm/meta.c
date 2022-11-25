/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: meta.c,v 70.1 92/03/09 15:45:21 ssa Exp $ */
/******************************************************************
* meta - handle sequences from keyshell - not used.
******************************************************************/
#include "person.h"
#include "meta_p.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "scroll.h"

#include "meta.h"
int	sT_meta_roll_count = { 0 };

char	*sT_out_meta_roll_down = { (char *) 0 };
char	*sT_out_meta_roll_up = { (char *) 0 };
char	*sT_in_meta_roll_cancel = { (char *) 0 };

/**************************************************************************
* dec_meta_roll_down
*	DECODE module for 'meta_roll_down'.
**************************************************************************/
/*ARGSUSED*/
dec_meta_roll_down( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	fct_meta_roll_down();
}
/**************************************************************************
* inst_meta_roll_down
*	INSTALL module for 'meta_roll_down'.
**************************************************************************/
inst_meta_roll_down( str )
	char	*str;
{
	dec_install( "meta_roll_down", (UNCHAR *) str, dec_meta_roll_down, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_meta_roll_up
*	DECODE module for 'meta_roll_up'.
**************************************************************************/
/*ARGSUSED*/
dec_meta_roll_up( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	fct_meta_roll_up();
}
/**************************************************************************
* inst_meta_roll_up
*	INSTALL module for 'meta_roll_up'.
**************************************************************************/
inst_meta_roll_up( str )
	char	*str;
{
	dec_install( "meta_roll_up", (UNCHAR *) str, dec_meta_roll_up, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_meta_cursor_home_down
*	DECODE module for 'meta_cursor_home_down'.
**************************************************************************/
/*ARGSUSED*/
dec_meta_cursor_home_down( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	fct_meta_cursor_home_down();
}
/**************************************************************************
* inst_meta_cursor_home_down
*	INSTALL module for 'meta_cursor_home_down'.
**************************************************************************/
inst_meta_cursor_home_down( str )
	char	*str;
{
	dec_install( "meta_cursor_home_down", (UNCHAR *) str, 
		dec_meta_cursor_home_down, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* fct_meta_roll_down
*	ACTION module for 'meta_roll_down'.
**************************************************************************/
fct_meta_roll_down()
{ 
	if ( outwin_is_curwin() == 0 )
	{
		term_beep();
		return;
	}
	if ( Outwin->fullscreen  == 0 )
	{
		term_beep();
		return;
	}
	if ( oT_meta_roll_count >= Rows_scroll_memory )
	{
		term_beep();
		return;
	}
	oT_meta_roll_count++;
	term_out_meta_roll_down();
}
/**************************************************************************
* fct_meta_roll_up
*	ACTION module for 'meta_roll_up'.
**************************************************************************/
fct_meta_roll_up()
{ 
	if ( outwin_is_curwin() == 0 )
	{
		term_beep();
		return;
	}
	if ( Outwin->fullscreen  == 0 )
	{
		term_beep();
		return;
	}
	/******************************************************************
	* roll up and decrement count - ignore if not rolled down.
	******************************************************************/
	if ( mT_meta_roll_count > 0 )
	{
		mT_meta_roll_count--;
		term_out_meta_roll_up();
	}
}
/**************************************************************************
* fct_meta_cursor_home_down
*	ACTION module for 'meta_cursor_home_down'.
**************************************************************************/
fct_meta_cursor_home_down()
{ 
	if ( outwin_is_curwin() && Outwin->fullscreen )
	{
		while ( mT_meta_roll_count > 0 )
		{
			mT_meta_roll_count--;
			term_out_meta_roll_up();
		}
	}
	fct_cursor_home_down();
}
/**************************************************************************
* term_out_meta_roll_down
*	TERMINAL OUTPUT module for 'meta_roll_down' for internally generated
*	requests.
**************************************************************************/
term_out_meta_roll_down()
{
	if ( mT_out_meta_roll_down != (char *) 0 );
		my_tputs( mT_out_meta_roll_down, 1 );
}
/**************************************************************************
* term_out_meta_roll_up
*	TERMINAL OUTPUT module for 'meta_roll_up' for internally generated
*	requests.
**************************************************************************/
term_out_meta_roll_up()
{
	if ( mT_out_meta_roll_up != (char *) 0 );
		my_tputs( mT_out_meta_roll_up, 1 );
}
/**************************************************************************
* t_meta_roll_undo
*	Undo roll down if active - ^W pressed, etc.
*	Assumes in window mode
**************************************************************************/
t_meta_roll_undo()
{
	if ( mT_meta_roll_count )
	{
		while ( mT_meta_roll_count > 0 )
		{
			mT_meta_roll_count--;
			term_out_meta_roll_up();
		}
		if ( mT_in_meta_roll_cancel != (char *) 0 )
			paste_string_to_winno( Outwin->number,
					       mT_in_meta_roll_cancel );
	}
}
/**************************************************************************
* t_se_meta_roll_cancel
*	Some action has cancelled the effect of roll down - e.g. clear screen.
**************************************************************************/
t_se_meta_roll_cancel()
{
	mT_meta_roll_count = 0;
}
/**************************************************************************
* extra_meta_roll
*	TERMINAL DESCRIPTION PARSER module for '..meta..'.
**************************************************************************/
/*ARGSUSED*/
extra_meta_roll( name, string, attribute_on_string, attribute_off_string )
	char	*name;
	char	*string;
	char	*attribute_on_string;		/* not used */
	char	*attribute_off_string;		/* not used */
{
	int	match;
	char	*dec_encode();
	char	*encoded;

	match = 1;
	if ( strcmp( name, "meta_roll_down" ) == 0 )
	{
		encoded = dec_encode( string );
		inst_meta_roll_down( encoded );
	}
	else if ( strcmp( name, "out_meta_roll_down" ) == 0 )
	{
		xT_out_meta_roll_down = dec_encode( string );
	}
	else if ( strcmp( name, "meta_roll_up" ) == 0 )
	{
		encoded = dec_encode( string );
		inst_meta_roll_up( encoded );
	}
	else if ( strcmp( name, "out_meta_roll_up" ) == 0 )
	{
		xT_out_meta_roll_up = dec_encode( string );
	}
	else if ( strcmp( name, "meta_cursor_home_down" ) == 0 )
	{
		encoded = dec_encode( string );
		inst_meta_cursor_home_down( encoded );
	}
	else if ( strcmp( name, "in_meta_roll_cancel" ) == 0 )
	{
		xT_in_meta_roll_cancel = dec_encode( string );
	}
	else
		match = 0;
	return( match );
}
