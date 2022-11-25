/*
 * @(#)eisa.h: $Revision: 1.8.83.4 $ $Date: 93/12/09 15:25:27 $
 * $Locker:  $
 */

#ifndef _SYS_EISA_INCLUDED /* allows multiple inclusion */
#define _SYS_EISA_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#if (defined(__hp9000s800) && defined(_WSIO))
#ifdef _KERNEL_BUILD
#include "../machine/timeout.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/timeout.h>
#endif /* _KERNEL_BUILD */
#endif

/*
 * Definitions for basic EISA constants for first EISA bus on 68K and PA 
 */

#define EISA1 0					/* EISA bus number - first bus */

/*
 * Hardwired physical host addresses
 */
#define EISA1_BASE_ADDR_68K 	0x30000000	
#define EISA1_IOMAP_ADDR_68K    0x30100000

#define EISA1_BASE_ADDR_PA 	0xfc000000
#define EISA1_IOMAP_ADDR_PA     0xfc100000

#ifdef __hp9000s300
#define EISA1_BASE_ADDR EISA1_BASE_ADDR_68K
#define EISA1_IOMAP_ADDR EISA1_IOMAP_ADDR_68K 
#define NUM_EISA_BUSES	1
#endif

#if defined(__hp9000s700) && defined(_WSIO)
#define EISA1_BASE_ADDR EISA1_BASE_ADDR_PA
#define EISA1_IOMAP_ADDR EISA1_IOMAP_ADDR_PA
#define NUM_EISA_BUSES	1

#define EISA1_MG_REG_BASE       0xfc010000      /* Host addr of Mongoose regs */
#define EISA1_IACK_REG  	0xfc01f000	/* Host addr of IACK register */
#endif

#define EISA1_NUM_SLOTS_MAX  8

/*
 * EISA system board ids
 */
#define COBRA_ESB_ID	0x22f0c000
#define CORAL_ESB_ID	0x22f0c010
#define TWAYS_ESB_ID	0x22f0c020
#define MACE_ESB_ID	0x22f0c031

#define EISA1_MIN 	0x40
#define EISA_REG_PAGE 	0xfff
#define EISA_MAXPHYS	(4*1024*1024)

#define valid_eisa_sc(sc) ((((sc) & 0xf0) == EISA1_MIN) && (((sc) & 0xf) > 0) \
			&& (((sc) & 0xf) <= eisa1_num_slots))

#define CARD_ID_OFFSET 		0xc80
#define CARD_CONTROL_OFFSET 	0xc84
#define EISA_CARD_ENABLE	0x1
#define IOCHKERR		0x2
#define IOCHKRESET		0x4

/*
 * Structure for maintaining essential information on various
 * system boards.
 */
struct eisa_sysbrd_parms {
	int id;                 /* system board id */
	int num_slots;		/* default number of slots */	
	int max_slots;		/* max number of slots based on eeprom slot information storage */
	int size_iomap;         /* size of iomap hardware */ 
};

/*
 * Data  structures for EISA drivers:  the eisa  resources  table, which
 * consists of a DMA channel table  describing  each DMA channel, and an
 * iomap table defining each iomap entry.
 */

#define NUM_DMA_CH_PER_BUS 	8
#define NUM_IRQ_PER_BUS 	16

struct dma_channel_data {
	struct isc_table_type *locked;		/* who owns channel */
	/* software copies of write-only registers */
	unsigned char dma_mode_sw;		/* sw copy of dma[0,1]_mode register */
	unsigned char dma_extended_mode_sw; 	/* sw copy, dma[0,1]_extended_mode reg */
	struct driver_to_call *driver_wait_list; /* drivers to call on free */
	struct dma_parms *dma_parms;        	/* Point to current dma_parms */
	struct addr_chain_type *next_cp;    	/* Next chain element to do */
};

struct irq_isr {
	struct irq_isr *next;   	/* Next ISR for this IRQ line */
	int (*isr)();           	/* ISR to call */
	caddr_t arg;            	/* Argument to pass to ISR */
	struct isc_table_type *isc;     /* Interface table entry */
};


#define NUM_IRQ_PER_BUS 16
struct irq_isr *eisa_rupttable[NUM_IRQ_PER_BUS];

#if defined (__hp9000s800) && defined (_WSIO)
struct mongoose_reg {
        u_char pad;
        u_char bus_lock;
        u_char liowait;
        u_char bus_speed;
};
#endif

/* 
 * EISA_BUS resources definition	
 */
struct eisa_bus_info {
	struct eisa_system_board *esb;	/* EISA system board */
	caddr_t eisa_base_addr;
	struct dma_channel_data	dma_channels[NUM_DMA_CH_PER_BUS]; /* DMA chan data */
	/* Software copies of DMA Status Registers */
	u_char dma0_status_sw;
	u_char dma1_status_sw;

#ifdef OLD_STUFF
	struct iomap_data *iomap_data;	/* Iomap data */
	int * iomap;    		/* Host base address of iomap */
	int eisa_iomap_base; 		/* EISA base address of iomap */
#endif

	int number_of_slots;            /* separate from sysbrd_parms */
	struct eisa_sysbrd_parms *sysbrd_parms;
#if defined (__hp9000s700) && defined (_WSIO)
        u_char *iack_reg;
        struct mongoose_reg *mongoose_reg;
	struct sw_intloc eisa_intloc[NUM_IRQ_PER_BUS];
#endif
};

/* 
 * EISA_BUS if_info definition	
 */
struct eisa_if_info {
	int	flags;		/* Various flags (INITIALIZED, etc.) */
	char	slot;		/* EISA bus slot		     */
};

/* bits used in flags field */
#define HAS_IOCHKERR	0x1
#define INITIALIZED	0x2
#define INIT_ERROR	0x4
#define IS_ISA_CARD	0x8

/* values returned by last attach about a card initialization */
#define ATTACH_INIT_ERROR      -1
#define ATTACH_OK		3
#define ATTACH_NO_DRIVER	1

/* dma_options bits */
#define DMA_ISA		0x1
#define DMA_TYPEA	0x2
#define DMA_TYPEB	0x4
#define DMA_BURST	0x8
#define DMA_TYPEC 	DMA_BURST
#define DMA_DEMAND	0x10
#define DMA_SINGLE	0x20
#define DMA_BLOCK	0x40
#define DMA_CASCADE	0x80  	/* used by 16bit ISA masters only */
#define DMA_8BYTE	0x100
#define DMA_16WORD	0x200
#define DMA_16BYTE	0x400
#define DMA_32BYTE	0x800
#define DMA_READ	0x1000 
#define DMA_WRITE	0x2000
#define DMA_SKIP_IOMAP	0x4000

/* dma_options bits for bus masters only */
#define ISA_CHANNEL_MASK	0xf

/* bit definitions for flags field of dma_parms - bits cleared are defaults */
#define ADDR_CHAIN	0x1	/* reserved for future use */
#define NO_CHECK	0x2	/* don't perform error checking */
#define NO_ALLOC_CHAIN	0x4	/* allocate chain for addr/count's */
#define EISA_BYTE_ENABLE 0x8	/* card supports EISA byte enable lines */
#if defined (__hp9000s700) && defined (_WSIO)
#define NO_WAIT		0x10	/* if set, don't wait during MALLOC call */
#define USER_PHYS	0x20    /* indicates that buffer was user space */
#define PAGELETS	0x100   /* indication to cleanup code that we need page copies */
#endif


/* errors returned by dma_setup */
#define UNSUPPORTED_FLAG	-1
#define RESOURCE_UNAVAILABLE	-2
#define BUF_ALIGN_ERROR		-3
#define MEMORY_ALLOC_FAILED	-4
#define TRANSFER_SIZE		-5
#define INVALID_OPTIONS_FLAGS	-6
#define ILLEGAL_CHANNEL		-7
#define NULL_CHAINPTR		-8
#define BUFLET_ALLOC_FAILED	-9

/* channel field for bus master DMA */
#define BUS_MASTER_DMA 	-1	/* set if this setup is for bus master */

/*
 * Bit definitions for the EISA interval timer registers 
 */

/* Interval Timer Control Word settings */
#define COUNT_BCD	0x1
#define MODE_0		0x0
#define MODE_1		0x2
#define MODE_2		0x4
#define MODE_3		0x6
#define MODE_4		0x8
#define MODE_5		0xa
#define COUNTER_LATCH	0x0
#define RW_LEAST	0x10
#define RW_MOST		0x20
#define RW_BOTH		0x30
#define COUNTER_0_SELECT 0x0
#define COUNTER_1_SELECT 0x40
#define COUNTER_2_SELECT 0x80
#define READ_BACK	0xc0


/*
 * Bit definitions for the EISA interrupt controller registers 
 */

/* ICW1 bits */
#define ICW1	0x10	/* This is a write to ICW1 */
#define LTIM	0x8	/* not set for EISA */
#define ADI	0x4	/* ignored for EISA */
#define SNGL	0x2	/* clear=multiple int controllers on EISA;set=single controller */
#define ICW4_USED	0x1	/* set = need ICW4 access */

/* ICW2 values */
#if defined (__hp9000s800) && defined (_WSIO)
#define EISA1_MASTER_VECTOR   	0x00
#define EISA1_SLAVE_VECTOR    	0x08
#else /* not SNAKES, 68K */
#define EISA1_MASTER_VECTOR     0x80
#define EISA1_SLAVE_VECTOR      0x88
#endif

/* ICW3 values */
#define SLAVE 0x2	/* IRQ level used to cascade from master */
#define MASTER 0x4	/* I have a slave */

/* ICW4 bits */
#define SFNM	0x10	/* special fully nested mode */
#define BUF	0x8	/* not set on EISA */
#define M-S	0x4	/* ignored for EISA */
#define AEOI 	0x2	/* auto EOI mode */
#define uPM	0x1	/* 1 on EISA */

/* OCW2 bits */
#define CMD_IRQ0	0x0  /* these select the IRQ line to act on */
#define CMD_IRQ1	0x1
#define CMD_IRQ2	0x2
#define CMD_IRQ3	0x3
#define CMD_IRQ4	0x4
#define CMD_IRQ5	0x5
#define CMD_IRQ6	0x6
#define CMD_IRQ7	0x7
#define CMD_IRQ8	0x0
#define CMD_IRQ9	0x1
#define CMD_IRQ10	0x2
#define CMD_IRQ11	0x3
#define CMD_IRQ12	0x4
#define CMD_IRQ13	0x5
#define CMD_IRQ14	0x6
#define CMD_IRQ15	0x7
#define NS_EOI		0x20 /* do non-specific EOI on this controller */
#define SP_EOI 		0x60 /* do a specific EOI on this controller */ 
#define SP_NOP 		0x40 /* nop required as workaround to ISP errata */
#define SET_PRIORITY    0xc0 /* used to set fixed interrupt priority with OCW2 */
#define OCW3		0x8  /* this is an OCW3 command */
#define READ_ISR	0x3  /* OCW3 command, next read is ISR register */
#define READ_IRR	0x2  /* OCW3 command, next read is IRR register */

/* NMI status bits */
#define PARITY_DISABLED 0x4  /* disable parity error NMI's */
#define IOCHK_DISABLED	0x8  /* disable IOCHK NMI's */
#define IOCHK_NMI	0x40 /* IOCHK NMI is active */
#define PARITY_NMI	0x80 /* parity error NMI is active */

/* NMI extended status bits */
#define RESET_SYSTEM_BUS 0x1
#define SW_ENABLED	0x2	/* enable software generated NMI's */
#define FAILSAFE_ENABLED 0x4	/* enable failsafe timer expired NMI's */
#define BUS_TO_ENABLED	0x8	/* enable bus timeout NMI's */
#define SW_NMI		0x20	/* software generated NMI is active */
#define BUS_TO_NMI	0x40	/* bus timeout NMI is active */
#define FAILSAFE_NMI	0x80	/* failsafe timer expired NMI is active */

/*
 * Bit definitions for the DMA controller registers   
 */

/* select specific channel, defined for several registers */
#define CHANNEL_0_SELECT	0x0
#define CHANNEL_1_SELECT	0x1
#define CHANNEL_2_SELECT	0x2
#define CHANNEL_3_SELECT	0x3
#define CHANNEL_4_SELECT	0x0
#define CHANNEL_5_SELECT	0x1
#define CHANNEL_6_SELECT	0x2
#define CHANNEL_7_SELECT	0x3

/*
 * Note that the way to use the _MASK definitions to clear the old value
 * of a  register  field and set a new value  is: (reg = (reg & ~mask) |
 * new bits).  This implies that for write-only registers  with multiple
 * bit fields to be updated,  we'll need to keep a software  copy of the
 * current register settings.
 */

/* extended mode register bits, uses channel select bits */
#define  ADDR_MODE_MASK	0xc	/* mask to select address mode bits */
#define BYTE_8BIT	0x0 
#define WORD_16BIT	0x4
#define BYTE_32BIT	0x8
#define BYTE_16BIT	0xc

#define CYCLE_TIMING_MASK 0x30	/* mask to select cycle timing bits */
#define ISA_TIMING	0x00 
#define TYPEA_TIMING	0x10
#define TYPEB_TIMING	0x20
#define TYPEC_TIMING	0x30
#define BURST TYPEC_TIMING

#define TC_INPUT	0x40 	/* set=TC on input; clear=TC on output */
#define STOP_DISABLED	0x80	/* use of stop register disabled */

/* chaining mode register bits */
#define MODE_MASK	0x0c	/* mask to select chaining mode bits */
#define DISABLE_CHAINING 0x0
#define ENABLE_CHAINING	0x4
#define START_CHAINING	0x8

#define ON_CHAIN_TERMINATION	0x10 /* set = IRQ13 on completion;
					clear = TC on completion */

/* channel mode status bits - channel enable */
#define CHANNEL_0_ENABLED	0x1
#define CHANNEL_1_ENABLED	0x2
#define CHANNEL_2_ENABLED	0x4
#define CHANNEL_3_ENABLED	0x8
#define CHANNEL_5_ENABLED	0x20
#define CHANNEL_6_ENABLED	0x40
#define CHANNEL_7_ENABLED	0x80

/* channel interrupt status bits - for chaining mode */
#define CHANNEL_0_IRQ13		0x1 /* IRQ13 pending for this channel */
#define CHANNEL_1_IRQ13		0x2
#define CHANNEL_2_IRQ13		0x4
#define CHANNEL_3_IRQ13		0x8
#define CHANNEL_5_IRQ13		0x20
#define CHANNEL_6_IRQ13		0x40
#define CHANNEL_7_IRQ13		0x80

/* command register bits */
#define DISABLE_ALL_CHANNELS	0x4	/* set=disabled; clear=enabled */
#define ROTATING_PRIORITY	0x10	/* set=rotating; clear=fixed */
#define DRQ_LOW			0x40	/* set = DRQ asserted low	*/
#define DAK_HIGH		0x80 	/* set = DAK asserted high 	*/

/* mode register bits, uses channel select bits */
#define TRANSFER_TYPE_MASK	0xc /* mask to select tfr type bits */
#define VERIFY_TRANSFER		0x0

/* 
 * The EISA spec  defines  these  backwards,  it  defines a "read" to be
 * memory  to  device  and a  "write  to  be  device  to  memory.  These
 * definitions  have been  reversed  from the spec so they make sense to
 * driver writers and kernel software engineers
 */

#define READ_TRANSFER		0x4 /* device to memory transfer */
#define WRITE_TRANSFER		0x8 /* memory to device transfer */

#define ENABLE_AUTO_INIT	0x10
#define DECREMENT_ADDR		0x20  /* set=decrement; clear=increment */

#define CHANNEL_MODE_MASK 	0xc0  /* mask to select mode bits */
#define DEMAND_MODE		0x0
#define	SINGLE_MODE		0x40
#define BLOCK_MODE		0x80
#define CASCADE_MODE		0xc0
	
/* request register bits */
#define REQUEST_DMA		0x4 /* sw DMA request feature (not used)*/	

/* single mask register bits */
#define SET_MASK		0x4

/* all mask register bits */
#define SET_CHANNEL0_MASK	0x1
#define SET_CHANNEL1_MASK	0x2
#define SET_CHANNEL2_MASK	0x4
#define SET_CHANNEL3_MASK	0x8
#define SET_CHANNEL4_MASK	0x1
#define SET_CHANNEL5_MASK	0x2
#define SET_CHANNEL6_MASK	0x4
#define SET_CHANNEL7_MASK	0x8

/* status register bits */
#define CHANNEL0_TC		0x1
#define CHANNEL1_TC		0x2
#define CHANNEL2_TC		0x4
#define CHANNEL3_TC		0x8
#define CHANNEL0_REQUEST	0x10
#define CHANNEL1_REQUEST	0x20
#define CHANNEL2_REQUEST	0x40
#define CHANNEL3_REQUEST	0x80
#define CHANNEL4_TC		0x1
#define CHANNEL5_TC		0x2
#define CHANNEL6_TC		0x4
#define CHANNEL7_TC		0x8
#define CHANNEL4_REQUEST	0x10
#define CHANNEL5_REQUEST	0x20
#define CHANNEL6_REQUEST	0x40
#define CHANNEL7_REQUEST	0x80

/*
 * Definition of the EISA system board hardware data structure 
 */

struct eisa_system_board {
	unsigned char dma0_ch0_base_addr; /* 000 (rw) - write DMA addr bytes 0 & 1 here	*/
	unsigned char dma0_ch0_count;	/* 001 (rw) - write DMA count bytes 0 & 1 here	*/
	unsigned char dma0_ch1_base_addr; /* 002 (rw) - write DMA addr bytes 0 & 1 here	*/
	unsigned char dma0_ch1_count;	/* 003 (rw) - write DMA count bytes 0 & 1 here	*/
	unsigned char dma0_ch2_base_addr; /* 004 (rw) - write DMA addr bytes 0 & 1 here	*/
	unsigned char dma0_ch2_count;	/* 005 (rw) - write DMA count bytes 0 & 1 here	*/
	unsigned char dma0_ch3_base_addr; /* 006 (rw) - write DMA addr bytes 0 & 1 here	*/
	unsigned char dma0_ch3_count;	/* 007 (rw) - write DMA count bytes 0 & 1 here	*/
	unsigned char dma0_status;	/* 008 (ro); (wo) = command 			*/
	unsigned char dma0_request;	/* 009 (wo) 					*/
	unsigned char dma0_single_mask;  /* 00a (wo) 					*/
	unsigned char dma0_mode;	/* 00b (wo) [ sw copy is kept ]			*/
	unsigned char dma0_clr_byte_ptr; /* 00c (wo) 					*/
	unsigned char dma0_master_clr;	/* 00d (wo) 					*/
	unsigned char dma0_clr_mask;	/* 00e (wo) 					*/
	unsigned char dma0_mask_all;	/* 00f (rw) - write=en/disable; read=get status */ 
	unsigned char pad1[16];		/* 010-01f 					*/
	unsigned char int1_icw1;	/* 020 (rw) - icw1, ocw2, ocw3 			*/
	unsigned char int1_int_mask_reg;/* 021 (rw) - interrupt mask, icw2, icw3, icw4 	*/
	unsigned char pad2[30];		/* 022 - 03f 					*/
	unsigned char timer1_clock; 	/* 040 - interval timer 1, system clock		*/
	unsigned char timer1_refresh_rqst; /* 041 - interval timer 1, refresh request	*/
	unsigned char timer1_speaker; 	/* 042 - interval timer 1, speaker tone		*/
	unsigned char timer1_control;   /* 043 - interval timer 1, control word		*/
	unsigned char pad3[4];		/* 044 - 047 					*/
	unsigned char interval_timer2[3]; /* 048-04a - unused regs for interval timer 2	*/
	unsigned char timer2_control;   /* 04b - interval timer 2, control word		*/
	unsigned char pad4[21];		/* 04c - 060 					*/
	unsigned char nmi_status;	/* 061 (rw) 					*/
	unsigned char pad5[14];		/* 062 - 06f 					*/
	unsigned char nmi_enable;	/* 070 (wo) 					*/
	unsigned char pad6[16];		/* 071 - 080 					*/
	unsigned char dma0_ch2_low_page;/* 081 (rw) - write DMA addr byte 2 here 	*/
	unsigned char dma0_ch3_low_page;/* 082 (rw) - write DMA addr byte 2 here 	*/
	unsigned char dma0_ch1_low_page;/* 083 (rw) - write DMA addr byte 2 here 	*/
	unsigned char pad7[3];		/* 084 - 086 - reserved bytes 			*/
	unsigned char dma0_ch0_low_page; /* 087 (rw) - write DMA addr byte 2 here 	*/
	unsigned char pad8;		/* 088 - reserved byte 				*/
	unsigned char dma1_ch6_low_page; /* 089 (rw) - write DMA addr byte 2 here 	*/
	unsigned char dma1_ch7_low_page; /* 08a (rw) - write DMA addr byte 2 here 	*/
	unsigned char dma1_ch5_low_page; /* 08b (rw) - write DMA addr byte 2 here 	*/
	unsigned char pad9[3];		/* 08c - 08e - reserved bytes 			*/
	unsigned char refresh_low_page;	/* 08f (rw) - refresh low page reg - unused? 	*/
	unsigned char pad10[16];	/* 090 - 09f 					*/
	unsigned char int2_icw1;	/* 0a0 (rw) - icw1, ocw2, ocw3 			*/
	unsigned char int2_int_mask_reg; /* 0a1 (rw) - interrupt mask, icw2, icw3, icw4 */
	unsigned char pad11[30];	/* 0a2 - 0bf 					*/
	unsigned char dma1_ch4_base_addr; /* 0c0 (rw) - write DMA addr bytes 0 & 1 here */
	unsigned char pad12;		/* 0c1 						*/
	unsigned char dma1_ch4_count;	/* 0c2 (rw) - write DMA count bytes 0 & 1 here 	*/
	unsigned char pad13;		/* 0c3 						*/
	unsigned char dma1_ch5_base_addr; /* 0c4 (rw) - write DMA addr bytes 0 & 1 here */
	unsigned char pad14;		/* 0c5 						*/
	unsigned char dma1_ch5_count;	/* 0c6 (rw) - write DMA count bytes 0 & 1 here 	*/
	unsigned char pad15;		/* 0c7 						*/
	unsigned char dma1_ch6_base_addr; /* 0c8 (rw) - write DMA addr bytes 0 & 1 here */
	unsigned char pad16;		/* 0c9 						*/
	unsigned char dma1_ch6_count;	/* 0ca (rw) - write DMA count bytes 0 & 1 here 	*/
	unsigned char pad17;		/* 0cb 						*/
	unsigned char dma1_ch7_base_addr; /* 0cc (rw) - write DMA addr bytes 0 & 1 here */
	unsigned char pad18;		/* 0cd 						*/
	unsigned char dma1_ch7_count;	/* 0ce (rw) - write DMA count bytes 0 & 1 here 	*/
	unsigned char pad19;		/* 0cf 						*/
	unsigned char dma1_status;	/* 0d0 (ro); (wo) = command 			*/
	unsigned char pad20;		/* 0d1 						*/
	unsigned char dma1_request;	/* 0d2 (wo) 					*/
	unsigned char pad21;		/* 0d3 						*/
	unsigned char dma1_single_mask; /* 0d4 (wo) 					*/
	unsigned char pad22;		/* 0d5 						*/
	unsigned char dma1_mode;	/* 0d6 (wo) - [ sw copy is kept ] 		*/
	unsigned char pad23;		/* 0d7 						*/
	unsigned char dma1_clr_byte_ptr;/* 0d8 (wo) 					*/
	unsigned char pad24;		/* 0d9 						*/
	unsigned char dma1_master_clr;	/* 0da (wo) 					*/
	unsigned char pad25;		/* 0db 						*/
	unsigned char dma1_clr_mask;	/* 0dc (wo) 					*/
	unsigned char pad26;		/* 0dd 						*/
	unsigned char dma1_mask_all;	/* 0de (rw) - write=en/disable; read=get status */ 
	unsigned char pad27[802];	/* 0df - 0400 					*/
	unsigned char dma0_ch0_high_count; /* 0401 (rw) - write DMA count byte 2 here 	*/
	unsigned char pad28;		/* 0402 - reserved byte 			*/
	unsigned char dma0_ch1_high_count; /* 0403 (rw) - write DMA count byte 2 here 	*/
	unsigned char pad29;		/* 0404 - reserved byte 			*/
	unsigned char dma0_ch2_high_count; /* 0405 (rw) - write DMA count byte 2 here 	*/
	unsigned char pad30;		/* 0406 - reserved byte 			*/
	unsigned char dma0_ch3_high_count; /* 0407 (rw) - write DMA count byte 2 here 	*/
	unsigned char pad31[2];		/* 0408 - 0409 - reserved bytes 		*/
	unsigned char dma_int_pending;	/* 040a (ro); (wo) = DMA(0-3) chaining mode 	*/
	unsigned char dma0_extended_mode; /* 040b (wo) - [ sw copy is kept ] 		*/
	unsigned char eisa_master;	/* 040c (ro) - ?? 				*/
	unsigned char pad32[84];	/* 040d - 0460 					*/
	unsigned char nmi_extended_status; /* 0461 (rw) 				*/
	unsigned char software_nmi;	/* 0462 (wo) 					*/
	unsigned char pad33;		/* 0463 					*/
	unsigned char last_eisa_bus_master_byte0; /* 0464 (ro) 				*/
	unsigned char last_eisa_bus_master_byte1; /* 0465 (ro) 				*/
	unsigned char pad34[27];	/* 0466 - 0480 					*/
	unsigned char dma0_ch2_high_page; /* 0481 (rw) - write DMA addr byte 3 here 	*/
	unsigned char dma0_ch3_high_page; /* 0482 (rw) - write DMA addr byte 3 here 	*/
	unsigned char dma0_ch1_high_page; /* 0483 (rw) - write DMA addr byte 3 here 	*/
	unsigned char pad35[3];		/* 0484 - 0486 - reserved bytes 		*/
	unsigned char dma0_ch0_high_page; /* 0487 (rw) - write DMA addr byte 3 here 	*/
	unsigned char pad36;		/* 0488 - reserved byte 			*/
	unsigned char dma1_ch6_high_page; /* 0489 (rw) - write DMA addr byte 3 here 	*/
	unsigned char dma1_ch7_high_page; /* 048a (rw) - write DMA addr byte 3 here 	*/
	unsigned char dma1_ch5_high_page; /* 048b (rw) - write DMA addr byte 3 here 	*/
	unsigned char pad37[58];	/* 048c - 04c5 					*/
	unsigned char dma1_ch5_high_count; /* 04c6 (rw) - write DMA count byte 2 here 	*/
	unsigned char pad38[3];		/* 04c7 - 04c9 					*/
	unsigned char dma1_ch6_high_count; /* 04ca (rw) - write DMA count byte 2 here 	*/
	unsigned char pad39[3];		/* 04cb - 04cd 					*/
	unsigned char dma1_ch7_high_count; /* 04ce (rw) - write DMA count byte 2 here 	*/
	unsigned char pad40;		/* 04cf 					*/
	unsigned char int1_edge_level;	/* 04d0 (rw) - int1 edge/level control 		*/
	unsigned char int2_edge_level;	/* 04d1 (rw) - int2 edge/level control 		*/
	unsigned char pad41[2];		/* 04d2 - 04d3 - reserved bytes 		*/
	unsigned char dma1_chaining_mode; /* 04d4 (wo); (ro) - dma chaining mode status */
	unsigned char pad42;		/* 04d5 - reserved byte 			*/
	unsigned char dma1_extended_mode; /* 04d6 (wo) - [ sw copy is kept ] 		*/
	unsigned char pad43[9];		/* 04d7 - 04df 					*/
	unsigned char dma0_ch0_stop0;	/* 04e0 					*/
	unsigned char dma0_ch0_stop1;	/* 04e1 					*/
	unsigned char dma0_ch0_stop2;	/* 04e2 					*/
	unsigned char pad44;		/* 04e3 - reserved byte 			*/
	unsigned char dma0_ch1_stop0;	/* 04e4 					*/
	unsigned char dma0_ch1_stop1;	/* 04e5 					*/
	unsigned char dma0_ch1_stop2;	/* 04e6 					*/
	unsigned char pad45;		/* 04e7 - reserved byte 			*/
	unsigned char dma0_ch2_stop0;	/* 04e8 					*/
	unsigned char dma0_ch2_stop1;	/* 04e9 					*/
	unsigned char dma0_ch2_stop2;	/* 04ea 					*/
	unsigned char pad46;		/* 04eb - reserved byte 			*/
	unsigned char dma0_ch3_stop0;	/* 04ec 					*/
	unsigned char dma0_ch3_stop1;	/* 04ed 					*/
	unsigned char dma0_ch3_stop2;	/* 04ee 					*/
	unsigned char pad47[5];		/* 04ef - 04f3 - reserved bytes 		*/
	unsigned char dma1_ch5_stop0;	/* 04f4 					*/
	unsigned char dma1_ch5_stop1;	/* 04f5 					*/
	unsigned char dma1_ch5_stop2;	/* 04f6 					*/
	unsigned char pad48;		/* 04f7 - reserved byte 			*/
	unsigned char dma1_ch6_stop0;	/* 04f8 					*/
	unsigned char dma1_ch6_stop1;	/* 04f9 					*/
	unsigned char dma1_ch6_stop2;	/* 04fa 					*/
	unsigned char pad49;		/* 04fb - reserved byte 			*/
	unsigned char dma1_ch7_stop0;	/* 04fc 					*/
	unsigned char dma1_ch7_stop1;	/* 04fd 					*/
	unsigned char dma1_ch7_stop2;	/* 04fe 					*/
	unsigned char pad50[1921];	/* 04ff - 0c7f 					*/
	unsigned char id_byte1;		/* 0c80 (ro) 					*/
	unsigned char id_byte2;		/* 0c81 (ro) 					*/
	unsigned char id_byte3;		/* 0c82 (ro) 					*/
	unsigned char id_byte4;		/* 0c83 (ro) 					*/
};

#define dma0_command dma0_status
#define dma1_command dma1_status
#define dma0_chaining_mode dma_int_pending 
#define dma_chaining_status dma1_chaining_mode 
#define int1_ocw2 int1_icw1
#define int1_ocw3 int1_icw1
#define int1_icw2 int1_int_mask_reg
#define int1_icw3 int1_int_mask_reg
#define int1_icw4 int1_int_mask_reg
#define int2_ocw2 int2_icw1
#define int2_ocw3 int2_icw1
#define int2_icw2 int2_int_mask_reg
#define int2_icw3 int2_int_mask_reg
#define int2_icw4 int2_int_mask_reg
#define int1_isr  int1_icw1
#define int1_irr  int1_icw1
#define int2_isr  int2_icw1
#define int2_irr  int2_icw1

/* 
 * byte swap and byte in/out macros 
 */

#define inb(addr)  (unsigned char)*(addr)

#define outb(addr, value) (*((unsigned char *)(addr)) = (value))

#define outs(addr, value)  (*((u_short *)(addr)) = (((value) >> 8) \
				+ (((value)&0xff) << 8)))

#define outl(addr, value)  (*((u_int *)(addr)) = (u_int)(((value)>>24) \
				+ (((value)&0xff0000)>>8) \
				+ (((value)&0xff00)<<8) + (((value)&0xff)<<24)))

#endif /* ! _SYS_EISA_INCLUDED */
