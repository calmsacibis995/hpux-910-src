/*
 * @(#)ddb.h: $Revision: 1.2.84.3 $ $Date: 93/09/17 21:03:21 $
 * $Locker:  $
 */

struct cmd_remote {
	unsigned int  cmd_command;
	char         *cmd_address;
	unsigned int  cmd_rcount;
};

#define LINK_CMND	1
#define LINK_READD	2
#define LINK_WRITED	3
#define LINKCMD_READ                       4
#define LINKCMD_WRITE                      5
#define LINKCMD_INTERRUPT                  6
#define	LINKCMD_VREAD		20
#define	LINKCMD_VWRITE		21
#define	LINKCMD_VTOR		24

#define DDB_IO_DONE	1
#define DDB_IO_NOT_DONE	2

/* Sizes of the various buffers used by the ddb driver */
#define DATA_BUFFER_SIZE	4096

/* device struct for 98626 rs232 card */
struct ddbpci		{
	char	byte0,
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

#define HP_UART		1
#define APOLLO_UART	2
#define	MAX_KDBIN	160

struct ddbtty {
	struct ddbpci *ddbt_addr;
	int ddbt_type;
	char ddbt_flags;
	char ddb_buffer[MAX_KDBIN];
	char *ddb_tail;
}ddbtp;

struct ddbtty ddb_tty;		/* "tty" structure for ddb             */
