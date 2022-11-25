/*
 * @(#)vtest.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 20:35:50 $
 * $Locker:  $
 */

#ifdef THIS_IS_A_PLACE_HOLDER
#ifndef _MACHINE_VTEST_INCLUDED /* allows multiple inclusion */
#define _MACHINE_VTEST_INCLUDED
#ifdef __hp9000s300

struct vmetestregs {					/* at powerup */
		short	ram_addr;      /* Reg 0-1 */    /* undefined  */
		char	low_ram_bit;   /* Reg 2   */	/* undefined  */
		char	ram_AM;	       /* Reg 3   */	/* 	0     */
		long	dest_addr;     /* Reg 4-7 */    /*	0     */
		char	pad_8;	       /*  unused */
		char	status;	       /* Reg 9   */    /*	0     */
		char	pad_10;	       /*  unused */
		char	dest_AM;       /* Reg 11  */    /* undefined  */
		short	pad_12_13;     /*  unused */
		short	tfr_count;     /*Reg 14-15*/	/*	0     */
		char	reset;         /* Reg 16  */
		char	pad[112];      /*  unused */
		char	int0;				/*	0     */
		char	pad_1;	       /*  unused*/
		char	int1;				/*	0     */
		char 	pad_2;	       /*  unused */
		char	int2;				/*	0     */
		char	pad_3;	       /*  unused*/
		char	int3;				/*	0     */
		char	pad_4;	       /*  unused*/
		char	intvec0;			/*	$0F   */
		char	pad_1x;        /*  unused*/
		char 	intvec1;			/*	$0F   */
		char	pad_2x;        /*  unused*/
		char	intvec2;			/*	$0F   */
		char 	pad_3x;        /*  unused*/
		char	intvec3;			/*	$0F   */
};

/* aliases for multi-purpose registers */
#define assert_intr	low_ram_bit
#define control		status

/* register 2 bits (low_ram_bit) */
#define	LOW_RAM_ADDR	0x80
#define	CLEAR_INT2	0x4
#define	CLEAR_INT1	0x2
#define	CLEAR_INT0	0x1

/* register 3 bits (ram_AM) */
#define	RAM_ENABLE	0x40
#define	RAM_AM_MASK	0x3F	/* = (ram_AM & ~RAM_AM_MASK) | (new_mask & RAM_AM_MASK) */

/* status/control bits */
#define	DMA_ENABLE	0x01
#define	BR_MASK		0x6	/* fields for bus request lines */
#define SET_BR3		0x0
#define SET_BR2		0x1
#define SET_BR1		0x2
#define SET_BR0		0x3
#define DMA_READ	0
#define DMA_WRITE	0x8
#define ROR		0x10	/* set release-on-request */
#define RMW		0x20	/* set read-modify-write cycle */
#define PIPE		0x40	/* enable address pipelining */
#define BERR		0x80	/* assert bus error */

/* register 11 bits (dest_AM) */
#define DMA_AM_MASK	0x3F	/* mask for DMA address modifier */
#define DMA_DATA_MASK	0xC0	/* mask for DMA data modes */
#define DMA_DISABLE	0
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


/* list of initialized VME test card select codes */
struct active_vme_sc {
	int 			sc;
	struct vmetestregs      *card_ptr;
	struct active_vme_sc	*next;
};

/* size of RAM on test card (32K) */
#define RAM_SIZE	32768

/* power-up default for interrupt level */
#define INT_LVL		2

/* isc table entry re-defines */
#define dma_data_mode	dma_active
#define use_iomap	locks_pending
#define vt_ram		mapped

/* these are for holding interrupt vectors per card for RESET ioctl */
#define  vec_num1 	lock_count 
#define  vec_num2 	intr_wait

/* transfer types for SET_TFR_TYPE ioctl */
#define DMA_TRANSFER	1
#define FHS_TRANSFER	0

/* defines for SET_USE_IOMAP */
#define VTEST_USE_IOMAP	1
#define VTEST_NO_IOMAP	0

/* defines for SET_USE_COMPAT */
#define VTEST_USE_BUILDCH	1
#define VTEST_USE_SETUP		0

/* ioctl's for vme test card 			     valid parameters  			*/
#define SET_RAM_ADDR	_IOW('v', 1, long)  /* valid RAM address (in hex)		*/
#define SET_RAM_AM	_IOW('v', 2, int)   /* A16_AM, A24_AM, A32_AM, also obscure ones*/
#define SET_DATA_WIDTH	_IOW('v', 3, int)   /* BYTE_DMA, WORD_DMA, LONG_DMA		*/
#define SET_BR_LINE	_IOW('v', 4, int)   /* SET_BR3, SET_BR2, SET_BR1, SET_BR0	*/
#define SET_TFR_TYPE	_IOW('v', 5, int)   /* DMA_TRANSFER, FHS_TRANSFER		*/
#define SET_RWD		_IO('v', 6)  	    /* No parameters				*/
#define SET_ROR		_IO('v', 7)  	    /* No parameters				*/
#define SET_RMW		_IO('v', 8)  	    /* No parameters				*/
#define INTERRUPT	_IO('v', 9)  	    /* No parameters				*/
#define SET_INT_LVL	_IOW('v', 10, int)  /* 1 - 6					*/
#define SET_USE_IOMAP	_IOW('v', 11, int)  /* VTEST_USE_IOMAP, VTEST_NO_IOMAP		*/
#define SET_USE_COMPAT	_IOW('v', 12, int)  /* VTEST_USE_BUILDCH, VTEST_USE_SETUP	*/
#define VTEST_RESET     _IO('v', 13)  	    /* No parameters				*/

#define ISCSIZE sizeof(struct isc_table_type)
#endif /* __hp9000s300 */
#endif /* _MACHINE_VTEST_INCLUDED */
#endif /* palce holder */
