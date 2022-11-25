/*
 * @(#)ddb_sio.h: $Revision: 1.2.84.3 $ $Date: 93/09/17 21:03:26 $
 * $Locker:  $
 */

/* some usefull ASCII characters */

#define	XOFF	0x13	/*ASCII XOFF*/
#define	XON	0x11	/*ASCII XON*/

/*since register_0 has different functionalities at different times, we
define three descriptive names for it*/

#define divisor_latch_lsb	register_0/* accessed when DLAB is 1(r/w)*/
#define rec_buffer_reg		register_0/*accessed when register is read*/
#define trans_holding_reg	register_0/*accessed when reg is written*/
					  /* DLAB is 0 for rec/trans regs*/

/*since register_1 has different functionalities at different times, we
define two descriptive names for it*/

#define	divisor_latch_msb	register_1  /*accessed when DLAB is 1(r/w)*/
#define	intr_enable_reg		register_1    /*r/w register when DLAB is 0*/

/*since register_2 has different functionalities at different times, we
define two descriptive names for it*/

#define	intr_ident_reg	register_2    /*accessed when reg 2 is read*/
#define	fifo_cntr_reg	register_2   /*accessed when reg 2 is written*/

/* structure representing the NS16550 internal registers in s375*/

typedef	struct
{
	unsigned char	pcif1;	/*extra space used in s375 addressing*/
	unsigned char  register_0;

	unsigned char	pcif2;	/*extra space used in s375 addressing*/
	unsigned char	register_1;

	unsigned char	pcif3;	/*extra space used in s375 addressing*/
	unsigned char	register_2;

	unsigned char	pcif4;	/*extra space used in s375 addressing*/
	unsigned char	line_cntr_reg;     /*r/w register*/

	unsigned char	pcif5;	/*extra space used in s375 addressing*/
	unsigned char 	modem_cntr_reg;    /*r/w register*/

	unsigned char	pcif6;	/*extra space used in s375 addressing*/
	unsigned char	line_status_reg;   /*read only register*/

	unsigned char	pcif7;	/*extra space used in s375 addressing*/
	unsigned char	modem_status_reg;  /*read only register*/
	
	unsigned char	pcif8;	/*extra space used in s375 addressing*/
	unsigned char	scratch_pad_reg;   /*r/w register for testing purposes*/

}	SIO_CHAN_REGS;

/* SIO bit constants in different registers */

/* bits for the Modem Control Register*/

#define	MCR_HWFC_DISABLE	0x04	/*hardware flow control disable */
#define	MCR_RTS_ENABLE		0x02	/*Request To Send enable*/
#define	MCR_DTR_ENABLE		0x01	/*Data Terminal Ready enable*/

/* bits for the Line Status Register */

#define	LSR_DATA_READY_MASK	0x01	/*all bits low but bit 0(DR) */
#define	LSR_DATA_READY		0x01	/*all bits low but bit 0(DR) */
#define	LSR_ERROR_MASK		0x8E	/*all bits low except error bits */
#define	LSR_PARITY_ERROR_MASK	0x04	/*all bits low but bit 2(PE) */
#define	LSR_THR_EMPTY_MASK	0x20	/*all bits low but bit 5(THRE) */
#define	LSR_THR_EMPTY		0x20	/*all bits low but bit 5(THRE) */
#define	LSR_TRANSMITTER_EMPTY_MASK	0x40	/*only bit 6(TEMT) is high*/
#define	LSR_TRANSMITTER_EMPTY	0x40	/*only bit 6(TEMT) is high*/
#define	LSR_TRANS_EMPTY		0x60	/*all bits low but 5 and 6*/

/*bits for FIFO Control Register */
#define	FCR_FIFOS_CLEAR_AND_ENABLE	0x7	/*enable fifo mode and clear*/
#define	FCR_FIFO_MODE			0x01	/*enable fifo mode */
#define	FCR_REC_FIFO_CLEAR		0x3	/*only clear receive fifo*/

/*bits for Interrupt Identification Register */
#define	IIR_FIFO_MODE_NO_INTR		0xc1	/*bits 7,6 and 1 are set */

/*bits for the Line Control Register */

#define	LCR_DLAB_RESET		0x7f	/*all bits set but DLAB bit 7 */
#define	LCR_DLAB_SET		0x80	/*all bits reset but bit 7 */

/*layer-0 masks */

#define	LCR_LAYER_MASK		0x3f	/*only LCR related bits set */
#define	DL_LAYER_MASK		0x3c0	/*only baudrate info bits are set */

/*bits for Interrupt Enable Register*/

#define	IER_INT_DISABLE		0x0

/* all the baudrate related constants */

#define	BAUD9600		0x280
#define BAUD9600LSB		0x10
#define	BAUD9600MSB		0x0

#define	BAUD19200		0x2c0
#define BAUD19200LSB		0x18
#define	BAUD19200MSB		0x0
