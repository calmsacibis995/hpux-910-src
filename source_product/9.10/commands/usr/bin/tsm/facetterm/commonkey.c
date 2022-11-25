/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: commonkey.c,v 70.1 92/03/09 15:41:23 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

/**************************************************************************
* Receiver's main module and interface to drivers or pseudo ttys
**************************************************************************/

#include "ptydefs.h"

#include        <sys/types.h>
#include	<sys/param.h>
#include        "facetwin.h"
#include	"fproto.h"



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

extern int	hungup();

#include "facetkey.h"

			/**************************************************
			* The sender sends commands and/or returns control
			* by sending the current window number on the 
			* pipe "Pipe_to_recv".
			**************************************************/
int	Pipe_to_recv_read;
			/**************************************************
			* Keys that are read before they are needed or by
			* the wrong process are kept in the pipe "Pipe_keys"
			* which serves as a keyboard buffer for both 
			* processes.
			**************************************************/
int	Pipe_keys_read;
int	Pipe_keys_write;

int	Pid_send;			/* process id of the sender process */
			/**************************************************
			* Communcations to sender is done through the driver
			* or through a pipe on the "Pipe_control" fds.
			* On pseudo-tty systems we also have the fds
			* of the master side of the ptys
			**************************************************/
int	Pipe_control_read;
int	Pipe_control_write;
int	Fd_master[ USR_WINDS ];
int	Pipe_to_recv_write;


#include <setjmp.h>
jmp_buf	Env;

char	*Text_receiver_terminating =
	"\r\nFacet process: receiver terminating\r\n";

#include "facetpath.h"
/**************************************************************************
* main
*	Driver:
*		1 	fd of facet driver for ioctls
*		2 & 3	read and write fds of "Pipe_to_recv" pipe.
*		4 & 5	read and write fds of "Pipe_keys" pipe.
*	Pseudo tty:
*		1 & 2	read and write fds of "Pipe_to_recv" pipe.
*		3 & 4	read and write fds of "Pipe_keys" pipe.
*		5 & 6	read and write fds of "Pipe_control" pipe.
*		7 & up	master pty fds of windows 1 & up.
**************************************************************************/
main( argc, argv )
	int	argc;
	char	*argv[];
{
	char		filename[ 256 ];

	/******************************************************************
	* Set the location and naming variables for facetterm based on
	* environment variables.
	******************************************************************/
	set_options_facetpath();
	/******************************************************************
	* Load optional override of text strings.
	******************************************************************/
	sprintf( filename, "%stext", Facetname );
	load_foreign( filename );
#ifdef NZERO
	nice( -NZERO );
#else
							/* DG431 SUN41 */
	nice( -20 );
#endif
	Pid_send = getppid();
	/******************************************************************
	* Get the numbers of the file descriptors of files opened by
	* the sender.
	******************************************************************/
	if ( argc < 7 )
	{
		frecv_kill( "Facet process: receiver arg count %d", argc );
		/* does not return */
	}
	Pipe_to_recv_read = atoi( argv[ 1 ] );
	Pipe_to_recv_write = atoi( argv[ 2 ] );
	Pipe_keys_read = atoi( argv[ 3 ] );
	Pipe_keys_write = atoi( argv[ 4 ] );
	close( Pipe_to_recv_write );
	get_pseudo_fd( argc - 5, &argv[ 5 ] );

	if ( setjmp( Env ) == 0 )
	{
		/**********************************************************
		* Set up signal handling and execute receiver main code.
		**********************************************************/
			/* Catch the hangup signals on the tty  */
		if ( signal( SIGHUP, hungup ) == SIG_ERR )
		{
		    frecv_kill( "Facet process: receiver signal SIGHUP", -1 );
		    /* does not return */
		}
		if ( signal( SIGTERM, hungup ) == SIG_ERR )
		{
		    frecv_kill( "Facet process: receiver signal SIGTERM", -1 );
		    /* does not return */
		}
		if ( signal( SIGUSR2, hungup ) == SIG_ERR )
		{
		    frecv_kill( "Facet process: receiver signal SIGUSR2", -1 );
		    /* does not return */
		}
		frecv_recv();
	}
	else
	{
		/**********************************************************
		* longjmp says time to quit.
		**********************************************************/
			/* "\r\nFacet process: receiver terminating\r\n" */
		printf( Text_receiver_terminating );
		term_outgo();
	}
	exit( 0 );
	/* NOTREACHED */
}
/**************************************************************************
* hungup
*	Received a quit signal from tty, shutdown, or sender.
*	Output msg and quit.
**************************************************************************/
hungup()
{
	signal( SIGHUP, SIG_IGN );
	signal( SIGTERM, SIG_IGN );
	signal( SIGUSR2, SIG_IGN );
	longjmp( Env, 1 );
}
/**************************************************************************
* frecv_kill
*	Receiver encountered a fatal error.
*	Output msg, signal sender, and quit.
**************************************************************************/
frecv_kill( msg, code )
	char	msg[];
	int	code;
{
	term_outgo();
	if ( code == -1 )
	{
		printf( "\r\n" );
		perror( msg );
		printf( "\r" );
	}
	else
	{
		printf( msg, code );
		printf( "\r\n" );
	}
	term_outgo();
	kill( Pid_send, SIGUSR2 );
	longjmp( Env, 1 );
}
/**************************************************************************
* frecv_winputc
*	Put a character "c" to a window number "winno" 0-9 .
* frecv_winputs
*	Put "cnt" characters from string "s" to window number "winno" 0-9 .
* frecv_towins
*	Send the characters in the structure "towins" to the driver.
* frecv_toacks
*	Send the message "ackvalue" to the sender.
**************************************************************************/
frecv_winputc( winno, c )				/* PSEUDOTTY */
	int	c;
	int	winno;
{
	char	cc;

	cc = c;
	if ( write( Fd_master[ winno ], &cc, 1 ) < 0 )
	{
		frecv_kill( "Facet process: frecv_winputc", -1 );
		/* does not return */
	}
}
frecv_winputs( winno, s, cnt )				/* PSEUDOTTY */
	int	winno;
	char	*s;
	int	cnt;
{
	if ( write( Fd_master[ winno ], s, cnt ) < 0 )
	{
		frecv_kill( "Facet process: frecv_winputs", -1 );
		/* does not return */
	}
}
frecv_toacks( ackvalue )				/* PSEUDOTTY */
	int ackvalue;
{
	char	c[ 2 ];

	c[ 0 ] = ( ackvalue >> 8 ) & 0xFF;
	c[ 1 ] =   ackvalue        & 0xFF;
	if ( write( Pipe_control_write, c, 2 ) < 0 )
	{
		frecv_kill( "Facet process: frecv_toacks", -1 );
		/* does not return */
	}
}
/**************************************************************************
* frecv_break
*	Send break to window number "winno" 0-9 .
**************************************************************************/
							/* PSEUDOTTY */
#include <sys/ptyio.h>
frecv_break( winno )					/* PSEUDOTTY */
	int	winno;
{
	if ( ioctl( Fd_master[ winno ], TIOCBREAK, 0 ) < 0 )
	{
		perror( "TIOCBREAK" );
	}
}
/**************************************************************************
* term_outgo
*	Flush characters written to the terminal in standard out.
**************************************************************************/
term_outgo()
{
	fflush( (FILE *) stdout );
}
/**************************************************************************
* get_pseudo_fd
*	Store the file descriptors of the "Pipe_control" pipe which is
*	used to communicate with the sender, and the file descriptors
*	of the master side of the pseudo-ttys being used for the windows.
**************************************************************************/
get_pseudo_fd( argc, argv )				/* PSEUDOTTY */
	int	argc;
	char	**argv;
{
	int	i;

	Pipe_control_read = atoi( argv[ 0 ] );
	close( Pipe_control_read );
	Pipe_control_write = atoi( argv[ 1 ] );
	for ( i = 0; i < argc - 2; i++ )
	{
		Fd_master[ i ] = atoi( argv[ 2 + i ] );
	}
}
