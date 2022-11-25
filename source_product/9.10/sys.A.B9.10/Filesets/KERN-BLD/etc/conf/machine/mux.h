/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/mux.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:15:15 $
 */
/* @(#) $Revision: 1.3.84.3 $ */      
#ifndef _MACHINE_MUX_INCLUDED /* allows multiple inclusion */
#define _MACHINE_MUX_INCLUDED
#ifdef __hp9000s300
/*
**	header for the MUX card 
*/
struct	mux_info_type {
	unsigned short flag;		/* current state of this port */
	char *addr;
	char *transmit_buf;		/* transmit/receive buffer pointers */
	char *receive_buf;
	char *variable_list;		/* list for buffer management */
	char *bit_array;		/* special character array */
	char *modem_reg;		/* modem registers (if on this port) */
	char *config_reg;		/* pointer to configure data */
	char *port_cmd_reg;		/* pointer to port INTR/STATUS base */
	unsigned char port;		/* port number */
	unsigned char port_mask;	/* port mask for interrupts */
};

/* areas where the tp structure is stored in the isc */
#define port0	ppoll_f
#define port1	ppoll_l
#define port2	event_f
#define port3	event_l

#define	mux_info	utility		/* pointer in tty structure */

#define OUTLINE	0x80			/* set if priveleged line */

#define	MODEM		0x0001		/* This port has modem lines */
#define TX_OFF		0x0002		/* XOFF received, stop transmit */
#define TX_BLOCK	0x0004		/* We sent XOFF, stop transmit */
#define TX_DELAY	0x0008		/* Delay character, wait till empty */
#define TX_BREAK	0x0010		/* In process of sending NULL's */
#define TX_STOP		0x0020		/* special char received, stop sending*/
#define TX_WAIT		0x0040		/* wait for port to empty, tty_wait */
#define TX_CONFIG	0x0080		/* wait,port to empty, then configure */

#define RX_SIZE		256+10		/* Receive buffer size  plus extra */

/* this is passed to the isr routine, set up at power up */

struct mux_intr_type {
	char		*card_addr;	/* pointer to card */
	struct tty	*mux_ttys[4];	/* tty's for this card */
};

/* Interrupt status masks */
#define INTR_MASK	0x03	/* Mask for status bits */
#define TX_INTR		0x01	/* Transmit buffer empty */
#define SPEC_CHAR_INTR	0x02	/* Received special character */
#define RX_INTR		0x04	/* Transmit buffer empty */

#define MODEM_INTR	0x20	/* Modem lines changed */

#define M_RXINTEN 	0x00

/* Configuration masks */
#define	M_PARITY_NONE	0x0000
#define	M_PARITY_ODD	0x0100
#define	M_PARITY_EVEN	0x0200
#define	M_TWO_STOP_BITS	0x0800
#define	M_ONE_STOP_BIT	0x0000

#define	ON	1
#define	OFF	0
#define	D1	0x11		/* XON/XOFF characters */
#define	D3	0x13

/* Character status masks */
#define	STATUS_MASK	0xf8
#define	FRAME_ERROR	0x80
#define	OVERRUN_ERROR	0x40
#define	PARITY_ERROR	0x20
#define	BREAK_DETECT	0x10
#define	BUFFER_OVERFLOW	0x08

/* Modem Control masks */
#define	MUX_RTS		0x01
#define	MUX_DTR		0x02
#define	MUX_DRS		0x04

/* Modem Status masks */
#define	MUX_RI		0x01
#define	MUX_DCD		0x02
#define	MUX_DSR		0x04
#define	MUX_CTS		0x08

#define MUX_CCITT_MSL       (MUX_DCD | MUX_CTS | MUX_DSR)
#define MUX_SIMPLE_MSL      (MUX_DCD)
#define MUX_CCITT_MCL       (MUX_DTR | MUX_RTS)
#define MUX_SIMPLE_MCL      (MUX_DTR)

/* initialization constants */
#define	TX_OFFSET	(char *)(addr+(0x8ee1+((3-i)*32)))
#define	RX_OFFSET	(char *)(addr+(0x8401+((3-i)*512)))
#define	VAR_OFFSET	(char *)(addr+(0x8e01+(i*2)))
#define	BITARRY_OFFSET	(char *)(addr+(0x8c01))
#define	CONFIG_OFFSET	(char *)(addr+(0x8e21+(i*4)))
#define	PORTCMD_OFFSET	(char *)(addr+(0x8e37+(i*2)))
#define	MODEM_OFFSET	(char *)(addr+(0x8e31))
#endif /* __hp9000s300 */
#endif /* _MACHINE_MUX_INCLUDED */
