/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: talksend.c,v 70.1 92/03/09 15:47:28 ssa Exp $ */

#include "ptydefs.h"

#include "ftproc.h"
#include "talk.h"
#include "wins.h"
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
extern int errno;

#define READ_PIPE 0
#define WRITE_PIPE 1

		/**********************************************************
		* Pipe from sender to receiver.
		* Receiver waits on this pipe after giving control of the
		* keyboard to the sender with an "ack" through either the
		* driver or the "contro pipe" used on pseudo-tty systems.
		* The primary message is the current window number which
		* signals go ahead. This message optionally indicates that
		* the keyboard is in scan code mode.
		* Other messages include paste characters, keystroke replay
		* filenames, etc.  This is the primary path for
		* communication to the receiver after the fork.
		**********************************************************/
int Pipe_to_recv[ 2 ];
		/**********************************************************
		* Pipe written to by the receiver and read by both the
		* sender and receiver.
		* This pipe is used a keyboard buffer for keys
		* that have been read prematurely.
		* The receiver reads multiple keys in an attempt to keep
		* all keys of a function key together.
		* The sender may catch keyboard keys unintentionally
		* while waiting for a printer status.
		**********************************************************/
int Pipe_keys[ 2 ];
							/* PSEUDOTTY */
						/* HP9000 AIX SYS54 */
		/**********************************************************
		* Pipe written by the receiver and read by the sender.
		* This pipe replaces the "ack" communications path
		* provided by the driver when pseudo ttys are used.
		* The receiver sends a message to the sender when a
		* hot-key is presses or when it is signaled by the
		* sender that the sender needs control of the keyboard.
		**********************************************************/
int Pipe_control[ 2 ];

						/* HP9000 AIX */
#include "fd.h"
						/* HP9000 AIX */

						/* HP9000 AIX SYS54 */


/**************************************************************************
* init_communications
*	Set up the pipes for communication between the sender and 
*	receiver.  This routine is called before the fork.
**************************************************************************/
init_communications()
{
	int	flags;

	if ( pipe( Pipe_to_recv ) == -1 )
	{
		perror( "Error opening pipe to recv" );
		exit( 1 );
	}
	if ( pipe( Pipe_keys ) == -1 )
	{
		perror( "Error opening pipe for keys" );
		exit( 1 );
	}
	if ( (flags = fcntl( Pipe_keys[ READ_PIPE ], F_GETFL, 0 )) == -1 )
	{
		perror( "Error on F_GETFL for pipe for keys" );
		exit( 1 );
	}
	if ( fcntl( Pipe_keys[ READ_PIPE ], F_SETFL, flags | O_NDELAY ) == -1 )
	{
		perror( "Error on F_SETFL for pipe for keys" );
		exit( 1 );
	}
						/* HP9000 AIX SYS54 */
	if ( pipe( Pipe_control ) == -1 )
	{
		perror( "Error opening pipe for control" );
		exit( 1 );
	}
						/* HP9000 AIX SYS54 */
						/* HP9000 AIX */
	if ( Pipe_control[ READ_PIPE ] > Fd_max )
		Fd_max = Pipe_control[ READ_PIPE ];
	Ack_mask = ( 1 << Pipe_control[ READ_PIPE ] );
						/* USE_PIPE_FOR_CONTROL */
}
/**************************************************************************
* start_receiver
*	top of receiver process - called immediately after fork.
*	Exec the program fct_key - passing the file descriptor numbers
*	of the relevant files that are open.
**************************************************************************/
#include "facetpath.h"
start_receiver( fd_master )				/* PSEUDOTTY */
	int	fd_master[];
{
	int	i;
	char	Pipe_control_read_str[ 10 ];
	char	Pipe_control_write_str[ 10 ];
	char	fd_master_str[ 10 ][ 10 ];
	int	argc;
	char	*argv[ 20 ];
	char	Pipe_to_recv_read_str[ 10 ];
	char	Pipe_to_recv_write_str[ 10 ];
	char	Pipe_keys_read_str[ 10 ];
	char	Pipe_keys_write_str[ 10 ];
	char	facettkey_name[ 20 ];
	char	facettkey_path[ 256 ];
	char	*getenv();

	sprintf( facettkey_name,"%skey", Facetprefix );
	if ( getenv( "FACETTEST" ) != (char *) 0 )
		sprintf( facettkey_path, "./%skey", Facetprefix );
	else
		sprintf( facettkey_path, "%s/sys/%skey", 
					 Facettermpath, Facetprefix );

	sprintf( Pipe_to_recv_read_str, "%d", Pipe_to_recv[ READ_PIPE ] );
	sprintf( Pipe_to_recv_write_str, "%d", Pipe_to_recv[ WRITE_PIPE ] );
	sprintf( Pipe_keys_read_str, "%d", Pipe_keys[ READ_PIPE ] );
	sprintf( Pipe_keys_write_str, "%d", Pipe_keys[ WRITE_PIPE ] );

						/* HP9000 AIX SYS54 */
	sprintf( Pipe_control_read_str, "%d", Pipe_control[ READ_PIPE ] );
	sprintf( Pipe_control_write_str, "%d", Pipe_control[ WRITE_PIPE ] );
						/* HP9000 AIX SYS54 */


	argc = 0;
	argv[ argc++ ] = facettkey_name;
	argv[ argc++ ] = Pipe_to_recv_read_str;
	argv[ argc++ ] = Pipe_to_recv_write_str;
	argv[ argc++ ] = Pipe_keys_read_str;
	argv[ argc++ ] = Pipe_keys_write_str; 
	argv[ argc++ ] = Pipe_control_read_str;
	argv[ argc++ ] = Pipe_control_write_str;
	for ( i = 0; i < Wins; i++ )
	{
		sprintf( fd_master_str[ i ], "%d", fd_master[ i ] );
		argv[ argc++ ] = fd_master_str[ i ];
	}
	argv[ argc ] = (char *) 0;
	execv( facettkey_path, argv );

	perror( facettkey_path );
	printf( "\r" );
	term_outgo();
	kill( Pid_send, SIGUSR2 );
	exit( 1 );
}
/***************************** SENDER *******************************/
/**************************************************************************
* fsend_init_communications
*	Close the ends of the pipes that are unnecessary in the sender
*	after the receiver has been forked.
**************************************************************************/
fsend_init_communications()
{
	close( Pipe_to_recv[ READ_PIPE ] );
	close( Pipe_keys[ WRITE_PIPE ] );


						/* HP9000 AIX SYS54 */
	close( Pipe_control[ WRITE_PIPE ] );
						/* HP9000 AIX SYS54 */

						/* PSEUDOTTY */

}
/**************************************************************************
* send_packet_signal
*	Sender signal to the receiver that the receiver needs control
*	of the keyboard and/or terminal.
*	Receiver will respond with FTPROC_WINDOW_PACKET through the 
*	driver or Pipe_control.
**************************************************************************/
send_packet_signal()
{
	if ( kill( Pid_recv, SIGUSR1 ) < 0 )
	{
		fsend_kill( "Can't signal receiver", -1 );
		/* does not return */
	}
}
/**************************************************************************
* fct_window_mode_ans
*	Send the character "ans" from the sender to the receiver on the
*	"Pipe_to_recv" pipe.
*	The receiver reads this pipe after receiving a "send_packet_signal"
*	or the user pressed a hot-key.
**************************************************************************/
fct_window_mode_ans( ans )
	char	ans;
{
	if ( write( Pipe_to_recv[ WRITE_PIPE ], &ans, 1 ) != 1 )
	{
		fsend_kill( "Can't talk to receiver", -1 );
		/* does not return */
	}
}
/**************************************************************************
* flush_keys_from_recv
*	Empty the "keyboard buffer" pipe.
**************************************************************************/
flush_keys_from_recv()
{
	while( chk_keys_from_recv() != -1 )
		;
}
/**************************************************************************
* chk_keys_from_recv
*	Check for the presence of keys in the "keyboard buffer" pipe
*	and return the first one as an integer if present.
*	Return -1 if empty.
**************************************************************************/
chk_keys_from_recv()	/* -1 = NONE */
{
	int	status;
	char	c;

	status = read( Pipe_keys[ READ_PIPE ], &c, 1 );
	if ( status == 1 )
		return( ( (int) c ) & 0xFF );
	else if ( status == 0 )
		return( -1 );
	else if ( ( status == -1 ) && ( errno == EAGAIN ) )
		return( -1 );
	else
	{
		fsend_kill( "Can't hear sender", -1 );
		/* does not return */
	}
	/* NOTREACHED */
}
/**************************************************************************
* read_control_pipe
*	Read the pseudo-tty system's pipe that replaces the "ack" channel
*	on the driver systems.
*	This pipe contains integer messages from talk.h that are responses
*	to "send_packet_signal" or hot-keys.
*	The results are returned in the array "ackchars".  A maximum of
*	one message is ever returned.
*	The number of messages in "ackchars" ( 0 or 1 ) is returned.
**************************************************************************/
read_control_pipe( ackchars )				/* PSEUDOTTY */
	unsigned int	ackchars[];
{
	int		status;
	int		ackcount;
	char		c[ 2 ];

	ackcount = 1;
						/* HP9000 AIX SYS54 */
	status = read( Pipe_control[ READ_PIPE ], c, (int) ( ackcount * 2 ) );
						/* HP9000 AIX SYS54 */
	if ( status == ackcount * 2 )
	{
		ackchars[ 0 ] = (( ((int) c[ 0 ]) & 0x00FF ) << 8) | 
				 ( ((int) c[ 1 ]) & 0x00FF );
		return( ackcount );
	}
	else if ( status >= 0 )
	{
		fsend_kill( "Can't read sender control pipe %d", status );
		/* does not return */
	}
	else
	{
		fsend_kill( "Can't read sender control pipe", -1 );
		/* does not return */
	}
	/* NOTREACHED */
}
