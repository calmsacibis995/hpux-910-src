/* @(#) $Revision: 70.2 $ */    

/* device struct for 98626 rs232 card */

struct kdbpci		{
	char	
		byte0,
	pci_id,		/* register 1: card id (read)     */
			/*             card reset (write) */
		byte2,
	pci_rupt,	/* register 3: card interrupt */
		byte4,
	pci_baud,	/* register 5: card baud rate select */
		byte6,
	pci_line,	/* register 7: card line characteristics */
		byte8, 
	byte9,		/* register 9: unused */
		byte10,
	byte11,		/* register 11: unused */
		byte12,
	byte13,		/* register 13: unused */
		byte14,
	byte15,		/* register 15: unused */
		byte16,
	pci_data,	/* register 17: uart receiver (dlab = 0) */
			/*              uart transmitter (dlab = 0) */
		byte18,
	pciicntl,	/* register 19: interrupt control (dlab = 0) */
		byte20,
	pciistat,	/* register 21: interrupt status */
		byte22,
	pcilcntl,	/* register 23: line control */
		byte24,
	pcimcntl,	/* register 25: modem control */
		byte26,
	pcilstat,	/* register 27: line status */
		byte28,
	pcimstat;	/* register 29: modem status */
};

#define	baudlo	pci_data
#define	baudhi	pciicntl

struct apollo_pci			/* device struct for hardware device */
{
	unsigned char	pci_data;
	unsigned char	pcif1[3];
	unsigned char	pciicntl;	/* interrupt control */
	unsigned char	pcif2[3];
	unsigned char	pciistat;	/* interrupt status */
	unsigned char	pcif3[3];
	unsigned char	pcilcntl;	/* line control */
	unsigned char	pcif4[3];
	unsigned char	pcimcntl;	/* modem control */
	unsigned char	pcif5[3];
	unsigned char	pcilstat;	/* line status */
	unsigned char	pcif6[3];
	unsigned char	pcimstat;	/* modem status */
};
#define	CNTL_C	0x03		/* control c       */
#define	LF	0x0a		/* line feed       */
#define	CR	0x0d		/* carriage return */
#define	BS	0x08		/* back space      */
#define	DC1	0x11		/* XON             */
#define	DC3	0x13		/* XOFF            */
#define BELL	0x07		/* Bell		   */

#define	KDBBAUD_RATE	0x10;	/* 9600 baud (153600/9600) (default) */
#define	KDBBAUD_RATE_APCI	0x34;	/* 9600 baud (153600/9600) (default) */
#define	KDB_LINECNTL	0x1A;	/* even parity, 7 bit chars, one stop bit */

#define	MAX_KDBIN	160

struct kdbtty {
	struct kdbpci *kdbt_addr;
	int kdbt_type;
	char kdbt_flags;
	char kdb_buffer[MAX_KDBIN];
	char *kdb_tail;
}kdbtp;

#define HP_UART		1
#define APOLLO_UART	2

/* kdbtty flag bits */
#define	WAIT_FOR_DC1	0x01		/* dc3 seen so wait for dc1 */

/* uart bit assignments */
#define RXRDY		0x01
#define DLAB		0x80
#define THRE		0x20
#define THRE_SHFT	0x40

struct kdbtty kdb_tty;		/* "tty" structure for kdb             */
int kdbtty_address;		/* IO address of current KDB interface */
