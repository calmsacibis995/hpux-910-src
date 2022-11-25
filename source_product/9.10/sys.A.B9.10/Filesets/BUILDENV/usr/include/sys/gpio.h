/* @(#) $Revision: 1.13.83.4 $ */     
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/gpio.h,v $
 * $Revision: 1.13.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:26:51 $
 */
#ifndef _SYS_GPIO_INCLUDED /* allows multiple inclusion */
#define _SYS_GPIO_INCLUDED

#ifdef __hp9000s300
/*
**	header for gpio interface
**
*/

#define	CONTROL_LINES		1
#define	STATUS_LINES		2
#endif /* __hp9000s300 */

#ifdef __hp9000s800
/************************************************************************/
/*                               ioctl                                  */
/*                                                                      */
/*  This file should be included by any program that wants to use ioctl */
/*  calls for the GPIO device adapter or any device adapter using the   */
/*  GPIO Logical Device Manager (LDM).                                  */
/************************************************************************/


#ifndef IO_CONTROL
/*--------------------- ioctl commands from HP-UX ----------------------*/

				/* macros from ioctl.h:			*/
#define	O_IO_CONTROL		_IOW('I', 0, struct io_ctl_status)
#define	IO_CONTROL		_IOWR('I', 0, struct io_ctl_status)
#define	IO_STATUS		_IOWR('I', 1, struct io_ctl_status)
#define	IO_ENVIRONMENT		_IOR('I', 2, struct io_environment)


/*------------------- ioctl structures used by HP-UX -------------------*/

struct io_ctl_status {
    int type;				/* command from below   	*/
    int arg[3];				/* arguments to command		*/
} ;

struct io_environment {
    int interface_type;			/* as defined below		*/
    int timeout;			/*	   for IO_STATUS...	*/
    int status;
    int term_reason;
    int read_pattern;
    int signal_mask;
    int width;
    int speed;
    int locking_pid;
    unsigned int config_mask;
    unsigned short delay;
    unsigned char reserved[22];		/* up to 64 bytes for expansion	*/
} ;

					/* For GPIO_RESET:   arg[0] =	*/
#define HW_CLR			1	/*    -- reset interface	*/

#define LOCK_INTERFACE		0	/* to lock the interface	*/
					/*    increment the "open" and 	*/
					/*    card lock counts		*/
#define UNLOCK_INTERFACE	1	/* to unlock the interface	*/
					/*    decrement the "open" and	*/
					/*    card lock counts. unlock	*/
					/*    interface if card lock	*/
					/*    drops to zero.		*/
#define CLEAR_ALL_LOCKS		2	/* to clear all locks 		*/
					/*    clear all "open" and card	*/
					/*    locks.			*/
#endif

				/* GPIO1_IO_DIAG is for diagnostics only */
#define	GPIO1_IO_DIAG		_IOWR('I', 99, struct gpio1_diag_ctl_type)

					/* For gpio1 diagnostics only: */
struct gpio1_diag_ctl_type{
    int type;				/* command from below   	*/
    int arg[3];				/* arguments to command		*/
};

				/*- type field for gpio1_diag_ctl_type -*/
#define GPIO1_DIAG_RESET	0	/* issue a CMD_RESET to the card */
					/* and restore eim,ie 		*/

#define GPIO1_DIAG_IDENT	1	/* return the card IODC and the	*/
					/* DAM RCS information		*/
					/* input:			*/
					/* 	arg[0] = addr of buffer */
					/*	to store DAM RCS info	*/
					/* output:			*/
					/*	8-byte IODC in arg[1],	*/
					/*	and arg[2];		*/
					/* 	RCS info in the		*/
					/*	specified buffer	*/

#define GPIO1_DIAG_READ_REG	2	/* read the register at the	*/
					/* requested offset on the GPIO	*/
					/* card and return the value	*/
					/* input:			*/
					/* 	arg[0] = word offset to	*/
					/* 		hpa		*/
					/* output:			*/
					/*	register value in arg[1]*/

#define GPIO1_DIAG_WRITE_REG	3	/* write the register at the	*/
					/* requested offset on the GPIO	*/
					/* card with the specified value*/
					/* input:			*/
					/* 	arg[0] = word offset to	*/
					/* 		hpa		*/
					/*	arg[1] = value to write	*/



/*----------------------- io_ctl_status "type"s ------------------------*/

					/* for IO_CONTROL:		*/
#define GPIO_TIMEOUT		0	/* -- set timeout value (u-sec)	*/
					/*    arg[0] = timeout value	*/
#define GPIO_WIDTH		1	/* -- set data path width	*/
					/*    arg[0] = width in bits    */
#define GPIO_SPEED		2	/* -- set transfer speed	*/
					/*    arg[0] = k-bytes/second	*/
#define GPIO_SIGNAL_MASK	4	/* -- define signaling events	*/
					/*    arg[0] = interrupt mask   */
					/*    arg[0] = 0  (disable)     */
#define ST_ARQ2	       	      	1	/* interrupt mask for ATTN signal */

#define GPIO_LOCK		5	/* -- lock/unlock interface	*/
					/*    arg[0] = lock_interface	*/
					/*	       unlock_interface */
					/*	       clear_all_locks  */

#define GPIO_RESET		6	/* -- reset 			*/
#define GPIO_CTL_LINES		7	/* -- write to control register	*/
					/*    arg[0] = mask             */

#define GPIO_SET_CONFIG		8	/* -- set up miscellaneous GPIO	*/
					/*    parameters as specified	*/
					/*    in arg[0] and arg[1]:	*/

					/*    arg[1]: this value is used*/
					/*	to set the delay to	*/
					/*	allow for data to settle.*/
					/*	The range for the delay	*/
					/* 	is 125-2000 nano-sec and	*/
					/*	the default value is	*/
					/*	2000 nano-sec.		*/

					/*    arg[0]: this configuration*/
					/*	mask is constructed by	*/
					/* 	or-ing flags from the 	*/
					/*	list below. Since each	*/
					/*	request overwrites the 	*/
					/*	previous setting for the*/
					/*	parameter, one can get 	*/
					/* 	the current settings	*/
					/*	through an IO_STATUS	*/
					/*	ioctl request of	*/
					/*	GPIO_GET_CONFIG type:	*/
				/*
				 * set a bit specified by "bit_num". bit 31
				 * is the least significant bit and bit 0
				 * the most significant.
				 */
#undef  BIT_MASK
#define BIT_MASK( bit_num ) ( 1L << ( 31 - bit_num ))

#define HANDSHAKE_MODE_SIZE	    ( BIT_MASK(30) | BIT_MASK(31) )
#define FULL_HANDSHAKE_MODE	    0
#define PULSE_HANDSHAKE_MODE	    ( BIT_MASK(31) )
#define STROBE_HANDSHAKE_MODE  	    ( BIT_MASK(30) )
#define SLAVE_HANDSHAKE_MODE   	    ( BIT_MASK(30) | BIT_MASK(31) )

#define	FIFO_MASTER		    0
#define	FULL_MASTER		    ( BIT_MASK(7) )
#define	FULL_SLAVE		    ( BIT_MASK(6) )

#define DIN_CLK_SIZE		    ( BIT_MASK(27) | BIT_MASK(28) | BIT_MASK(29) )
#define DIN_CLK_PFLG_BUSY_TO_READY  0
#define DIN_CLK_PCTL_SET_TO_CLEAR   ( BIT_MASK(29) )
#define DIN_CLK_PFLG_READY_TO_BUSY  ( BIT_MASK(27) )
#define DIN_CLK_PCTL_CLEAR_TO_SET   ( BIT_MASK(27) | BIT_MASK(29) )
#define DIN_CLK_READ_OF_LO_BYTE   ( BIT_MASK(27) | BIT_MASK(28) )
#define DIN_CLK_READ_OF_HI_BYTE   ( BIT_MASK(27) | BIT_MASK(28) | BIT_MASK(29) )

#define LOGIC_SENSE_SIZE	    ( BIT_MASK(24) | BIT_MASK(25) | BIT_MASK(26) )
#define PFLG_LOGIC_SENSE	    ( BIT_MASK(26) )
#define PCTL_LOGIC_SENSE	    ( BIT_MASK(25) )
#define PDDR_LOGIC_SENSE	    ( BIT_MASK(24) )
#define EDGE_LOGIC_SENSE	    ( BIT_MASK(0) )

#define ABORT_ON_EXT_INT	    ( BIT_MASK(23) )
#define TRNSFR_CTR_EN		    ( BIT_MASK(3) )
#define PEND_OPT_EN		    ( BIT_MASK(27) )
#define PDIR_OPT_EN		    ( BIT_MASK(25) )

					/* for IO_STATUS:		*/
/*	GPIO_TIMEOUT			/* -- return timeout value 	*/
					/*    arg[0] = timeout value	*/
/*	GPIO_WIDTH		 	/* -- return data path width	*/
					/*    arg[0] = width in bits    */
/*      GPIO_SPEED		 	/* -- return transfer speed	*/
					/*    arg[0] = k-bytes/second	*/
/*      GPIO_SIGNAL_MASK	 	/* -- return SIGALRM reason	*/
					/*    arg[0] = interrupt mask   */
/*      GPIO_LOCK		 	/* -- return locking process id	*/
					/*    arg[0] = process id       */
					/*    arg[0] = -1 (not locked)  */
#define GPIO_STS_LINES		 8	/* -- read status lines		*/
					/*    arg[0] = mask             */
#define GPIO_INTERFACE_TYPE	 9	/* -- return interface type	*/
					/*    arg[0] = interface type   */

#define GPIO_GET_CONFIG		18	/* -- returns the miscellaneous	*/
					/*    GPIO parameters as	*/
					/*    described in GPIO_SET_CONFIG */
					/*    command of the IO_CONTROL	*/
					/*    ioctl call. In arg[0] will*/
					/*    be the configuration mask	*/
					/*    as decribed above and in	*/
					/*    arg[1] the delay value 	*/
					/*    in nano-sec.		*/

					/* FOR gpio0 DIAGNOSTIC SUPPORT ONLY */

#define GPIO_REG0		10      /* -- access CIO register 0	*/
#define GPIO_REG1		11 	/* -- access CIO register 1     */
#define GPIO_REG3		12	/* -- access CIO register 3	*/
#define GPIO_REG7		13	/* -- access CIO register 7	*/
#define GPIO_REG9		14	/* -- access CIO register 9	*/
#define GPIO_CLEAR		15	/* -- clear FIFO on AFI		*/
#define GPIO_POLL		16	/* -- check poll response	*/
#define GPIO_REGA		17	/* -- access CIO register A     */
#define GPIO_REGB		19	/* -- access CIO register B     */

#define GPIO_PORTNUM		999

					/* value returned by the gpio1	*/
					/* driver in high 16 bits for	*/
					/* GPIO_INTERFACE_TYPE command. */
					/* return type A DMA driver	*/
					/* software model:		*/
#define GPIO1_SOFTWARE_MODEL 0x44
#define GPIO1_INTERFACE	(( GPIO1_SOFTWARE_MODEL << 8 ) | 0x4 )
#endif /* __hp9000s800 */

#endif /* _SYS_GPIO_INCLUDED */
