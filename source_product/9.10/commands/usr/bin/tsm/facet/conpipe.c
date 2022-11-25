/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: conpipe.c,v 70.1 92/03/09 15:39:40 ssa Exp $ */
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
extern int errno;

#define READ_PIPE 0
#define WRITE_PIPE 1

int Pipe_control[ 2 ];
#include "fd.h"

init_communications()
{
	int	flags;

	if ( pipe( Pipe_control ) == -1 )
	{
		perror( "Error opening pipe for control" );
		exit( 1 );
	}
	if ( Pipe_control[ READ_PIPE ] > Fd_max )
		Fd_max = Pipe_control[ READ_PIPE ];
	Ack_mask = ( 1 << Pipe_control[ READ_PIPE ] );
}
fsend_init_communications()
{
	close( Pipe_control[ WRITE_PIPE ] );
}
frecv_init_communications()
{
	close( Pipe_control[ READ_PIPE ] );
}
read_control_pipe( ackchars )				/* PSEUDOTTY */
	int	ackchars[];
{
	int		status;
	int		ackcount;
	char		c[ 2 ];

	ackcount = 1;
	status = read( Pipe_control[ READ_PIPE ], c, ackcount * 2 );
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
write_control_pipe( ackvalue )
	int	ackvalue;
{
	char	c[ 2 ];
	int	status;

	c[ 0 ] = ( ackvalue >> 8 ) & 0xFF;
	c[ 1 ] =   ackvalue        & 0xFF;
	if ( (status=write( Pipe_control[ WRITE_PIPE ], c, 2 )) < 0 )
	{
		frecv_kill( "Facet process: frecv_toacks", -1 );
		/* does not return */
	}
}
							/* PSEUDOTTY */
unsigned Ack_value = 0;
/*---------------------------------------------------------------------*\
| fsend_ackreset                                                        |
|                                                                       |
| Allow a ack to be received.  Must be done before fsend_ackwait.       |
\*---------------------------------------------------------------------*/
fsend_ackreset()					/* PSEUDOTTY */
{
	int i;

	/* read and process acks only - discarding received */
	while( if_acks_present( 0 ) )
	{
		fsend_get_acks_only();
	}
	Ack_value = 0;
}
/*---------------------------------------------------------------------*\
| fsend_ackwait                                                         |
|                                                                       |
| wait for "seconds" seconds for an acknowledgement to be received.     |
| Return the value of the ack or -1 if it times out.                    |
\*---------------------------------------------------------------------*/

fsend_ackwait( seconds )				/* PSEUDOTTY */
	int seconds;
{
	int	 i;

	while ( Ack_value == 0 )
	{
		/* read acks only - process send - timeout if no received */
		if ( if_acks_present( seconds ) == 0 )
		{
			Ack_value = -1;
			break;
		}
		fsend_get_acks_only();
	}
	return( Ack_value );
}
/*---------------------------------------------------------------------*\
| fsend_ackrcvd                                                         |
|                                                                       |
| Post an ack that was received.  In order to be valid, an ack must be  |
| be received while the timer is running.  It is ignored otherwise.     |
| Storing the ack cancels the timer.  Wakeup anyone waiting on the ack. |
\*---------------------------------------------------------------------*/
fsend_ackrcvd( ackvalue )					/* PSEUDOTTY */
	unsigned	ackvalue;
{
	if ( Ack_value == 0 )
		Ack_value = ackvalue;
}
#include <time.h>
if_acks_present( seconds ) /* PSEUDOTTY */
	int	seconds;
{
	int	readfds;
	int	writefds;
	int	exceptfds;
	int	status;
	struct timeval	timeout;

	readfds = Ack_mask;
	writefds = 0;
	exceptfds = 0;
	status = 0;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	status = select( Fd_max +1, 
			 &readfds, &writefds, &exceptfds,
			 &timeout );
	if ( status < 0 )
	{
		fsend_kill( "Facet process: select T", -1 );
		/* does not return */
	}
	if ( status == 0 )
		return( 0 );
	return( 1 );
}
