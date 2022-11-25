/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: facet.c,v 70.1 92/03/09 15:39:46 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/*---------------------------------------------------------------------*\
| facet.c                                                               |
|                                                                       |
| Facet send and receive processes for invoking the task time portion   |
| of Facet protocol processing.                                         |
\*---------------------------------------------------------------------*/

#include        <sys/types.h>
#include        "facetwin.h"
#include	"fproto.h"



#include	<stdio.h>

#include	"facet.h"

                        /* needed for setcrt */
#include        <termio.h>

#ifndef NO_IOCTL_H
#include        <sys/ioctl.h>
#endif
                        /* needed for error check */
#include        <errno.h>

extern int      errno;

int	Fct_send_timeout = 4;
int	Fct_maxsendpacket = 5;
int	Fct_log_timeout = 5;
int	Fct_log_retrys = 4;

extern int	Fctchan;                            /* facet channel */

#ifdef DPRINT
int Dbgfd;		/* Debug message file descriptor */
#endif

main()
{
	set_options_facetpath();
	init_communications();
	common();
	/* NOTREACHED */
}
fsend_finish()
{
}
/* ==================================================================== */

#define LOGINOK		(0x100 | FP_RQST_LOGIN )

#define FCT_MSG_REQUEST 0x200
#define FCT_LOGMSG	0
#define FCT_LOGFMSG	1
#define FCT_ESTBMSG	2
#define FCT_CHARMSG	3
#define FCT_INVMSG	4
#define FCT_MATCHMSG	5

#define RESTARTOK	0x300
/*=====================================================================*/
char *Fct_msg[] =
{
    "\r\nEstablishing FACET host connection - Facet/PC should be in Multi-session mode\r\n",
	"\r\nFACET host connection FAILED\r\n",
	"FACET host connection established\r\n",
	"FACET invalid registration character\r\n",
	"FACET invalid registration number\r\n",
	"FACET registration number already logged in on another line\r\n"
};

int	Send_seq = 0;
fsend_init()
{
	fsend_init_communications();
	Send_seq = 0;	/* dont match */
#ifdef LOGIN_REQUIRED
	fsend_loginget();
#endif
}
/* ARGSUSED */
fsend_winrcvd( window )
	int window;
{
	Inwin = window;
	fsend_outprotocol( FP_WINACK | window );
}
fsend_window( window, ptr, count )
	int		window;
	unsigned char	*ptr;
	int		count;
{
#ifdef HOSTCOMMANDACK
	int ack;				/* value of ack received */

	fsend_ackreset();
#endif
					/* switch to this output window */
	fsend_outprotocol( FP_WINSEL | window );
#ifdef HOSTCOMMANDACK
					/* if host window selections are
					   acked, start the output and wait
					   for the ack, retrying if it times
					   out. */
	fsend_outgowait();
	while( (ack=fsend_ackwait( Fct_send_timeout )) != (FP_WINACK | window) )
	{
		fsend_ackreset();
		if ( ack == -1 )
		{
			fsend_outprotocol( FP_WINSEL | window );
			fsend_outgowait();
		}
	} 
#endif
	fsend_outchars( ptr, count );
					/* start the transmission if needed */
	fsend_outgowait();
}
#ifdef LOGIN_REQUIRED
			/* login to host
			   when a facet driver first comes up, it should
			   request the PC that is connected to it to login.
			   it sends a login request and waits with timeout
			   for the receiver to get a packet with a serial
			   number.  This serial number must be different
			   than the serial numbers on all the other logged
			   in lines.  Be careful that serial numbers on
			   the other lines are not left around.      
			   If the timer expires, request again.
			   If the serial number is not unique, complain. */
fsend_loginget()	/* request a login until the receiver signals that
			   it has received a good login packet by internally
			   generating an ack */
{
	int ack;
	int trys;

	trys = 0;
	fsend_msg( FCT_LOGMSG );
	fsend_ackreset();
	fsend_outprotocol( FP_RQST_LOGIN );
	fsend_outgowait();
	while( (ack=fsend_ackwait( Fct_log_timeout )) !=  LOGINOK )
	{
		fsend_ackreset();
		trys++;
		if ( trys >= Fct_log_retrys )
		{
			fsend_msg( FCT_LOGFMSG );
			fsend_kill( "", 0 ); /* does not return */
		}
		if ( ack == -1 )
		{
			fsend_msg( FCT_LOGMSG );
			fsend_outprotocol( FP_RQST_LOGIN );
			fsend_outgowait();
		}
	} 
	fsend_msg( FCT_ESTBMSG );
}
#endif
fsend_msg( msg )
	int msg;
{
	fsend_outchars( (unsigned char * ) Fct_msg[ msg ],
			strlen( Fct_msg[ msg ] ) );
	fsend_outgowait();
}
/*---------------------------------------------------------------------*\
| fsend_outchars                                                        |
|                                                                       |
| Transmit "count" chars from pointer "ptr " to line "line_tp".         |
\*---------------------------------------------------------------------*/
fsend_outchars( ptr, count )
	unsigned char *ptr;
	int count;
{
	register unsigned char *p;	/* current characters to send */
	register int i;			/* number of bytes to send */

	while ( count )
	{
		p = ptr;
		i = 0;
		while( *p != FP_LEADIN  && i < count )
		{
			p++;
			i++;
		}
		write( 1, (char *) ptr, i );
		ptr += i;
		count -= i;
		if ( count > 0 )
		{
			if ( *p == FP_LEADIN )
			{
				write( 1, (char *) p, 1 );
				write( 1, (char *) p, 1 );
				count--;
				ptr++;
			}
		}
	}
}
fsend_outprotocol( command )/* output a protocol command - indivisible */
	int command;
{
	char c;

	c = FP_LEADIN;
	write( 1, &c, 1 );
	c = command;
	write( 1, &c, 1 );
}
fsend_outgowait()	/* start the output on the line and
				   wait until the output drains */
{
	fsend_outgo();
	fsend_waitempty();
}
fsend_outgo()		/* start the output on the line */
{
}
fsend_waitempty()	/* wait until the output on the line drains */
{
}
fsend_procack_array( ackcount, ackchars )	/* process array of acks */
	int		ackcount;
	unsigned int	ackchars[];
{
	int	i;

	for ( i = 0; i < ackcount; i++ )
		fsend_procack( ackchars[i] );
}
#include "restart.h"
fsend_procack( c )
	unsigned int c;
{
	int	window;
	int	seq;

	if ( c & 0xFF00 )
	{				/* receiver generated message */
		if ( c == LOGINOK )
		{			/* receiver has verified login */
			fsend_ackrcvd( c );
		}
		else if ( ( c & 0xFF00 ) == FCT_MSG_REQUEST )
		{			/* receiver requesting msg print */
			fsend_msg( (int) (c & 0xFF) );
		}
		else if ( c == RESTARTOK )
		{
			Saw_restart = 1;
		}
		else
		{
			printf( "\r\nFSEND ERROR: invalid from frecv = %x\r\n",
				c );
			term_outgo();
		}
	}
	else
	{
		if ( (c & WINSEL_MASK) == FP_WINSEL )
		{			/* window select */
			window = c & WINDOW_MASK;
			fsend_winrcvd( window );
		}
		else if ( (c & WINSEL_MASK) == FP_WINACK )
		{			/* window ack */
			fsend_ackrcvd( c );
		}
#ifdef HOSTCOMMANDACK
		else if ( (c & PKT_MASK) == FP_ACK_BEG_PKT )
		{			/* packet ack */
			fsend_ackrcvd( c );
		}
#endif
		else if ( (c & PKT_MASK) == FP_ACK_END_PKT )
		{			/* packet ack */
			fsend_ackrcvd( c );
		}
		else if ( (c & PKT_MASK ) == FP_BEG_PKT )
		{			/* packet start */
			seq = c & PKT_SEQ_MASK;
			fsend_outprotocol( FP_ACK_BEG_PKT | seq );
		}
		else if ( (c & PKT_MASK ) == FP_END_PKT )
		{			/* packet start */
			seq = c & PKT_SEQ_MASK;
			fsend_outprotocol( FP_ACK_END_PKT | seq );
		}
		else
		{
			printf( "\r\nFSEND ERROR: invalid from frecv = %x\r\n",
				c );
			term_outgo();
		}
	}
}
/*---------------------------------------------------------------------*\
| fsend_packsend                                                          |
|                                                                       |
| Transmit the packet that is in the send packet storage for this line. |
| The checksum is computed and stored in the packet.      		|
\*---------------------------------------------------------------------*/
fsend_packsend( iowin, sp )
	int iowin;
	struct facet_packet *sp;/* pointer to send packet */
{
	int count;		/* number of bytes to send including header */
	int ack;		/* value of ack received */
	int pseq;		/* packet sequence */
	int tries;

	if ( sp->pkt_header.arg_cnt > MAXPROTOARGS )
	{
		return( EINVAL );
	}
	sp->pkt_header.pkt_window = iowin;
	fsend_build_checksum( sp );
	pseq = Send_seq & PKT_SEQ_MASK;
	Send_seq++;
	if ( (Send_seq & PKT_SEQ_MASK) == 0 )
		Send_seq++;
	for ( tries=0; tries < Fct_maxsendpacket; tries++ )
	{
				/* packet start protocol */
		fsend_outprotocol( FP_BEG_PKT | pseq );
#ifdef HOSTCOMMANDACK
				/* if the PC is acknowledging windows and
				   packet starts, wait for the ack */
		fsend_ackreset();
		fsend_outgowait();
		while( (ack=fsend_ackwait( Fct_send_timeout )) !=
						FP_ACK_BEG_PKT | pseq )
		{
			fsend_ackreset();
			if ( ack == -1 )
			{
				fsend_outprotocol( FP_BEG_PKT | pseq );
				fsend_outgowait();
			}
		}
#endif
				/* send the packet characters */
		count = sizeof(struct fpkt_header) + sp->pkt_header.arg_cnt;
		fsend_outchars( (unsigned char *) sp, count );
				/* send end packet protocol */
		fsend_ackreset(); 
		fsend_outprotocol( FP_END_PKT | pseq );
		fsend_outgowait();
				/* wait for ack - retry if timeout */
		while( (ack=fsend_ackwait( Fct_send_timeout )) != -1 )
		{
			if ( ack == (FP_ACK_END_PKT | pseq) )
				return( 0 );
			else
				fsend_ackreset();
		} 
	}
	return( EIO );
}
/*---------------------------------------------------------------------*\
| fsend_build_checksum                                                  |
|                                                                       |
| compute and store the packet checksum in the packet                   |
\*---------------------------------------------------------------------*/
fsend_build_checksum( sp )
	struct facet_packet *sp;
{
	register unsigned char *p;
	unsigned char checksum;
	int i;
	int count;

	sp->pkt_header.pkt_checksum = 0;
	checksum = 0;
	p = (unsigned char *) sp;
	count = sizeof(struct fpkt_header) + sp->pkt_header.arg_cnt;
	for ( i = 0; i < count; i++ )
	{
		checksum ^= *p++;
	}
	sp->pkt_header.pkt_checksum = checksum;
}
/* ========================================================================= */
int Recv_seq = 0;
struct facet_packet Recv_pkt;	
/*---------------------------------------------------------------------*\
| frecv_recv                                                            |
|                                                                       |
| Performs receive process  receive processing needed for Facet.        |
\*---------------------------------------------------------------------*/
frecv_recv()
{
	int c;

#ifdef DPRINT
	Dbgfd = open( "/dev/console", 1 );
#endif
	frecv_init_communications();
			/* allow any seq # on first packet */
	Recv_seq = 0;
			/* process all incoming characters */
	while (1) 
	{
		c = frecv_lingetc();
		if ( c == FP_LEADIN )
		{			/* command or data start */
			frecv_winprocess();
			c = frecv_lingetc();
			if ( c == FP_LEADIN ) 	/* LEADIN is data */
			{
				frecv_winbuffc( c );
			}
			else if ( c == FP_SCONTROL ) 
			{			/* user typed control-s */
				frecv_winbuffc( CSTOP );
			}
			else if ( c == FP_QCONTROL ) 
			{			/* user typed control-q */
				frecv_winbuffc( CSTART );
			}
			else if ( c == FP_SCTL8BIT ) 
			{			/* user typed control-s with
						   8th bit set */
				frecv_winbuffc( 0x93 );
			}
			else if ( c == FP_QCTL8BIT ) 
			{			/* user typed control-q with
						   8th bit set */
				frecv_winbuffc( 0x91 );
			}
			else if ( (c & WINSEL_MASK) == FP_WINSEL )
			{			/* window select */
				frecv_winrcvd( c & WINDOW_MASK );
						/* window could have changed */
			}
			else if ( (c & WINSEL_MASK) == FP_WINACK )
			{			/* window ack */
				frecv_ackrcvd( c );
			}
#ifdef HOSTCOMMANDACK
			else if ( (c & PKT_MASK) == FP_ACK_BEG_PKT )
			{			/* packet ack */
				frecv_ackrcvd( c );
			}
#endif
			else if ( (c & PKT_MASK) == FP_ACK_END_PKT )
			{			/* packet ack */
				frecv_ackrcvd( c );
			}
			else if ( (c & PKT_MASK ) == FP_BEG_PKT )
			{			/* packet start */
				if ( frecv_packrecv( c ) == 0 )
				{
#ifdef LOGIN_REQUIRED
				    if ( frecv_loginchk() == 0 )
#endif
				     if ( frecv_startchk() == 0 )
				      if ( frecv_restartchk() == 0 )
				      {
				      }
				}
						/* window could have changed */
			}
			else if ( c == FP_HANGUP )
			{			/* PC going away */
				frecv_kill( "", 0 );	/* does not return */
			}
#ifdef DPRINT
			else
			{			/* unknown commmand */
				dprint("Facet receiver unk protocol %x\n",c);
			}
#endif
		}
		else
		{		/* normal character */
			frecv_winbuffc( c );
		}
	}
}
#define MY_CLSIZE 64
unsigned char	Inbuff[ MY_CLSIZE ];
int		Incount = 0;
unsigned char	*Inptr = Inbuff;
frecv_lingetc()		/* get a character from the line */
{
	if ( Incount-- <= 0 )
	{
		frecv_winprocess();
					/* don't know why yet, but
					   going into the kernel to
					   do a ttin of a full MY_CLSIZE
					   buffer of characters causes
					   the raw queue to be flushed. */

		Incount = read( 0, (char *) Inbuff, MY_CLSIZE-1 );
		if ( Incount < 1 )
			frecv_kill( "Facet process: read", Incount );
		Incount--;
		Inptr = Inbuff;
	}
	return( *Inptr++ );
}
unsigned char	Outbuff[ MY_CLSIZE ];
int		Outcount = 0;
unsigned char	*Outptr = Outbuff;
frecv_winbuffc( c )
	int c;
{
	if ( ++Outcount >= MY_CLSIZE )
	{
		frecv_winprocess();
		Outcount = 1;
		Outptr = Outbuff;
	}
	*Outptr++ = c;
}
frecv_winprocess()
{
	if ( Outcount )
	{
		frecv_winputs( Inwin, Outbuff, Outcount );
		Outcount = 0;
		Outptr = Outbuff;
	}
}
#ifdef LOGIN_REQUIRED
			/* login to host
			   when a facet driver first comes up, it should
			   request the PC that is connected to it to login.
			   it sends a login request and waits with timeout
			   for the receiver to get a packet with a serial
			   number.  This serial number must be different
			   than the serial numbers on all the other logged
			   in lines.  Be careful that serial numbers on
			   the other lines are not left around. ????
			   If the timer expires, request again.
			   If the serial number is not unique, complain. */

frecv_loginchk()	/* See if the packet just received successfully is
			   a login.  If so, process and return 1. Else return
			   0 */
{
	struct facet_packet *rp;
	long serial;
	short check;
	char *inserial;			/* registration number string */


	rp = &Recv_pkt;
	if ( rp->pkt_header.cmd_byte != FACET_LOGIN )
		return( 0 );			/* not a logon */
	frecv_toacks( LOGINOK );
	return( 1 );
}
#endif
frecv_startchk()	/* See if the packet just received successfully is
			   a start request.  If so, process and return 1.
			   Else return 0 */
{
	struct facet_packet *rp;
	int	window;			/* window 0-9 */
	char	*program;			/* program to run */
	char	*shell;			/* shell to run */
	char	*getenv();


	rp = &Recv_pkt;
	window = rp->pkt_header.pkt_window;
	if ( rp->pkt_header.cmd_byte == FACET_RUN_PROG )
	{
		program = (char *) rp->pkt_args;
		startwin( window, "", program );
		return( 1 );			/* was run program */
	}
	else if ( rp->pkt_header.cmd_byte == FACET_RUN_SHELL )
	{
		shell = getenv( "SHELL" );
		if ( shell == NULL )
			shell = "-sh";
		else if ( strcmp(shell, "/bin/sh") == 0)
			shell = "-sh";
		else if ( strcmp(shell, "sh") == 0)
			shell = "-sh";
		startwin( window, "", shell );
		return( 1 );			/* was run shell */
	}
	else
		return( 0 );			/* not either */
}
frecv_restartchk()	/* See if the packet just received successfully is
			   a restart.  If so, process and return 1. Else return
			   0 */
{
	struct facet_packet *rp;


	rp = &Recv_pkt;
	if ( rp->pkt_header.cmd_byte != FACET_RESTART )
		return( 0 );			/* not a logon */
	frecv_toacks( RESTARTOK );
	return( 1 );
}
/*---------------------------------------------------------------------*\
| frecv_packrecv                                                        |
|                                                                       |
| receive a packet assuming that the start packet has already been      |
| received.                                                             |
| Protocol for the transmitter can be imbedded.  It is sent to the      |
| transmitter and packet reception continues.                           |
| Protocol for the receiver is assumed to be an indication that the PC  |
| has ceased sending the packet.  The protocol is processed normally    |
| and then the packet reception is aborted.  If it is a start packet,   |
| then the packet reception restarts from the beginning.                |
\*---------------------------------------------------------------------*/
frecv_packrecv( c)
	int c;
{
	struct facet_packet *rp;		/* ptr to receive packet */
	unsigned char *p;			/* where to store next char */
	unsigned char checksum;
	int count;				/* chars in packet w/o header */
	int pseq;				/* packet sequence number */

	rp = &Recv_pkt;
			/* unexpected packet start while receiving a packet
			   jumps to here to start over */
restart:
			/* get the packet sequence number */
	pseq = c & PKT_SEQ_MASK;
#ifdef DPRINT
	dprint("frecv_packrecv pkt %d\n",pseq);
#endif
#ifdef PCCOMMANDACK
			/* if pc commands are being acked, ack the
			   packet start */
	frecv_toacks( c );
#endif
			/* collect the packet - count includes the arguments
			   only. */
	p = (unsigned char *) rp;
	checksum = 0;
	count = -( sizeof(struct fpkt_header) );
	while( 1 )
	{
		c = frecv_lingetc();
		if ( c == FP_LEADIN )
		{			/* command or data start in packet*/
			c = frecv_lingetc();
			if ( c == FP_LEADIN ) 	/* LEADIN is data in packet*/
			{
				if ( ++count <= MAXPROTOARGS )
				{
					checksum ^= c;
					*p++ = c;
				}
			}
			else if ( c == FP_SCONTROL ) 
			{			/* data is control-s */
				c = CSTOP;
				if ( ++count <= MAXPROTOARGS )
				{
					checksum ^= c;
					*p++ = c;
				}
			}
			else if ( c == FP_QCONTROL ) 
			{			/* data is control-q */
				c = CSTART;
				if ( ++count <= MAXPROTOARGS )
				{
					checksum ^= c;
					*p++ = c;
				}
			}
			else if ( c == FP_SCTL8BIT ) 
			{			/* data is control-s with high
						   bit set */
				c = CSTOP | 0x80;
				if ( ++count <= MAXPROTOARGS )
				{
					checksum ^= c;
					*p++ = c;
				}
			}
			else if ( c == FP_QCTL8BIT ) 
			{			/* data is control-q with high
						   bit set */
				c = CSTART | 0x80;
				if ( ++count <= MAXPROTOARGS )
				{
					checksum ^= c;
					*p++ = c;
				}
			}
			else if ( (c & WINSEL_MASK) == FP_WINACK )
			{			/* window ack in packet */
				frecv_ackrcvd( c );
			}
#ifdef HOSTCOMMANDACK
			else if ( (c & PKT_MASK) == FP_ACK_BEG_PKT )
			{			/* packet ack in packet */
				frecv_ackrcvd( c );
			}
#endif
			else if ( (c & PKT_MASK) == FP_ACK_END_PKT )
			{			/* packet ack in packet */
				frecv_ackrcvd( c );
			}
			else if ( (c & PKT_MASK) == FP_END_PKT )
			{			/* packet end in packet */
						/* normal end of packet */
				break;
			}
			else if ( (c & PKT_MASK) == FP_BEG_PKT )
			{			/* packet start in packet */
						/* must have started over */
#ifdef DPRINT
				dprint("restarting packet\n");
#endif
				goto restart;
			}
			else if ( (c & WINSEL_MASK) == FP_WINSEL )
			{			/* window select in packet*/
				frecv_winrcvd( c & WINDOW_MASK );
						/* packet receive is aborted */
				return(-1);
			}
			else if ( c == FP_HANGUP )
			{			/* PC going away */
				frecv_kill( "", 0 );	/* does not return */
			}
			else
			{			/* unk commmand in packet */
#ifdef DPRINT
				dprint(
				  "Facet driver unk protocol in packet %x\n",c);
#endif
						/* packet receive is aborted */
				return( -1 ); 
			}
		}
		else
		{		/* normal character in packet -if overflow
				   then stop storing */
			if ( ++count <= MAXPROTOARGS )
			{
				checksum ^= c;
				*p++ = c;
			}
#ifdef DPRINT
			else
				dprint("throwing away extra pkt chars\n");
#endif
		}
	}

#ifdef DPRINT
	dprint("complete packet received\n");
#endif
			/* packet must have same sequence number in start and
			   end packet, a good checksum, a valid number of 
			   bytes, and the length received must match the
			   length sent. */
	if ( ( pseq == (c & PKT_SEQ_MASK) )
	&&   ( count >= 0 ) &&   ( count <= MAXPROTOARGS )
	&&   ( count == rp->pkt_header.arg_cnt )
	&&   ( checksum == 0 ) )
	{
			/* good packet reception */
		frecv_toacks( c );
		if ( pseq != Recv_seq || pseq == 0 )
		{			/* not a duplicate */
			Recv_seq = pseq;
#ifdef DPRINT
			dprint ("good packet received\n");
#endif
			return( 0 );
		}
		else			/* duplicate */
		{
#ifdef DPRINT
			dprint ("duplicate packet received\n");
#endif
			return( -1 );		
		}
	}
	else
	{
			/* bad packet reception - ignore it */
#ifdef DPRINT
	if ( ( pseq == (c & PKT_SEQ_MASK) )
	&&   ( count >= 0 ) &&   ( count <= MAXPROTOARGS )
	&&   ( count == rp->pkt_header.arg_cnt )
	&&   ( checksum == 0 ) )
		dprint( "bad packet rcvd: pseq = %d, c & SM = %d, count = %d, arg_cnt = %d, checksum = %d\n",pseq,(c&PKT_SEQ_MASK),count,rp->pkt_header.arg_cnt,checksum);
#endif
		return( -1 );
	}
}
/*---------------------------------------------------------------------*\
| frecv_ackrcvd                                                           |
|                                                                       |
| Post an ack that was received.  In order to be valid, an ack must be  |
| be received while the timer is running.  It is ignored otherwise.     |
| Storing the ack cancels the timer.  Wakeup anyone waiting on the ack. |
\*---------------------------------------------------------------------*/
frecv_ackrcvd( ackvalue )
	int ackvalue;
{
	frecv_toacks( ackvalue );
}
struct termio T_normal_master;
/*======================================================================*/
setcrt()
{
        struct termio term;

        if ( ioctl( 0, TCGETA, &term) )
        {
                printf( "Facet process: Ioctl TCGETA failed, error %d\n",
                        errno );
                perror( "             " );
		term_outgo();
                return( -1 );
        }
						/* PSEUDOTTY */
	if ( ioctl( 1, TCGETA, &T_normal_master ) < 0 )
	{
                printf( "Facet process: Ioctl TCGETA M failed, error %d\n",
                        errno );
                perror( "             " );
		term_outgo();
		return( -1 );
	}
	T_normal_master.c_line = 0;
	T_normal_master.c_oflag |= TAB3;
                                /* leave baud rate and CLOCAL */
        term.c_cflag |= ( CS8 | CREAD | HUPCL );
        term.c_cflag &= ~( CSTOPB | PARENB | PARODD );
                                /* RAW  w/ flow control */
        term.c_iflag = IGNBRK | IGNPAR | IXON;
                                /* RAW */
        term.c_oflag = 0;
        term.c_lflag = 0; /* was XCLUDE; */
        term.c_cc[VTIME] = 0;
        term.c_cc[VMIN] = 1;
        if ( ioctl( 0, TCSETAW, &term) )
        {
                printf( "Facet process: Ioctl TCSETA failed, error %d\n",
                        errno );
                perror( "             " );
		term_outgo();
                return( -1 );
        }
        return(0);
}
						/* PSEUDOTTY */
termio_normal_master( fd )
	int	fd;
{
	int	status;

	term_outgo();
	status = ioctl( fd, TCSETA, &T_normal_master );
	if ( status < 0 )
	{
		perror( "termio_normal_master" );
		return( -1 );
	}
	return( 0 );
}
term_outgo()		/* dummy function called by common for facetterm */
{
	fflush( (FILE *) stdout );
}


#ifdef DPRINT
/*---------------------------------------------------------------------*\
| dprint                                                                |
|                                                                       |
| a special printf for reporting internal facet process errors.         |
\*---------------------------------------------------------------------*/

/* VARARGS */
dprint (fmt, args)
char *fmt, *args;
{
	char **s_argptr, *fmtptr;
	int *i_argptr;
	char buff[16];
	char *strptr;
	int len;

	s_argptr = &args;
	i_argptr = (int *)(&args);
	for (fmtptr = fmt; *fmtptr; fmtptr++)
	{
		if (*fmtptr == '%')
		{
			fmtptr++;
			if (*fmtptr == 's')
			{
				strptr = *s_argptr;
				len = strlen (strptr);
				s_argptr++;
				i_argptr = (int *)(s_argptr);
			}
			else if (*fmtptr == 'x')
			{
				strptr = buff;
				len = itos ((unsigned) *i_argptr, buff, 16);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == 'd')
			{
				strptr = buff;
				len = itos ((unsigned) *i_argptr, buff, 10);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == 'o')
			{
				strptr = buff;
				len = itos ((unsigned) *i_argptr, buff, 8);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else
			{
				strptr = fmtptr - 1;
				len = 2;
			}
		}
		else
		{
			strptr = fmtptr;
			len = 1;
		}
		write( Dbgfd, "\033[7m", 4 );
		while (len--)
		{
			if (*strptr == '\n')
			{
				write( Dbgfd, "\033[0m", 4 );
				write (Dbgfd, "\r\n", 2);
				write( Dbgfd, "\033[7m", 4 );
			}
			else
				write (Dbgfd, strptr, 1);
			strptr++;
		}
		write( Dbgfd, "\033[0m", 4 );
	}
}

/*---------------------------------------------------------------------*\
| itos                                                                  |
|                                                                       |
| The integer value to be converted is sent in "intval" and a pointer   |
| to the buffer to hold the string is sent in "string". Itos returns    |
| the length of the resultant string. The integer is an unsigned short. |
\*---------------------------------------------------------------------*/

int itos (intval, string, radix)
unsigned intval;
char *string;
int radix;
{
	register int i, digit;
 
	i = 0;
	do
	{
		digit = intval % radix;
		if (digit >= 10)
			*(string + (i++)) = digit - 10 + 'A';
		else
			*(string + (i++)) = digit + '0';
	} while ((intval /= radix) != 0);
	*(string+i) = '\0';
	revstr (string, i);
	return (i);
}




/*---------------------------------------------------------------------*\
| revstr                                                                |
|                                                                       |
| Reverses the string pointed to by "buffer".                           |
\*---------------------------------------------------------------------*/
 
revstr (buffer, len)
char buffer[];
int len;
{
	char c;
	register int i,j;
 
	for (i = 0, j = len-1; i < j; i++, j--)
	{
		c = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = c;
	}
}

#endif
get_curwin_string( s )
char    *s;
{
	sprintf( s, "%d", Inwin + 1 );
}
window_first_open( winno )
	int	winno;
{
	set_window_active( winno );
}
window_closed( winno )
	int	winno;
{
	clear_window_active( winno );
	forget_exec_list( winno );
}
window_termio_in_hex( string )
	char	*string;
{
	char	*p;
	int	i;

	p = (char *) &T_normal_master;
	for ( i = 0; i < sizeof( struct termio ); i++ )
		sprintf( &string[ i * 2 ], "%02x", *p++ & 0x00FF );
}
