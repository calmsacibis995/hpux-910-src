
/*	98659 definitions	*/
/*	File Name = 98659.h 	*/

#include	"/usr/include/sys/termio.h"

/* register definitions */

#define	CARD_ID			0	/* Status Only Driver	*/
#define	INT_STATUS		1	/* Status Only Driver	*/
#define	ACTIVITY		2	/* Status Only Driver	*/

#define	ID_REG			3	/* Status Only	*/
#define	INTR_REG		4	/* Status Only	*/

#define	RX_BYTES_AVAIL		5	/* Status Only Driver	*/

#define	BREAK_REG		6	/* Control and Status */
#define	MODEM_REG_7		7	/* Status Only	*/
#define	MODEM_REG_8		8	/* Control and Status */

#define	LAST_TERM		9	/* Status Only Driver	*/
#define	LAST_MODE		10	/* Status Only Driver	*/
#define	TRANSMIT_AVAILABLE	11	/* Status Only Driver	*/

#define	CONNECT_REG		12	/* Control Only */
#define	LINE_STATE		12	/* Status Only	*/

#define	MODEM_INTCOND_REG	13	/* Control and Status */
#define	CBLOCK_MASK_REG		14	/* Control and Status */
#define	MODEM_INTMASK_REG	15	/* Control and Status */

#define	OUTPUT_QUEUE		16	/* Status Only  Driver*/
#define	RECEIVE_AVAILABLE	17	/* Control and Status */
#define	BRK_CTRL_REG		18	/* Control and Status */
#define	PARITY_CHECK		19	/* Control and Status */

#define	BAUD_REG		20	/* Control and Status */
				/* 20 is TSPEED and 21 is RSPEED */
#define	HANDSHAKE_REG		22	/* Control and Status */
#define	HW_HANDSHAKE_REG	23	/* Control and Status */

#define	ERROR_REPORT_REG	26	/* Control and Status */
#define	CANONICAL_REG		27	/* Control and Status */
#define	VEOF_VMIN_REG		28	/* Control and Status */
#define	VEOL_VTIME_REG		29	/* Control and Status */

#define	CHAR_MASK_REG		30	/* Control and Status */

#define	CHAR_SIZE_REG		34	/* Control and Status */
#define	STOP_BIT_REG		35	/* Control and Status */
#define	PARITY_REG		36 	/* Control and Status */

#define	GAP_REG			37 	/* Control and Status */
#define	ALL_SENT_REG		38	/* Status Only	*/
#define	BREAK_VALUE_REG		39 	/* Control and Status */

#define	CLEAR_REG		101 	/* Control Only */
#define	CLEAR_INPUT		102	/* Control Only */
#define	CLEAR_OUTPUT		103	/* Control Only */

#define	COMMAND_REG		110	/* Control Only */
#define	JUMP_REG		120	/* Control Only */

#define	INTR_COND_REG		121	/* Control and Status */

#define	POKE_ADDR_REG		123	/* Control Only	*/
#define	POKE_REG		124	/* Control Only	*/
#define	PEEK_REG		124	/* Status Only	*/
#define	ABORT_REG		125	/* Control Only */

/*	Values for Registers	*/

/* 0	CARD_ID		pdi card id Code */
#define	PDI_ID_CODE		32+20

/* 1	INT_STATUS	Hardware Interrupt Status */
/*	Not presently used	*/

/* 2	ACTIVITY	is this card active */
/*	Not presently used	*/

/* 3	ID_REG		Protocol ID*/
#define	ID_98659		55

/* 4	INTR_REG	Reason for mainframe interrupt	*/
/*			For MODEM_INTEN/MODEM_STATUS interrupts */
#define	DATA_CTL_AVAIL		1
#define	WRITE_WHEN_PRT		2
#define	FRAM_PAR_ERR		4
#define	MODEM_CHANGE		8
#define	EOL_RECEIVED		64
#define	BREAK_RECEIVED		128

/* 5	RX_BYTES_AVAIL	Bytes available in receive buffer */
/*	Returns number of bytes in receive buffer	*/

/* 6	BREAK_REG	Break Output Control Register	*/
/*	On control data is a don't care	*/
#define	NOBRK	0
#define	PENDING	1
#define	SENDING	2
#define	WAITING	3	/*after sending a break	*/

/* 7	MODEM_REG_7	What are modem input lines	*/
#define	DM			1
#define	RR			2
#define	CS			4
#define	OCR1			8
#define	OCR2			16

/* Modem Status masks */

#define	ODY_DSR		0x01
#define	ODY_DCD		0x02
#define	ODY_CTS		0x04
#define	ODY_RI		0x08
#define ODY_CCITT_IN	ODY_DSR | ODY_DCD | ODY_CTS | ODY_RI

#define CCITT_IN	(MDCD | MCTS | MDSR | MRI)


/* 8	MODEM_REG_8	What are modem output lines R/W	*/
#define	RS			1
#define	TR			2
#define	OCD1			4
#define	OCD2			8
#define	OCD3			16
#define	OCD4			32
#define	TXBAUDINT		64
#define	RXBAUDINT		128

/* Modem Control masks */

#define	ODY_RTS		0x01
#define	ODY_DTR		0x02
#define	ODY_DRS		0x04

#define CCITT_OUT	(MDTR | MRTS | MDRS)

/* 9	LAST_TERM	Last Term used or received*/
/*	Not presently used	*/

/* 10	LAST_MODE	Last Mode used or received*/
/*	Not presently used	*/

/* 11	TRANSMIT_AVAILABLE	*/
/*	Returns as the number of bytes the space available 
/*	in the output buffer	*/

/* 12	CONNECT_REG	*/
#define DISCONNECT		0
#define	CONNECT			1/* Transmit Only */
#define	RECEIVE			2/* Transmit and Receive */

/* 12	LINE_STATE	*/
#define DISCONNECTED		0
#define	CONNECTED		1
#define	RECEIVING		2

/* 13	MODEM_INTCOND_REG	*/
/*	Same as INTR_REG	*/
/*	What conditions to allow a MODEM_INTEN to occur	*/

/* 14	CBLOCK_MASK_REG		What kind of control blocks
				to include in RX buffer */
#define	PROMPT_POS		1
#define	EOL_POS			2 /* canonical mode */
#define	FR_PR_CTRL		4 /* framing or parity error */
#define	BREAK_RCVD		8

/* 15	MODEM_INTMASK_REG	Which modem receivers to
				interrupt on	*/
/*	same as MODEM_REG_7	*/
/*	Not presently used */

/* 16	OUTPUT_QUEUE		 Status Only */
/*	Number of bytes still in the output queue */

/* 17	RECEIVE_AVAILABLE	 Status */
/*	Number of lines to read for canonical */
/*	IS there qualified data to read for non-canonical*/
/* 17	RECEIVE_AVAILABLE	 Control */
/*	For Canonical, decrement Receive Available */

/* 18	BRK_CTRL_REG	Break Input control Register	*/
#define	BREAK_NULL	0	/*insert breaks as a null with a
				control block*/
#define	BREAK_IGNORE	1	/* Do nothing on receiving breaks*/
#define	BRK_INTERRUPT	2	/* Send a signal interrupt on
				receiving breaks as well as clearing
				input and output queues	*/

/* 19	PARITY_CHECK	Enable/Disable Parity checking on input */

/* 20	BAUD_REG TX		/*
/* 21	BAUD_REG RX		/*
/*
/*#define	B50		001  These values are taken from 
/*#define	B75		002  TERMIO and are octal values 
/*#define	B150		005  They will be picked up in
/*#define	B200		006  termio.h masked by CBAUD
/*#define	B300		007 
/*#define	B600		010   Values not shown here will
/*#define	B1200		012   Cause ody_control to return
/*#define	B1800		013   a value error
/*#define	B2400		014 
/*#define	B4800		016 
/*#define	B7200		017 
/*#define	B9600		020 
/*#define	B19200		021 
/*#define	*B38400		022 	;NOT AVAILABLE 
/*
/*		ADDED for this card	*/

#define	B57600			0000023 
#define	B115200			0000024 /* NOT AVAILABLE */
#define	B230400			0000030 
#define	B460800			0000031 
#define	BEXT16X			0000036 /* same as EXTA	*/
#define	BEXT1X			0000037 /* same as EXTB	*/

/* 22	HANDSHAKE_REG		*/
/*	Software Handshaking Mode	*/

#define	NO_SHSHAKE		0
#define	XON_XOFF_HS		5

/* 23	HW_HANDSHAKE_REG	*/

#define	HRD_HANDSKE_OFF		0
#define	HRD_HANDSKE_ON		3

/* 26	ERROR_REPORT_REG	*/
/*	Enable/disable reporting framing and parity errors
		in receive data stream  if disabled, all
		error input bytes are discarded*/

/* 27	CANONICAL_REG	Canonical mode enabled/disabled	*/

/* 28	VEOF_VMIN_REG	VALUE for VEOF and VMIN */

/* 29	VEOL_VTIME_REG	VALUE for VEOL and VTIME */

/* 30	CHAR_MASK_REG	Mask for received Characters */
			/* Set by CSIZE but changed by
			ISTRIP	*/

/* 34	CHAR_SIZE_REG		*/
/*
/*From termio masked by CSIZE and divided by CS6
/*#define	CS5			0
/*#define	CS6			1
/*#define	CS7			2
/*#define	CS8			3
/**/

/* 35	STOP_BIT_REG		*/

#define	ONE_STOP_BIT		0x00
#define	TWO_STOP_BITS		0x02

/* 36	PARITY_REG		*/

#define	PARITY_NONE		0x00
#define	PARITY_ODD		0x01
#define	PARITY_EVEN		0x02

/* 37	GAP_REG			*/
/*	Number of gap characters, not presently used */

/* 38	ALL_SENT_REG		*/

#define	ALL_SENT		1	/* TRUE	*/

/* 39	BREAK_VALUE_REG		*/
/*	Value of break sent in character times	*/

/* 101	CLEAR_REG	Clear Card	*/
/*	Value sent is a don't care	*/

/* 102	CLEAR_INPUT   Clear Input Buffer	*/
/*	Value sent is a don't care	*/

/* 102	CLEAR_OUTPUT   Clear Output Buffer	*/
/*	Value sent is a don't care	*/

/* 110	COMMAND_REG		*/
/*	Not presently used	*/

/* 120	JUMP_REG		*/
/*	available for downloading code into RAM
	value is offset from end of RAM to jump to */

/* 121	INTR_COND_REG		*/

#define	ERR_INT		0x01

#define	DATA_AVAILABLE	0x02
#define	WRITE_AVAILABLE	0x04
#define	MODEM_STATUS	0x08

/*	the following also used for t_sicnl setting	*/

#define RXINTEN 	0x02
#define TXINTEN 	0x04
#define MODEM_INTEN	0x08

/*	Offsets from card_address for direct access */

#define	INT_COND_OFFSET		0x4001
#define	INT_ENABLE_OFFSET	0x0003
#define	INT_STATUS_OFFSET	0x0003
#define	ERROR_CODE_OFFSET	0x400d


/* 123	POKE_ADDR_REG		*/
/*	Not presently used	*/

/* 124	POKE_REG		*/
/*	Not presently used	*/

/* 124	PEEK_REG		*/
/*	Not presently used	*/

/* 125	ABORT_REG		*/
/*	Value sent is a don't care	*/

/*	ERROR CODE for ERROR INTERRUPT */
/*	From ERROR_CODE	*/
#define	SELFTESTFAIL	 6
#define	INPUT_OVERRUN	13
#define	RX_OVERFLOW	14
#define	REG_ADDR_BAD	26
#define	REG_VALUE_BAD	27

/*	ERROR CODE FROM SELFTESTFAILTYP	*/
#define	ROM_FAIL	0x001
#define	RAM_FAIL	0x010
#define	CTC_FAIL	0x002
#define	SIO_FAIL	0x004
#define	SEM_FAIL	0x008

/*	Offsets from card_address for direct access */

#define	SELFTESTFAILTYP_OFFSET		0x402d

/* control blocks for break, parity and framing errors */

/*	used for line control t_slcnl	*/

#define SETBRK		0x40

/* for control block reporting */

#define	MODE_MASK	0xff00	/* control block is composed*/
#define	BREAK_IN	250<<8  /* of TERM and MODE<<8 OR'd*/
#define	ERROR		251<<8  /* together*/
#define	EOL_EOF_LF	253<<8  /**/
#define	CHANGE_BUFFER	255<<8

#define	TERM_MASK	0x0003
#define	FRAMING_ERROR	0x0001		
#define	PARITY_ERROR	0x0002
#define	BREAK		0x0001

#define ON	1
#define OFF	0

#define REPORT_ERROR	1
#define NO_ERROR	0

#define ENABLE	1
#define DISABLE	0

struct ody_desc_type {
	int card_address;
	short which_RXbuf;
	short term_and_mode;
	struct sw_intloc intlock;
};

#define PASS_ODY tp->utility

/*	Offsets from card_address for direct access */

#define	RESET_OFFSET		0x0001

/* to provide consistency between termio.h and
	termio manual pages */
/*  No longer needed with release 6.5 */
/*
#define	LOBLK			CRTS
*/

/* ID string for use by the SCMS Manager */
/* @(subid) MAIN:                                                                */
/*		          1         2         3         4         5         6    */
/*               1234567890123456789012345678901234567890123456789012345678901234*/
