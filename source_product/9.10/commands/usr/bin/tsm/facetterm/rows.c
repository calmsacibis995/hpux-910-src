/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: rows.c,v 70.1 92/03/09 15:46:37 ssa Exp $ */
/**************************************************************************
* rows.c
*	Support terminals that can change the number of rows on the screen.
**************************************************************************/
#include <stdio.h>
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"

int	M_rows_changeno = 0;
int	F_rows_change_does_clear_screen = 0;

#include "ftterm.h"
#define MAX_ROWS_CHANGE 5
int	Rows_changeno = 0;
int	Rows_change_display_rows[ MAX_ROWS_CHANGE ] = { 0 };
char	*T_rows_change[ MAX_ROWS_CHANGE ][ 2 ] = { NULL };
int	Rows_change_no_status[ MAX_ROWS_CHANGE] = { 0 };

int	F_has_extra_data_row = 0;
int	M_extra_data_row = 0;
int	F_extra_data_row_does_clear_screen = 0;
char	*T_extra_data_row[ 2 ][ MAX_ROWS_CHANGE ] = { NULL };
int	Extra_data_row_no_status[ 2 ] = { 0 };

int	F_rows_change_se_on_switch_only = 0;
int	F_rows_change_t_auto_wrap_off = 0;
int	F_rows_change_t_auto_wrap_on = 0;
int	F_rows_change_switch_page_number_0 = 0;
int	F_rows_change_clears_pages = 0;

int	F_rows_change_sets_cursor_on = 0;
int	F_rows_change_resets_insert_mode = 0;
int	F_rows_change_resets_appl_keypad_mode = 0;
int	F_rows_change_resets_keypad_xmit = 0;
int	F_rows_change_resets_scroll_region = 0;
int	F_rows_change_resets_character_set = 0;
FTCHAR	F_rows_change_attributes_off = 0;
int	F_rows_change_resets_save_cursor = 0;
int	F_rows_change_resets_origin_mode = 0;
long	F_rows_change_sets_mode_on = 0;
long	F_rows_change_sets_mode_off = 0;

char	*T_rows_change_clears_perwindow = { (char *) 0 };

int	F_extra_data_row_se_on_switch_only = 0;
int	F_extra_data_row_t_auto_wrap_off = 0;
int	F_extra_data_row_t_auto_wrap_on = 0;
int	F_extra_data_row_switch_page_number_0 = 0;
int	F_extra_data_row_clears_pages = 0;

int	F_extra_data_row_sets_cursor_on = 0;
int	F_extra_data_row_resets_insert_mode = 0;
int	F_extra_data_row_resets_appl_keypad_mode = 0;
int	F_extra_data_row_resets_keypad_xmit = 0;
int	F_extra_data_row_resets_scroll_region = 0;
int	F_extra_data_row_resets_character_set = 0;
FTCHAR	F_extra_data_row_attributes_off = 0;
int	F_extra_data_row_resets_save_cursor = 0;
int	F_extra_data_row_resets_origin_mode = 0;
long	F_extra_data_row_sets_mode_on = 0;
long	F_extra_data_row_sets_mode_off = 0;

char	*T_extra_data_row_clears_perwindow = { (char *) 0 };

/**************************************************************************
* init_windows_rows_change
*	Startup at default number of rows.
**************************************************************************/
init_windows_rows_change()
{
	Outwin->rows_changeno = 0;
	Outwin->extra_data_row = 0;
}
/**************************************************************************
* get_rows_change_max
*	Return the max number of rows of any configuration for allocation.
**************************************************************************/
get_rows_change_max()
{
	int	i;
	int	max;

	max = 0;
	for ( i = 0; i < Rows_changeno; i++ )
	{
		if ( Rows_change_display_rows[ i ] > max )
		{
			max = Rows_change_display_rows[ i ];
		}
	}
	if ( F_has_extra_data_row )
		max++;
	return( max );
}
/**************************************************************************
* dec_rows_change
*	DECODE module for 'rows_change'.
*	Change the number of rows on the terminal.
*	"rows_changeno" specifies the index of the 'rows_change' sequence.
**************************************************************************/
/*ARGSUSED*/
dec_rows_change( rows_changeno, parm_ptr )
	int	rows_changeno;
	char	*parm_ptr;
{
	fct_rows_change( rows_changeno );
}
/**************************************************************************
* inst_rows_change
*	INSTALL module for 'rows_change'.
*	"rows_changeno" specifies the index of the 'rows_change' sequence.
**************************************************************************/
inst_rows_change( str, rows_changeno )
	char	*str;
	int	rows_changeno;
{
	dec_install( "rows_change", (UNCHAR *) str, 
			dec_rows_change, rows_changeno, 0,
			(char *) 0 );
}
/**************************************************************************
* fct_rows_change
*	ACTION module for 'rows_change'.
*	"rows_changeno" specifies the index of the 'rows_change' sequence.
**************************************************************************/
fct_rows_change( rows_changeno )
	int	rows_changeno;
{
	int	rows;
					/* if not switching and 
					   rows_change has no effect if 
					   it was already on then ignore it.
					*/
	if (  ( rows_changeno == Outwin->rows_changeno ) 
	   && F_rows_change_se_on_switch_only )
		return;
	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	Outwin->rows_changeno = rows_changeno;
	rows = Rows_change_display_rows[ rows_changeno ]
	     + Outwin->extra_data_row;
	change_display_rows( rows,
			     0, F_rows_change_does_clear_screen );
	if ( F_rows_change_t_auto_wrap_off )
		win_se_auto_wrap_off();
	if ( F_rows_change_does_clear_screen )
	{
		Outwin->xenl = 0;
		Outwin->real_xenl = 0;
		d_blankwin();
		Outwin->row = 0;
		Outwin->col = 0;
		b_cursor_home();			/* pan buffer to top */
	}
	else
	{
		if ( Outwin->row > Outwin->display_row_bottom )
			Outwin->row = Outwin->display_row_bottom;
	}
	win_se_rows_change_soft_reset();
	if ( T_rows_change_clears_perwindow != (char *) 0 )
		win_se_perwindow_clear_string( T_rows_change_clears_perwindow,
					       Outwin->number );
	if ( Rows_change_no_status[ rows_changeno ] )
		win_se_no_status();
	if ( outwin_is_curwin() )
	{
		term_rows_change( rows_changeno );
		t_cursor();
	}
	if ( Outwin->onscreen )
	{
		if (  ( Outwin->fullscreen == 0 )
		   && F_rows_change_does_clear_screen )
		{
			split_screen_incompatible( 
				F_rows_change_does_clear_screen );
		}
	}
}
/**************************************************************************
* t_sync_rows_change
*	Syncronize the terminal, if necessary, to the number of rows.
*	"rows_changeno" specifies the index of the 'rows_change' sequence.
**************************************************************************/
t_sync_rows_change( rows_changeno, extra_data_row )
	int	rows_changeno;
	int	extra_data_row;
{
	int	changed;

	changed = 0;
	if ( rows_changeno != M_rows_changeno )
	{
		term_rows_change( rows_changeno );
		changed = 1;
	}
	if ( extra_data_row != M_extra_data_row )
	{
		term_extra_data_row( extra_data_row );
		changed = 1;
	}
	return( changed );
}
/**************************************************************************
* t_sync_rows_change_normal
*	Syncronize the terminal, if necessary, to the defaul number of rows.
**************************************************************************/
t_sync_rows_change_normal()
{
	if ( M_rows_changeno )
		term_rows_change( 0 );
	if ( M_extra_data_row )
		term_extra_data_row( 0 );
}
/**************************************************************************
* term_rows_change
*	TERMINAL OUTPUT module for 'rows_change'.
*	"rows_changeno" specifies the index of the 'rows_change' sequence.
**************************************************************************/
term_rows_change( rows_changeno )
{
	char	*p;
	int	rows;

	p = T_rows_change[ rows_changeno ][ M_extra_data_row ];
	if ( p != NULL )
	{
		term_tputs( p );
		term_outgo();
		M_rows_changeno = rows_changeno;
		rows = Rows_change_display_rows[ rows_changeno ]
		     + M_extra_data_row;
		change_rows_terminal( rows,
				      0, F_rows_change_does_clear_screen );
		if ( F_rows_change_t_auto_wrap_off )
			term_se_auto_wrap_off();
		if ( F_rows_change_switch_page_number_0 )
			term_se_switch_page_number_0();
		if ( F_rows_change_clears_pages )
			forget_all_pages();
		term_se_rows_change_soft_reset();
		if ( T_rows_change_clears_perwindow != (char *) 0 )
			term_se_perwindow_default_string(
				T_rows_change_clears_perwindow );
		if ( Rows_change_no_status[ rows_changeno ] )
			term_se_no_status();
		term_outgo();
	}
}
/**************************************************************************
* term_se_rows_change_soft_reset
*	Changing the number of rows on the terminal has caused a soft reset.
**************************************************************************/
term_se_rows_change_soft_reset()
{
	if ( F_rows_change_sets_cursor_on )
		term_se_cursor( 1 );
	if ( F_rows_change_resets_insert_mode )
		term_se_insert_mode( 0 );
	if ( F_rows_change_resets_appl_keypad_mode )
		term_se_appl_keypad_mode( 0 );
	if ( F_rows_change_resets_keypad_xmit )
		term_se_keypad_xmit( 0 );
	if ( F_rows_change_resets_scroll_region )
		term_se_scroll_region_normal();
	if ( F_rows_change_resets_character_set )
		term_se_character_set_normal();
	if ( F_rows_change_attributes_off )
		term_se_attribute_off( F_rows_change_attributes_off );
	/* save_cursor is software - not terminal */
	if ( F_rows_change_resets_origin_mode )
		term_se_origin_mode( 0 );
	if ( F_rows_change_sets_mode_on || F_rows_change_sets_mode_off )
		term_se_mode( F_rows_change_sets_mode_on, 
			      F_rows_change_sets_mode_off );
}
/**************************************************************************
* win_se_rows_change_soft_reset
*	Changing the number of rows on the window has caused a soft reset.
**************************************************************************/
win_se_rows_change_soft_reset()
{
	if ( F_rows_change_sets_cursor_on )
		win_se_cursor( 1 );
	if ( F_rows_change_resets_insert_mode )
		win_se_insert_mode( 0 );
	if ( F_rows_change_resets_appl_keypad_mode )
		win_se_appl_keypad_mode( 0 );
	if ( F_rows_change_resets_keypad_xmit )
		win_se_keypad_xmit( 0 );
	if ( F_rows_change_resets_scroll_region )
		win_se_scroll_region_normal();		/* Rows must be set */
	if ( F_rows_change_resets_character_set )
		win_se_character_set_normal();
	if ( F_rows_change_attributes_off )
		win_se_attributes_off( F_rows_change_attributes_off );
	if ( F_rows_change_resets_save_cursor )
		init_save_cursor();
	if ( F_rows_change_resets_origin_mode )
		win_se_origin_mode( 0 );
	if ( F_rows_change_sets_mode_on || F_rows_change_sets_mode_off )
		win_se_mode( F_rows_change_sets_mode_on, 
			     F_rows_change_sets_mode_off );
}
/**************************************************************************
* win_se_rows_changeno_match_screen
*	An event has occurred that had the side effect of setting the
*	number of rows on the window.
**************************************************************************/
					/* match popwin rows to window */
win_se_rows_changeno_match_screen()
{
	int	rows;

	if (  ( Outwin->rows_changeno ==  M_rows_changeno  )
	   && ( Outwin->extra_data_row == M_extra_data_row ) )
	{
		return;
	}
	Outwin->rows_changeno =  M_rows_changeno;
	Outwin->extra_data_row = M_extra_data_row;
	rows = Rows_change_display_rows[ M_rows_changeno ]
	     + M_extra_data_row;
	change_display_rows( rows, 0, 
			       F_rows_change_does_clear_screen
			     | F_extra_data_row_does_clear_screen );
	win_se_scroll_region_normal();
	if ( Rows_change_no_status[ Outwin->rows_changeno ] )
		win_se_no_status();
}
/**************************************************************************
* win_se_rows_changeno_normal
*	An event has occurred that had the side effect of setting the
*	number of rows on the window.
**************************************************************************/
					/* match popwin rows to normal */
win_se_rows_changeno_normal()
{
	int	rows;

	if (  ( Outwin->rows_changeno ==  0 )
	   && ( Outwin->extra_data_row == 0 ) )
	{
		return;
	}
	Outwin->rows_changeno =  0;
	Outwin->extra_data_row = 0;
	rows = Rows_change_display_rows[ 0 ];
	change_display_rows( rows, 0, 
			       F_rows_change_does_clear_screen
			     | F_extra_data_row_does_clear_screen );
	win_se_scroll_region_normal();
}
/**************************************************************************
* dec_extra_data_row
*	DECODE module for 'extra_data_row'.
*	"extra_data_row" specifies on or off.
**************************************************************************/
/*ARGSUSED*/
dec_extra_data_row( extra_data_row, parm_ptr )
	int	extra_data_row;
	char	*parm_ptr;
{
	fct_extra_data_row( extra_data_row );
}
/**************************************************************************
* inst_extra_data_row
*	INSTALL module for 'extra_data_row'.
*	"extra_data_row" specifies on or off.
**************************************************************************/
inst_extra_data_row( str, extra_data_row )
	char	*str;
	int	extra_data_row;
{
	dec_install( "extra_data_row", (UNCHAR *) str, 
			dec_extra_data_row, extra_data_row, 0,
			(char *) 0 );
}
/**************************************************************************
* fct_extra_data_row
*	ACTION module for 'extra_data_row'.
*	"extra_data_row" specifies on or off.
**************************************************************************/
fct_extra_data_row( extra_data_row )
	int	extra_data_row;
{
	int	rows;
					/* if not switching and 
					   extra_data_row has no effect if 
					   it was already on then ignore it.
					*/
	if (  ( extra_data_row == Outwin->extra_data_row ) 
	   && F_extra_data_row_se_on_switch_only )
		return;
	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	Outwin->extra_data_row = extra_data_row;
	rows = Rows_change_display_rows[ Outwin->rows_changeno ]
	     + extra_data_row;
	change_display_rows( rows,
			     0, F_extra_data_row_does_clear_screen );
	if ( F_extra_data_row_t_auto_wrap_off )
		win_se_auto_wrap_off();
	if ( F_extra_data_row_does_clear_screen )
	{
		Outwin->xenl = 0;
		Outwin->real_xenl = 0;
		d_blankwin();
		Outwin->row = 0;
		Outwin->col = 0;
		b_cursor_home();			/* pan buffer to top */
	}
	else
	{
		if ( Outwin->row > Outwin->display_row_bottom )
			Outwin->row = Outwin->display_row_bottom;
	}
	win_se_extra_data_row_soft_reset();
	if ( T_extra_data_row_clears_perwindow != (char *) 0 )
		win_se_perwindow_clear_string(
					T_extra_data_row_clears_perwindow,
					Outwin->number );
	if ( Extra_data_row_no_status[ extra_data_row ] )
		win_se_no_status();
	if ( outwin_is_curwin() )
	{
		term_extra_data_row( extra_data_row );
		t_cursor();
	}
	if ( Outwin->onscreen )
	{
		if (  ( Outwin->fullscreen == 0 )
		   && F_extra_data_row_does_clear_screen )
		{
			split_screen_incompatible( 
				F_extra_data_row_does_clear_screen );
		}
	}
}
/**************************************************************************
* term_extra_data_row
*	TERMINAL OUTPUT module for 'extra_data_row'.
**************************************************************************/
term_extra_data_row( extra_data_row )
{
	char	*p;
	int	rows;

	p = T_extra_data_row[ extra_data_row ][ M_rows_changeno ];
	if ( p != NULL )
	{
		term_tputs( p );
		term_outgo();
		M_extra_data_row = extra_data_row;
		rows = Rows_change_display_rows[ M_rows_changeno ]
		     + extra_data_row;
		change_rows_terminal( rows,
				      0, F_extra_data_row_does_clear_screen );
		if ( F_extra_data_row_t_auto_wrap_off )
			term_se_auto_wrap_off();
		if ( F_extra_data_row_switch_page_number_0 )
			term_se_switch_page_number_0();
		if ( F_extra_data_row_clears_pages )
			forget_all_pages();
		term_se_extra_data_row_soft_reset();
		if ( T_extra_data_row_clears_perwindow != (char *) 0 )
			term_se_perwindow_default_string(
				T_extra_data_row_clears_perwindow );
		if ( Extra_data_row_no_status[ extra_data_row ] )
			term_se_no_status();
		term_outgo();
	}
}
/**************************************************************************
* term_se_extra_data_row_soft_reset
*	Changing the extra data row on the terminal has caused a soft reset.
**************************************************************************/
term_se_extra_data_row_soft_reset()
{
	if ( F_extra_data_row_sets_cursor_on )
		term_se_cursor( 1 );
	if ( F_extra_data_row_resets_insert_mode )
		term_se_insert_mode( 0 );
	if ( F_extra_data_row_resets_appl_keypad_mode )
		term_se_appl_keypad_mode( 0 );
	if ( F_extra_data_row_resets_keypad_xmit )
		term_se_keypad_xmit( 0 );
	if ( F_extra_data_row_resets_scroll_region )
		term_se_scroll_region_normal();
	if ( F_extra_data_row_resets_character_set )
		term_se_character_set_normal();
	if ( F_extra_data_row_attributes_off )
		term_se_attribute_off( F_extra_data_row_attributes_off );
	/* save_cursor is software - not terminal */
	if ( F_extra_data_row_resets_origin_mode )
		term_se_origin_mode( 0 );
	if ( F_extra_data_row_sets_mode_on || F_extra_data_row_sets_mode_off )
		term_se_mode( F_extra_data_row_sets_mode_on, 
			      F_extra_data_row_sets_mode_off );
}
/**************************************************************************
* win_se_extra_data_row_soft_reset
*	Changing the extra data row on the window has caused a soft reset.
**************************************************************************/
win_se_extra_data_row_soft_reset()
{
	if ( F_extra_data_row_sets_cursor_on )
		win_se_cursor( 1 );
	if ( F_extra_data_row_resets_insert_mode )
		win_se_insert_mode( 0 );
	if ( F_extra_data_row_resets_appl_keypad_mode )
		win_se_appl_keypad_mode( 0 );
	if ( F_extra_data_row_resets_keypad_xmit )
		win_se_keypad_xmit( 0 );
	if ( F_extra_data_row_resets_scroll_region )
		win_se_scroll_region_normal();		/* Rows must be set */
	if ( F_extra_data_row_resets_character_set )
		win_se_character_set_normal();
	if ( F_extra_data_row_attributes_off )
		win_se_attributes_off( F_extra_data_row_attributes_off );
	if ( F_extra_data_row_resets_save_cursor )
		init_save_cursor();
	if ( F_extra_data_row_resets_origin_mode )
		win_se_origin_mode( 0 );
	if ( F_extra_data_row_sets_mode_on || F_extra_data_row_sets_mode_off )
		win_se_mode( F_extra_data_row_sets_mode_on, 
			     F_extra_data_row_sets_mode_off );
}
/**************************************************************************
* split_screen_incompatible
*	A change has occurred that makes the halves of a split screen
*	not displayable at the same time.
*	Remove one of them from the screen.
*	"screen_cleared_if_curwin" indicates that if the current output
*	window is the current window then the screen was cleared as a
*	side effect of getting here.
**************************************************************************/
split_screen_incompatible( screen_cleared_if_curwin )
	int	screen_cleared_if_curwin;
{
	if ( outwin_is_curwin() )
	{
		w_all_off();
		Outwin->onscreen = 1;
		check_split_windows();
		modes_off_redo_screen();
		if ( screen_cleared_if_curwin == 0 )
			clear_other_split_win();
		split_label_split_row();
		split_number( Outwin->win_bot_row + 1, Outwin->number );
		modes_on_redo_screen();
		s_cursor();
	}
	else
	{
		force_split_outwin_offscreen();
	}
}
/****************************************************************************/
char *dec_encode();
/**************************************************************************
* extra_rows_change
*	TERMINAL DESCRIPTION PARSER module for 'rows_change'.
**************************************************************************/
extra_rows_change( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;
	long	mode_encode();
	FTCHAR	attribute_encode();
	int	display_rows;
	int	i;
	int	first;
	int	last;

	if ( strcmp( buff, "rows_change" ) == 0 )
	{
		if ( Rows_changeno < MAX_ROWS_CHANGE )
		{
			if ( attr_on_string != NULL )
			{
				display_rows = atoi( attr_on_string );
				if ( display_rows > 0 )
				{
					encoded = dec_encode( string );
					T_rows_change[ Rows_changeno ][ 0 ] = 
					T_rows_change[ Rows_changeno ][ 1 ] = 
						encoded;
					Rows_change_display_rows[ 
						Rows_changeno ] = display_rows;
					inst_rows_change( encoded, 
						Rows_changeno );
					rows_change_encode( Rows_changeno, 
							    attr_off_string );
					Rows_changeno++;
				}
				else
					printf(
"Invalid display_rows on rows_change.\n" );
			}
			else
				printf( 
"Missing display_rows on rows_change.\n" );
		}
		else
		{
			printf( "Too many rows_change. Max = %d\n", 
				MAX_ROWS_CHANGE );
		}
	}
	else if ( strcmp( buff, "out_rows_change" ) == 0 )
	{
		if ( Rows_changeno > 0 )
		{
			T_rows_change[ Rows_changeno - 1 ][ 0 ] = 
			T_rows_change[ Rows_changeno - 1 ][ 1 ] = 
				dec_encode( string );
		}
		else
		{
			printf( "Out_rows_change preceeds rows_change.\n" );
		}
	}
	else if ( strcmp( buff, "out_rows_change_extra_data_row_off" ) == 0 )
	{
		if ( Rows_changeno > 0 )
		{
			T_rows_change[ Rows_changeno - 1 ][ 0 ] = 
				dec_encode( string );
		}
		else
		{
			printf(
		"Out_rows_change_extra_data_row_off preceeds rows_change.\n" );
		}
	}
	else if ( strcmp( buff, "out_rows_change_extra_data_row_on" ) == 0 )
	{
		if ( Rows_changeno > 0 )
		{
			T_rows_change[ Rows_changeno - 1 ][ 1 ] = 
				dec_encode( string );
		}
		else
		{
			printf(
		"Out_rows_change_extra_data_row_on preceeds rows_change.\n" );
		}
	}
	else if ( strcmp( buff, "rows_change_se_on_switch_only" ) == 0 )
	{
		F_rows_change_se_on_switch_only =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_does_clear_screen" ) == 0 )
	{
		F_rows_change_does_clear_screen =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_turns_auto_wrap_off" ) == 0 )
	{
		F_rows_change_t_auto_wrap_off =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_turns_auto_wrap_on" ) == 0 )
	{
		F_rows_change_t_auto_wrap_on =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_switch_page_number_0" ) == 0 )
	{
		F_rows_change_switch_page_number_0 =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_clears_pages" ) == 0 )
	{
		F_rows_change_clears_pages =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_sets_cursor_on" ) == 0 )
	{
		F_rows_change_sets_cursor_on = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_resets_insert_mode" ) == 0 )
	{
		F_rows_change_resets_insert_mode = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_resets_appl_keypad_mode" ) == 0 )
	{
		F_rows_change_resets_appl_keypad_mode = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_resets_keypad_xmit" ) == 0 )
	{
		F_rows_change_resets_keypad_xmit = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_resets_scroll_region" ) == 0 )
	{
		F_rows_change_resets_scroll_region = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_resets_character_set" ) == 0 )
	{
		F_rows_change_resets_character_set = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_attributes_off" ) == 0 )
	{
		F_rows_change_attributes_off = 
				attribute_encode( attr_off_string );
	}
	else if ( strcmp( buff, "rows_change_resets_save_cursor" ) == 0 )
	{
		F_rows_change_resets_save_cursor = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_resets_origin_mode" ) == 0 )
	{
		F_rows_change_resets_origin_mode = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "rows_change_sets_mode" ) == 0 )
	{
		F_rows_change_sets_mode_on = mode_encode( attr_on_string );
		F_rows_change_sets_mode_off = mode_encode( attr_off_string );
	}
	else if ( strcmp( buff, "rows_change_clears_perwindow" ) == 0 )
	{
		T_rows_change_clears_perwindow = dec_encode( string );
	}
	else if ( strcmp( buff, "extra_data_row_off" ) == 0 )
	{
		encoded = dec_encode( string );
		inst_extra_data_row( encoded, 0 );
		for ( i = 0; i < MAX_ROWS_CHANGE; i++ )
			T_extra_data_row[ 0 ][ i ] = encoded;
		extra_data_row_encode( 0, attr_off_string );
	}
	else if ( strcmp( buff, "extra_data_row_on" ) == 0 )
	{
		F_has_extra_data_row = 1;
		encoded = dec_encode( string );
		inst_extra_data_row( encoded, 1 );
		for ( i = 0; i < MAX_ROWS_CHANGE; i++ )
			T_extra_data_row[ 1 ][ i ] = encoded;
		extra_data_row_encode( 1, attr_off_string );
	}
	else if ( strcmp( buff, "out_extra_data_row_off" ) == 0 )
	{
		if ( ( attr_on_string != NULL ) 
		   && ( *attr_on_string >= '0' )
		   && ( *attr_on_string <= ( '0' + MAX_ROWS_CHANGE - 1 ) ) )
		{
			first = last = *attr_on_string - '0';
		}
		else
		{
			first = 0;
			last = MAX_ROWS_CHANGE - 1;
		}
		encoded = dec_encode( string );
		for ( i = first; i <= last; i++ )
			T_extra_data_row[ 0 ][ i ] = encoded;
	}
	else if ( strcmp( buff, "out_extra_data_row_on" ) == 0 )
	{
		if ( ( attr_on_string != NULL ) 
		   && ( *attr_on_string >= '0' )
		   && ( *attr_on_string <= ( '0' + MAX_ROWS_CHANGE - 1 ) ) )
		{
			first = last = *attr_on_string - '0';
		}
		else
		{
			first = 0;
			last = MAX_ROWS_CHANGE - 1;
		}
		encoded = dec_encode( string );
		for ( i = first; i <= last; i++ )
			T_extra_data_row[ 1 ][ i ] = encoded;
	}
	else if ( strcmp( buff, "extra_data_row_se_on_switch_only" ) == 0 )
	{
		F_extra_data_row_se_on_switch_only =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_does_clear_screen" ) == 0 )
	{
		F_extra_data_row_does_clear_screen =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_turns_auto_wrap_off" ) == 0 )
	{
		F_extra_data_row_t_auto_wrap_off =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_turns_auto_wrap_on" ) == 0 )
	{
		F_extra_data_row_t_auto_wrap_on =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_switch_page_number_0" ) == 0 )
	{
		F_extra_data_row_switch_page_number_0 =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_clears_pages" ) == 0 )
	{
		F_extra_data_row_clears_pages =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_sets_cursor_on" ) == 0 )
	{
		F_extra_data_row_sets_cursor_on = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_resets_insert_mode" ) == 0 )
	{
		F_extra_data_row_resets_insert_mode = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_resets_appl_keypad_mode" )
									== 0 )
	{
		F_extra_data_row_resets_appl_keypad_mode = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_resets_keypad_xmit" ) == 0 )
	{
		F_extra_data_row_resets_keypad_xmit = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_resets_scroll_region" ) == 0 )
	{
		F_extra_data_row_resets_scroll_region = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_resets_character_set" ) == 0 )
	{
		F_extra_data_row_resets_character_set = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_attributes_off" ) == 0 )
	{
		F_extra_data_row_attributes_off = 
				attribute_encode( attr_off_string );
	}
	else if ( strcmp( buff, "extra_data_row_resets_save_cursor" ) == 0 )
	{
		F_extra_data_row_resets_save_cursor = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_resets_origin_mode" ) == 0 )
	{
		F_extra_data_row_resets_origin_mode = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "extra_data_row_sets_mode" ) == 0 )
	{
		F_extra_data_row_sets_mode_on = mode_encode( attr_on_string );
		F_extra_data_row_sets_mode_off = mode_encode( attr_off_string );
	}
	else if ( strcmp( buff, "extra_data_row_clears_perwindow" ) == 0 )
	{
		T_extra_data_row_clears_perwindow = dec_encode( string );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* rows_change_encode
*	Encode the string "string" from a 'rows_change' command.
*	'N' indicates that this row configuration has no status line.
*	"rows_changeno" specifies the index of the 'rows_change' sequence.
**************************************************************************/
rows_change_encode( rows_changeno, string )
	int	rows_changeno;
	char	*string;
{
	char	c;

	if ( string != NULL )
	{
		while( ( c = *string++ ) != '\0' )
		{
			switch( c )
			{
			case 'N':
			case 'n':
				Rows_change_no_status[ rows_changeno ] = 1;
				break;
			default:
				printf( "rows_change did not encode '%c'\n", 
					c );
			}
		}
	}
}
/**************************************************************************
* rows_change_encode
*	Encode the string "string" from a 'rows_change' command.
*	'N' indicates that this row configuration has no status line.
*	"rows_changeno" specifies the index of the 'rows_change' sequence.
**************************************************************************/
extra_data_row_encode( extra_data_row, string )
	int	extra_data_row;
	char	*string;
{
	char	c;

	if ( string != NULL )
	{
		while( ( c = *string++ ) != '\0' )
		{
			switch( c )
			{
			case 'N':
			case 'n':
				Extra_data_row_no_status[ extra_data_row ] = 1;
				break;
			default:
				printf( "extra_data_row did not encode '%c'\n", 
					c );
			}
		}
	}
}
