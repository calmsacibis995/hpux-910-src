/*
 * @(#)etest.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 20:28:56 $
 * $Locker:  $
 */

#ifndef _SYS_ETEST_INCLUDED /* allows multiple inclusion */
#define _SYS_ETEST_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../wsio/eisa.h"
#include "../wsio/eeprom.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include "/etc/conf/machine/eisa.h"
#include "/etc/conf/machine/eeprom.h"
#endif /* _KERNEL_BUILD */

#define ETEST_ID		0x22f02050
#define ETEST_ID_OLD		0x22f00050

struct etest_reg {
	u_char pad1[0x80];	 /* 128 bytes of pad to ID reg */
	u_int  etest_id;	 /* 0x80-0x83 -- EISA ID of test card */
	u_char eisa_control;	 /* 0x84 - EISA defined control bits */
	u_char pad2[3];		 /* 0x85-0x87 */
	u_char io_select;	 /* 0x88 - address of BMIC registers - 32-127 */
	u_char isa20_select;	 /* 0x89 - address of RAM in ISA 20-bit space - 0-31 */
	u_char isa24_select;	 /* 0x8a - address of RAM in ISA 24-bit space - 16-127 */ 
	u_char eisa_select;	 /* 0x8b - address of RAM in EISA space - 1-127 */ 
	u_char isa_waitstates;	 /* 0x8c - wait states generated on ISA cycles */ 
	u_char eisa_waitstates;	 /* 0x8d - wait states generated on EISA cycles */ 
	u_char dma_burst_length; /* 0x8e - # of tfrs before card will release bus */ 
	u_char dma_control; 	 /* 0x8f - misc dma control bits */ 
	u_char dma_ch_enable; 	 /* 0x90 - assert DRQ for a channel */ 
	u_char dma_done; 	 /* 0x91 - write: clear DMA interrupt, read: ch is interrupting */ 
	u_char more_dma_control; /* 0x92 - more misc dma control bits */
	u_char pad3;		 /* 0x93 */
	u_short level_irqs;	 /* 0x94-0x95 - these IRQs are level triggered */
	u_short edge_irqs;	 /* 0x96-0x97 - these IRQs are edge triggered */
	u_short isa_dma_addr;	 /* 0x98-0x99 - lower two bytes of ISA master DMA destination */
	u_char isa_dma_high_byte;/* 0x9a - third byte of ISA master DMA destination */
	u_char isa_dma_cycles;	 /* 0x9b - for ISA master, do memory or I/O cycles */
	u_short	dma_count;	 /* 0x9c-0x9d - ISA master DMA count */
	u_char isa_dma_odd;	 /* 0x9e - ISA master DMA, last transfer is 8 bit */
	u_char dma_tc;		 /* 0x9f - DMA, behavior of T-C */
	u_int eisa_addr_match;	 /* 0xa0-0xa3 */
	u_int isa_addr_match;	 /* 0xa4-0xa7 */
	u_int eisa_data_match;	 /* 0xa8-0xab */
	u_char eisa_bclk_match;	 /* 0xac */
	u_char isa_bclk_match;	 /* 0xad */
	u_char pad4;		 /* 0xae-0xaf */
	u_int misc_match;	 /* 0xb0-0xb3 */
	u_short trace_cycles;	 /* 0xb4-0xb5 */
	u_char assert_iochk; 	 /* 0xb6 */
	u_char set_led;		 /* 0xb7 */
}; 

/* use card_ptr field (type int *) in isc to store my dma_parms data */
#define my_dma_parms card_ptr

/* eisa_control bits */
#define ETEST_ENABLE		0x1
#define ETEST_IOCHKERR		0x2
#define ETEST_IOCHKRST		0x4

/* io_select bits, 32 == 0x100-0x107 */
#define E_IOSEL_ENABLE		0x80
#define E_ISA_IO_CHUNK		0x8	/* divided into 8 byte chunks, set reg to addr/chunk_size */
#define E_ISA_IO_MIN		32
#define E_ISA_IO_MAX		127

/* isa20_select bits, 0 = 0-32K, valid values on our EISA are 16-31  */
#define E_SMEMSEL_ENABLE 	0x80
#define E_ISA20_CHUNK		0x8000	/* divided into 32K chunks, set reg to addr/chunk_size */
#define E_ISA20_MIN		16
#define E_ISA20_MAX		31

/* isa24_select bits, 16 = 1M - 1M+32K, valid values on our EISA are 80-127 */
#define E_IMEMSEL_ENABLE	0x80
#define E_ISA24_CHUNK		0x10000	/* divided into 64K chunks, set reg to addr/chunk_size */
#define E_ISA24_MIN		80
#define E_ISA24_MAX		127

/* eisa_select bits, 1 = 16M - 32M, valid values on our EISA are 1-15 */
#define E_EMEMSEL_ENABLE	0x80
#define E_EISA_CHUNK		0x1000000	/* divided into 16M chunks */
#define E_EISA_MIN		1
#define E_EISA_MAX		15

/* isa_waitstates bits */
#define E_NUM_EWAIT_MASK	0x37
#define E_NOWAIT		0x40  /* 1=run no wait states, 0 = use bits 0-5 for # wait states */
#define E_ISA_16BIT		0x80  /* 1 = ISA transfers are 16bit, 0 = ISA transfers are 8bit */

/* eisa_waitstates bits */
#define E_NUM_IWAIT_MASK	0x17
#define E_EISA_32BIT		0x40  /* 1 = EISA tfrs are 32bit, 0 = EISA tfrs are 16bit */
#define E_EISA_BURST		0x80  /* 1=card supports EISA Burst, 0=card doesn't support Burst */

/* dma_control bits */
#define E_DMA_SIZE_MASK		0x3	/* 2 bits of tfr size, must match DMA controller setting */
#define E_DMA_SIZE_BYTE		0x0	/* DMA_SIZE_MASK = 0 = byte tfrs */
#define E_DMA_SIZE_WORD		0x1	/* DMA_SIZE_MASK = 1 = short tfrs */
#define E_DMA_SIZE_LONG		0x2	/* DMA_SIZE_MASK = 2 = long tfrs */
#define E_DREQ_ON 		0x4	/* 1 = DREQ always asserted, 0 = DREQ asserted until DAK */ 
#define E_DMA_BURST 		0x8	/* 1=BURST cycles, 0=ISA, A, or B, must match controller */
#define E_DMA_REFRESH 		0x10	/* 1 = do refresh during ISA master DMA, 0 = don't */
#define E_DMA_WRITE 		0x20	/* 1 = DMA write into card RAM, 0 = read from card RAM */

/* dma_ch_enable bits */
#define E_ENABLE_CH0		0x1
#define E_ENABLE_CH1		0x2
#define E_ENABLE_CH2		0x4
#define E_ENABLE_CH3		0x8
#define E_ENABLE_CH5		0x20
#define E_ENABLE_CH6		0x40
#define E_ENABLE_CH7		0x80

/* dma_done bits */
#define E_CH0_INTERRUPT		0x1
#define E_CH1_INTERRUPT		0x2
#define E_CH2_INTERRUPT		0x4
#define E_CH3_INTERRUPT		0x8
#define E_FORCE_INTERRUPT 	0x10	/* causes an immediate interrupt */
#define E_CH5_INTERRUPT		0x20
#define E_CH6_INTERRUPT		0x40
#define E_CH7_INTERRUPT		0x80

/* more_dma_control bits */
#define E_DRQ_0-3		0x1	/* 1 = DRQ active low, 0 = DRQ active high, ch 0-3 */
#define E_DRQ_5-7		0x2	/* 1 = DRQ active low, 0 = DRQ active high, ch 5-7 */
#define E_DAK_0-3		0x4	/* 1 = DAK active high, 0 = DAK active low, ch 0-3 */
#define E_DAK_5-7		0x8	/* 1 = DAK active high, 0 = DAK active low, ch 5-7 */
#define E_DO_ISA_MASTER		0x80	/* 1 = act as ISA master, 0 = don't */

/* IRQ's for edge and level registers */
#define ETEST_IRQ3	0x8
#define ETEST_IRQ4	0x10
#define ETEST_IRQ5	0x20
#define ETEST_IRQ6	0x40
#define ETEST_IRQ7	0x80
#define ETEST_IRQ9	0x200
#define ETEST_IRQ10	0x400
#define ETEST_IRQ11	0x800
#define ETEST_IRQ12	0x1000
#define ETEST_IRQ14	0x4000
#define ETEST_IRQ15	0x8000

/* isa_dma_cycles bits */
#define E_ISA_DMA_MEMORY	0x1	/* 1 = run ISA memory cycles, 0 = run ISA I/O cycles */

/* isa_dma_odd bits */
#define E_ISA_DMA_HALF 		0x1	/* 1 = force final tfr to be 8 bits */

/* isa_dma_tc bits */
#define E_ISA_DMA_TC_OUT 	0x1	/* 1 = assert T-C, 0 = T-C is input from controller */

/* assert_iochk bits */
#define E_ASSERT_IOCHK		0x1


/*
 * structure and defines for bmic register portion of card 
 */

struct bmic_reg {
	u_char local_data;
	u_char pad0;
	u_char local_index;
	u_char pad1;
	u_char local_status;
};

#define local_control local_status

/* 
 * bits for bmic_reg registers 
 */

/* local_index */
#define E_AUTO_INCR		0x80	/* autoincrement local index reg, 0 = don't */

/* local_status */
#define E_CH0_BASE_BUSY		0x1	/* ch0 base register set is being used, 0 = free */
#define E_CH1_BASE_BUSY		0x2	/* ch1 base register set is being used, 0 = free */
#define E_POKE_PENDING		0x4	/* unused on our system */
#define E_LOCAL_INT		0x8	/* local interrupt pending, unused on our system */
#define E_LOCAL_INT_ENABLE 	0x10	/* 1 = local ints enabled, 0 = unabled, 0 for our use */
#define E_CH0_INT		0x20	/* ch0 is causing interrupt */
#define E_CH1_INT		0x40	/* ch1 is causing interrupt */
#define E_DOORBELL_INT		0x80	/* interrupt pending in doorbell reg,unused on our system */

/* offsets for index register for other registers of bmic */
#define global_config		0x8	/* global configuration register */
#define ch0_count_0		0x40	/* low byte of DMA count - ch0 */
#define ch0_count_1		0x41	/* 2nd byte of DMA count - ch0 */
#define ch0_count_2		0x42	/* 3rd byte of DMA count - ch0 */
#define ch0_base_addr_0		0x43	/* low byte of DMA address - ch0 */
#define ch0_base_addr_1		0x44	/* 2nd byte of DMA address - ch0 */
#define ch0_base_addr_2		0x45	/* 3rd byte of DMA address - ch0 */
#define ch0_base_addr_3		0x46	/* 4th byte of DMA address - ch0 */
#define ch0_config		0x48	/* transfer configuratoin for DMA */
#define ch0_strobe		0x49	/* writing this reg updates current regs and starts DMA */
#define ch0_status		0x4a	/* status of dma ch0 */
#define ch0_tbi_addr_0		0x4b	/* tbi addr holds offset into cards RAM for transfer- ch0 */
#define ch0_tbi_addr_1		0x4c	/* tbi addr holds offset into cards RAM for transfer- ch0 */
#define ch0_curr_count_0	0x50	/* low byte of current DMA count - ch 0 */
#define ch0_curr_count_1	0x51	/* 2nd byte of current DMA count - ch 0 */
#define ch0_curr_count_2	0x52	/* 3rd byte of current DMA count - ch 0 */
#define ch0_curr_addr_0		0x53	/* low byte of current DMA address - ch0 */
#define ch0_curr_addr_1		0x54	/* 2nd byte of current DMA address - ch0 */
#define ch0_curr_addr_2		0x55	/* 3rd byte of current DMA address - ch0 */
#define ch0_curr_addr_3		0x56	/* 4th byte of current DMA address - ch0 */
#define ch0_tbi_curr_addr_0	0x58	/* current tbi addr - ch0 */
#define ch0_tbi_curr_addr_1	0x59	/* current tbi addr - ch0 */
#define ch1_count_0		0x60	/* low byte of DMA count - ch1 */
#define ch1_count_1		0x61	/* 2nd byte of DMA count - ch1 */
#define ch1_count_2		0x62	/* 3rd byte of DMA count - ch1 */
#define ch1_base_addr_0		0x63	/* low byte of DMA address - ch1 */
#define ch1_base_addr_1		0x64	/* 2nd byte of DMA address - ch1 */
#define ch1_base_addr_2		0x65	/* 3rd byte of DMA address - ch1 */
#define ch1_base_addr_3		0x66	/* 4th byte of DMA address - ch1 */
#define ch1_config		0x68	/* transfer configuratoin for DMA */
#define ch1_strobe		0x69	/* writing this reg updates current regs & starts DMA */
#define ch1_status		0x6a	/* status of dma ch1 */
#define ch1_tbi_addr_0		0x6b	/* tbi addr holds offset into cards RAM for transfer- ch1 */
#define ch1_tbi_addr_1		0x6c	/* tbi addr holds offset into cards RAM for transfer- ch1 */
#define ch1_curr_count_0	0x70	/* low byte of current DMA count - ch 1 */
#define ch1_curr_count_1	0x71	/* 2nd byte of current DMA count - ch 1 */
#define ch1_curr_count_2	0x72	/* 3rd byte of current DMA count - ch 1 */
#define ch1_curr_addr_0		0x73	/* low byte of current DMA address - ch1 */
#define ch1_curr_addr_1		0x74	/* 2nd byte of current DMA address - ch1 */
#define ch1_curr_addr_2		0x75	/* 3rd byte of current DMA address - ch1 */
#define ch1_curr_addr_3		0x76	/* 4th byte of current DMA address - ch1 */
#define ch1_tbi_curr_addr_0	0x78	/* current tbi addr - ch1 */
#define ch1_tbi_curr_addr_1	0x79	/* current tbi addr - ch1 */


/* 
 * defines for local_data bits for each bmic register 
 */

/* global_config */
#define E_LEVEL_TRIG		0x8
#define E_PREEMPT_TIMER		0x3

/* ch0_count_2, ch1_count_2 */
#define E_BMIC_READ		0x40	/* 1 = DMA read, 0 = DMA write */
#define E_START_DMA		0x80	/* 1 = start DMA on current reg update, 0 = wait */

/* ch0_config, ch1_config */
#define E_SUSPEND_TFR		0x1	/* 1 = temporarily suspend DMA, 0 = proceed */
#define E_NO_LOAD_CURR		0x2	/* 1 = don't load current regs from base, 0 = do */
#define E_CLEAR_CH		0x4	/* stop tfr and flush fifo on ch */
#define E_BMIC_BURST		0x8	/* 1 = enable EISA burst, 0 = disable burst */
#define E_RELEASE_CH		0x10	/* 1 = release ch on request for it, 0 = hold through tfr */
#define E_ENABLE_TC_INT		0x20	/* 1 = interrupt on TC, 0 = disable interrupt */
#define E_ENABLE_EOP_INT	0x40	/* 1 = interrupt on external EOP, 0 = disable interrupt */
#define E_TBI_ENABLE		0x80	/* 1 = enable TBI address update, 0 = disable */

/* ch0_status, ch1_status */
#define E_TFR_COMPLETE		0x1	/* 1 = tfr complete on this ch, 0 = not */
#define E_EOP_TERM		0x2	/* 1 = tfr terminated by external EOP, 0= no EOP detected */
#define E_TFR_GOING		0x4	/* 1 = tfr in progress, 0 = no tfr in progress */
#define E_TFR_ENABLED		0x8	/* 1 = tfr is enabled, 0 = tfr not enabled */
#define E_FIFO_STALLED		0x10	/* 1 = fifo is stalled, 0 = fifo is active */


/*-----------------------------------------------------------------------------*/
/* structure containing eeprom information read from function data             */
/*-----------------------------------------------------------------------------*/
struct etest_func_data {
	int lvl_ints;           /* this cards level triggered interrupts */
	int edge_ints;          /* this cards edge triggered interrupts */
	char dma_chans;         /* this cards dma channels */
	int  mem_loc;           /* this cards EISA memory pointer */
	int  port_loc;          /* this cards bmic pointer */
};

/* my if_drv_data info in isc */
struct etest_card_info {
	int flags;
	caddr_t phys_bmic_ptr;
	caddr_t	phys_isa20_ptr;
	caddr_t phys_isa24_ptr;
	caddr_t phys_eisa_ptr;
	caddr_t virt_bmic_ptr;
	caddr_t	virt_isa20_ptr;
	caddr_t virt_isa24_ptr;
	caddr_t virt_eisa_ptr;
	int card_initialized;                 /* exclusive init checking */
	int etest_irq;	                      /* which irq line I'm using */
	int card_opened;                      /* enforce exclusive open */
	struct etest_func_data* etd;          /* store this cards eeprom info */
	int  max_block_xfer_len;              /* store max block xfer length */

	/* test specific stuff */
	int force_test_card_count;            /* used for resid calculation testing */
	struct dma_parms* dp;
};

#define IOMAP_ENTRIES 1
#define DMA_CHANNELS  2
#define RESRC_ERROR   5

#define AQUIRE        1
#define RELEASE       2
#define STATUS        3
#define ACT_ERROR     6

struct resource_manip {
	int resource_type;     /* hold the resource type */
	int action;            /* do this with the resource */
	int num;               /* how many or which */
	int *storage;          /* a generic hold area */
};

/* bits for flags field */
#define E_ISA_MASTER	0x1
#define E_EISA_MASTER	0x2
#define E_EISA_SLAVE	0x4

/* 
 * ioctl's for eisa test card
 */

#define E_SET_BMIC_ADDR		_IOW('v', 1, long)  /* valid BMIC register address (hex)*/
#define E_SET_ISA20_RAM 	_IOW('v', 2, long)  /* valid ISA 20bit RAM address (hex)*/
#define E_SET_ISA24_RAM		_IOW('v', 3, long)  /* valid ISA 24bit RAM address (hex)*/
#define E_SET_EISA_RAM		_IOW('v', 4, long)  /* valid EISA RAM address (in hex) */
#define E_SET_FLAGS		_IOW('v', 5, int)   /* ISA master, EISA */
#define E_SET_DATA_SIZE		_IOW('v', 6, int)   /* BYTE_DMA, WORD_DMA, LONG_DMA */
#define E_SET_DMA_TYPE		_IOW('v', 7, int)   /* ISA, TYPE A, TYPE B, BURST */
#define E_SET_DMA_MODE		_IOW('v', 8, int)   /* demand, block, single mode */
#define E_INTERRUPT		_IO('v', 9)         /* No parameters, generate interrupt */
#define E_SET_IRQ		_IOW('v', 10, int)  /* 3-7, 9-12, 14, 15 */
#define E_DMA_CHANNEL		_IOW('v', 11, int)  /* 0-3, 5-7	 */
#define E_ASSERT_IOCHECK 	_IO('v', 12)        /* No parameters, generate IOCHK NMI */
#define E_RESET         	_IO('v', 13)        /* No parameters, RESET card */
#define E_FORCE_XFER_COUNT 	_IOW('v', 14, int)  /* used for resid calcs */ 
#define E_SET_BUF        	_IOW('v', 15, int)  /* used by memory tests */
#define E_CMP_BUF        	_IOWR('v', 16, int) /* used by memory tests */
#define E_SET_BLK_XFER_LEN      _IOW('v', 25, int)  /* set max block xfer length */
#define E_CHECK_EISA_ADDR       _IOWR('v', 26, int) /* run address tests */

#ifdef EISA_TESTS
struct chain_stat { char stat[50]; };
#define READ_CHAIN_STAT  	_IOR('v', 17, struct chain_stat)  /* Read chaining test coverage */
#endif
#define E_GRAB_RESOURCE  	_IOWR('v', 18, struct resource_manip)  
#define E_NRML_TST_ROUTINES 	_IOR('v', 19, int)  /* performs generic repeat tests, and reports */
#define E_BAD_TST_ROUTINES 	_IO('v', 20)        /* performs generic one time tests,& reports */

/*
 * probably should write a separate test driver for these
 * but time do has its limitations
 */
struct mov_pair { caddr_t addr; unsigned int size; };
#define E_UIOMOVE_IN            _IOWR('v',21, struct mov_pair)
#define E_UIOMOVE_OUT           _IOWR('v',22, struct mov_pair)
#define E_COPYIN                _IOWR('v',23, struct mov_pair)
#define E_COPYOUT               _IOWR('v',24, struct mov_pair)




#endif /* _SYS_ETEST_INCLUDED */
