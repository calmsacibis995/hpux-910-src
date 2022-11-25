/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftmodes.c,v 70.1 92/03/09 15:43:31 ssa Exp $ */
#include <stdio.h>
#include "ftwindow.h"
#include "modes.h"
#include "ftterm.h"
#include "perwindow.h"
#include "features.h"

/**************************************************************************
* modes_init
*	Set window's modes back to default on going idle
**************************************************************************/
modes_init( winno )
	int	winno;
{
	FT_WIN	*win;
	int	j;

	win = Wininfo[ winno ];
	win->insert_mode = 0;
	win->auto_wrap_on = F_auto_wrap_on_default;
	modes_init_write_protect_on( win );
	modes_init_auto_scroll_on( win );
	win->appl_keypad_mode_on = 0;
	win->cursor_key_mode_on = 0;
	win->keypad_xmit = 0;
	win->cursor_on = 1;
	win->status_on = 0;
	win->origin_mode = 0;
	modes_init_save_cursor( win );
	win->pc_mode_on = 0;
	modes_init_terminal_mode( win );
	modes_init_transparent_print( win );
	modes_init_graph_mode( win );
	modes_init_graph_screen( win );
	modes_init_special( win );
	modes_init_onstatus( win );
	win->ftattrs = 0;
	win->character_set = 0;
	win->cursor_type = 0;
	for ( j = 0; j < MAX_CHARACTER_SETS; j++ )
		win->select_character_set[ j ] = 0;
	modes_init_hp_charset_select( win );
	win->csr_buff_top_row = 0;
	win->csr_buff_bot_row = win->display_row_bottom;
	win->memory_lock_row = -1;
	modes_idle_init_margins( win );
	win->mode = 0;
#ifdef DO_NOT_CHANGE
	win->status_type = 0;
	win->status_line[ 0 ] = '\0';
	win->xenl = 0;
	win->real_xenl = 0;
	win->columns_wide_mode_on = 0;
	win->line_attribute_current = 0;
	win->cols = Cols;
	win->col_right = Col_right; 
	for ( j = 0; j < win->display_rows; j++ )
		win->line_attribute[ j ] = 0;
#endif
}
#define ALL_MODES		0
#define OUTPUT_MODES_ONLY	1
/**************************************************************************
* modes_off_window_mode
*	Set terminals modes to the defaults for window command mode.
*	This could be ^W or ioctl FIOC_WINDOW_COMMAND.
**************************************************************************/
modes_off_window_mode()
{
	modes_off( ALL_MODES );
	t_sync_perwindow_default( SYNC_MODE_WINDOW );
	/* t_sync_keypad_xmit( 1 );  only on ^W */
}
/**************************************************************************
* modes_on_window_mode
*	Restore the modes changed by modes_off_window_mode.
**************************************************************************/
modes_on_window_mode()
{
	modes_on( ALL_MODES );
	t_sync_perwindow_all( SYNC_MODE_WINDOW );
	t_sync_keypad_xmit( Outwin->keypad_xmit );
}
/**************************************************************************
* modes_off_outwin
*	Set terminals modes to default when writing to a non-current but
*	on-screen window. I.E. other split screen.
**************************************************************************/
modes_off_outwin()
{
	modes_off( OUTPUT_MODES_ONLY );
	t_sync_perwindow_default( SYNC_MODE_OUTWIN );
}
/**************************************************************************
* modes_on_outwin
*	Restore modes changed by modes_off_outwin.
**************************************************************************/
modes_on_outwin()
{
	modes_on( OUTPUT_MODES_ONLY );
	t_sync_perwindow_all( SYNC_MODE_OUTWIN );
}
/**************************************************************************
* modes_off_redo_screen
*	Set modes to default appropriate for rearranging the split screen
*	configuration.
**************************************************************************/
modes_off_redo_screen()
{
	modes_off( ALL_MODES );
	t_sync_perwindow_default( SYNC_MODE_REDO_SCREEN );
}
/**************************************************************************
* modes_on_redo_screen
*	Reset the modes changed by modes_off_redo_screen.
**************************************************************************/
modes_on_redo_screen()
{
	modes_on( ALL_MODES );
	t_sync_perwindow_all( SYNC_MODE_REDO_SCREEN );
}
#define NOT_PROCESSING_FTKEY 0
/**************************************************************************
* modes_init_idle
*	Set window's modes back to default on going idle.
**************************************************************************/
modes_init_idle( winno )
	int	winno;
{
	modes_init( winno );
	perwindow_init( SYNC_MODE_IDLE, winno, NOT_PROCESSING_FTKEY );
}
/**************************************************************************
* t_sync_modes_idle
*	Set terminal to modes appropriate for current window went idle.
**************************************************************************/
t_sync_modes_idle()
{
	modes_off( ALL_MODES );
	modes_on( ALL_MODES );
	t_sync_input_modes();
	t_sync_perwindow_default( SYNC_MODE_IDLE );
}
/**************************************************************************
* modes_off_quit_start
*	Set modes appropriate for start of shutdown.
**************************************************************************/
modes_off_quit_start()
{
	modes_off( ALL_MODES );
}
/**************************************************************************
* modes_off_quit
*	Set modes appropriate for shutdown.
**************************************************************************/
modes_off_quit()
{
	modes_off( ALL_MODES );
	t_sync_perwindow_default( SYNC_MODE_QUIT );
	t_sync_keypad_xmit( 0 );
}
/**************************************************************************
* t_sync_modes_current
*	Set modes appropriate for new current window.
**************************************************************************/
t_sync_modes_current()
{
	t_sync_input_modes();
	t_sync_perwindow_all( SYNC_MODE_CURRENT );
}
/**************************************************************************
* modes_default_start
*	Set terminals starting modes if necessary.
*	Called during sender initialization.
**************************************************************************/
modes_default_start()
{
	t_sync_perwindow_default( SYNC_MODE_START );
}
/**************************************************************************
* t_sync_modes_screen_attribute
*	Set modes appropriate for current window attributes.
**************************************************************************/
t_sync_modes_screen_attribute()
{
	t_sync_perwindow_all( SYNC_MODE_SCREEN_ATTRIBUTE );
	t_sync_ibm_control_modes_screen_attribute();
}
/**************************************************************************
* modes_off
*	Set terminal modes to defaults.
*	If "output_modes_only" is greater than 0,
*	do not change status line to default.
**************************************************************************/
modes_off( output_modes_only )
	int	output_modes_only;
{
	if ( Outwin->insert_mode )
		term_exit_insert_mode();
	t_sync_attr( (FTCHAR) 0 );
	t_sync_character_set_base();
	t_sync_hp_charset_select_base();
	t_sync_ibm_control_modes_off();
	t_sync_auto_wrap( 1 );
	t_sync_write_protect( 0 );
	t_sync_auto_scroll( 1 );
	t_sync_cursor( 1, 0 );
	t_sync_mode( 0L );
	if ( output_modes_only == 0 )
		t_sync_status_line_modes_off();
	t_sync_graph_mode( 0 );
	t_sync_margins_full();
	t_sync_memory_unlock();
	if ( t_sync_scroll_region( 0, Row_bottom_terminal ) )
		t_cursor();
	/* ??? origin_mode ??? */
}
/**************************************************************************
* modes_on
*	Reset modes set by modes_on.
*	If "output_modes_only" is greater than 0, status line is ok.
**************************************************************************/
modes_on( output_modes_only )
	int	output_modes_only;
{
	if ( output_modes_only == 0 )
		t_sync_status_line( Outwin );
	t_sync_graph_mode( Outwin->graph_mode_on );
	if ( Outwin->insert_mode )
		term_enter_insert_mode();
	t_sync_attr( Outwin->ftattrs );
	t_sync_character_set();
	t_sync_hp_charset_select_outwin();
	t_sync_ibm_control_modes_on();
	t_sync_auto_wrap( Outwin->auto_wrap_on );
	t_sync_write_protect( Outwin->write_protect_on );
	t_sync_auto_scroll( Outwin->auto_scroll_on );
	t_sync_cursor( (int) (Outwin->cursor_on), (int) (Outwin->cursor_type) );
	t_sync_mode( Outwin->mode );
	t_sync_margins_outwin();
	if ( Outwin->fullscreen )
	{
		/**********************************************************
		* do scroll_region first since it cancels memory_lock.
		**********************************************************/
		if (  ( t_sync_scroll_region( Outwin->csr_buff_top_row,
					   Outwin->csr_buff_bot_row ) )
		   || ( t_sync_memory_lock( Outwin->memory_lock_row ) ) )
		{
			t_cursor();
		}
	}
	else
	{				/* leave whole screen if split mode */
		t_sync_memory_unlock();
		if ( t_sync_scroll_region( 0, Row_bottom_terminal ) )
		{
			t_cursor();
		}
	}
}
/**************************************************************************
* t_sync_input_modes
*	Syncronize the terminal (if necessary) to the input modes 
*	appropriate the current window.
**************************************************************************/
t_sync_input_modes()
{
	t_sync_appl_keypad_mode( Outwin->appl_keypad_mode_on );
	if ( Outwin->cursor_key_mode_on != M_cursor_key_mode_on )
	{
		if ( Outwin->cursor_key_mode_on )
			term_enter_cursor_key_mode();
		else
			term_exit_cursor_key_mode();
	}
	/* t_sync_keypad_xmit( Outwin->keypad_xmit ); done fsend_procack_array*/
	t_sync_function_key();
	t_sync_ibm_control_input_modes();
}
/**************************************************************************
* modes_reload_current
*	Reload all perwindow capablities that are set when the current
*	window changes.
*	This is used after screen saver for status line etc.
**************************************************************************/
modes_reload_current()
{
	t_reload_perwindow_all( SYNC_MODE_CURRENT );
}
/**************************************************************************
* t_reload_insert_mode
*	Set the terminal to the insert mode of the current output window.
**************************************************************************/
t_reload_insert_mode()
{
	if ( Outwin->insert_mode )
		term_enter_insert_mode();
	else
		term_exit_insert_mode();
}
