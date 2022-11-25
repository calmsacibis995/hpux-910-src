/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: facetterm.c,v 70.1 92/03/09 15:42:10 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/**************************************************************************
* facetterm.c
*	Main module for startup and sender - forks and execs fct_key.
**************************************************************************/
#include	"version.h"
#include	<stdio.h>
#include        <sys/types.h>
#include        "facetwin.h"


/*
*/

#include	"facet.h"
#include	"facetterm.h"
#include	"talk.h"
#include	"ftterm.h"
#include	"ftwindow.h"
#include	"wins.h"
#include	"options.h"
int	Opt_quit_on_all_windows_idle = 0;

#include	<errno.h>

#include "facetpath.h"

/******************************************************************
* facetterm
*	Extract options from command line, environment, and .facet file.
*	Possibly override text string from .facettext files.
*	Create pipes for interprocess communication.
*	Proceed to main body of program.
******************************************************************/
main( argc, argv )
	int	argc;
	char	**argv;
{
	char		filename[ 256 ];

	set_options( argc, argv );
	sprintf( filename, "%stext", Facetname );
	load_foreign( filename );
	init_communications();
	common();
/* NOTREACHED */
}
/******************************************************************
* header
*	Called during startup to output the name ( or alias ),
*	version and copyright messages.
******************************************************************/
header()
{
/*               09876543210987654321*12345678901234567890                */
	printf( "\r\n" );
	printf(
 "******************************************************************************\n" );
	printf(
 "*                                   T S M                                    *\n" );
	printf( Ft_version );
	printf(
 "*        Copyright (c) Structured Software Solutions, Inc. 1986-1991.        *\n" );
	printf(
 "******************************************************************************\n" );
}
/**************************************************************************
* fsend_finish
*	Facetterm is about to exit.  Put the terminal back, as close as
*	possible, to the state that it was in, or was assumed to be in,
*	when we started.
*	This includes terminal mode, rows, columns, page 0, default
*	function keys, blank status line, etc.
*	The "exit_ca_mode=" lines from the .fi file are output.
*	The cursor is positioned to the lower left of the screen and
*	scrolled to a blank line for the shell prompt.
*	The screen is intentionally not cleared - would make it tough
*	to read an error msg.
**************************************************************************/
fsend_finish()
{
	modes_off_quit_start();
	t_sync_terminal_mode_normal();
	t_sync_rows_change_normal();
	t_sync_columns_normal();
	t_sync_page_number_normal();
	t_sync_function_key_default();
	t_sync_onstatus_quit();
	modes_off_quit();
	term_exit_ca_mode();
	term_pos( Row_bottom_terminal, 0 );
	term_putc( '\n' );
}
/**************************************************************************
* fsend_procack_array
*	Sender process "acks" coming from the receiver process.
*	These are one word messages from the receiver to the sender,
*	that have arrived through a pipe between them, that indicate
*	either an unsolicited  request for action ( e.g. Control-W 
*	pressed ) or a response to a signal sent to the receiver when
*	the sender wants the keyboard.
*	(The term ack is historical - this was, and still is, the path
*	used in the facet software for acks and naks of packets to the
*	PC.)
*	In either the unsolicited or response case, the receiver is 
*	now waiting for a message back from the receiver and is not 
*	reading the keyboard.
*	If keystroke capture is on,  the receiver will have placed a
*	timing message in the file.  The sender only needs to account for
*	timing starting now.
**************************************************************************/
#include "keystroke.h"
fsend_procack_array( ackcount, ackchars )	/* process array of acks */
	int		ackcount;
	unsigned int	ackchars[];
{
	int	i;
	long	time();

	if ( ackcount < 1 )
		return;
	if ( Keystroke_capture_fd )
		Keystroke_capture_time = time( (long *) 0 );
	for ( i = 0; i < ackcount; i++ )
		fsend_procack( ackchars[i] );
}
/**************************************************************************
* fsend_procack
*	Process a single "ack" message "c" - see fsend_procack_array.
* Input:
*	c	A receiver message from "talk.h".
**************************************************************************/
#include "controlopt.h"
fsend_procack( c )
	unsigned int c;
{
	if ( (c & 0xFF00) == FTPROC_SCREEN_SAVER )
	{
		/**********************************************************
		* Receiver requesting: Any output from windows since last 
		* request?  Used to prevent screen saver coming on when
		* no keyboard activity but output activity.
		* This request does not enter window mode.
		**********************************************************/
		report_screen_activity();
		fct_window_mode_ans_curwin();
		return;
	}
	/******************************************************************
	* Put the terminal in the mode specified for window command mode.
	******************************************************************/
	graph_screen_off_window_mode();
	modes_off_window_mode();
	if ( (c & 0xFF00) == FTPROC_WINDOW_KEY )
	{					/* user pressed Control-W */
		/**********************************************************
		* User pressed hot-key.
		* Enter Window Command Mode indicating hot-key pressed.
		**********************************************************/
		ftproc_window_mode( (int) (c & 0xFF) );
	}
	else if ( (c & 0xFF00) == FTPROC_NO_PROMPT_WINDOW_KEY )
	{
		/**********************************************************
		* User pressed no prompt hot-key.
		* Enter Window Command Mode indicating hot-key pressed.
		**********************************************************/
		turn_prompting_off();
		ftproc_window_mode( (int) (c & 0xFF) );
		turn_prompting_on();
	}
	else if ( (c & 0xFF00) == FTPROC_WINDOW_WINDOW )
	{					/* user pressed Break */
		/**********************************************************
		* User pressed break and break is being sent to facetterm.
		* If Windows-window is set then the program on that window
		* is notified, otherwise this is treated as pressing the
		* hot-key.
		**********************************************************/
		if ( ( Windows_window >= 0 ) && ( Windows_window < Wins ) )
			ftproc_window_mode_notify( Windows_window );
		else
			ftproc_window_mode( -1 );
	}
	else if ( (c & 0xFF00) == FTPROC_WINDOW_PACKET )
	{					/* receiver response to sender 
						   request for control */
		/**********************************************************
		* Sender signaled receiver that control of the keyboard
		* was needed.  This is receivers response - it now waits
		* for message from sender.
		**********************************************************/
		if ( key_read_pkt_start() == 0 )
		{
			/**************************************************
			* We were waiting to process a fct_command ioctl.
			**************************************************/
			ftproc_window_mode( -1 );
			key_read_pkt_end();
		}
		else if (  ( Switch_window >= 0 ) 
			|| ( Switch_window == IDLE_WINDOW_IS_NEXT_ACTIVE ) )
		{
			/**************************************************
			* We were waiting to switch windows.
			**************************************************/
			ftproc_window_mode_select( Switch_window );
			Switch_window = -1;
		}
#ifdef SOFTPC
		else if ( chk_terminal_mode_switch() )
		{
			/**************************************************
			* We were waiting to switch the terminal in or out
			* of scan code mode for SOFTPC.
			**************************************************/
		}
#endif
		else if ( chk_transparent_print_notify() )
		{
			/**************************************************
			* We were waiting to to a transparent print where
			* the terminal responds with a status.
			**************************************************/
		}
		else if ( check_paste_pending() )
		{
			/**************************************************
			* We were waiting to paste characters into a 
			* window triggered by an ioctl from an application.
			**************************************************/
		}
		else if ( check_paste_to_winno_pending() )
		{
			/**************************************************
			* We were waiting to paste an "answer" triggered
			* by a "question" to the terminal by the application.
			*************************************************/
		}
	}
	else if ( (c & 0xFF00) == FTPROC_WINDOW_NOPLAY )
	{
		/**********************************************************
		* Receiver got a keystroke from the keyboard during a 
		* keystroke replay - this cancels replay.
		**********************************************************/
		keystroke_play_cancel();
		fct_window_mode_ans_curwin();
	}
	else if ( (c & 0xFF00) == FTPROC_WINDOW_READY )
	{
		/**********************************************************
		* Receiver signaling ready to start.
		**********************************************************/
		receiver_says_ready();
	}
	else
	{
		/**********************************************************
		* Bug - receiver sent unimplemented message.
		**********************************************************/
		term_outgo();
		printf( "\r\nFSEND ERROR: invalid from frecv = %x\r\n",
				c );
		term_outgo();
	}
	/******************************************************************
	* Restore the modes that were set for window command mode.
	******************************************************************/
	modes_on_window_mode();
	graph_screen_on_window_mode();
}
/**************************************************************************
* do_push_popup_screen
*	Called in response to a FIOC_PUSH_POPUP_SCREEN ioctl.
*	This saves the current appearance of the screen so that a
*	subsequent "pop" can restore it to this state.
*	Returns the new level number in the string "level".
**************************************************************************/
#include "invisible.h"
#include "winactive.h"
#include "notify.h"
do_push_popup_screen( level )
	char	*level;
{
	int	result;

	result = push_popup_screen();
	sprintf( level, "%d", result );
	if ( result >= 0 )
		return( 0 );
	else
		return( ENOSPC );
}
/**************************************************************************
* do_pop_popup_screen
*	Called in response to a FIOC_POP_POPUP_SCREEN ioctl.
*	Restore the screen to the appearance at the time of the
*	corresponding "push".
*	Returns the new level number in the string "level".
**************************************************************************/
do_pop_popup_screen( level )
	char	*level;
{
	int	result;

	result = pop_popup_screen();
	sprintf( level, "%d", result );
	if ( result >= 0 )
		return( 0 );
	else
		return( ENOSPC );
}
char	*Text_window_is_active =
	"\r\n>>>Window %d is active.<<<\r\n";
/******************************************************************
* window_first_open
*	Window number "winno" has changed from closed to open.
*	Only occurs on first open.
******************************************************************/
window_first_open( winno )
	int	winno;
{
	char	buff[ 80 ];

	set_window_active( winno );
	if ( Opt_clear_window_on_open )
	{
		/**********************************************************
		* "clear_window_on_open" in .facet file.
		**********************************************************/
		make_out_clear_screen( buff, 80 );
		fct_decode_string( winno, buff );
	}
		    /* "\r\n>>>Window %d is active.<<<\r\n" */
	sprintf( buff, Text_window_is_active, winno + 1 );
	fct_decode_string( winno, buff );
}
char	*Text_window_is_idle =
	"\r\n>>>Window %d is idle.<<<\r\n";
/******************************************************************
* window_closed
*	Window number "winno" has changed from open to closed.
*	All applications have closed the window.
*	Only occurs on last close.
*	Shut down the window and reset.
******************************************************************/
window_closed( winno )
	int	winno;
{
	char	buff[ 80 ];

	clear_window_active( winno );
	Window_invisible[ winno ] = 0;
	Window_notify[ winno ] = 0;
	forget_exec_list( winno );
	if (  ( Windows_window_no_cancel == 0 ) 
	   && ( winno == Windows_window ) )
	{
		/**********************************************************
		* This is the "windows_window" and "windows_window_no_cancel"
		* in not specified in the .facet file.
		**********************************************************/
		Windows_window = -1;
	}
	if (  ( Idle_window_no_cancel == 0 ) 
	   && ( winno == Idle_window ) )
	{
		/**********************************************************
		* This is the "idle_window" and "idle_window_no_cancel"
		* in not specified in the .facet file.
		**********************************************************/
		Idle_window = NO_IDLE_WINDOW;
		Idle_window_paste_chars[ 0 ] = '\0';
	}
	fct_modes_normal( winno );
	if ( Opt_clear_window_on_close )
	{
		make_out_clear_screen( buff, 80 );
		fct_decode_string( winno, buff );
	}
	else
	{
		if ( make_out_nomagic( buff, 80 ) == 0 )
			fct_decode_string( winno, buff );
	}
		    /* "\r\n>>>Window %d is idle.<<<\r\n" */
	sprintf( buff, Text_window_is_idle, winno + 1 );
	fct_decode_string( winno, buff );
}
/**************************************************************************
* window_idle_actions
*	Check for options that have been set about actions to be taken
*	when windows a window goes idle.
**************************************************************************/
window_idle_actions( winno )
	int	winno;
{
	int	i;

	/******************************************************************
	* Check for all windows closed and "quit_on_all_windows_idle".
	******************************************************************/
	if ( Opt_quit_on_all_windows_idle )
	{
		for ( i = 0; i < Wins; i++ )
		{
			if( Window_active[ i ] )
				break;
		}
		if ( i >= Wins )
		{
			term_outgo();
			fsend_kill( "", 0 );
			/* does not return */
		}
	}
	/******************************************************************
	* Check for switch windows when current windows goes idle.
	******************************************************************/
	if (  ( ( Idle_window >=0 ) && ( Idle_window < Wins ) )
	   || ( Idle_window == IDLE_WINDOW_IS_NEXT_ACTIVE     ) )
	{
		if (  ( Idle_window != IDLE_WINDOW_IS_NEXT_ACTIVE )
		   && ( Idle_window_paste_chars[ 0 ] != '\0' ) )
		{
			if ( winno_is_curwinno( winno ) )
			{
				paste_receiver_not_waiting(
					Idle_window_paste_chars, Idle_window );
			}
		}
		else
			switch_if_current( winno, Idle_window );
	}
}
/**************************************************************************
* get_curwin_string
*	Translate the current window number ( external 1-10 ) into a
*	string and return in "s".
**************************************************************************/
get_curwin_string( s )
	char	*s;
{
	sprintf( s, "%d", Curwin->number + 1 );
}
/**************************************************************************
* process_window_mode_commands
*	An FIOC_WINDOW_MODE ioctl has been received on window_number "winno"
*	requesting that the string "s" be executed as if it was typed on
*	the keyboard following a hot-key.
*	The string is executed when the receiver response to
*	"send_packet_signal" causes a call to "fsend_procack_array".
**************************************************************************/
process_window_mode_commands( s, winno )
	char	*s;
	int	winno;
{
	key_read_pkt_setup( s, winno );
	send_packet_signal();
	while( key_read_pkt_pending() )
		fsend_get_acks_only();
}
/* ========================================================================= */
/**************************************************************************
* setcrt
*	Setup ioctls for terminal line, read terminal description file,
*	get user acknowlegement, initialize error recording.
**************************************************************************/
setcrt()
{

	if ( term_init() != 0 )
		return( -1 );
	fsend_init_errors();
	return( 0 );
}
/**************************************************************************
* fsend_init
*	Sender initialization after fork. 
*		Close unneeded ends of pipes to receiver.
*		Allocate and initialize window buffers.
*		Set terminals function keys if necessary.
*		Set terminals starting modes if necessary.
**************************************************************************/
fsend_init()
{
	fsend_init_communications();
        fct_init_windows();
	init_default_function_key();
	modes_default_start();
}

/**************************************************************************
* fsend_window
*	Process the "count" characters in the string "ptr" onto the
*	window number "window".
**************************************************************************/
#include "capture.h"
#include "monitor.h"
#include "transpar.h"
#include "printer.h"
fsend_window( window, ptr, count )
	int	window;
	char	*ptr;
	int	count;
{
	int	i;
	char	*p;
	FILE	*file;

	/******************************************************************
	* If capture is on record the characters.
	******************************************************************/
	if ( Capture_active )
	{
		if ( ( file = Capture_file[ window ] ) != NULL )
		{
			p = ptr;
			for ( i = 0; i < count; i++ )
			{
				putc( *p, file );
				p++;
			}
			if ( Opt_capture_no_buffer )
				fflush( file );
		}
	}
	/******************************************************************
	* If monitor mode, transparent print, or printer mode - handle
	* the characters specially.  Otherwise, handle normally.
	******************************************************************/
	if ( Window_monitor[ window ] )
		fct_monitor( window, ptr, count );
	else if ( Window_transparent[ window ] )
		fct_transparent( window, ptr, count );
	else if ( Window_printer[ window ] )
		fct_printer( window, ptr, count );
	else
		fct_decode( window, (unsigned char *) ptr, count );
}

char	*Text_user_number = 
	"FacetTerm user # %d of %d  -  ";
char	*Text_too_many_users = 
	"Sorry - %d FacetTerm users already active\n";

/**************************************************************************
* check_users				(registration systems only)
*	If the number of users is limited on this system, this is called.
*	For the number of authorized users, see if a slot is available.
*	If the authorized number of users is already running, output an
*	error message, wait for return to be pressed, and return -1.
*	Otherwise output the user number message and return 0.
*	The input "talk" is set to 0 to suppress the output of the
*	user number found.
**************************************************************************/
