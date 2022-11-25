/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/ddb.c,v $
 * $Revision: 1.2.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:03:00 $
 */

#ifdef DDB

#include "../h/param.h"
#include "ddb.h"

#define ID626   0x2
#define ID644   0x42
#define	ddb_ltor new_vtop
#define	INITIALIZE	'A'
#define	ACK		'B'
#define	SIG_COMMAND	'C'
#define	SIG_READY	'D'
#define	SIG_INIT	'F'

#define	TRUE	1
#define FALSE	0

/*
 * NOTE: ddb_io_addr is a common parameter in this file.  It always refers to
 * 	the address of the DDB device.
 */
char	*ddb_io_addr = 0;

int	link_state = 0;
int	call_ddb_stop;
int	rs232_length = 0;
char	*rs232_buffer = 0;
int	rs232_out_int	= 0;
int	rs232_in_dma = FALSE;
static	u_int ddb_bytes_transferred = 0;
char	ddb_data_buffer [ DATA_BUFFER_SIZE*2 + 32 ] = { 0 };
struct	cmd_remote	cmd_buffer = { 0 };

extern	int ktext_ptes;
extern	int ddb_stop();
extern	int contflag;
extern  int ddb_do_proc;
extern	int ddb_call_proc();
extern	unsigned int *tablewalk();
extern	struct ste Syssegtab[];

/*
 * Finds and initializes the serial line.
 */
ddb_initialize_machine_and_module ()
{
	int id;
	unsigned temp_addr;
	extern testr();
	extern int current_io_base;

	/* Find a rs232-line */
        /* First, try HP select code 9 */
        temp_addr = 0x690000 + current_io_base;
        ddb_tty.ddbt_type = HP_UART;
        if (testr((temp_addr + 1),1)) {
                id = *(char *)(temp_addr+1) & 0x7F;
                if (id == ID626 || id == ID644)
                        goto good;
        }

        /* Apollo utility chip */
        ddb_tty.ddbt_type = APOLLO_UART;
        temp_addr = 0x41C020 + current_io_base;
        if (testr(temp_addr,1))
                goto good;

        panic("DDB no tty");

good:   ddb_tty.ddbt_addr = (struct ddbpci *)temp_addr;
	ddb_io_addr = (char *)(temp_addr + 16);

	rs232_init(ddb_io_addr);	/* init io path */
}

/*
 * Stop handler for DDB
 */
ddb_int ()
{
	struct device *rs232_addr = (struct device *)ddb_io_addr;
	contflag = 0;
	rs232_int();
	while (!contflag) {
		/* Handle any DDB communication interrupts */
		while ((rs232_out_int || rs232_receive_pending(rs232_addr)) &&
                       (!ddb_do_proc))
			ddb_state_engine();
		if (ddb_do_proc) {
			ddb_call_proc();
			ddb_do_proc = 0;
		}
	}
} 

/* this is a routine called to check for DDB "interrupts."  Since DDB is
 * polled, and does not use interrupts.  This routine is called from
 * the clock isr (clkrupt) to simulate an external interrupt.
 */
ddb_poll_ss()
{
	struct device *rs232_addr = (struct device *)ddb_io_addr;
	struct timeval start_time, curr_time;
        unsigned int elapsed;

	call_ddb_stop = 0;

	while (rs232_out_int || rs232_receive_pending(rs232_addr)) {
		ddb_state_engine();
                get_precise_time(&start_time);
                elapsed = 0;
                while(elapsed < 5000)
                {
                      get_precise_time(&curr_time);
                      timevalsub(&curr_time, &start_time);
                      elapsed = curr_time.tv_sec * 1000000 + curr_time.tv_usec;
                }
	}

	if (call_ddb_stop)
   		ddb_stop();
}

/* WriteEnable
   Modify kernel page table entries such that writes to kernel text pages are
   legal.  Since the debugger might have any number of reasons for modifying
   just about any kernel text address, open all of the PTE's.  This routine
   should only be called if Virtual Address mapping is currently enabled and
   we are on the way into the debugger from the kernel.

   A stripped kernel can not provide DDB with Sysmap to point to the page
   tables.  In that case, we're basically headed for a crash.
*/
WriteEnable()
{
        register int *PtrSysmap;
        PtrSysmap = (int *) ktext_ptes;
        PtrSysmap[0] &= 0xfffffffb;
}

WriteProtect()
{
        register int *PtrSysmap;
        PtrSysmap = (int *) ktext_ptes;
        PtrSysmap[0] |= 0x00000004;
}

/* vaddr must be a kernel virtual address */
caddr_t
new_vtop(vaddr)
unsigned int vaddr;
{
	unsigned int offset;
	unsigned int pfn;

	offset = vaddr & 0x0fff;
	pfn = (*tablewalk(Syssegtab, vaddr)) & 0xfffff000;
	return ((caddr_t)(pfn + offset));
}

/*
 * Starts up DDB -- initializes the state engine, and starts up the first I/O.
 */
ddb_start_engine()
{
	ddb_initialize_machine_and_module();
        /* Send INITIALIZE character across the link and wait for response */
	while (!rs232_waitchar(INITIALIZE, /* msec = */ 1000))
		rs232_putchar(ddb_io_addr, INITIALIZE);
	rs232_putchar(ddb_io_addr, SIG_INIT);
	link_state = LINK_CMND;
	ddb_io_setup(&cmd_buffer, LINK_CMND);
}

/*
 * Determines whether this interrupt should be treated as a command, as input,
 *	or as output.  Sets up the device for the next interrupt.
 */
ddb_state_engine()
{
	struct cmd_remote *cmd_bufferp = &cmd_buffer;
	char *temp_pointer = ddb_data_buffer;

	switch (link_state) {

	case LINK_CMND:
		rs232_copyin(temp_pointer, (char *)cmd_bufferp,
				 sizeof(*cmd_bufferp));
		link_state = ddb_do_command(cmd_bufferp);
		break;
	
	case LINK_READD:
		if (ddb_read_complete(cmd_bufferp, ddb_bytes_transferred)
			== DDB_IO_DONE)
		{
			link_state = LINK_CMND;
		}
		break;

	case LINK_WRITED:
		if (ddb_write_complete(cmd_bufferp, ddb_bytes_transferred)
			== DDB_IO_DONE)
		{
			link_state = LINK_CMND;
		}
		break;

	default:
		/* Unrecognized state */
		panic("DDB bad state");
		break;
	}
	ddb_bytes_transferred =
		ddb_io_setup(cmd_bufferp, link_state);
}

/*
 * Interprets command buffer sent over the ddb link.
 * Returns:
 *	LINK_CMND:	If no reads or writes are required ("Get ready for
 *				next command").
 *	LINK_READD:	If DDB needs to read from the link.
 *	LINK_WRITED:	If DDB needs to write to the link.
 */
ddb_do_command(cmd_bufferp)
	struct cmd_remote *cmd_bufferp; /* Pointer to remote command struct */
{
	char *ddb_address_ptr;
	char *dummy_pointer;
	int temp_word;
	char *ddb_ltor();

	switch (cmd_bufferp->cmd_command) {

	/* these first two are the basic commands, from which all others flow */
	case LINKCMD_READ:
		return LINK_READD;

	case LINKCMD_WRITE:
		return LINK_WRITED;

	case LINKCMD_VTOR:
		ddb_purge();
		ddb_address_ptr = ddb_ltor(cmd_bufferp->cmd_address);
		dummy_pointer = (char *)&ddb_address_ptr;
		cmd_bufferp->cmd_address = ddb_ltor(dummy_pointer);
		cmd_bufferp->cmd_command = LINKCMD_WRITE;
		cmd_bufferp->cmd_rcount = sizeof (int);
		return LINK_WRITED;

	case LINKCMD_VWRITE:

		/* First, convert the virtual address into a real one */
		ddb_purge();
		cmd_bufferp->cmd_address = ddb_ltor(
						cmd_bufferp->cmd_address);
		if (!cmd_bufferp->cmd_address) {
			panic("DDB: Bad VWRITE addr");
			return LINK_CMND;
		}
		/* the sense is reversed here because VWRITE is a
		 * protocol command, while LINKCMD_READ refers to the
		 * I/O taking place to the device. */
		cmd_bufferp->cmd_command = LINKCMD_READ;
		return LINK_READD;

	case LINKCMD_VREAD:

		ddb_purge();
		/* First, convert the virtual address into a real one */
		cmd_bufferp->cmd_address = ddb_ltor(
						cmd_bufferp->cmd_address);
		if (!cmd_bufferp->cmd_address) {
			cmd_bufferp->cmd_address = ddb_data_buffer;
			*(int *)ddb_data_buffer = -1;
			return LINK_WRITED;
		}
		/* the sense is reversed here because VREAD is a
		 * protocol command, while LINKCMD_WRITE refers to the
		 * I/O taking place to the device. */
		cmd_bufferp->cmd_command = LINKCMD_WRITE;
		return LINK_WRITED;

	case LINKCMD_INTERRUPT:
		call_ddb_stop = 1;
		return LINK_CMND;
	
	default:
		panic("Bad DDB command");
		return LINK_CMND;
	}
}

/*
 * Get ready to write <count> characters to <address>.
 * Returns: The number of characters that will actually be written.
 */
ddb_write_setup(address, count)
	char *address;
	u_int count;
{
	char *dest_address = ddb_data_buffer;
	ddb_purge();
	count = rs232_copyout(address, dest_address, count);
	count = rs232_write_device(dest_address, count);
	return count;
}

/*
 * Records the fact that a dma transfer has occurred.  Decrements the transfer
 * 	count by the number of bytes transferred, and increments the target
 * 	address by the same amount.
 * Returns: 1 if the transfer is complete and 0 if it is not.
 */
ddb_write_complete(cmd_bufferp, bytes_transferred)
	struct cmd_remote *cmd_bufferp;
	u_int bytes_transferred;
{
	/* the dma transfer is complete, just decrement counters */
	cmd_bufferp->cmd_address += bytes_transferred;
	cmd_bufferp->cmd_rcount -= bytes_transferred;
	return (cmd_bufferp->cmd_rcount == 0) ? DDB_IO_DONE : DDB_IO_NOT_DONE;
}

/*
 * Records the fact that a dma transfer has occurred.  Copies the bytes in
 * 	from the transfer buffer.  Decrements the transfer count by the number
 * 	of bytes transferred, and increments the target address by the same
 * 	amount.
 * Returns: 1 if the transfer is complete and 0 if it is not.
 */
ddb_read_complete(cmd_bufferp, bytes_transferred)
	struct cmd_remote *cmd_bufferp;
	u_int bytes_transferred;
{
	char *temp_pointer;
	u_int count;

	/* dummy_pointer needed for position independent code */
	temp_pointer = ddb_data_buffer;
	count = rs232_copyin(temp_pointer, cmd_bufferp->cmd_address,
							bytes_transferred);
	if (count != bytes_transferred)
		panic("DDB bad buffer");
	cmd_bufferp->cmd_address += bytes_transferred;
	cmd_bufferp->cmd_rcount -= bytes_transferred;

	return (cmd_bufferp->cmd_rcount == 0) ? DDB_IO_DONE : DDB_IO_NOT_DONE;
}

/*
 * Performs I/O setup appropriate for the link_state.
 * Returns: the number of bytes to be transferred.
 */
ddb_io_setup(cmd_bufferp, link_state)
	struct cmd_remote *cmd_bufferp;
	int link_state;
{
	char *temp_pointer = ddb_data_buffer;

	switch (link_state) {

	case LINK_CMND:
		return rs232_read_device(temp_pointer, sizeof(*cmd_bufferp));
	
	case LINK_READD:
		return rs232_read_device(temp_pointer, cmd_bufferp->cmd_rcount);

	case LINK_WRITED:
		return ddb_write_setup(cmd_bufferp->cmd_address,
						cmd_bufferp->cmd_rcount);
	default:
		panic("DDB bad state");
		/*NOTREACHED*/
	}
}

/*
 * Initializes data structures for the inbound rs232 transfer.  Signals across
 * 	the link that the device is ready to read.
 * Returns: the actual size of the proposed transfer.
 */
rs232_read_device(buf, length)
	char *buf;
	u_int length;
{
	rs232_out_int = FALSE;
	rs232_buffer = buf;
	/* extra space for fixed encoding */
	if (length > DATA_BUFFER_SIZE)
		length = DATA_BUFFER_SIZE;
	rs232_length = length;
	rs232_int();
	return length;
}

/*
 * Copies <length> characters across the rs232 link.
 * Returns: the actual size of the transfer.
 */
rs232_write_device(buf, length)
	char *buf;
	u_int length;
{
	int i;

	rs232_buffer = buf;
	if (length > DATA_BUFFER_SIZE)
		length = DATA_BUFFER_SIZE;
	for (i = 0; i < rs232_length; i++)
		if (rs232_putchar(ddb_io_addr, rs232_buffer[i]))
			return -1;
	rs232_length = 0;
	rs232_out_int = 1;
	return length;
}

/*
 * Send a <SIG_READY> character across the rs232 line and wait for an ACK.
 */
rs232_int()
{
	rs232_putchar(ddb_io_addr, SIG_READY);
        if (!rs232_waitchar(ACK, 0))
		panic("DDB no ack");
	return 0;
}

/*
 * Wait <msec> milliseconds for the character <c> to arrive, discarding others.
 * Returns: TRUE, if the character arrives.
 * 	    FALSE, otherwise.
 */
rs232_waitchar(c, msecs)
	char c;
	int msecs;
{
	char tempchar;
	struct timeval start_time, curr_time;
	unsigned int elapsed;
	while (!msecs || --msecs) {
		while ((tempchar = rs232_getchar(ddb_io_addr)))
			if (c == tempchar)
				return 1;
		get_precise_time(&start_time);
		elapsed = 0;
		while(elapsed < 1000)
		{
		      get_precise_time(&curr_time);
		      timevalsub(&curr_time, &start_time);
		      elapsed = curr_time.tv_sec * 1000000 + curr_time.tv_usec;
		}
	}
	return 0;
}

/*
 * Copy the data read from the rs232 line, packing "hex" digits into binary,
 *	from the input buffer <unpacked> to the destination area <packed>.
 *	Count is the length of the destination area.
 * Returns: the number of characters placed in the destination area.
 */
rs232_copyin(unpacked, packed, count)
	char	*unpacked, *packed;
	unsigned int	count;
{
	register unsigned char	*i, *j;
	register unsigned char	*loop_limit;
	int rdcnt;
	int c;

	while (!rs232_waitchar(
	        link_state == LINK_CMND ? SIG_COMMAND : SIG_READY,
		/* msec = */ 10))
	{
		;
	}
	rs232_putchar(ddb_io_addr,
		link_state == LINK_CMND ? SIG_COMMAND : SIG_READY);
	rs232_in_dma = TRUE;
	for (rdcnt = 0; rdcnt < rs232_length; rdcnt++) {
		while (!rs232_receive_pending(ddb_io_addr));
		c = rs232_getchar(ddb_io_addr);
		packed[rdcnt] = c;
	}
	ddb_purge();
	rs232_in_dma = FALSE;
	rs232_length = 0;
	return count;
}

/*
 * Copy the data to be written to the rs232 line, formatting into "hex" digits
 *	for transmission, from the source <packed> to the output buffer
 *	<unpacked>.  Count is the length of the source.
 * Returns: the number of characters copied from the source area.
 */
rs232_copyout(packed, unpacked, count)
	char	*packed, *unpacked;
	unsigned int	count;
{
	extern testr();
	int i;
	char *junk;

	if (count > DATA_BUFFER_SIZE)
		count = DATA_BUFFER_SIZE;

        if (testr(packed,1)) {
		ddb_purge();
		bcopy(packed, unpacked, count);
	} else {
		junk = unpacked;
		for (i=0; i < count; i++) {
			*junk = 0xff;
			junk = (unsigned)junk + 1;
                }
	}
	rs232_buffer = unpacked;
	rs232_length = count;
	return count;
}

#endif DDB
