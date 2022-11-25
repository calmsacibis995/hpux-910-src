/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: talkkey.c,v 70.1 92/03/09 15:47:21 ssa Exp $ */
/**************************************************************************
* talkkey.c
*	receiver keyboard handling
**************************************************************************/
#include <sys/types.h>
#include "talk.h"
#include <signal.h>
#include <errno.h>
#include "facetkey.h"

int	F_tty_is_CS8 = 0;
int	Opt_use_PARMRK_for_break = 0;

extern int errno;
#define	CONTROL ( - 0x40 )
int	Window_char = CONTROL + 'W';
int	No_prompt_window_char = -1;
int	Inwin = 0;				/* current input window */
int	Sender_signal_caught = 0;
int	Screen_saver_alarm_caught = 0;
int	Screen_saver_time = 0;

#if defined( VPIX ) || defined( SOFTPC )
int	Scan_code_mode = 0;
int	Eat_scan_code_up = 0;
#endif

int	Opt_send_break_to_window = 0;

int	K_screen_saver_first_done = 0;
int	K_screen_activity = 0;

/**************************************************************************
* frecv_recv
*	receiver main loop
**************************************************************************/
frecv_recv()
{
	int	c;
	int	ctmp;

	/******************************************************************
	* Set signal handlers.
	* Tell sender ready.
	* Receive hotkey.
	* Determine if line is 8 bit and/or PARMRK.
	******************************************************************/
	catch_packet_signal();
	catch_screen_saver_alarm();
	tell_sender_ready();
	wait_for_sender( -1 );
	determine_tty_characteristics();
	/******************************************************************
	* process all incoming characters
	******************************************************************/
	while (1) 
	{
		/**********************************************************
		* Sender signals when it needs the tty for io,
		* has a window mode ioctl to execute, etc.
		* Tell it ready and wait for instructions.
		**********************************************************/
		if ( Sender_signal_caught )
		{		
#if defined( VPIX ) || defined( SOFTPC )
			if ( Scan_code_mode )
				vpix_keys_up();
#endif
			Sender_signal_caught = 0;
			send_keys_to_window();
			fct_window_packet_mode();
			wait_for_sender( -1 );
			continue;
		}
		/**********************************************************
		* Screen saver alarm is at half the desired time.
		* The first time ask for screen activity which clears
		* the activity flag.
		* The second time ask for screen activity which tells if
		* the sender has been idle also.
		* If both idle, pretend ^Wxs\r was typed.
		**********************************************************/
		if ( Screen_saver_alarm_caught )
		{
#if defined( VPIX ) || defined( SOFTPC )
			if ( Scan_code_mode )
				vpix_keys_up();
#endif
			send_keys_to_window();
			Screen_saver_alarm_caught = 0;
			ask_for_screen_activity();
			wait_for_sender( -1 );
			if ( K_screen_saver_first_done )
			{
				if ( K_screen_activity == 0 )
				{
#if defined( VPIX ) || defined( SOFTPC )
					if ( Scan_code_mode )
						put_keys_in_pipe(
							"\055\037\034", 3 );
					else
#endif
						put_keys_in_pipe( "xs\r", 3 );
					fct_window_key_mode( Window_char );
					wait_for_sender( Window_char );
					K_screen_saver_first_done = 0;
				}
			}
			else
				K_screen_saver_first_done = 1;
			continue;
		}
		/**********************************************************
		* Get a key.
		**********************************************************/
		c = read_keyboard();
		if ( c < 0 )
			continue;		/* interrupted by signal */
		K_screen_saver_first_done = 0;
		if ( F_tty_is_CS8 )
			ctmp = c;
		else
			ctmp = c & 0x007F;
#if defined( VPIX ) || defined( SOFTPC )
		/**********************************************************
		* When we switch windows or modes in scan code mode,
		* the key up scan codes will just confuse the new
		* window.
		* Ignore them.
		**********************************************************/
		if ( Eat_scan_code_up )
		{
			if ( ( c >= 0x80 ) && ( c < 0xE0 ) )
			{
				Eat_scan_code_up--;
				continue;
			}
			else
				Eat_scan_code_up = 0;
		}
		if ( Scan_code_mode )
		{
			int	status;

			/**************************************************
			* Its tough to watch for ^W in scan code mode,
			* but someone has to doit.
			**************************************************/
			status = vpix_key_in( c );
			if ( status == 0 )
				queue_key_to_window( c );
			else if ( status > 0 )
			{
				vpix_keys_up();
				send_keys_to_window();
				fct_window_key_mode( Window_char );
				wait_for_sender( Window_char );
			}
			else
			{
				continue;	/* mapped */
			}
		}
		else
#endif
		/* This is an elseif when VPIX is defined.
		   Do not insert code here!!!
		*/
		if ( ctmp == Window_char )
		{
			/**************************************************
			* Hotkey - tell sender and wait.
			**************************************************/
			send_keys_to_window();
			fct_window_key_mode( Window_char );
			wait_for_sender( Window_char );
		}
		else if ( c == No_prompt_window_char )
		{
			/**************************************************
			* Hotkey (no prompt) - tell sender and wait.
			**************************************************/
			send_keys_to_window();
			fct_no_prompt_window_key_mode( No_prompt_window_char );
			wait_for_sender( No_prompt_window_char );
		}
		else if ( ( ctmp == 0 ) && ( Opt_use_PARMRK_for_break == 0 ) )
		{
			/**************************************************
			* Break - send to window or send to sender and wait.
			**************************************************/
			if ( Opt_send_break_to_window )
			{
				send_keys_to_window();
				frecv_break( Inwin );
			}
			else
			{
				send_keys_to_window();
				fct_window_window_mode();
				wait_for_sender( -1 );
			}
		}
		else if ( ( c == 0xFF ) && ( Opt_use_PARMRK_for_break ) )
		{
			/**************************************************
			* Start of break -
			* if it turns out not to be send keys to window.
			**************************************************/
			c = read_keyboard();
			if ( c < 0 )
				continue;	/* interrupted by signal */
			if ( c == 0xFF )
			{
				queue_key_to_window( c );
				continue;
			}
			else if ( c != 0 )
			{
				queue_key_to_window( 0xFF );
				queue_key_to_window( c );
				continue;
			}
			c = read_keyboard();
			if ( c < 0 )
				continue;	/* interrupted by signal */
			if ( c != 0 )
			{
				queue_key_to_window( 0xFF );
				queue_key_to_window( 0 );
				queue_key_to_window( c );
				continue;
			}
			/**************************************************
			* Break - send to window or send to sender and wait.
			**************************************************/
			if ( Opt_send_break_to_window )
			{
				send_keys_to_window();
				frecv_break( Inwin );
			}
			else
			{
				send_keys_to_window();
				fct_window_window_mode();
				wait_for_sender( -1 );
			}
		}
		else
		{
			/**************************************************
			* Plain old everyday key.
			**************************************************/
			queue_key_to_window( c );
		}
	}
}
#include "mapped.h"

#include "keystroke.h"
int	Keystroke_capture_fd = 0;
long	Keystroke_capture_time = 0;
int	Keystroke_play_fd = 0;

/**************************************************************************
* read_keyboard
*	Get the next character to process.
**************************************************************************/
read_keyboard()
{
	int	count;
	int	c;
	int	cc;
	char	buff[ 10 ];
	int	i;
	char	input_string[ 2 ];
	char	*output_string;
	int	to_winno;
	int	mapcount;

	while( 1 )
	{
		/**********************************************************
		* A pipe is used by the receiver to buffer
		* keys that were read ahead and not processed yet.
		* Sender reads from here also.
		**********************************************************/
		cc = chk_keys_in_pipe();
		if ( cc >= 0 )
			return( cc );
		send_keys_to_window();
		/**********************************************************
		* On HP terminals, the sender may read an ordinary key
		* while it is trying to eat the status characters that
		* result from a print request.
		**********************************************************/
		count = chk_keys_from_send( buff, 10 );
		if ( count <= 0 )
		{
			/**************************************************
			* No keys yet - try keystroke replay - could cause stop.
			**************************************************/
			if ( Keystroke_play_fd )
			{
				count = keystroke_play_read_receive( buff, 10 );
			}
			if ( Keystroke_play_fd == 0 )
			{
				/******************************************
				* Keystoke replay not active.
				* Set alarm if screen saver.
				******************************************/
				if ( Screen_saver_time )
					alarm( Screen_saver_time / 2 );
				/******************************************
				* Check for sender tapping us on the shoulder.
				******************************************/
				if ( Sender_signal_caught )
				{
					if ( Screen_saver_time )
						alarm( 0 );
					return( -1 );	/* interrupted */
				}
				/******************************************
				* Read the real keyboard.
				******************************************/
				for ( i = 0; i < 10; i++ )
				{
					count = read( 0, buff, 10 );
					if ( count != 0 )
						break;
				}
				if ( Screen_saver_time )
					alarm( 0 );
			}
			if ( count < 1 )
			{
				/******************************************
				* Sender signalling or somethings wrong.
				*****************************************/
				if ( count == -1 )
				{
					if ( errno == EINTR ) 
						/* interrupted by signal */
						return( -1 );
					else
						frecv_kill( 
						    "Receiver read error", -1 );
						/* does not return */
				}
				else
					frecv_kill( "Receiver read error= %d", 
						    count );
					/* does not return */
			}
		}
		/**********************************************************
		* Have a key - record if keystroke capture.
		**********************************************************/
		c = ( ( int )( buff[ 0 ] ) ) & 0x00FF;
		if ( Keystroke_capture_fd )
			keystroke_capture_key( buff, count );
		/**********************************************************
		**********************************************************/
#if defined( VPIX ) || defined( SOFTPC )
		if ( Scan_code_mode )
		{
			if ( count > 1 )
				put_keys_in_pipe( &buff[ 1 ], count - 1 );
			return( c );
		}
#endif
		/**********************************************************
		* Just return it if only one unmapped char.
		**********************************************************/
		if ( count == 1 && Mapped_char[ c ] == 0 )
			return( c );
		/**********************************************************
		* Put first char ( after possible mapping )in c for 
		* returning below - rest or chars ( after possible mapping )
		* into the pipe.
		**********************************************************/
		c = -1;
		for( i = 0; i < count; i++ )
		{
			cc = ( ( int )( buff[ i ] ) ) & 0x00FF;
			if ( Mapped_char[ cc ] == 0 )
			{
				if ( c == -1 )
					c = cc;
				else
					put_keys_in_pipe( &buff[ i ], 1 );
				continue;
			}
			input_string[ 0 ] = cc;
			input_string[ 1 ] = '\0';
			if ( search_mapped( input_string, Inwin, 
				&output_string, &to_winno ) != 0 )
			{
				/******************************************
				* -1 = no match    1 = partial match (not imp)
				******************************************/
				if ( c == -1 )
					c = cc;
				else
					put_keys_in_pipe( &buff[ i ], 1 );
			}
			else
			{
				/******************************************
				* Match - send to window or expand in place
				* for current window.
				******************************************/
				mapcount = strlen( output_string );
				if ( mapcount <= 0 )
					continue;
				if ( to_winno >= 0 )
				{
					frecv_winputs( to_winno, 
						output_string, mapcount );
				}
				else
				{
					if ( c == -1 )
					{
						c = output_string[ 0 ];
						c &= 0x00FF;
						output_string++;
						mapcount--;
					}
					if ( mapcount > 0 )
					{
						put_keys_in_pipe( 
							output_string, 
							mapcount );
					}
				}
			}
		}
		if ( c != -1 )
			return( c );
	}
}
#define MAX_WINDOW_QUEUE 10
char	Window_queue[ MAX_WINDOW_QUEUE ];
int	Window_queue_count = 0;
/**************************************************************************
* queue_key_to_window
*	The character "c" should go to the current input window.
*	Buffer to send all at once to save cpu.
**************************************************************************/
queue_key_to_window( c )
	int	c;
{
	if ( Window_queue_count >= MAX_WINDOW_QUEUE )
		send_keys_to_window();
	Window_queue[ Window_queue_count ] = c;
	Window_queue_count++;
}
/**************************************************************************
* send_keys_to_window
*	Send queued characters to window - done everytime we could get
*	stopped.
**************************************************************************/
send_keys_to_window()
{
	if ( Window_queue_count )
	{
		frecv_winputs( Inwin, Window_queue, Window_queue_count );
		Window_queue_count = 0;
	}
}
/**************************************************************************
* output_key_to_window
*	Send the key "c" to a window.
*	Send already queued so not out of order.
**************************************************************************/
output_key_to_window( c )
	int	c;
{
	send_keys_to_window();
	frecv_winputc( Inwin, c );
}
/**************************************************************************
* caught_packet_signal
*	Sender signal catcher.
*	Reset trap and set flag.
*	No race condition since sender waits for ack before doing anything
*	else.
**************************************************************************/
caught_packet_signal( ignore )
	int	ignore;
{
	catch_packet_signal();
	Sender_signal_caught = 1;
}
/**************************************************************************
* catch_packet_signal
*	Set sender signal catcher.
**************************************************************************/
catch_packet_signal()
{
	signal( SIGUSR1, caught_packet_signal );
}
/**************************************************************************
* caught_screen_saver_alarm
*	Screen saver alarm handler.
**************************************************************************/
caught_screen_saver_alarm( ignore )
	int	ignore;
{
	catch_screen_saver_alarm();
	Screen_saver_alarm_caught = 1;
}
/**************************************************************************
* catch_screen_saver_alarm
*	Setup screen saver alarm handler.
**************************************************************************/
catch_screen_saver_alarm()
{
	signal( SIGALRM, caught_screen_saver_alarm );
}
/**************************************************************************
* tell_sender_ready
*	Sender waits for this message on startup to make sure to give
*	receiver time to set up signal catching before signal is sent.
**************************************************************************/
tell_sender_ready()	
{
	frecv_toacks( FTPROC_WINDOW_READY );
}
/*****************************************************************************
*	0-9	select window
*	A-J	select window scan code mode
*	p	paste		( window, chars, null )
*	P	paste char	( window, char )
*	w	set hotkey
*	n	set no-prompt hotkey
*	m	map
*	k	keystroke capture
*	K	keystroke play
*	W	send hotkey to window
*	b	set Opt_send_break_to_window
*	s	screen saver seconds
*	a	screen activity - response to FTPROC_SCREEN_SAVER
*	f	new file descriptor - SEQUENT only
*	F	new file descriptor - SEQUENT only
*****************************************************************************/
/**************************************************************************
* wait_for_sender
*	Sender will reply with commands and finally with the current window
*	number.  
*	Interpret commands until current window number and the resume.
**************************************************************************/
wait_for_sender( c )
	int	c;
{
	int	result;
	long	time();

#if defined( VPIX ) || defined( SOFTPC )
	if ( Scan_code_mode )
		Eat_scan_code_up = 2;
#endif
	while( 1 )
	{
		result = get_window_mode_ans();
		if ( result == 'W' )
		{
#if defined( VPIX ) || defined( SOFTPC )
			if ( Scan_code_mode )
			{
				output_key_to_window( 0x1D );
				output_key_to_window( 0x11 );
				output_key_to_window( 0x91 );
				output_key_to_window( 0x9D );
			}
			else
#endif
			if ( c >= 0 )
			{
				output_key_to_window( c );
			}
		}
		else if ( ( result >= '0' ) && ( result <= '9' ) )
		{
			Inwin = result - '0';
#if defined( VPIX ) || defined( SOFTPC )
			Scan_code_mode = 0;
#endif
		}
#if defined( VPIX ) || defined( SOFTPC )
		else if ( ( result >= 'A' ) && ( result <= 'J' ) )
		{
			Inwin = result - 'A';
			Scan_code_mode = 1;
		}
#endif
		else if ( result == 'p' )
		{
						/* paste to alternate window
						** and wait
						*/
			int	old_inwin;
			int	old_scan_code_mode;

			old_inwin = Inwin;
#if defined( VPIX ) || defined( SOFTPC )
			old_scan_code_mode = Scan_code_mode;
#endif
			result = get_window_mode_ans_inwin();
			receive_paste();
			Inwin = old_inwin;
#if defined( VPIX ) || defined( SOFTPC )
			Scan_code_mode = old_scan_code_mode;
#endif
			continue;
		}
		else if ( result == 'P' )	/* Paste single char */
		{
						/* paste to alternate window
						** and wait
						*/
			int	old_inwin;
			int	old_scan_code_mode;

			old_inwin = Inwin;
#if defined( VPIX ) || defined( SOFTPC )
			old_scan_code_mode = Scan_code_mode;
#endif
			result = get_window_mode_ans_inwin();
			result = get_window_mode_ans();
			output_key_to_window( result );
			Inwin = old_inwin;
#if defined( VPIX ) || defined( SOFTPC )
			Scan_code_mode = old_scan_code_mode;
#endif
			continue;
		}
		else if ( result == 'w' )
		{
			result = get_window_mode_ans();
			Window_char = result;
			continue;
		}
		else if ( result == 'n' )
		{
			result = get_window_mode_ans();
			No_prompt_window_char = result;
			continue;
		}
		else if ( result == 'm' )
		{
						/* set up map and wait */
			receive_map();
			continue;
		}
		else if ( result == 'k' )
		{
						/* set up keystroke and wait */
			receive_keystroke_capture();
			continue;
		}
		else if ( result == 'K' )
		{
						/* set up keystroke and wait */
			receive_keystroke_play();
			continue;
		}
		else if ( result == 'b' )
		{
			Opt_send_break_to_window = 1;
			continue;
		}
		else if ( result == 's' )
		{
			receive_screen_saver_time();
			continue;
		}
		else if ( result == 't' )
		{
			result = get_window_mode_ans();
			save_keys_from_send( result );
			continue;
		}
		else if ( result == 'a' )
		{
			result = get_window_mode_ans();
			if ( result == '1' )
				K_screen_activity = 1;
			else
				K_screen_activity = 0;
			continue;
		}
		else
		{
			printf( "Invalid from sender %x\r\n", result );
			term_outgo();
		}
		if ( Keystroke_capture_fd )
			Keystroke_capture_time = time( (long *) 0 );
		return;
	}
}
/**************************************************************************
* get_window_mode_ans_inwin
*	Receive a sender communication specifying
*	the current input window and the scan code mode.
**************************************************************************/
get_window_mode_ans_inwin()
{
	int	result;

	result = get_window_mode_ans();
	if (  ( result >= '0' ) && ( result <= '9' ) )
	{
		Inwin = result - '0';
#if defined( VPIX ) || defined( SOFTPC )
		Scan_code_mode = 0;
#endif
		return( result );
	}
#if defined( VPIX ) || defined( SOFTPC )
	else if ( ( result >= 'A' ) && ( result <= 'J' ) )
	{
		Inwin = result - 'A';
		Scan_code_mode = 1;
		return( result );
	}
#endif
	else
	{
		printf( "Invalid window from sender %x\r\n", result );
		term_outgo();
		return( -1 );
	}
}
#if defined( VPIX ) || defined( SOFTPC )
unsigned char	Vpix_scan_codes[ 255][ 5 ] =
{
	0,	0,	0,	0,	0,	/* ^@ */
	0x1D,	0x1E,	0x9E,	0x9D,	0,	/* ^A */
	0x1D,	0x30,	0xB0,	0x9D,	0,	/* ^B */
	0x1D,	0x2E,	0xAE,	0x9D,	0,	/* ^C */
	0x1D,	0x20,	0xA0,	0x9D,	0,	/* ^D */
	0x1D,	0x12,	0x92,	0x9D,	0,	/* ^E */
	0x1D,	0x21,	0xA1,	0x9D,	0,	/* ^F */
	0x1D,	0x22,	0xA2,	0x9D,	0,	/* ^G */
	0x1D,	0x23,	0xA3,	0x9D,	0,	/* ^H */
	0x1D,	0x17,	0x97,	0x9D,	0,	/* ^I */
	0x1D,	0x24,	0xA4,	0x9D,	0,	/* ^J */
	0x1D,	0x25,	0xA5,	0x9D,	0,	/* ^K */
	0x1D,	0x26,	0xA6,	0x9D,	0,	/* ^L */
	0x1C,	0x9C,	0x9E,	0,	0,	/* CR */
	0x1D,	0x31,	0xB1,	0x9D,	0,	/* ^N */
	0x1D,	0x18,	0x98,	0x9D,	0,	/* ^O */
	0x1D,	0x19,	0x99,	0x9D,	0,	/* ^P */
	0x1D,	0x10,	0x90,	0x9D,	0,	/* ^Q */
	0x1D,	0x13,	0x93,	0x9D,	0,	/* ^R */
	0x1D,	0x1F,	0x9F,	0x9D,	0,	/* ^S */
	0x1D,	0x14,	0x94,	0x9D,	0,	/* ^T */
	0x1D,	0x16,	0x96,	0x9D,	0,	/* ^U */
	0x1D,	0x2F,	0xAF,	0x9D,	0,	/* ^V */
	0x1D,	0x11,	0x91,	0x9D,	0,	/* ^W */
	0x1D,	0x2D,	0xAD,	0x9D,	0,	/* ^X */
	0x1D,	0x15,	0x95,	0x9D,	0,	/* ^Y */
	0x1D,	0x2C,	0xAC,	0x9D,	0,	/* ^Z */
	0,	0,	0,	0,	0,	/* ^[ */
	0,	0,	0,	0,	0,	/* ^\ */
	0,	0,	0,	0,	0,	/* ^] */
	0,	0,	0,	0,	0,	/* ^^ */
	0,	0,	0,	0,	0,	/* ^_ */
	0x39,	0xB9,	0,	0,	0,	/* SPACE */
	0x2A,	0x02,	0x82,	0xAA,	0,	/* ! */
	0x2A,	0x28,	0xA8,	0xAA,	0,	/* " */
	0x2A,	0x04,	0x83,	0xAA,	0,	/* # */
	0x2A,	0x05,	0x85,	0xAA,	0,	/* $ */
	0x2A,	0x06,	0x86,	0xAA,	0,	/* % */
	0x2A,	0x08,	0x88,	0xAA,	0,	/* & */
	0x28,	0xA8,	0,	0,	0,	/* ' */
	0x2A,	0x0A,	0x8A,	0xAA,	0,	/* ( */
	0x2A,	0x0B,	0x8B,	0xAA,	0,	/* ) */
	0x2A,	0x09,	0x89,	0xAA,	0,	/* * */
	0x2A,	0x0D,	0x8D,	0xAA,	0,	/* + */
	0x33,	0xB3,	0,	0,	0,	/* , */
	0x0C,	0x8C,	0,	0,	0,	/* - */
	0x34,	0xB4,	0,	0,	0,	/* . */
	0x35,	0xB5,	0,	0,	0,	/* / */
	0x0B,	0x8B,	0,	0,	0,	/* 0 */
	0x02,	0x82,	0,	0,	0,	/* 1 */
	0x03,	0x83,	0,	0,	0,	/* 2 */
	0x04,	0x84,	0,	0,	0,	/* 3 */
	0x05,	0x85,	0,	0,	0,	/* 4 */
	0x06,	0x86,	0,	0,	0,	/* 5 */
	0x07,	0x87,	0,	0,	0,	/* 6 */
	0x08,	0x88,	0,	0,	0,	/* 7 */
	0x09,	0x89,	0,	0,	0,	/* 8 */
	0x0A,	0x8A,	0,	0,	0,	/* 9 */
	0x2A,	0x27,	0xA7,	0xAA,	0,	/* : */
	0x27,	0xA7,	0,	0,	0,	/* ; */
	0x2A,	0x33,	0xB3,	0xAA,	0,	/* < */
	0x0D,	0x8D,	0,	0,	0,	/* = */
	0x2A,	0x34,	0xB4,	0xAA,	0,	/* > */
	0x2A,	0x35,	0xB5,	0xAA,	0,	/* ? */
	0x2A,	0x03,	0x83,	0xAA,	0,	/* @ */
	0x2A,	0x1E,	0x9E,	0xAA,	0,	/* A */
	0x2A,	0x30,	0xB0,	0xAA,	0,	/* B */
	0x2A,	0x2E,	0xAE,	0xAA,	0,	/* C */
	0x2A,	0x20,	0xA0,	0xAA,	0,	/* D */
	0x2A,	0x12,	0x92,	0xAA,	0,	/* E */
	0x2A,	0x21,	0xA1,	0xAA,	0,	/* F */
	0x2A,	0x22,	0xA2,	0xAA,	0,	/* G */
	0x2A,	0x23,	0xA3,	0xAA,	0,	/* H */
	0x2A,	0x17,	0x97,	0xAA,	0,	/* I */
	0x2A,	0x24,	0xA4,	0xAA,	0,	/* J */
	0x2A,	0x25,	0xA5,	0xAA,	0,	/* K */
	0x2A,	0x26,	0xA6,	0xAA,	0,	/* L */
	0x2A,	0x32,	0xB2,	0xAA,	0,	/* M */
	0x2A,	0x31,	0xB1,	0xAA,	0,	/* N */
	0x2A,	0x18,	0x98,	0xAA,	0,	/* O */
	0x2A,	0x19,	0x99,	0xAA,	0,	/* P */
	0x2A,	0x10,	0x90,	0xAA,	0,	/* Q */
	0x2A,	0x13,	0x93,	0xAA,	0,	/* R */
	0x2A,	0x1F,	0x9F,	0xAA,	0,	/* S */
	0x2A,	0x14,	0x94,	0xAA,	0,	/* T */
	0x2A,	0x16,	0x96,	0xAA,	0,	/* U */
	0x2A,	0x2F,	0xAF,	0xAA,	0,	/* V */
	0x2A,	0x11,	0x91,	0xAA,	0,	/* W */
	0x2A,	0x2D,	0xAD,	0xAA,	0,	/* X */
	0x2A,	0x15,	0x95,	0xAA,	0,	/* Y */
	0x2A,	0x2C,	0xAC,	0xAA,	0,	/* Z */
	0x1A,	0x9A,	0,	0,	0,	/* [ */
	0x2B,	0xAB,	0,	0,	0,	/* \ */
	0x1B,	0x9B,	0,	0,	0,	/* ] */
	0x2A,	0x07,	0x87,	0xAA,	0,	/* ^ */
	0x2A,	0x0C,	0x8C,	0xAA,	0,	/* _ */
	0x29,	0xA9,	0,	0,	0,	/* ` */
	0x1E,	0x9E,	0,	0,	0,	/* a */
	0x30,	0xB0,	0,	0,	0,	/* b */
	0x2E,	0xAE,	0,	0,	0,	/* c */
	0x20,	0xA0,	0,	0,	0,	/* d */
	0x12,	0x92,	0,	0,	0,	/* e */
	0x21,	0xA1,	0,	0,	0,	/* f */
	0x22,	0xA2,	0,	0,	0,	/* g */
	0x23,	0xA3,	0,	0,	0,	/* h */
	0x17,	0x97,	0,	0,	0,	/* i */
	0x24,	0xA4,	0,	0,	0,	/* j */
	0x25,	0xA5,	0,	0,	0,	/* k */
	0x26,	0xA6,	0,	0,	0,	/* l */
	0x32,	0xB2,	0,	0,	0,	/* m */
	0x31,	0xB1,	0,	0,	0,	/* n */
	0x18,	0x98,	0,	0,	0,	/* o */
	0x19,	0x99,	0,	0,	0,	/* p */
	0x10,	0x90,	0,	0,	0,	/* q */
	0x13,	0x93,	0,	0,	0,	/* r */
	0x1F,	0x9F,	0,	0,	0,	/* s */
	0x14,	0x94,	0,	0,	0,	/* t */
	0x16,	0x96,	0,	0,	0,	/* u */
	0x2F,	0xAF,	0,	0,	0,	/* v */
	0x11,	0x91,	0,	0,	0,	/* w */
	0x2D,	0xAD,	0,	0,	0,	/* x */
	0x15,	0x95,	0,	0,	0,	/* y */
	0x2C,	0xAC,	0,	0,	0,	/* z */
	0x2A,	0x1A,	0x9A,	0xAA,	0,	/* { */
	0x2A,	0x2B,	0xAB,	0xAA,	0,	/* | */
	0x2A,	0x1B,	0x9B,	0xAA,	0,	/* } */
	0x2A,	0x29,	0xA9,	0xAA,	0,	/* ~ */
};
#endif
/**************************************************************************
* receive_paste
*	Sender is wanting a paste to a window.
*	Paste is null terminated.
*	Sleep 1 second on a 0xFF or 80 chars ( 25 in scan code mode ).
*	Translate to scan codes if scan code mode using the above table.
**************************************************************************/
receive_paste()
{
	int	result;
	int	max;
	int	count;
#if defined( VPIX ) || defined( SOFTPC )
	unsigned char	*ptr;
	int	j;
#endif
	max = 80;
#if defined( VPIX ) || defined( SOFTPC )
	if ( Scan_code_mode )
		max = 25;
#endif

	count = 0;
	while ( 1 )
	{
		result = get_window_mode_ans();
		if ( result <= 0 )
			return;
		if ( result == 0xFF )
		{
			sleep( 1 );
			count = 0;
			continue;
		}
#if defined( VPIX ) || defined( SOFTPC )
		if ( Scan_code_mode )
		{
			ptr = &Vpix_scan_codes[ (int) result & 0xFF ]
					      [ 0 ];
			for ( j = 0; j < 5; j++ )
			{
				if ( *ptr == 0 )
					break;
				output_key_to_window( (int) *ptr );
				count++;
				ptr++;
			}
		}
		else
		{
#endif
			output_key_to_window( result );
			count++;
#if defined( VPIX ) || defined( SOFTPC )
		}
#endif
		if ( count >= max )
		{
			sleep( 1 );
			count = 0;
		}
	}
}
/**************************************************************************
* fct_window_window_mode
*	User pressed break which is being sent to facetterm.
**************************************************************************/
fct_window_window_mode()	
{
	if ( Keystroke_capture_fd )
		keystroke_capture_time();
	frecv_toacks( FTPROC_WINDOW_WINDOW );
}
/**************************************************************************
* fct_window_key_mode
*	Hotkey "c" pressed.
**************************************************************************/
fct_window_key_mode( c )	
	int	c;
{
	if ( Keystroke_capture_fd )
		keystroke_capture_time();
	frecv_toacks( FTPROC_WINDOW_KEY | c );
}
/**************************************************************************
* fct_no_prompt_window_key_mode
*	No prompt Hotkey "c" pressed.
**************************************************************************/
fct_no_prompt_window_key_mode( c )	
	int	c;
{
	if ( Keystroke_capture_fd )
		keystroke_capture_time();
	frecv_toacks( FTPROC_NO_PROMPT_WINDOW_KEY | c );
}
/**************************************************************************
* fct_window_packet_mode
*	Acknowledge a signal from the sender that it wants the keyboard.
**************************************************************************/
fct_window_packet_mode()	
{
	if ( Keystroke_capture_fd )
		keystroke_capture_time();
	frecv_toacks( FTPROC_WINDOW_PACKET );
}
/**************************************************************************
* fct_window_noplay
*	Tell the sender that the keystroke replay is done.
**************************************************************************/
fct_window_noplay()
{
	if ( Keystroke_capture_fd )
		keystroke_capture_time();
	frecv_toacks( FTPROC_WINDOW_NOPLAY );
}
/**************************************************************************
* ask_for_screen_activity
*	Ask the sender if the windows have output characters since the
*	last time we asked.
**************************************************************************/
ask_for_screen_activity()
{
	if ( Keystroke_capture_fd )
		keystroke_capture_time();
	frecv_toacks( FTPROC_SCREEN_SAVER );
}
/**************************************************************************
* get_window_mode_ans
*	Get a character from the sender.
**************************************************************************/
get_window_mode_ans()	
{
	int	result;
	char	r;
	int	status;

	while ( (status = read( Pipe_to_recv_read, &r, 1 ))  == -1 )
	{
		if ( errno != EINTR )
		{
			frecv_kill( "Can't hear sender", -1 );
			/* does not return */
		}
	}
	if ( status != 1 )
	{
		frecv_kill( "Sender unexpectedly terminated %d", status );
		/* does not return */
	}
	result = r;
	result &= 0xFF;
	return( result );
}
#if defined( VPIX ) || defined( SOFTPC )
unsigned char Scan_to_char[ 256 ] =
{
	/* 0	 1	 2	 3	 4	 5	 6	 7	*/
	/* 8	 9	 A	 B	 C	 D	 E	 F	*/
/* 0 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 1 */	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
	'O',	'P',	0,	0,	0,	0,	'A',	'S',
/* 2 */	'D',	'F',	'G',	'H',	'J',	'K',	'L',	0,
	0,	0,	0,	0,	'Z',	'X',	'C',	'V',
/* 3 */	'B',	'N',	'M',	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 4 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 5 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 6 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 7 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0
};
#define VKEY_W			0x11
#define VKEY_CONTROL		0x1D
#define VKEY_CONTROL_RIGHT	( 128 + 0x1D )
#define VKEY_SHIFT		0x2A
#define VKEY_SHIFT_RIGHT	0x36
#define VKEY_ALT		0x38
#define VKEY_ALT_RIGHT		( 128 + 0x38 )
unsigned char	Vpix_keys[ 256 ] = { 0 };
int		Vpix_E0 = 0;
/**************************************************************************
* vpix_key_in
*	Try to figure out what the sender pressed by watching the stream
*	of scan codes.
*	Keep track of what keys are down.
*	If it is a hotkey return 1.
*	Otherwise if it is a mapped char then map it.
*	Return 0.
**************************************************************************/
vpix_key_in( c )
	int	c;
{
	if ( c == 0xE1 )
	{
	}
	else if ( c == 0xE0 )
	{
		Vpix_E0 = 1;
	}
	else if ( ( c & 0x80 ) == 0 )
	{
		if (  ( Vpix_keys[ VKEY_CONTROL ] == 1 )
		   && ( Vpix_E0 == 0 )
		   && ( Vpix_keys[ VKEY_SHIFT ] == 0 )
		   && ( Vpix_keys[ VKEY_SHIFT_RIGHT ] == 0 )
		   && ( Vpix_keys[ VKEY_CONTROL_RIGHT ] == 0 )
		   && ( Vpix_keys[ VKEY_ALT ] == 0 )
		   && ( Vpix_keys[ VKEY_ALT_RIGHT ] == 0 ) )
		{
			int	real;
			char	input_string[ 2 ];
			char	*output_string;
			int	to_winno;
			int	mapcount;

			real = Scan_to_char[ c ] - 0x40;
			if ( real > 0 )
			{
				if ( real == Window_char )
					return( 1 );
			}
			if ( Mapped_char[ real ] )
			{
				input_string[ 0 ] = real;
				input_string[ 1 ] = '\0';
				if ( search_mapped( input_string, Inwin, 
					&output_string, &to_winno ) == 0 )
				{
					mapcount = strlen( output_string );
					if (  ( mapcount > 0  ) 
					   && ( to_winno >= 0 ) )
					{
						frecv_winputs( to_winno, 
						    output_string, mapcount );
						return( -1 );
					}
				}
			}
		}
		if ( Vpix_E0 )
		{
			Vpix_keys[ c + 128 ] = 1;
			Vpix_E0 = 0;
		}
		else
			Vpix_keys[ c ] = 1;
	}
	else
	{
		c &= 0x7F;
		if ( Vpix_E0 )
		{
			Vpix_keys[ c + 128 ] = 0;
			Vpix_E0 = 0;
		}
		else
			Vpix_keys[ c ] = 0;
	}
	return( 0 );
}
/**************************************************************************
* vpix_keys_up
*	When we switch windows, the keys in scan code mode have to come
*	up.  Fake it.
**************************************************************************/
vpix_keys_up()
{
	int	i;

	if ( Vpix_E0 )
	{
		output_key_to_window( 0xB8 );
	}
	for ( i = 0; i < 128; i++ )
	{
		if ( Vpix_keys[ i ] )
		{
			output_key_to_window( i | 0x80 );
			Vpix_keys[ i ] = 0;
		}
	}
	for ( i = 0; i < 128; i++ )
	{
		if ( Vpix_keys[ i + 128 ] )
		{
			output_key_to_window( 0xE0 );
			output_key_to_window( i | 0x80 );
			Vpix_keys[ i + 128 ] = 0;
		}
	}
}
#endif
/**************************************************************************
* chk_keys_in_pipe
*	See if there are any characters read ahead that were queued in
*	the pipe and not read by the sender.
*	Return character or -1 for none.
**************************************************************************/
chk_keys_in_pipe()	/* -1 = NONE */
{
	int	status;
	char	c;

	status = read( Pipe_keys_read, &c, 1 );
	if ( status == 1 )
		return( ( (int) c ) & 0x00FF );
	else if ( status == 0 )
		return( -1 );
	else if ( ( status == -1 ) && ( errno == EAGAIN ) )
		return( -1 );
	else
	{
		frecv_kill( "Can't check keys in pipe", -1 );
		/* does not return */
		/*NOTREACHED*/
	}
}
/**************************************************************************
* put_keys_in_pipe
*	Put the "count" number of keys in string "s" into the read ahead
*	buffer pipe.
*	Sender may read from here if we switch to window command mode.
**************************************************************************/
put_keys_in_pipe( s, count )	
	char	*s;
	int	count;
{
	int	status;

	status = write( Pipe_keys_write, s, count );
	if ( status == count )
		return( 0 );
	else if ( status < 0 )
	{
		frecv_kill( "Can't put keys in pipe", -1 );
		/* does not return */
		/*NOTREACHED*/
	}
	else
	{
		printf( "Tried to write %d keys in pipe\r\n", count );
		term_outgo();
		frecv_kill( "Only wrote %d keys in pipe", status );
		/*NOTREACHED*/
	}
}
#include <termio.h>
/**************************************************************************
* determine_tty_characteristics
*	See if we are in 8 bit and if PARMRK is set.
**************************************************************************/
determine_tty_characteristics()
{
	struct termio Termio;

	ioctl( 0, TCGETA, &Termio );
	if ( ( Termio.c_cflag & CSIZE ) == CS8 )
		F_tty_is_CS8 = 1;
	if ( ( Termio.c_iflag & PARMRK ) == PARMRK )
		Opt_use_PARMRK_for_break = 1;
}
#define MAX_MAPPED_BUFF 512
/**************************************************************************
* receive_map
*	Get a map command sent by the sender.
*	This is:
*		string to map - only single chars are implemented
*		0xFF
*		type:					m=map    u=unmap
*		window on which map applies 		+ '0'
*		window to which result string is sent	+ '0'
*		result string
*		0x00
**************************************************************************/
receive_map()
{
	int	result;
	int	count;
	char	*p;
	char	*string2;
	int	map_type;
	int	on_winno;
	int	to_winno;
	char	buff[ MAX_MAPPED_BUFF + 1 ];
	char	*input_string;
	char	*output_string;

	count = 0;
	p = buff;
	string2 = (char *) 0;
	while ( 1 )
	{
		result = get_window_mode_ans();
		*p++ = result;
		if ( result <= 0 )
			break;
		if ( result == 0xFF )
			string2 = p;
		count++;
		if ( count >= MAX_MAPPED_BUFF )
		{
			while ( get_window_mode_ans() != 0 )
				;
			return;
		}
	}
	if ( string2 == (char *) 0 )
		return;
	if ( count < 5 )
		return;
	map_type = buff[ 0 ];
	on_winno = buff[ 1 ] - '0';
	to_winno = buff[ 2 ] - '0';
	input_string = &buff[ 3 ];
	*(string2 - 1) = '\0';
	output_string = string2;
	switch( map_type )
	{
		case 'm':
			install_mapped( input_string, on_winno, 
					output_string, to_winno );
			break;
		case 'u':
			unmap_mapped( input_string, to_winno );
			break;
		default:
			break;
	}
}
#include <fcntl.h>
#define MAX_KEYSTROKE_BUFF 512
/**************************************************************************
* receive_keystroke_capture
*	Sender is specifying a keystroke capture file. Null terminated.
*	Close old file if open.
*	Open new file.
**************************************************************************/
receive_keystroke_capture()
{
	int	result;
	int	count;
	char	*p;
	char	pathname[ MAX_KEYSTROKE_BUFF + 1 ];
	int	fd;
	long	time();

	count = 0;
	p = pathname;
	while ( 1 )
	{
		result = get_window_mode_ans();
		*p++ = result;
		if ( result <= 0 )
			break;
		count++;
		if ( count >= MAX_KEYSTROKE_BUFF )
		{
			while ( get_window_mode_ans() != 0 )
				;
			return;
		}
	}
	if ( Keystroke_capture_fd > 0 )
	{
		close( Keystroke_capture_fd );
		Keystroke_capture_fd = 0; 
	}
	if ( count <= 0 )
		return;
	fd = open( pathname, O_RDWR | O_APPEND, 0 );
	if ( fd <= 0)
	{
		return;
	}
	else
	{
		Keystroke_capture_fd = fd;
		Keystroke_capture_time = time( (long *) 0 );
	}
}
/**************************************************************************
* receive_keystroke_play
*	Sender is specifying a keystroke replay file - null terminated.
*	Close old file if open.
*	Open new file.
**************************************************************************/
receive_keystroke_play()
{
	int	result;
	int	count;
	char	*p;
	char	pathname[ MAX_KEYSTROKE_BUFF + 1 ];
	int	fd;

	count = 0;
	p = pathname;
	while ( 1 )
	{
		result = get_window_mode_ans();
		*p++ = result;
		if ( result <= 0 )
			break;
		count++;
		if ( count >= MAX_KEYSTROKE_BUFF )
		{
			while ( get_window_mode_ans() != 0 )
				;
			return;
		}
	}
	if ( Keystroke_play_fd > 0 )
		close_keystroke_play();
	if ( count <= 0 )
		return;
	fd = open( pathname, O_RDONLY );
	if ( fd <= 0)
	{
		return;
	}
	else
		Keystroke_play_fd = fd;
}
/**************************************************************************
* keystroke_play_cancel
*	Stop a keystroke replay. Tell the sender and wait for go ahead.
**************************************************************************/
keystroke_play_cancel()
{
	close_keystroke_play();
	fct_window_noplay();
	wait_for_sender( -1 );
}
/**************************************************************************
* close_keystroke_play
*	Close keystroke replay file.
**************************************************************************/
close_keystroke_play()
{
	close( Keystroke_play_fd );
	Keystroke_play_fd = 0; 
}
#define MAX_SCREEN_SAVER_BUFF 10
/**************************************************************************
* receive_screen_saver_time
*	Receive screen saver alarm time from sender.
**************************************************************************/
receive_screen_saver_time()
{
	int	result;
	int	count;
	char	*p;
	char	buffer[ MAX_SCREEN_SAVER_BUFF + 1 ];

	count = 0;
	p = buffer;
	while ( 1 )
	{
		result = get_window_mode_ans();
		*p++ = result;
		if ( result <= 0 )
			break;
		count++;
		if ( count >= MAX_SCREEN_SAVER_BUFF )
		{
			while ( get_window_mode_ans() != 0 )
				;
			return;
		}
	}
	if ( count <= 0 )
		return;
	Screen_saver_time = atoi( buffer );
}
#define MAX_SENDER_BUFFER 100
char	Sender_buffer[ MAX_SENDER_BUFFER + 1 ];
int	Sender_buffer_count = 0;
/**************************************************************************
* save_keys_from_send
*	The key "c" was read by the sender by mistake while trying to
*	negotiate with an HP terminal to print something.
**************************************************************************/
save_keys_from_send( c )
	int	c;
{
	if ( Sender_buffer_count < MAX_WINDOW_QUEUE )
	{
		Sender_buffer[ Sender_buffer_count ] = c;
		Sender_buffer_count++;
	}
}
/**************************************************************************
* chk_keys_from_send
*	See if there are any characters from save_keys_from_send above.
**************************************************************************/
chk_keys_from_send( buff, max )
	char	*buff;
	int	max;
{
	int	count;
	int	i;

	if ( Sender_buffer_count <= 0 )
		return( 0 );
	if ( Sender_buffer_count <= max )
	{
		count = Sender_buffer_count;
		Sender_buffer_count = 0;
	}
	else
	{
		count = max;
		Sender_buffer_count -= count;
	}
	memcpy( buff, Sender_buffer, count );
	for ( i = 0; i < Sender_buffer_count; i++ )
		Sender_buffer[ i ] = Sender_buffer[ i + count ] ;
	return( count );
}
