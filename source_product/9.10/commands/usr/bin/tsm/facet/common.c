/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: common.c,v 70.1 92/03/09 15:39:35 ssa Exp $ */
/*---------------------------------------------------------------------*\
| common.c                                                              |
|                                                                       |
| Process level facet host code common to both                          |
| PC facet ( fproc ) and TERMINAL facet ( ftproc )                      |
\*---------------------------------------------------------------------*/

#include        <sys/types.h>
#include	"sys/param.h"
#include        "facetwin.h"
#include	"fproto.h"



#include	"wins.h"
int	Opt_nonstop = 0;

int	Opt_kill_processes_on_shutdown = 0;

#include	<stdio.h>

			/* fcntl needed for open */
#include        <fcntl.h>
			/* needed for signal */
#include        <signal.h>
			/* needed for error check */
#include        <errno.h>

                        /* TCGETA and TCSETAW */
#ifdef INTEL310
#include        <sys/ioctl.h>
#endif

#ifndef SIG_ERR
#define SIG_ERR         (int (*)()) -1
#endif

extern char     *ttyname();
extern int      errno;

extern int      shutdown_func();

#include "wincommon.h"


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
int	Inwin;				/* current input window */

int	Pid_recv;			/* process id of the receiver process */
unsigned	Facetline;		/* facet line number */

#include <setjmp.h>
jmp_buf	Env;

#include "facetpath.h"

						/* PSEUDOTTY */
#include "fd.h"
int	Fd_max = -1;
int	Fd_mask = 0;
int	Fd_masks[ USR_WINDS ] = { 0 };
int	Ack_mask = 0;
int	Fd_master[ USR_WINDS ];
int	Fd_blocked_mask = 0;
#include <sys/ptyio.h>
int	Trap_on = 1;
char	Slave_pty_name_store[ 10 ][ 15 ] = { "" };
char	*Slave_pty_name[ 10 ] =
{ 	Slave_pty_name_store[ 0 ], Slave_pty_name_store[ 1 ],
	Slave_pty_name_store[ 2 ], Slave_pty_name_store[ 3 ],
	Slave_pty_name_store[ 4 ], Slave_pty_name_store[ 5 ],
	Slave_pty_name_store[ 6 ], Slave_pty_name_store[ 7 ],
	Slave_pty_name_store[ 8 ], Slave_pty_name_store[ 9 ]
};
common()					/* PSEUDOTTY */
{
	char 	*ttyn;          /* name of the tty */
	int	stat;
	int	window;
	int	status;
	char	*getenv();

					/* make sure this is a tty */
	if ( (ttyn = ttyname( 0 )) == 0 )
	{
		printf(
		  "Facet process: Only tty ports support FACET mode.\n" );
		printf(
		  "               Stdin is not a tty.\n" );
		exit( 1 );
	}
	if ( getenv( "RECURSIVE" ) == NULL )
	{
		if ( iswindow() )
				exit( 0 );
	}

	header();
	/******************************************************************
	** Select the facet line and windows.
	******************************************************************/
	status = open_facet_line();
	if ( status != 0 )
	{
		printf(
			"Facet process: All Facet lines are busy. (%d)\r\n",
			Facetline );
		term_outgo();
		wait_return_pressed();
		exit( 1 );
	}
#ifdef NOT_FACET_PC
	 putenv_localprinter( Slave_pty_path );
#endif
			/* save current crt status */
	if ( termio_save() )
		exit( 1 );
			/* set stdin and stdout to raw mode */
	if ( setcrt() )
		exit( 1 );

	Pid_recv = getpid();
	set_user_id();
	set_owner_on_windows();
	/* fork off the send process */
	if ( (Pid_send = fork()) == -1 )
	{
		printf( "Facet process: Can't fork, error %d\n", errno );
		perror( "             " );
		term_outgo();
		termio_restore();
		exit( 1 );
	}

	if ( Pid_send )
	{               /* parent - receiver */
		nice( -NZERO );
		if ( setjmp( Env ) == 0 )
		{
				/* Catch the hangup signals on the tty  */
			if ( signal( SIGHUP, shutdown_func ) == SIG_ERR )
			{
			    frecv_kill( 
				"Facet process: receiver signal SIGHUP", -1 );
			    /* does not return */
			}
			if ( signal( SIGTERM, shutdown_func ) == SIG_ERR )
			{
			    frecv_kill( 
				"Facet process: receiver signal SIGTERM", -1 );
			    /* does not return */
			}
			if ( signal( SIGUSR2, shutdown_func ) == SIG_ERR )
			{
			    frecv_kill( 
				"Facet process: receiver signal SIGUSR2", -1 );
			    /* does not return */
			}
			Inwin = 0;		/* default to window 0 */
			frecv_recv();
		}
		else
		{
			kill( Pid_send, SIGUSR2 );
			wait( &stat );
			termio_restore();
			set_utmp_idle_all();
			reset_owner_on_windows();
			close_windows();
		}
	}
	else
	{               /* child - sender */
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
			printf( "Facet process: activating\r\n" );
			term_outgo();
			fsend_send();
		}
		else
		{
			fsend_finish();
			term_outgo();
			printf( "\r\nFacet process: sender terminating\r\n" );
			term_outgo();
			termio_restore();
		}
	}
	exit( 0 );
/* NOTREACHED */
}
shutdown_func()
{
	signal( SIGHUP, SIG_IGN );
	signal( SIGTERM, SIG_IGN );
	signal( SIGUSR2, SIG_IGN );
	longjmp( Env, 1 );
}
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
	kill( Pid_recv, SIGHUP );
	longjmp( Env, 1 );
}
frecv_kill( msg, code )
	char	msg[];
	int	code;
{
	if ( code == -1 )
	{
		perror( msg );
		printf( "\r" );
	}
	else
		printf( msg, code );
	term_outgo();
	longjmp( Env, 1 );
}

char	*Text_receiver_wait_close =
"Facet process: receiver waiting for windows to close...\n";

char	*Text_facet_attempt_kill =
"Facet process: attempting to kill processes that would not hang up...\n";

char	*Text_receiver_term_wait_close =
"Facet process: receiver terminating - waiting for windows to close...\n";

char	*Text_receiver_term =
"Facet process: receiver terminating...\n";

char	*Text_facet_attempt_list=
"Facet process: attempting to list processes that would not hang up...\n";

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
/* "Facet process: receiver waiting for windows to close...\n" */
		printf( Text_receiver_wait_close );
		fflush( stdout );
		result = fsend_offcarrier();
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
			system( command );
/* "Facet process: receiver terminating - waiting for windows to close...\n" */
			printf( Text_receiver_term_wait_close );
			fflush( stdout );
			return;
		}
	}
	/* "Facet process: receiver terminating...\n" */
	printf( Text_receiver_term );
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
/*---------------------------------------------------------------------*\
| fsend_send                                                            |
|                                                                       |
| Performs send process for Facet
\*---------------------------------------------------------------------*/
#include "restart.h"
#include "facetterm.h"
fsend_send() 
{

	fsend_init();
			/* turn carrier on for all windows */
	fsend_oncarrier();

	run_facet();

	signal( SIGINT, SIG_IGN );
			/* multiplex all window output onto real line */
	fsend_process_windows();
	/* does not return */
}
/******************************************************************
******************************************************************/
/******************************************************************
******************************************************************/
							/* PSEUDOTTY */
fsend_oncarrier()		/* turn on carrier for all windows on line */
{
	/* not used in PSEUDOTTY */
}
/******************************************************************
******************************************************************/
							/* PSEUDOTTY */
fsend_offcarrier()		/* turn off carrier for all windows on line */
				/* call only when shutdown in progress and
				   line is not in raw mode.
				*/
{
	int	result;			/* 0  = all closed */
					/* 1  = some windows still open */
					/* -1 = error */

	fsend_hangup();
	result = fsend_wait_windows_close( Kill_seconds );
	fsend_kill_signal();
	return( result );
}
/******************************************************************
******************************************************************/
							/* PSEUDOTTY */
#include "facetterm.h"
fsend_get_acks_only()					/* PSEUDOTTY */
{
	int ackchars[ 1 ];
			/* process only acks from receiver */
	term_outgo();
						/* get acks only, 
						   no window chars
						   and/or packet */
	
	read_control_pipe( ackchars );
						/* process acks */
	fsend_procack_array( 1, ackchars );
}
#include "readwindow.h"
int	Read_window_max[ TOPWIN ] = 
{
	READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX,
	READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX, READ_WINDOW_MAX,
	READ_WINDOW_MAX, READ_WINDOW_MAX
};
int Transparent_print_read_window_max = READ_WINDOW_MAX;
int Fd_print_mask = 0;

int Transparent_print_timer = 0;

#include "printtimer.h"
int Transparent_print_quiet_timer = 0;
int Transparent_print_idle_timer = 0;

#include <time.h>

fsend_process_windows() /* PSEUDOTTY */
{
	int	winno;
	int	iowin;
	int	count;
	int	readfds;
	int	writefds;
	int	exceptfds;
	int	status;
	int	blocked;

			/* multiplex all window output onto real line */
	while (1) 
	{
		term_outgo();
		if ( Saw_restart )
		{
			Saw_restart = 0;
			restart();
			term_outgo();
		}
		blocked = Fd_blocked_mask & ( ~ Fd_masks[ Inwin ] );
		readfds = Ack_mask | ( Fd_mask & ( ~blocked ) );
		writefds = 0;
		exceptfds = Fd_mask;
		status = 0;
		if ( Fd_print_mask && ( Transparent_print_timer > 0 ) )
		{
			int		printreadfds;
			struct timeval	timeout;

			printreadfds = readfds & ( ~Fd_print_mask );
			timeout.tv_sec = Transparent_print_timer / 1000;
			timeout.tv_usec = Transparent_print_timer % 1000;
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
					Transparent_print_idle_timer;
			else
			{
				readfds = printreadfds;
				Transparent_print_timer = 
					Transparent_print_quiet_timer;
			}
		}
		if ( status == 0 )
		{
			status = select( Fd_max +1, 
					 &readfds, &writefds, &exceptfds,
					 (char *) 0 );
			if ( status < 0 )
			{
				fsend_kill( "Facet process: select", -1 );
				/* does not return */
			}
			if ( Fd_print_mask )
				if ( ( readfds & Fd_print_mask ) == 0 )
					Fd_print_mask = 0;
		}
		if ( readfds & Ack_mask )
		{
						/* process acks */
			fsend_get_acks_only();
			continue;
		}
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
set_print_active( winno )
	int	winno;
{
	if ( Fd_print_mask == 0 )
		Transparent_print_timer = Transparent_print_quiet_timer;
	Fd_print_mask = Fd_masks[ winno ];
}
set_window_blocked_mask( window_blocked )		/* PSEUDOTTY */
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
							/* PSEUDOTTY */
exception_window( winno, fd )
	int	winno;
	int	fd;
{
	struct request_info req;
	int	request;
	char	fioc_buffer[ FIOC_BUFFER_SIZE ];
	char	fioc_buffer_out[ FIOC_BUFFER_SIZE ];
	int	status;

	status = ioctl( Fd_master[ winno ], TIOCREQCHECK, &req );
	if ( status < 0 )
	{
		perror( "TIOCREQCHECK" );
		return;
	}
	request = req.request & IOCCMD_MASK;
	if ( req.request == TIOCOPEN )
	{
		if ( check_window_active( winno ) == 0 )
		{
			termio_normal_master( fd );
			window_first_open( winno );
			pc_notify( FIRST_OPEN, winno );
		}
	}
	else if ( req.request == TIOCCLOSE )
	{
		window_closed( winno );
		set_utmp_idle( winno );
		set_owner_on_window( winno );
#ifdef NOT_PC_FACET
		/* window_idle_actions( winno ); */
#endif
		pc_notify( LAST_CLOSE, winno );
	}
	else if ( ( request >> 8 ) == 'M' )
	{
	}
	else if ( ( request >> 8 ) == 'K' )
	{
	}
	else if ( req.request == FIOC_CURWIN )
	{
		get_curwin_string( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_CURWIN" );
	}
#ifdef NOT_PC_FACET
	else if ( req.request == FIOC_WINDOW_MODE )
	{
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_WINDOW_MODE" );
		else
			process_window_mode_commands( fioc_buffer, winno );
	}
#endif
	else if ( req.request == FIOC_GET_PROGRAM_NAME )
	{
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
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_EXEC_LIST" );
		else
		{
			remember_exec_list( winno, fioc_buffer );
			pc_message( EXEC_LIST, winno, fioc_buffer );
		}
	}
	else if ( req.request == FIOC_ACTIVE )
	{
		build_active_list( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_ACTIVE set" );
	}
	else if ( req.request == FIOC_IDLE )
	{
		build_idle_list( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_IDLE set" );
	}
#ifdef NOT_PC_FACET
	else if ( req.request == FIOC_PUSH_POPUP_SCREEN )
	{
		req.errno_error = do_push_popup_screen( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_PUSH_POPUP_SCREEN set" );
	}
	else if ( req.request == FIOC_POP_POPUP_SCREEN )
	{
		req.errno_error = do_pop_popup_screen( fioc_buffer );
		if ( ioctl( fd, req.argset, fioc_buffer ) < 0 )
			perror( "FIOC_POP_POPUP_SCREEN set" );
	}
#endif
	else if ( req.request == FIOC_GET_INFORMATION )
	{
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
#ifdef NOT_PC_FACET
	else if ( req.request == FIOC_PASTE )
	{
		if ( ioctl( fd, req.argget, fioc_buffer ) < 0 )
			perror( "FIOC_WINDOW_MODE" );
		else
			process_ioctl_paste( fioc_buffer, winno );
	}
#endif
	else if ( req.request == FIOC_UTMP )
	{
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
	status = ioctl( fd, TIOCREQSET, &req );
	if ( status < 0 )
	{
		perror( "TIOCREQSET" );
	}
}
pc_notify( cmd, winno )
	int	cmd;
	int	winno;
{
	struct facet_packet	notify_pkt;
	int			result;

	notify_pkt.pkt_header.cmd_byte = cmd & 0xFF;
	notify_pkt.pkt_header.pkt_window = winno;
	notify_pkt.pkt_header.arg_cnt = 1;
	notify_pkt.pkt_args[ 0 ] = winno + 1;
	result = fsend_packsend( winno, &notify_pkt );
}
pc_message( cmd, winno, message )
	int	cmd;
	int	winno;
{
	struct facet_packet	msg_pkt;
	int			result;

	msg_pkt.pkt_header.cmd_byte = cmd & 0xFF;
	msg_pkt.pkt_header.pkt_window = winno;
	strncpy( msg_pkt.pkt_args, message, MAXPROTOARGS );
	msg_pkt.pkt_args[ MAXPROTOARGS - 1 ] = 0;
	msg_pkt.pkt_header.arg_cnt = strlen( msg_pkt.pkt_args )+1;
	result = fsend_packsend( winno, &msg_pkt );
}
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
fsend_wait_windows_close( seconds ) /* PSEUDOTTY */
	int	seconds;
{
	int	winno;
	int	iowin;
	int	count;
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
	start_time = time( (char *) 0 );
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	while (1) 
	{
		if ( ( time( (char *) 0 ) - start_time ) > ( seconds -1 ) )
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
				read_wait_windows_close( winno,
							Fd_master[ winno ] );
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
							/* PSEUDOTTY */
exception_wait_windows_close( winno, fd )
	int	winno;
	int	fd;
{
	struct request_info req;
	int	request;
	char	fioc_buffer[ FIOC_BUFFER_SIZE ];
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
		perror( "TIOCREQSET" );
	}
	return( all_closed );
}
							/* PSEUDOTTY */
read_wait_windows_close( winno, fd )
	int	winno;
	int	fd;
{
	int	count;
	char	window_out_buff[ READ_WINDOW_MAX ];

	count = read( fd, window_out_buff, READ_WINDOW_MAX );
}
							/* PSEUDOTTY */
int	Using_pty_directories = 0;
char	*Master_pty_dir = NULL;
char	*Slave_pty_dir = NULL;
set_pty_dirs()
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
open_facet_line()					/* PSEUDOTTY */
{
	int	i;
	int	fd;
	int	pty_start;
	char	master_pty_path[ 80 ];

	if ( Opt_nonstop == 0 )
		printf( "Using ptys: " );
	pty_start = 0;
	set_pty_dirs();
	for ( i = 0; i < Wins; i++ )
	{
		fd = open_master( &pty_start, master_pty_path,
				  Slave_pty_path[ i ], Slave_pty_name[ i ] );
		if  ( fd < 0 )
		{
			if ( Opt_nonstop == 0 )
				printf( "\n" );
			return( -1 );
		}
		if ( Opt_nonstop == 0 )
			printf( " %s", Slave_pty_name[ i ] );
		Fd_master[ i ] = fd;
		if ( Fd_master[ i ] > Fd_max )
			Fd_max = Fd_master[ i ];
		Fd_masks[ i ] = ( 1 << Fd_master[ i ] );
		Fd_mask |= Fd_masks[ i ];
	}
	if ( Opt_nonstop == 0 )
		printf( "\n" );
	return( 0 );
}
						/* PSEUDOTTY */
open_master( p_pty_start, master_pty_path, slave_pty_path, slave_pty_name )
	int	*p_pty_start;
	char	*master_pty_path;
	char	*slave_pty_path;
	char	*slave_pty_name;
{
	int	fd;
	int	pty_number;

	pty_number = *p_pty_start;
	while ( 1 )
	{
	    while ( 1 )
	    {
		pty_number++;
		if ( pty_number == *p_pty_start )
			break;
		if ( get_pty_names( pty_number, master_pty_path,
				    slave_pty_path, slave_pty_name ) < 0 )
		{
			break;
		}
		if ( (fd = open( master_pty_path, O_RDWR )) >= 0 )
		{
			if ( fcntl( fd, F_SETFL,
					    fcntl( fd, F_GETFL, 0 ) | O_NDELAY )
			     < 0 )
			{
				perror( "on F_SETFL" );
				close( fd );
				continue;
			}
			if ( access( slave_pty_path, 0 ) < 0 )
			{
				close( fd );
				continue;
			}
			if ( ioctl( fd, TIOCTRAP, &Trap_on ) < 0 )
			{
				perror( "TIOCTRAP" );
				close( fd );
				continue;
			}
			*p_pty_start = pty_number;
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
	    if ( pty_number > *p_pty_start )
		pty_number = -1;
	    else
		break;
	}
	return( -1 );
}
							/* PSEUDOTTY */
get_pty_names( pty_number, master_pty_path, slave_pty_path, slave_pty_name )
	int	pty_number;
	char	*master_pty_path;
	char	*slave_pty_path;
	char	*slave_pty_name;
{
	static	char	*master_pty_base[] = 
	{
		"ptyp", "ptyq", "ptyr", "ptys", "ptyt", "ptyu",
		"ptyv", "ptyw", "ptyx", "ptyy", "ptyz"
	};
	static	char	*slave_pty_base[] = 
	{
		"ttyp", "ttyq", "ttyr", "ttys", "ttyt", "ttyu",
		"ttyv", "ttyw", "ttyx", "ttyy", "ttyz"
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

	base = pty_number / num_pty_ending;
	if ( base >= num_base )
		return( -1 );
	ending = pty_number % num_pty_ending;
	sprintf( slave_pty_name, "%s%s",
				slave_pty_base[ base ],  pty_ending[ ending ] );
	/******************************************************************
	* Allow for possibility that ptys are in /dev directory even
	* though there is a pty directory.
	* The function "ttyname" gives preference to these, so we must
	* also.
	******************************************************************/
	sprintf( master_pty_path, "%s%s%s",
		"/dev/",	master_pty_base[ base ], pty_ending[ ending ] );
	sprintf( slave_pty_path, "%s%s%s",
		"/dev/",	slave_pty_base[ base ],  pty_ending[ ending ] );
	if ( access( master_pty_path, 0 ) == 0 )
		return( 0 );
	if ( access( slave_pty_path, 0 ) == 0 )
		return( 0 );
	/******************************************************************
	* Pty is in sub directory or is missing.
	******************************************************************/
	sprintf( master_pty_path, "%s%s%s",
		Master_pty_dir,	master_pty_base[ base ], pty_ending[ ending ] );
	sprintf( slave_pty_path, "%s%s%s",
		Slave_pty_dir,	slave_pty_base[ base ],  pty_ending[ ending ] );
	return( 0 );
}
							/* PSEUDOTTY */
fsend_hangup()		/* send hangup to all windows on line */
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
							/* PSEUDOTTY */
fsend_kill_signal()		/* send kill to all windows on line */
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
/* ======================================================================== */
run_facet()
{
	FILE	*facetfile;
	char	filename[ 256 ];
	char	buffer[ STARTWIN_COMMAND_MAX + 1 ];
	int	window;
	int	len;
	char	*home;
	char	*getenv();
	int	seconds;
	char	window_title[ STARTWIN_TITLE_MAX + 1 ];
	int	window_title_last;

	window_title_last = 0;
	strcpy( window_title, "" );
	sprintf( filename, ".%s", Facetname );
	facetfile = fopen( filename, "r" );
	if ( facetfile == NULL )
	{
		home = getenv( "HOME" );
		if ( home != NULL )
		{
			strcpy( filename, home );
			strcat( filename, "/" );
			strcat( filename, "." );
			strcat( filename, Facetname );
			facetfile = fopen( filename, "r" );
		}
	}
	if ( facetfile == NULL )
	{
		sprintf( filename, "%s/.%s", Facetpath, Facetname );
		facetfile = fopen( filename, "r" );
	}
	if ( facetfile == NULL )
		return;
	while ( fgets( buffer, STARTWIN_COMMAND_MAX, facetfile ) != NULL )
	{
		len = strlen( buffer );
		if ( (len < 3) || (buffer[ 0 ] == '#') )
			continue;
		if ( buffer[ len-1 ] == '\n' )
			buffer[ len-1 ] = '\0';
		if ( window_title_last == 0 )
			strcpy( window_title, "" );
		else
			window_title_last = 0;
		if ( strncmp( buffer, "kill_seconds=", 13 ) == 0 )
		{
			seconds = atoi( &buffer[ 13 ] );
			if ( seconds > 0 )
				fsend_kill_seconds( seconds );
			continue;
		}
		if ( strncmp( buffer, "window_title=", 13 ) == 0 )
		{
			strncpy( window_title, &buffer[ 13 ],
				 STARTWIN_TITLE_MAX );
			window_title[ STARTWIN_TITLE_MAX ] = '\0';
			window_title_last  = 1;
			continue;
		}
		if (  (buffer[ 0 ] < '0')
		   || (buffer[ 0 ] > '9')
		   || ( (buffer[ 1 ] != ' ') && (buffer[ 1 ] != '\t') ) )
			continue;
		window = buffer[ 0 ] - '0' - 1;
		if ( window == -1 )
			window = 9;
		startwin( window, window_title, &buffer[ 2 ] );
	}
	fclose( facetfile );
}
#include <termio.h>
struct termio T_save;

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
	strcpy( F_userid, "FACETUSER" );
	strcat( F_userid, "=" );
	Cuserid = &F_userid[ strlen( F_userid ) ];
	strcat( F_userid, p_cuserid );
	F_userid[ F_USERID_LEN ] = '\0';
	putenv( F_userid );
}
/* ========================================================================= */
frecv_winputc( winno, c )		/* put a character to a window */
	int	c;
	int	winno;
{
	char	cc;
	int	status;

	cc = c;
	if ( (status = write( Fd_master[ winno ], &cc, 1 )) < 0 )
	{
		frecv_kill( "Facet process: frecv_winputc", -1 );
		/* does not return */
	}
}
frecv_winputs( winno, s, cnt )		/* put a string to a window */
	int		winno;
	unsigned char	*s;
	int		cnt;
{
	int	status;

	if ( (status = write( Fd_master[ winno ], s, cnt )) < 0 )
	{
		frecv_kill( "Facet process: frecv_winputs", -1 );
		/* does not return */
	}
}
frecv_toacks( ackvalue )
	int ackvalue;
{
	write_control_pipe( ackvalue );
}
/*---------------------------------------------------------------------*\
| frecv_winrcvd                                                         |
|                                                                       |
| process new window selection received on the line                     |
\*---------------------------------------------------------------------*/
frecv_winrcvd( newwindow )
	int newwindow;
{
	if (newwindow < USR_WINDS )
	{
		Inwin =  newwindow;
#ifdef PCCOMMANDACK
		frecv_toacks( FP_WINSEL | newwindow );
#endif
	}
}
frecv_refresh()
{
	frecv_winrcvd( Inwin );
}
