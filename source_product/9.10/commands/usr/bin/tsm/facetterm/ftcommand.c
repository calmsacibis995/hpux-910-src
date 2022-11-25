/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftcommand.c,v 70.1 92/03/09 15:42:42 ssa Exp $ */
#include "stdio.h"
#include "ftcomman_p.h"

#include "ftchar.h"
#include "ftwindow.h"
#include "wins.h"
#include "ftterm.h"
#include "output.h"
#include "keyval.h"
#include "features.h"
#include "controlopt.h"
int	Switch_window = -1;

#include "options.h"
int	Opt_disable_control_W_r = 0;
int	Opt_disable_control_W_x_excl = 0;

#include "cut.h"
int	Cut_type = STREAM_CUT_TYPE;
int	Cut_and_paste_timer = 0;

#include "facetterm.h"
#include "myattrs.h"

FT_WIN	*Wininfo[ TOPWIN ];

int	Wins = TOPWIN;

#include "curwinno.h"
int	Curwinno;               /* current window */
FT_WIN	*Curwin;
int	Outwinno;		/* current output window */
FT_WIN	*Outwin;
int	O_pe = 0;		/* current output window (Outwin) personality */

int	Last_curwinno;

int	Overwritten_winno = -1;
int	Overwritten_split_winno = -1;

int	D_split_row = 11;

char	*Paste_buffer_ptr;

int Prompting_on = 1;

#define MAX_POPUP_SCREEN_LEVEL 10
int	Popup_screen_level = 0;
int	Popup_screen_first_col[ MAX_POPUP_SCREEN_LEVEL ] = { 0 };
int	Popup_screen_last_col[  MAX_POPUP_SCREEN_LEVEL ] = { 0 };
int	Popup_screen_first_row[ MAX_POPUP_SCREEN_LEVEL ] = { 0 };
int	Popup_screen_last_row[  MAX_POPUP_SCREEN_LEVEL ] = { 0 };

#include "scroll.h"

#include "facetpath.h"

char *sT_out_screen_saver_start = { NULL };
char *sT_out_screen_saver_end = { NULL } ;

char *sT_out_run_raw_tty_start = { NULL };
char *sT_out_run_raw_tty_end = { NULL } ;

char	*Text_facetterm_window_label =
"FacetTerm Window %d\n";

char	*Text_window_printer_mode =
"Transparent print mode\n";

#include "printer.h"
int	Window_printer[ TOPWIN ] = { 0 };
int	Window_printer_disable = 0;

/**************************************************************************
* fct_init_windows
*	Allocate paste buffer.
*	Allocate and initialize window structures and row memory.
*	Allocate scroll memory for HP terminals.
*	Display window 1 on the screen.
**************************************************************************/
fct_init_windows()
{
        int     i;
	int	j;
	char	buff[ 80 ];
	FTCHAR	*p;
	long	*malloc_run();

	Paste_buffer_ptr = (char *) malloc_run( MAX_ROWS * ( MAX_COLS + 1 ),
						"paste_buffer" );
	if ( Paste_buffer_ptr == NULL )
	{
		fsend_kill( "Facet process: pastebuffer malloc failed\r\n",
				0 );
	}
	*Paste_buffer_ptr = '\0';
        for ( i=0; i < Wins; i++ )
        {
		Outwinno = i;
		Wininfo[ i ] = ( FT_WIN * ) malloc_run( sizeof( FT_WIN ),
							"window_information" );
		if ( Wininfo[ i ] == NULL )
		{
			fsend_kill( "Facet process: wininfo malloc failed\r\n",
					0 );
		}
		Outwin = Wininfo[ i ];
		Outwin->number = i;
		Outwin->display_rows = Rows_default;
		Outwin->display_row_bottom = Row_bottom_default;
		Outwin->onscreen = 0;
		Outwin->in_terminal = 0;
		clear_row_changed( Outwin );
		clear_col_changed( Outwin );
		Outwin->fullscreen = 1;
		Outwin->insert_mode = 0;
		Outwin->auto_wrap_on = F_auto_wrap_on_default;
		init_auto_scroll_on();
		init_write_protect_on();
		Outwin->appl_keypad_mode_on = 0;
		Outwin->cursor_key_mode_on = 0;
		Outwin->keypad_xmit = 0;
		Outwin->cursor_on = 1;
		init_status_line( Outwin );
		Outwin->xenl = 0;
		Outwin->real_xenl = 0;
		Outwin->origin_mode = 0;
		init_save_cursor();
		Outwin->columns_wide_mode_on = F_columns_wide_mode_on_default;
		Outwin->line_attribute_current = 0;
		Outwin->pc_mode_on = 0;
		init_windows_rows_change();
		init_windows_terminal_mode();
		init_windows_transparent_print();
		init_windows_graph_mode();
		init_windows_graph_screen();
		init_windows_special();
		init_windows_onstatus();
		Outwin->ftattrs = 0;
		Outwin->character_set = 0;
		Outwin->cursor_type = 0;
		if ( F_columns_wide_mode_on_default )
		{
			Outwin->cols = 			Cols_wide;
			Outwin->col_right = 		Col_right_wide; 
			Outwin->col_right_line = 	Col_right_wide; 
		}
		else
		{
			Outwin->cols = 			Cols;
			Outwin->col_right = 		Col_right; 
			Outwin->col_right_line = 	Col_right; 
		}
		init_margins();
		for ( j = 0; j < MAX_CHARACTER_SETS; j++ )
			Outwin->select_character_set[ j ] = 0;
		init_hp_charset( Outwin );
		Outwin->col = 0;
		Outwin->row = 0;
		Outwin->win_top_row = 0;
		Outwin->win_bot_row = Outwin->display_row_bottom;
		Outwin->buff_top_row = 0;
		Outwin->buff_bot_row = Outwin->display_row_bottom;
		Outwin->csr_buff_top_row = 0;
		Outwin->csr_buff_bot_row = Outwin->display_row_bottom;
		Outwin->memory_lock_row = -1;
		Outwin->mode = 0;
		Outwin->paste_end_of_line[ 0 ] = '\r';
		Outwin->paste_end_of_line[ 1 ] = '\0';
		init_function_key( Outwin );
		for ( j = 0; j < Rows_max; j++ )
		{
			Outwin->line_attribute[ j ] = 0;
			Outwin->ftchars[ j ] = ( FTCHAR * )
				malloc_run( Cols_wide * sizeof( FTCHAR ),
					    "window_row" );
			if ( Outwin->ftchars[ j ] == NULL )
			{
				fsend_kill(
				    "Facet process: chars malloc failed\r\n",
				    0 );
			}
		}
		if ( Rows_scroll_memory > 0 )
		{
			Outwin->scroll_memory_ftchars = ( FTCHAR ** )
			     malloc_run( Rows_scroll_memory * sizeof( FTCHAR *),
					 "scroll_memory_pointers" );
			if ( Outwin->scroll_memory_ftchars == NULL )
			{
				fsend_kill(
			"Facet process: scroll_memory malloc failed\r\n",
				    0 );
			}
			for ( j = 0; j < Rows_scroll_memory; j++ )
			{
				p = ( FTCHAR * ) malloc_run(
						 Cols_wide * sizeof( FTCHAR ),
						 "scroll_memory_row" );
				if ( p == NULL )
				{
					fsend_kill(
			"Facet process: scroll ftchars malloc failed\r\n",
					    0 );
				}
				Outwin->scroll_memory_ftchars[ j ] = p;
				d_blankline( p );
			}
		}
		d_blankwin_max();
                /* "FacetTerm Window %d\n" */
                sprintf( buff, Text_facetterm_window_label, i+1 );
		ftproc_puts( buff );
		if ( Window_printer[ i ] )
		{
			ftproc_puts( Text_window_printer_mode );
		}
        }
	Last_curwinno = 0;
	Curwinno = 0;
	Curwin = Wininfo[ 0 ];
	Outwinno = 0;
	Outwin = Wininfo[ 0 ];
	O_pe = Outwin->personality;
	out_term_clear_screen();
	Outwin->onscreen = 1;
	tell_full_refresh();
	full_refresh();
	f_cursor();
	term_outgo();
}
#include "winactive.h"
#include "invisible.h"
int	Window_invisible[ TOPWIN ] = { 0 };
/**************************************************************************
* ftproc_window_mode_select
*	The current window has gone idle, the Idle_window was set to a 
*	window or to 'next active', the receiver has been notified
*	and is standing by, the contents of Idle_window is in "winno".
*	Switch windows if necessary and tell the receiver to go.
**************************************************************************/
ftproc_window_mode_select( winno )
	int	winno;
{
	int	i;
					/* this is idle window switch -
					   do not
					   cause refresh if already current
					*/
	if ( winno == IDLE_WINDOW_IS_NEXT_ACTIVE )
	{
		winno = Curwinno;
		for ( i = 0; i < Wins; i++ )
		{
			winno++;
			if ( winno >= Wins )
				winno = 0;
			if ( Window_active[ winno ] 
			  && ( Window_invisible[ winno ] == 0 ))
			{
				break;
			}
		}
	}
	if ( winno != Curwinno )
	{
		check_notify_when_current( winno );
		ftproc_select( winno, 0 );
	}
	fct_window_mode_ans_curwin();
}
#include "wwindowkey.h"
/**************************************************************************
* ftproc_window_mode_notify
*	User pressed break, break is being sent to facetterm, the receiver
*	is standing by, the 'windows window' was set to "winno".
*	Send the notify char to "winno" and tell the receiver to go.
**************************************************************************/
ftproc_window_mode_notify( winno )
	int	winno;
{					/* this is windows window notify */
	window_mode_notify( winno );
	fct_window_mode_ans_curwin();
}
/**************************************************************************
* window_mode_notify
*	Paste the character signifying that break was pressed to the
*	'windows window' "winno".
**************************************************************************/
window_mode_notify( winno )
	int	winno;
{
	fct_window_mode_ans( (char) 'p' );
	fct_window_mode_ans_winno( winno );
	fct_window_mode_ans( Windows_window_char_start );
	fct_window_mode_ans( 0 );
}
#include "notify.h"
int	Window_notify[ TOPWIN ] = { 0 };
#define CONTROL ( -0x40 )
#include "notifykey.h"
char	Notify_when_current_char = NOTIFY_WHEN_CURRENT_CHAR;
/**************************************************************************
* check_notify_when_current
*	The window number "winno" has become current.
*	If it had requested to be notified, paste the notify character
*	to that window.
**************************************************************************/
check_notify_when_current( winno )
	int	winno;
{
	if ( Window_notify[ winno ] )
	{
		fct_window_mode_ans( (char) 'p' );
		fct_window_mode_ans_winno( winno );
		fct_window_mode_ans( Notify_when_current_char );
		fct_window_mode_ans( 0 );
	}
}
#include "monitor.h"
int	Window_monitor[ TOPWIN ] = { 0 };
#include "transpar.h"
int	Window_transparent[ TOPWIN ] = { 0 };
#include "blocked.h"
int	Window_blocked[ TOPWIN ] = { 0 };

#include "paste.h"

int	Redirected_winno = -1;

char	*Map_window_mode_prompt = "";

char	 *Map_get_command_redirect = "";
char	*Text_get_command_redirect =
	"Redirect to Window";
char	*Prmt_get_command_quit_active = "";
char	 *Map_get_command_quit_active = "";
char	*Text_get_command_quit_active =
	"WARNING! Windows active. QUIT FacetTerm ? (Y or N)";
char	*Text_get_command_quit_idle =
	"QUIT FacetTerm ? (Y or N)";
char	*Text_name_input_run_program =
	"Run Program";
char	*Text_name_input_key_file =
	"Key File";
char	*Text_name_input_global_key_file =
	"Gl. Key File";
char	*Text_name_input_paste_eol_type =
	"Paste eol type";
char	 *Map_get_command_extra_commands = "";
char	*Text_get_command_extra_commands =
	"Cap Inv Mon Nfy Trsp Hng Blk Key Rpl Scr Prt Lck ! "; /* TBD */
char	 *Map_get_command_control_chars = "";
char	*Text_get_command_control_chars =
	"Break  q=Control-Q  s=Control-S  @=Control-@";
char	*Text_name_input_window_title =
	"Window title";
char	 *Map_get_command_mapped = "";
char	*Text_get_command_mapped =
	"Hotkey Map Filename Unmap";
char	 *Map_get_command_paste_commands = "";
char	*Text_get_command_paste_commands =
	"paste to Printer Script File Append Overwrite";
char	*Text_printer_mode_on = "\nPrinter mode on.\n";
char	*Text_printer_mode_off = "\nPrinter mode off.\n";


char	*Text_name_input_run_raw_tty =
	"Run RAW tty";
char	*Text_facetterm_run_raw_tty =
	"FacetTerm suspended until command exits\r\n\n";
char	*Text_facetterm_resuming =
	"\r\n\nFacetTerm resuming\r\n";
/**************************************************************************
* ftproc_window_mode
*	The main processor for 'window command mode'.
*	"winc" is the hotkey pressed - used to check for 2 hotkey's meaning
*	one hotkey character to the window.
**************************************************************************/
ftproc_window_mode( winc )
	int	winc;
{
	int	c;
	int	first;
	int	winno;
	int	command;
	char	*shell;
	char	*getenv();
#define MAX_PROG 41
	char	program[ MAX_PROG + 1 ];
	char	paste_eol_name[ MAX_PASTE_EOL_NAME_LEN + 1 ];
#define MAX_WINDOW_TITLE 41
	char	window_title[ MAX_WINDOW_TITLE + 1 ];
	int	i;
	int	watch_for_2_null_break;
	char	*prompt;

	/******************************************************************
	* If a popup is active ( e.g. facet_ui ) - deactivate it.
	******************************************************************/
	if ( Overwritten_winno >= 0 )
	{
		ftproc_overwrite_restore();
	}
	/******************************************************************
	* Force hp terminals to a reasonable mode.
	******************************************************************/
	t_sync_keypad_xmit( 1 );
	t_meta_roll_undo();
	/******************************************************************
	* If the command is coming through an ioctl - set up to read from
	* the buffer instead of the keyboard.
	******************************************************************/
	key_read_start();
	Redirected_winno = -1;
	first = 1;
	watch_for_2_null_break = 1;
	/******************************************************************
	* Prompt the user. 
	******************************************************************/
	command = 0;
	show_window_prompt( Curwinno, command );
	while( 1 )
	{
		/**********************************************************
		* Some mux cards like to send two nulls on a break instead
		* of just the one they were told to send.  A second one
		* arriving with in 0.5 seconds is ignored.
		**********************************************************/
		if ( watch_for_2_null_break )
		{
			watch_for_2_null_break = 0;
			c = key_read( 5, Map_window_mode_prompt );
			if ( c == -1 )			/* timeout */
				continue;
			if ( c == 0 )			/* second break */
				continue;
		}
		else
		{
			c = key_read( 0, Map_window_mode_prompt );
		}
		/**********************************************************
		* A hotkey ( ^W ) as the first character sends a hotkey
		* to the window - otherwise it is ignored.
		**********************************************************/
		if ( c == winc )
		{
			if ( first )
			{
				refresh_prompt_area();
				t_cursor();
				key_read_end();
				check_notify_when_current( Curwinno );
				fct_window_mode_ans( 'W' );
				return;
			}
			else
				continue;
		}
		/**********************************************************
		* Normal window mode command.
		**********************************************************/
		first = 0;
		switch( c )
		{
		case '#':
			/**************************************************
			* Some programs ( facet_ui ) need to have a command
			* apply to a different window than the current
			* window.  ^W # <window#> makes most commands
			* apply to <window#>.
			* Stay in window command mode.
			**************************************************/
			refresh_prompt_area();
				/* "Redirect to Window" */
			c = get_command_char( 
					Text_get_command_redirect, "*",
					 Map_get_command_redirect );
			if ( c == -1 )
				break;
			if ( c == '*' )
				winno = Curwinno;
			else if ( ( c >= '1' ) || ( c <= '9' ) )
				winno = c - '1';
			else if ( c == '0' )
				winno = 9;
			else
			{
				term_beep();
				break;
			}
			if ( winno >= Wins )
			{
				term_beep();
				break;
			}
			Redirected_winno = winno;
			refresh_prompt_area();
			show_window_prompt( Curwinno, command );
			continue;
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': case '0':
		case 'l': case 'L':
			/**************************************************
			* A number selects an window and exits window 
			* command mode.
			* "l" or "L" selects the last window active.
			* Exit window command mode.
			**************************************************/
			if ( ( c == 'l' ) || ( c == 'L' ) )
				winno = Last_curwinno;
			else if ( c == '0' )
				winno = 9;
			else
				winno = c - '1';
			if ( winno < Wins )
			{
				/* same window with no change does nothing */
				if ( ( command == 0 ) && ( winno == Curwinno ) )
					break;
				refresh_prompt_area();
				check_notify_when_current( winno );
				ftproc_select( winno, command );
				key_read_end();
				fct_window_mode_ans_curwin();
				return;
			}
			else
			{
				term_beep();
				continue;
			}
		case '\r':			/* command release */
		case FTKEY_RETURN:
			/**************************************************
			* return causes a refresh unless it was preceded
			* by a top, bottom, or full split screen command.
			* Exit window command mode.
			**************************************************/
			refresh_prompt_area();
			redo_curwinno( command );	/* refresh or resize */
			key_read_end();
			check_notify_when_current( Curwinno );
			fct_window_mode_ans_curwin();
			return;
		case '-':
		case FTKEY_LEFT:
		case FTKEY_BACKSPACE:
		case '\b':
			/**************************************************
			* Select the previous window ( lower number wrapping
			* to the top.  Consider only active and not
			* invisible.
			* Exit window command mode.
			**************************************************/
			winno = Curwinno;
			for ( i = 0; i < Wins; i++ )
			{
				winno--;
				if ( winno < 0 )
					winno = Wins - 1;
				if ( Window_active[ winno ] 
				  && ( Window_invisible[ winno ] == 0 ))
				{
					break;
				}
			}
			refresh_prompt_area();
			check_notify_when_current( winno );
			ftproc_select( winno, command );
			command = 0;
			show_window_prompt( Curwinno, command );
			continue;
		case '+':
		case '=':
		case FTKEY_RIGHT:
			/**************************************************
			* Select the next window ( higher number wrapping
			* to the bottom ).  Consider only active and not
			* invisible.
			* Exit window command mode.
			**************************************************/
			winno = Curwinno;
			for ( i = 0; i < Wins; i++ )
			{
				winno++;
				if ( winno >= Wins )
					winno = 0;
				if ( Window_active[ winno ] 
				  && ( Window_invisible[ winno ] == 0 ))
				{
					break;
				}
			}
			refresh_prompt_area();
			check_notify_when_current( winno );
			ftproc_select( winno, command );
			command = 0;
			show_window_prompt( Curwinno, command );
			continue;
		case 'q':
		case 'Q':
			/**************************************************
			* Quit facetterm after possible prompting for
			* confirmation.
			**************************************************/
			if ( Opt_no_quit_prompt )
			{
				term_outgo();
				fsend_kill( "", 0 );
				/* does not return */
			}
			refresh_prompt_area();
			if ( get_first_active_window() >= 0 )
			{
				if ( Opt_disable_quit_while_windows_active )
				{
					/*************************************
					* if command is not from a program, or
					* that program is not the last window,
					* say no.
					*************************************/
					if ( ((winno = get_key_read_pkt_winno())
					      < 0 )
					   ||( is_winno_only_active_window(
								winno ) == 0)
					   )
					{
						term_beep();
						break;
					}
				}
/* "WARNING! Windows active. QUIT FacetTerm ? (Y or N)" */
				prompt = Text_get_command_quit_active;
			}
			else
			{
				/* "QUIT FacetTerm ? (Y or N)" */
				prompt = Text_get_command_quit_idle;
			}
			c = get_command_char( prompt, "",
					Map_get_command_quit_active );
			if ( c == 'y' || c == 'Y' )
			{
				if ( Prompting_on )
				{
					char	promptc;
					char	map_prompt();

					promptc = map_prompt( 'Y',
						Prmt_get_command_quit_active);
					putchar( promptc ); /* INTER */
				}
				term_outgo();
				fsend_kill( "", 0 );
				/* does not return */
			}
			else if ( c == 'n' || c == 'N' || c == -1 )
			{
			}
			else
				term_beep();
			break;
		case 'A':
		case 'a':
			/**************************************************
			* Activate the program for this window specified in
			* the .facet file.
			**************************************************/
			winno = Redirected_winno;
			if ( winno < 0 )
				winno = get_key_read_pkt_winno();
			if ( winno < 0 )
				winno = Curwinno;
			if ( Window_active[ winno ] )
			{
				term_beep();
				break;
			}
			if ( Window_printer[ winno ] )
			{
				term_beep();
				break;
			}
			if ( respawn( winno ) < 0 )
			{
				term_beep();
				break;
			}
			refresh_prompt_area();
			check_notify_when_current( winno );
			if ( winno != Curwinno )
				ftproc_select( winno, command );
			else
				t_cursor();
			key_read_end();
			fct_window_mode_ans_curwin();
			return;
		case 'S':
		case 's':
			/**************************************************
			* Start a shell that runs its profile on an idle
			* window.
			* Select that window.
			* Exit window command mode.
			**************************************************/
			if ( (shell = getenv( "FACETSHELL" )) != NULL )
				strcpy( program, shell );
			else if ( (shell = getenv( "SHELL" )) != NULL )
				sprintf( program, "-%s", shell );
			else
				strcpy( program, "-sh" );
			if ( Window_active[ Curwinno ] )
			{
			    for ( winno = 0; winno < Wins; winno++ )
			    {
				if (  ( Window_active[ winno ] == 0 )
				   && ( Window_printer[ winno ] == 0 ) )
				{
					startwin( winno, "", program );
					refresh_prompt_area();
					check_notify_when_current( 
								winno );
					ftproc_select( winno, command );
					key_read_end();
					fct_window_mode_ans_curwin();
					return;
				}
			    }
			    term_beep();
			    break;
			}
			startwin( Curwinno, "", program );
			break;
		case 'R':
		case 'r':
			/**************************************************
			* Run a program.
			* Prompt for the program name and run it on an idle
			* window.
			* Select the window.
			* Exit window command mode.
			**************************************************/
			if ( Opt_disable_control_W_r )
			{
				term_beep();
				break;
			}
			program[ 0 ] = '\0';
					/* "Run Program" */
			get_name_input( program, MAX_PROG,
					Text_name_input_run_program, 0 );
			blank_trim( program );
			if ( program[ 0 ] != '\0' )
			{
				if ( Window_active[ Curwinno ] )
				{
				    for ( winno = 0; winno < Wins; winno++ )
				    {
					if (  ( Window_active[ winno ] == 0 )
					   && ( Window_printer[ winno ] == 0 ) )
					{
						startwin( winno, "", program );
						refresh_prompt_area();
						check_notify_when_current( 
									winno );
						ftproc_select( winno, command );
						key_read_end();
						fct_window_mode_ans_curwin();
						return;
					}
				    }
				    term_beep();
				    break;
				}
				startwin( Curwinno, "", program );
			}
			break;
		case 'k':
		case 'K':
			/**************************************************
			* Load a key file.
			* Prompt for the key file name, and load it onto
			* the current window.
			* Exit window command mode.
			**************************************************/
			program[ 0 ] = '\0';
							/* "Key File" */
			get_name_input( program, MAX_PROG,
						Text_name_input_key_file, 0 );
			blank_trim( program );
			if ( program[ 0 ] != '\0' )
			{
				winno = Redirected_winno;
				if ( winno < 0 )
					winno = get_key_read_pkt_winno();
				if ( winno < 0 )
					winno = Curwinno;
				load_all_function_keys( program, winno );
			}
			break;
		case 'g':
		case 'G':
			/**************************************************
			* Load a key file globally.
			* Prompt for the key file name, and load it onto
			* all windows.
			* Exit window command mode.
			**************************************************/
			program[ 0 ] = '\0';
							/* "Global Key File" */
			get_name_input( program, MAX_PROG,
					Text_name_input_global_key_file, 0 );
			blank_trim( program );
			if ( program[ 0 ] != '\0' )
				load_all_function_keys( program, -1 );
			break;
		case 'e':
		case 'E':
			/**************************************************
			* Set the "paste end of line type".
			* The terminal description file can have user 
			* selectable substitutions for the normal "newline"
			* added at the end of each line by paste.
			* Prompt the user for the symbolic name and 
			* set it if it is valid.
			* Exit window command mode.
			**************************************************/
			paste_eol_name[ 0 ] = '\0';
				/* "Paste eol type" */
			get_name_input( paste_eol_name, MAX_PASTE_EOL_NAME_LEN,
				Text_name_input_paste_eol_type, 0 );
			blank_trim( paste_eol_name );
			if ( paste_eol_name[ 0 ] != '\0' )
				set_paste_eol( paste_eol_name );
			break;
		case 'm':
		case 'M':
			/**************************************************
			* Allow user to change terminal mode. E.G. transparent
			* print, 7bit to 8bit, pc mode.
			**************************************************/
			get_mode_inputs();
			break;
		case 'W':
		case 'w':
			/**************************************************
			* Allow user to set "windows window".  This is a
			* window to be notified when the user presses break
			* and the break is being sent to facetterm.
			* Exit window command mode.
			**************************************************/
			refresh_prompt_area();
			get_windows_window( &Windows_window );
			break;
		case 'I':
		case 'i':
			/**************************************************
			* Allow user to set "idle window".  This is a window
			* to be made current when the current window goes
			* idle.
			* Exit window command mode.
			**************************************************/
			refresh_prompt_area();
			get_idle_window( &Idle_window );
			break;
		case 'C':
		case 'c':
			/**************************************************
			* Cut a section of the screen to the cut buffer.
			**************************************************/
			refresh_prompt_area();
			get_cut();
			break;
		case 'P':
		case 'p':
			/**************************************************
			* Paste the cut buffer to the current window.
			**************************************************/
			put_paste();
			break;
		case 'O':
		case 'o':
			/**************************************************
			* Write an image of the screen to a temporary file
			* and send the file to .facetprint.
			**************************************************/
			print_window();
			break;
		case ' ':
		case '\033':
			/**************************************************
			* Exit window command mode.
			**************************************************/
			break;
		case 'x':
		case 'X':
			/**************************************************
			* eXtra commands.
			*	c	capture mode
			*	i	invisible to scan
			*	m	monitor mode
			*	n	notify when current
			*	t	transparent mode
			*	b	blocked mode
			*	h	hangup or kill signalling
			*	k	keystroke capture
			*	r	replay keystoke capture
			*	p	printer mode
			*	s	screen saver
			*	l	lock screen
			**************************************************/
			refresh_prompt_area();
				/* "c=capture i=invisible n=notify h=hangup" */
			c = get_command_char( 
					Text_get_command_extra_commands, "",
					 Map_get_command_extra_commands );
			if ( c == 'c' || c == 'C' )
				get_capture_input( Curwinno );
			else if ( c == 'i' ||  c == 'I' )
				get_invisible_input( Curwinno );
			else if ( c == 'm' ||  c == 'M' )
				get_monitor_input( Curwinno );
			else if ( c == 'n' ||  c == 'N' )
				get_notify_input( Curwinno );
			else if ( c == 't' ||  c == 'T' )
				get_transparent_input( Curwinno );
			else if ( c == 'b' ||  c == 'B' )
				get_blocked_input( Curwinno );
			else if ( c == 'h' ||  c == 'H' )
				get_hangup_input();
			else if ( c == 'k' ||  c == 'K' )
				get_keystroke_capture_input();
			else if ( c == 'r' ||  c == 'R' )
				get_keystroke_play_input();
			else if ( c == 'p' ||  c == 'P' )
			{
				winno = get_printer_input( Curwinno );
				if ( winno >= 0 )
				{
					refresh_prompt_area();
					t_cursor();
					key_read_end();
					if ( Window_printer[ winno ] )
					{
						puts_winno(
							Text_printer_mode_on,
							winno );
					}
					else
					{
						puts_winno(
							Text_printer_mode_off,
							winno );
					}
					t_cursor();
					check_notify_when_current( Curwinno );
					fct_window_mode_ans_curwin();
					return;
				}
			}
			else if ( c == 's' ||  c == 'S' )
			{
				if ( get_screen_saver_input() < 0 )
					break;
				screen_saver();
				if ( Curwin->fullscreen )
				{
					tell_full_refresh();
					scroll_memory_refresh();
					full_refresh();
					f_cursor();
				}
				else
				{
					winno = 
					    find_other_split_winno( Curwinno );
					rebuild_split_screen( winno );
				}
				modes_reload_current();
				key_read_end();
				check_notify_when_current( Curwinno );
				fct_window_mode_ans_curwin();
				return;
			}
			else if ( c == 'l' ||  c == 'L' )
			{
				if ( get_screen_lock_input() < 0 )
					break;
				screen_lock();
				if ( Curwin->fullscreen )
				{
					tell_full_refresh();
					scroll_memory_refresh();
					full_refresh();
					f_cursor();
				}
				else
				{
					winno = 
					    find_other_split_winno( Curwinno );
					rebuild_split_screen( winno );
				}
				modes_reload_current();
				key_read_end();
				check_notify_when_current( Curwinno );
				fct_window_mode_ans_curwin();
				return;
			}
			else if ( c == '!'  )
			{
				if ( get_run_raw_input( Curwinno ) < 0 )
					break;
				if ( Curwin->fullscreen )
				{
					rebuild_full_screen();
				}
				else
				{
					winno = 
					    find_other_split_winno( Curwinno );
					rebuild_split_screen( winno );
				}
				modes_reload_current();
				key_read_end();
				check_notify_when_current( Curwinno );
				fct_window_mode_ans_curwin();
				return;
			}
			else if ( c == -1 )
			{
			}
			else
				term_beep();
			break;
		case 'z':
		case 'Z':
			/**************************************************
			* Popup a window over the current window.
			* This is used by facet_ui or other popup programs
			* and is not really useful from the keyboard though
			* it does work.
			**************************************************/
			winno = get_overwrite_window();
			if ( winno >= 0 )
			{
				refresh_prompt_area();
				ftproc_overwrite( winno );
				key_read_end();
				fct_window_mode_ans_curwin();
				return;
			}
			break;
		case 0:
			/**************************************************
			* BREAK received - if break caused window mode then
			* two breaks causes a break on the window.
			**************************************************/
			if ( Opt_use_PARMRK_for_break )
			{
				term_beep();
				break;
			}
			if ( Opt_send_break_to_window == 0 )
			{
				winno = Redirected_winno;
				if ( winno < 0 )
					winno = get_key_read_pkt_winno();
				if ( winno < 0 )
					winno = Curwinno;
				fsend_break( winno );
			}
			break;
		case FTKEY_BREAK:
			/**************************************************
			* BREAK received - if break caused window mode then
			* two breaks causes a break on the window.
			**************************************************/
			if ( Opt_send_break_to_window == 0 )
			{
				winno = Redirected_winno;
				if ( winno < 0 )
					winno = get_key_read_pkt_winno();
				if ( winno < 0 )
					winno = Curwinno;
				fsend_break( winno );
			}
			break;
		case '.':
			/**************************************************
			* Special chars to window
			*	b = send break
			*	q = send Control-Q
			*	s = send Control-S
			*	@ = send Control-@ ( NULL )
			**************************************************/
			winno = Redirected_winno;
			if ( winno < 0 )
				winno = get_key_read_pkt_winno();
			if ( winno < 0 )
				winno = Curwinno;
			refresh_prompt_area();
			   /* "b=break  q=Control-Q  s=Control-S @=Control-@" */
			c = get_command_char( 
					Text_get_command_control_chars, "",
					 Map_get_command_control_chars );
			if ( c == 'b' || c == 'B' )
				fsend_break( winno );
			else if ( c == 'q' || c == 'Q' )
				paste_char_to_window( winno, 
							(char) ( 'Q' - 0x40 ) );
			else if ( c == 's' || c == 'S' )
				paste_char_to_window( winno, 
							(char) ( 'S' - 0x40 ) );
			else if ( c == '@' )
				paste_char_to_window( winno,     '\0' );
			else if ( c == -1 )
			{
			}
			else
				term_beep();
			break;
		case '"':
		case '\'':
			/**************************************************
			* Window title.
			* Prompt and input and store for later retrieval.
			**************************************************/
			window_title[ 0 ] = '\0';
			get_name_input( window_title, MAX_WINDOW_TITLE,
					Text_name_input_window_title, 0 );
			blank_trim( window_title );
			if ( window_title[ 0 ] != '\0' )
			{
				winno = Redirected_winno;
				if ( winno < 0 )
					winno = get_key_read_pkt_winno();
				if ( winno < 0 )
					winno = Curwinno;
				remember_exec_list( winno, window_title );
			}
			break;
		case 'h':
		case 'H':
		case '?':
		case '/':
			/**************************************************
			* Help.
			* Temporarily suppend facetterm and run the help
			* script.
			**************************************************/
			help();
			break;
		case ':': 
		case ';':
			/**************************************************
			* Key mapping
			* h = hotkey  <key> on any window sends <map> to this
			*	      window.
			* m = map     <key> is replaced by <map>
			* u = unmap   <key>
			* f = filename NOT IMPLEMENTED
			**************************************************/
			winno = Redirected_winno;
			if ( winno < 0 )
				winno = get_key_read_pkt_winno();
			if ( winno < 0 )
				winno = Curwinno;
			refresh_prompt_area();
			   /* "h=hotkey m=map f=filename u=unmap" */
			c = get_command_char( 
					Text_get_command_mapped, "",
					 Map_get_command_mapped );
			if ( c == 'h' || c == 'H' )
				get_mapped_key( winno );
			else if ( c == 'm' || c == 'M' )
				get_mapped_key( -1 );
			else if ( c == 'f' || c == 'F' )
				get_mapped_filename( winno );
			else if ( c == 'u' || c == 'U' )
				get_mapped_unmap( winno );
			else if ( c == -1 )
			{
			}
			else
				term_beep();
			break;
		case '>':
			/**************************************************
			* Paste options
			* p	paste to printer
			* s	paste to script
			* f	paste to file which must not exist.
			* a	paste to file append.
			* o	paste to file overwrite if is exists.
			**************************************************/
			if (  ( Paste_buffer_ptr == NULL ) 
			   || ( *Paste_buffer_ptr == '\0' ) )
			{
				term_beep();
				break;
			}
			refresh_prompt_area();
			/* "p=printer s=script f=file a=append o=overwrite" */
			c = get_command_char( 
					Text_get_command_paste_commands, "",
					 Map_get_command_paste_commands );
			if ( c == 'p' || c == 'P' )
				paste_command_printer();
			else if ( c == 's' ||  c == 'S' )
				paste_command_script();
			else if ( c == 'f' ||  c == 'F' )
				paste_command_file( 'F' );
			else if ( c == 'a' ||  c == 'A' )
				paste_command_file( 'A' );
			else if ( c == 'o' ||  c == 'O' )
				paste_command_file( 'O' );
			else if ( c == -1 )
			{
			}
			else
				term_beep();
			break;
		default:
			term_beep();
			continue;
		}
		/**********************************************************
		* Normal exit of window command mode.
		* Put screen and cursor back.
		* If reading from ioctl buffer - terminate it.
		* See if current window wants notify when current.
		* Tell receiver current window number and go.
		**********************************************************/
		refresh_prompt_area();
		t_cursor();
		key_read_end();
		check_notify_when_current( Curwinno );
		fct_window_mode_ans_curwin();
		return;
	}
}
/**************************************************************************
* string_ends_with
*	Return 1 if the string "string" ends with the character string "test".
*	Return 0 otherwise.
**************************************************************************/
string_ends_with( string, test )	/* 1 = yes */
	char	*string;
	char	*test;
{
	int	len_string;
	int	len_test;
	int	start;

	len_string = strlen( string );
	len_test = strlen( test );
	if ( len_string < len_test )
		return( 0 );
	start = len_string - len_test;
	if ( strcmp( &string[ start ], test ) == 0 )
		return( 1 );
	else
		return( 0 );
}
/**************************************************************************
* set_paste_eol
*	Set the "paste end of line type".
*	The terminal description file can have user selectable substitutions
*	for the normal "newline" added at the end of each line by paste.
*	"paste_eol_name" is a symbolic type entered by the user with an
*	optional window number preceding it.
*	Set the paste_eol_type if it is valid.
**************************************************************************/
set_paste_eol( paste_eol_name )
	char	*paste_eol_name;
{
	int	i;
	FT_WIN	*win;
	int	winno;
	char	*name;

	win = Curwin;
	name = paste_eol_name;
	if ( ( *name >= '1' ) && ( *name <= '9' ) )
	{
		win = Wininfo[ *name - '1' ];
		name++;
	}
	else if ( *name == '0' )
	{
		win = Wininfo[ 9 ];
		name++;
	}
	else if ( Redirected_winno >= 0 )
		win = Wininfo[ Redirected_winno ];
	else if ( ( winno = get_key_read_pkt_winno() ) >= 0 )
	{
		win = Wininfo[ winno ];
	}
	if ( strcmp( name, "DEFAULT" ) == 0 )
	{
		strcpy( win->paste_end_of_line, "\r\0" );
		return;
	}
	for ( i = 0; i < T_paste_eol_no; i++ )
	{
		if ( strcmp( name, T_paste_eol_name[ i ] ) == 0 )
		{
			strncpy( win->paste_end_of_line, T_paste_eol[ i ],
				 MAX_PASTE_END_OF_LINE );
			win->paste_end_of_line[ MAX_PASTE_END_OF_LINE ] = '\0';
			return;
		}
	}
	term_beep();
}
/* 	"12345678901234567890123456789012345678901234" */
char	*Text_block_cut_and_paste_first =
	"Block  CUT  RETURN=corner 1  SPACE=quit";
char	*Text_block_cut_and_paste_second =
	"Block  CUT  RETURN=corner 2  SPACE=quit";
char	*Text_stream_cut_and_paste_first =
	"Stream CUT  RETURN=corner 1  SPACE=quit";
char	*Text_stream_cut_and_paste_second =
	"Stream CUT  RETURN=corner 2  SPACE=quit";
char	*Text_wrap_cut_and_paste_first =
	"Wrap   CUT  RETURN=corner 1  SPACE=quit";
char	*Text_wrap_cut_and_paste_second =
	"Wrap   CUT  RETURN=corner 2  SPACE=quit";
char	*Text_cut_and_paste_paste =
	"PASTING";
char	*Map_cut_and_paste = "";
/**************************************************************************
* get_cut
*	^W c
*	Determine the type of cut and the area of the cut and store the
*	cut characters in the cut buffer.
**************************************************************************/
get_cut()
{
	int	cut_rows[ 2 ];
	int	cut_cols[ 2 ];
		/**********************************************************
		* Have we ever marked the first corner?
		* If we haven't then the second corner tracks the first.
		**********************************************************/
	int	corners_are_together;
	int	cursor_is_corner_2;
	int	c;
	int	time_out;
	int	show_count;
	int	side_count;
	int	i;

	corners_are_together = 1;
	cursor_is_corner_2 = 0;
	prompt_for_cut( cursor_is_corner_2 );
	cut_rows[ 0 ] = Curwin->row;
	cut_cols[ 0 ] = Curwin->col;
	time_out = 0;
	show_count = 0;
	side_count = 0;
	while( 1 )
	{
		if ( corners_are_together )
		{
			cut_rows[ 1 ] = cut_rows[ 0 ];
			cut_cols[ 1 ] = cut_cols[ 0 ];
		}
		if ( time_out == 0 )
		{
			win_pos_w_pan( Curwin, cut_rows[ cursor_is_corner_2 ], 
					       cut_cols[ cursor_is_corner_2 ] );
			show_count = 0;
		}
		else
		{
			if (  ( Cut_type == STREAM_CUT_TYPE )
			   || ( Cut_type == WRAP_CUT_TYPE   ) )
			{
				show_count++;
				if ( show_count > 1 )
					show_count = 0;
				show_other_corner( cursor_is_corner_2,
						   show_count,
						   cut_rows, cut_cols );
			}
			else if (  ( cut_rows[ 0 ] == cut_rows[ 1 ] )
			        || ( cut_cols[ 0 ] == cut_cols[ 1 ] ) )
			{
				show_count++;
				if ( show_count > 1 )
					show_count = 0;
				show_other_corner( cursor_is_corner_2,
						   show_count,
						   cut_rows, cut_cols );
			}
			else
			{
				show_count++;
				if ( show_count > 3 )
					show_count = 0;
				show_box_corner( cursor_is_corner_2, 
						 show_count,
						 cut_rows, cut_cols );
			}
		}
		time_out = 0;
		/**********************************************************
		* Must time out Cut_and_paste_timer times to count.
		* If Cut_and_paste_timer is 0, it does not time out.
		* Timing is left at 5 to match function key gathering -
		* switching in and out of timing is too slow, especially HP.
		**********************************************************/
		for ( i = 0;
		      (i < Cut_and_paste_timer) || (Cut_and_paste_timer == 0);
		      i++ )
		{
			c = key_read( 5, Map_cut_and_paste );
			if ( c != -1 )
				break;
			/**************************************************
			* In case this cut was started by ioctl and now
			* reading keyboard.
			**************************************************/
			if ( corners_are_together )
				win_pos_w_pan( Curwin, cut_rows[ 0 ],
						       cut_cols[ 0 ] );
		}
		if ( c == 'c' )
		{
			corners_are_together = 0;
			if ( cursor_is_corner_2 == 0 )
				cursor_is_corner_2 = 1;
			else
				cursor_is_corner_2 = 0;
			prompt_for_cut( cursor_is_corner_2 );
		}
		else if ( ( c == 'o' ) || ( c == 'O' ) )
		{
			if (  ( Cut_type == STREAM_CUT_TYPE )
			   || ( Cut_type == WRAP_CUT_TYPE ) )
			{
				show_outline_stream( cursor_is_corner_2,
						     cut_rows, cut_cols );
			}
			else
			{
				show_outline_box( cursor_is_corner_2,
						  cut_rows, cut_cols );
			}
		}
		else if (  ( ( c == 'u' ) || ( c == FTKEY_UP ) )
		     && ( cut_rows[ cursor_is_corner_2 ] > 0 ) )
		{
			cut_rows[ cursor_is_corner_2 ]--;
		}
		else if ( c == 'U' )
		{
			cut_rows[ cursor_is_corner_2 ] = 0;
		}
		else if (  (  ( c == 'd' ) 
			|| ( c == FTKEY_DOWN )
			|| ( c == '\n' ) )
			&& ( cut_rows[ cursor_is_corner_2 ] <
			     Curwin->display_row_bottom ) )
		{
				cut_rows[ cursor_is_corner_2 ]++;
		}
		else if ( c == 'D' )
		{
			cut_rows[ cursor_is_corner_2 ] = 
						Curwin->display_row_bottom;
		}
		else if (  (  ( c == 'l' ) 
			   || ( c == FTKEY_LEFT )
			   || ( c == FTKEY_BACKSPACE )
			   || ( c == '\b' ) )
			&& ( cut_cols[ cursor_is_corner_2 ] > 0 ) )
		{
				cut_cols[ cursor_is_corner_2 ]--;
		}
		else if ( c == 'L' )
		{
			cut_cols[ cursor_is_corner_2 ] = 0;
		}
		else if (  ( ( c == 'r' ) || ( c == FTKEY_RIGHT ) )
			&& ( cut_cols[ cursor_is_corner_2 ] < 
			     Curwin->col_right ) )
		{
			cut_cols[ cursor_is_corner_2 ]++;
		}
		else if ( c == 'R' )
		{
			cut_cols[ cursor_is_corner_2 ] = Curwin->col_right;
		}
		else if ( c == FTKEY_HOME)
		{
			cut_rows[ cursor_is_corner_2 ] = 0;
			cut_cols[ cursor_is_corner_2 ] = 0;
		}
		else if ( ( c == 'm' ) || ( c == 'M' ) )
		{
			if ( cursor_is_corner_2 == 0 )
			{
				corners_are_together = 0;
				cursor_is_corner_2 = 1;
				prompt_for_cut( cursor_is_corner_2 );
			}
			else
			{
				cut_rows[ 0 ] = cut_rows[ 1 ];
				cut_cols[ 0 ] = cut_cols[ 1 ];
			}
		}
		else if ( ( c == '\r' ) || ( c == FTKEY_RETURN ) )
		{
			if ( cursor_is_corner_2 == 0 )
			{
				corners_are_together = 0;
				cursor_is_corner_2 = 1;
				prompt_for_cut( cursor_is_corner_2 );
			}
			else
				break;
		}
		else if ( ( c == 'a' ) || ( c == 'A' ) )
		{
			cut_rows[ 0 ] = 0;
			cut_cols[ 0 ] = 0;
			cut_rows[ 1 ] = Curwin->display_row_bottom;
			cut_cols[ 1 ] = Curwin->col_right;
			corners_are_together = 0;
			cursor_is_corner_2 = 1;
			prompt_for_cut( cursor_is_corner_2 );
		}
		else if ( ( c == 'e' ) || ( c == 'E' ) )
		{
			if ( cursor_is_corner_2 )
			{
				cut_rows[ 0 ] = cut_rows[ 1 ];
				cut_cols[ 0 ] = cut_cols[ 1 ];
			}
			else
				cut_rows[ 1 ] = cut_rows[ 0 ];
			cut_cols[ 1 ] = Curwin->col_right;
			corners_are_together = 0;
			cursor_is_corner_2 = 1;
			prompt_for_cut( cursor_is_corner_2 );
		}
		else if ( ( c == 's' ) || ( c == 'S' ) )
		{
			Cut_type = STREAM_CUT_TYPE;
			prompt_for_cut( cursor_is_corner_2 );
		}
		else if ( ( c == 'w' ) || ( c == 'W' ) )
		{
			Cut_type = WRAP_CUT_TYPE;
			prompt_for_cut( cursor_is_corner_2 );
		}
		else if ( ( c == 'b' ) || ( c == 'B' ) )
		{
			Cut_type = BLOCK_CUT_TYPE;
			prompt_for_cut( cursor_is_corner_2 );
		}
		else if (  ( c == '\033' )
			|| ( c == ' ' )
			|| ( c == 'q' ) || ( c == 'Q' )
			|| ( c == FTKEY_BREAK )
			)
		{
			win_pos_w_pan( Curwin, Curwin->row, Curwin->col );
			return( -1 );
		}
		else if ( c == -1 )
		{
			time_out = 1;
		}
		else
		{
			term_beep();
		}
	}
	if ( Cut_type == STREAM_CUT_TYPE )
		save_paste_stream( Curwin, cut_rows, cut_cols );
	else if ( Cut_type == WRAP_CUT_TYPE )
		save_paste_wrap( Curwin, cut_rows, cut_cols );
	else
		save_paste_block( Curwin, cut_rows, cut_cols );
	win_pos_w_pan( Curwin, Curwin->row, Curwin->col );
	return( 0 );
}
/**************************************************************************
* prompt_for_cut
*	Display cut prompt to user choosing appropriate prompt based on
*	"cursor_is_corner_2" which is 0 when the cursor is currently on
*	corner 1 and is >0 when the cursor is on corner 2.
**************************************************************************/
prompt_for_cut( cursor_is_corner_2 )
	int	cursor_is_corner_2;
{
	if ( cursor_is_corner_2 == 0 )
	{
		if ( Cut_type == STREAM_CUT_TYPE )
			show_command_prompt( Curwinno, 
					Text_stream_cut_and_paste_first );
		else if ( Cut_type == WRAP_CUT_TYPE )
			show_command_prompt( Curwinno, 
					Text_wrap_cut_and_paste_first );
		else
			show_command_prompt( Curwinno, 
					Text_block_cut_and_paste_first );
	}
	else
	{
		if ( Cut_type == STREAM_CUT_TYPE )
			show_command_prompt( Curwinno,
					Text_stream_cut_and_paste_second );
		else if ( Cut_type == WRAP_CUT_TYPE )
			show_command_prompt( Curwinno,
					Text_wrap_cut_and_paste_second );
		else
			show_command_prompt( Curwinno,
					Text_block_cut_and_paste_second );
	}
}
/**************************************************************************
* show_outline_stream
*	Trace the outline of a "stream" cut which is 
*		the right part of the top line
*		all the lines between
*		the left part of the last line
*	The cursor is currently on corner 2 when "cursor_is_corner_2" is
*	> 0.  The corners are specified by the arrays "cut_rows" and
*	"cut_cols".
**************************************************************************/
show_outline_stream( cursor_is_corner_2, cut_rows, cut_cols )
	int	cursor_is_corner_2;
	int	cut_rows[ 2 ];
	int	cut_cols[ 2 ];
{
	int	top_row;
	int	top_col;
	int	bot_row;
	int	bot_col;

	if ( cut_rows[ 0 ] == cut_rows[ 1 ] )
	{
		if ( cut_cols[ 0 ] < cut_cols[ 1 ] )
		{
			draw_cursor_line( 
				cut_rows[ 0 ],		cut_cols[ 0 ],
				cut_rows[ 1 ],		cut_cols[ 1 ] );
		}
		else
		{
			draw_cursor_line( 
				cut_rows[ 1 ],		cut_cols[ 1 ],
				cut_rows[ 0 ],		cut_cols[ 0 ] );
		}
		return;
	}
	if ( cut_rows[ 0 ] < cut_rows[ 1 ] )
	{
		top_row = cut_rows[ 0 ];
		top_col = cut_cols[ 0 ];
		bot_row = cut_rows[ 1 ];
		bot_col = cut_cols[ 1 ];
	}
	else
	{
		bot_row = cut_rows[ 0 ];
		bot_col = cut_cols[ 0 ];
		top_row = cut_rows[ 1 ];
		top_col = cut_cols[ 1 ];
	}
	if ( bot_row <= top_row + 2 )
	{
		draw_cursor_line(	top_row,	top_col,
					top_row,	Curwin->col_right );
		if ( bot_row == top_row + 2 )
		    draw_cursor_line(	top_row + 1,	0,
					top_row + 1,	Curwin->col_right );
		draw_cursor_line(	bot_row,	0,
					bot_row,	bot_col );
		return;
	}
	draw_cursor_line(		top_row,	top_col,
					top_row,	Curwin->col_right );
	draw_cursor_line(		top_row,	Curwin->col_right,
					bot_row - 1,	Curwin->col_right );
	draw_cursor_line(		bot_row - 1,	Curwin->col_right,
					bot_row - 1,	bot_col );
	draw_cursor_line(		bot_row,	bot_col,
					bot_row,	0);
	draw_cursor_line(		bot_row,	0,
					top_row + 1,	0);
	draw_cursor_line(		top_row + 1,	0,
					top_row + 1,	top_col );
}
/**************************************************************************
* show_outline_box
*	Trace the outline of a "box" cut which is 
*		the rectangle formed by the two corners.
*	The cursor is currently on corner 2 when "cursor_is_corner_2" is
*	> 0.  The corners are specified by the arrays "cut_rows" and
*	"cut_cols".
**************************************************************************/
show_outline_box( cursor_is_corner_2, cut_rows, cut_cols )
	int	cursor_is_corner_2;
	int	cut_rows[ 2 ];
	int	cut_cols[ 2 ];
{
	int	i;
	int	show_row[ 4 ];
	int	show_col[ 4 ];

	for ( i = 0; i < 4; i++ )
	{
		calc_box_corner( cursor_is_corner_2, i, cut_rows, cut_cols,
					&show_row[ i ], &show_col[ i ] );
	}
	draw_cursor_line( show_row[ 0 ], show_col[ 0 ],
			  show_row[ 1 ], show_col[ 1 ] );
	draw_cursor_line( show_row[ 1 ], show_col[ 1 ],
			  show_row[ 2 ], show_col[ 2 ] );
	draw_cursor_line( show_row[ 2 ], show_col[ 2 ],
			  show_row[ 3 ], show_col[ 3 ] );
	draw_cursor_line( show_row[ 3 ], show_col[ 3 ],
			  show_row[ 0 ], show_col[ 0 ] );
}
/**************************************************************************
* draw_cursor_line
*	Move the cursor to every position from the point
*	"row0" and "col0"  to the point  "row1" and "col1".
*	It is assumed that this is a horizontal or vertical line
*	so "col1" is ignored if "row0" is not equal to "row1".
*	On a split screen, the cursor is restricted to the visible parts
*	of the window.
**************************************************************************/
draw_cursor_line( row0, col0, row1, col1 )
	int	row0;
	int	col0;
	int	row1;
	int	col1;
{
	int	row;
	int	col;

	if ( row0 == row1 )
	{
		if ( col0 < col1 )
		{
			for ( col = col0; col <= col1; col++ )
			{
				win_pos_limited( Curwin, row0, col );
				term_outgo();
				term_drain();
			}
		}
		else
		{
			for ( col = col0; col >= col1; col-- )
			{
				win_pos_limited( Curwin, row0, col );
				term_outgo();
				term_drain();
			}
		}
	}
	else if ( row0 < row1 )
	{
		for ( row = row0; row <= row1; row++ )
		{
			win_pos_limited( Curwin, row, col0 );
			term_outgo();
			term_drain();
		}
	}
	else
	{
		for ( row = row0; row >= row1; row-- )
		{
			win_pos_limited( Curwin, row, col0 );
			term_outgo();
			term_drain();
		}
	}
}
/**************************************************************************
* show_other_corner
*	Move the cursor to a sequenced portion of the area being cut.
*	"cursor_is_corner_2" is > 0 if the cursor that the user is moving
*	is displaying corner 2.
*	"show_count" is the sequence number.
*	"cut_rows" and "cut_cols" hold the rows and columns of corner 1 and
*	corner 2.
*	Display the current corner when "show_count" is 0 and the other
*	corner when "show_count" is 1.
**************************************************************************/
show_other_corner( cursor_is_corner_2, show_count, cut_rows, cut_cols )
	int	cursor_is_corner_2;
	int	show_count;
	int	cut_rows[ 2 ];
	int	cut_cols[ 2 ];
{
	int	show_corner;

	if (  ( cursor_is_corner_2 == 0 )
	   && ( show_count == 0 ) )
	{
		show_corner = 0;
	}
	else if (  ( cursor_is_corner_2 )
	&& ( show_count ) )
	{
		show_corner = 0;
	}
	else
		show_corner = 1;
	win_pos_limited( Curwin,
			 cut_rows[ show_corner ], 
			 cut_cols[ show_corner ] );
}
/**************************************************************************
* show_box_corner
*	Move the cursor to a sequenced portion of the area being cut.
*	"cursor_is_corner_2" is > 0 if the cursor that the user is moving
*	is displaying corner 2.
*	"show_count" is the sequence number.
*	"cut_rows" and "cut_cols" hold the rows and columns of corner 1 and
*	corner 2.
*	Move cursor clockwise where "show_count" = 0 is the current cursor.
**************************************************************************/
show_box_corner( cursor_is_corner_2, show_count, cut_rows, cut_cols )
	int	cursor_is_corner_2;
	int	show_count;
	int	cut_rows[ 2 ];
	int	cut_cols[ 2 ];
{
	int	show_row;
	int	show_col;

	calc_box_corner( cursor_is_corner_2, show_count, cut_rows, cut_cols,
					&show_row, &show_col );
	win_pos_limited( Curwin, show_row, show_col );
}
/**************************************************************************
* calc_box_corner
*	Calculate a cursor position for a sequenced portion of the area being
*	cut.
*	"cursor_is_corner_2" is > 0 if the cursor that the user is moving
*	is displaying corner 2.
*	"show_count" is the sequence number."show_count" = 0 is the current
*	cursor. "show_count" = 1 is the next corner clockwise, etc.
*	"cut_rows" and "cut_cols" hold the rows and columns of corner 1 and
*	corner 2.
*	Return the calculated position at the integers pointed to by 
*	"p_show_row" and "p_show_col".
**************************************************************************/
calc_box_corner( cursor_is_corner_2, show_count, cut_rows, cut_cols,
					p_show_row, p_show_col )
	int	cursor_is_corner_2;
	int	show_count;
	int	cut_rows[ 2 ];
	int	cut_cols[ 2 ];
	int	*p_show_row;
	int	*p_show_col;
{
	int	top_row;
	int	bottom_row;
	int	left_col;
	int	right_col;
	int	current_row;
	int	current_col;
	int	offset;
	int	show_row;
	int	show_col;

	if ( cut_rows[ 0 ] < cut_rows[ 1 ] )
	{
		top_row = cut_rows[ 0 ];
		bottom_row = cut_rows[ 1 ];
	}
	else
	{
		top_row = cut_rows[ 1 ];
		bottom_row = cut_rows[ 0 ];
	}
	if ( cut_cols[ 0 ] < cut_cols[ 1 ] )
	{
		left_col = cut_cols[ 0 ];
		right_col = cut_cols[ 1 ];
	}
	else
	{
		left_col = cut_cols[ 1 ];
		right_col = cut_cols[ 0 ];
	}
	current_row = cut_rows[ cursor_is_corner_2 ];
	current_col = cut_cols[ cursor_is_corner_2 ];
	if ( current_row == top_row )
	{
		if ( current_col == left_col )
			offset = 0;
		else
			offset = 1;
	}
	else
	{
		if ( current_col == right_col )
			offset = 2;
		else
			offset = 3;
	}
	switch ( offset + show_count )
	{
	case 1:
	case 5:
		show_row = top_row;
		show_col = right_col;
		break;
	case 2:
	case 6:
		show_row = bottom_row;
		show_col = right_col;
		break;
	case 3:
		show_row = bottom_row;
		show_col = left_col;
		break;
	default:
		show_row = top_row;
		show_col = left_col;
		break;
	}
	*p_show_row = show_row;
	*p_show_col = show_col;
}
/**************************************************************************
* save_paste_block
*	Store the characters on window "window" in the rectangle specified
*	by the corners in "cut_rows" and "cut_cols" into the cut buffer.
*	Trailing blanks are removed.
*	The  ends of the lines are marked with 0xFF.
**************************************************************************/
save_paste_block( window, cut_rows, cut_cols )
	FT_WIN	*window;
	int	cut_rows[];
	int	cut_cols[];
{
	int	t_row;
	int	b_row;
	int	l_col;
	int	r_col;
	int	row;
	int	col;
	FTCHAR	*p;
	char	c;
	FTCHAR	c_w_attr;
	char	*d;
	int	blankcount;
	int	i;


	d = Paste_buffer_ptr;
	if ( cut_rows[ 0 ] <= cut_rows[ 1 ] )
	{
		t_row = cut_rows[ 0 ];
		b_row = cut_rows[ 1 ];
	}
	else
	{
		t_row = cut_rows[ 1 ];
		b_row = cut_rows[ 0 ];
	}
	if ( cut_cols[ 0 ] <= cut_cols[ 1 ] )
	{
		l_col = cut_cols[ 0 ];
		r_col = cut_cols[ 1 ];
	}
	else
	{
		l_col = cut_cols[ 1 ];
		r_col = cut_cols[ 0 ];
	}
	for ( row = t_row; row <= b_row; row++ )
	{
		blankcount = 0;
		p = &window->ftchars[ row ][ l_col ];
		for ( col = l_col; col <= r_col; col++ )
		{
					/* if magic cookie, the character is
					   an irrelevant index into the magic
					   array - likely to be an unprintable
					   control char.  Force to blank.
					*/
			c_w_attr = *p;
			c = (char) *p++;
			if ( c_w_attr & ATTR_MAGIC )
			{
				c = ' ';
			}
			if ( c == ' ' )
				blankcount++;
			else
			{
				for ( i = 0; i < blankcount; i++ )
					*d++ = ' ';
				blankcount = 0;
				*d++ = c;
			}
		}
		if ( row != b_row )
			*d++ = 0xFF;
	}
	*d++ = '\0';
}
/**************************************************************************
* save_paste_stream
*	Store the characters on window "window" specified by the corners
*	in "cut_rows" and "cut_cols" into the cut buffer.
*	This is the character at the top corner and all characters to the
*	right of it on the top row; all characters on all rows
*	between the corners; and all characters to the left of and at the
*	corner on the bottom row.
*	Trailing blanks are removed.
*	The  ends of the lines are marked with 0xFF.
**************************************************************************/
save_paste_stream( window, cut_rows, cut_cols )
	FT_WIN	*window;
	int	cut_rows[];
	int	cut_cols[];
{
	int	t_row;
	int	b_row;
	int	t_col;
	int	b_col;
	int	l_col;
	int	r_col;
	int	row;
	int	col;
	FTCHAR	*p;
	char	c;
	FTCHAR	c_w_attr;
	char	*d;
	int	blankcount;
	int	i;


	d = Paste_buffer_ptr;
	if ( cut_rows[ 0 ] <= cut_rows[ 1 ] )
	{
		t_row = cut_rows[ 0 ];
		b_row = cut_rows[ 1 ];
		t_col = cut_cols[ 0 ];
		b_col = cut_cols[ 1 ];
	}
	else
	{
		t_row = cut_rows[ 1 ];
		b_row = cut_rows[ 0 ];
		t_col = cut_cols[ 1 ];
		b_col = cut_cols[ 0 ];
	}
	for ( row = t_row; row <= b_row; row++ )
	{
		blankcount = 0;
		if ( row == t_row )
			l_col = t_col;
		else
			l_col = 0;
		if ( row == b_row )
			r_col = b_col;
		else
			r_col = Curwin->col_right;
		p = &window->ftchars[ row ][ l_col ];
		for ( col = l_col; col <= r_col; col++ )
		{
					/* if magic cookie, the character is
					   an irrelevant index into the magic
					   array - likely to be an unprintable
					   control char.  Force to blank.
					*/
			c_w_attr = *p;
			c = (char) *p++;
			if ( c_w_attr & ATTR_MAGIC )
			{
				c = ' ';
			}
			if ( c == ' ' )
				blankcount++;
			else
			{
				for ( i = 0; i < blankcount; i++ )
					*d++ = ' ';
				blankcount = 0;
				*d++ = c;
			}
		}
		if ( row != b_row )
			*d++ = 0xFF;
	}
	*d++ = '\0';
}
/**************************************************************************
* save_paste_wrap
*	Store the characters on window "window" specified by the corners
*	in "cut_rows" and "cut_cols" into the cut buffer.
*	This is the character at the top corner and all characters to the
*	right of it on the top row; all characters on all rows
*	between the corners; and all characters to the left of and at the
*	corner on the bottom row.
*	The ends of the lines are not marked.
*	If blanks exist at the end of a line and/or at the beginning of the
*	next line, they will all be replaced by a single blank.  If no
*	blanks exist at either place, no blank is inserted.
**************************************************************************/
save_paste_wrap( window, cut_rows, cut_cols )
	FT_WIN	*window;
	int	cut_rows[];
	int	cut_cols[];
{
	int	t_row;
	int	b_row;
	int	t_col;
	int	b_col;
	int	l_col;
	int	r_col;
	int	row;
	int	col;
	FTCHAR	*p;
	char	c;
	FTCHAR	c_w_attr;
	char	*d;
	int	blankcount;
	int	i;
	int	suppress_leading_blanks;
	int	compress_blanks;


	d = Paste_buffer_ptr;
	if ( cut_rows[ 0 ] <= cut_rows[ 1 ] )
	{
		t_row = cut_rows[ 0 ];
		b_row = cut_rows[ 1 ];
		t_col = cut_cols[ 0 ];
		b_col = cut_cols[ 1 ];
	}
	else
	{
		t_row = cut_rows[ 1 ];
		b_row = cut_rows[ 0 ];
		t_col = cut_cols[ 1 ];
		b_col = cut_cols[ 0 ];
	}
	suppress_leading_blanks = 0;
	for ( row = t_row; row <= b_row; row++ )
	{
		blankcount = 0;
		if ( row == t_row )
		{
			l_col = t_col;
			compress_blanks = 0;
		}
		else
		{
			l_col = 0;
			compress_blanks = 1;
		}
		if ( row == b_row )
			r_col = b_col;
		else
			r_col = Curwin->col_right;
		p = &window->ftchars[ row ][ l_col ];
		for ( col = l_col; col <= r_col; col++ )
		{
					/* if magic cookie, the character is
					   an irrelevant index into the magic
					   array - likely to be an unprintable
					   control char.  Force to blank.
					*/
			c_w_attr = *p;
			c = (char) *p++;
			if ( c_w_attr & ATTR_MAGIC )
			{
				c = ' ';
			}
			if ( c == ' ' )
			{
				if ( compress_blanks )
				{
					if ( suppress_leading_blanks == 0 )
						blankcount = 1;
				}
				else
					blankcount++;
			}
			else
			{
				for ( i = 0; i < blankcount; i++ )
					*d++ = ' ';
				blankcount = 0;
				*d++ = c;
				compress_blanks = 0;
			}
		}
		if ( ( row != b_row ) && ( blankcount > 0 ) )
		{
			*d++ = ' ';
			suppress_leading_blanks = 1;
		}
		else
			suppress_leading_blanks = 0;
	}
	*d++ = '\0';
}
/**************************************************************************
* put_paste
*	Send the contents of the paste buffer to the receiver to be 
*	typed into the current window.  The 0xFF 'end of line' markers
*	are replaced by the current  paste end of line string which
*	defaults to a newline.
**************************************************************************/
put_paste()
{
	char	c;
	char	*d;
	char	*paste_end_of_line;

	if ( ( Paste_buffer_ptr == NULL ) || ( *Paste_buffer_ptr == '\0' ) )
	{
		term_beep();
		return( -1 );
	}
	show_command_prompt( Curwinno, Text_cut_and_paste_paste );
	d = Paste_buffer_ptr;
	fct_window_mode_ans( (char) 'p' );
	fct_window_mode_ans_curwin();
	while( 1 )
	{
		c = *d++;
		if ( (c & 0x0FF) == 0x0FF )
		{
			paste_end_of_line = 
				Wininfo[ Curwinno ]->paste_end_of_line;
			while ( (c = *paste_end_of_line++) != '\0' )
				fct_window_mode_ans( c );
		}
		else
		{
			fct_window_mode_ans( c );
			if ( c == '\0' )
				break;
		}
	}
	return( 0 );
}
/**************************************************************************
* process_ioctl_paste
*	Paste the string "buff" into window number "in_winno" if the
*	first char of "buff" is "*" or to the window specified if the
*	first char of "buff" is "0" to "9".
*	This is a request from a program with a FIOC_PASTE ioctl.
**************************************************************************/
char	*Paste_ptr = (char *) 0;
int	Paste_winno = -1;
process_ioctl_paste( buff, in_winno )
	char	*buff;
	int	in_winno;
{
	char	c;
	int	winno;

	c = buff[ 0 ];
	if ( c == '*' )
	{
		winno = in_winno;
	}
	else if ( ( c >= '0' ) && ( c <= '9' ) )
	{
		if ( c == '0' )
			winno = 9;
		else
			winno = c - '1';
	}
	else
		return( -1 );
	paste_receiver_not_waiting( &buff[ 1 ], winno );
	return( 0 );
}
/**************************************************************************
* paste_receiver_not_waiting
*	Paste the string "buff" into window number "winno".
*	This is done by storing the information in Globals, sending a
*	signal to the receiver, and waiting for the response.
*	The actual paste is done by check_paste_pending below which is
*	called below fsend_get_acks_only when the receiver acks the signal.
**************************************************************************/
paste_receiver_not_waiting( buff, winno )
	char	*buff;
	int	winno;
{
	Paste_winno = winno;
	Paste_ptr = buff;
	send_packet_signal();
	while( Paste_winno >= 0 )
		fsend_get_acks_only();
	return( 0 );
}
/**************************************************************************
* check_paste_pending
*	Are we waiting to to a paste to a window in response to 
*	"process_ioctl_paste" which is called by an ioctl from an 
*	application.
*	E.g. a Control-F from facet_ui when two Control-F s are pressed.
* Returns
*	0 = It wasn't "process_ioctl_paste"
*	1 = It was "process_ioctl_paste and paste is complete.
**************************************************************************/
check_paste_pending()
{
	if ( Paste_winno < 0 )
		return( 0 );
	paste_string_to_winno( Paste_winno, Paste_ptr );
	fct_window_mode_ans_curwin();
	Paste_winno = -1;
	return( 1 );
}
/**************************************************************************
* paste_string_to_winno
*	Send command to the receiver to paste the string "string" to
*	window number "winno".  Replace any end of line indicators ( 0xFF )
*	with the current paste end of line string which defaults to a
*	newline.
**************************************************************************/
paste_string_to_winno( winno, string )
	int	winno;
	char	*string;
{
	char	*d;
	char	*paste_end_of_line;
	char	c;

	d = string;
	fct_window_mode_ans( (char) 'p' );
	fct_window_mode_ans_winno( winno );
	while( 1 )
	{
		c = *d++;
		if ( (c & 0x0FF) == 0x0FF )
		{
			paste_end_of_line = 
				Wininfo[ winno ]->paste_end_of_line;
			while ( (c = *paste_end_of_line++) != '\0' )
				fct_window_mode_ans( c );
		}
		else
		{
			fct_window_mode_ans( c );
			if ( c == '\0' )
				break;
		}
	}
}
/**************************************************************************
* put_paste_file
*	Write the contents of the paste buffer to the file "filename"
*	which is to be opened as a stream file with open mode "filemode".
*	Replace any end of line indicators ( 0xFF ) with the current paste
*	end of line string which defaults to a newline.
**************************************************************************/
put_paste_file( filename, filemode )
	char	*filename;
	char	*filemode;
{
	FILE	*file;
	char	c;
	char	*d;

	if ( ( Paste_buffer_ptr == NULL ) || ( *Paste_buffer_ptr == '\0' ) )
		return( -1 );
	show_command_prompt( Curwinno, Text_cut_and_paste_paste );
	file = fopen( filename, filemode );
	if ( file == NULL )
		return( -1 );
	d = Paste_buffer_ptr;
	while( (c = *d++) != '\0' )
	{
		if ( (c & 0x0FF) == 0x0FF )
			putc( '\n', file );
		else
			putc( c, file );
	}
	putc( '\n', file );
	fclose( file );
	chmod( filename, 0666 );
	return( 0 );
}
/**************************************************************************
* paste_char_to_window
*	Send a command to the receiver to paste the character "c" to
*	window number "winno".
**************************************************************************/
paste_char_to_window( winno, c )
	int	winno;
	char	c;
{
	fct_window_mode_ans( (char) 'P' );
	fct_window_mode_ans_winno( winno );
	fct_window_mode_ans( c );
}
/**************************************************************************
* print_window
*	Write the characters on the current window to a temporary file
*	and execute the script .facetprint, passing it the name of the
*	temporary file.
**************************************************************************/
print_window()
{
	int	row;
	int	col;
	FTCHAR	*p;
	char	c;
	FTCHAR	c_w_attr;
	int	blankcount;
	int	i;
	char	command[ 256 ];
	char	filename[ L_tmpnam ];
	char	filenamehex[ L_tmpnam ];
	FILE	*screen;
	FILE	*screenhex;
	FT_WIN	*win;
	char	dotfacetprintname[ 20 ];

	sprintf( dotfacetprintname, ".%sprint", Facetname );
	if ( generate_script_command( dotfacetprintname, 
					filename, filenamehex, command ) < 0 )
	{
		term_beep();
		return;
	}
	screen = fopen( filename, "w" );
	if ( screen == NULL )
	{
		term_beep();
		return;
	}
	screenhex = fopen( filenamehex, "w" );
	if ( screenhex == NULL )
	{
		term_beep();
		fclose( screen );
		return;
	}
	win = Curwin;
	for ( row = 0; row < win->display_rows; row++ )
	{
		blankcount = 0;
		p = &win->ftchars[ row ][ 0 ];
		for ( col = 0; col < win->cols; col++ )
		{
			fprintf( screenhex, "%08lx\n", *p );
					/* if magic cookie, the character is
					   an irrelevant index into the magic
					   array - likely to be an unprintable
					   control char.  Force to blank.
					*/
			c_w_attr = *p;
			c = (char) *p++;
			if ( c_w_attr & ATTR_MAGIC )
			{
				c = ' ';
			}
			if ( c == ' ' )
				blankcount++;
			else
			{
				for ( i = 0; i < blankcount; i++ )
					putc( ' ', screen );
				blankcount = 0;
				putc( c, screen );
			}
		}
		fprintf( screenhex, "\n" );
		putc( '\n', screen );
	}
	fclose( screen );
	fclose( screenhex );
	/******************************************************************
	* In case user has a umask wherein facetterm (root) creates
	*  files that the user (.facetprint) cannot read - really (066)!
	******************************************************************/
	chown( filename, getuid(), getgid() );
	chmod( filename, 0666 );
	chmod( filenamehex, 0666 );
	mysystem( command );
}
/**************************************************************************
* generate_script_command
*	Generate the command to execute the .facetprint file.
*	The script name '.facetprint' is passed in in "script".
*	Look in
*		the current directory.
*		the $HOME directory.
*		the /usr/facetterm directory. $FACETTERMPATH $TSMPATH
*		the /usr/facet directory.
*	Return the name of the temporary file in "filename".
*	Return the name of another temporary file in "filenamehex" if it is
*		not a null pointer.
*	Return the command in "command".
*	Return -1 if the script cannot be found. Return 0 for ok.
**************************************************************************/
generate_script_command( script, filename, filenamehex, command )
	char	*script;
	char	*filename;
	char	*filenamehex;
	char	*command;
{
	char	scriptpath[ 256 ];
	char	*home;
	char	*getenv();
	int	status;

	if ( tmpnam( filename ) == NULL )
		return( -1 );
	if ( filenamehex != (char *) 0 )
	{
		if ( tmpnam( filenamehex ) == NULL )
			return( -1 );
	}
	/******************************************************************
	* Look in current directory - do not assume that it is in $PATH.
	* Believe it or not lots of users don't.
	******************************************************************/
	strcpy( scriptpath, "./" );
	strcat( scriptpath, script );
	status = access( scriptpath, 1 );
	if ( status < 0 )
	{
		/**********************************************************
		* Look in $HOME
		**********************************************************/
		home = getenv( "HOME" );
		if ( home != NULL )
		{
			strcpy( scriptpath, home );
			strcat( scriptpath, "/" );
			strcat( scriptpath, script );
			status = access( scriptpath, 1 );
		}
	}
	if ( status < 0 )
	{
		/**********************************************************
		* Look in /usr/facetterm
		**********************************************************/
		strcpy( scriptpath, Facettermpath );
		strcat( scriptpath, "/" );
		strcat( scriptpath, script );
		status = access( scriptpath, 1 );
	}
	if ( status < 0 )
	{
		/**********************************************************
		* Look in /usr/facet
		**********************************************************/
		strcpy( scriptpath, Facetpath );
		strcat( scriptpath, "/" );
		strcat( scriptpath, script );
		status = access( scriptpath, 1 );
	}
	if ( status < 0 )
		return( -1 );
	/******************************************************************
	* nohup sh -c ".facetprint tempfile; rm tempfile" > /dev/null 2>&1 &
	******************************************************************/
	strcpy( command, "nohup sh -c \"" );
	strcat( command, scriptpath );
	strcat( command, " " );
	strcat( command, filename );
	if ( filenamehex != (char *) 0 )
	{
		strcat( command, " " );
		strcat( command, filenamehex );
	}
	strcat( command, "; rm " );
	strcat( command, filename );
	if ( filenamehex != (char *) 0 )
	{
		strcat( command, " " );
		strcat( command, filenamehex );
	}
	strcat( command, "\" > /dev/null 2>&1 &" );
	return( 0 );
}
/**************************************************************************
* fct_window_mode_ans_curwin
*	Tell the receiver the current window ( and possibly that we are in
*	scan code mode ) and to resume operation.
*	'0' to '9' --> window 1 to 10
*	'A' to 'J' --> window 1 to 10 in scan code mode.
**************************************************************************/
fct_window_mode_ans_curwin()
{
#ifdef SOFTPC
	if ( Curwin->terminal_mode_is_scan_code )
		fct_window_mode_ans( (char) ('A' + Curwinno) );
	else
#endif
		fct_window_mode_ans( (char) ('0' + Curwinno) );
}
/**************************************************************************
* fct_window_mode_ans_winno
*	Tell the receiver that the current window number is "winno" and
*	to resume operation.
**************************************************************************/
fct_window_mode_ans_winno( winno )
	int	winno;
{

#ifdef SOFTPC
	if ( Wininfo[ winno ]->terminal_mode_is_scan_code )
		fct_window_mode_ans( (char) ('A' + winno) );
	else
#endif
		fct_window_mode_ans( (char) ('0' + winno) );
}
/**************************************************************************
* ftproc_select
*	Select window number "winno" with possible modifying "command".
*	b = bottom of split screen
*	t = top of split screen
*	f = full screen
*	'\0' = no change
**************************************************************************/
ftproc_select( winno, command )		/* assumes modes off */
	int	winno;
	int	command;
{
	FT_WIN	*old_window;

	if ( winno == Curwinno )
		redo_curwinno( command );
	else
	{
		Last_curwinno = Curwinno;
		old_window = Curwin;
		Curwinno = winno;
		Outwinno = winno;
		Curwin = Wininfo[ winno ];
		Outwin = Wininfo[ winno ];
		O_pe = Outwin->personality;
		if ( Curwin->onscreen )		/* split already on */
			select_onscreen( command );
		else
			select_offscreen( command, old_window );
		t_sync_modes_current();
	}
}
/**************************************************************************
* ftproc_overwrite
*	Pop up the window number "winno" over the current window.
**************************************************************************/
ftproc_overwrite( winno )		/* assumes modes off */
	int	winno;
{
	int	i;
	FT_WIN	*popwin;
	FT_WIN	*overwin;
	FT_WIN	*splitwin;

	if ( winno == Curwinno )
		expand_to_full();
	popwin = Wininfo[ winno ];
	Overwritten_winno = Curwin->number;
	overwin = Curwin;
	Curwin->onscreen = 0;
	/******************************************************************
	* Clear the arrays that track what on the screen has been changed.
	* Most operations set the appropriate bits in these arrays all the
	* time.
	******************************************************************/
	clear_row_changed( Curwin );
	clear_col_changed( Curwin );
	/******************************************************************
	* If popping up on a split screen, remember the non-current other
	* split for later restoration.
	******************************************************************/
	if ( Curwin->fullscreen == 0 )
	{					/* find other half if any */
		int	other_split;

		other_split = find_other_split_winno( Curwinno );
		if ( other_split >= 0 )
		{
			Overwritten_split_winno = other_split;
			splitwin = Wininfo[ other_split ];
			clear_row_changed( splitwin );
			clear_col_changed( splitwin );
		}
	}
	/******************************************************************
	* Make the popup the current window.
	******************************************************************/
	Curwinno = winno;
	Outwinno = winno;
	Curwin = popwin;
	Outwin = popwin;
	O_pe = Outwin->personality;
	/******************************************************************
	* Popup must match the current screen whether it likes it or not.
	* Full screen and matching number of columns - forced back to 80 
	* later.
	******************************************************************/
	if ( Curwin->fullscreen == 0 )
		w_set_full( Curwin );	
	if ( term_is_columns_wide_on() )
		win_se_columns_wide_on();
	else if ( Curwin->columns_wide_mode_on )
		win_se_columns_wide_off();
	win_se_rows_changeno_match_screen();
	/******************************************************************
	******************************************************************/
	Popup_screen_level = 0;
	if ( t_sync_terminal_all_modes() == 0 )
	{
		/**********************************************************
		* Popping up did not clear screen.
		* Remember the current screen for later restoration.
		**********************************************************/
		if ( overwin->fullscreen == 0 )
		{
			int	rows;

			rows = overwin->win_bot_row - overwin->win_top_row + 1;
			for ( i = 0; i < rows; i++ )
			{
			    memcpy( 
				(char *)
				popwin->ftchars[ overwin->win_top_row + i ], 
				(char *)
				overwin->ftchars[ overwin->buff_top_row + i ], 
				(int) ( Cols_wide * sizeof( FTCHAR ) ) );
			}
			if ( Overwritten_split_winno >= 0 )
			{
				rows = splitwin->win_bot_row 
				     - splitwin->win_top_row + 1;
				for ( i = 0; i < rows; i++ )
				{
				memcpy( 
				(char *)
				popwin->ftchars[ splitwin->win_top_row + i ],
				(char *)
				splitwin->ftchars[ splitwin->buff_top_row + i ],
				(int) ( Cols_wide * sizeof( FTCHAR ) ) );
				}
			}
		}
		else
		{
			for ( i = 0; i < overwin->display_row_bottom; i++ )
				memcpy( (char *) popwin->ftchars[ i ], 
					(char *) overwin->ftchars[ i ], 
					(int) ( Cols_wide * sizeof( FTCHAR ) ));
		}
		push_overwritten_screen();
	}
	/******************************************************************
	* Only the pop up appears to be on the screen.
	******************************************************************/
	w_all_off();
	Curwin->onscreen = 1;
	forget_page( Curwin );			/* going to be modified here */
	clear_row_changed( Curwin );
	clear_col_changed( Curwin );
	/******************************************************************
	* Popping up on a screen with line attributes will scramble the
	* pop up - clear it for now.
	******************************************************************/
	clear_line_attribute( 0, Curwin->display_row_bottom );
	f_cursor();
	t_sync_modes_current();
}
/**************************************************************************
* find_other_split_winno
*	Return the window number of any window that is onscreen besides
*	window number "winno".
**************************************************************************/
find_other_split_winno( winno )
	int	winno;
{
	int	i;

	for ( i = 0; i < Wins; i++ )
	{
		if ( i != winno )
		{
			if ( Wininfo[ i ]->onscreen )
				return( i );
		}
	}
	return( -1 );
}
/**************************************************************************
* ftproc_overwrite_restore
*	Pop down the popup and restore the screen.
**************************************************************************/
ftproc_overwrite_restore()		/* assumes modes off */
{
	FT_WIN	*popwin;
	int	row;
	int	first_col;
	int	last_col;

	popwin = Curwin;
	/******************************************************************
	** Clear the popwin buffer in case it poped up over itself
	** in which case the refresh should show blanks
	******************************************************************/
	clear_popwin_buffer( popwin );
	/******************************************************************
	** Paste a ^B to the popup window to tell it popup canceled.
	******************************************************************/
	fct_window_mode_ans( (char) 'p' );
	fct_window_mode_ans_curwin();
	fct_window_mode_ans( Windows_window_char_stop );
	fct_window_mode_ans( 0 );
					/* force pop-up to 80 col */
	/******************************************************************
	** Force the popup back to a "normal window".
	******************************************************************/
	if ( term_is_columns_wide_on() )
		win_se_columns_wide_off();
	win_se_rows_changeno_normal();
	/******************************************************************
	* Put the original window back.
	******************************************************************/
	Curwinno = Overwritten_winno;
	Outwinno = Overwritten_winno;
	Curwin = Wininfo[ Overwritten_winno ];
	Outwin = Wininfo[ Overwritten_winno ];
	O_pe = Outwin->personality;
	w_all_off();
	Outwin->onscreen = 1;
	/******************************************************************
	* If switching back clears the screen - start from scratch.
	******************************************************************/
	if ( t_sync_terminal_all_modes() )
	{
	    if ( Outwin->fullscreen )
	    {
		tell_full_refresh();
		full_refresh();
		f_cursor();
	    }
	    else
	    {
		rebuild_split_screen( Overwritten_split_winno );
	    }
	}
	else
	{
	    /**********************************************************
	    * Restore the screen.
	    **********************************************************/
	    if ( Outwin->fullscreen )
	    {
		if ( Outwin->row_changed_all || popwin->row_changed_all )
		{
			/**************************************************
			* Popup had a field day - scrolled or cleared etc.
			**************************************************/
			full_refresh();
		}
		else
		{
			/**************************************************
			** Refresh the rectangle for each level pushed.
			**************************************************/
			find_col_changed( popwin, &first_col, &last_col );
			while ( Popup_screen_level > 1 )
			{
				for ( row = Outwin->display_rows - 1; row >= 0;
					row-- )
				{
					if ( popwin->row_changed[ row ] )
						f_refresh_overwritten_cols( 
							row,
							first_col, last_col );
				}
				Popup_screen_level--;
				pop_popup_screen_restore( popwin, 
							  Popup_screen_level );
				find_col_changed( popwin, 
						  &first_col, &last_col );
			}
			/**************************************************
			** Refresh anything that changed in the overwritten
			** window or before the first push.
			**************************************************/
			for ( row = Outwin->display_rows - 1; row >= 0; row-- )
			{
				if ( Outwin->line_attribute[ row ] )
					f_refresh( row, row );
				else if ( Outwin->row_changed[ row ] )
					f_refresh( row, row );
				else if ( popwin->row_changed[ row ] )
					f_refresh_overwritten_cols( 
						row, first_col, last_col );
			}
		}
		f_cursor();
	    }
	    else
	    {
		rebuild_split_screen( Overwritten_split_winno );
	    }
	}
	/******************************************************************
	* Finish up
	******************************************************************/
	t_sync_modes_current();
	modes_off_window_mode();
	term_outgo();
	Overwritten_winno = -1;
	Overwritten_split_winno = -1;
}
/**************************************************************************
* rebuild_split_screen
*	Popping down a pop up program that came up over a split screen.
**************************************************************************/
rebuild_split_screen( other_winno )
	int	other_winno;
{
	int	save_curwinno;

	if ( other_winno >= 0 )
	{
		save_curwinno = Curwinno;
		split_label_split_row();
		Curwinno = other_winno;
		Outwinno = other_winno;
		Curwin = Wininfo[ other_winno ];
		Outwin = Wininfo[ other_winno ];
		O_pe = Outwin->personality;
		Outwin->onscreen = 1;
		split_number( Outwin->win_bot_row + 1, 
				      Outwin->number );
		split_refresh();
		s_cursor_w_pan();
		Curwinno = save_curwinno;
		Outwinno = save_curwinno;
		Curwin = Wininfo[ save_curwinno ];
		Outwin = Wininfo[ save_curwinno ];
		O_pe = Outwin->personality;
	}
	else
	{
		clear_other_split_win();
		split_label_split_row();
	}
	split_number( Outwin->win_bot_row + 1, Outwin->number );
	split_refresh();
	s_cursor_w_pan();
}
/**************************************************************************
* push_overwritten_screen
*	Push a level of the memory that holds screens to be restored.
**************************************************************************/
push_overwritten_screen()
{
	int	first_col;
	int	last_col;
	int	first_row;
	int	last_row;
	int	level;

	first_col = 0;
	last_col = Curwin->col_right;
	first_row = 0;
	last_row = Curwin->display_row_bottom;
	level = push_popup_level( first_col, last_col, first_row, last_row );
	if ( level >= 0 )
	{
		return( level );
	}
	else
	{
		term_beep();
		return( -1 );
	}
}
/**************************************************************************
* push_popup_screen
*	Determine what the popup screen has changed on the current push
*	level, remember it, and push a new level.
*	Reset the column changed and row changed arrays since nothing
*	changed on the new level yet.
**************************************************************************/
push_popup_screen()
{
	int	first_col;
	int	last_col;
	int	first_row;
	int	last_row;
	int	level;

	find_col_changed( Curwin, &first_col, &last_col );
	find_row_changed( Curwin, &first_row, &last_row );
	level = push_popup_level( first_col, last_col, first_row, last_row );
	if ( level >= 0 )
	{
		clear_col_changed( Curwin );
		clear_row_changed( Curwin );
		return( level );
	}
	else
	{
		term_beep();
		return( -1 );
	}
}
/**************************************************************************
* push_popup_level
*	Remember the rectangle specified in the arguments as as the screen
*	contents to be remembered for this level and then remember the
*	appropriate rows.
*	Start a new level.
**************************************************************************/
push_popup_level( first_col, last_col, first_row, last_row )
	int first_col;
	int last_col;
	int first_row;
	int last_row;
{

	if ( Popup_screen_level < MAX_POPUP_SCREEN_LEVEL )
	{
		Popup_screen_first_col[ Popup_screen_level ] = first_col;
		Popup_screen_last_col[  Popup_screen_level ] = last_col;
		Popup_screen_first_row[ Popup_screen_level ] = first_row;
		Popup_screen_last_row[  Popup_screen_level ] = last_row;
		popup_rows_save( Popup_screen_level, Curwin, 
							first_row, last_row );
		Popup_screen_level++;
		return( Popup_screen_level );
	}
	else
	{
		return( -1 );
	}
}
/**************************************************************************
* pop_popup_screen
*	Pop a level.
*	The idea here is that the popwin's buffer is completely restored
*	to its state of the last push by loading and reloading all of the
*	saved rows from the original popped over screen and all pushed
*	popups.
*	Now everything is back except the screen.
*	Update the rectangle that the level being popped damaged.
**************************************************************************/
pop_popup_screen()
{
	FT_WIN	*popwin;

	if ( Overwritten_winno < 0 )
		return( -1 );
	modes_off_window_mode();
	popwin = Curwin;
	/******************************************************************
	** Erase the characters in the buffer that were modified since push.
	** If Outwin is the popup window we want to refresh blanks here.
	******************************************************************/
	clear_popup_block_buffer( popwin );
	/******************************************************************
	** Restore popwin's buffer to state when Popup_screen_level was pushed.
	******************************************************************/
	popup_rows_restore( Popup_screen_level, popwin );
	/******************************************************************
	* Refresh the characters that were changed since the last push.
	* "Popup_screen_level" is being popped.
	******************************************************************/
	f_refresh_popup_block( popwin );
	modes_on_window_mode();
	f_cursor();
	/******************************************************************
	* Put the changed arrays back to the way they were at the last push.
	******************************************************************/
	if ( Popup_screen_level > 0 )
	{
		Popup_screen_level--;
		pop_popup_screen_restore( popwin, Popup_screen_level );
	}
	else
	{
		clear_col_changed( Curwin );
		clear_row_changed( Curwin );
	}
	return( Popup_screen_level );
}
/**************************************************************************
* clear_popup_block_buffer
*	If the pop up popped up when it itself was the current window then
*	it essentially pops up over a blank screen.
*	This finds the rectangle that was damaged since the last push
*	and clears it from the buffer.
*	That way if the buffer restore does not restore an area that
*	was changed, the screen goes back to blanks.
**************************************************************************/
clear_popup_block_buffer( popwin )
	FT_WIN	*popwin;
{
	int	changed;
	int	row;
	int	first_col;
	int	last_col;

	changed = find_col_changed( popwin, &first_col, &last_col );
	if ( changed == 0 )
		return;
	for ( row = 0; row < Outwin->display_rows; row++ )
	{
		if ( popwin->row_changed[ row ] )
			d_blank( &popwin->ftchars[ row ][ first_col ], 
				 last_col - first_col + 1 );
	}
}
/**************************************************************************
* clear_popwin_buffer
*	Clear the pop up's window buffer.
**************************************************************************/
clear_popwin_buffer( popwin )
	FT_WIN	*popwin;
{
	REG     int     i;
	REG     int     rows;

	rows = popwin->display_rows;
	for ( i = 0; i < rows; i++ )
		d_blankline( popwin->ftchars[ i ] );
}
/**************************************************************************
* pop_popup_screen_restore
*	Put the changed rows and changed columns arrays to the state
*	that they were in when the last push was done.
**************************************************************************/
pop_popup_screen_restore( win, level )
	FT_WIN	*win;
	int	level;
{
	clear_row_changed( win );
	clear_col_changed( win );
	set_col_changed_1( win, Popup_screen_first_col[ level ] );
	set_col_changed_1( win, Popup_screen_last_col[  level ] );
	set_row_changed(   win, Popup_screen_first_row[ level ],
				Popup_screen_last_row[  level ] );
}
/**************************************************************************
* switch_if_current
*	If window number "winno" is the current window, arrange for a 
*	switch to the window number "to_winno".
*	This is done by storing the proper info in globals,
*	sending a signal to the receiver,
*	and waiting for the response in fsend_get_acks_only.
*	When the response comes the switch will be done by a subroutine
*	of fsend_get_acks_only and the globals will be cleared.
*	Since the response may not be the first communication from
*	the receiver ( may have had a simultaneous hotkey, etc )
*	keep at it until it works.
**************************************************************************/
switch_if_current( winno, to_winno )
	int	winno;
	int	to_winno;
{
	if ( winno == Curwinno )
	{
		Switch_window = to_winno;
		send_packet_signal();
		while(  ( Switch_window >= 0 ) 
		     || ( Switch_window == IDLE_WINDOW_IS_NEXT_ACTIVE ) )
		{
			fsend_get_acks_only();
		}
	}
}
/**************************************************************************
* outwin_is_curwin
*	Return 1 if the window "Outwinno" is the current window.
*	This is for routines in modules without access to the current
*	window number.
*	Return 0 otherwise.
**************************************************************************/
outwin_is_curwin()
{
	return( Outwinno == Curwinno );
}
/**************************************************************************
* winno_is_curwinno
*	Return 1 if the window "winno" is the current window.
*	This is for routines in modules without access to the current
*	window number.
*	Return 0 otherwise.
**************************************************************************/
winno_is_curwinno( winno )
	int	winno;
{
	return( winno == Curwinno );
}
/**************************************************************************
* fct_modes_normal
*	Put window and ( and terminal modes if current ) to the default.
*	Called when window goes idle - assumes Outwinno is Curwinno.
*	If window is not current, its modes are handled when it goes
*	current or it is written to.
**************************************************************************/
fct_modes_normal( window )
	int	window;
{
	modes_init_idle( window );
	if ( window == Curwinno )
	{
		t_sync_terminal_all_modes_normal();
		/* leave cols_wide if set so they can see it */
		t_sync_modes_idle();
	}
}
/**************************************************************************
* fct_set_outwinno
*	Set the current output window ( as opposed to the current (input)
*	window ) to the window number "window".
*	If the window is not the current window but is onscreen ( other split)
*	then set up the terminal modes for outputting to it.
*	Remember that you did that in Modes_are_outwins because it has to
*	be put back before returning to the current window.
*	If it is not on screen, mark it as modified offscreen for possible
*	terminal page forced refresh.
**************************************************************************/
int	Modes_are_outwins = 0;
long	Transparent_start_time = 0;
long	time();
fct_set_outwinno( window )
	int	window;
{
	FT_WIN	*newwin;
	if ( window != Curwinno )
	{
		/**********************************************************
		* Not current window
		**********************************************************/
		newwin = Wininfo[ window ];
		if ( newwin->onscreen )
		{
			/**************************************************
			* Other side of split screen.
			**************************************************/
			Modes_are_outwins = 1;
			modes_off_outwin();
		}
		else
		{
			/**************************************************
			* Off screen.
			**************************************************/
			Modes_are_outwins = 0;
			newwin->in_terminal = 0;
		}
		/**********************************************************
		*  Setup to process the window.
		**********************************************************/
		Outwinno = window;
		Outwin = newwin;
		O_pe = Outwin->personality;
		if ( Modes_are_outwins )
		{
			t_cursor();
			modes_on_outwin();
		}
	}
	/******************************************************************
	* Window is in transparent print - let it go through.
	******************************************************************/
	if ( Outwin->transparent_print_on )
	{
		Transparent_start_time = time( (long *) 0 );
		term_transparent_print_on();
	}
	else
		Transparent_start_time = 0;
}
/**************************************************************************
* fct_done_outwinno
*	Undo fct_set_outwinnno above now that the output is done.
**************************************************************************/
fct_done_outwinno()
{
	int	x;

	if ( Outwin->transparent_print_on )
	{
		int	seconds_delayed;

		seconds_delayed = 0;
		/**********************************************************
		* Turn off transparent print - drain to try to prevent
		* clogged tty driver buffers from delaying echo.
		**********************************************************/
		term_transparent_print_off();
		term_outgo();
		term_drain();
		if ( Transparent_start_time > 0 )
		{
			seconds_delayed = Transparent_start_time
				        - time( (long *) 0 );
		}
		set_print_active( Outwinno, seconds_delayed );
	}
	if ( Outwinno != Curwinno )
	{
		/**********************************************************
		* Undo the terminal modes that were set up for the window.
		**********************************************************/
		if ( Modes_are_outwins )
		{
			modes_off_outwin();
		}
		Outwinno = Curwinno;
		/**********************************************************
		* Compilers get excessivly optimistic if you don't use the
		* temporary.
		**********************************************************/
		x = Curwinno;
		Outwin = Wininfo[ x ];
		O_pe = Outwin->personality;
		if ( Modes_are_outwins )
		{
			t_cursor();
			modes_on_outwin();
			Modes_are_outwins = 0;
		}
	}
}
/**************************************************************************
* puts_winno
*	Output a string "string" containing no special characters except 
*	possibly newline to the window number winno.
*	This is used by the hp print window module on break.
**************************************************************************/
puts_winno( string, winno )
	char	*string;
	int	winno;
{
	int	save_winno;
	FT_WIN	*save_win;

	save_winno = Outwinno;
	save_win = Outwin;
	Outwinno = winno;
	Outwin = Wininfo[ winno ];
	O_pe = Outwin->personality;
	ftproc_puts( string );
	term_outgo();
	Outwinno = save_winno;
	Outwin = save_win;
	O_pe = Outwin->personality;
}
/**************************************************************************
* redo_curwinno
*	Move a top split to a bottom split or a full screen,
*	a bottom split to a top split or a full screen,
*	or a full screen to a top split or a bottom split.
*	Or decide it is correct and position the cursor.
*	Refresh the window if no command.
**************************************************************************/
redo_curwinno( command )
	int	command;
{
	char	buffer[ 80 ];

	switch( command )
	{
	case 'b':
		if ( Curwin->fullscreen )
		{
			shrink_to_bottom( Curwin );
			t_cursor();
		}
		else if ( window_is_at_top( Curwin ) )
			move_split_bottom();
		else
			s_cursor();
		break;
	case 't':
		if ( Curwin->fullscreen )
		{
			shrink_to_top( Curwin );
			t_cursor();
		}
		else if ( window_is_at_bottom( Curwin ) )
			move_split_top();
		else
			s_cursor();
		break;
	case 'f':
		if ( Curwin->fullscreen == 0 )
			expand_to_full();
		else
			f_cursor();
		break;
	case 'W':
	case 0:
		if ( Curwin->fullscreen )			
		{
			tell_full_refresh();
			scroll_memory_refresh();
			full_refresh();
			f_cursor();
		}
		else
		{
			split_refresh();
			s_cursor();
		}
		break;
	default:
		sprintf( buffer, "(<rc=0x_%x>)", command );
		error_puts( buffer );
		t_cursor();
		break;
	}
}
/**************************************************************************
* select_onscreen
*	Make an on screen window the current window reworking the screen
*	if necessary.
**************************************************************************/
select_onscreen( command )
	int	command;
{
	char	buffer[ 80 ];

					/* since already onscreen:
					   - modes cannot be incompatible 
					   - width must match
					*/
	t_sync_terminal_all_modes();
	t_sync_modes_screen_attribute();
	onstatus_refresh();
	switch( command )
	{
	case 'b':
		if ( window_is_at_top( Curwin ) )
			move_split_bottom();
		else
			s_cursor();
		break;
	case 't':
		if ( window_is_at_bottom( Curwin ) )
			move_split_top();
		else
			s_cursor();
		break;
	case 'f':
		expand_to_full();
		break;
	case 0:
		s_cursor();
		break;
	default:
		sprintf( buffer, "(<rons=0x_%x>)", command );
		error_puts( buffer );
		t_cursor();
		break;
	}
}
/**************************************************************************
* select_offscreen
*	Make an offscreen window the new current window, reworking the
*	screen if necessary.
**************************************************************************/
select_offscreen( command, old_window )
	int	command;
	FT_WIN	*old_window;
{
	char	buffer[ 80 ];

	if (  (  command == 'f' )
	   || ( ( command == 0 ) && ( Curwin->fullscreen ) ) )
	{
		if ( Curwin->fullscreen == 0 )
			w_set_full( Curwin );
		if ( old_window->fullscreen )
			full_to_full( old_window );
		else
			split_to_full();
	}
	else if (  (  command == 't' )
		|| ( ( command == 0 ) && ( window_is_at_top( Curwin ) ) ) )
	{
		set_to_top( Curwin );	/* in case top changed */
		if ( old_window->fullscreen )
			full_to_split( old_window );
		else
			split_to_split();
	}
	else if (  ( command == 'b' )
		|| (  command == 0 ) )
	{
		set_to_bottom( Curwin ); /* in case bottom changed */
		if ( old_window->fullscreen )
			full_to_split( old_window );
		else
			split_to_split();
	}
	else
	{
		sprintf( buffer, "(<rofff=0x_%x>)", command );
		error_puts( buffer );
	}
}
/**************************************************************************
* full_to_full
*	Switch from a full screen window to a new full screen window
*	specified by Curwin.
**************************************************************************/
full_to_full( old_window )
	FT_WIN	*old_window;
{			/* not in terminal */
	remember_page( old_window );
	old_window->onscreen = 0;
	Curwin->onscreen = 1;
	if ( find_page( Curwin ) )
	{
		/**********************************************************
		* Cannot find required page in terminal - get a new one.
		**********************************************************/
		get_page( Curwin );
					/* don't care if clears */
		t_sync_terminal_all_modes();
					/* don't care if clears */
		t_sync_columns( Curwin->columns_wide_mode_on );
					/* don't care if clears */
		t_sync_rows_change( Curwin->rows_changeno,
				    Curwin->extra_data_row );
		t_sync_modes_screen_attribute();
		tell_full_switch();
		scroll_memory_refresh();
		full_refresh();
	}
	else if ( Curwin->in_terminal == 0  )
	{
		/**********************************************************
		* Found the page but it was changed offscreen.
		**********************************************************/
					/* don't care if clears */
		t_sync_terminal_all_modes();
					/* don't care if clears */
		t_sync_columns( Curwin->columns_wide_mode_on );
					/* don't care if clears */
		t_sync_rows_change( Curwin->rows_changeno,
				    Curwin->extra_data_row );
		t_sync_modes_screen_attribute();
		tell_full_switch();
		scroll_memory_refresh();
		full_refresh();
	}
	else
	{
		/**********************************************************
		* Found the window in a terminal page.
		**********************************************************/
		if (  t_sync_terminal_all_modes()
		   || (  t_sync_columns( Curwin->columns_wide_mode_on )
		      && F_columns_wide_clears_screen )
		   || (  t_sync_rows_change( Curwin->rows_changeno,
					     Curwin->extra_data_row )
		      && F_rows_change_does_clear_screen ) )
		{
			/**************************************************
			* Getting to it messed it up.
			**************************************************/
			t_sync_modes_screen_attribute();
			tell_full_switch();
			scroll_memory_refresh();
			full_refresh();
		}
		else
		{
			t_sync_modes_screen_attribute();
			onstatus_refresh();		/* status line */
		}
	}
	f_cursor();
}
/**************************************************************************
* split_to_full
*	selected a fullscreen window when terminal is split screen.
**************************************************************************/
split_to_full()
{			/* not in terminal */
	w_all_off();
	Curwin->onscreen = 1;
	if ( find_page( Curwin ) )
	{				/* can't find it - use this one */
					/* don't care if clears */
		t_sync_terminal_all_modes();
					/* don't care if clears */
		t_sync_columns( Curwin->columns_wide_mode_on );
					/* don't care if clears */
		t_sync_rows_change( Curwin->rows_changeno,
				    Curwin->extra_data_row );
		t_sync_modes_screen_attribute();
		tell_full_switch();
		scroll_memory_refresh();
		full_refresh();
	}
	else if ( Curwin->in_terminal == 0  )
	{				/* selected - was changed offscreen */
					/* don't care if clears */
		t_sync_terminal_all_modes();
					/* don't care if clears */
		t_sync_columns( Curwin->columns_wide_mode_on );
					/* don't care if clears */
		t_sync_rows_change( Curwin->rows_changeno,
				    Curwin->extra_data_row );
		t_sync_modes_screen_attribute();
		tell_full_switch();
		scroll_memory_refresh();
		full_refresh();
	}
	else
	{				/* found it and it is ok */
		if (  t_sync_terminal_all_modes()
		   || (  t_sync_columns( Curwin->columns_wide_mode_on )
		      && F_columns_wide_clears_screen )
		   || (  t_sync_rows_change( Curwin->rows_changeno,
					     Curwin->extra_data_row )
		      && F_rows_change_does_clear_screen ) )
		{
			t_sync_modes_screen_attribute();
			tell_full_switch();
			scroll_memory_refresh();
			full_refresh();
		}
		else
		{
			t_sync_modes_screen_attribute();
			onstatus_refresh();
		}
	}
	f_cursor();
}
/**************************************************************************
* full_to_split
*	Selected a split window when old window was a full screen.
**************************************************************************/
full_to_split( old_window )
	FT_WIN	*old_window;
{
	remember_page( old_window );
	old_window->onscreen = 0;
	Curwin->onscreen = 1;
	forget_page( Curwin );
	get_page( Curwin );
					/* don't care if clears */
	t_sync_terminal_all_modes();
					/* don't care if clears */
	t_sync_columns( Curwin->columns_wide_mode_on );		
					/* don't care if clears */
	t_sync_rows_change( Curwin->rows_changeno,
			    Curwin->extra_data_row );
	t_sync_modes_screen_attribute();
	out_term_clear_screen();
	split_label( D_split_row );
	new_split_on_split();
}
/**************************************************************************
* split_to_split
*	Selected an offscreen split window when terminal is currently split.
**************************************************************************/
split_to_split()
{
	int	other_split_cannot_stay;

	other_split_cannot_stay = 0;
	if ( t_sync_terminal_all_modes() )
	{				/* cleared screen */
		other_split_cannot_stay = 1;
	}
	if ( t_sync_columns( Curwin->columns_wide_mode_on ) != 0 )
	{
		other_split_cannot_stay = 1;
	}
	if ( t_sync_rows_change( Curwin->rows_changeno,
				 Curwin->extra_data_row ) )
	{
		other_split_cannot_stay = 1;
	}
	t_sync_modes_screen_attribute();
	if ( other_split_cannot_stay )
	{
		w_all_off();
		out_term_clear_screen();
		split_label( D_split_row );
	}
	else
	{
		w_overlaps_off( Curwin );
	}
	Curwin->onscreen = 1;
	forget_page( Curwin );
	new_split_on_split();
}
/**************************************************************************
* t_sync_terminal_all_modes
*	Synchronize the terminal (if necessary) to those of the window
*	"Curwin".  This includes:
*		scan code mode on/off
*		7 bit - 8 bit 
**************************************************************************/
t_sync_terminal_all_modes()
{
	int	cleared_screen;
	int	clears;

	cleared_screen = 0;
	if ( t_sync_terminal_mode( Curwin, &clears ) )
	{
		t_sync_auto_wrap( 1 );
		if ( clears )
			cleared_screen = 1;
	}
	return( cleared_screen );
}
/**************************************************************************
* t_sync_terminal_all_modes_normal
*	Syncronize terminal modes ( if necessary ) to the default modes
*	for pc mode ( scan code ) and 7bit 8bit.
**************************************************************************/
t_sync_terminal_all_modes_normal()
{
		t_sync_terminal_mode_normal();
}
/**************************************************************************
* split_label_split_row
*	Draw split screen dividers.
**************************************************************************/
split_label_split_row()
{
	split_label( D_split_row );
}
/**************************************************************************
* new_split_on_split
*	Refresh an offscreen split screen window on a screen already
*	arranged for split screen.
**************************************************************************/
new_split_on_split()	/* offscreen split on split screen */
{
	check_split_windows();
	split_number( Outwin->win_bot_row + 1, Outwin->number );
	split_refresh();
	s_cursor_w_pan();
}
/**************************************************************************
* expand_to_full
*	Onscreen split window to full screen window.
**************************************************************************/
expand_to_full()
{
	w_all_off();
	Curwin->onscreen = 1;
	w_set_full( Curwin );
	s_sync_to_buffer();
	tell_full_refresh();
	f_refresh_around_split();
	f_cursor();
}
/**************************************************************************
* shrink_to_top
*	Onscreen full screen window to the top of a split screen.
**************************************************************************/
shrink_to_top( window )
	FT_WIN	*window;
{
	set_to_top( window );
	s_sync_to_window( window );

	clear_bottom_win();
	split_label( D_split_row );
	split_number( D_split_row, window->number );
}
/**************************************************************************
* shrink_to_bottom
*	Onscreen full screen window to the bottom of a split screen.
**************************************************************************/
shrink_to_bottom( window )
	FT_WIN	*window;
{
	set_to_bottom( window );
	s_sync_to_window( window );

	clear_top_win();
	split_label( D_split_row );
	split_number( Row_bottom_terminal, window->number );
}
/**************************************************************************
* move_split_top
*	Onscreen split bottom to split top.
**************************************************************************/
move_split_top()
{
	move_split( Curwin, 0, D_split_row - 1 );
	t_cursor();
}
/**************************************************************************
* move_split_bottom
*	Onscreen split top to split bottom.
**************************************************************************/
move_split_bottom()
{
	move_split( Curwin, D_split_row + 1, Row_bottom_terminal - 1 );
	t_cursor();
}
/**************************************************************************
* move_split
*	Onscreen split to other split.
**************************************************************************/
move_split( window, to_top_row, to_bot_row )
	FT_WIN	*window;
	int	to_top_row;
	int	to_bot_row;
{
	int	diff;
	int	above;
	int	below;
	int	above_cursor;
	int	to_rows;
	int	from_top_row;
	int	from_bot_row;
	int	from_rows;

	w_all_off();
	window->onscreen = 1;
	to_rows = to_bot_row - to_top_row + 1;
	from_top_row = window->win_top_row;
	from_bot_row = window->win_bot_row;
	from_rows = from_bot_row - from_top_row + 1;
	diff = to_rows - from_rows;
	if ( diff >= 0 )
	{		/* add lines above if any - add lines below if necc */
		if ( diff <= window->buff_top_row )
		{
			above = diff;
			below = 0;
		}
		else
		{
			above = window->buff_top_row;
			below = diff - above;
		}
		s_move_lines( window->win_top_row, to_top_row + above );
		window->win_top_row = to_top_row;
		window->win_bot_row = to_bot_row;
		window->buff_top_row -= above;
		window->buff_bot_row += below;
		if ( above > 0 )
			s_refresh( window, 0, above - 1 );
		if ( below > 0 )
			s_refresh( window, to_rows - below, to_rows - 1 );
	}
	else
	{				/* shrinking */
		above_cursor = window->row - window->buff_top_row + 1;
		if ( above_cursor <= to_rows )
		{			/* discard rows from bottom */
			above = 0;
			below = ( - diff );
		}
		else
		{			/* remove rows too high above cursor */
			above = above_cursor - to_rows;
					/* discard rest of rows from bottom */
			below = ( - diff ) - above;
		}
		s_move_lines( window->win_top_row + above, to_top_row );
		window->win_top_row = to_top_row;
		window->win_bot_row = to_bot_row;
		window->buff_top_row += above;
		window->buff_bot_row -= below;
		clear_rows( from_top_row, from_bot_row );
	}
	split_label( D_split_row );
	split_number( to_bot_row + 1, window->number );
}
/**************************************************************************
* set_to_top
*	Arrange buffer pointers on window "window" for top of split screen.
**************************************************************************/
set_to_top( window )
	FT_WIN	*window;
{
	w_set_top( window );
	b_pan_high_w_cursor( window, D_split_row );
}
/**************************************************************************
* set_to_bottom
*	Arrange buffer pointers on window "window" for bottom of split screen.
**************************************************************************/
set_to_bottom( window )
	FT_WIN	*window;
{
	int	rows_in_window;

	w_set_bottom( window );
	rows_in_window = Row_bottom_terminal - D_split_row - 1;
	b_pan_high_w_cursor( window, rows_in_window );
}
/**************************************************************************
* w_set_full
*	Set window "window" to full screen.
**************************************************************************/
w_set_full( window )
	FT_WIN	*window;
{
	window->fullscreen = 1;
}
/**************************************************************************
* w_set_top
*	Set window "window" to top of split screen.
**************************************************************************/
w_set_top( window )
	FT_WIN	*window;
{
	window->fullscreen = 0;
	window->win_top_row = 0;
	window->win_bot_row = D_split_row - 1;
}
/**************************************************************************
* w_set_bottom
*	Set window "window" to bottom of split screen.
**************************************************************************/
w_set_bottom( window )
	FT_WIN	*window;
{
	window->fullscreen = 0;
	window->win_top_row = D_split_row + 1;
	window->win_bot_row = Row_bottom_terminal - 1;
}
/**************************************************************************
* clear_other_split_win
*	The split window that is onscreen but is not Outwin must be
*	cleared from the screen.
**************************************************************************/
clear_other_split_win()
{
	if ( window_is_at_top( Outwin ) )
		clear_bottom_win();
	else
		clear_top_win();
}
/**************************************************************************
* clear_top_win
*	Clear the top half of a split screen.
**************************************************************************/
clear_top_win()
{
	int i;

	term_pos( 0, 0 );
	for ( i = 0; i < D_split_row; i++ )
	{
		out_term_clr_eol();
		term_line_attribute_off();
		term_linefeed();
	}
}
/**************************************************************************
* clear_bottom_win
*	Clear the bottom half of a split screen.
**************************************************************************/
clear_bottom_win()
{
	term_pos( D_split_row + 1, 0 );
	out_term_clr_eos( D_split_row + 1 );
}
/**************************************************************************
* clear_rows
*	Clear screen from "top_row" to "bot_row".
**************************************************************************/
clear_rows( top_row, bot_row )
	int	top_row;
	int	bot_row;
{
	int 	i;

	if ( bot_row < top_row )
		return;
	if ( bot_row >= Row_bottom_terminal )
	{
		term_pos( top_row, 0 );
		out_term_clr_eos( top_row );
	}
	else
	{
		term_pos( top_row, 0 );
		out_term_clr_eol();
		term_line_attribute_off();
		for ( i = top_row + 1; i <= bot_row; i++ )
		{
			term_linefeed();
			out_term_clr_eol();
			term_line_attribute_off();
		}
	}
}
char	*Text_window_mode_prompt =
	" >>> FacetTerm Window %2d %s  '?' for Help <<< ";
char	*Text_window_mode_prompt_bottom =
	"bottom ";
char	*Text_window_mode_prompt_top =
	"top    ";
char	*Text_window_mode_prompt_full =
	"full   ";
char	*Text_window_mode_prompt_popup =
	"popup  ";
char	*Text_window_mode_prompt_default =
	"";
/**************************************************************************
* show_window_prompt
*	Output window command mode prompt where window number "window"
*	is the current window and "command" is a split screen directive
*	that is pending.
**************************************************************************/
show_window_prompt( window, command )
	int	window;
	int	command;
{
	char	string[ 80 ];
	char	*comstr;
	int	len;
	int	split_start;
	int	cursor_offset;

	len = strlen( Text_window_mode_prompt );
	split_start = 40 -
		( ( len + strlen( Text_window_mode_prompt_bottom ) ) / 2 );
	for ( cursor_offset = 0; cursor_offset < len; cursor_offset++ )
	{
		if ( Text_window_mode_prompt[ cursor_offset ] == '%' )
			break;
	}
	cursor_offset++;
	switch( command )
	{
	case 'b':	comstr = Text_window_mode_prompt_bottom;	break;
	case 't':	comstr = Text_window_mode_prompt_top;		break;
	case 'f':	comstr = Text_window_mode_prompt_full;		break;
	case 'z':	comstr = Text_window_mode_prompt_popup;		break;
	default:	comstr = Text_window_mode_prompt_default;	break;
	}
	/* " >>> FacetTerm Window %2d %s<<< " */
	sprintf( string, Text_window_mode_prompt, window + 1, comstr );
	show_prompt( string, split_start, cursor_offset );
}
char	*Text_cut_and_paste =
	" >>> Window %2d %-44.44s <<< ";
/**************************************************************************
* show_command_prompt
*	Output a cut and paste prompt for window number "window" where
*	"command" is the specific operation being performed.
**************************************************************************/
show_command_prompt( window, command )
	int	window;
	char	*command;
{
	char	string[ 80 ];
	
			/* " >>> Window %2d %-44.44s <<< " */
	sprintf( string, Text_cut_and_paste, window + 1, command );
	show_prompt( string, 7, 13 );
}
char	 *Map_name_input = "";
char	*Text_name_input =
	" >>> %s: %-*.*s <<< ";
/**************************************************************************
* get_name_input
*	Prompt and input a "name" such as a filename or programname.
*	Return the string "program" a maximum of "max" characters.
*	Use "prompt" to prompt the user.
*	Return 0 for successful, -1 for cancelled.
**************************************************************************/
get_name_input( program, max, prompt, secure )
	char	*program;
	int	max;
	char	*prompt;
	int	secure;
{
	int	cursor;
	int	cursor_offset;
	int	c;
	char	string[ 80 ];
	int	redraw;
	int	text_len;
	int	prompt_len;
	char	secure_string[ 80 + 1 ];
	int	i;

	text_len = strlen( Text_name_input );
	prompt_len = strlen( prompt );
	if ( max > 72 - text_len - prompt_len )
		max = 72 - text_len - prompt_len;
	cursor = 0;
	cursor_offset = find_second_percent( Text_name_input );
	cursor_offset -= 2;
	cursor_offset += strlen( prompt);
	redraw = 1;
	while( 1 )
	{
		if ( redraw )
		{
			if ( secure )
			{
				for ( i = 0;
				      ( i < strlen( program ) ) && ( i < 80 );
				      i++ )
				{
					secure_string[ i ] = '*';
				}
				secure_string[ i ] = '\0';

				sprintf( string, Text_name_input,
					 prompt, max, max, secure_string );
			}
			else
			{
					/* " >>> %s: %-*.*s <<< " */
				sprintf( string, Text_name_input,
					 prompt, max, max, program );
			}
			show_prompt( string, 7, cursor_offset + cursor );
		}
		redraw = 1;
		c = key_read( 0, Map_name_input );
		if ( ( c == ' ' ) && ( cursor == 0 ) )
		{
			program[ 0 ] = '\0';
			return( -1 );
		}
		else if ( ( c >= ' ' ) && ( c < 0x7F ) )
		{				/* printable */
			if ( cursor < max )
			{
				if ( ( c != ' ' ) || ( cursor != 0 ) )
				{
					program[ cursor ] = c;
					cursor++;
					program[ cursor ] = '\0';
					if ( Prompting_on )
					{
						if ( secure )
							term_putc( '*' );
						else
							term_putc( c );
						redraw = 0;
					}
				}
			}
			else
				term_beep();
		}
		else if (  c == '\b'            || c == 0x7F 
			|| c == FTKEY_BACKSPACE || c == FTKEY_LEFT )
		{
			if ( cursor > 0 )
			{
				cursor--;
				program[ cursor ] = '\0';
				if ( Prompting_on )
				{
					term_backspace();
					term_putc( ' ' );
					term_backspace();
					redraw = 0;
				}
			}
			else
				term_beep();
		}
		else if ( ( c == '\r' ) || ( c == FTKEY_RETURN ) )
		{
			return( 0 );
		}
		else if ( c == '\033' )
		{
			program[ 0 ] = '\0';
			return( -1 );
		}
		else
			term_beep();
	}
}
/**************************************************************************
* find_second_percent
*	Return the index of the second "%" character in "string".
*	This is used to position the cursor in prompt strings that
*	that are user modifiable.
**************************************************************************/
find_second_percent( string )
	char	*string;
{
	int	len;
	int	first_found;
	int	i;

	len = strlen( string );
	first_found = 0;
	for ( i = 0; i < len; i++ )
	{
		if ( string[ i ] == '%' )
		{
			if ( first_found )
				break;
			else
				first_found = 1;
		}
	}
	return( i );
}
char	*Text_get_window_idle_window =
	"Idle Window";
/**************************************************************************
* get_idle_window
*	Prompt user and input the desired window number of the idle window.
*	"p_winno" is a pointer to the integer where the result should go.
**************************************************************************/
get_idle_window( p_winno )
	int	*p_winno;
{
			/* "Idle Window" */
	get_window_number_idle( Text_get_window_idle_window, p_winno );
}
char	*Text_get_window_windows_window =
	"Windows Window";
/**************************************************************************
* get_windows_window
*	Prompt user and input the desired window number of the windows window.
*	"p_winno" is a pointer to the integer where the result should go.
**************************************************************************/
get_windows_window( p_winno )
	int	*p_winno;
{
			/* "Windows Window" */
	get_window_number( Text_get_window_windows_window, p_winno );
}
char	*Map_window_mode_prompt_popup = "";
/**************************************************************************
* get_overwrite_window
*	Prompt, input and return the window number of the popup window.
*	Return -1 for invalid.
**************************************************************************/
get_overwrite_window()
{
	int	c;
	int	winno;

	show_window_prompt( Curwinno, 'z' );
	c = key_read( 0, Map_window_mode_prompt_popup );
	if ( ( c >= '0' ) && ( c <= '9' ) )
	{
		if ( c == '0' )
			winno = 9;
		else
			winno = c - '1';
		if ( winno < Wins )
			return( winno );
	}
	term_beep();
	term_beep();
	term_beep();
	return( -1 );
}
char	 *Map_get_window = "";
char	*Text_get_window =
	" >>> %s %2d      <<< ";
char	*Text_get_window_next_active =
	" >>> %s  Active <<< ";
char	*Text_get_window_menu =
	" >>> %s %2d Menu <<< ";
char	*Text_get_window_none =
	" >>> %s  None   <<< ";
/**************************************************************************
* get_window_number
*	Input a windows window number.
*	using name for a prompt.  Return the result in the integer
*	pointed to by "p_winno".
*	Accept N for none.
*	Put the cursor on the right digit of the number.
**************************************************************************/
get_window_number( name, p_winno )
	char	*name;
	int	*p_winno;
{
	char	string[ 80 ];
	int	c;
	int	winno;
	int	name_len;
	int	tempwinno;
	int	cursor_offset;

	cursor_offset = find_second_percent( Text_get_window );
	cursor_offset--;
	winno = *p_winno;
	name_len = strlen( name );
	while( 1 )
	{
			/* " >>> %s %2d       <<< " */
			/* " >>> %s  None    <<< " */
			/* " >>> %s  Active  <<< " */
		if ( ( winno >= 0 ) && ( winno < Wins ) )
		{
			sprintf( string, Text_get_window, name, winno + 1 );
		}
		else
			sprintf( string, Text_get_window_none, name );
		show_prompt( string, 20, cursor_offset + name_len );
		c = key_read( 0, Map_get_window );
		if ( ( c >= '0' ) && ( c <= '9' ) )
		{
			if ( c == '0' )
				tempwinno = 9;
			else
				tempwinno = c - '1';
			if ( tempwinno < Wins )
				winno = tempwinno;
			else
				term_beep();
		}
		else if ( ( c == 'n' ) || ( c == 'N' ) )
			winno = -1;
		else if ( ( c == '\r' ) || ( c == FTKEY_RETURN ) )
		{
			*p_winno = winno;
			return;
		}
		else if ( ( c == '\033' ) || ( c == ' ' ) )
			return;
		else
			term_beep();
	}
}
#include "options.h"
/**************************************************************************
* get_window_number_idle
*	Input an idle window number.
*	using name for a prompt.  Return the result in the integer
*	pointed to by "p_winno".
*	Accept N for none.
*	Accept A for next active.
*	Put the cursor on the right digit of the number.
**************************************************************************/
get_window_number_idle( name, p_winno )
	char	*name;
	int	*p_winno;
{
	char	string[ 80 ];
	int	c;
	int	winno;
	int	name_len;
	int	tempwinno;
	int	cursor_offset;
	char	idle_window_paste_chars[ IDLE_WINDOW_PASTE_CHARS_MAX + 1 ];

	strcpy( idle_window_paste_chars, Idle_window_paste_chars );
	cursor_offset = find_second_percent( Text_get_window );
	cursor_offset--;
	winno = *p_winno;
	name_len = strlen( name );
	while( 1 )
	{
			/* " >>> %s %2d Paste <<< " */
			/* " >>> %s %2d       <<< " */
			/* " >>> %s  None    <<< " */
			/* " >>> %s  Active  <<< " */
		if ( ( winno >= 0 ) && ( winno < Wins ) )
		{
			if ( idle_window_paste_chars[ 0 ] == '\0' )
			{
				sprintf( string,
					 Text_get_window, name, winno + 1 );
			}
			else
			{
				sprintf( string,
					 Text_get_window_menu,
					 name, winno + 1 );
			}
		}
		else if ( winno == IDLE_WINDOW_IS_NEXT_ACTIVE )
		{
			sprintf( string, Text_get_window_next_active, name );
		}
		else
			sprintf( string, Text_get_window_none, name );
		show_prompt( string, 20, cursor_offset + name_len );
		c = key_read( 0, Map_get_window );
		if ( ( c >= '0' ) && ( c <= '9' ) )
		{
			if ( c == '0' )
				tempwinno = 9;
			else
				tempwinno = c - '1';
			if ( tempwinno < Wins )
			{
				winno = tempwinno;
				idle_window_paste_chars[ 0 ] = '\0';
			}
			else
				term_beep();
		}
		else if ( ( c == 'a' ) || ( c == 'A' ) )
		{
			winno = IDLE_WINDOW_IS_NEXT_ACTIVE;
			idle_window_paste_chars[ 0 ] = '\0';
		}
		else if ( ( c == 'n' ) || ( c == 'N' ) )
		{
			winno = -1;
			idle_window_paste_chars[ 0 ] = '\0';
		}
		else if ( ( c == 'm' ) || ( c == 'M' ) )
		{
			if ( Opt_menu_hotkey != '\0' )
				idle_window_paste_chars[ 0 ] = 
						Opt_menu_hotkey;
			else
				idle_window_paste_chars[ 0 ] = 
						Windows_window_char_start;
			idle_window_paste_chars[ 1 ] = '\0';
		}
		else if ( ( c == '\r' ) || ( c == FTKEY_RETURN ) )
		{
			if ( winno < 0 )
				Idle_window_paste_chars[ 0 ] = '\0';
			else
				strcpy( Idle_window_paste_chars,
					idle_window_paste_chars );
			*p_winno = winno;
			return;
		}
		else if ( ( c == '\033' ) || ( c == ' ' ) )
			return;
		else
			term_beep();
	}
}
char	*Prmt_get_command_window = "";
char	 *Map_get_command_window_monitor = "";
char	*Text_get_command_window_monitor =
	"Window %2d monitor mode (window #, Y or N)";
/**************************************************************************
* get_monitor_input
*	Prompt user and input and set windows into and out of 
*	monitor mode.
**************************************************************************/
get_monitor_input( curwinno )
	int	curwinno;
{
	/* "Window %2d monitor mode (window #, Y or N)" */
	get_window_switch_input( curwinno, 
				 Text_get_command_window_monitor, 
				  Map_get_command_window_monitor, 
				 Window_monitor );
}
char	 *Map_get_command_window_invisible = "";
char	*Text_get_command_window_invisible =
	"Window %2d invisible to scan (window #, Y or N)";
/**************************************************************************
* get_invisible_input
*	Prompt user, input, and set windows invisible to scan.
*	I.E. ^W+ and ^W- skip these windows.
**************************************************************************/
get_invisible_input( curwinno )
	int	curwinno;
{
	/* "Window %2d invisible to scan (window #, Y or N)" */
	get_window_switch_input( curwinno, 
				 Text_get_command_window_invisible, 
				  Map_get_command_window_invisible, 
				 Window_invisible );
}
char	 *Map_get_command_window_notify = "";
char	*Text_get_command_window_notify =
	"Window %2d notify when current (window #, Y or N)";
/**************************************************************************
* get_notify_input
*	Prompt user, input, and set windows notify when current.
*	I.E. Send the notify when current char to this window when it
*	becomes the current window.
**************************************************************************/
get_notify_input( curwinno )
	int	curwinno;
{
	/* "Window %2d notify when current (window #, Y or N)" */
	get_window_switch_input( curwinno, 
				 Text_get_command_window_notify, 
				  Map_get_command_window_notify, 
				 Window_notify );
}
char	 *Map_get_command_window_transparent = "";
char	*Text_get_command_window_transparent =
	"Window %2d transparent mode (window #, Y or N)";
/**************************************************************************
* get_transparent_input
*	Prompt user, input, and set windows into and out of transparent
*	mode.  In transparent mode the window goes directly to the terminal
*	without facetterm checking or remembering the output.
*	This is for test purposes only.
**************************************************************************/
get_transparent_input( curwinno )
	int	curwinno;
{
	/* "Window %2d transparent mode (window #, Y or N)" */
	get_window_switch_input( curwinno, 
				 Text_get_command_window_transparent, 
				  Map_get_command_window_transparent, 
				 Window_transparent );
}
char	 *Map_get_command_window_blocked = "";
char	*Text_get_command_window_blocked =
	"Window %2d blocked (window #, Y or N)";
/**************************************************************************
* get_blocked_input
*	Prompt user, input, and set windows into and out of blocked mode.
*	I.E. do not read window unless it is current.  This does not 
*	actually block the window but as it outputs, its buffers will fill
*	and the window will block on high water.
**************************************************************************/
get_blocked_input( curwinno )
	int	curwinno;
{
	/* "Window %2d blocked (window #, Y or N)" */
	get_window_switch_input( curwinno, 
				 Text_get_command_window_blocked, 
				  Map_get_command_window_blocked, 
				 Window_blocked );
	set_window_blocked_mask( Window_blocked );
}
#include "readwindow.h"
char	 *Map_get_command_window_printer = "";
char	*Text_get_command_window_printer =
	"Window %2d printer mode (window #, Y or N)";
/**************************************************************************
* get_printer_input
*	Prompt user, input, and set windows into and out of printer mode.
*	I.E. the window is forced to transparent print.
**************************************************************************/
get_printer_input( curwinno )
	int	curwinno;
{
	int	winno;

	/* "Window %2d printer mode (window #, Y or N)" */
	winno = get_window_switch_input(
				curwinno, 
				Text_get_command_window_printer, 
				 Map_get_command_window_printer, 
				Window_printer );
	if ( winno >= 0 )
	{
		Window_printer_disable = 0;
		if ( Window_printer[ winno ] )
		{
			Read_window_max[ winno ] = 
				Transparent_print_read_window_max;
		}
		else
		{
			Read_window_max[ winno ] = READ_WINDOW_MAX;
			clear_transparent_print_hold( winno );
			check_print_inactive( winno );
		}
		set_window_printer_mask();
	}
	return( winno );
}
/**************************************************************************
* get_window_switch_input
*	Prompt user ( with "prompt" ), input, and set window into and out
*	of the various modes above.  "window_switch_array" is a per window
*	set of switches that are on or off for each window.
*	"mapping" is a user definable remap of keystrokes to what the
*	program is expecting. E.G. 'j' for "ja" is mapped to 'y' for "yes".
*	"curwinno" is the current ( default ) window number.
**************************************************************************/
get_window_switch_input( curwinno, prompt, mapping, window_switch_array )
	int	curwinno;
	char	*prompt;
	char	*mapping;
	int	window_switch_array[];
{
	int	winno;
	char	buff[ 80 ];
	char	current[ 2 ];
	int	c;
	char	map_prompt();


	winno = Redirected_winno;
	if ( winno < 0 )
		winno = get_key_read_pkt_winno();
	if ( winno < 0 )
		winno = curwinno;
	refresh_prompt_area();
	while( 1 )
	{
		/* "Window %2d notify when current (window #, Y or N)" */
		sprintf( buff, prompt, winno + 1 );
		if ( window_switch_array[ winno ] )
			current[ 0 ] = map_prompt( 'Y',
						   Prmt_get_command_window );
		else
			current[ 0 ] = map_prompt( 'N',
						   Prmt_get_command_window );
		current[ 1 ] = '\0';
		c = get_command_char( buff, current, mapping );	/* INTER */
		if ( c == 'Y' || c == 'y' )
		{
			window_switch_array[ winno ] = 1;
			return( winno );
		}
		else if ( c == 'N' || c == 'n' )
		{
			window_switch_array[ winno ] = 0;
			return( winno );
		}
		else if ( c >= '1' && c <= '9' )
			winno = c - '1';
		else if ( c == '0' )
			winno = 9;
		else if ( ( c == '\r' ) || ( c == FTKEY_RETURN ) )
			return( -1 );
		else if ( c == -1 )
			return( -1 );
		else
			term_beep();
	}
}
char	 *Map_get_command_send_hangup = "";
char	*Text_get_command_send_hangup =
	"SEND HANGUP ? (N=No Y=Current A=All K=Kill)";
/**************************************************************************
* get_hangup_input
*	Prompt user, input, and send signals to windows.
**************************************************************************/
get_hangup_input()
{
	int	c;
	int	winno;

	refresh_prompt_area();
	if ( Opt_disable_quit_while_windows_active )
	{
		term_beep();
		return;
	}
	winno = Redirected_winno;
	if ( winno < 0 )
		winno = get_key_read_pkt_winno();
	if ( winno < 0 )
		winno = Curwinno;
		/* "SEND HANGUP ? (N=No Y=Current A=All K=Kill)" */
	c = get_command_char( Text_get_command_send_hangup, "",
			       Map_get_command_send_hangup );
	if ( c == 'y' || c == 'Y' )
	{
		if ( Prompting_on )
			putchar( 'Y' );
		term_outgo();
		fsend_hangup_to_winno( winno );
	}
	else if ( c == 'a' || c == 'A' )
	{
		if ( Prompting_on )
			putchar( 'A' );
		term_outgo();
		fsend_hangup();
	}
	else if ( c == 'k' || c == 'K' )
	{
		if ( Prompting_on )
			putchar( 'K' );
		term_outgo();
		fsend_kill_to_winno( winno );
	}
	else if ( c == 'n' || c == 'N' || c == -1 )
	{
	}
	else
		term_beep();
}
char	*Activate_screen_saver = "Y";
char	*Prmt_get_command_screen_saver = "";
char	 *Map_get_command_screen_saver = "";
char	*Text_get_command_screen_saver =
	"Activate screen saver ? (Y or N)";
#include "screensave.h"
/**************************************************************************
* get_screen_saver_input
*	Prompt user, input, and activate or deactivate screen saver.
**************************************************************************/
get_screen_saver_input()
{
	int	c;
	int	defaulted;
	char	prompt[ 2 ];
	char	promptc;
	char	map_prompt();

	refresh_prompt_area();
	prompt[ 0 ] = map_prompt( *Activate_screen_saver,
				  Prmt_get_command_screen_saver );
	prompt[ 1 ] = '\0';
		/* "Activate screen saver ? (Y or N)" */
	c = get_command_char( Text_get_command_screen_saver, 
			      prompt,		/* INTER */
			       Map_get_command_screen_saver );
	if ( ( c == '\r' ) || ( c == FTKEY_RETURN ) )
	{
		c = *Activate_screen_saver;
		defaulted = 1;
	}
	else
		defaulted = 0;
	if ( c == 'y' || c == 'Y' )
	{
		if ( Prompting_on )
		{
			promptc = map_prompt( 'Y',
				  Prmt_get_command_screen_saver );
			putchar( promptc );
		}
		term_outgo();
		Activate_screen_saver = "Y";
		if ( defaulted == 0 )
		{
			if ( Screen_saver_timer <= 0 )
				Screen_saver_timer = DEFAULT_SCREEN_SAVER_TIMER;
			fct_window_mode_ans_int( 's', Screen_saver_timer );
		}
		return( 0 );
	}
	else if ( c == 'n' || c == 'N' )
	{
		Activate_screen_saver = "N";
		if ( defaulted == 0 )
			fct_window_mode_ans_int( 's', 0 );
		return( -1 );
	}
	else if ( c == -1 )
	{
		return( -1 );
	}
	else
	{
		term_beep();
		return( -1 );
	}
}
char	 *Map_get_command_screen_lock = "";
char	*Text_get_command_screen_lock =
	"Activate screen lock ? (Y or N)";
char	*Text_name_input_password =
	"Enter lock word";
char	*Text_name_input_password2 =
	"Reenter lock word";
char	*Text_name_input_unpassword =
	"Enter unlock word";
#define MAX_PASSWORD 10
char	Password[ MAX_PASSWORD + 1 ];
/**************************************************************************
* get_screen_lock_input
*	Prompt user, input.
**************************************************************************/
get_screen_lock_input()
{
	int	c;
	int	defaulted;
	char	password2[ MAX_PASSWORD + 1 ];

	refresh_prompt_area();
		/* "Activate screen lock ? (Y or N)" */
	c = get_command_char( Text_get_command_screen_lock, 
			      "",
			       Map_get_command_screen_lock );
	if ( c == 'y' || c == 'Y' )
	{
		if ( Prompting_on )
			putchar( 'Y' );
		term_outgo();
		Password[ 0 ] = '\0';
		get_name_input( Password, MAX_PASSWORD,
				Text_name_input_password, 1 );
		blank_trim( Password );
		if ( Password[ 0 ] != '\0' )
		{
			password2[ 0 ] = '\0';
			get_name_input( password2, MAX_PASSWORD,
					Text_name_input_password2, 1 );
			blank_trim( password2 );
			if ( strcmp( Password, password2 ) == 0 )
			{
				return( 0 );
			}
			term_beep();
		}
		return( -1 );
	}
	else if ( c == 'n' || c == 'N' )
	{
		return( -1 );
	}
	else if ( c == -1 )
	{
		return( -1 );
	}
	else
	{
		term_beep();
		return( -1 );
	}
}
/**************************************************************************
* get_run_raw_input
*	Prompt user, input, and run program on raw tty.
**************************************************************************/
get_run_raw_input()
{
	char	program[ MAX_PROG + 1 ];

	refresh_prompt_area();
	if ( Opt_disable_control_W_x_excl )
	{
		term_beep();
		return( -1 );
	}
	program[ 0 ] = '\0';
		/* "Run RAW tty" */
	get_name_input( program, MAX_PROG, Text_name_input_run_raw_tty, 0 );
	blank_trim( program );
	if ( program[ 0 ] != '\0' )
	{
		if ( mT_out_run_raw_tty_start != NULL )
			term_tputs( mT_out_run_raw_tty_start );
		out_term_clear_screen();
		termio_normal();
		printf( Text_facetterm_run_raw_tty );
		term_outgo();
		mysystem( program );
		printf( Text_facetterm_resuming );
		term_outgo();
		wait_return_pressed_continue();
		termio_window();
		out_term_clear_screen();
		if ( mT_out_run_raw_tty_end != NULL )
			term_tputs( mT_out_run_raw_tty_end );
		/**********************************************************
		* HP commands used in script damage keypad transmit and 
		* insert mode.
		**********************************************************/
		t_reload_keypad_xmit();
		t_reload_insert_mode();
		return( 0 );
	}
	return( -1 );
}
#define FUNC_CONTROL_8_BIT	1
#define FUNC_TRANSPARENT_PRINT	2
/**************************************************************************
* get_mode_inputs
*	Prompt user, input, and set modes for a window.
*	This list of modes depends on the capabilities present in the
*	terminal description.
**************************************************************************/
get_mode_inputs()
{
	int	line;
	int	number_of_lines;
	char	*prompt[ 10 ];
	int	index[ 10 ];
	int	max_index[ 10 ];
	char	*names[ 50 ];
	int	func[ 10 ];
	int	i;
	int	result;

	number_of_lines = 0;
	/******************************************************************
	* See if terminal mode is settable.
	******************************************************************/
	if ( get_terminal_mode_mode( &prompt[ number_of_lines ],
				     &index[ number_of_lines ],
				     &max_index[ number_of_lines ],
				     &names[ number_of_lines * 5 ] ) == 0 )
	{
		func[ number_of_lines ] = FUNC_CONTROL_8_BIT;
		number_of_lines++;
	}
	/******************************************************************
	* See if transparent print is settable
	******************************************************************/
	if ( get_transparent_print_mode( 
					&prompt[ number_of_lines ],
					&index[ number_of_lines ],
					&max_index[ number_of_lines ],
					&names[ number_of_lines * 5 ] ) == 0 )
	{
		func[ number_of_lines ] = FUNC_TRANSPARENT_PRINT;
		number_of_lines++;
	}
	if ( number_of_lines <= 0 )
	{
		term_beep();
		return;
	}
	line = 0;
	/******************************************************************
	* Allow user to choose.
	******************************************************************/
	while( 1 )
	{
		result = get_mode_input( prompt[ line ],
					 &index[ line ], max_index[ line ],
					 &names[ line * 5 ] );
		if ( result == 0 )
		{
			/**************************************************
			* Set window ( and terminal if current ) to modes.
			**************************************************/
			for ( i = 0; i < number_of_lines; i++ )
			{
			    if ( func[ i ] == FUNC_CONTROL_8_BIT )
				put_terminal_mode_mode( index[ i ] );
			    else if ( func[ i ] == FUNC_TRANSPARENT_PRINT )
				put_transparent_print_mode( index[ i ] );
			}
			return;
		}
		else if ( result == 1 )
		{
			line--;
			if ( line < 0 )
				line = number_of_lines - 1;
		}
		else if ( result == 2 )
		{
			line++;
			if ( line >= number_of_lines )
				line = 0;
		}
		else
			return;
	}
}
char	 *Map_get_mode_input = "";
char	*Text_get_mode_input = 
	" >>> %-15.15s %-15.15s RETURN=set SPACE=cancel <<< ";
/**************************************************************************
* get_mode_input
*	Allow user to cycle through choices, or specify up or down, or
*		cancel.
*	"prompt" is the general type of this line.
*	"names" are the symbolic identifiers of the different possibilities
*		for this type.
*	There are "max_index" + 1 of these "names" and
*		"p_index" is a pointer to an integer specifying the 
*		current setting.
**************************************************************************/
get_mode_input( prompt, p_index, max_index, names )
	char	*prompt;
	int	*p_index;
	int	max_index;
	char	*names[];
{
	int	c;
	char	string[ 80 ];
	int	new_index;

	new_index = *p_index;
	while( 1 )
	{
		 /* " >>> %-15.15s %-15.15s RETURN=set SPACE=cancel <<< " */
		sprintf( string, Text_get_mode_input,
			 prompt, names[ new_index ] );
		show_prompt( string, 7, 21 );
		c = key_read( 0, Map_get_mode_input );
		if ( ( c == '\r' ) || ( c == FTKEY_RETURN ) )
		{
			*p_index = new_index;
			return( 0 );
		}
		else if (  c == 'l'             || c == 'L'
			|| c == 'p'             || c == 'P'
			|| c == '\b'
		        || c == FTKEY_BACKSPACE || c == FTKEY_LEFT )
		{
			new_index--;
			if ( new_index < 0 )
				new_index = max_index;
		}
		else if (  c == 'r'             || c == 'R'
			|| c == 'n'             || c == 'N'
		        || c == FTKEY_RIGHT )
		{
			new_index++;
			if ( new_index > max_index )
				new_index = 0;
		}
		else if (  c == 'u'             || c == 'U'
		        || c == FTKEY_UP )
		{
			*p_index = new_index;
			return( 1 );
		}
		else if (  c == 'd'             || c == 'D'
		        || c == FTKEY_DOWN )
		{
			*p_index = new_index;
			return( 2 );
		}
		else if ( ( c == ' ' )  || ( c == '\033' ) )
		{
			return( -1 );
		}
		else
			term_beep();
	}
}
char	*Text_get_command =
	" >>> %s: %s  <<< ";
/**************************************************************************
* get_command_char
*	Prompt the user (with "prompt"), and input and return a character.
*	Return -1 if ESC or SPACE is entered.
*	The default answer is "default_ans" and
*	the character input is subject to remapping with "mapping".
**************************************************************************/
get_command_char( prompt, default_ans, mapping )
	char	*prompt;
	char	*default_ans;
	char	*mapping;
{
	int	cursor;
	int	cursor_offset;
	int	c;
	char	string[ 80 ];

			/* " >>> %s: %s  <<< " */
	cursor = 0;
	cursor_offset = find_second_percent( Text_get_command );
	cursor_offset -= 2;
	cursor_offset += strlen( prompt);
	sprintf( string, Text_get_command, prompt, default_ans );
	show_prompt( string, 7, cursor_offset + cursor );
	c = key_read( 0, mapping );
	if ( ( c == ' ' ) || ( c == '\033' ) )
		return( -1 );
	else
		return( c );
}
int Prompt_split_start = 0;
int Prompt_length = 0;
int Prompt_cursor_offset = 0;
#define MAX_PROMPT 79
char Prompt_string[ MAX_PROMPT + 1 ];
/**************************************************************************
* turn_prompting_off
*	This turns off the window command mode prompting so that the
*	user does not see prompts go by when the command is coming in
*	from a program through an ioctl.
**************************************************************************/
turn_prompting_off()
{
	if ( Prompt_length )
		refresh_prompt_area();
	Prompting_on = 0;
}
/**************************************************************************
* turn_prompting_on
*	This turns the prompting back on and outputs the prompt that 
*	would have been visible had it not been off to start with.
**************************************************************************/
turn_prompting_on()
{
	if ( Prompt_length )
		show_prompt_output();
	Prompting_on = 1;
}
/**************************************************************************
* show_prompt
*	Record the current prompt and output it if prompting is on.
**************************************************************************/
show_prompt( string, split_start, cursor_offset )
	char	*string;
	int	split_start;
	int	cursor_offset;
{
	strncpy( Prompt_string, string, MAX_PROMPT );
	Prompt_string[ MAX_PROMPT ] = '\0';
	Prompt_length = strlen( string );
	Prompt_cursor_offset = cursor_offset;
	if ( Curwin->fullscreen == 0 )
		Prompt_split_start = split_start;
	else
		Prompt_split_start = 0;
	if ( Prompting_on )
		show_prompt_output();
}
/**************************************************************************
* refresh_prompt_area
*	Restore the prompt area ( the bottom line of the screen ). If 
*	prompting was not on, then it is already ok.
**************************************************************************/
refresh_prompt_area()
{
	if ( Prompting_on )
		refresh_prompt_area_output();
	Prompt_length = 0;
}
/**************************************************************************
* show_prompt_output
*	Output the prompt at the correct spot on the screen, and position
*	the cursor appropriately.
**************************************************************************/
show_prompt_output()
{
	if ( Prompt_split_start )
		term_pos_puts( Row_bottom_terminal, Prompt_split_start, 
			       Prompt_string );
	else
	{
		term_pos_clrlin_puts( Row_bottom_terminal, Prompt_string );
		term_pos( Row_bottom_terminal, 0 );
		term_nomagic();
	}
	term_pos( Row_bottom_terminal, 
		  Prompt_split_start + Prompt_cursor_offset );
}
char *sSplit_divider = { (char *) 0 };
char *Split_divider_default = 
" ============================================================================";
char *sT_split_divider_start =  { NULL };
char *sT_split_divider_end = { NULL };

#if defined( VPIX ) || defined( SOFTPC )
char *Pc_split_divider = 
" ============================================================================";
char *T_pc_split_divider_start = { NULL };
char *T_pc_split_divider_end = { NULL };
#endif

/**************************************************************************
* extra_split
*	TERMINAL DESCRIPTION PARSER module for "split_divider...".
*	TERMINAL DESCRIPTION PARSER module for "out_screen_saver...".
**************************************************************************/
/*ARGSUSED*/
extra_split( name, string, attribute_on_string, attribute_off_string )
	char	*name;
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	int	match;
	char	*dec_encode();

	match = 1;
	if ( strcmp( name, "split_divider" ) == 0 )
	{
		xSplit_divider = dec_encode( string );
	}
	else if ( strcmp( name, "split_divider_start" ) == 0 )
	{
		xT_split_divider_start = dec_encode( string );
	}
	else if ( strcmp( name, "split_divider_end" ) == 0 )
	{
		xT_split_divider_end = dec_encode( string );
	}
	else if ( strcmp( name, "pc_split_divider" ) == 0 )
	{
#if defined( VPIX ) || defined( SOFTPC )
		Pc_split_divider = dec_encode( string );
#endif
	}
	else if ( strcmp( name, "pc_split_divider_start" ) == 0 )
	{
#if defined( VPIX ) || defined( SOFTPC )
		T_pc_split_divider_start = dec_encode( string );
#endif
	}
	else if ( strcmp( name, "pc_split_divider_end" ) == 0 )
	{
#if defined( VPIX ) || defined( SOFTPC )
		T_pc_split_divider_end = dec_encode( string );
#endif
	}
	else if ( strcmp( name, "out_screen_saver_start" ) == 0 )
	{
		xT_out_screen_saver_start = dec_encode( string );
	}
	else if ( strcmp( name, "out_screen_saver_end" ) == 0 )
	{
		xT_out_screen_saver_end = dec_encode( string );
	}
	else if ( strcmp( name, "out_run_raw_tty_start" ) == 0 )
	{
		xT_out_run_raw_tty_start = dec_encode( string );
	}
	else if ( strcmp( name, "out_run_raw_tty_end" ) == 0 )
	{
		xT_out_run_raw_tty_end = dec_encode( string );
	}
	else
		match = 0;
	return( match );
}
/**************************************************************************
* use_pc_split_divider
*	Return 1 if the terminal is in a special pc mode.  0 otherwise.
**************************************************************************/
use_pc_split_divider()
{
#ifdef SOFTPC
	if ( terminal_mode_is_scan_code_mode() )
		return( 1 );
#endif
	return( 0 );
}
/**************************************************************************
* refresh_prompt_area_output
*	Restore the prompt area.
**************************************************************************/
refresh_prompt_area_output()
{
	char	buffer[ 80 ];

	if ( Curwin->fullscreen == 0 )
	{
		if ( Opt_no_split_dividers )
		{
			sprintf( buffer, "%-*.*s", Prompt_length, Prompt_length,
"                                                                          " );
			term_pos_puts( Row_bottom_terminal, Prompt_split_start,
				       buffer );
		}
#if defined( VPIX ) || defined( SOFTPC )
		else if ( use_pc_split_divider() )
		{
			sprintf( buffer, "%-*.*s", Prompt_length, Prompt_length,
				&Pc_split_divider[ Prompt_split_start ] );
			if ( T_pc_split_divider_start != NULL )
				term_tputs( T_pc_split_divider_start );
			term_pos_puts( Row_bottom_terminal, Prompt_split_start,
				       buffer );
			if ( T_pc_split_divider_end != NULL )
				term_tputs( T_pc_split_divider_end );
		}
#endif
		else
		{
			if ( mSplit_divider == (char *) 0 )
				mSplit_divider = Split_divider_default;
			sprintf( buffer, "%-*.*s", Prompt_length, Prompt_length,
				&mSplit_divider[ Prompt_split_start ] );
			if ( mT_split_divider_start != NULL )
				term_tputs( mT_split_divider_start );
			term_pos_puts( Row_bottom_terminal, Prompt_split_start,
				       buffer );
			if ( mT_split_divider_end != NULL )
				term_tputs( mT_split_divider_end );
		}
	}
	else
	{
		term_pos( Row_bottom_terminal, 0 );
		out_term_clr_eol();
		toscreen_row( Curwin, Row_bottom_terminal, Row_bottom_terminal,
							NO_ATTRS_AT_END );
		term_carriage_return();
	}
}
/**************************************************************************
* w_all_off
*	Mark all windows off screen.
**************************************************************************/
w_all_off()
{
	int	i;

	for ( i = 0; i < Wins; i++ )
		Wininfo[ i ]->onscreen = 0;
}
/**************************************************************************
* w_overlaps_off
*	Mark all windows that occupy the same screen rows as window
*	"window" as off screen.
**************************************************************************/
w_overlaps_off( window )
	FT_WIN	*window;
{
	int	i;
	FT_WIN	*lookwin;

	for ( i=0; i < Wins; i++ )
	{
		lookwin = Wininfo[ i ];
		if ( lookwin->onscreen )
		{
			if (( lookwin->win_top_row <= window->win_bot_row )
			&&  ( lookwin->win_bot_row >= window->win_top_row ))
			{
				lookwin->onscreen = 0;
			}
		}
	}
}
char	*Text_full_switch_window =
	"Switching to FacetTerm Window %d";
/**************************************************************************
* tell_full_switch
*	Prompt user that a full screen window switch is in progress.
**************************************************************************/
tell_full_switch()
{
	char	buffer[ 80 ];

	if ( ( Prompting_on == 0 ) && Opt_quiet_switch_when_prompting_off )
		return;
			/* "Switching to FacetTerm Window %d" */
	sprintf( buffer, Text_full_switch_window, Curwinno + 1 );
	out_term_clr_row_bottom_terminal();
	term_puts( buffer );
}
char	*Text_full_refresh_window =
	"Refreshing FacetTerm Window %d";
/**************************************************************************
* tell_full_refresh
*	Prompt user that a full screen window refresh is in progress.
**************************************************************************/
tell_full_refresh()
{
	char	buffer[ 80 ];

	if ( ( Prompting_on == 0 ) && Opt_quiet_switch_when_prompting_off )
		return;
			/* "Refreshing FacetTerm Window %d" */
	sprintf( buffer, Text_full_refresh_window, Curwinno + 1 );
	out_term_clr_row_bottom_terminal();
	term_puts( buffer );
}
/**************************************************************************
* split_label
*	Output split screen dividers.
**************************************************************************/
split_label( row )
	int 	row;
{
	if ( Opt_no_split_dividers )
		term_pos_clrlin_puts( row, "" );
	else
		split_label_divider( row );
	term_pos( row, 0 );
	term_nomagic();
	if ( Opt_no_split_dividers )
		term_pos_clrlin_puts( Row_bottom_terminal, "" );
	else
		split_label_divider( Row_bottom_terminal );
	term_pos( Row_bottom_terminal, 0 );
	term_nomagic();
}
/**************************************************************************
* split_label_divider
*	Output split screen dividers on row "row".
**************************************************************************/
split_label_divider( row )
	int	row;
{
#if defined( VPIX ) || defined( SOFTPC )
	if ( use_pc_split_divider() )
	{
		if ( T_pc_split_divider_start != NULL )
			term_tputs( T_pc_split_divider_start );
		term_pos_clrlin_puts( row, Pc_split_divider );
		if ( T_pc_split_divider_end != NULL )
			term_tputs( T_pc_split_divider_end );
		return;
	}
#endif
	if ( mSplit_divider == (char *) 0 )
		mSplit_divider = Split_divider_default;
	if ( mT_split_divider_start != NULL )
		term_tputs( mT_split_divider_start );
	term_pos_clrlin_puts( row, mSplit_divider );
	if ( mT_split_divider_end != NULL )
		term_tputs( mT_split_divider_end );
}
/**************************************************************************
* split_number
*	Output split screen window numbers on split screen dividers.
**************************************************************************/
split_number( row, winno )
	int	row;
	int	winno;
{
	char		buffer[ 8 ];

	if ( Opt_no_split_numbers )
		return;
	sprintf( buffer, "%3d ", winno + 1 );
	term_pos_puts( row, 3, buffer );
	term_pos_puts( row, 71, buffer );
}
/**************************************************************************
* window_is_at_top
*	Return 1 if the window is in the top half of a split screen.
*	Return 0 otherwise.
**************************************************************************/
window_is_at_top( window )
	FT_WIN	*window;
{
	return( window->win_top_row == 0 );
}
/**************************************************************************
* window_is_at_bottom
*	Return 1 if the window is in the bottom half of a split screen.
*	Return 0 otherwise.
**************************************************************************/
window_is_at_bottom( window )
	FT_WIN	*window;
{
	return( window->win_top_row != 0 );
}
/**************************************************************************
* get_first_active_window
*	Return the window number of the lowest numbered active window.
*	Return -1 if all idle.
**************************************************************************/
get_first_active_window()	/* -1 = none */
{
	int	winno;

	for ( winno = 0; winno < Wins; winno++ )
	{
		if ( Window_active[ winno ] )
			return( winno );
	}
	return( -1 );
}
/**************************************************************************
* is_winno_only_active_window
*	Return true if "winno" is the only active window. ( all idle counts )
*	Return 0 if other windows are active.
**************************************************************************/
is_winno_only_active_window( in_winno )
	int	in_winno;
{
	int	winno;

	for ( winno = 0; winno < Wins; winno++ )
	{
		if ( winno == in_winno )
			continue;
		if ( Window_active[ winno ] )
			return( 0 );
	}
	return( 1 );
}
/**************************************************************************
* adjust_D_split_row
*	Move the split divider if it is too low because the number of
*	rows changed.
**************************************************************************/
adjust_D_split_row( max_split )
	int	max_split;
{
	if ( D_split_row > max_split )
	{			/* split is too low */
		D_split_row = max_split;
	}
}
/**************************************************************************
* check_split_windows
*	D_split_row may have decreased and/or Rows_terminal may have changed.
**************************************************************************/
check_split_windows()
{
	int	winno;
	FT_WIN	*win;
	int	low;

	for ( winno = 0; winno < Wins; winno++ )
	{
	    win = Wininfo[ winno ];
	    if ( win->onscreen )
	    {
		if ( window_is_at_top( win ) )
		{		
			while ( win->win_bot_row > D_split_row - 1 )
			{		/* top shrinking - steal from
					   bottom unless cursor there
					*/
				win->win_bot_row--;
				if ( win->row < win->buff_bot_row )
					win->buff_bot_row--;
				else
					win->buff_top_row++;
			}
		}
		else
		{
			if ( win->win_top_row > D_split_row + 1 )
			{		/* 2 row bottom window moved up
					*/
				low = win->win_top_row - ( D_split_row + 1 );
				win->win_top_row -= low;
				win->win_bot_row -= low;
			}
			while ( win->win_bot_row > Row_bottom_terminal - 1 )
			{		/* bottom shrinking - steal from
					   bottom unless cursor there
					*/
				win->win_bot_row--;
				if ( win->row < win->buff_bot_row )
					win->buff_bot_row--;
				else
					win->buff_top_row++;
			}
			while ( win->win_bot_row < Row_bottom_terminal - 1 )
			{		/* bottom expanding - add to
					   top unless at top.
					*/
				win->win_bot_row++;
				if ( win->buff_top_row > 0 )
					win->buff_top_row--;
				else
					win->buff_bot_row++;
			}
		}
	    }
	}
}
char	*Text_name_input_mapped_key =
	"Key and mapping";
#define MAX_MAP 41
/**************************************************************************
* get_mapped_key
*	Prompt user, input and store a key mapping.  The default window is
*	window number "to_winno".
**************************************************************************/
get_mapped_key( to_winno )
	int	to_winno;
{
	char map_line[ MAX_MAP + 1 ];
	char	encode_buffer[ MAX_MAP + 1 ];
	char	input_string[ 2 ];
	char	*temp_encode();

	map_line[ 0 ] = '\0';
	get_name_input( map_line, MAX_MAP, Text_name_input_mapped_key, 0 );
	if ( map_line[ 0 ] != '\0' )
	{
		temp_encode( map_line, encode_buffer );
		if ( strlen( encode_buffer ) < 2 )
		{
			term_beep();
			return;
		}
		input_string[ 0 ] = encode_buffer[ 0 ];
		input_string[ 1 ] = '\0';
		show_command_prompt( Curwinno, Text_cut_and_paste_paste );
		send_mapped( 'm', input_string, -1, 
				  &encode_buffer[ 1 ], to_winno );
	}
}
char	*Text_name_input_mapped_key_unmap =
	"Key to unmap";
/**************************************************************************
* get_mapped_unmap
*	Prompt user, input, and remove the mapping for the input key.
*	The default window is window number "to_winno".
**************************************************************************/
get_mapped_unmap( to_winno )
	int	to_winno;
{
	char map_line[ MAX_MAP + 1 ];
	char	encode_buffer[ MAX_MAP + 1 ];
	char	input_string[ 2 ];
	char	*temp_encode();

	map_line[ 0 ] = '\0';
	get_name_input( map_line, MAX_MAP, Text_name_input_mapped_key_unmap,
									0 );
	if ( map_line[ 0 ] != '\0' )
	{
		temp_encode( map_line, encode_buffer );
		if ( strlen( encode_buffer ) < 1 )
		{
			term_beep();
			return;
		}
		input_string[ 0 ] = encode_buffer[ 0 ];
		input_string[ 1 ] = '\0';
		show_command_prompt( Curwinno, Text_cut_and_paste_paste );
		send_mapped( 'u', input_string, -1, "", to_winno );
	}
}
/**************************************************************************
* send_mapped
*	Send a mapping command to the receiver who will do the actual 
*	mapping.
**************************************************************************/
send_mapped( map_type, input_string, on_winno, output_string, to_winno )
	char	map_type;
	char	*input_string;
	int	on_winno;
	char	*output_string;
	int	to_winno;
{
	fct_window_mode_ans( (char) 'm' );
	fct_window_mode_ans( map_type );
	fct_window_mode_ans( (char) ( '0' + on_winno ) ); 
	fct_window_mode_ans( (char) ( '0' + to_winno ) );
	fct_window_mode_ans( (char) input_string[ 0 ] );
	fct_window_mode_ans( (char) 0xFF );
	fct_window_mode_ans_str( output_string );   /* output_string */
	fct_window_mode_ans( (char) 0 );
}
/**************************************************************************
* fct_window_mode_ans_str
*	Send the null terminated string "string" to the receiver.
**************************************************************************/
fct_window_mode_ans_str( string )
	char	*string;
{
	char	c;

	while( ( c = *string++ ) != '\0' )
		fct_window_mode_ans( c );
}
/**************************************************************************
* get_mapped_filename
*	Prompt user, input, and load a mapping file. Not implemented.
**************************************************************************/
/*ARGSUSED*/
get_mapped_filename( winno )
	int	winno;
{
}
/**************************************************************************
* f_refresh_popup_block
*	Refresh the characters that were changed since the last push.
*	"level" is being popped.
**************************************************************************/
f_refresh_popup_block( popwin )
	FT_WIN	*popwin;
{
	int	row;
	int	first_col;
	int	last_col;

	find_col_changed( popwin, &first_col, &last_col );
	for ( row = Outwin->display_rows - 1; row >= 0; row-- )
	{
		if ( popwin->row_changed[ row ] )
			f_refresh_popup_cols( popwin,
					row, first_col, last_col );
	}
}
/**************************************************************************
* f_refresh_overwritten_cols
*	Refresh the characters on "row" from "first_col" to "last_col"
*	that were overwritten by a popup.
**************************************************************************/
f_refresh_overwritten_cols( row, first_col, last_col )
	int	row;
	int	first_col;
	int	last_col;
{
	int	len;
	int	fnb_col;
	int	lnb_col;
	int	full_line;
	FTCHAR	*string;

	if ( oF_hp_attribute )
	{
		/**********************************************************
		** Determine last non blank column that needs to be refreshed.
		**********************************************************/
		len = get_non_blank( Outwin, Outwin->ftchars[ row ], 
					Outwin->line_attribute[ row ],
					&fnb_col, &full_line,  &string );
		if ( len <= 0 )
			lnb_col = -1;
		else
			lnb_col = fnb_col + len - 1;
		/**********************************************************
		** If everything under popup is blank, clear and exit.
		*********************************************************/
		if ( lnb_col < first_col )
		{
			term_pos( row, lnb_col + 1 );
			out_term_clr_eol();
			return;
		}
		/**********************************************************
		** Clear under the popup and refresh to last non blank.
		*********************************************************/
		term_pos( row, first_col );
		out_term_clr_eol();
		f_refresh_cols( row, first_col, lnb_col);
	}
	else
		f_refresh_cols( row, first_col, last_col);
}
/**************************************************************************
* f_refresh_popup_cols
*	Refresh the characters on "row" from "first_col" to "last_col"
*	Use characters that were visible when current "level" was pushed.
**************************************************************************/
f_refresh_popup_cols( popwin, row, first_col, last_col )
	FT_WIN	*popwin;
	int	row;
	int	first_col;
	int	last_col;
{
	int	len;
	int	fnb_col;
	int	lnb_col;
	int	full_line;
	FTCHAR	*string;

	if ( oF_hp_attribute )
	{
		/**********************************************************
		** Determine last non blank column that needs to be refreshed.
		**********************************************************/
		len = get_non_blank( popwin, popwin->ftchars[ row ], 
					popwin->line_attribute[ row ],
					&fnb_col, &full_line,  &string );
		if ( len <= 0 )
			lnb_col = -1;
		else
			lnb_col = fnb_col + len - 1;
		/**********************************************************
		** If everything under popup is blank, clear and exit.
		*********************************************************/
		if ( lnb_col < first_col )
		{
			term_pos( row, lnb_col + 1 );
			out_term_clr_eol();
			return;
		}
		/**********************************************************
		** Clear under the popup and refresh to last non blank.
		*********************************************************/
		term_pos( row, first_col );
		out_term_clr_eol();
		f_refresh_cols_on_row( popwin->ftchars[ row ], 
					row, first_col, lnb_col );
	}
	else
		f_refresh_cols_on_row( popwin->ftchars[ row ], 
					row, first_col, last_col );
}
#define MAX_POPUP_SCREEN_ROWS 128
int	Popup_screen_first_row_slot[ MAX_POPUP_SCREEN_LEVEL + 1 ] = { 0 };
FTCHAR	*Popup_screen_ftchars[ MAX_POPUP_SCREEN_ROWS ]= { (FTCHAR *) 0 };
int	Popup_screen_row[ MAX_POPUP_SCREEN_ROWS ] = { 0 }; 
/**************************************************************************
* popup_rows_save
*	Save the rows changed at this level.
*	"level" is the push level.
*	window "win" is the window that is to be saved.
*	"first_row" and "last_row" are the changed rows.
**************************************************************************/
popup_rows_save( level, win, first_row, last_row )
	int	level;
	FT_WIN	*win;
	int	first_row;
	int	last_row;
{
	int	i;
	FTCHAR	*p;
	int	slot;
	int	j;
	long	*malloc_run();

	slot = Popup_screen_first_row_slot[ level ];
	for ( i = first_row; i <= last_row; i++ )
	{
		if ( slot >= MAX_POPUP_SCREEN_ROWS )
		{
			term_beep();
			break;
		}
		if ( Popup_screen_ftchars[ slot ] == ( FTCHAR *) 0 )
		{
			p = ( FTCHAR * ) malloc_run(
						Cols_wide * sizeof( FTCHAR ),
						"popup_screen_row" );
			if ( p == (FTCHAR *) 0 )
			{
				fsend_kill(
				"Facet process: popup chars malloc failed\r\n",
				0 );
			}
			Popup_screen_ftchars[ slot ] = p;
		}
		memcpy( (char *) Popup_screen_ftchars[ slot ], 
			(char *) win->ftchars[ i ], 
			(int) ( Cols_wide * sizeof( FTCHAR ) ) );
		Popup_screen_row[ slot ] = i; 
		slot++;
	}
	for ( j = level + 1; j < MAX_POPUP_SCREEN_LEVEL + 1; j++ )
		Popup_screen_first_row_slot[ j ] = slot;
}
/**************************************************************************
* popup_rows_restore
*	Restore the popup window "win" 's buffer with the saved lines
*	of all of the levels below "level".
*	This restores the buffer to the state it was when "level" was
*	pushed.
**************************************************************************/
popup_rows_restore( level, win )
	int	level;
	FT_WIN	*win;
{
	int	slot;
	int	i;

	slot = Popup_screen_first_row_slot[ level ];
	for ( i = 0; i < slot; i++ )
	{
		memcpy( (char *) win->ftchars[ Popup_screen_row[ i ] ], 
			(char *) Popup_screen_ftchars[ i ], 
			(int) ( Cols_wide * sizeof( FTCHAR ) ) );
	}
}
/**************************************************************************
* paste_command_printer
*	^W > p   paste the paste buffer to the .facetprint script.
**************************************************************************/
paste_command_printer()
{
	char	dotfacetprintname[ 20 ];

	sprintf( dotfacetprintname, ".%sprint", Facetname );
	paste_command_scriptname( dotfacetprintname );
}
char	*Text_name_input_paste_script =
	"Paste to script";
#define MAX_SCRIPT 41
/**************************************************************************
* paste_command_script
*	^W > s   paste the paste buffer to the script entered by the user.
**************************************************************************/
paste_command_script()
{
	char	script[ MAX_SCRIPT + 1 ];

	script[ 0 ] = '\0';
	get_name_input( script, MAX_SCRIPT, Text_name_input_paste_script, 0 );
	blank_trim( script );
	if ( script[ 0 ] != '\0' )
		paste_command_scriptname( script );
}
char	*Text_name_input_paste_filename =
	"Paste to File";
#define MAX_PASTE_FILENAME 41
/**************************************************************************
* paste_command_file
*	^W > f   paste the paste buffer to the file entered by the user.
*	"filetype" is
*		"O" for overwrite if it exists.
*		"A" for append.
*		Must be new otherwise.
**************************************************************************/
paste_command_file( filetype )
	char	filetype;
{
	char	filename[ MAX_PASTE_FILENAME + 1 ];
	int	status;

	filename[ 0 ] = '\0';
	get_name_input( filename, MAX_PASTE_FILENAME, 
		Text_name_input_paste_filename, 0 );
	blank_trim( filename );
	if ( filename[ 0 ] != '\0' )
	{
		if ( filetype == 'O' )
			status = put_paste_file( filename, "w" );
		else if ( filetype == 'A' )
			status = put_paste_file( filename, "a" );
		else if ( access( filename, 0 ) == -1 )
			status = put_paste_file( filename, "w" );
		else
		{
			term_beep();
			return;
		}
		if ( status < 0 )
			term_beep();
	}
}
/**************************************************************************
* paste_command_scriptname
*	Paste to the script "script".
**************************************************************************/
paste_command_scriptname( script )
	char	*script;
{
	char	filename[ L_tmpnam ];
	char	command[ 256 ];

	if ( generate_script_command( script, filename, (char *) 0, command )
	     < 0
	   )
	{
		term_beep();
		return;
	}
	if ( put_paste_file( filename, "w" ) < 0 )
	{
		term_beep();
		return;
	}
	chown( filename, getuid(), getgid() );
	mysystem( command );
}
#include "screensave.h"
int	Screen_saver_interval = 5;
char	*Text_screen_saver = 
	"FacetTerm - press any key to continue";
char	*User_text_screen_saver = (char *) 0;
char	*Text_screen_lock = 
	"FacetTerm";
char	*User_text_screen_lock = (char *) 0;
/**************************************************************************
* screen_saver
*	Run the screen saver exiting if a key is pressed or one of the
*	windows has something to output.
**************************************************************************/
screen_saver()
{
	int	len;
	int	row;
	int	col;
	char	*text;

	if ( User_text_screen_saver != (char *) 0 )
		text = User_text_screen_saver;
	else
		text = Text_screen_saver;
	if ( mT_out_screen_saver_start != NULL )
		term_tputs( mT_out_screen_saver_start );
	len = strlen( text );
	do
	{
		row = rand() % Rows;
		col = rand() % ( Cols - len - 1 );
		out_term_clear_screen();
		term_pos( row, col );
		term_puts( text );
		term_outgo();
		if ( any_windows_are_ready() )
			break;
	}
	while( key_read( Screen_saver_interval * 10, "" ) < 0 )
		;
	out_term_clear_screen();
	if ( mT_out_screen_saver_end != NULL )
		term_tputs( mT_out_screen_saver_end );
}
screen_lock()
{
	char	password2[ MAX_PASSWORD + 1 ];

	while( 1 )
	{
		screen_lock_screen_saver();
		password2[ 0 ] = '\0';
		get_name_input( password2, MAX_PASSWORD,
					Text_name_input_unpassword, 1 );
		blank_trim( password2 );
		if ( strcmp( Password, password2 ) == 0 )
		{
			return( 0 );
		}
		term_beep();
	}
}
screen_lock_screen_saver()
{
	int	len;
	int	row;
	int	col;
	char	*text;

	if ( User_text_screen_lock != (char *) 0 )
		text = User_text_screen_lock;
	else
		text = Text_screen_lock;
	if ( mT_out_screen_saver_start != NULL )
		term_tputs( mT_out_screen_saver_start );
	len = strlen( text );
	do
	{
		row = rand() % Rows;
		col = rand() % ( Cols - len - 1 );
		out_term_clear_screen();
		term_pos( row, col );
		term_puts( text );
		term_outgo();
	}
	while( key_read( Screen_saver_interval * 10, "" ) < 0 )
		;
	out_term_clear_screen();
	if ( mT_out_screen_saver_end != NULL )
		term_tputs( mT_out_screen_saver_end );
}
/**************************************************************************
* help
*	run the help screen and rebuild the screen when done.
**************************************************************************/
help()
{
	int	other_split;

	if ( Curwin->fullscreen == 0 )
		other_split = find_other_split_winno( Curwinno );
	if ( show_help() < 0 )
		term_beep();
	else if ( Outwin->fullscreen )
		rebuild_full_screen();
	else
		rebuild_split_screen( other_split );
}
/**************************************************************************
* rebuild_full_screen
*	Screen saver done.
**************************************************************************/
rebuild_full_screen()
{
	out_term_clear_screen();
	tell_full_refresh();
	scroll_memory_refresh();
	full_refresh();
	f_cursor();
}
/**************************************************************************
* show_help
*	Run the help script.
**************************************************************************/
show_help()
{
	char	help_script[ 80 ];
	char	scriptpath[ 1024 ];
	char	*p;

	strcpy( scriptpath, "SHELL=/ " );
	p = &scriptpath[ strlen( scriptpath ) ];
	sprintf( help_script, "%shelp", Facetprefix );
	if ( find_script( help_script, "/bin", p ) < 0 )
		return( -1 );
	out_term_clear_screen();
	termio_normal();
	if ( mysystem( scriptpath ) != 0 )
		sleep( 5 );
	termio_window();
	out_term_clear_screen();
	/******************************************************************
	* HP commands used in script damage keypad transmit and 
	* insert mode.
	******************************************************************/
	t_reload_keypad_xmit();
	t_reload_insert_mode();
	return( 0 );
}
/**************************************************************************
* find_script
*	Find the help script "script" looking in
*		current directory
*		$HOME directory
*		/usr/faceterm/"subdirectory"
*		/user/facet/"subdirectory"
*	Return the pathname found in "scriptpath".
*	Return -1 if not found, 0 otherwise.
**************************************************************************/
find_script( script, subdirectory, scriptpath )
	char	*script;
	char	*subdirectory;
	char	*scriptpath;
{
	char	*home;
	char	*getenv();
	int	status;

	strcpy( scriptpath, "./" );
	strcat( scriptpath, script );
	status = access( scriptpath, 1 );
	if ( status < 0 )
	{
		home = getenv( "HOME" );
		if ( home != NULL )
		{
			strcpy( scriptpath, home );
			strcat( scriptpath, "/" );
			strcat( scriptpath, script );
			status = access( scriptpath, 1 );
		}
	}
	if ( status < 0 )
	{
		strcpy( scriptpath, Facettermpath );
		strcat( scriptpath, subdirectory );
		strcat( scriptpath, "/" );
		strcat( scriptpath, script );
		status = access( scriptpath, 1 );
	}
	if ( status < 0 )
	{
		strcpy( scriptpath, Facetpath );
		strcat( scriptpath, subdirectory );
		strcat( scriptpath, "/" );
		strcat( scriptpath, script );
		status = access( scriptpath, 1 );
	}
	if ( status < 0 )
		return( -1 );
	return( 0 );
}
#ifdef lint
static int lint_conversion_warning_ok_4;
static int lint_alignment_warning_ok_5;
#endif
