/*****************************************************************************
** Copyright (c) 1986 - 1991 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: commonsend.c,v 70.1 92/03/09 15:41:37 ssa Exp $ */

#include "ptydefs.h"

/**************************************************************************
* commonsend.c
*	Sender's primary interface to driver or pseudo-ttys
**************************************************************************/
#include	"person.h"

#include        <sys/types.h>
#include        "facetwin.h"



#include	"wins.h"
#include	"options.h"
int	Opt_kill_processes_on_shutdown = 0;

#include	<stdio.h>

#include <termio.h>


			/* fcntl needed for open */
#include        <fcntl.h>
			/* needed for signal */
#include        <signal.h>
			/* needed for error check */
#include        <errno.h>


#ifndef SIG_ERR
#define SIG_ERR         (int (*)()) -1
#endif

extern char     *ttyname();
extern int      errno;

extern int	shutdown_func();
extern int	recvquit();

#include "wincommon.h"


		/**********************************************************
		* The names of the windows.
		**********************************************************/
#include "ptypath.h"
char	Slave_pty_path_store[ 10 ][ 80 ] = { "" };
char	*Slave_pty_path[ 10 ] = 
{ 	Slave_pty_path_store[ 0 ], Slave_pty_path_store[ 1 ],
	Slave_pty_path_store[ 2 ], Slave_pty_path_store[ 3 ],
	Slave_pty_path_store[ 4 ], Slave_pty_path_store[ 5 ],
	Slave_pty_path_store[ 6 ], Slave_pty_path_store[ 7 ],
	Slave_pty_path_store[ 8 ], Slave_pty_path_store[ 9 ]
};

#include "facet.h"
int	Pid_send;			/* process id of the sender process */

int	Pid_recv;			/* process id of the receiver process */
unsigned	Facetline;		/* facet line number */

#include <setjmp.h>
jmp_buf	Env;

#include "ft_notbusy.h"

#include "facetterm.h"

#include "facetpath.h"


		/**********************************************************
		* respawn information
		*	Respawn_wanted 	!= 0 means respawn
		*	Respawn_program is program name and arguments.
		*	Respawn_title   is the window title.
		**********************************************************/
int	Respawn_wanted[ USR_WINDS] = { 0 };
char	*Respawn_program[ USR_WINDS] = { (char *) 0 };
char	*Respawn_window_title[ USR_WINDS ] = { (char *) 0 };

char	*Text_number_of_windows =
	"\nNumber of windows set to %d\n";

char	*Text_facet_line_available =
	"Facet process: Facet line # %d is available.\n";

char	*Text_all_facet_busy =
	"Facet process: All psuedo ttys are busy.\n";

char	*Text_driver_not_installed =
	"Facet process: Facet driver not properly installed. (%d)\n";

char	*Text_assign_facet_busy =
	"Facet process: Assigned Facet line is busy.\n";

char	*Text_assign_facet_exist =
	"Facet process: Assigned Facet line does not exist. (%d)\n";

char	*Text_cannot_open_dev_facet =
	"Facet process: can't open %s, error %d\n";

char	*Text_window_status_error =
	"Facet process: Error determining status of windows. (%d)\n";

char	*Text_assign_windows_open =
	"Facet process: Assigned Facet line # %d has windows already open.\n";

char	*Text_facet_line_windows_open =
  "Facet process: Facet line # %d is available but has windows already open.\n";

char	*Text_facet_activating =
	"Facet process: activating\r\n";

char	*Text_use_activate_to_run =
	"Use the 'Activate' command to run: '";	/* TBD */
		/**********************************************************
		* Composite file descriptor mask of all windows.
		* File descriptor mask indexed by window number 0-9.
		**********************************************************/
int	Fd_mask = 0;
int	Fd_masks[ USR_WINDS ] = { 0 };
		/**********************************************************
		* File descriptor mask of all blocked windows.
		**********************************************************/
int	Fd_blocked_mask = 0;

/**************************************************************************
* common
*	The main routine after initialization.
**************************************************************************/
						/* PSEUDOTTY */
#include "fd.h"
int	Fd_max = -1;
int	Ack_mask = 0;
int	Fd_master[ USR_WINDS ];

#include <sys/ptyio.h>
int	Trap_on = 1;
		/**********************************************************
		* The names of the windows.
		**********************************************************/
char	Slave_pty_name_store[ 10 ][ 15 ] = { "" };
char	*Slave_pty_name[ 10 ] =
{ 	Slave_pty_name_store[ 0 ], Slave_pty_name_store[ 1 ],
	Slave_pty_name_store[ 2 ], Slave_pty_name_store[ 3 ],
	Slave_pty_name_store[ 4 ], Slave_pty_name_store[ 5 ],
	Slave_pty_name_store[ 6 ], Slave_pty_name_store[ 7 ],
	Slave_pty_name_store[ 8 ], Slave_pty_name_store[ 9 ]
};





/**************************************************************************
* common
*	The main routine after initialization.
**************************************************************************/
common()					/* PSEUDOTTY */
{
	int	stat;
	char	*getenv();
	int	status;

	/******************************************************************
	* Stdin must be a tty and have a ttyname.
	******************************************************************/
	if ( ttyname( 0 ) == (char *) 0 )
	{
		printf(
		  "Facet process: Only tty ports support FACET mode.\n" );
		printf(
		  "               Stdin is not a tty.\n" );
		exit( 1 );
	}
	/******************************************************************
	* Stdin must not be a window unless the environment variable is
	* set to indicate that it is all right to run on a window.
	******************************************************************/
	if ( getenv( "RECURSIVE" ) == NULL )
	{
		if ( iswindow() )
		{
			if ( This_is_ft_notbusy )
				exit( 1 );
			else
				exit( 0 );
		}
	}
	/******************************************************************
	* If number of windows is limited - display prompt.
	******************************************************************/
			/* "\nNumber of windows set to %d\n" */
	if (  ( Wins < 10 ) 
	   && ( This_is_ft_notbusy == 0 ) 
	   && ( Opt_nonstop == 0 ) )
	{
		printf( Text_number_of_windows, Wins );
	}
	/******************************************************************
	* Title, version, and Copyright
	******************************************************************/
	if ( This_is_ft_notbusy == 0 )
		header();
	/******************************************************************
	** Select the facet line and windows.
	******************************************************************/
	set_pty_dirs();
	if ( Opt_nonstop == 0 )
	{
		printf( "Ptys: " );
		fflush( stdout );
	}
	status = open_facet_line();
	if ( Opt_nonstop == 0 )
		printf( "\n" );
	if ( status != 0 )
	{
			/* "Facet process: All pseudo ttys are busy.\n" */
		printf( Text_all_facet_busy,
			Facetline );
		wait_return_pressed();
		exit( 1 );
	}
	if ( This_is_ft_notbusy )
		exit( 0 );
	/******************************************************************
	* Save current crt status.
	******************************************************************/
	if ( termio_save() )
		exit( 1 );
	/******************************************************************
	* Setup ioctls for terminal line, read terminal description file,
	* get user acknowlegement, initialize error recording.
	******************************************************************/
	if ( setcrt() )
		exit( 1 );

	/******************************************************************
	* Record sender pid.
	* Export user name in FACETUSER to environment.
	* Set windows to 622 owned by user.
	******************************************************************/
	Pid_send = getpid();
	set_user_id();
	set_owner_on_windows();
	/******************************************************************
	* Catch receiver fatal error signal.
	******************************************************************/
	if ( signal( SIGUSR2, recvquit ) == SIG_ERR )
	{
		perror( "Facet process: Can't catch SIGUSR2" );
		printf( "\r\n" );
		termio_restore();
		exit( 1 );
	}
	if ( signal( SIGINT, SIG_IGN ) == SIG_ERR )
	{
		perror( "Facet process: Can't ignore SIGINT" );
		printf( "\r\n" );
		termio_restore();
		exit( 1 );
	}
	/******************************************************************
	* Fork off the receive process.
	******************************************************************/
	if ( (Pid_recv = fork()) == -1 )
	{
		printf( "Facet process: Can't fork, error %d\r\n", errno );
		perror( "             " );
		term_outgo();
		termio_restore();
		exit( 1 );
	}

	if ( Pid_recv )
	{
		/**********************************************************
		* Parent - SENDER.
		**********************************************************/
		if ( setjmp( Env ) == 0 )
		{
				/* Catch the hangup signals on the tty  */
			if ( signal( SIGHUP, shutdown_func ) == SIG_ERR )
			{
			    fsend_kill( 
				"Facet process: sender signal SIGHUP", -1 );
			    /* does not return */
			}
			if ( signal( SIGTERM, shutdown_func ) == SIG_ERR )
			{
			    fsend_kill(
			    	"Facet process: sender signal SIGTERM", -1 );
			    /* does not return */
			}
			if ( signal( SIGUSR2, shutdown_func ) == SIG_ERR )
			{
			    fsend_kill(
			    	"Facet process: sender signal SIGUSR2", -1 );
			    /* does not return */
			}
			/* "Facet process: activating\r\n" */
			printf( Text_facet_activating );
			term_outgo();
			/**************************************************
			* Main loop of sender.
			**************************************************/
			fsend_send();
		}
		else
		{
			/**************************************************
			* SENDER exiting.
			**************************************************/
			close_capture_files();
			fsend_finish();
			term_outgo();
			kill( Pid_recv, SIGUSR2 );
			wait( &stat );
			termio_restore();
			set_utmp_idle_all();
			reset_owner_on_windows();
			close_windows();
		}
	}
	else
	{
		/**********************************************************
		* Child - RECEIVER.
		**********************************************************/
		start_receiver( Fd_master );
	}
	exit( 0 );
/* NOTREACHED */
}
								/* PSEUDOTTY */
/**************************************************************************
* shutdown_func
*	Interrupt routine for SIGHUP, SIGINT, SIGTERM,, SIGUSR2.
*	Cause facetterm to shutdown and exit.
**************************************************************************/
shutdown_func()
{
	signal( SIGHUP, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGTERM, SIG_IGN );
	signal( SIGUSR2, SIG_IGN );
	longjmp( Env, 1 );
}
/**************************************************************************
* recvquit
*	Temporary shutdown function for immediate receiver fatal error.
**************************************************************************/
recvquit()
{
	signal( SIGHUP, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGTERM, SIG_IGN );
	signal( SIGUSR2, SIG_IGN );
	termio_restore();
	reset_owner_on_windows();
	close_windows();
	exit( 1 );
}
/**************************************************************************
* fsend_kill
*	Sender shutdown - possibly from error.
*	"code" being -1 indicates a system call error where errno is
*	significant.
*	Otherwise both code and message are internal.
*	Print msg and shutdown.
**************************************************************************/
fsend_kill( msg, code )
	char	msg[];
	int	code;
{
	term_outgo();
	if ( code == -1 )
	{
		perror( msg );
		printf( "\r" );
	}
	else
		printf( msg, code );
	term_outgo();
	shutdown_func();
}

char	*Text_sender_wait_close =
	"Facet process: sender waiting for windows to close...\n";

char	*Text_facet_attempt_kill =
"Facet process: attempting to kill processes that would not hang up...\n";

char	*Text_facet_attempt_list =
"Facet process: attempting to list processes that would not hang up...\n";

char	*Text_sender_term_wait_close =
"Facet process: sender terminating - waiting for windows to close...\n";

char	*Text_sender_term =
	"Facet process: sender terminating...\n";

							/* PSEUDOTTY */
int Kill_seconds = 20;
/**************************************************************************
* close_windows
*	If kill processes option is in effect:
*	Turn off carrier on windows and signal processes to exit.
*	If any windows fail to close, run a script to try and locate
*	them and list them.
**************************************************************************/
close_windows()					/* HP9000 PSEUDOTTY TSM */
{
	int	result;
	char	command[ 800 ];
	int	i;

	if ( Opt_kill_processes_on_shutdown )
	{
/* "Facet process: sender waiting for windows to close...\n" */
		printf( Text_sender_wait_close );
		fflush( stdout );
		fsend_hangup();
		result = fsend_wait_windows_close( Kill_seconds );
		fsend_kill_signal();
		if ( result )
		{
/* "Facet process: attempting to list processes that would not hang up...\n" */
			printf( Text_facet_attempt_list );
			fflush( stdout );
			sprintf( command, "%s/sys/%swinlist",
						Facettermpath, Facetprefix );
			for ( i = 0; i < Wins; i++ )
			{
				strcat( command, " " );
				strcat( command, Slave_pty_path[ i ] );
			}
			mysystem( command );
/* "Facet process: sender terminating - waiting for windows to close...\n" */
			printf( Text_sender_term_wait_close );
			fflush( stdout );
			return;
		}
	}
	/* "Facet process: sender terminating...\n" */
	printf( Text_sender_term );
	fflush( stdout );
}
						/* HP9000 PSEUDOTTY TSM */

/**************************************************************************
* fsend_kill_seconds
*	Set users option for the grace period for processes to exit.
**************************************************************************/
							/* PSEUDOTTY */
fsend_kill_seconds( seconds )
	int	seconds;
{
	Kill_seconds = seconds;
}
#include "screensave.h"
/**************************************************************************
* fsend_send
*	Main routine for SENDER.
**************************************************************************/
fsend_send() 
{
	/******************************************************************
	* Close unneeded ends of pipes to receiver.
	* Allocate and initialize window buffers.
	* Set terminals function keys if necessary.
	* Set terminals starting modes if necessary.
	******************************************************************/
	fsend_init();
	/******************************************************************
	* Turn carrier on for all windows.
	******************************************************************/
	fsend_oncarrier();
	/******************************************************************
	* Process options and start programs in .facet file.
	******************************************************************/
	run_facet();

	/******************************************************************
	* Wait for the receiver to be ready.
	******************************************************************/
	wait_until_receiver_ready();
	/******************************************************************
	* Send the receiver:
	*	hot-key
	*	indication that "break" key should be sent to window.
	*	screen saver timer - if applicable.
	*	current window - to start receiver processing.
	******************************************************************/
	fct_window_mode_ans( 'w' );
	fct_window_mode_ans( (char) Opt_hotkey );
	if ( Opt_no_prompt_hotkey > 0 )
 	{
 		fct_window_mode_ans( 'n' );
 		fct_window_mode_ans( (char) Opt_no_prompt_hotkey );
 	}
	if ( Opt_send_break_to_window )
		fct_window_mode_ans( 'b' );
	if ( Screen_saver_timer )
		fct_window_mode_ans_int( 's', Screen_saver_timer );
	fct_window_mode_ans_curwin();

			/* multiplex all window output onto real line */
	fsend_process_windows();
	/* does not return */
}
int Receiver_ready = 0;
/**************************************************************************
* wait_until_receiver_ready
*
**************************************************************************/
wait_until_receiver_ready()
{
	while( Receiver_ready == 0 )
		fsend_get_acks_only();
}
/**************************************************************************
* receiver_says_ready
*
**************************************************************************/
receiver_says_ready()
{
	Receiver_ready = 1;
}
/**************************************************************************
* fct_window_mode_ans_int
*	Send the receiver:
*	a character command "type"
*	followed by an integer number "number" as a string of ascii digits,
*	followed by a null to terminate the number.
**************************************************************************/
fct_window_mode_ans_int( type, number )
	char	type;
	int	number;
{
	char	buffer[ 80 ];
	char	c;
	char	*p;

	fct_window_mode_ans( type );
	sprintf( buffer, "%d", number );
	p = buffer;
	while( ( c = *p++ ) != '\0' )
		fct_window_mode_ans( c );
	fct_window_mode_ans( '\0' );
}
/**************************************************************************
* fsend_oncarrier
*	Cause the driver to simulate carrier on on all windows at startup.
**************************************************************************/
							/* PSEUDOTTY */
fsend_oncarrier()		/* turn on carrier for all windows on line */
{
	/* not used in PSEUDOTTY */
}
/**************************************************************************
* fsend_offcarrier
*	Simulate loss of carrier and signalling to processes.
*	Call only when shutdown in progress and line is not in raw mode.
**************************************************************************/
		/**********************************************************
		* Number of characters to read from a window. Used to 
		* restrict the number of characters processed from a 
		* transparent print window.
		**********************************************************/
#include "curwinno.h"
#include "readwindow.h"
int	Read_window_max[ TOPWIN ] = 
{
	READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX,
	READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX,
	READ_WINDOW_MAX, READ_WINDOW_MAX
};
		/**********************************************************
		* Set Read_window_max to Transparent_print_read_window_max
		* when a window goes into transparent print.
		**********************************************************/
int Transparent_print_read_window_max = READ_WINDOW_MAX;

		/*********************************************************
		* When print is active, this is set to the number of 
		* milliseconds to wait for output from other windows before 
		* reading transparent print window.
		**********************************************************/
int Transparent_print_timer = 0;

#include "printtimer.h"
		/**********************************************************
		* Number of milliseconds to wait after there was output 
		* from other windows.
		**********************************************************/
int Transparent_print_quiet_timer = 2000;
		/**********************************************************
		* Number of milliseconds to wait after there was no output 
		* from other windows.
		**********************************************************/
int Transparent_print_idle_timer = 100;
int Transparent_print_short_timer = 100;
int Transparent_print_idle_timer_max = 1000;
		/**********************************************************
		* Number of milliseconds to wait after a significant output
		* drain delay.
		**********************************************************/
int Transparent_print_full_timer = 10000;
int Transparent_print_delay_count = 5;
		/**********************************************************
		* Do not throttle transparent print output.
		**********************************************************/
int Transparent_print_timers_disable = 0;
		/**********************************************************
		* Mask of windows with transparent print chars waiting.
		**********************************************************/
int Fd_print_mask = 0;
		/**********************************************************
		* Mask of windows with in printer mode. - These do not show
		* idle even if not open.
		**********************************************************/
#include "printonly.h"
int Fd_window_printer_mask = 0;
int Transparent_print_delayed = 0;
/**************************************************************************
* set_print_active
*	New transparent print characters - set timer and mask.
**************************************************************************/
set_print_active( winno, seconds_delayed )
	int	winno;
	int	seconds_delayed;
{
	if ( Transparent_print_timers_disable )
		return;
	if ( Fd_print_mask == 0 )
		Transparent_print_timer = Transparent_print_quiet_timer;
	if ( seconds_delayed > 0 )
	{
		if ( ( seconds_delayed > 1 ) || Transparent_print_delayed )
		{
			Transparent_print_timer = Transparent_print_full_timer;
			Transparent_print_short_timer +=
						Transparent_print_idle_timer;
			if ( Transparent_print_short_timer >
					Transparent_print_idle_timer_max )
			{
				Transparent_print_short_timer = 
					Transparent_print_idle_timer_max;
			}
			Transparent_print_delayed = 0;
		}
		else
		{
			Transparent_print_delayed =
						Transparent_print_delay_count;
		}
	}
	else if ( Transparent_print_delayed > 0 )
	{
		Transparent_print_delayed--;
	}
	Fd_print_mask = Fd_masks[ winno ];
}
#include "printer.h"
/**************************************************************************
* check_print_inactive
*	New transparent print condition - see if mask is inappropriate.
**************************************************************************/
check_print_inactive( winno )
	int	winno;
{
	if ( Window_printer[ winno ] )
		return;
	if ( transparent_print_is_on_winno( winno ) )
		return;
	if ( Fd_print_mask == Fd_masks[ winno ] )
		Fd_print_mask = 0;
}
/**************************************************************************
* set_window_blocked_mask
*	The Fd_blocked_mask tracks the window_blocked array.
*	These windows are not read unless current.
**************************************************************************/
set_window_blocked_mask( window_blocked )
	int	window_blocked[];
{
	int	i;

	Fd_blocked_mask = 0;
	for ( i = 0; i < Wins; i++ )
	{
		if ( window_blocked[ i ] )
			Fd_blocked_mask |= Fd_masks[ i ];
	}
}
/**************************************************************************
* set_window_printer_mask
*	The Fd_window_printer_mask track the Window_printer array.  These
*	windows do not show idle even if closed.
**************************************************************************/
set_window_printer_mask()
{
	int	i;

	Fd_window_printer_mask = 0;
	for ( i = 0; i < Wins; i++ )
	{
		if ( Window_printer[ i ] )
			Fd_window_printer_mask |= Fd_masks[ i ];
	}
}
/**************************************************************************
* report_screen_activity
*	The receiver calls this at half the screen saver timing and at
*	full screen saver timing, and will not activate the screen saver
*	if output activity is reported in the second half.
**************************************************************************/
int	T_screen_activity = 0;
report_screen_activity()
{
	fct_window_mode_ans( 'a' );
	if ( T_screen_activity )
		fct_window_mode_ans( '1' );
	else
		fct_window_mode_ans( '0' );
	T_screen_activity = 0;
}
							/* PSEUDOTTY */
/**************************************************************************
* fsend_get_acks_only
*	Ingore windows and listen only for messages from the receiver.
*	Used when a send_packet_signal has been sent to the receiver.
*	Because of timing, the message from the receiver could be a
*	hot-key or other message rather than a result of the signal.
*	We handle whatever message arrives, making it the responsibility
*	of the caller to determine if the expected message has arrived.
**************************************************************************/
fsend_get_acks_only()					/* PSEUDOTTY */
{
	unsigned int	ackchars[ 1 ];
			/* process only acks from receiver */
	term_outgo();
						/* get acks only, 
						   no window chars
						   and/or packet */
	
	read_control_pipe( ackchars );
						/* process acks */
	fsend_procack_array( 1, ackchars );
}

#include <time.h>

/**************************************************************************
* any_windows_are_ready
*	0     = All windows have no output or are blocked.
*	not 0 = a non-blocked window has output.
*	Used to check if screen saver should be cancelled.
**************************************************************************/
any_windows_are_ready()
{
	int	readfds;
	int	writefds;
	int	exceptfds;
	int	status;
	int	blocked;
	struct timeval	timeout;

	blocked = Fd_blocked_mask & ( ~ Fd_masks[ Curwinno ] );
	readfds = Ack_mask | ( Fd_mask & ( ~blocked ) );
	if ( Opt_print_only_hp_personality && M_pe )
		readfds &= ( ~ Fd_window_printer_mask );
	writefds = 0;
	exceptfds = Fd_mask;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	status = select( Fd_max +1, 
				 &readfds, &writefds, &exceptfds,
				 &timeout );
	if ( status < 0 )
	{
		fsend_kill( "Facet process: select R", -1 );
		/* does not return */
	}
	return( status );
}
/**************************************************************************
* fsend_process_windows
*	Main loop of SENDER.
**************************************************************************/
fsend_process_windows() /* PSEUDOTTY */
{
	int	winno;
	int	readfds;
	int	writefds;
	int	exceptfds;
	int	status;
	int	blocked;

			/* multiplex all window output onto real line */
	while (1) 
	{
		term_outgo();
		/**********************************************************
		* If transparent print active, watch for output from all
		* non transparent print windows for period in 
		* Transparent_print_timer.
		* If no characters then switch to idle timer.
		* Otherwise continue to ignore transparent print windows
		* switch to longer quiet timer.
		**********************************************************/
		blocked = Fd_blocked_mask & ( ~ Fd_masks[ Curwinno ] );
		readfds = Ack_mask | ( Fd_mask & ( ~blocked ) );
		if ( Opt_print_only_hp_personality && M_pe )
			readfds &= ( ~ Fd_window_printer_mask );
		writefds = 0;
		exceptfds = Fd_mask;
		status = 0;
		if ( Fd_print_mask && ( Transparent_print_timer > 0 ) )
		{
			int		printreadfds;
			struct timeval	timeout;

			printreadfds = readfds & ( ~Fd_print_mask );
			timeout.tv_sec = Transparent_print_timer / 1000;
			timeout.tv_usec = 
				( (long)( Transparent_print_timer % 1000 ) )
				* 1000;
			status = select( Fd_max +1, 
					 &printreadfds, &writefds, &exceptfds,
					 &timeout );
			if ( status < 0 )
			{
				fsend_kill( "Facet process: select T", -1 );
				/* does not return */
			}
			if ( status == 0 )
				Transparent_print_timer = 
					Transparent_print_short_timer;
			else
			{
				readfds = printreadfds;
				Transparent_print_timer = 
					Transparent_print_quiet_timer;
			}
		}
		/**********************************************************
		* If no infomation was ready above, do an untimed wait for
		* information including print windows.
		* If a window that was transparent printing has no characters
		* then remove it from the transparent print active mask.
		**********************************************************/
		if ( status == 0 )
		{
			exceptfds = Fd_mask;
			status = select( Fd_max +1, 
					 &readfds, &writefds, &exceptfds,
					 (struct timeval *) 0 );
			if ( status < 0 )
			{
				fsend_kill( "Facet process: select", -1 );
				/* does not return */
			}
			if ( Fd_print_mask )
				if ( ( readfds & Fd_print_mask ) == 0 )
				{
					Fd_print_mask = 0;
					Transparent_print_short_timer =
						Transparent_print_idle_timer;
				}
		}
		/**********************************************************
		* Process "acks" - messages from receiver.
		**********************************************************/
		if ( readfds & Ack_mask )
		{
						/* process acks */
			fsend_get_acks_only();
			continue;
		}
		/**********************************************************
		* Process characters from the windows.
		* Set screen activity if any characters from windows.
		**********************************************************/
		T_screen_activity = 1;
		for ( winno = 0; winno < Wins; winno++ )
	   	{
			if ( readfds & Fd_masks[ winno ] )
			{
				read_window( winno, Fd_master[ winno ] );
				continue;
			}
			if ( exceptfds & Fd_masks[ winno ] )
			{
				exception_window( winno, Fd_master[ winno ] );
			}
	    	} 
	}
}
/**************************************************************************
* exception_window
*	Process an open, close, or ioctl from window number "winno"
*	whose master side is open on file descriptor "fd".
**************************************************************************/
exception_window( winno, fd )				/* PSEUDOTTY */
	int	winno;
	int	fd;
{
	struct request_info req;
	int	request;
	char	fioc_buffer[ FIOC_BUFFER_SIZE ];
	char	fioc_buffer_out[ FIOC_BUFFER_SIZE ];
	int	status;

	/******************************************************************
	* Get the request.
	******************************************************************/
	status = ioctl( Fd_master[ winno ], TIOCREQCHECK, &req );
	if ( status < 0 )
	{
		perror( "TIOCREQCHECK" );
		return;
	}
	/******************************************************************
	* Initialize the error return just in case.
	******************************************************************/
	req.errno_error = 0;
	/******************************************************************
	* Act on the request.
	******************************************************************/
	request = req.request & IOCCMD_MASK;
	if ( req.request == TIOCOPEN )
	{
		if ( check_window_active( winno ) == 0 )
		{
			/**************************************************
			* Application has opened the window.
			**************************************************/
			termio_normal_master( fd );
			window_first_open( winno );
		}
	}
	else if ( req.request == TIOCCLOSE )
	{
		/**********************************************************
		* All applications have closed the window.
		**********************************************************/
		window_closed( winno );
		set_utmp_idle( winno );
		set_owner_on_window( winno );
		if ( Respawn_wanted[ winno] == 0 )
			window_idle_actions( winno );
		termio_normal_master_off( fd );
	}
	else if ( ( request >> 8 ) == 'M' )
	{
	}
	else if ( ( request >> 8 ) == 'K' )
	{
	}
	else if ( req.request == FIONBIO )
	{
		/**********************************************************
		* from SoftPC on exit
		**********************************************************/
	}
	else if ( req.request == FIOC_CURWIN )
	{
		/**********************************************************
		* Return the current window number ( 1-10 ) as a
		* string of decimal digits.
		**********************************************************/
		get_curwin_string( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_CURWIN" );
	}
	else if ( req.request == FIOC_WINDOW_MODE )
	{
		/**********************************************************
		* Execute the string as if it had been typed on
		* the keyboard following a hot-key.
		**********************************************************/
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_WINDOW_MODE" );
		else
			process_window_mode_commands( fioc_buffer, winno );
	}
	else if ( req.request == FIOC_GET_PROGRAM_NAME )
	{
		/**********************************************************
		* Retrieve the window title for the window.
		**********************************************************/
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_GET_PROGRAM_NAME get" );
		else
		{
			get_program_name( winno, fioc_buffer, fioc_buffer );
			if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
				perror( "FIOC_GET_PROGRAM_NAME set" );
		}
	}
	else if ( req.request == EXEC_LIST )
	{
		/**********************************************************
		* Title for a window.
		**********************************************************/
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_EXEC_LIST" );
		else
			remember_exec_list( winno, fioc_buffer );
	}
	else if ( req.request == FIOC_ACTIVE )
	{
		/**********************************************************
		* Retrieve a blank separated list of the window
		* numbers of windows that are not or are
		* in print mode.
		**********************************************************/
		build_active_list( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_ACTIVE set" );
	}
	else if ( req.request == FIOC_IDLE )
	{
		/**********************************************************
		* Retrieve a blank separated list of the window
		* numbers of windows that are not open and are
		* not in print mode.
		**********************************************************/
		build_idle_list( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_IDLE set" );
	}
	else if ( req.request == FIOC_PUSH_POPUP_SCREEN )
	{
		/**************************************************
		* Save the appearance of the screen for restoration
		* by a subsequent pop.
		**************************************************/
		req.errno_error = do_push_popup_screen( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_PUSH_POPUP_SCREEN set" );
	}
	else if ( req.request == FIOC_POP_POPUP_SCREEN )
	{
		/**************************************************
		* Restore the appearance of the screen to that
		* saved by the corresponding push.
		**************************************************/
		req.errno_error = do_pop_popup_screen( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_POP_POPUP_SCREEN set" );
	}
	else if ( req.request == FIOC_GET_INFORMATION )
	{
		/**********************************************************
		* Retrieve information based on the request string.
		**********************************************************/
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_GET_INFORMATION get" );
		else
		{
			get_information( winno, fioc_buffer, fioc_buffer_out,
					 FIOC_BUFFER_SIZE );
			if ( ioctl( fd, req.argset, fioc_buffer_out ) < 0 )
				perror( "FIOC_GET_INFORMATION set" );
		}
	}
	else if ( req.request == FIOC_PASTE )
	{
		/**********************************************************
		* Paste the specified characters into a window.
		**********************************************************/
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_PASTE" );
		else
			process_ioctl_paste( fioc_buffer, winno );
	}
	else if ( req.request == FIOC_UTMP )
	{
		/**********************************************************
		* Add this window to the utmp file with the the
		* users name.
		* This prevents "fct_startw" and "fct_runwin" from
		* having to be suid root.
		**********************************************************/
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_UTMP" );
		else
			set_utmp_in_use( fioc_buffer, winno );
	}
	else
	{
		printf( "\r\nrequest=%x %c %d argget=%x argset=%x %x\r\n",
			req.request, 
			(req.request & IOCCMD_MASK) >>8,
			req.request & 0xFF,
			req.argget,
			req.argset,
			FIOC_GET_PROGRAM_NAME );
		req.errno_error = EINVAL;
	}
	/******************************************************************
	* Return the result.
	******************************************************************/
	status = ioctl( fd, TIOCREQSET, &req );
	if ( status < 0 )
	{
		perror( "TIOCREQSET" );
	}
	if ( ( req.request == TIOCCLOSE ) && ( Respawn_wanted[ winno] ) )
	{
		respawn( winno );
	}
}
/**************************************************************************
* read_window
*	Process characters from window number "winno"
*	whose master side is open on file descriptor "fd".
**************************************************************************/
read_window( winno, fd )				/* PSEUDOTTY */
	int	winno;
	int	fd;
{
	int	count;
	char	window_out_buff[ READ_WINDOW_MAX ];

	count = read( fd, window_out_buff, Read_window_max[ winno ] );
	if ( count < 0 )
	{
		perror( "window read" );
	}
	else if ( count > 0 )
	{
		fsend_window( winno, window_out_buff, count );
	}
}
/**************************************************************************
* fsend_wait_windows_close
*	Watch the windows until they all close or "seconds" seconds 
*	has expired.
**************************************************************************/
fsend_wait_windows_close( seconds )			/* PSEUDOTTY */
	int	seconds;
{
	int	winno;
	int	readfds;
	int	writefds;
	int	exceptfds;
	int	status;
	struct timeval timeout;
	int	all_closed;
	long	time();
	long	start_time;


	if ( window_closed_check_all( -1 ) )
		return( 0 );
	start_time = time( (long *) 0 );
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	while (1) 
	{
		if ( ( time( (long *) 0 ) - start_time ) > ( seconds -1 ) )
			return( 1 );
		readfds = Fd_mask;
		writefds = 0;
		exceptfds = Fd_mask;
		status = select( Fd_max +1, 
				 &readfds, &writefds, &exceptfds,
				 &timeout );
		if ( status == 0 )
			return( 1 );
		if ( status < 0 )
			return( -1 );
		for ( winno = 0; winno < Wins; winno++ )
	   	{
			if ( readfds & Fd_masks[ winno ] )
			{
				read_wait_windows_close( Fd_master[ winno ] );
				continue;
			}
			if ( exceptfds & Fd_masks[ winno ] )
			{
				all_closed = exception_wait_windows_close(
							winno,
							Fd_master[ winno ] );
				if ( all_closed )
					return( 0 );
			}
	    	} 
	}
}
/**************************************************************************
* exeception_wait_windows_close
*	Process an open, close, or ioctl from window number "winno"
*	whose master side is open on file descriptor "fd".
*	During fsend_wait_windows_close.
**************************************************************************/
exception_wait_windows_close( winno, fd )		/* PSEUDOTTY */
	int	winno;
	int	fd;
{
	struct request_info req;
	int	request;
	int	status;
	int	all_closed;

	all_closed = 0;
	status = ioctl( Fd_master[ winno ], TIOCREQCHECK, &req );
	if ( status < 0 )
	{
		perror( "TIOCREQCHECK" );
		return( all_closed );
	}
	request = req.request & IOCCMD_MASK;
	if ( req.request == TIOCCLOSE )
	{
		if ( window_closed_check_all( winno ) )
			all_closed = 1;
	}
	else if ( ( request >> 8 ) == 'F' )
	{
		req.errno_error = EIO;
	}
	else if ( ( request >> 8 ) == 'M' )
	{
		req.errno_error = EIO;
	}
	else if ( ( request >> 8 ) == 'K' )
	{
		req.errno_error = EIO;
	}
	else if ( req.request == FIONBIO )
	{
		/**********************************************************
		* from SoftPC on exit
		**********************************************************/
		req.errno_error = EIO;
	}
	else
	{
		printf( "\r\nrequest=%x %c %d argget=%x argset=%x %x\r\n",
			req.request, 
			(req.request & IOCCMD_MASK) >>8,
			req.request & 0xFF,
			req.argget,
			req.argset,
			FIOC_GET_PROGRAM_NAME );
		req.errno_error = EIO;
	}
	status = ioctl( fd, TIOCREQSET, &req );
	if ( status < 0 )
	{
		perror( "TIOCREQSET c" );
	}
	return( all_closed );
}
/**************************************************************************
* read_window
*	Process characters from window number "winno"
*	whose master side is open on file descriptor "fd".
*	During fsend_wait_windows_close.
**************************************************************************/
read_wait_windows_close( fd )				/* PSEUDOTTY */
	int	fd;
{
	char	window_out_buff[ READ_WINDOW_MAX ];

	read( fd, window_out_buff, READ_WINDOW_MAX );
}
							/* PSEUDOTTY */
/**************************************************************************
* set_pty_dirs
*	Initialize the pty_directory variables, if necessary, based
*	upon which directories are present on the machine.
**************************************************************************/
int	Using_pty_directories = 0;
char	*Master_pty_dir = NULL;
char	*Slave_pty_dir = NULL;
int	Pty_start = -1;
set_pty_dirs()						/* PSEUDOTTY */
{
	if ( Master_pty_dir == NULL )
	{
		if (  ( access( "/dev/ptym", 0 ) == 0 )
		   && ( access( "/dev/pty", 0 ) == 0 ) )
		{
			Master_pty_dir = "/dev/ptym/";
			Slave_pty_dir = "/dev/pty/";
			Using_pty_directories = 1;
		}
		else
		{
			Master_pty_dir = "/dev/";
			Slave_pty_dir = "/dev/";
			Using_pty_directories = 0;
		}
	}
}
/**************************************************************************
* open_facet_line
*	Select "Wins" number of psuedo_ttys for use as windows.
*	Set up the masks for the select calls based on the file
*	descriptors.
*	If the user has not chosen the "nonstop" option, indicate
*	which ones are being used.
**************************************************************************/
open_facet_line()					/* PSEUDOTTY */
{
	int	i;
	int	fd;
	char	master_pty_path[ 80 ];

	for ( i = 0; i < Wins; i++ )
	{
		/**********************************************************
		* Find 1 line returning the paths and names of the master
		* and slave side.  The slave info is stored for later use.
		**********************************************************/
		fd = open_master( master_pty_path,
				  Slave_pty_path[ i ], Slave_pty_name[ i ] );
		if  ( fd < 0 )
			return( -1 );
		if ( Opt_nonstop == 0 )
		{
			printf( " %s", Slave_pty_name[ i ] );
			fflush( stdout );
		}
		/**********************************************************
		* Store the file descriptor for later use.
		**********************************************************/
		Fd_master[ i ] = fd;
		if ( Fd_master[ i ] > Fd_max )
			Fd_max = Fd_master[ i ];
		Fd_masks[ i ] = ( 1 << Fd_master[ i ] );
		Fd_mask |= Fd_masks[ i ];
	}
	set_window_printer_mask();
	return( 0 );
}
/**************************************************************************
* open_master
*	Select and open a suitable pair of pseudo ttys,
*	returning the pathname of the master side in "master_pty_path",
*	the pathname of the slave size in "slave_pty_path"
*	and just the slave filename in "slave_pty_name".
*	"Pty_start" is an index into the names to help
*	prevent trying the same ptys for subsequent windows.
**************************************************************************/
						/* PSEUDOTTY */
open_master( master_pty_path, slave_pty_path, slave_pty_name )
	char	*master_pty_path;
	char	*slave_pty_path;
	char	*slave_pty_name;
{
	int	fd;
	int	pty_number;

	pty_number = Pty_start;
	while ( 1 )
	{
		pty_number++;
		/**********************************************************
		* Generate the names for pty number "pty_number".
		**********************************************************/
		if ( get_pty_names( pty_number, master_pty_path,
				    slave_pty_path, slave_pty_name ) < 0 )
		{
			break;
		}
		/**********************************************************
		* Try to open the master side.
		**********************************************************/
		if ( (fd = open( master_pty_path, O_RDWR )) >= 0 )
		{
			/**************************************************
			* NDELAY is used to prevent getting stuck on a 
			* window that appeared to have characters but 
			* didn't. - We wait at the select.
			**************************************************/
			if ( fcntl( fd, F_SETFL,
					    fcntl( fd, F_GETFL, 0 ) | O_NDELAY )
			     < 0 )
			{
				printf( "\n" );
				perror( "on F_SETFL" );
				close( fd );
				continue;
			}
			/**************************************************
			* Make sure the slave is not missing.
			**************************************************/
			if ( access( slave_pty_path, 0 ) < 0 )
			{
				close( fd );
				continue;
			}
			/**************************************************
			* Make sure we can set TRAP ioctl MODE.
			**************************************************/
			if ( ioctl( fd, TIOCTRAP, &Trap_on ) < 0 )
			{
				printf( "\n" );
				perror( "TIOCTRAP" );
				close( fd );
				continue;
			}
			/**************************************************
			* Make PTY block signals till ioctls complete.
			**************************************************/
			if ( ioctl( fd, TIOCSIGMODE, TIOCSIGBLOCK ) < 0 )
			{
				printf( "\n" );
				perror( "TIOCSIGMODE" );
				close( fd );
				continue;
			}
			/**************************************************
			* Echo is off when window not open.
			**************************************************/
			termio_normal_master_off( fd );
			/**************************************************
			* Return the number of the one we used.
			**************************************************/
			Pty_start = pty_number;
			return( fd );
		}
		else if ( errno == EBUSY )
		{
		}
		else if ( errno == ENXIO )
		{
			break;
		}
		else if ( errno == ENOENT )
		{
		}
		else
		{
		}
	}
	return( -1 );
}
/**************************************************************************
* get_pty_names
*	Generate the names for pty number "pty_number". Return path names
*	in "master_pty_path" and "slave_pty_path", and just the file name
*	In "slave_pty_name".
**************************************************************************/
							/* PSEUDOTTY */
get_pty_names( pty_number, master_pty_path, slave_pty_path, slave_pty_name )
	int	pty_number;
	char	*master_pty_path;
	char	*slave_pty_path;
	char	*slave_pty_name;
{
	static	char	*master_pty_base[] = 
	{
		"ptyp", "ptyq", "ptyr",
		"ptys", "ptyt", "ptyu", "ptyv", "ptyw", "ptyx", "ptyy", "ptyz",
		"ptya", "ptyb", "ptyc",
		"ptye", "ptyf", "ptyg", "ptyh", "ptyi", "ptyj", "ptyk", "ptyl",
		"ptym", "ptyn", "ptyo",
	};
	static	char	*slave_pty_base[] = 
	{
		"ttyp", "ttyq", "ttyr",
		"ttys", "ttyt", "ttyu", "ttyv", "ttyw", "ttyx", "ttyy", "ttyz",
		"ttya", "ttyb", "ttyc",
		"ttye", "ttyf", "ttyg", "ttyh", "ttyi", "ttyj", "ttyk", "ttyl",
		"ttym", "ttyn", "ttyo",
	};
	static	char	*pty_ending[] =
	{
		"0", "1", "2", "3", "4", "5", "6", "7", 
		"8", "9", "a", "b", "c", "d", "e", "f"
	};
	static int num_pty_ending = sizeof( pty_ending ) / sizeof( char * );
	static int num_base = sizeof( master_pty_base ) / sizeof( char * );
	int	base;
	int	ending;

	if ( ( Using_pty_directories == 0 ) && ( pty_number >= 48 ) )
		return( -1 );
	base = pty_number / num_pty_ending;
	if ( base >= num_base )
	{
		int	pty_number_2;

		pty_number_2 = pty_number - ( num_base * num_pty_ending );
		return (
			get_pty_names_2(  pty_number_2,
					master_pty_path,
					slave_pty_path,
					slave_pty_name )
		       );
	}
	ending = pty_number % num_pty_ending;
	sprintf( slave_pty_name, "%s%s",
				slave_pty_base[ base ],  pty_ending[ ending ] );

	/******************************************************************
	* Allow for possibility that ptys are in /dev directory even
	* though there is a pty directory.
	* The function "ttyname" gives preference to these, so we must
	* also.
	******************************************************************/
	if (  ( strcmp( master_pty_base[ base ], "ptyp" ) == 0 )
	   || ( strcmp( master_pty_base[ base ], "ptyq" ) == 0 )
	   || ( strcmp( master_pty_base[ base ], "ptyr" ) == 0 ) )
	{
		sprintf( master_pty_path, "%s%s%s",
		    "/dev/",	master_pty_base[ base ], pty_ending[ ending ] );
		sprintf( slave_pty_path, "%s%s%s",
		    "/dev/",	slave_pty_base[ base ],  pty_ending[ ending ] );
		if ( access( master_pty_path, 0 ) == 0 )
			return( 0 );
		if ( access( slave_pty_path, 0 ) == 0 )
			return( 0 );
	}
	/******************************************************************
	* Pty is in sub directory or is missing.
	******************************************************************/
	sprintf( master_pty_path, "%s%s%s",
		Master_pty_dir,	master_pty_base[ base ], pty_ending[ ending ] );
	sprintf( slave_pty_path, "%s%s%s",
		Slave_pty_dir,	slave_pty_base[ base ],  pty_ending[ ending ] );

	return( 0 );
}
/**************************************************************************
* get_pty_names_2
*	Generate the names for pty number "pty_number_2". Return path names
*	in "master_pty_path" and "slave_pty_path", and just the file name
*	In "slave_pty_name".
*	This is the algorithm for the next 2500 ptys on HP.
**************************************************************************/
							/* HP9000 PSEUDOTTY */
get_pty_names_2( pty_number_2, master_pty_path, slave_pty_path, slave_pty_name )
	int	pty_number_2;
	char	*master_pty_path;
	char	*slave_pty_path;
	char	*slave_pty_name;
{
	static	char	*master_pty_base_2[] = 
	{
		"ptyp", "ptyq", "ptyr",
		"ptys", "ptyt", "ptyu", "ptyv", "ptyw", "ptyx", "ptyy", "ptyz",
		"ptya", "ptyb", "ptyc",
		"ptye", "ptyf", "ptyg", "ptyh", "ptyi", "ptyj", "ptyk", "ptyl",
		"ptym", "ptyn", "ptyo"
	};
	static	char	*slave_pty_base_2[] = 
	{
		"ttyp", "ttyq", "ttyr",
		"ttys", "ttyt", "ttyu", "ttyv", "ttyw", "ttyx", "ttyy", "ttyz",
		"ttya", "ttyb", "ttyc",
		"ttye", "ttyf", "ttyg", "ttyh", "ttyi", "ttyj", "ttyk", "ttyl",
		"ttym", "ttyn", "ttyo"
	};
	static	char	*pty_ending_2[] =
	{
		"00", "01", "02", "03", "04", "05", "06", "07", "08", "09",
		"10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
		"20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
		"30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
		"40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
		"50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
		"60", "61", "62", "63", "64", "65", "66", "67", "68", "69",
		"70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
		"80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
		"90", "91", "92", "93", "94", "95", "96", "97", "98", "99",
	};
	static int num_pty_ending_2 = sizeof( pty_ending_2 ) / sizeof( char * );
	static int num_base_2 = sizeof( master_pty_base_2 ) / sizeof( char * );
	int	base;
	int	ending;

	base = pty_number_2 / num_pty_ending_2;
	if ( base >= num_base_2 )
	{
		int	pty_number_3;

		pty_number_3 = pty_number_2 - ( num_base_2 * num_pty_ending_2 );
		return (
			get_pty_names_3(  pty_number_3,
					master_pty_path,
					slave_pty_path,
					slave_pty_name )
		       );
	}
	ending = pty_number_2 % num_pty_ending_2;
	sprintf( slave_pty_name, "%s%s",
			slave_pty_base_2[ base ],  pty_ending_2[ ending ] );
	/******************************************************************
	* Pty is in sub directory.
	******************************************************************/
	sprintf( master_pty_path, "%s%s%s",
		Master_pty_dir,
		master_pty_base_2[ base ], pty_ending_2[ ending ] );
	sprintf( slave_pty_path, "%s%s%s",
		Slave_pty_dir,
		slave_pty_base_2[ base ],  pty_ending_2[ ending ] );
	return( 0 );
}
/**************************************************************************
* get_pty_names_3
*	Generate the names for pty number "pty_number_3". Return path names
*	in "master_pty_path" and "slave_pty_path", and just the file name
*	In "slave_pty_name".
*	This is the algorithm for the last 25000 ptys on HP.
**************************************************************************/
							/* HP9000 PSEUDOTTY */
get_pty_names_3( pty_number_3, master_pty_path, slave_pty_path, slave_pty_name )
	int	pty_number_3;
	char	*master_pty_path;
	char	*slave_pty_path;
	char	*slave_pty_name;
{
	static	char	*master_pty_base_3[] = 
	{
		"ptyp", "ptyq", "ptyr",
		"ptys", "ptyt", "ptyu", "ptyv", "ptyw", "ptyx", "ptyy", "ptyz",
		"ptya", "ptyb", "ptyc",
		"ptye", "ptyf", "ptyg", "ptyh", "ptyi", "ptyj", "ptyk", "ptyl",
		"ptym", "ptyn", "ptyo"
	};
	static	char	*slave_pty_base_3[] = 
	{
		"ttyp", "ttyq", "ttyr",
		"ttys", "ttyt", "ttyu", "ttyv", "ttyw", "ttyx", "ttyy", "ttyz",
		"ttya", "ttyb", "ttyc",
		"ttye", "ttyf", "ttyg", "ttyh", "ttyi", "ttyj", "ttyk", "ttyl",
		"ttym", "ttyn", "ttyo"
	};
	static	char	*pty_ending_3[] =
	{
	"000", "001", "002", "003", "004", "005", "006", "007", "008", "009",
	"010", "011", "012", "013", "014", "015", "016", "017", "018", "019",
	"020", "021", "022", "023", "024", "025", "026", "027", "028", "029",
	"030", "031", "032", "033", "034", "035", "036", "037", "038", "039",
	"040", "041", "042", "043", "044", "045", "046", "047", "048", "049",
	"050", "051", "052", "053", "054", "055", "056", "057", "058", "059",
	"060", "061", "062", "063", "064", "065", "066", "067", "068", "069",
	"070", "071", "072", "073", "074", "075", "076", "077", "078", "079",
	"080", "081", "082", "083", "084", "085", "086", "087", "088", "089",
	"090", "091", "092", "093", "094", "095", "096", "097", "098", "099",
	"100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
	"110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
	"120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
	"130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
	"140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
	"150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
	"160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
	"170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
	"180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
	"190", "191", "192", "193", "194", "195", "196", "197", "198", "199",
	"200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
	"210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
	"220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
	"230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
	"240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
	"250", "251", "252", "253", "254", "255", "256", "257", "258", "259",
	"260", "261", "262", "263", "264", "265", "266", "267", "268", "269",
	"270", "271", "272", "273", "274", "275", "276", "277", "278", "279",
	"280", "281", "282", "283", "284", "285", "286", "287", "288", "289",
	"290", "291", "292", "293", "294", "295", "296", "297", "298", "299",
	"300", "301", "302", "303", "304", "305", "306", "307", "308", "309",
	"310", "311", "312", "313", "314", "315", "316", "317", "318", "319",
	"320", "321", "322", "323", "324", "325", "326", "327", "328", "329",
	"330", "331", "332", "333", "334", "335", "336", "337", "338", "339",
	"340", "341", "342", "343", "344", "345", "346", "347", "348", "349",
	"350", "351", "352", "353", "354", "355", "356", "357", "358", "359",
	"360", "361", "362", "363", "364", "365", "366", "367", "368", "369",
	"370", "371", "372", "373", "374", "375", "376", "377", "378", "379",
	"380", "381", "382", "383", "384", "385", "386", "387", "388", "389",
	"390", "391", "392", "393", "394", "395", "396", "397", "398", "399",
	"400", "401", "402", "403", "404", "405", "406", "407", "408", "409",
	"410", "411", "412", "413", "414", "415", "416", "417", "418", "419",
	"420", "421", "422", "423", "424", "425", "426", "427", "428", "429",
	"430", "431", "432", "433", "434", "435", "436", "437", "438", "439",
	"440", "441", "442", "443", "444", "445", "446", "447", "448", "449",
	"450", "451", "452", "453", "454", "455", "456", "457", "458", "459",
	"460", "461", "462", "463", "464", "465", "466", "467", "468", "469",
	"470", "471", "472", "473", "474", "475", "476", "477", "478", "479",
	"480", "481", "482", "483", "484", "485", "486", "487", "488", "489",
	"490", "491", "492", "493", "494", "495", "496", "497", "498", "499",
	"500", "501", "502", "503", "504", "505", "506", "507", "508", "509",
	"510", "511", "512", "513", "514", "515", "516", "517", "518", "519",
	"520", "521", "522", "523", "524", "525", "526", "527", "528", "529",
	"530", "531", "532", "533", "534", "535", "536", "537", "538", "539",
	"540", "541", "542", "543", "544", "545", "546", "547", "548", "549",
	"550", "551", "552", "553", "554", "555", "556", "557", "558", "559",
	"560", "561", "562", "563", "564", "565", "566", "567", "568", "569",
	"570", "571", "572", "573", "574", "575", "576", "577", "578", "579",
	"580", "581", "582", "583", "584", "585", "586", "587", "588", "589",
	"590", "591", "592", "593", "594", "595", "596", "597", "598", "599",
	"600", "601", "602", "603", "604", "605", "606", "607", "608", "609",
	"610", "611", "612", "613", "614", "615", "616", "617", "618", "619",
	"620", "621", "622", "623", "624", "625", "626", "627", "628", "629",
	"630", "631", "632", "633", "634", "635", "636", "637", "638", "639",
	"640", "641", "642", "643", "644", "645", "646", "647", "648", "649",
	"650", "651", "652", "653", "654", "655", "656", "657", "658", "659",
	"660", "661", "662", "663", "664", "665", "666", "667", "668", "669",
	"670", "671", "672", "673", "674", "675", "676", "677", "678", "679",
	"680", "681", "682", "683", "684", "685", "686", "687", "688", "689",
	"690", "691", "692", "693", "694", "695", "696", "697", "698", "699",
	"700", "701", "702", "703", "704", "705", "706", "707", "708", "709",
	"710", "711", "712", "713", "714", "715", "716", "717", "718", "719",
	"720", "721", "722", "723", "724", "725", "726", "727", "728", "729",
	"730", "731", "732", "733", "734", "735", "736", "737", "738", "739",
	"740", "741", "742", "743", "744", "745", "746", "747", "748", "749",
	"750", "751", "752", "753", "754", "755", "756", "757", "758", "759",
	"760", "761", "762", "763", "764", "765", "766", "767", "768", "769",
	"770", "771", "772", "773", "774", "775", "776", "777", "778", "779",
	"780", "781", "782", "783", "784", "785", "786", "787", "788", "789",
	"790", "791", "792", "793", "794", "795", "796", "797", "798", "799",
	"800", "801", "802", "803", "804", "805", "806", "807", "808", "809",
	"810", "811", "812", "813", "814", "815", "816", "817", "818", "819",
	"820", "821", "822", "823", "824", "825", "826", "827", "828", "829",
	"830", "831", "832", "833", "834", "835", "836", "837", "838", "839",
	"840", "841", "842", "843", "844", "845", "846", "847", "848", "849",
	"850", "851", "852", "853", "854", "855", "856", "857", "858", "859",
	"860", "861", "862", "863", "864", "865", "866", "867", "868", "869",
	"870", "871", "872", "873", "874", "875", "876", "877", "878", "879",
	"880", "881", "882", "883", "884", "885", "886", "887", "888", "889",
	"890", "891", "892", "893", "894", "895", "896", "897", "898", "899",
	"900", "901", "902", "903", "904", "905", "906", "907", "908", "909",
	"910", "911", "912", "913", "914", "915", "916", "917", "918", "919",
	"920", "921", "922", "923", "924", "925", "926", "927", "928", "929",
	"930", "931", "932", "933", "934", "935", "936", "937", "938", "939",
	"940", "941", "942", "943", "944", "945", "946", "947", "948", "949",
	"950", "951", "952", "953", "954", "955", "956", "957", "958", "959",
	"960", "961", "962", "963", "964", "965", "966", "967", "968", "969",
	"970", "971", "972", "973", "974", "975", "976", "977", "978", "979",
	"980", "981", "982", "983", "984", "985", "986", "987", "988", "989",
	"990", "991", "992", "993", "994", "995", "996", "997", "998", "999",
	};
	static int num_pty_ending_3 = sizeof( pty_ending_3 ) / sizeof( char * );
	static int num_base_3 = sizeof( master_pty_base_3 ) / sizeof( char * );
	int	base;
	int	ending;

	base = pty_number_3 / num_pty_ending_3;
	if ( base >= num_base_3 )
		return( -1 );
	ending = pty_number_3 % num_pty_ending_3;
	sprintf( slave_pty_name, "%s%s",
			slave_pty_base_3[ base ],  pty_ending_3[ ending ] );
	/******************************************************************
	* Pty is in sub directory.
	******************************************************************/
	sprintf( master_pty_path, "%s%s%s",
		Master_pty_dir,
		master_pty_base_3[ base ], pty_ending_3[ ending ] );
	sprintf( slave_pty_path, "%s%s%s",
		Slave_pty_dir,
		slave_pty_base_3[ base ],  pty_ending_3[ ending ] );
	return( 0 );
}
							/* PSEUDOTTY */

/**************************************************************************
* fsend_hangup
*	Send SIGHUP to all windows.
* fsend_hangup_to_winno
*	Send SIGHUP to window number winno 0-9.
* fsend_kill_to_winno
*	Send SIGKILL to window number winno 0-9.
**************************************************************************/
fsend_hangup()						/* PSEUDOTTY */
{
	int	i;

	for ( i = 0; i < Wins; i++ )
	{
		if ( ioctl( Fd_master[ i ], TIOCSIGSEND, SIGHUP ) < 0 )
		{
			perror( "TIOCSIGSEND hup" );
		}
	}
}
fsend_hangup_to_winno( winno )				/* PSEUDOTTY */
	int	winno;
{
	if ( ioctl( Fd_master[ winno ], TIOCSIGSEND, SIGHUP ) < 0 )
	{
		perror( "TIOCSIGSEND hup" );
	}
}
fsend_kill_to_winno( winno )				/* PSEUDOTTY */
	int	winno;
{
	if ( ioctl( Fd_master[ winno ], TIOCSIGSEND, SIGKILL ) < 0 )
	{
		perror( "TIOCSIGSEND hup" );
	}
}
fsend_int_to_winno( winno )				/* PSEUDOTTY */
	int	winno;
{
	if ( ioctl( Fd_master[ winno ], TIOCSIGSEND, SIGINT ) < 0 )
	{
		perror( "TIOCSIGSEND hup" );
	}
}
/**************************************************************************
* fsend_kill_signal
*	Send SIGKILL to all windows.
**************************************************************************/
fsend_kill_signal()					/* PSEUDOTTY */
{
	int	i;

	for ( i = 0; i < Wins; i++ )
	{
		if ( ioctl( Fd_master[ i ], TIOCSIGSEND, SIGKILL ) < 0 )
		{
			perror( "TIOCSIGSEND kill" );
		}
	}
}
							/* PSEUDOTTY */
/**************************************************************************
* fsend_break
*	Send "break" to window number "winno" 0-9.
**************************************************************************/
fsend_break( winno )					/* PSEUDOTTY */
	int	winno;
{
	if ( ioctl( Fd_master[ winno ], TIOCBREAK, 0 ) < 0 )
	{
		perror( "TIOCBREAK" );
	}
}



							/* PSEUDOTTY */

#include "options.h"
int	Opt_clear_window_on_open = 0;
int	Opt_clear_window_on_close = 0;
int	Opt_no_split_numbers = 0;
int	Opt_no_split_dividers = 0;
int	Opt_no_quit_prompt = 0;
int	Opt_disable_quit_while_windows_active = 0;

#include "pagingopt.h"
int	Lock_window_1 = 0;

#include "controlopt.h"
int	Idle_window = NO_IDLE_WINDOW;
int	Idle_window_no_cancel = 0;
char	Idle_window_paste_chars[ IDLE_WINDOW_PASTE_CHARS_MAX + 1 ] = "";
int	Windows_window = -1;
int	Windows_window_no_cancel = 0;

#include "wwindowkey.h"
char Windows_window_char_start = WINDOWS_WINDOW_CHAR_START;
char Windows_window_char_stop = WINDOWS_WINDOW_CHAR_STOP;

#include "touserkey.h"
int Window_mode_to_user_char = WINDOW_MODE_TO_USER_CHAR;

#include "notifykey.h"

#include "allowbeep.h"

#include "blocked.h"

/**************************************************************************
* run_facet
*	Process options and start programs in .facet file.
**************************************************************************/
run_facet()
{
	FILE	*open_dotfacet();
	FILE	*facetfile;
	char	buffer[ STARTWIN_COMMAND_MAX + 1 ];
	int	window;
	char	*temp_encode();
	char	tempbuff[ STARTWIN_COMMAND_MAX + 1 ];
	int	seconds;
	char	window_title[ STARTWIN_TITLE_MAX + 1 ];
	int	window_title_last;
	char	*p;
	char	c;
	char	cc;
	int	winno;
	char	dotfacetname[ 256 ];
	char	*b;
	int	respawn;
	int	nostart;
	char	*strdup();
	char	*getenv();
	int	facet_echo_dotfacet;
	char	*window_title_to_use;

	/******************************************************************
	* Window title lines are effective only on immediately following
	* line.
	******************************************************************/
	window_title_last = 0;
	strcpy( window_title, "" );
	/******************************************************************
	* Find and open the .facet file.
	******************************************************************/
	sprintf( dotfacetname, ".%s", Facetname );
	facetfile = open_dotfacet( dotfacetname );
	if ( facetfile == NULL )
		return;
	/******************************************************************
	* Process each line of .facet file.
	******************************************************************/
	facet_echo_dotfacet = 0;
	if ( getenv( "FACETECHODOTFACET" ) != ( (char *) 0 ) )
		facet_echo_dotfacet = 1;
	while ( read_dotfacet( buffer, STARTWIN_COMMAND_MAX, facetfile ) >= 0 )
	{
		if ( facet_echo_dotfacet )
		{
			term_outgo();
			fct_decode_string( 0, ".facet: " );
			fct_decode_string( 0, buffer );
			fct_decode_string( 0, "\r\n" );
			term_outgo();
		}
		/**********************************************************
		* Window title is only effective on next line.
		**********************************************************/
		if ( window_title_last == 0 )
			strcpy( window_title, "" );
		else
			window_title_last = 0;
		if ( strcmp( buffer, "lock_window_1" ) == 0 )
		{
			/**************************************************
			* Try to keep window 1 in a terminal page.
			**************************************************/
			Lock_window_1 = 1;
			continue;
		}
		if ( strcmp( buffer, "clear_window_on_open" ) == 0 )
		{
			/**************************************************
			* Clear screen when a window first opens.
			**************************************************/
			Opt_clear_window_on_open = 1;
			continue;
		}
		if ( strcmp( buffer, "clear_window_on_close" ) == 0 )
		{
			/**************************************************
			* Clear screen on last close of a window. 
			* This could erase error message of why 
			* application did not run or erase its results.
			**************************************************/
			Opt_clear_window_on_close = 1;
			continue;
		}
		if ( strncmp( buffer, "idle_window=", 12 ) == 0 )
		{
			/**************************************************
			* Window to switch to when current window goes idle.
			* If set to 'A', the next window that is active is
			* selected.
			**************************************************/
			if ( (buffer[ 12 ] == 'A') || (buffer[ 12 ] == 'a') )
			{
				Idle_window = IDLE_WINDOW_IS_NEXT_ACTIVE;
				Idle_window_paste_chars[ 0 ] = '\0';
			}
			else
			{
				window = atoi( &buffer[ 12 ] ) - 1;
				if ( window < 0 )
					window = 9;
				if ( window < Wins )
				{
					Idle_window = window;
					Idle_window_paste_chars[ 0 ] = '\0';
				}
			}
			continue;
		}
		if ( strncmp( buffer, "idle_window_paste_chars=", 24 ) == 0 )
		{
			/**************************************************
			* Instead of switching paste these chars to idle window.
			**************************************************/
			if (buffer[ 24 ] != '\0')
			{
				temp_encode( &buffer[ 24 ], tempbuff );
				strncpy( Idle_window_paste_chars,
					 tempbuff,
					 IDLE_WINDOW_PASTE_CHARS_MAX );
				Idle_window_paste_chars[
					IDLE_WINDOW_PASTE_CHARS_MAX ] = '\0';
			}
			continue;
		}
		if ( strcmp( buffer, "idle_window_no_cancel" ) == 0 )
		{
			/**************************************************
			* Do not disable the idle window setting even if
			* the idle window closes.
			**************************************************/
			Idle_window_no_cancel = 1;
			continue;
		}
		if ( strncmp( buffer, "windows_window=", 15 ) == 0 )
		{
			/**************************************************
			* Send this window a 'windows_window_start_char'
			* when break is pressed.
			**************************************************/
			window = atoi( &buffer[ 15 ] ) - 1;
			if ( window < 0 )
				window = 9;
			if ( window < Wins )
				Windows_window = window;
			continue;
		}
		if ( strcmp( buffer, "windows_window_no_cancel" ) == 0 )
		{
			/**************************************************
			* Do not disable the windows_window setting even if
			* the window closes.
			**************************************************/
			Windows_window_no_cancel = 1;
			continue;
		}
		if ( strncmp( buffer, "allow_beep_offscreen=", 21 ) == 0 )
		{
			/**************************************************
			* Control-G from the specified window goes to the
			* terminal even if it is not on-screen. More than
			* one window may be specified.
			**************************************************/
			window = atoi( &buffer[ 21 ] ) - 1;
			if ( window < 0 )
				window = 9;
			if ( window < Wins )
				Allow_beep_offscreen[ window ] = 1;
			continue;
		}
		if ( strncmp( buffer, "notify_when_current_char=", 25 ) == 0 )
		{
			/**************************************************
			* The character to send to a window when it is
			* set to "notify_on_current" and it becomes current.
			**************************************************/
			Notify_when_current_char = 
				*( temp_encode( &buffer[ 25 ], tempbuff ) );
			continue;
		}
		if ( strncmp( buffer, "windows_window_char_start=", 26 ) == 0 )
		{
			/**************************************************
			* The character to send to a 'windows_window' when
			* break is pressed.
			**************************************************/
			Windows_window_char_start = 
				*( temp_encode( &buffer[ 26 ], tempbuff ) );
			continue;
		}
		if ( strncmp( buffer, "windows_window_char_stop=", 25 ) == 0 )
		{
			/**************************************************
			* The character to send to a pop-up window when
			* the pop-up is cancelled.
			**************************************************/
			Windows_window_char_stop = 
				*( temp_encode( &buffer[ 25 ], tempbuff ) );
			continue;
		}
		if ( strncmp( buffer, "window_mode_to_user_char=", 25 ) == 0 )
		{
			/**************************************************
			* The character to be used in a "WINDOW_MODE" ioctl
			* to indicate that the user must finish the command.
			* E.g. Put the program in cut mode and then user
			* must position and ok.
			**************************************************/
			Window_mode_to_user_char = 
				*( temp_encode( &buffer[ 25 ], tempbuff ) );
			continue;
		}
		if ( strncmp( buffer, "function_keys=", 14 ) == 0 )
		{
			/**************************************************
			* Load the specified function key file to all 
			* windows.
			**************************************************/
			if ( load_all_function_keys( &buffer[ 14 ], -1 ) )
			{
				fct_decode_string( 0,
					"\r\nWARNING - function key file '" );
				fct_decode_string( 0, &buffer[ 14 ] );
				fct_decode_string( 0, "' was not found\r\n" );
			}
			continue;
		}
		if ( strncmp( buffer, "function_keys-", 14 ) == 0 )
		{
			/**************************************************
			* Load the specified function key file in the
			* specified window.
			**************************************************/
			winno = -1;
			p = &buffer[ 14 ];
			c = *p++;
			if ( c == '0' ) 
			{
				cc = *p++;
				if ( cc == '=' )
					winno = 9;
			}
			else if ( c == '1' )
			{
				cc = *p++;
				if ( cc == '0' )
				{
					c = *p++;
					if ( c == '=' )
						winno = 9;
				}
				else if ( cc == '=' )
					winno = 0;
			}
			else if ( ( c >= '2' ) && ( c <= '9' ) )
			{
				cc = *p++;
				if ( cc == '=' )
					winno = c - '1';
			}
			if ( winno < 0 )
			{
				fct_decode_string( 0, 
					"Invalid window number: " );
				fct_decode_string( 0, buffer );
				continue;
			}
			/**************************************************
			* Ignore if windows are limited to less than this
			* specification.
			**************************************************/
			if ( winno >= Wins )
				continue;
			if ( load_all_function_keys( p, winno ) )
			{
				fct_decode_string( 0,
					"\r\nWARNING - function key file '" );
				fct_decode_string( 0, p );
				fct_decode_string( 0, "' was not found\r\n" );
			}
			continue;
		}
		if ( strcmp( buffer, "no_split_numbers" ) == 0 )
		{
			/**************************************************
			* Do not put the window number on the split screen
			* dividers.
			**************************************************/
			Opt_no_split_numbers = 1;
			continue;
		}
		if ( strcmp( buffer, "no_split_dividers" ) == 0 )
		{
			/**************************************************
			* Use blanks for the split screen dividers.
			**************************************************/
			Opt_no_split_dividers = 1;
			continue;
		}
		if ( strcmp( buffer, "quit_on_all_windows_idle" ) == 0 )
		{
			/**************************************************
			* Exit facetterm if all windows are closed.
			**************************************************/
			Opt_quit_on_all_windows_idle = 1;
			continue;
		}
		if ( strcmp( buffer, "disable_quit_while_windows_active"
									) == 0 )
		{
			/**************************************************
			* Do not allow ^Wqy or ^Wxhy with windows open
			**************************************************/
			Opt_disable_quit_while_windows_active = 1;
			continue;
		}
		if ( strcmp( buffer, "no_quit_prompt" ) == 0 )
		{
			/**************************************************
			* Do not ask for confirmation on Control-W Q.
			**************************************************/
			Opt_no_quit_prompt = 1;
			continue;
		}
		if ( strncmp( buffer, "kill_seconds=", 13 ) == 0 )
		{
			/**************************************************
			* Set the number of seconds allowed for processes
			* to shut down and close the windows when exiting.
			**************************************************/
			seconds = atoi( &buffer[ 13 ] );
			if ( seconds > 0 )
				fsend_kill_seconds( seconds );
			continue;
		}
		if ( strcmp( buffer, "disable_control_W_r" ) == 0 )
		{
			/**************************************************
			* Do not allow the user to use Control-W r to 
			* start programs. Useful to prevent running an
			* unrestricted shell for rsh users.
			**************************************************/
			Opt_disable_control_W_r = 1;
			continue;
		}
		if ( strcmp( buffer, "disable_control_W_x_excl" ) == 0 )
		{
			/**************************************************
			* Do not allow the user to use Control-W v r to 
			* start programs on raw tty. 
			**************************************************/
			Opt_disable_control_W_x_excl = 1;
			continue;
		}
		if ( strncmp( buffer, "window_title=", 13 ) == 0 )
		{
			/**************************************************
			* Use the specified title on the immediately 
			* following window and program.
			**************************************************/
			strncpy( window_title, &buffer[ 13 ],
				 STARTWIN_TITLE_MAX );
			window_title[ STARTWIN_TITLE_MAX ] = '\0';
			window_title_last  = 1;
			continue;
		}
		if ( strncmp( buffer, "window_blocked=", 15 ) == 0 )
		{
			/**************************************************
			* Block the specified window if it is not current.
			**************************************************/
			window = atoi( &buffer[ 15 ] ) - 1;
			if ( window < 0 )
				window = 9;
			if ( window < Wins )
				Window_blocked[ window ] = 1;
			set_window_blocked_mask( Window_blocked );
			continue;
		}
		b = buffer;
		respawn = 0;
		nostart = 0;
		if (  ( strncmp( buffer, "respawn ",  8 ) == 0 )
		   || ( strncmp( buffer, "respawn\t", 8 ) == 0 ) )
		{
			respawn = 1;
			b = &buffer[ 8 ];
		}
		else if (  ( strncmp( buffer, "nostart ",  8 ) == 0 )
			|| ( strncmp( buffer, "nostart\t", 8 ) == 0 ) )
		{
			nostart = 1;
			b = &buffer[ 8 ];
		}
		if (  (b[ 0 ] >= '0')
		   && (b[ 0 ] <= '9')
		   && ( (b[ 1 ] != ' ') || (b[ 1 ] != '\t') ) )
		{
			window = b[ 0 ] - '0' - 1;
			if ( window == -1 )
				window = 9;
			b = &b[ 2 ];
		}
		else if (  ( b[ 0 ] == 'L')
			&& ( (b[ 1 ] != ' ') || (b[ 1 ] != '\t') ) )
		{
			window = Wins - 1;
			b = &b[ 2 ];
		}
		else
		{
			if ( facet_echo_dotfacet )
			{
				term_outgo();
				fct_decode_string( 0,
					 "^^^^^ NOT THIS PASS ^^^^^\r\n" );
				term_outgo();
			}
			continue;
		}
		if ( window < Wins )
		{
			/**************************************************
			* Run the specified program on the specified 
			* window using an optional title.
			**************************************************/
			if ( window_title[ 0 ] != '\0' )
				window_title_to_use = window_title;
			else
				window_title_to_use = b;
			remember_exec_list( window, window_title_to_use );
			if ( nostart )
			{
				/* "Use the 'Activate' command to run: '" */
				fct_decode_string( window,
						Text_use_activate_to_run );
				fct_decode_string( window,
						window_title_to_use );
				fct_decode_string( window, "' \r\n" );
			}
			else
			{
				startwin( window, window_title, b );
			}
			if ( respawn )
			{
				Respawn_wanted[ window] = 1;
			}
			Respawn_program[ window] = strdup( b );
			Respawn_window_title[ window ] = 
						strdup( window_title );
		}
	}
	/******************************************************************
	* Close .facet file.
	******************************************************************/
	fclose( facetfile );
}
/**************************************************************************
* respawn
*	Restart the program specified in the .facet file.
**************************************************************************/
respawn( winno )
	int	winno;
{
	char	*window_title;
	char	*program;

	window_title = Respawn_window_title[ winno ];
	if ( window_title == (char *) 0 )
		return( -1 );
	program = Respawn_program[ winno ];
	if ( program == (char *) 0 )
		return( -1 );
	if ( window_title[ 0 ] != '\0' )
		remember_exec_list( winno, window_title );
	else
		remember_exec_list( winno, program );
	startwin( winno, window_title, program );
	return( 0 );
}
struct termio T_save;

/**************************************************************************
* termio_save
*	Store the current termio settings for subsequent restoration.
**************************************************************************/
termio_save()
{
	int	status;

	status = ioctl( 1, TCGETA, &T_save );
	if ( status < 0 )
	{
		perror( "ioctl TCGETA failed" );
		return( -1 );
	}
	return( 0 );
}
/**************************************************************************
* termio_restore
*	Restore the previously saved termio settings.
**************************************************************************/
termio_restore()
{
	int	status;

	status = ioctl( 1, TCSETAW, &T_save );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW normal failed" );
		return( -1 );
	}
	return( 0 );
}
/**************************************************************************
* set_user_id
*	Get the user name of the user and export it in FACETUSER for
*	use in setting the utmp file on open windows.
**************************************************************************/
#define F_USERID_LEN  40
char F_userid[ F_USERID_LEN + 1 ] = "";
char *Cuserid = (char *) 0;
set_user_id()
{
	char		*cuserid();
	char		*p_cuserid;
	char		*getlogin();

	if ( (p_cuserid = getlogin()) == NULL )
	{
		if ( (p_cuserid = cuserid( NULL )) == NULL )
			return;
	}
	strcpy( F_userid, "FACETUSER=" );
	Cuserid = &F_userid[ strlen( F_userid ) ];
	strcat( F_userid, p_cuserid );
	F_userid[ F_USERID_LEN ] = '\0';
	putenv( F_userid );
}


