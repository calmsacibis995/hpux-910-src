/* @(#) $Revision: 1.18.83.5 $ */
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/hpibio.h,v $
 * $Revision: 1.18.83.5 $       $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/03 17:11:45 $
 */
#ifndef _SYS_HPIBIO_INCLUDED /* allows multiple inclusion */
#define _SYS_HPIBIO_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _WSIO
#  ifdef _KERNEL_BUILD
#    include "../h/types.h"
#    include "../machine/timeout.h"
#    include "../h/io.h"
#    include "../machine/cpu.h"
#  else /* ! _KERNEL_BUILD */
#    include <sys/types.h>
#    include <sys/timeout.h>
#    include <sys/io.h>
#    include <machine/cpu.h>
#  endif /* _KERNEL_BUILD */
#endif /* _WSIO */

#ifdef _WSIO
#  ifndef	TRUE
#    define TRUE	   1
#    define FALSE      0
#  endif
        
/* card address constants */

#  define IOSTART_ADDR	(0x400000 + LOG_IO_OFFSET) /* start of I/O space    */
#  define START_ADDR	(0x600000 + LOG_IO_OFFSET) /* start for other cards */
#  define INTERNAL_CARD	(0x478000 + LOG_IO_OFFSET) /* INTERNAL HPIB address */

/* card id constants */
        
#  define NO_ID		0
#  define INTERNAL_HPIB	1
#  define HP98624	2           /* hpib */
#  define HP98626	3           /* serial */
#  define HP98625	4           /* high-speed disk interface */
#  define HP98627A	5           /* moon unit */
#  define HP98628	6           /* hp98628 */
#  define HP98204A	7	     /* 9920  graphics frame buffer */
#  define HP98204B	8	     /* marbox graphics frame buffer */
#  define HP9826A	9	     /* 9826a graphics frame buffer */
#  define HP9836A	10	     /* 9836a graphics frame buffer */
#  define HP9836C	11	     /* 9836c graphics frame buffer */
#  define HP9837H	12	     /* GATOR graphics frame buffer */
#  define HP98700	13	     /* GATORBOX graphics frame buffer */
#  define HP98642	14           /* hp98642 */
#  define HP98643	15           /* hp98643 */
#  define HP98644	16           /* hp98644 */
#  define HP98622	17           /* hp98622 */
#  define BOBCAT	18           /* BOBCAT graphics frame buffer */
#  define SCUZZY	19           /* HP Series 300 SCSI Card */
#  define HP98629	20	     /* hp98629 SRM card */
#  define HP98577	21	     /* hp98577 Stealth card */
#  define HP98646	22	     /* hp98646 BCD VME card */
#  define HP98641	23	     /* hp98641 RJE card */
#  define HP98630D	24	     /* hp98630 double breadboard card */
#  define HP98630Q	25	     /* hp98630 quad breadboard card */

/* below are those cards that are not supported by HP-UX but we will
 * include because we need to ID them.
 */
#  define HP98253	26           /* hp98253 EPROM programmer card */
#  define HP98259	27	     /* hp98259 bubble memory card */
#  define HP98623	28	     /* hp98623 BCD interface card */
#  define HP98640	29	     /* hp98640 ADC card */
#  define HP98691	30	     /* hp98691 programmable datacomm card */
#  define HP98649	31	     /* hp98649 SNA (SDLC) interface card */
#  define HP98647	32	     /* hp98647 PC instruments interface */
#  define HP98695	33	     /* hp98695 IBM 3270 emulator */
#  define HP98633	34	     /* hp98633 multi-programmer card */

#  define HIL_CARD	35	     /* hp????? DIO HIL card */

/* hp986PP Centronics compatible 8 bit parallel bi-directional interface */
#define HP986PP         36

/* pseudo card ID for Apollo utility chip RS-232 UARTS */
#define APOLLOPCI	37
        
#  define HPE1480	38	     /* VIX Vixen Interface */
#  define DIGITAL_AUDIO 39           /* Digital Audio built in hardware */
#  define HP25560       41           /* EISA HPIB card */

/* Centronics will use this for HW dependent transfer termination */
#define BUSY_END_TFR BURST_TFR

#define CONTINUE_TRANSFER       0
#define TRANSFER_COMPLETED      1

/* transfer requests */
   enum transfer_request_type {MAX_OVERLAP, MAX_SPEED, MUST_FHS, MUST_INTR, MUST_DMA};

/* obtainable interrupts */
#  define INT_CTLR	0x01
#  define INT_SRQ	0x02	/* requested SRQ interrupt */
#  define INT_TADS	0x04	/* talk address interrupt */
#  define INT_LADS	0x08	/* listen address interrupt */
#  define INT_PPL	0x10	/* requested parallel poll interrupt */
#  define INT_READ	0x20
#  define INT_WRITE	0x40
#  define SIMON_FAKEISR	0x80

/* hp-ib addressing group bases */
#  define LAG_BASE   	0x20	 /*  listener address base */
#  define TAG_BASE   	0x40	 /*  talker address base */
#  define SCG_BASE   	0x60	 /*  secondary address base */

/* hp-ib command equates in odd parity */
#  define GTL         0x01	/*  go to local */
#  define SDC         0x04	/*  selective device clear */
#  define PPC         0x85	/*  ppoll configure */
#  define GET         0x08	/*  group execute trigger */
#  define TCT         0x89	/*  take control */
#  define LLO         0x91	/*  local lockout */
#  define DCL         0x94	/*  device clear */
#  define PPU         0x15	/*  ppoll unconfigure */
#  define SPE         0x98	/*  spoll enable */
#  define SPD         0x19	/*  spoll disable */
#  define UNL         0xbf	/*  unlisten */
#  define UNT         0xdf	/*  untalk */

#  define RSV_MASK    0x40	 /* SRQ is set by this guy (in spoll byte) */

/* return values for iod_io_check */
#  define TRANSFER_READY	0x00
#  define TRANSFER_ERROR	0x01
#  define TRANSFER_WAIT		0x02

/* defines for iod_dma_setup */
#  define FIRST_ENTRIES		0x00
#  define LAST_ENTRY		0x01

/* driver declarations */
   struct drv_table_type {
	int (*iod_init)();
	int (*iod_ident)();
	int (*iod_clear_wopp)();
	int (*iod_clear)();
	int (*iod_scratch3)();		
	int (*iod_scratch4)();		
	int (*iod_tfr)();
	int (*iod_pplset)();
	int (*iod_pplclr)();
	int (*iod_ppoll)();
	int (*iod_spoll)();
	int (*iod_preamb)();
	int (*iod_postamb)();
	int (*iod_mesg)();
	int (*iod_hst)();
	int (*iod_abort)();
	int (*iod_ren)();
	int (*iod_send_cmd)();
	int (*iod_dil_abort)();
	int (*iod_ctlr_status)();
	int (*iod_set_ppoll_resp)();
	int (*iod_srq)();
	int (*iod_save_state)();
	int (*iod_restore_state)();
	int (*iod_io_check)();
	int (*iod_dil_reset)();
	int (*iod_status)();
	int (*iod_pass_control)();
	int (*iod_abort_io)();
	int (*iod_scratch1)();	
	int (*iod_wait_set)();
	int (*iod_wait_clr)();
	int (*iod_intr_set)();
	struct dma_chain * (*iod_dma_setup)();
	int (*iod_set_addr)();
	int (*iod_atn_ctl)();
	int (*iod_reg_access)();
	int (*iod_preamb_intr)();
	int (*iod_scratch2)();
	int (*iod_savecore_xfer)();
   };

/* this is a copy of the tty_driver in tty.h */
   struct tty_drivercp {
	int	type;			/* Driver Number */
	int	(*open)();		/* open(dev, tp, flag) */
	int	(*close)();		/* close(dev, tp) */
	int	(*read)();		/* read(tp) */
	int	(*write)();		/* write(tp) */
	int	(*ioctl)();		/* ioctl(dev, tp, cmd, arg, mode) */
	int	(*select)();		/* select(tp) */
	int	(*kputchar)();		/* putchar(c); */
	int	(*wait)();		/* wait(tp); */
	int	(*pwr_init)();	/* pwr_init(&tp, addr, id, il, &cs,
						&pci_total); */
	int	(*who_init)();		/* who_init(tp, tty_number, sc)
						For console tty_number == -1 */
	struct tty_drivercp *next;	/* Pointer to next one in list */
   };

/* flags for state above */
#  define WAIT_BO		0x0001
#  define WAIT_BI		0x0002

#  define F_SIMONA		0x0001
#  define F_WORD_DMA		0x0002	/* flag to dma_build_chain */
#  define F_PRIORITY_DMA	0x0004	/* flag to dma_build_chain */
#  define F_SIMONB		0x0008	/* flag to dma_build_chain */
#  define LOCKED		0x0010
#  define F_SIMONC		0x0020
#  define ACTIVE_CONTROLLER	0x0040
#  define SYSTEM_CONTROLLER	0x0080
#  define IST			0x0100	/* for ppoll response */
#  define STATE_SAVED		0x0200
#  define BOBCAT_HPIB		0x0400
#  define IN_HOLDOFF		0x0800
#  define DO_BOBCAT_PPOLL	0x1000
#  define HIDDEN_MODE		0x2000
/*				0x4000 -- unused */
#  define F_LWORD_DMA		0x8000
#  define FHS_TIMED_OUT	       0x10000
#  define FHS_ACTIVE	       0x20000

#  define DIOII_START_ADDR	0x1000000
#  define DIOII_SC_SIZE		0x400000


/* message IO flags */
#  define T_WRITE	0x1
#  define T_EOI		0x2
#  define T_PPL		0x4

#  define TIMED_OUT	2	/* escape value */

#ifdef __hp9000s700
#  define START_POLL	{						\
			bp->b_queue->markstack = NULL;			\
			}

#else
#  define START_POLL	{						\
			bp->b_queue->markstack = (int (**)())get_sp();	\
			if (bp->b_queue->timeflag) {			\
				bp->b_queue->timeflag = FALSE;		\
				escape(TIMED_OUT);			\
			}						\
			}
#endif

#  define END_POLL	{						\
			bp->b_queue->markstack = NULL;			\
			}

/* temporary, till we get it right */
#  define SWAP_BASE 2000

/*
**  undocumented cs80 ioctl function used by ioscan to do an HPIB_identify
**  on hpib devices.
*/
#define IOSCAN_HPIB_FIXED_DISK	0
#define IOSCAN_HPIB_REMOV_DISK	1
#define IOSCAN_HPIB_TAPE	2

struct io_hpib_ident{
		  unsigned char dev_type;
		  short		ident;
};

#define	IO_HPIB_IDENT		_IOR('I', 0, struct io_hpib_ident)


#else /* _not _WSIO */

#ifdef _KERNEL_BUILD
#    include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/types.h>
#endif /* _KERNEL_BUILD */

/************************************************************************/
/*                           hpibio.h                                   */
/*                                                                      */
/*  This file should be included by any program that wants to use ioctl */
/*  calls for the HPIB device adapter or any device adapter using the   */
/*  HPIB Logical Device Manager (LDM).                                  */
/************************************************************************/

#  ifndef IO_CONTROL
/*--------------------- ioctl commands from HP-UX ----------------------*/

				/* macros from ioctl.h:			*/
#    define	O_IO_CONTROL		_IOW('I', 0, struct io_ctl_status)
#    define	IO_CONTROL		_IOWR('I', 0, struct io_ctl_status)
#    define	IO_STATUS		_IOWR('I', 1, struct io_ctl_status)
#    define	IO_ENVIRONMENT		_IOR('I',  2, struct io_environment)

/*------------------- ioctl structures used by HP-UX -------------------*/

     struct io_ctl_status {
       int type;			/* command from below   	*/
       int arg[3];			/* arguments to command		*/
     };

     struct io_environment {
       int interface_type;		/* as defined below		*/
       int timeout;			/*	   for IO_STATUS...	*/
       int status;
       int term_reason;
       int read_pattern;
       int signal_mask;
       int width;
       int speed;
       pid_t locking_pid;
       unsigned int config_mask;	/* unused			*/
       unsigned short delay;		/* unused			*/
       unsigned char reserved[22];	/* up to 64 bytes for expansion	*/
     };

					/* For HPIB_RESET:   arg[0] =	*/
#    define HW_CLR		1	/*    -- reset interface	*/

					/* For HPIB_LOCK:   arg[0] =	*/
#    define LOCK_INTERFACE	0	/*    -- increment p/open count	*/
#    define UNLOCK_INTERFACE	1	/*    -- decrement p/open count	*/
#    define CLEAR_ALL_LOCKS	2	/*    -- release all locks	*/
#    define UNLOCKED 		-1

#  endif /* IO_CONTROL */

/*----------------------- io_ctl_status "type"s ------------------------*/

					/* for IO_CONTROL:		*/
#  define HPIB_TIMEOUT		0	/* -- set timeout value (u-sec)	*/
					/*    arg[0] = timeout value	*/
#  define HPIB_WIDTH		1	/* -- set data path width	*/
					/*    arg[0] = width in bits    */
#  define HPIB_SPEED		2	/* -- set transfer speed	*/
					/*    arg[0] = k-bytes/second	*/
#  define HPIB_READ_PATTERN	3	/* -- terminate on pattern match*/
					/*    arg[0] = 0  (disable)     */
					/*    arg[0] = ~0 (enable)      */
					/*    arg[1] = pattern          */
#  define HPIB_SIGNAL_MASK	4	/* -- define signaling events	*/
					/*    arg[0] = interrupt mask   */
					/*    arg[0] = 0  (disable)     */
					/*    arg[1] = ??? for ST_PPOLL	*/
#  define ST_PPOLL	    0x00400000	/* Responded to Parallel Poll	*/
#  define ST_SRQ	    0x00004000	/* SRQ Asserted			*/
#  define ST_REN	    0x00002000	/* Put in Remote State		*/
#  define ST_ACTIVE_CTLR    0x00001000  /* Became Controller in Charge  */
#  define ST_TALK	    0x00000400	/* Was Addressed to Talk	*/
#  define ST_LISTEN	    0x00000200	/* Was Addressed to Listen	*/
#  define ST_DAT	    0x00000100	/* Received Data while Idle	*/
#  define ST_GET	    0x00000010	/* Received Group Execute Trgr	*/
#  define ST_DCL	    0x00000004	/* Received Device Clear	*/
#  define ST_IFC	    0x00000001	/* IFC Asserted			*/
#  define HPIB_LOCK		5	/* -- lock/unlock interface	*/
					/*    arg[0] =			*/
/*        LOCK_INTERFACE		/*    -- increment p/open count	*/
/*	  UNLOCK_INTERFACE		/*    -- decrement p/open count	*/
/*	  CLEAR_ALL_LOCKS		/*    -- release all locks	*/
#  define HPIB_RESET		6	/* -- reset 			*/
					/*    arg[0] =			*/
#  define BUS_CLR		0	/*    -- reset hardware		*/
/*	  HW_CLR			/*    -- reset interface	*/
#  define DEVICE_CLR		2	/*    -- reset device		*/
#  define HPIB_REN		7	/* -- set Remote Enable		*/
					/*    arg[0] = 0  (disable)     */
					/*    arg[0] = ~0 (enable)      */
#  define HPIB_EOI	        8	/* -- control EOI assertion	*/
					/*    arg[0] = 0  (disable)     */
					/*    arg[0] = ~0 (enable)      */
#  define HPIB_ADDRESS	       10	/* -- set interface bus address	*/
					/*    arg[0] = non-CIC address 	*/
#  define HPIB_TALK_ALWAYS	0x40
#  define HPIB_LISTEN_ALWAYS	0x20
#  define HPIB_PPOLL_RESP       11	/* -- control response to ppoll	*/
					/*    arg[0] = ???		*/
					/*    arg[1] = ???		*/
#  define HPIB_SRQ               12	/* -- assert Service Request	*/
#  define HPIB_PASS_CONTROL      13	/* -- release bus control	*/
					/*    arg[0] = addr to pass to	*/
#  define HPIB_GET_CONTROL       14	/* -- become active controller	*/
#  define HPIB_PPOLL_IST         21	/* -- define parallel poll	*/
					/*    arg[0] = response byte	*/
#  define HPIB_SYSTEM_CTLR       22	/* -- become system controller	*/
					/*    arg[0] = 0  (drop)	*/
					/*    arg[0] = ~0 (become)	*/

					/* for IO_STATUS:		*/
/*	HPIB_TIMEOUT			/* -- return timeout value 	*/
					/*    arg[0] = timeout value	*/
/*	HPIB_WIDTH		 	/* -- return data path width	*/
					/*    arg[0] = width in bits    */
/*      HPIB_SPEED		 	/* -- return transfer speed	*/
					/*    arg[0] = k-bytes/second	*/
/*      HPIB_READ_PATTERN	 	/* -- return termination pattern*/
					/*    arg[0] = pattern          */
					/*    arg[0] = -1 (disabled)    */
/*      HPIB_SIGNAL_MASK	 	/* -- return SIGEMT reason	*/
					/*    arg[0] = interrupt mask   */
/*      HPIB_LOCK		 	/* -- return locking process id	*/
					/*    arg[0] = process id       */
					/*    arg[0] = -1 (not locked)  */
/*	HPIB_ADDRESS	       		/* -- get bus address		*/
					/*    arg[0] = bus address 	*/
#  define HPIB_INTERFACE_TYPE	9	/* -- return interface type	*/
					/*    arg[0] = interface type   */
#  define HPIB_INTERFACE	0x20
#  define HPIB_DEVICE		0x40
#  define HPIB_BUS_STATUS       15	/* -- read bus status 		*/
					/*    arg[0] = mask             */
#  define  ST_TALK_ALWAYS	0x40000000 /* Is set up to talk always	*/
#  define  ST_LISTEN_ALWAYS	0x20000000 /* Is set up to listen always	*/
#  define  ST_NDAC	  	0x00008000 /* State of NDAC line		*/
/*	   ST_SRQ    			/* State of SRQ			*/
/*	   ST_REN    			/* State of REN			*/
/*	   ST_ACTIVE_CTLR 		/* Is Controller in Charge	*/
#  define  ST_SYSTEM_CTLR  0x00000800	/* Is System Controller		*/
/*	   ST_TALK	    		/* Is Addressed to Talk		*/
/*	   ST_LISTEN	    		/* Is Addressed to Listen	*/
/*	   ST_DAT    	    		/* Received Data while Idle	*/
#  define HPIB_TERM_REASON       16	/* -- explain last read's term	*/
					/*    arg[0] = mask		*/
#  define TR_ERROR     0x80		/* Read had unclassified error	*/
#  define TR_TIMEOUT   0x40		/* Read timed out		*/
#  define TR_END       0x20		/* Device asserted EOI		*/
#  define TR_MATCH     0x08		/* Read matched term character	*/
#  define TR_COUNT     0x04		/* Device sent requested #bytes	*/
#  define TR_NOTERM    0x00		/* No read done yet		*/
#  define HPIB_WAIT_ON_PPOLL     17	/* -- wait for ppoll response	*/
					/*    arg[0] = ???		*/
#  define HPIB_WAIT_ON_STATUS    18	/* -- wait for bus condition	*/
					/*    arg[0] = mask		*/
					/*	same as signals		*/
#  define HPIB_PPOLL             19	/* -- conduct parallel poll	*/
					/*    arg[0] = ppoll byte	*/
#  define HPIB_SPOLL             20	/* -- conduct serial poll	*/
					/*    arg[0] = spoll response	*/
					/*    arg[1] = bus address (in)	*/

/* HPIB-only ioctl							*/
#  define HPIB_COMMAND   _IOW('I', 3, struct hpib_cmd) 
#  define MAX_HPIB_COMMANDS 	100

   struct hpib_cmd {
        int length;                     /* number of bytes to send      */
        char buffer[MAX_HPIB_COMMANDS]; /* user's ATN buffer 		*/
   };
#endif /* __hp9000s800 */

#endif /* _SYS_HPIBIO_INCLUDED */
