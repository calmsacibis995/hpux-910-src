/* HPUX_ID: @(#)ody.c	01.1		89/04/26 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

/*
 hp 98628 driver modified from the 98626 driver 
	and then remodified for the 98659
 */

#include "../h/param.h"		/* various types and macros */
#include "../h/file.h"
#include "../h/tty.h"		/* tty structure and parameters */
#include "../h/conf.h"		/* linesw stuff */
#include "../wsio/intrpt.h"
#include "../wsio/hpibio.h"
#include "../h/modem.h"
#include "../h/signal.h"

#include "../h/termio.h"

#include "../h/dir.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/errno.h"		/* system error values */
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/types.h"
#include "../h/uio.h"

#include "../s200io/98659.h"



#define ODYSIZE sizeof(struct ody_desc_type)
#define ody_tp  (struct tty *)(isc_table[m_selcode(dev)]->ppoll_f)
#define ody_tp_ptr (struct tty *)(isc->ppoll_f) 
#define card_to_sc	((((int) (tp->t_card_ptr)) >> 16) & 0x1F)
#define	ody_errors	(char)tp->t_col
				/* number of 98659 card crashes */
#define	ody_int_error	(char)tp->t_row
				/* to pass errors from ISR to ioctl */

extern struct tty *cons_tty;
extern dev_t cons_dev;

extern	int	ody_control();

void	odyproc();
int	ody_intr();
int	ody_intr_0();
void	odystrt();

/* ID string for use by the SCMS Manager */
char sccsid[] ="@(subid) MAIN:                                                                ";
/*		                       1         2         3         4         5         6    */
/*  @(subid) MAIN:<64 Spaces> 1234567890123456789012345678901234567890123456789012345678901234*/


/****************************************************************************
 * ody_open (dev, flag)   
 *
 *   dev_t dev - Describes which serial card will be opened.
 *   int flag - Specifies 'open' access modes.
 *
 *	if(Bad Minor Number)
 *		return(ENXIO)
 *	if(No device at sel code)
 *		return(ENXIO)
 *	if(first open)
 *		establish a process group for distribution of
 *		signals from the driver
 *		clear any pending interrupts
 *		clear the card buffers
 *		connect the output
 *		return(0)
 *
 *	Access mode 
 *		This device will always open independent of the state
 *		of any modem lines i.e. carrier.  The state of O_NDELAY
 *		will not not cause any delay in opening.
 *
 *	Card level interrupts are enabled by the ody_control calls
 *
 *	If the 98659 card fails to respond to any commands
 *		The message "98659 at sc XX down" is printed
 *		This message is kept in the kernel message queue
 *		and can be accessed by executing the /etc/dmesg
 *		program.  This is also where more detailed information
 *		about other errors will be kept.
 *
 *	If an open 98659 card ever fails to respond to any command, the
 *		driver will block any further access through read, write,
 *		ioctl, or select until the card is closed and then reopened.
 *		This will only occur if the card dies through a hardware
 *		fault or firmware error.
 *
 *	THIS WILL BE TRUE FOR ALL COMMANDS TO THIS DRIVER
 *
 ****************************************************************************/

int
ody_open(dev, flag)

   dev_t  dev;
   int   flag;

{
	register struct	isc_table_type	*isc;
	register int	selcode;
	register struct	tty	*tp;
	int	status;
	int	x;
	int	err = 0;
	
	selcode = m_selcode(dev);
	if ((selcode < 0) || (selcode > 31)) /* bad minor number */
	{
		msg_printf("Invalid Select code on open \n");
		msg_printf("Device Number %d, Select code = %d \n",
							dev,selcode);
		return(ENXIO);
	}
	if ((isc = isc_table[selcode]) == NULL) 
				/* no device at sel code */
	{
		msg_printf("No device at Select Code \n");
		msg_printf("Device Number %d, Select code = %d \n",
							dev,selcode);
		return(ENXIO);
	}
	
	tp = ody_tp_ptr;


	/* poke the card to allow it to connect if first open */
	if ( (tp->tty_count) == 0 )	/*FIRST OPEN */
	{
		x = splx (tp->t_int_lvl);

		tp->t_dev = dev;
		tp->t_state = (ISOPEN | CARR_ON | TTY);

		tp->t_rsel = 0;		/* for use by select */
		tp->t_wsel = 0;
		ody_errors = 0;
		ody_int_error = 0;	
					/* process group for break*/
		tp->t_pgrp == u.u_procp->p_pgrp;

		/* clear possible interrupt by reading this location*/
		status = *((unsigned char *)(tp->t_card_ptr +
							INT_COND_OFFSET));

		/* reset the card */

		if( (ody_control( PASS_ODY, CLEAR_REG, 1)) < 0 )
		{
			odydown(tp,"ody_control CLEAR_REG,1 during open");
		}

		/* setup for initial values of B300, CS8, and CREAD. */

		tp->t_iflag = (IGNBRK | IGNPAR);
		tp->t_oflag = NULL;
		tp->t_cflag = (B300 | CS8 | CREAD);
		tp->t_lflag = NULL;
		tp->t_line = NULL;
		tp->t_cc[VINTR] = NULL;	/* Not used */
		tp->t_cc[VQUIT] = NULL;	/* Not used */
		tp->t_cc[VERASE] = NULL;	/* Not used */
		tp->t_cc[VKILL] = NULL;	/* Not used */
		tp->t_cc[VEOF] = NULL;	/* also VMIN */
		tp->t_cc[VEOL] = NULL;	/* also VTIME */
		tp->t_cc[6] = NULL;	/* Not used */
		tp->t_cc[7] = NULL;	/* Not used */

		if ((ody_control( PASS_ODY, CONNECT_REG, CONNECT)) < 0)	
			odydown(tp,"ody_control CONNECT_REG,CONNECT during open");

		err = odyparam(tp);		/* setup all */

		/* initialize modem control stuff */
		if ( ody_control( PASS_ODY,
				HW_HANDSHAKE_REG, HRD_HANDSKE_OFF) < 0)
			odydown(tp,"ody_control HW_HANDSHAKE_REG,HRD_HANDSKE_OFF during open");

		tp->t_smcnl = MDTR;	/* always start with DTR set */
		if ( ody_control( PASS_ODY, MODEM_REG_8, ODY_DTR) < 0)
			odydown(tp,"ody_control MODEM_REG_8,ODY_DTR during open");

		splx(x);
	}

	/* if the card failed to acknowledge communications to it,
		during initialization, the TTY bit in t_state is
		cleared. If err != 0 there was a communications
		error in the odyparam setup*/
	if( (tp->t_state & TTY) && (err == 0) )
	{
		/* increment number of opens called */
		++tp->tty_count;
	}
	else 
	{
		/* we are dead, allow next open to reinitialize */
		tp->tty_count = 0;
		err = EIO;
	}
	return(err);
}

/****************************************************************************
 * ody_close (dev)
 *
 *   dev_t dev - Describes which serial card is to be closed.
 *
 *   ody_close is called when the last close command has been issues for
 *   a specific serial card.  It sets the open count to zero 
 *	and disconnects	the card.  
 *	
 *	Zero out count	
 *	Wait for output to drain or be flushed
 *		if output rate is less than 9600 baud and there are more
 *			than 256 bytes still in the output buffer queue,
 *			the output queue may be flushed, otherwise the 
 *			driver will wait for the queue to empty before 
 *			finishing
 *	Disconnect card, both input and output
 *	Disable interrupts from card
 *	return(0)
 *	
 ****************************************************************************/

int
ody_close(dev)

   dev_t  dev;
{
	register struct	tty	*tp;
	register int		x;
	int 	err = 0;
	
	tp = ody_tp;

	ody_wait(tp);

	x = splx (tp->t_int_lvl);

	/* zero out count */
	tp->tty_count = 0;

	/* any state bits are past history */
	tp->t_state = 0;
	tp->t_dev = 0;
	tp->t_pgrp = 0;

	/* poke the card to force it to disconnect */
	/* while closed, we want it to neither transmit or receive */
	if ((ody_control( PASS_ODY, CONNECT_REG, DISCONNECT)) < 0)
	{
		err = EIO;
		odydown(tp,"ody_control CONNECT_REG,DISCONNECT during close");
	}
	
	/* Disable all card level interrupts */
		*((char *)(tp->t_card_ptr + INT_ENABLE_OFFSET)) = 0x00;

	splx(x);

	return(err);
}


/****************************************************************************
 * ody_strategy (bp)
 *
 *   struct buf	bp;
 *
 *   ody_strategy is written to allow us to use the higher speed physio 
 *   routine to read and write to the 98659 card
 *   This may reduce speed for character by character transfers but 
 *   allows much better performance on larger block transfers
 *
 *	This cannot be directly called from outside the driver
 *   
 ****************************************************************************/

int
ody_strategy (bp)

	struct buf	*bp;
{
	dev_t	dev;
	register struct tty *tp;     /* Ptr to tty structure */
	int	to_send,sent;
	int	to_receive,received;
	ushort	report;
	caddr_t	buffer;
	int	x;

 int	temp;
 char	clear_error[1];

	/* initialize bp structure that we will be using */

	bp->b_error = 0;
	bp->b_resid = bp->b_bcount;

	buffer = bp->b_un.b_addr;

	dev = bp->b_dev;
	tp = ody_tp;

	if (bp->b_flags & B_READ)
	/*	READ request */
	{
	    to_receive = bp->b_resid;

	    x = splx (tp->t_int_lvl);

	    if( (received = ody_enter(PASS_ODY,buffer,
	    		to_receive,&report)) < 0 )
	    {
		msg_printf("Unable to read card buffer on 98659 at ");
		msg_printf("sc %d, semaphore locked \n",card_to_sc);
	    }

	    splx(x);

	    bp->b_resid -= received;

		/* report is MODE and TERM returned by the
		   ody_enter routine,  They represent a control
		   block in the card receive buffer.  If Zero,
		   there was no control block read */

		/* A non-zero report indicates a control block was
		   read.  If the control block indicates an ERROR
		   or a break, the associated character is discarded.
		   If multiple ERRORs or breaks follow each other,
		   they will be read and discarded until the buffer
		   is empty or good data is found */

	    while((report & MODE_MASK) != 0)
	    {
		/* When in canonical mode, receive available equals
		   the number of defined lines available to read.
		   a line is defined as 0 or more characters
		   terminated by either a EOF, a LF, a EOL,
		   a break, or an ERROR.  We need to inform the
		   card each time we consume a control block */

		/*	Decrement RECEIVE_AVAILABLE	*/
		if(tp->t_lflag & ICANON)
		{
			if( ody_control(PASS_ODY,
					RECEIVE_AVAILABLE,1) < 0 )
				odydown(tp,"ody_control RECEIVE_AVAILABLE,1 during strategy");
		}


	        switch (report & MODE_MASK)
	        {
	          case BREAK_IN:
			/* break received */
	    		/* clear byte out and throw away */

		        x = splx (tp->t_int_lvl);

	    		if( ody_enter(PASS_ODY,clear_error,
						1,&report) < 0 )
			    {
				msg_printf("Unable to read card buffer on 98659 at ");
				msg_printf("sc %d, semaphore locked \n",card_to_sc);
			    }

			splx(x);

	    		break;

	    	  case ERROR:	
	    		/* framing and/or parity error on the
	    		** byte that we haven't got yet.*/
			if (report & PARITY_ERROR)
			{
			    msg_printf("Parity Error ");
			    msg_printf("on 98659A at select code ");
			    msg_printf(" %d, ",card_to_sc);
			}
			if (report & FRAMING_ERROR)
			{
			    msg_printf("Framing Error ");
			    msg_printf("on 98659A at select code ");
			    msg_printf(" %d, ",card_to_sc);
			}

	    		bp->b_error |= EIO;
	    		/* clear byte out, send to dmesg buffer*/
	    		if( ody_enter(PASS_ODY,clear_error,
	    				1,&report) < 0 )
		        {
				msg_printf("Unable to read card buffer on 98659 at ");
				msg_printf("sc %d, semaphore locked \n",card_to_sc);
			}
			else
			{
				msg_printf("char = %02x \n",
					(u_char)clear_error[0]);
			}
	    		break;

	    	  case EOL_EOF_LF:	
	    		/* clear report */
			report = 0;
	    		break;

	    	  default:
	    		/* report is 
	    		** presently undefined but is
	    		** probably an error also */

			msg_printf("Unknown Error ");
			msg_printf("on 98659A at select code ");
			msg_printf(" %d, ",card_to_sc);

	    		/* clear byte out */
	    		bp->b_error |= EIO;

		        x = splx (tp->t_int_lvl);

	    		if( ody_enter(PASS_ODY,clear_error,
	    			1,&report) < 0 )
		        {
				msg_printf("Unable to read card buffer on 98659 at ");
				msg_printf("sc %d, semaphore locked \n",card_to_sc);
			}
			else
			{
			msg_printf("char = %02x \n",
					(u_char)clear_error[0]);
			}

			splx(x);

	    		break;
	        }
	    }
	}
	else	/* WRITE */
	/*	WRITE request */
	/*	If card is responding, there are no errors possible
		on a write request at this level 
		Presently do not block even if NDELAY is clear and
		there is no room in the buffer*/
		
	{
		to_send = bp->b_resid;

		x = splx (tp->t_int_lvl);

		if((sent = ody_output(PASS_ODY, buffer, to_send)) < 0)
		{
			msg_printf("Unable to write to card buffer on 98659 at ");
			msg_printf("sc %d, semaphore locked \n",card_to_sc);
		}
	
		splx(x);

		bp->b_resid -= sent;
	}

	if( bp->b_error != 0)
	{
		bp->b_flags = B_ERROR;
	}

	iodone(bp);
	return(bp->b_error);
}

/****************************************************************************
 * ody_read (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be read from.
 *
 *   ody_read is called each time a read request is made for a specific
 *   serial card.  It, in turn, calls the strategy routine.
 *
 *	Read returns
 *	    FNDELAY (Non-Blocking)
 *
 *	      ICANON
 * 		Will immediately return
 *		    if(line count > 0) [line count == RECEIVE_AVAILABLE]
 *			    return Minimum of
 *				nbytes
 *				bytes to end of line
 *
 *		    else (line count == 0)
 *			    return 0
 *	    ~ICANON
 *		Will immediately return
 *		    return Minimum of
 *			nbytes
 *			bytes available
 *
 *	    !FNDELAY (Blocking)
 *
 *	read 
 *	    ICANON
 *	        if(line count == 0)
 *			sleep until line count > 0
 *		return Minimum of
 *			nbytes
 *			bytes to end of line
 *		
 *	    ~ICANON
 *	        if(min/time not met)
 *			sleep until min/time met
 *		return Minimum of
 *			nbytes
 *			bytes available
 *
 *	RECEIVE_AVAILABLE is used to determine the line count for
 *		canonical processing and whether min/time have been
 *		met in non-canonical processing.
 *		If blocking is required, the interrupt on receive is 
 *		enabled on the card and then the process is put to
 *		sleep until the ISR wakes it up.  It then returns 
 *		with whatever is available on the card.
 *
 *	RECEIVE_AVAILABLE is defined as the following:
 *
 *	  ICANON true  
 *		At least one EOL, EOF, or LF is available in the
 *		input buffer  (This is a canonical line)
 *	     OR	An enabled ERROR (parity or framing) has occurred
 *	     OR A BREAK was received and enabled
 *
 *		RECEIVE_AVAILABLE is the number of control buffers 
 *		available.  If a control block is read, 
 *		RECEIVE_AVAILABLE is decremented.
 *
 *	  ICANON false
 *		At least MIN characters in the buffer 
 *	     OR At least TIME since the last character received
 *	     OR	An enabled ERROR (parity or framing) has occurred
 *	     OR A BREAK was received and enabled
 *
 *	Through the strategy routine read returns:
 *	the minimum of:
 *		the number of bytes available in the card buffer 
 *	     OR the number of bytes requested by the read
 *	     OR the number of bytes up to a control block
 *
 *		Control blocks include EOL_EOF_LF from canonical mode,
 *		  Break Received, or ERROR (Parity or Framing)
 *		    If ERROR, the return value is -1 and errno
 *			is set to EIO.
 *		  For EOL_EOF_LF, if EOL or LF, the EOL or LF is 
 *		    included in the returned stream.  If EOF, the
 *		    EOF character is not returned.
 *		  If Break Received, the null character is discarded
 *	          IF multiple sequential ERRORs, all will be read
 *			in the same read request and an descriptive
 *			message will be written in the system
 *			error message buffer for each character
 *			associated with an error.
 *
 *	Currently the input buffer on the card is 8192 bytes for data 
 *		and 255 locations for errors
 *
 *	If the input buffer or the hardware FIFO buffer overflow because
 *		of a lack of flow control or over 255 control blocks 
 *		without a read, the card is reset, reinitialized, and
 *		reconnected.
 *		All previously saved input characters are thrown away 
 *		similar to the standard TERMIO mode.  In addition, any
 *		characters in the output queue are also thrown away.
 *
 ****************************************************************************/

int
ody_read(dev, uio)
   dev_t  dev;
   struct uio	*uio;
{
int	x;
register struct tty *tp;                  /* Ptr to tty structure */
int	status;

	tp = ody_tp;	/* get the tty pointer */
	x = splx (tp->t_int_lvl);

	if ( !(tp->t_state & TTY) )
	{
		return(EIO);
	}

	if( (status = ody_status( PASS_ODY,RECEIVE_AVAILABLE)) < 0 )
	{
		odydown(tp,"ody_status RECEIVE_AVAILABLE during read");
		return(EIO);
	}
		

	if (uio->uio_fpflags & FNDELAY) /* Non-Blocking */
	{
		if ((tp->t_lflag & ICANON) && (status == 0) )
		{
			splx(x);
			return(0);
		}
		else /* Don't block at all */
		{
			splx(x);
			return(physio(ody_strategy,NULL,dev,
					B_READ,minphys,uio));
		}
	}

	else  /* Blocking */
	{
		if( status == 0 )
		{
		    /* block until RECEIVE_AVAILABLE conditions met */
			/*enable interrupts from card, then sleep */
			tp->t_sicnl |= RXINTEN;
			if( ody_control(PASS_ODY, INTR_COND_REG, 
							tp->t_sicnl) < 0 )
				odydown(tp,"ody_control INTR_COND_REG during blocking read");
			tp->t_state |= IASLP;

			sleep ((caddr_t) PASS_ODY, TTIPRI);

			splx(x);
			return(physio(ody_strategy,NULL,dev,
					B_READ,minphys,uio));
		}
		else  /* RECEIVE_AVAILABLE > 0  */
		{
			splx(x);
			return(physio(ody_strategy,NULL,dev,
					B_READ,minphys,uio));
		}
	}
}


/****************************************************************************
 * ody_write (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be written to.
 *
 *   ody_write is called each time a write request is made for 
 *	a specific serial card.  
 *
 *	write returns with no delay the number of bytes written to the 
 *		card buffers. This is the minimum of the number of bytes
 *		requested or the number of bytes available in the output
 *		queue.  The output queue is currently 6144 bytes long 
 *		which should allow most outputs to take place in a 
 *		single write.  The only error that can occur during a
 *		write is a failure of the card to respond to the command.
 *
 ****************************************************************************/

int
ody_write(dev, uio)

   dev_t  dev;
   struct uio	*uio;
{

register struct tty *tp;                  /* Ptr to tty structure */

	tp = ody_tp;	/* get the tty pointer */
	if ( !(tp->t_state & TTY) )
	{
		return(EIO);
	}
	else
	{
		return(physio(ody_strategy,NULL,dev,B_WRITE,minphys,uio));
	}
}



/****************************************************************************
* ody_ioctl (dev, cmd, arg, mode)
*
*   dev_t dev - Describes which serial card is to be affected.
*   int  cmd - Specifies the ioctl command being requested.
*   char *arg - Points to the passed-in parameter block.
*   int  mode - Specifies the current access modes.
*
*   ody_ioctl is called each time an ioctl request is made for a
*   specific serial card.  
*
*	Unsupported requests or bad values will return -1 with errno EINVAL.
*		Diagnostic messages are placed in the kernel message queue.
*
*  SUPPORTED system calls are as follows
*
*	MODEM(7)
*	    Using an unsigned long as the ioctl arg
*
*		MCGETA will return the state of the 
*			MCTS, MDSR, MDCD, and MRI lines
*			MDSR and MDCD are tied together in the 98659A Cable
*
*		MCSETA, MCSETAW, and MCSETAF 
*			(AW and AF do the wait and flush actions)
*			if MRTS or MCTS is set
*				Hardware Handshaking for the 64700 is
*				enabled.  This handshake uses CTS for 
*				output control and RTS as a "reverse 
*				channel CTS" for input control.
*			if neither MRTS or MCTS is set
*				The Hardware Handshaking is disabled.
*
*			NOTE!  If hardware handshaking is enabled, RR 
*				(DCD) must be set true to enable
*				the receiver to receive.
*			
*			DTR and DRS are set/cleared according to the
*				values of the arguement
*
*		MCGETT and MCSETT are not supported
*			MCSETT returns with no action taken
*			MCGETT	returns a zero in all 6 timer locations
*		
*		None of the monitoring of the MODEM lines or interrupt
*			control are supported.  No other modes besides
*			those described above are supported.
*
*	TERMIO(7)
*	    Using struct termio as the arg
*
*		TCSETA, TCSETAW, TCSETAF, and TCGETA  are supported	
*			for the following flag values
*			(AW and AF do the wait and flush actions)
*
*			c_iflag
*				IGNBRK	(Break Input is always a
*				BRKINT   Null Character with no int)
*				IGNPAR
*				INPCK
*				ISTRIP
*				IXON and IXOFF
*					if either is set, XON/XOFF
*					start/stop is enabled on both
*					input and output
*					In canonical mode, Start/Stop
*					is still valid on input even
*					if no line delimiter is in the
*					input buffer.  This could cause
*					a deadlock if a "line" is longer
*					than input buffer size 
*					(8192 bytes).
*
*				PARMRK Not Supported	(EINVAL)
*				INLCR  Not Supported	(EINVAL)
*				IGNCR  Not Supported	(EINVAL)
*				ICRNL  Not Supported	(EINVAL)
*				IXANY  Not Supported	(EINVAL)
*				IENQAK  Not Supported	(EINVAL)
*			c_oflag
*				Ignored
*				No output processing is supported
*			c_cflag
*				Baud rates supported
*				 B50
*				 B75
*				 B150
*				 B200
*				 B300
*				 B600
*				 B1200
*				 B1800
*				 B2400
*				 B4800
*				 B7200
*				 B9600
*				 B19200
*				 B57600		Defined as Octal 023
*						Can also use EXTA
*				 B230400	Defined as Octal 030
*				 B460800	Defined as Octal 031
*				 EXTA		Also BEXT16X
*				 EXTB		Also BEXT1X
*				If 38400 is chosen on ODYSSEY, use
*					EXTB or BEXT1X
*				If 115200 is chosen on ODYSSEY, use
*					EXTA or BEXT16X
*				B115200 is defined as Octal 024
*				For use with ODYSSEY at 230400 or
*					460800 EXTB or BEXT1 can 
*					also be used
*				IF UNSUPPORTED BAUD IS INPUT, DRIVER
*					RETURNS -1 and errno is EINVAL
*
*				Character size supported
*				 CS5	characters masked to 5 bits
*				 CS6	characters masked to 6 bits
*				 CS7    mask determined by ISTRIP value
*				 CS8    mask determined by ISTRIP value
*
*				CSTOPB
*
*				CREAD
*
*				PARENB
*				PARODD
*
*				HUPCL  Not Supported	No error
*				CLOCAL  Not Supported  No error
*				       All opens complete immediately
*				       and do not depend on modem status
*					lines
*				LOBLK  Not Supported	(EINVAL)
*
*			c_lflag
*				Only 0 supported
*			c_line
*				ICANON
*
*				ISIG	Not Supported	(EINVAL)
*				XCASE	Not Supported	(EINVAL)
*				ECHO	Not Supported	(EINVAL)
*				ECHOE	Not Supported	(EINVAL)
*				ECHOK	Not Supported	(EINVAL)
*				ECHONL	Not Supported	(EINVAL)
*				NOFLSH	Not Supported	(EINVAL)
*			
*			c_cc
*			  	       ICANON TRUE    ICANON FALSE
*				0	Not used	Not Used
*				1	Not used	Not Used
*				2	Not used	Not Used
*				3	Not used	Not Used
*				4	VEOF		VMIN
*				5	VEOL		VTIME
*				6	Not used	Not Used
*				7	Not used	Not Used
*		
*
*	    Using int as the arg
*
*		TCSBRK
*			Wait for output to drain
*				If arg == 0
*					send break of at length 25
*						character times or if
*						external clock 250 mSec
*		TCFLSH
*			If arg is 0 flush input queue
*			If arg is 1 flush output queue
*			If arg is 2 flush both input and output queues
*
*	No Other ioctl commands are supported
*
*	When the system call is to wait for output to drain or be 
*		flushed
*		if output rate is less than 9600 baud and there are more
*			than 256 bytes still in the output buffer queue,
*			the output queue may be flushed, otherwise the 
*			driver will wait for the queue to empty before 
*			finishing
*
****************************************************************************/

int
ody_ioctl(dev, cmd, arg, mode)

   dev_t  dev;
   int   cmd;
   int   *arg;
   int   mode;

{
   register struct tty *tp;                  /* Ptr to tty structure */
   register int x;                           /* Current intr level */
   register err = 0;
   struct termio	cb;
   unsigned long	modemflag;
   struct mtimer	timerconfig;
   int			modemcontrol;

   tp = ody_tp;	/* get the tty pointer */
   /*
    * Check the requested ioctl command, 
    * determine where to process it
    */

    if ( !(tp->t_state & TTY) )
    {
    	return(EIO);
    }

   switch(cmd) 
   {

/* MODEM COMMANDS */

      case MCGETA:		
	 /* Return the current state of the modem lines */
	 odyproc(tp, T_MODEM_STAT);
	 *arg = tp->t_smcnl;
	 break;

      case MCSETAW:
      case MCSETAF:
	 /* Wait for output to drain */
	 ody_wait (tp);
	 if (cmd == MCSETAF)
		/* if AF then flush receive buffer */
		odyproc(tp, T_RFLUSH);

      case MCSETA:
	 /* Set the modem control lines */
	 /* for MRTS or MCTS we enable/disable the 64700 Hardware
	    handshaking.  This uses RTS as a reverse channel CTS
	    to throttle data transmission in both directions */

	 x = splx (tp->t_int_lvl);

	 	modemflag = *((unsigned long *)arg);
		if (modemflag & (MRTS | MCTS ))
		{
			if( ody_control( PASS_ODY, 
				HW_HANDSHAKE_REG, HRD_HANDSKE_ON) < 0)
				odydown(tp);
		}
		else
		{
			if ( ody_control( PASS_ODY,
				HW_HANDSHAKE_REG, HRD_HANDSKE_OFF) < 0)
				odydown(tp);
		}

		/* CCITT_OUT includes MRTS  */
		/* RTS is locally controlled on the card  and if
			send down it is ignored */
		/* DTR is actively used by the application, DRS 
		  (data rate selector) is sent through and if the 
		  cable picks it up, is useable */
		tp->t_smcnl &= ~(CCITT_OUT);
		tp->t_smcnl |= modemflag;
		modemcontrol = 0;
		if(modemflag & MDTR)	modemcontrol |= ODY_DTR;
		if(modemflag & MDRS)	modemcontrol |= ODY_DRS;
		if ( ody_control( PASS_ODY,
				MODEM_REG_8, modemcontrol) < 0)
			odydown(tp);

	 splx (x);
	 break;

	case MCGETT:
	 /* Return the current timer settings */
	 /* always return zero since we have no timers*/
		timerconfig.m_timers[0] = 0;
		timerconfig.m_timers[1] = 0;
		timerconfig.m_timers[2] = 0;
		timerconfig.m_timers[3] = 0;
		timerconfig.m_timers[4] = 0;
		timerconfig.m_timers[5] = 0;
		bcopy( &timerconfig, arg, sizeof timerconfig);
		break;

	case MCSETT:
	 /* Set the timers */
	 /* Timers not supported */
		break;

/* TERMIO COMMANDS */

      case TCSETAW:
      case TCSETAF:
	 /* Wait for output to drain */
		ody_wait(tp);
		if (cmd == TCSETAF)
			/* AF flushes the input queue */
			odyproc(tp, T_RFLUSH);
      case TCSETA:

		bcopy(arg, &cb, sizeof cb);

		/* line discipline and local line modes not
			supported NOFLSH is a don't care*/
		/* Only if a flag value has changed do we
			change anything on the card 
			This increases performance for ioctl
			calls making small changes */

		if (cb.c_line != 0)
		{
			msg_printf("Unsupported c_line ");
			msg_printf("value in termio structure ");
			msg_printf("98659A at ");
			msg_printf("select code %d \n",card_to_sc);
			err = EINVAL;
			break;
		}

		if(tp->t_cflag != cb.c_cflag)
		{
			tp->t_cflag = cb.c_cflag;
			err |= odycontrol(tp);
		}
		if(tp->t_iflag != cb.c_iflag)
		{
			tp->t_iflag = cb.c_iflag;
			err |= odyinput(tp);
		}
		/*no output flags are supported, this is a dummy call */
		if(tp->t_oflag != cb.c_oflag)
		{
			tp->t_oflag = cb.c_oflag;
			err |= odyoutput(tp);
		}
		if(tp->t_lflag != cb.c_lflag)
		{
			tp->t_lflag = cb.c_lflag;
			err |= odylocal(tp);
		}

		tp->t_line = cb.c_line;	/* always 0 */

		if((cb.c_cc[VEOF] != tp->t_cc[VEOF]) ||
				(cb.c_cc[VEOL] != tp->t_cc[VEOL]))
		{
			bcopy(cb.c_cc, tp->t_cc, NCC);
			err |= odycc(tp);
		}

		break;

	case TCGETA:
		/* get current TERMIO settings */
		cb.c_iflag = tp->t_iflag;
		cb.c_oflag = tp->t_oflag;
		cb.c_cflag = tp->t_cflag;
		cb.c_lflag = tp->t_lflag;
		cb.c_line = tp->t_line;
		bcopy(tp->t_cc, cb.c_cc, NCC);
		bcopy(&cb, arg, sizeof cb);
		break;

	case TCFLSH:

		switch (*(int *)arg) {
		case 0:
			/* input buffer only */
			odyproc(tp, T_RFLUSH);
			break;
		case 1:
			/* output buffer only */
			odyproc(tp, T_WFLUSH);
			break;
		case 2:
			/* both buffers */
			odyproc(tp, T_WFLUSH);
			odyproc(tp, T_RFLUSH);
			break;
		default:
			return(EINVAL);
		}
		break;

	case TCSBRK:
		/* send a break after output drains */
		ody_wait(tp);
		if((*(int *)arg) == 0)
			odyproc(tp, T_BREAK);
		break;

	case TCXONC:
		err = EINVAL;
		break;

	default:
		err = EINVAL;
		break;
    }
    if(ody_int_error != 0)
    {
	err = EINVAL;
	ody_int_error = 0;
    }
    return(err);
}


/**********************************************************
*int
* odyparam(tp)
*register struct tty *tp;
*
*		initialize all
*	returns
*		EINVAL if unsupported request
*		EIO if card dies
*		Else 0
***********************************************************
*/

int
odyparam(tp)
register struct tty *tp;
{
	register int	err = 0;

	err = odycontrol(tp);
	err |= odyinput(tp);
/*	err |= odyoutput(tp); this is a don't care*/
	err |= odylocal(tp);
	err |= odycc(tp);

	return(err);
}


/**********************************************************
*int
* odyinput(tp)
*register struct tty *tp;
*
*		initialize input modes
*	returns
*		EINVAL if unsupported request
*		EIO if card dies
*		Else 0
***********************************************************
*/

int
odyinput(tp)
register struct	tty *tp;

{
	register int status = 0;
	register int	scratch;
	register int	err = 0;

/*CHECK INPUT MODE FLAG */

	/*BREAKS ALWAYS BECOME NULL INPUT CHARACTERS*/
	/* IGNBRK and BRKINT are now don't Cares
		jdh 26 SEP 1988 */
/*##*/
	status |= ody_control( PASS_ODY, BRK_CTRL_REG,
						BREAK_NULL );

	if (tp->t_iflag & IGNPAR) 
	/*SET IGNORE ALL INPUT ERRORS*/
	{
/*##*/
		scratch = ody_status(PASS_ODY,CBLOCK_MASK_REG);
		scratch &= ~FR_PR_CTRL;
/*##*/
		status |= ody_control( PASS_ODY, CBLOCK_MASK_REG,
							scratch );
/*##*/
		status |= ody_control( PASS_ODY, ERROR_REPORT_REG,
							NO_ERROR );
	}
	else
	/* want to report parity/framing errors in control blocks */
	{
/*##*/
		scratch = ody_status(PASS_ODY,CBLOCK_MASK_REG);
		scratch |= FR_PR_CTRL;
/*##*/
		status |= ody_control( PASS_ODY, CBLOCK_MASK_REG,
							scratch );
/*##*/
		status |= ody_control( PASS_ODY, ERROR_REPORT_REG,
							REPORT_ERROR );
	}

	if (tp->t_iflag & PARMRK) 
	{
		msg_printf("PARMRK not supported by 98659A at ");
		msg_printf("select code %d \n",card_to_sc);
		err = EINVAL;
	}

	if (tp->t_iflag & INPCK)
/*##*/
	      status |= ody_control( PASS_ODY, PARITY_CHECK,
					ENABLE);
	else
/*##*/
	      status |= ody_control( PASS_ODY, PARITY_CHECK,
					DISABLE);

	/* CHARACTER MASK is set to the size of character when the
	   the CHAR_SIZE_REG is called.  If ISTRIP is set, it is
	   changed to the value of ISTRIP.  IF !ISTRIP, then we
	   always get all 8 bits */

	if (tp->t_iflag & ISTRIP)  /* Always strip to seven bits */
/*##*/
	      status |= ody_control( PASS_ODY, CHAR_MASK_REG,
					0x07f);
	else if (tp->t_cflag & CS7) /* bit is set for CS7 or CS8 */
/*##*/
	      status |= ody_control( PASS_ODY, CHAR_MASK_REG,
					0x0ff);
	/* else leave mask as set by CHAR_SIZE_REG (For CS5 and CS6) */

	/* INLCR, IGNCR, ICRNL, IUCLC not supported */
	/* IXANY Not supported */
	if (tp->t_iflag & (INLCR | IGNCR | ICRNL | IUCLC | IXANY)) 
	{
		msg_printf("INLCR, IGNCR, ICRNL, IUCLC, or IXANY ");
		msg_printf("not supported by 98659A at ");
		msg_printf("select code %d \n",card_to_sc);
		err = EINVAL;
	}

	if (tp->t_iflag & (IXON | IXOFF)) 
	{
		/* turn XON_XOFF_HS on if not on yet */
/*##*/
		if (ody_status( PASS_ODY, HANDSHAKE_REG) != XON_XOFF_HS)
/*##*/
			status |= ody_control( PASS_ODY, HANDSHAKE_REG,
							XON_XOFF_HS);
	}
	else
	{
/*##*/
		status |= ody_control( PASS_ODY, HANDSHAKE_REG,
						NO_SHSHAKE);
	}

	/* IENQAK Not supported */
	if (tp->t_iflag & IENQAK) 
	{
		msg_printf("IENQAK not supported by 98659A at ");
		msg_printf("select code %d \n",card_to_sc);
		err = EINVAL;
	}

	if (status < 0)
	{
		err = EIO;
		odydown(tp,"during iflag configuration");
	}
	return(err);
}


/**********************************************************
*int
* odyoutput(tp)
*register struct tty *tp;
*
*		initialize output modes
*		dummy, no output modes supported
*
*	returns
*		0
***********************************************************
*/

int
odyoutput(tp)
register struct tty *tp;
{
	return(0);
}


/**********************************************************
*int
* odycontrol(tp)
*register struct tty *tp;
*
*		initialize control modes
*	returns
*		EINVAL if unsupported request
*		EIO if card dies
*		Else 0
***********************************************************
*/

int
odycontrol(tp)
register struct	tty *tp;

{
	register int flags;
	register int status = 0;
	int	scratch;
	register int	err = 0;

/*CHECK CONTROL MODE FLAG */

	flags = tp->t_cflag;
	/* check for valid baud rates */
	switch (flags & CBAUD)
	{
	  case B50:
	  case B75:
	  case B150:
	  case B200:
	  case B300:
	  case B600:
	  case B1200:
	  case B1800:
	  case B2400:
	  case B4800:
	  case B7200:
	  case B9600:
	  case B19200:
	  case B57600:
	  case B230400:
	  case B460800:
	  case EXTA:
	  case EXTB:
		break;
	  case B38400:
	  case B115200:
	  default:
		msg_printf("Unsupported Baud Rate of ");
		msg_printf("%07o octal for 98659A at ",(flags & CBAUD));
		msg_printf("select code %d \n",card_to_sc);
		err = EINVAL;
		return (err);
		break;
	}

	  /* set both transmit and recieve baud registers */
/*##*/
	status |= ody_control( PASS_ODY, BAUD_REG, flags & CBAUD );
/*##*/
	status |= ody_control( PASS_ODY, BAUD_REG+1, flags & CBAUD );

/*##*/
	status |= ody_control( PASS_ODY, CHAR_SIZE_REG,
					(flags&CSIZE)/CS6 );

	/* CHARACTER MASK is set to the size of character when the
	   the CHAR_SIZE_REG is called.  If ISTRIP is set, it is
	   changed to the value of ISTRIP.  IF !ISTRIP, then we
	   always get all 8 bits */

	if (tp->t_iflag & ISTRIP)  /* Always strip to seven bits */
/*##*/
	      status |= ody_control( PASS_ODY, CHAR_MASK_REG,
					0x07f);
	else if (tp->t_cflag & CS7) /* bit is set for CS7 or CS8 */
/*##*/
	      status |= ody_control( PASS_ODY, CHAR_MASK_REG,
					0x0ff);
	/* else leave mask as set by CHAR_SIZE_REG (For CS5 and CS6) */
	
	if (flags & CSTOPB)
/*##*/
	      status |= ody_control( PASS_ODY, STOP_BIT_REG,
					TWO_STOP_BITS);
	else
/*##*/
	      status |= ody_control( PASS_ODY, STOP_BIT_REG,
					ONE_STOP_BIT);

	/* enable receive on RX */
	if (flags & CREAD)
/*##*/
		status |= ody_control( PASS_ODY, CONNECT_REG, RECEIVE);

	if (flags & PARENB) 
	{
		if (flags & PARODD) 
/*##*/
			status |= ody_control( PASS_ODY, PARITY_REG, 
							PARITY_ODD);
		else
/*##*/
			status |= ody_control( PASS_ODY, PARITY_REG,
							PARITY_EVEN);
	}
	else
/*##*/
		status |= ody_control( PASS_ODY, PARITY_REG, 
						PARITY_NONE);

	/* HUPCL CLOCAL LOBLK are not supported */
	/* CLOCAL does not generate an error message */
	/* HUPCL does not generate an error message */
	/* LOBLK and CRTS are the same */
	if (flags & LOBLK) 
	{
		msg_printf("LOBLK / CRTS ");
		msg_printf("not supported by 98659A at ");
		msg_printf("select code %d \n",card_to_sc);
		err = EINVAL;
	}

	if (status < 0)
	{
		err = EIO;
		odydown(tp,"during cflag configuration");
	}
	return(err);
}


/**********************************************************
*int
* odylocal(tp)
*register struct tty *tp;
*
*		initialize local modes
*	returns
*		EINVAL if unsupported request
*		EIO if card dies
*		Else 0
***********************************************************
*/

int
odylocal(tp)
register struct	tty *tp;

{
	register int status = 0;
	register int	scratch;
	register int	err = 0;

/*CHECK LOCAL MODE FLAG */
	/* Only ICANON is supported */

	if (tp->t_lflag & ICANON)
	{
/*##*/
		status |= ody_control( PASS_ODY, CANONICAL_REG, ON);
/*##*/
		scratch = ody_status(PASS_ODY,CBLOCK_MASK_REG);
		scratch |= EOL_POS;
/*##*/
		status |= ody_control( PASS_ODY, CBLOCK_MASK_REG,
							scratch );
	}
	else
	{
/*##*/
		status |= ody_control( PASS_ODY, CANONICAL_REG, OFF);
/*##*/
		scratch = ody_status(PASS_ODY,CBLOCK_MASK_REG);
		scratch &= ~EOL_POS;
/*##*/
		status |= ody_control( PASS_ODY, CBLOCK_MASK_REG,
							scratch );
	}

	if (status < 0)
	{
		err = EIO;
		odydown(tp,"during local configuration");
	}
	return(err);
}


/**********************************************************
*int
* odycc(tp)
*register struct tty *tp;
*
*		initialize control characters
*			only VEOF and VEOL are supported
*				all others are ignored
*	returns
*		EINVAL if unsupported request
*		EIO if card dies
*		Else 0
***********************************************************
*/

int
odycc(tp)
register struct	tty *tp;

{
	register int status = 0;
	register int	err = 0;

	/* set control characters */
/*##*/
	status |= ody_control( PASS_ODY, VEOF_VMIN_REG,
				tp->t_cc[VEOF]);
/*##*/
	status |= ody_control( PASS_ODY, VEOL_VTIME_REG,
				tp->t_cc[VEOL]);

	if (status < 0)
	{
		err = EIO;
		odydown(tp,"during control character configuration");
	}
	return(err);
}


/****************************************************************************
 *ody_select(dev,flag) -	select routine
 *
 *   dev_t dev - Describes which serial card to select on
 *   int flag - Specifies whether to select on READ or WRITE
 *
 *	This describes the action of the low level select routine which
 *		is called by the system select routine.
 *
 *	If READ
 *		IF ( RECEIVE_AVAILABLE )
 *			Returns true
 *		ELSE IF Someone else is already waiting on 
 *			select
 *			Set Collision
 *		ELSE
 *			set up wakeup parameters
 *			request an interrupt from card when 
 *				data is available
 *			Return False so kernel select can 
 *				put us to sleep
 *	If WRITE
 *		IF(WRITE_BUFFER > 1024) (out of a 6K byte buffer) 
 *			Returns true
 *		Else if Someone else is already waiting on select
 *			Set Collision
 *		Else
 *			set up wakeup parameters
 *			request an interrupt from card when room is 
 *				available
 *			Return False so kernel select can put us to 
 *				sleep
 *
 ****************************************************************************/

ody_select(dev,flag)
dev_t	dev;
int	flag;
{
	register struct	tty	*tp;
	int	status;
	int	x;
	int	retval;

	
	tp = ody_tp;

	retval = FALSE;

	/* always return true if card has previously died */
	if ( !(tp->t_state & TTY) )
	{
    		return(TRUE);
	}

	x = splx(tp->t_int_lvl);

	switch(flag)
	{
	    case(FREAD):
		/* Blocking */
	        status = ody_status( PASS_ODY, RECEIVE_AVAILABLE);
		if( status < 0 )
		{
			odydown(tp,"ody_status RECEIVE_AVAILABLE during select read");
			retval = TRUE;
		}
		else if (status > 0)
		{
		    retval = TRUE;
		}
	        else if (tp->t_rsel->p_wchan == (caddr_t)&selwait)
		{
		    tp->t_state |= RCOLL;
		}
	        else
	        {
		    tp->t_rsel = u.u_procp;
		    /*enable interrupts from card, then sleep */
		    tp->t_sicnl |= RXINTEN;
		    if( ody_control(PASS_ODY, INTR_COND_REG, 
						tp->t_sicnl) < 0)
		    {
			odydown(tp,"ody_control INTR_COND_REG during select read");
			retval = TRUE;
		    }
	        }
	        break;

	    case(FWRITE):
		status = ody_status( PASS_ODY,
					TRANSMIT_AVAILABLE);
		if( status < 0 )
		{
			odydown(tp,"ody_status TRANSMIT_AVAILABLE during select write");
			retval = TRUE;
		}
		if (status > 1024 )
		{
			retval = TRUE;
		}
		else if (tp->t_wsel->p_wchan == (caddr_t)&selwait)
		{
			tp->t_state |= WCOLL;
		}
		else
		{
			tp->t_wsel = u.u_procp;
			/*enable interrupts from card, then sleep */
			tp->t_sicnl |= TXINTEN;
			if( ody_control(PASS_ODY, INTR_COND_REG, 
						  tp->t_sicnl) < 0 )
		        {
			    odydown(tp,"ody_control INTR_COND_REG during select write");
			    retval = TRUE;
		        }
		}
		break;
	}
	splx(x);
	return(retval);
}


/****************************************************************************
 ody_intr -	interrupt service routine
 ****************************************************************************/

ody_intr(inf)
struct interrupt *inf;
{
	register struct tty *tp = (struct tty *) inf->temp;
	register unsigned char status;

	/* read interrupt status */
	status = *((unsigned char *)(tp->t_card_ptr + INT_COND_OFFSET));
	/* ERROR INTERRUPT */
	if(status & ERR_INT)
	{
		/* take care of it outside of the interrupt routine */
		sw_trigger(&tp->rcv_intloc,ody_intr_0,tp
					,0,tp->t_int_lvl);
	}

	/* DATA AVAILABLE INTERRUPT */
	if(status & DATA_AVAILABLE)
	{
		/* only wakeup those who are asleep */

		if (tp->t_state & IASLP)
		{
			/* from read request */
			wakeup((caddr_t) PASS_ODY);
			tp->t_state &= ~IASLP;
			/* disable receive interrupts from the card  */
			tp->t_sicnl &= ~RXINTEN;
			if( ody_control(PASS_ODY, INTR_COND_REG, 
							tp->t_sicnl) < 0 )
				odydown(tp,"ody_control INTR_COND_REG during intr");
		}
		if(tp->t_rsel)
		{
			/* from select request */
			/* If SSEL is true, we are in a critical
			   section of the select routine, donot wakeup,
			   instead clear the SSEL bit.  Select will
			   retry ody_select which will now return true
			   tr->t_rsel is a copy of u.u_procp.  This
			   is duplicated in the Write section.*/
			
			if ( (tp->t_rsel->p_flag & SSEL) == 0)
			{
				selwakeup(tp->t_rsel,tp->t_state & RCOLL);
			}
			else
			{
				tp->t_rsel->p_flag &= ~SSEL;
			}
			tp->t_rsel = 0;
			tp->t_state &= ~RCOLL;
			/* disable receive interrupts from the card  */
			tp->t_sicnl &= ~RXINTEN;
			if( ody_control(PASS_ODY, INTR_COND_REG, 
							tp->t_sicnl) < 0 )
				odydown(tp,"ody_control INTR_COND_REG during intr");
		}
	}


	/* OUTPUT SPACE AVAILABLE INTERRUPT */
	if(status & WRITE_AVAILABLE)
	{
		if(tp->t_wsel)
		{
			/* only from select routine */
			if ( (tp->t_wsel->p_flag & SSEL) == 0)
			{
				selwakeup(tp->t_wsel,tp->t_state & WCOLL);
			}
			else
			{
				tp->t_wsel->p_flag &= ~SSEL;
			}
			tp->t_wsel = 0;
			tp->t_state &= ~WCOLL;
			/* disable transmit interrupts from the card  */
			tp->t_sicnl &= ~TXINTEN;
			if( ody_control(PASS_ODY, INTR_COND_REG, 
							tp->t_sicnl) < 0 )
				odydown(tp,"ody_control INTR_COND_REG during intr");
		}
	}

	if(status & MODEM_STATUS)
	{
		status = ody_status(PASS_ODY, INTR_REG);
		if(status < 0)
			odydown(tp,"ody_status INTR_REG during intr");
		else if(status & BREAK_RECEIVED)
		{
			/* BRKINT was enabled */
			odyproc(tp, T_WFLUSH);
			odyproc(tp, T_RFLUSH);
			signal(tp->t_pgrp,SIGINT);
		}
	}
}


/****************************************************************************
 ody_intr_0 -	software trigger service routine
 ****************************************************************************/
ody_intr_0(tp)
register struct tty *tp;
{
	register char status;
	register int transerr, out;
	register int x;
	ushort c, report;

	x = splx(tp->t_int_lvl);

	status = *((char *)(tp->t_card_ptr + ERROR_CODE_OFFSET));
	/* clear out the error reporting byte */
	*((char *)(tp->t_card_ptr + ERROR_CODE_OFFSET)) = 0x00;

	splx(x);

	switch(status)
	{	
		case 0:
			break;
		case REG_ADDR_BAD:
			msg_printf("BAD REGISTER VALUE value to 98659 ");
			msg_printf("at select code %d \n",card_to_sc);
			ody_int_error = status;
			break;	
		case REG_VALUE_BAD:
			msg_printf("BAD REGISTER ADDR value to 98659 ");
			msg_printf("at select code %d \n",card_to_sc);
			ody_int_error = status;
			break;	
		case SELFTESTFAIL:
			/* Get SELFTESTFAILTYP	*/
			status = *((char *)(tp->t_card_ptr + 
						SELFTESTFAILTYP_OFFSET));
			if(status & (char)ROM_FAIL)
			    msg_printf("98659 at sc %d ROM TEST FAILED \n",
				card_to_sc);
			if(status & (char)RAM_FAIL)
			    msg_printf("98659 at sc %d RAM TEST FAILED \n",
				card_to_sc);
			if(status & (char)CTC_FAIL)
			    msg_printf("98659 at sc %d CTC TEST FAILED \n",
				card_to_sc);
			if(status & (char)SIO_FAIL)
			    msg_printf("98659 at sc %d SIO TEST FAILED \n",
				card_to_sc);
			if(status & (char)SEM_FAIL)
			    msg_printf("98659 at sc %d SEMAPHORE TEST FAILED \n",
				card_to_sc);
			break;
		case INPUT_OVERRUN:
			msg_printf("98659 at sc %d INPUT OVERRUN\n",
				card_to_sc);

			if( (ody_control( PASS_ODY, CLEAR_REG, 1)) < 0 )
				odydown(tp);
			/*Reconnect at current level */
			if( (report = ody_status( PASS_ODY, LINE_STATE)) < 0 )
				odydown(tp);
			if( (ody_control( PASS_ODY, CONNECT_REG, report)) < 0 )
				odydown(tp);

			break;
		case RX_OVERFLOW:
			msg_printf("98659 at sc %d RX BUFFER OVERFLOW\n",
				card_to_sc);

			if (ody_control( PASS_ODY, CLEAR_REG, 1) < 0)
				odydown(tp);
			/*Reconnect at current level */
			if( (report = ody_status( PASS_ODY, LINE_STATE)) < 0 )
				odydown(tp);
			if( (ody_control( PASS_ODY, CONNECT_REG, report)) < 0 )
				odydown(tp);
			break;
		default:
			msg_printf("Unknown error on 98659A at ");
			msg_printf("select code %d \n",card_to_sc);

			reset_card(tp);
			break;
	}
	ody_errors++;	/* keep track of errors */
	msg_printf("98659 at sc %d, %d total errors\n", card_to_sc, ody_errors);

	return;

}

reset_card(tp)
register struct tty *tp;
{
	register int x;

	/* reset the card because it lost its mind */

	x = splx(tp->t_int_lvl);

	*((char *)(tp->t_card_ptr + RESET_OFFSET)) = 0x80;
	*((char *)(tp->t_card_ptr + RESET_OFFSET)) = 0x00;

	splx(x);

	/* wait for 400ms to settle card */
	snooze(400000);	

	x = splx(tp->t_int_lvl);

		/* set to current state */
	if(tp->t_state & ISOPEN)
	{
		if ((ody_control( PASS_ODY, CONNECT_REG, CONNECT)) < 0)	
			odydown(tp);
	}

	ody_setup(tp);	/* re-initialize the card */
	odyparam(tp);/* set to current states */

	splx(x);

	msg_printf("98659 at sc %d ERROR REPORTED, RESET\n", card_to_sc);
}


/****************************************************************************
 odyproc -	
 ****************************************************************************/

void
odyproc(tp, cmd)
register struct tty *tp;
{
    register c;
    register int x;

    switch(cmd) 
    {
	case T_TIME:
		/* used in the TCSBRK routine */
		tp->t_slcnl &= ~SETBRK;
		tp->t_state &= ~TIMEOUT;
		break;

	case T_WFLUSH:
		/* clear card write buffers and any 
				pending interrupt */

		x = splx(tp->t_int_lvl);

		if (ody_control( PASS_ODY, CLEAR_OUTPUT, 0) < 0)
			odydown(tp);

		splx(x);

		break;

	case T_RFLUSH:
		/* clear card read buffers and any 
				pending interrupt */

		x = splx(tp->t_int_lvl);

		if (ody_control( PASS_ODY, CLEAR_INPUT, 0) < 0)
			odydown(tp);

		splx(x);

		break;

	case T_BREAK:
		/* used in the TCSBRK routine */
		tp->t_slcnl |= SETBRK;
		if (ody_control( PASS_ODY, BREAK_REG, 0) < 0)
			odydown(tp);
		tp->t_state |= TIMEOUT;
		timeout(odystrt, tp, HZ/4, NULL);
		break;

	case T_MODEM_STAT:
		{
			/* return the state of all the modem
			   status and control lines */
			register int modem;
			register int smcnl = tp->t_smcnl;
			/* read modem status */

			x = splx(tp->t_int_lvl);

			if( (modem = ody_status(PASS_ODY,
					MODEM_REG_7)) < 0 )
				odydown(tp);


			smcnl &= ~(CCITT_IN);
			if (modem & ODY_CTS)	smcnl |= MCTS;
			if (modem & ODY_DSR)	smcnl |= MDSR;
			if (modem & ODY_DCD)	smcnl |= MDCD;
			if (modem & ODY_RI )	smcnl |= MRI ;

 			if( (modem = ody_status(PASS_ODY,
 					MODEM_REG_8)) < 0 )
 				odydown(tp);

 			if(modem & ODY_RTS)	smcnl |= MRTS;
 			if(modem & ODY_DTR)	smcnl |= MDTR;
 			if(modem & ODY_DRS)	smcnl |= MDRS;
			
			tp->t_smcnl = smcnl;

			splx(x);
		}
		break;
    }
}

void
odystrt(tp)
register struct tty *tp;

{
	odyproc(tp, T_TIME);
}




/**********************************************************************
 * This routine returns when the
 * 98659 output buffer is empty.
 * if the buffer does is not emptying at a reasonable rate
 * it will flush the buffer and then return
 * A reasonable rate is at least 256 bytes or the entire
 * buffer out in less than 350 mSec.
 * It gets impatient quickly on slower than 9600 baud rates
 **********************************************************************
 */
ody_wait(tp)
register struct tty *tp;
{
	register int x,y;

	y = splsx(tp->t_int_lvl);
	x = spl6();
	/* use t_time as a temporary holder for present number
		of bytes still in output queue */
	tp->t_time = 0;
	if (ody_try_queue( tp))
	{
	/* wait for the queue to empty or flush queue if it
		is not being emptied, that is if it is stuck */


		/* the queue is not empty so let's sleep */
		splsx(y);
		sleep((caddr_t) PASS_ODY, TTOPRI);
		/* wait for UART (@ 150 baud ??) to empty */
		delay(HZ/15);
		y = splsx(tp->t_int_lvl);
	}
	splx(x);
	splsx(y);
	return;
}

/**********************************************************************
 * This routine checks to see if the output buffer is empty
 * 	IF empty
 *		wake up anyone sleeping on PASS_ODY
 *	If no change since last call 
 *		Flush output buffer
 *		wake up anyone sleeping on PASS_ODY
 *	Else
 *		sleep for 350 mSeconds and try again
 **********************************************************************
 */
ody_try_queue(tp)
register struct tty *tp;
{
	register int	temp;
	if ((temp = ody_status( PASS_ODY, OUTPUT_QUEUE )) == 0 )
	{	 /* yes the buffer is empty */
		wakeup((caddr_t) PASS_ODY);
		return(0);
	}
	else if(temp < 0) /* error on ody_status */
	{	/* return anyway after calling odydown*/
		odydown(tp,"ody_status OUTPUT_QUEUE during wait for empty");
		wakeup((caddr_t) PASS_ODY);
		return(0);
	}
	else if(temp == tp->t_time) /* no change from last call */
	{
		/*flush output queue */
		odyproc(tp, T_WFLUSH);
		 /* now the buffer is empty */
		wakeup((caddr_t) PASS_ODY);
		return(0);
	}
	else	
	/* lets try a little later (350 msecs) */
	/* will handle output rates >= 9600 baud */
	{
		tp->t_time = temp;
		timeout(ody_try_queue, tp, 20, NULL);
		return(1);
	}
}


odydown(tp,string)
register struct tty *tp;
char	*string;

/*
*	Called when 98659A refuses to respond to a
*	command or status request
*/

{
	msg_printf("98659 at sc %d failed to acknowledge %s\n",
					card_to_sc,string);
	tp->t_state &= ~TTY;
	tp->tty_count = 0;	/* allow next open to
				   reinitialize card */
}



extern int (*make_entry)();
int (*ody_saved_make_entry)();

/*
** linking and inititialization routines
*/

ody_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	if (id == PDI_ID_CODE) 
		if(data_com_type(isc) == ID_98659) /*returns MY_ID */
		{
			register struct tty *tp;

			/* make tty structure now */
			tp = (struct tty *)calloc(sizeof(struct tty));
			ody_tp_ptr = tp;	/* and save it */

			/* initialize card_ptr and int_lvl */
			tp->t_card_ptr = (char *)isc->card_ptr;
			tp->t_int_lvl = isc->int_lvl;

			/* initialize the isc_table structure */
			isc->card_type = HP98659; /* Need to Add HP98659 to hpibio.h */

			isrlink(ody_intr, isc->int_lvl,
				(int) isc->card_ptr + INT_STATUS_OFFSET,
				0xc0, 0xc0, 0, tp);

			ody_power_up(tp);

			return(io_inform( "HP98659 RS-422 64700 Interface",
					isc, isc->int_lvl));
			/* when io_inform receives isc->int_lvl, it
			will return true, which alows for the temporary
			isc structure to be copied into an allocated 
				permanent structure */
		}
	return(*ody_saved_make_entry)(id, isc);
}


void
ody_link()
{
	ody_saved_make_entry = make_entry;
	make_entry = ody_make_entry;
}


ody_power_up(tp)
register struct tty *tp;
{

	tp->t_state = 0;

		/* setup for initial values of B300, CS8, and CREAD. */

	tp->t_iflag = (IGNBRK | IGNPAR);
	tp->t_oflag = NULL;
	tp->t_cflag = (B300 | CS8 | CREAD);
	tp->t_lflag = NULL;
	tp->t_line = NULL;
	tp->t_cc[VINTR] = NULL;	/* Not used */
	tp->t_cc[VQUIT] = NULL;	/* Not used */
	tp->t_cc[VERASE] = NULL;	/* Not used */
	tp->t_cc[VKILL] = NULL;	/* Not used */
	tp->t_cc[VEOF] = NULL;	/* also VMIN */
	tp->t_cc[VEOL] = NULL;	/* also VTIME */
	tp->t_cc[6] = NULL;	/* Not used */
	tp->t_cc[7] = NULL;	/* Not used */
	tp->tty_count = 0;
					
	PASS_ODY = (int *)calloc(ODYSIZE);

	/* let ody drivers init their variables */
	ody_init( PASS_ODY,tp->t_card_ptr);

	ody_setup(tp);

	odyparam(tp);/* set to current states */

	/* Disable all card level interrupts */
	*((char *)(tp->t_card_ptr + INT_ENABLE_OFFSET)) = 0x00;

	return;
}


/***************************************************************************
 ody_setup -	set up the card (done at powerup for each card)
 ***************************************************************************/
ody_setup(tp)
register struct tty *tp;
{
	unsigned char status;

	/* clear interrupt */
	status = *((unsigned char *)(tp->t_card_ptr + INT_COND_OFFSET));
	/* reset the card */
	if( (ody_control( PASS_ODY, CLEAR_REG, 1)) < 0 )
		odydown(tp);

				/* ERR_INT */
	tp->t_sicnl = ERR_INT;
	if( (ody_control( PASS_ODY, INTR_COND_REG,
					tp->t_sicnl)) < 0 )
		odydown(tp);

	/*do not want to report parity/framing errors in 
		control blocks */
	if( (ody_control( PASS_ODY, CBLOCK_MASK_REG,
					0 )) < 0)
		odydown(tp);

	/* turn off hardware handshake */
	if( (ody_control( PASS_ODY, HW_HANDSHAKE_REG,
					HRD_HANDSKE_OFF)) < 0)
		odydown(tp);

 	/* set break time to default of 25 characters */
 	if( (ody_control( PASS_ODY, BREAK_VALUE_REG, 25)) < 0)
		odydown(tp);
}
