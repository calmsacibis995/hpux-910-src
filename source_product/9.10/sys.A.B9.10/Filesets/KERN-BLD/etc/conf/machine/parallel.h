/*
 * @(#)parallel.h: $Revision: 1.3.84.4 $ $Date: 93/12/06 14:06:00 $
 * $Locker:  $
 */

/* @(#) $Revision: 1.3.84.4 $ */     

#ifndef _SYS_PARALLEL_INCLUDED /* allows multiple inclusion */
#define _SYS_PARALLEL_INCLUDED

/*
**	Header file for the HP986PP Centronics compatible 
**	bi-directional 8 bit parallel printer interface
*/


/* 
**     Control Register Interface Definition
**     -------------------------------------
**
**     The following structure  represents  the  memory  layout  of  the
**     interface card's DIO control register set.  It is used for memory
**     mapped accesses to the interface card.   Card  offsets  represent
**     the  byte  offset of a register from the beginning of the address
**     space dtack'd by the card.  Note  that  even  addresses  are  not
**     defined  and  will  not be dtack'd by the interface.  Accesses to
**     these addresses will  buserror.   The  register  names  used  are
**     neumonic  for reading.  Register aliases neumonic for writing are
**     defined below.
*/
struct parallel_if {			/* card offsets */
	unsigned char pad1;		/*           0  */
	unsigned char id;		/* register  1  */
	unsigned char pad2;		/*           2  */
	unsigned char dio_status;	/* register  3  */
	unsigned char pad3;		/*           4  */
	unsigned char bus_status;	/* register  5  */
	unsigned char pad4;		/*           6  */
	unsigned char ext_status;	/* register  7  */
	unsigned char pad5;		/*           8  */
	unsigned char interrupt_status;	/* register  9  */
	unsigned char pad6;		/*          10  */
	unsigned char fifo;		/* register 11  */
};



/*
**     Register 1: ID/RESET
**     --------------------
**
**     Register one (byte offset  one)  is  the  standard  DIO  ID/Reset
**     register.   This  register  provides  the  interface ID when read
**     which for this interface is always 6.  Writing to  this  register
**     resets the interface to its power on state.  The fifo is flushed,
**     all state machines are reset, and all registers return  to  their
**     power  on  value.   Note  that it doesn't matter what we write to
**     this register.  The data we write is ignored and the fact that we
**     did  a  write  cycle  to  this address causes the interface to be
**     reset.  By  tradition,  drivers  usually  write  a  one  to  this
**     location to reset the interface, however, any value will do.
*/
#define reset 		id	/* register 1 define for writing */

#define PARALLEL_PRINTER_INTERFACE_ID	6



/*
**     Register 3: DIO Status/Control
**     ------------------------------
**
**     Register  three  (byte  offset  three)  is   the   standard   DIO
**     Status/Control register.  This register provides interface status
**     when read and provides interface control when written.
*/
#define dio_control 	dio_status	/* register 3 define for writing */


/*
**     Interrupt enable.  If set, this bit indicates that the  interface
**     card  is  enabled  for  interrupts.  When clear, the interface is
**     disabled from generating any interrupts.  This bit can be used to
**     turn interrupts on/off on a card wide basis.
*/
#define	PARALLEL_INT_ENABLE	0x80


/*
**     Interrupt  request.   If  set,  this  bit  indicates   that   the
**     interface   card    is    interrupting   or   would   be  if  the
**     PARALLEL_INT_ENABLE bit were set.  When clear, the  interface  is
**     not interrupting.  Note that this is a read only bit.
*/
#define	PARALLEL_INT_REQUEST	0x40 /* define for reading */


/*
**     Interrupt level.  The value in this 2  bit  field  added  to  the
**     value  3  is  the  current  interrupt  level  being  used by this
**     interface. Interrupt levels 3, 4, 5, and 6  are  possible.   Note
**     that these are read only bits.  The interrupt level cannot be set
**     with these bits.   See the hidden mode register  definitions  for
**     setting the interrupt level.
*/
#define	PARALLEL_INT_LEVEL	0x30 /* define for reading */


/*
**     This bit indicates whether or not the card is in slow mode.  When
**     clear, normal transfers are done for both input and output.  When
**     set, on output, NACK is ignored  to  finish  the  handshake.   On
**     input,  the  fifo  is disabled.  In slow mode input we get a fifo
**     full interrupt on each byte.
*/
#define	PARALLEL_SLOW		0x08


/*
**     This bit indicates the direction of the transfer to the interface
**     card.  Setting  this bit puts the card in input mode, clearing it
**     puts the card in output mode.
*/
#define	PARALLEL_IO		0x04


/*
**     The following two bits enable  and  disable  the  interface  from
**     doing  DMA.  If set, the interface is enabled to do DMA transfers
**     on the indicated DMA channel.  If clear,  DMA  on  the  indicated
**     channel is disabled.  As with all DIO interfaces, only one of the
**     DMA enable bits may be set at a time.  Bad things happen if  both
**     are set.  Also, these bits must be cleared upon DMA completion or
**     other DMA operations may (will) be corrupted.
*/
#define	PARALLEL_DMA_1		0x02
#define	PARALLEL_DMA_0		0x01



/*
**     Register 5:  Parallel Bus Status
**     --------------------------------
**
**     Register five (byte offset  five)  is  the  parallel  bus  status
**     register.   This  register  provides parallel bus hardware status
**     when read.  All writes  to  this  register  are  ignored  by  the
**     hardware and have no effect.
*/


/*
**     If set, this bit indicates that the data fifo is full.  Note that
**     for input, the interface will not assert BUSY low unless the fifo
**     is not full.  If this bit is clear, the fifo is not full.
*/
#define	PARALLEL_FIFO_FULL	0x80


/*
**     If set, this bit indicates that the data  fifo  is  empty.   Note
**     that for output, the interface will assert NSTROBE if the fifo is
**     not empty, BUSY is low, and NACK is high.
*/
#define	PARALLEL_FIFO_EMPTY	0x40


/*
**     This bit represents the current state of the  NSTROBE  signal  on
**     the parallel interface.
*/
#define	PARALLEL_STROBE		0x20


/*
**     This bit represents the current state of the BUSY signal  on  the
**     parallel interface.
*/
#define	PARALLEL_BUSY		0x10


/*
**     This bit represents the current state of the NACK signal  on  the
**     parallel interface.
*/
#define	PARALLEL_ACK		0x08


/*
**     This bit represents the current state of the NERROR signal on the
**     parallel   interface.   If  set,   the  NERROR  signal  is  being
**     asserted (low)  on  the  bus.   This  line  is  asserted  by  the
**     peripheral  and  indicates  some  device specific error condition
**     (off line, etc).
*/
#define	PARALLEL_ERROR		0x04


/*
**     This bit represents the current state of the SELECT signal on the
**     parallel   interface.   If  set,   the  SELECT  signal  is  being
**     asserted (high) on  the  bus.   This  line  is  asserted  by  the
**     peripheral and indicates that the device is ready.
*/
#define	PARALLEL_SELECT		0x02


/*
**     This bit represents the current state of the PERROR signal on the
**     parallel   interface.   If  set,   the  PERROR  signal  is  being
**     asserted (high) on  the  bus.   This  line  is  asserted  by  the
**     peripheral and indicates paper error.
*/
#define	PARALLEL_PERR		0x01



/*
**     Register 7:  Extended Status/Control
**     ------------------------------------
**
**     Register  seven  (byte offset seven)  is  the  Extended  Parallel
**     Status/Control   register.    This   register  provides  parallel
**     hardware status when read and provides control when written.
*/
#define ext_control 	ext_status	/* register 7 define for writing */


/*
**     This bit represents the current state of the NINIT signal on  the
**     parallel   interface  when  read.   Setting  this bit asserts the
**     NINIT signal (low) on the bus.  This  line  is  asserted  by  the
**     interface  to reset the peripheral.  To reset the peripheral this
**     line should be held low for at least 50 us.
*/
#define	PARALLEL_INIT		0x04


/*
**     This bit represents the current state of the NSELECT_IN signal on
**     the parallel  interface when read.  Setting  this bit asserts the
**     NSELECT_IN signal (low) on the bus.  This line is asserted by the
**     interface to indicate to the peripheral that it has been selected
**     for I/O.
*/
#define	PARALLEL_SELECTIN	0x02


/*
**     This bit represents the current state of the NAUTO_FEED_XT signal
**     on  the parallel  interface when read.  Setting  this bit asserts
**     the NAUTO_FEED_XT signal (low) on the bus.  This line is asserted
**     by the interface to indicate the direction of the transfer to the
**     peripheral.  Setting this bit indicates output mode and  clearing
**     it indicates input mode.
*/
#define	PARALLEL_WRRD		0x01


/*
**     Register 9: Interrupt Status/Control
**     ------------------------------------
**
**     Register nine (byte offset nine) is the interrupt  status/control
**     register.   This register provides hardware interrupt status when
**     read and provides interrupt control when written.  The suffix  EI
**     is  neumonic  for  "enable  interrupt"  and  IR  is  neumonic for
**     "interrupt request".
*/
#define interrupt_control interrupt_status /* register 9 define for writing */



/*
**     Setting this bit enables the fifo  full  interrupt,  clearing  it
**     disables  the  interrupt.   If  set,  this bit indicates that the
**     interface is  requesting  an  interrupt  due  to  the  fifo  full
**     condition.
*/
#define	PARALLEL_FIFO_FULL_EI	0x80 /* define for writing */
#define	PARALLEL_FIFO_FULL_IR	0x80 /* define for reading */


/*
**     Setting this bit enables the fifo empty  interrupt,  clearing  it
**     disables  the  interrupt.   If  set,  this bit indicates that the
**     interface is requesting  an  interrupt  due  to  the  fifo  empty
**     condition.
*/
#define	PARALLEL_FIFO_EMPTY_EI	0x40 /* define for writing */
#define	PARALLEL_FIFO_EMPTY_IR	0x40 /* define for reading */


/*
**     Setting this bit enables the interface to interrupt on BUSY  low,
**     clearing  it disables  the interrupt.  If set, this bit indicates
**     that the interface is requesting an interrupt  because  the  BUSY
**     signal is low.
*/
#define	PARALLEL_BUSY_EI	0x10 /* define for writing */
#define	PARALLEL_BUSY_IR	0x10 /* define for reading */


/*
**     Setting this bit enables the interface to interrupt on NACK  high
**     to  low transitions, clearing it disables the interrupt.  If set,
**     this bit indicates that the interface is requesting an  interrupt
**     because the NACK signal has transitioned from high to low.
*/
#define	PARALLEL_ACK_EI		0x08 /* define for writing */
#define	PARALLEL_ACK_IR		0x08 /* define for reading */


/*
**     Setting this bit enables the interface  to  interrupt  on  NERROR
**     transitions,  clearing  it  disables the interrupt.  If set, this
**     bit indicates that  the  interface  is  requesting  an  interrupt
**     because  the  NERROR  signal  has transitioned.  This bit is only
**     cleared by writing a zero to this bit position.
*/
#define	PARALLEL_ERROR_EI	0x04 /* define for writing */
#define	PARALLEL_ERROR_IR	0x04 /* define for reading */


/*
**     Setting this bit enables the interface  to  interrupt  on  SELECT
**     transitions,  clearing  it  disables the interrupt.  If set, this
**     bit indicates that  the  interface  is  requesting  an  interrupt
**     because  the  SELECT  signal  has transitioned.  This bit is only
**     cleared by writing a zero to this bit position.
*/
#define	PARALLEL_SELECT_EI	0x02 /* define for writing */
#define	PARALLEL_SELECT_IR	0x02 /* define for reading */


/*
**     Setting this bit enables the interface  to  interrupt  on  PERROR
**     transitions,  clearing  it  disables the interrupt.  If set, this
**     bit indicates that  the  interface  is  requesting  an  interrupt
**     because  the  PERROR  signal  has transitioned.  This bit is only
**     cleared by writing a zero to this bit position.
*/
#define	PARALLEL_PERROR_EI	0x01 /* define for writing */
#define	PARALLEL_PERROR_IR	0x01 /* define for reading */



/*
**     Hidden Mode Register 1 Definition
**     ---------------------------------
**
**     The hardware interrupt level switches are implemented  in  EEROM.
**     When hidden mode is enabled, register one of the parallel printer
**     interface is re-defined to contain the interrupt level  switches.
**     The  interrupt  level  for  the  interface can be set by enabling
**     hidden mode and then writing this register.  Register one is  the
**     only  hidden  register  defined  by  this  interface.   Only  the
**     interrupt level bits are defined.  Other bits are  undefined  and
**     will return zero when read.
*/
#define interrupt_level_control	id	/* register 1 define for hidden mode */


/*
**     Interrupt level.  The value in this 2  bit  field  added  to  the
**     value  3  is  the  current  interrupt  level  being  used by this
**     interface. Interrupt levels 3, 4, 5, and 6 are possible.  Writing
**     this  field  set the interrupt level.  The current state of these
**     bits is reflected in register three.
*/
#define PARALLEL_INT_LEVEL_MASK      0x30


/*
**     Isc_table_type Entries
**     ----------------------	
**	
**     The   following   defines   provide   pneumonic    meaning    for
**     isc_table_entries the driver will be using.  Most of these fields
**     are interface specific to the TI9914 driver and have  nothing  to
**     do  with  a  general  isc.   We  will use the entries for our own
**     purposes.
*/
#define iob_dil_state		intcopy
#define iob_read_pattern	intmsksav
#define bp_b_flags		intmskcpy
#define	parallel_rptmask	ppoll_mask

#endif _SYS_PARALLEL_INCLUDED
