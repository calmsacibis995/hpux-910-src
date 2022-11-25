/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: margins.c,v 70.1 92/03/09 15:45:16 ssa Exp $ */
/**************************************************************************
* margins.c
*	Handle terminals that have settable left and right margins.
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"

#include "ftterm.h"
#include "decode.h"
#include "modes.h"

/**************************************************************************
* init_margins
*	Initialize the settings on startup.
**************************************************************************/
init_margins()
{
						/* full screen width */
	Outwin->set_margin_left = 	0; 
	Outwin->set_margin_right =	Col_right; 
						/* same */
	Outwin->wrap_margin_left =	Outwin->set_margin_left; 
	Outwin->wrap_margin_right =	Outwin->set_margin_right; 
}
/**************************************************************************
* modes_idle_init_margins
*	Modes_init - window wind idle.
*	Set modes so that modes on will go back to full width margins.
**************************************************************************/
modes_idle_init_margins( win )
	FT_WIN	*win;
{
					/* set to full screen width */
	win->set_margin_left = 0;
	win->set_margin_right = win->col_right;
					/* full screen width unless double */
	win->wrap_margin_left =  0;
	win->wrap_margin_right = win->col_right_line;
}
/**************************************************************************
* win_se_margins_full
**************************************************************************/
win_se_margins_full()			/* columns_wide_on or columns_wide_off
					** has presumably put the margins back
					** to full width.
					** Or soft reset.
					*/
{
					/* set to full screen width */
	Outwin->set_margin_left =   0;
	Outwin->set_margin_right =  Outwin->col_right;
					/* full screen width unless double */
	Outwin->wrap_margin_left =  0;
	Outwin->wrap_margin_right = Outwin->col_right_line;
}
/**************************************************************************
* d_set_wrap_margins
**************************************************************************/
d_set_wrap_margins()			/* col_right_line has changed in
					** d_set_col_right_line because of going
					** on or off a double line.
					** Or set_margin_right or 
					** set_margin_left has changed.
					*/
{
	if ( Outwin->set_margin_right <= Outwin->col_right_line )
		Outwin->wrap_margin_right = Outwin->set_margin_right;
	else
		Outwin->wrap_margin_right = Outwin->col_right_line;

	if ( Outwin->set_margin_left <= Outwin->col_right_line )
		Outwin->wrap_margin_left = Outwin->set_margin_left;
	else
		Outwin->wrap_margin_left = Outwin->col_right_line;
}
/**************************************************************************
* dec_set_margins
*	DECODE module for 'set_margins'.
*	Set left and right margins.
**************************************************************************/
#define SET_MARGIN_RIGHT_TO_MAXIMUM 1000
/*ARGSUSED*/
dec_set_margins( not_used, parm_ptr, parms_valid, parm )
	int	not_used;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	set_margin_left;
	int	set_margin_right;

	if ( parms_valid & 1 )
		set_margin_left = parm[ 0 ];
	else
		set_margin_left = 0;
	if ( parms_valid & 2 )
		set_margin_right = parm[ 1 ];
	else
		set_margin_right = SET_MARGIN_RIGHT_TO_MAXIMUM;
	fct_set_margins( set_margin_left, set_margin_right );
}
/**************************************************************************
* inst_set_margins
*	INSTALL module for 'set_margins'.
**************************************************************************/
inst_set_margins( str )
	char	*str;
{
	dec_install( "set_margins", (UNCHAR *) str,
		dec_set_margins, 0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_set_margins
*	ACTION module for 'set_margins'.
**************************************************************************/
fct_set_margins( set_margin_left, set_margin_right )
	int	set_margin_left;
	int	set_margin_right;
{ 
	int	max;

	if ( Outwin->columns_wide_mode_on )
		max = Col_right_wide;
	else
		max = Col_right;
	if ( set_margin_right > max )
		set_margin_right = max;
	if ( set_margin_left >= set_margin_right )
		set_margin_left = set_margin_right - 1;
	if ( set_margin_left < 0 )
		set_margin_left = 0;
	if ( set_margin_right <= set_margin_left )
		set_margin_right = set_margin_left + 1;
	Outwin->set_margin_left = set_margin_left;
	Outwin->set_margin_right = set_margin_right;
	d_set_wrap_margins();
	if ( Outwin->onscreen )
		term_set_margins( Outwin->set_margin_left,
				  Outwin->set_margin_right );
}
char	*T_set_margins = NULL;
int	M_set_margin_left = 0;
int	M_set_margin_right = 0;
/**************************************************************************
* term_init_set_margins
*	Initalize the remembered left and right margins currently in the
*	terminal.
**************************************************************************/
term_init_set_margins( set_margin_left, set_margin_right )
	int	set_margin_left;
	int	set_margin_right;
{
	M_set_margin_left = set_margin_left;
	M_set_margin_right = set_margin_right;
}
/**************************************************************************
* term_se_set_margins_normal
*	An event has occurred that had the side effect of setting the
*	margins of the terminal to their default state.
**************************************************************************/
term_se_set_margins_normal()
{
	M_set_margin_left = 0;
	if ( M_columns_wide_on )
		M_set_margin_right = Col_right_wide;
	else
		M_set_margin_right = Col_right;
}
/**************************************************************************
* term_set_margins
*	TERMINAL OUTPUT module for 'set_margins'.
**************************************************************************/
term_set_margins( set_margin_left, set_margin_right )
	int	set_margin_left;
	int	set_margin_right;
{
	char	*p;
	char	*my_tparm();
	int	parm[ 2 ];
	char	*string_parm[ 1 ];		/* not used */
	char	buff[ 80 ];

	if ( T_set_margins != NULL )
	{
		parm[ 0 ] = set_margin_left;
		parm[ 1 ] = set_margin_right;
		p = my_tparm( T_set_margins, parm, string_parm, -1 );
		my_tputs( p, 1 );
	}
	else
	{
		sprintf( buff, "SET MARGINS %d %d %d %d\007", 
			set_margin_left, set_margin_right,
			M_set_margin_left, M_set_margin_right );
		my_tputs( buff, 1 );
	}
	M_set_margin_left = set_margin_left;
	M_set_margin_right = set_margin_right;
}
/**************************************************************************
* t_sync_margins_full
*	modes_off
*	Syncronize the terminal, if necessary, to full width margins
**************************************************************************/
t_sync_margins_full()
{
	if ( M_columns_wide_on )
		t_sync_margins( 0, Col_right_wide );
	else
		t_sync_margins( 0, Col_right );
}
/**************************************************************************
* t_sync_margins_outwin
*	modes_on - put terminal back ot programmed margins.
**************************************************************************/
t_sync_margins_outwin()
{
	t_sync_margins( Outwin->set_margin_left, Outwin->set_margin_right );
}
/**************************************************************************
* t_sync_margins
*	Syncronize the terminal, if necessary, to have margins at
*	"set_margin_left" and "set_margin_right".
**************************************************************************/
t_sync_margins( set_margin_left, set_margin_right )
	int	set_margin_left;
	int	set_margin_right;
{
	if (  ( set_margin_left != M_set_margin_left )
	   || ( set_margin_right != M_set_margin_right ) )
	{
		term_set_margins( set_margin_left, set_margin_right );
		return( 1 );
	}
	return( 0 );
}
/**************************************************************************
* extra_margins
*	TERMINAL DESCRIPTION PARSER module for 'set_margins'.
**************************************************************************/
/*ARGSUSED*/
extra_margins( name, string, attribute_on_string, attribute_off_string )
	char	*name;
	char	*string;
	char	*attribute_on_string;		/* not used */
	char	*attribute_off_string;		/* not used */
{
	int	match;
	char	*dec_encode();

	match = 1;
	if ( strcmp( name, "set_margins" ) == 0 )
	{
		T_set_margins = dec_encode( string );
		inst_set_margins( T_set_margins );
	}
	else
		match = 0;
	return( match );
}
