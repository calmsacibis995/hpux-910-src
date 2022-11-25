/*
 * @(#)vtest2.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 20:36:03 $
 * $Locker:  $
 */

#ifndef _MACHINE_VTEST2_INCLUDED /* allows multiple inclusion */
#define _MACHINE_VTEST2_INCLUDED

struct vtest2regs {					/* at powerup */
	short	ram_addr;	/* Reg 0-1 */	/* undefined  */
	char	low_ram_bit;	/* Reg 2   */	/* undefined  */
	char	ram_AM;		/* Reg 3   */	/* 	0     */
	long	dest_addr;	/* Reg 4-7 */	/*	0     */
	char	pad_8;		/*  unused */
	char	status;		/* Reg 9   */	/*	0     */
	char	pad_10;		/*  unused */
	char	dest_AM;	/* Reg 11  */	/* undefined  */
	short	pad_12_13;	/*  unused */
	short	tfr_count;	/*Reg 14-15*/	/*	0     */
	char	reset;		/* Reg 16  */
	char	pad_a[15];	/*  unused */
	char	bus_error;	/* Reg 32  */
	char	pad_b[96];	/*  unused */
	char	int0;		/* 0x81 129*/	/*	0     */
	char	pad_1;		/*  unused */
	char	int1;		/* 0x83 131*/	/*	0     */
	char 	pad_2;		/*  unused */
	char	int2;		/* 0x85 133*/	/*	0     */
	char	pad_3;		/*  unused */
	char	int3;		/* 0x87 135*/	/*	0     */
	char	pad_4;		/*  unused */
	char	intvec0;	/* 0x89 137*/	/*	$0F   */
	char	pad_1x;		/*  unused */
	char 	intvec1;	/* 0x8b 139*/	/*	$0F   */
	char	pad_2x;		/*  unused */
	char	intvec2;	/* 0x8d 141*/	/*	$0F   */
	char 	pad_3x;		/*  unused */
	char	intvec3;	/* 0x8F 143*/	/*	$0F   */
};

/* aliases for multi-purpose registers */
#define assert_intr	low_ram_bit
#define control		status

/*
 * per card structure information
 */
struct vtest2_card_info {
	caddr_t ram_virtual;
	caddr_t ram_physical;
	int vec_num1;
	int vec_num2;
	int transfer;
	int tfr_control;
	int use_iomap;
	int dma_data_mode;
};

/* register 2 bits (low_ram_bit) */
#define	LOW_RAM_ADDR	0x80
#define	CLEAR_INT2	0x4
#define	CLEAR_INT1	0x2
#define	CLEAR_INT0	0x1

/* register 3 bits (ram_AM) */
#define	RAM_ENABLE	0x40
#define	RAM_AM_MASK	0x3F	/* = (ram_AM & ~RAM_AM_MASK) | (new_mask & RAM_AM_MASK) */

/* status/control bits */
#define	VDMA_ENABLE	0x01
#define	BR_MASK		0x6	/* fields for bus request lines */
#define SET_BR3		0x0
#define SET_BR2		0x1
#define SET_BR1		0x2
#define SET_BR0		0x3
#define VDMA_READ	0
#define VDMA_WRITE	0x8
#define ROR		0x10	/* set release-on-request */
#define RMW		0x20	/* set read-modify-write cycle */
#define PIPE		0x40	/* enable address pipelining */
#define BERR		0x80	/* assert bus error */

/* register 11 bits (dest_AM) */
#define VDMA_AM_MASK	0x3F	/* mask for DMA address modifier */
#define VDMA_DATA_MASK	0xC0	/* mask for DMA data modes */
#define VDMA_DISABLE	0
#define BYTE_DMA	0x1
#define WORD_DMA	0x2
#define LONG_DMA	0x3

/* int* bits (interrupt controller) */
#define INT_LVL_MASK	0x7		/* mask for interrupt level bits */
#define INT_ENABLE	0x10
#define INT_CLEAR	0x8		/* should always be 0 */
#define INT_EXTERN	0x20		/* should always be 0 */
#define FLAG_CLEAR	0x40		/* should always be 0 */
#define FLAG		0x80		/* should always be 0 */

/* defines for the usual address modifiers */
#define A16_AM		0x29
#define A24_AM		0x39
#define A32_AM		0x09


/* size of RAM on test card (32K) */
#define RAM_SIZE	32768

/* power-up default for interrupt level */
#define INT_LVL		2

/* transfer types for SET_TFR_TYPE ioctl */
#define DMA_TRANSFER	1
#define FHS_TRANSFER	0

/* defines for SET_USE_IOMAP */
#define VTEST_USE_IOMAP	1
#define VTEST_NO_IOMAP	0

/* defines for SET_USE_COMPAT */
#define VTEST_USE_BUILDCH	1
#define VTEST_USE_SETUP		0

/* ioctl */
#define READ_TEST	_IOW('v', 1, unsigned char)  /* set all of ram contents to value */


#endif /* _MACHINE_VTEST2_INCLUDED */
