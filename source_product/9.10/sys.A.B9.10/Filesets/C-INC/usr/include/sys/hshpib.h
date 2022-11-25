/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/hshpib.h,v $
 * $Revision: 1.3.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 16:42:30 $
 */
/* @(#) $Revision: 1.3.83.4 $ */    
#ifndef _SYS_MEDUSA_INCLUDED /* allows multiple inclusion */
#define _SYS_MEDUSA_INCLUDED

/* MEDUSA High speed HPIB interface card */
/* PHI/ABI/MEDUSA chip registers are 'M_', Simon/Python I/O card
   registers are 'S/P_' and EISA/DIO/ETC backplane registers are 'B_' */

#define MA		30

/* Define the medusa register masks for ints and status */
/* Medusa high order access bits via the status register in 8-bit mode */
#define M_HIGH_0        0x80
#define M_HIGH_1        0x40

/* Medusa interrupt register and mask */
#define M_INT_PEND      M_HIGH_0	/* interrupting condition usage */
#define M_INT_ENAB      M_HIGH_0	/* interrupt mask usage */
#define M_PRTY_ERR      M_HIGH_1
#define M_STAT_CHNG     0x80
#define M_PROC_ABRT     0x40
#define M_POLL_RESP     0x20
#define M_SERV_RQST     0x10
#define M_FIFO_ROOM     0x08
#define M_FIFO_BYTE     0x04
#define M_FIFO_IDLE     0x02
#define M_DEV_CLR       0x01

/* Medusa outbound fifo */
#define M_FIFO_EOI      M_HIGH_0
#define M_FIFO_ATN      M_HIGH_1
#define M_FIFO_LF_INH   M_HIGH_0
#define M_FIFO_UCT_XFR  (M_HIGH_0|M_HIGH_1)

/* Medusa status */
#define M_REM           0x20
#define M_HPIB_CTRL     0x10
#define M_SYST_CTRL     0x08
#define M_TLK_IDF       0x04
#define M_TLK           0x04
#define M_LTN           0x02
#define M_DATA_FRZ      0x01

/* Medusa control */
#define M_POLL_HDLF     M_HIGH_0	/* ABI only */
#define M_DHSK_DLY      M_HIGH_1	/* ABI only */
#define M_8BIT_PROC     0x80
#define M_PRTY_FRZ      0x40
#define M_REN           0x20
#define M_IFC           0x10
#define M_RSPD_POLL     0x08
#define M_RQST_SRVC     0x04
#define M_FIFO_SEL      0x02
#define M_INIT_FIFO     0x01

/* Medusa HP-IB control */
#define M_CRCE          M_HIGH_0	/* ABI only */
#define M_EPAR          M_HIGH_1	/* ABI only */
#define M_ONL           0x80
#define M_TA            0x40
#define M_LA            0x20
#define M_HPIB_ADR_MASK 0x1f

/* simon control bits */
#define S_ENAB		0x80
#define S_PEND		0x40
#define S_LEVMSK	0x30
#define S_LEVSHF	4
#define S_WRIT		0x08
#define S_WORD		0x04
#define S_DMA_1		0x02
#define S_DMA_0		0x01

/* simple simon control 2 bits */
#define S2_DISABLE_DONE	0x01

/* simple simon status bit */
#define S_HALFWORD	0x01

/* Register template for the Simon (98625A/B) DIO High speed HPIB
   interface card. */

volatile struct simon {
	volatile unsigned char
		sf0,	sim_reset,
		sf2,	sim_stat,
		sf4,	sim_ctrl2,
		sf6,	sim_latch,
		sf8,	sf9,
		sf10,	sf11,
		sf12,	sf13,
		sf14,	sf15,
		sf16,	med_intr,
		sf18,	med_imsk,
		sf20,	med_fifo,
		sf22,	med_status,
		sf24,	med_ctrl,
		sf26,	med_address,
		sf28,	med_ppmsk,
		sf30,	med_ppsns;
};

#define sim_id sim_reset
#define sim_ctrl sim_stat

#define med_fstid med_ppmsk
#define med_secid med_ppsns

/* Register template for the Python (25560A) EISA High speed HPIB
   interface card. */
/* python I/O card control bits and masks */

/* python card structure */

#define NUM_IO_REGS	0x5
#define NUM_MED_REGS	0x8
#define IO_REG_BASE	0x0000
#define MED_REG_BASE	0x0400
#define EISA_REG_BASE	0x0c80

volatile struct python {
	volatile unsigned char
		pyt_cardcfg,		/* 0xs000h */
		pyt_romcfg,		/* 0xs001h */
		pyt_dmacfg,		/* 0xs002h */
		pyt_intrcfg,		/* 0xs003h */
		pyt_hpibstat,		/* 0xs004h */
		dummy1[MED_REG_BASE-NUM_IO_REGS],
		med_intr,		/* 0xs400h */
		med_imsk,		/* 0xs401h */
		med_fifo,		/* 0xs402h */
		med_status,		/* 0xs403h */
		med_ctrl,		/* 0xs404h */
		med_address,		/* 0xs405h */
		med_ppmsk,		/* 0xs406h */
		med_ppsns,		/* 0xs407h */
		dummy2[EISA_REG_BASE-NUM_IO_REGS-NUM_MED_REGS-(MED_REG_BASE-NUM_IO_REGS)],
		eisa_prodid0,		/* 0xsc80h */
		eisa_prodid1,		/* 0xsc81h */
		eisa_prodid2,		/* 0xsc82h */
		eisa_prodid3,		/* 0xsc83h */
		eisa_ctrl		/* 0xsc84h */
};



#define PYTHON_ID       0x22f00c70      /* the real HP-IB card ID */
#define HSHPIB_MAX_CARDS 31     /* max number of cards to be connected */
#define PYTHON_INT_LVL  5

/* bits for IO_REG register */

/* IO_CARD_CONFIG */
#define P_SYS_CNTRL     0x1
#define P_SPD_SEL       0x2
#define P_ENABLE_FIFO   0x4
#define P_DATAIN_FIFO   0x8

/* IO_ROM_CONFIG */
#define P_A14_MASK      0x1
#define P_A15_MASK      0x2
#define P_ROM_ADDR      0x7C
#define P_ROM_ENB       0x80

/* IO_DMA_CONFIG */
#define P_IRQ_MASK      0x7
#define P_CH_SEL        0x7
#define P_DMA_ENB       0x8
#define P_DMA_DIR       0x10
#define P_AUTO_ENB      0x20
#define P_FULL_WORD     0x40
#define P_BYTE_DMA      0x80

/* IO_INTR_CONFIG */
#define P_LEV_SEL       0x7
#define P_INTR_ENB      0x8
#define P_IFC_INTR      0x10
#define P_GET_INTR      0x20
#define P_DMA_INTR      0x40
#define P_MED_INTR      0x80

/* IO_HPIB_STATUS */
#define P_NRFD          0x1
#define P_NDAC          0x2
#define P_DAV           0x4
#define P_IFC           0x8
#define P_ATN           0x10
#define P_SRQ           0x20
#define P_REN           0x40
#define P_EOI           0x80

/* bits for EISA_CONTROL register */
#define E_ENABLE        0x1
#define E_IOCHKERR      0x2
#define E_IOCHKRST      0x4

/* FAKE_ISR flag for sc->int_flags */
#define HSHPIB_FAKEISR  0x80

struct python_data {
        unsigned char
                pyt_cardcfg,    /* card register 0xZ000 */
                pyt_romcfg,    	/* card register 0xZ001 */
                pyt_dmacfg,     /* card register 0xZ002 */
                pyt_intrcfg,    /* card register 0xZ003 */
                med_imsk,       /* card register 0xZ401 */
                med_ctrl,       /* card register 0xZ404 */
                med_address,    /* card register 0xZ405 */
                med_ppsns,      /* card register 0xZ406 */
                med_ppmsk,      /* card register 0xZ407 */
                eisa_ctrl       /* card register 0xZc84 */
};

#ifdef _KERNEL
#ifdef EISA_TEST
/********************************************************************
 ************   register structure for EISA TEST CARD   *************
 ********************************************************************/
volatile struct eisa_test_card {
                unsigned char   dummy1[0x80];
                unsigned int    eisa_id;                /* $x80 */
                unsigned char   eisa_cntl;              /* $x84 */
                unsigned char   dummy2[3];
                unsigned char   iosel;                  /* $x88 */
                unsigned char   smemsel;                /* $x89 */
                unsigned char   imemsel;                /* $x8a */
                unsigned char   ememsel;                /* $x8b */
                unsigned char   isawsm;                 /* $x8c */
                unsigned char   eisawsm;                /* $x8d */
                unsigned char   dmablen;                /* $x8e */
                unsigned char   dmactrl;                /* $x8f */
                unsigned char   dmaenbl;                /* $x90 */
                unsigned char   dmadone;                /* $x91 */
                unsigned char   dmainv;                 /* $x92 */
                unsigned char   dmadummy;               /* $x93 */
                unsigned short  lirqen;                 /* $x94 */
                unsigned short  eirqen;                 /* $x96 */
                unsigned int    ibmaddr;                /* $x98 */
                unsigned short  dmalen;                 /* $x9c */
                unsigned char   half;                   /* $x9e */
                unsigned char   tc;                     /* $x9f */
                unsigned int    eisa_addr;              /* $xa0 */
                unsigned int    isa_addr;               /* $xa4 */
                unsigned int    data_match;             /* $xa8 */
                unsigned int    match_cycle;            /* $xac */
                unsigned int    ctrl_match;             /* $xb0 */
                unsigned int    trace_ptn;              /* $xb4 */
};
static struct eisa_test_card *eisa_test_cd;
caddr_t v_eisa_mem_ptr;
caddr_t p_eisa_mem_ptr;

struct bmic_type {
        u_char local_data;
        u_char pad0;
        u_char local_index;
        u_char pad1;
        u_char local_status;
};
static struct bmic_type *v_bmic;
static struct python python_dummy_reg;
#endif /* EISA_TEST */
#endif /* _KERNEL */

typedef union {
	struct simon s; 
	struct python p; 
	unsigned char r[sizeof(struct python)]; 
} HSHPIB;
#endif /* _SYS_MEDUSA_INCLUDED */
