/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/ddb_rs232.c,v $
 * $Revision: 1.2.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:03:46 $
 */

#ifdef DDB

#include	"ddb_sio.h"
#include	"../h/param.h"

char iobuf[256] = { 0 };
int iobuf_start = 0;
int iobuf_end = 0;

#define TRUE	1
#define FALSE	0
#define OK	0
#define	ERROR	-1
#define	NULL	0
/* FUNCTIONS AND SUBROUTINES */

/*
 * rs232_receive_pending - determines if a character is present in
 *	the input fifo of the requested rs232 port.  It does so by checking
 *	the LSB of the line status register.
 *
 *	Outputs: status - 0 for character present and -1 for no character
 */
int rs232_receive_pending(sio_chan_ptr)
	SIO_CHAN_REGS *sio_chan_ptr;
{
	unsigned int temp;	/*temporary storage */

	if (iobuf_start != iobuf_end)
		return 1;
	/* check for character present in the fifo */
	temp = sio_chan_ptr->line_status_reg;
	delay_for_sio();
	return ((temp & LSR_DATA_READY_MASK) == LSR_DATA_READY);
}

/*
 * rs232_getchar - returns a single character from the rs232 port
 * 	fifo selected by the passed address.  In doing so it checks to see
 *	if a character has been received by looking at the data ready bit
 *	(LSB) of the line status register.  If no character was present a
 *	value of -1 is returned.
 *
 *	NOTE: This routine currently sends pacing characters back to the
 *	calling program to be handled there.
 *
 *	Outputs:
 *		status - NULL if no character present, otherwise value of
 *			character read.
 */
char rs232_getchar(sio_chan_ptr)
	SIO_CHAN_REGS	*sio_chan_ptr;
{
	unsigned int temp;	/*temporary storage */
	unsigned char found;	/* temporary storage for input char */

	if (iobuf_end > iobuf_start)
		return iobuf[iobuf_start++ % sizeof(iobuf)];

	/* check for character present in the fifo */
	temp = sio_chan_ptr->line_status_reg;
	delay_for_sio();
	if ((temp & LSR_DATA_READY_MASK) == LSR_DATA_READY) {

	/* check for good line status before getting character */
		if ((temp & LSR_ERROR_MASK) == 0) {
			found = sio_chan_ptr->rec_buffer_reg;
			delay_for_sio();
			return found;
		} else {
			sio_chan_ptr->fifo_cntr_reg = FCR_REC_FIFO_CLEAR;
			delay_for_sio();
			delay_for_sio();
		}
	}
	return(NULL);
}

/*
 * rs232_putchar - sends a characte out on the selected rs232 port.
 *	Prior to sending out a character it checks to make sure that no
 *	pacing characters have been received.  If an XOFF was received then
 *	it waits until an XON is received before sending the  character.
 *
 *	Additionally, a check is made to determine if the transmitter is
 *	empty prior to sending the character.  If the transmitter is not
 *	empty it waits long enough for an 11 bit character (longest possible
 *	1 start, 8 bits/char, 2 stop) and tries again.
 *
 *	NOTE: character read from the input fifo while attempting to find
 *	pacing character will be discarded.  A -1 will be returned to
 *	indicate this situation.
 *
 *	NOTE: This routine waits for the character to be sent before
 *	returning to the caller.
 *
 *	Outputs:
 *		status - NULL if character is transmitted and nothing lost
 *			from the input fifo.  -1 indicates contents of the
 *			input fifo were lost.
 */

int rs232_putchar(sio_chan_ptr,out_char)
	SIO_CHAN_REGS	*sio_chan_ptr;
	char out_char;
{
	int check_for_pace_chars();	/* function declaration */
	unsigned int temp;	/*temporary storage */
	unsigned int timeout = FALSE;	/* timeout variable */

	/* check for pacing characters.  Make sure XON before sending */
	check_for_pace_chars(sio_chan_ptr);

	temp = sio_chan_ptr->line_status_reg;
	delay_for_sio();
	/* check to see if the transmitter is empty */
	if ((temp & LSR_THR_EMPTY_MASK) != LSR_THR_EMPTY) {

           /*if you got here, it means transmitter is not empty yet, loop */
	      timeout=TRUE;
              while(timeout)
              {
                 temp = sio_chan_ptr->line_status_reg;
		 delay_for_sio();
                 if ((temp & LSR_THR_EMPTY_MASK) == LSR_THR_EMPTY)
                 {
		    sio_chan_ptr->trans_holding_reg = out_char;
		    delay_for_sio();
		    delay_for_sio();
                    timeout = FALSE;
                 }
              }/*while*/

	} /* if */

	sio_chan_ptr->trans_holding_reg = out_char;
	delay_for_sio();

        /*we get out of this while loop either after we finished outputing
        all characters or there is an error.  If there is no error, wait till
        the last character outputed leaves the transmit buffer properly
        and then return*/

        temp = sio_chan_ptr->line_status_reg;
	delay_for_sio();

        /*if you got here, it means all characters were outputted with
        no error, we have to wait for EMPTY */

	timeout = TRUE;
        while(timeout)
        {
               temp = sio_chan_ptr->line_status_reg;
	       delay_for_sio();
               if ((temp & LSR_TRANS_EMPTY) == LSR_TRANS_EMPTY)
               {
                 timeout = FALSE;
               }
        }/*while*/

        return(0);

} /* rs232_putchar */

/*
 * rs232_init - this routine initializes the requested device by
 *	by setting the device and module registers to known values.
 *
 *	Registers inside of the WD16C552 chip are divided into module and
 *	device registers.
 *
 *	Default configuration is:
 *			9600 baud rate
 *		 	8 bits/char, 1 stop bits
 *			no parity bits
 */
#define	SNAKES_RS232_CONFIG	0x283
/* This stuff is needed for 4800 baud on s300 UARTs
#define	SNAKES_RS232_CONFIG	0x203
#define BAUD4800		0x200
#define BAUD4800LSB		0x20
#define BAUD4800MSB		0x0
*/
rs232_init(sio_chan_ptr)
	SIO_CHAN_REGS	*sio_chan_ptr;
{
	unsigned char	temp, temp1;	/*temporary storage*/
	int	baud;
        struct timeval start_time;   /* time at beginning of timeout period */

     /* initialize the module registers */
	
	/* Set IER to no interrupts enabled, FCR to FIFO enable, no DMA and
	   interrupt level set to 1. */
	sio_chan_ptr->intr_enable_reg = IER_INT_DISABLE;
	delay_for_sio();
	sio_chan_ptr->fifo_cntr_reg = FCR_FIFO_MODE;
	delay_for_sio();

	/* verify configuration by reading IIR */
	temp = sio_chan_ptr->intr_ident_reg;
	delay_for_sio();

    /* initialize the device registers */

	/* Mask off (only because IODC did this) the specific bits */
	/* from the layer structure needed to configure the port   */
	/* Then verify the setting.				   */
	temp = (SNAKES_RS232_CONFIG & LCR_LAYER_MASK) | LCR_DLAB_SET;
	sio_chan_ptr->line_cntr_reg = temp;
	delay_for_sio();
	temp1 = sio_chan_ptr->line_cntr_reg;
	delay_for_sio();

	/* Mask the baudrate indicators from the configuration value */
	/* and set the device.					     */
	baud = SNAKES_RS232_CONFIG & DL_LAYER_MASK;
	
	/* switch on baud and depending on its value, write the proper */
	/* values to DLLSB and DLMSB.				       */
        switch(baud)
        {

/* This stuff is needed for 4800 baud on s300 UARTs
        case BAUD4800:
           sio_chan_ptr->divisor_latch_lsb = BAUD4800LSB;
	   delay_for_sio();
           sio_chan_ptr->divisor_latch_msb = BAUD4800MSB;
	   delay_for_sio();
           break;
*/

        case BAUD9600:
           sio_chan_ptr->divisor_latch_lsb = BAUD9600LSB;
	   delay_for_sio();
           sio_chan_ptr->divisor_latch_msb = BAUD9600MSB;
	   delay_for_sio();
           break;

        case BAUD19200:
           sio_chan_ptr->divisor_latch_lsb = BAUD19200LSB;
	   delay_for_sio();
           sio_chan_ptr->divisor_latch_msb = BAUD19200MSB;
	   delay_for_sio();
           break;

        } /*switch*/

        /*reset the DLAB bit in LCR*/

        temp = sio_chan_ptr->line_cntr_reg;
	delay_for_sio();
        sio_chan_ptr->line_cntr_reg = temp & LCR_DLAB_RESET;
	delay_for_sio();

	/* clear the transmit/receive FIFO and enable FIFO mode. Before
	 * any other bits of FCR can be written, the fifo enable bit (0)
	 * should be set 
	 */
	sio_chan_ptr->fifo_cntr_reg = FCR_FIFO_MODE;
	delay_for_sio();
	sio_chan_ptr->fifo_cntr_reg = FCR_FIFOS_CLEAR_AND_ENABLE;
	delay_for_sio();

	/* wait 10 MSEC */
        get_precise_time(&start_time);
        while (!has_timeout_occurred(&start_time, 1));

	/* read the LSR and if it says there is data in the receiver FIFO,
	   read the data and discard */
	if (((temp=sio_chan_ptr->line_status_reg) & LSR_DATA_READY_MASK) ==
						    LSR_DATA_READY)
	{
		delay_for_sio();
		temp = sio_chan_ptr->rec_buffer_reg;
	}
	delay_for_sio();

	/* set mcr to DTR and RTS enable, no int and no  hardware flow
	 * control
	 */
	temp = (MCR_HWFC_DISABLE | MCR_RTS_ENABLE | MCR_DTR_ENABLE);
	sio_chan_ptr->modem_cntr_reg = temp;
	delay_for_sio();
}

/*
 *       Description:    This routine is called to check for XOFF
 *                       characters in the receive fifo.  If there
 *                       is an XOFF, this routine keeps reading the
 *                       receive fifo and won't return until a final
 *                       XON is received for all XOFFs and there are
 *                       no more characters in the receive fifo.
 */
int check_for_pace_chars(sio_chan_ptr)
	SIO_CHAN_REGS *sio_chan_ptr;   /*pointer to the channel registers*/
{
        unsigned char temp;    /*temporary storage */
        unsigned char inputchar;       /*storage for input chars*/

        /* read the line status register to find out if there are
           any characters in the receive fifo*/

        temp = sio_chan_ptr->line_status_reg;
	delay_for_sio();

        /*while LSR indicates a characters in the  receive fifo, read the
          character.  If the character is XOFF, go into a loop of reading
          incoming characters until you receive an XON.  Before believing
	  a character to be XON or XOFF, make sure it didn't look that way
	  because of a parity error */

        while( (temp & LSR_DATA_READY_MASK) == LSR_DATA_READY)
        {
           inputchar = sio_chan_ptr->rec_buffer_reg;
	   delay_for_sio();
           if ((inputchar == XOFF) && ((temp & LSR_PARITY_ERROR_MASK) == 0))
           {
              inputchar = 0x0;
              while ((inputchar != XON) || ((temp & LSR_PARITY_ERROR_MASK) != 0))
              {
                 temp = sio_chan_ptr->line_status_reg;
		 delay_for_sio();

                 if((temp & LSR_DATA_READY_MASK) == LSR_DATA_READY) {
                    inputchar = sio_chan_ptr->rec_buffer_reg;
		    delay_for_sio();
		    if (inputchar != XON)
			if (iobuf_end < iobuf_start + sizeof(iobuf))
				iobuf[iobuf_end++ % sizeof(iobuf)] = inputchar;
		 }

              }/*while*/

           } else {
	      if (iobuf_end < iobuf_start + sizeof(iobuf))
		 iobuf[iobuf_end++ % sizeof(iobuf)] = inputchar;
	   } /* else */


           /*Read the line status register again*/

           temp = sio_chan_ptr->line_status_reg;
	   delay_for_sio();

        } /*while*/

        return;

}/*check_for_pace_chars*/

#define EEPROM_OFFSET 	0x00810000	/* EEPROM offset into core IO base */
delay_for_sio()
{
	int i;
	extern int current_io_base;

	i = *(int *)(current_io_base + 0x690000);
	i = *(int *)(current_io_base + 0x690000);
}

/*
 * DESCRIPTION 
 *	This routine accepts two parameters, a starting time and a timeout
 *	duration, and returns a boolean value denoting whether the specified
 *	timeout period has elapsed. It is non-blocking.
 * RETURNS:
 *	0 : timeout period has not elapsed
 *	1 : timeout period HAS elapsed
 *	-1 : invalid parameter-- timeout period > 10 seconds
 *
 * RESTRICTION:
 *	This routine will only accept timeout periods of up to 10 seconds.
 *	This is due to the integer multiplication overflow problem.  If
 *	a longer timeout period is required, the caller must call this 
 *	routine repeatedly until the total period expires.
 *	A limit of 10 seconds allows this routine to be used with clock
 *	speeds of up to at least 100 mHz.
 *	The 10 second limit means that the timeout parameter cannot
 *	be over 1000.
 */
#define MAX_TIMEOUT_PERIOD	1000  
#define PGZ_MEM_10MSEC	0x038c		/* place to store 10ms constant */
has_timeout_occurred (start_time, timeout)
	struct timeval *start_time;   /* time at beginning of timeout period */
	unsigned int timeout;
{
	struct timeval curr_time;

	/* check to make sure period doesn't surpass maximum value (1000, or
	 * 10 seconds) */
	if (timeout > MAX_TIMEOUT_PERIOD)
		return(-1);
	timeout = timeout * 10000;  /* convert to microseconds */

        get_precise_time(&curr_time);
	timevalsub(&curr_time, start_time);
	return( ((curr_time.tv_sec * 1000000) + curr_time.tv_usec) >= timeout);
}

#endif DDB
