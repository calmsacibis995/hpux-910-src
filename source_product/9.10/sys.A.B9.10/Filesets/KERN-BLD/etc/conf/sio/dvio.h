/* $Header: dvio.h,v 1.4.83.8 94/07/05 12:22:05 drew Exp $ */

#ifndef _MACHINE_DVIO_INCLUDED
#define _MACHINE_DVIO_INCLUDED

/* dvio.h - Definitions for the Device I/O Library */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

/* interrupt definitions */

struct interrupt_struct {
	int cause;
	int mask;
};

/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
    extern int gpio_get_status(int);
    extern int gpio_set_ctl(int, int);
    extern int hpib_abort(int);
    extern int hpib_address_ctl(int, int);
    extern int hpib_atn_ctl(int, int);
    extern int hpib_bus_status(int, int);
    extern int hpib_card_ppoll_resp(int, int);
    extern int hpib_eoi_ctl(int, int);
    extern int hpib_parity_ctl(int, int);
    extern int hpib_pass_ctl(int, int);
    extern int hpib_ppol(int);
    extern int hpib_ppoll_resp_ctl(int, int);
    extern int hpib_ren_ctl(int, int);
    extern int hpib_rqst_srvce(int, int);
    extern int hpib_spoll(int, int);
    extern int hpib_status_wait(int, int);
    extern int hpib_wait_on_ppoll(int, int, int);
    extern int hpib_send_cmnd(int, char *, int);
    extern int io_burst(int, int);
    extern int io_eol_ctl(int, int, int);
    extern int io_get_term_reason(int);
    extern int io_interrupt_ctl(int, int);
    extern int io_lock(int);
    extern int io_unlock(int);
    extern int io_reset(int);
    extern int io_speed_ctl(int, int);
    extern int io_timeout_ctl(int, long);
    extern int io_width_ctl(int, int);
    extern int io_dma_ctl(int, int);
    extern int (*io_on_interrupt(int, struct interrupt_struct *, 
				 void (*)(int, struct interrupt_struct *)))
				 (int, struct interrupt_struct *);
#else /* not _PROTOTYPES */
    extern int gpio_get_status();
    extern int gpio_set_ctl();
    extern int hpib_abort();
    extern int hpib_address_ctl();
    extern int hpib_atn_ctl();
    extern int hpib_bus_status();
    extern int hpib_card_ppoll_resp();
    extern int hpib_eoi_ctl();
    extern int hpib_parity_ctl();
    extern int hpib_pass_ctl();
    extern int hpib_ppol();
    extern int hpib_ppoll_resp_ctl();
    extern int hpib_ren_ctl();
    extern int hpib_rqst_srvce();
    extern int hpib_spoll();
    extern int hpib_status_wait();
    extern int hpib_wait_on_ppoll();
    extern int hpib_send_cmnd();
    extern int io_burst();
    extern int io_eol_ctl();
    extern int io_get_term_reason();
    extern int io_interrupt_ctl();
    extern int io_lock();
    extern int io_unlock();
    extern int io_reset();
    extern int io_speed_ctl();
    extern int io_timeout_ctl();
    extern int io_width_ctl();
    extern int io_dma_ctl();
    extern int (*io_on_interrupt())();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


struct iodetail {
	char mode;
	char terminator;
	int count;
	char *buf;
};

#define	HPIBWRITE	0x00	/* write transfer request */
#define	HPIBREAD	0x01	/* read transfer request */
#define	HPIBATN		0x02	/* write bytes out with ATN set */
#define	HPIBEOI		0x04	/* write last byte out with EOI set */
#define	HPIBCHAR	0x08	/* terminate read with 'terminator' pattern */


/* definitions for hpib_bus_status argument */

#define REMOTE_STATUS		0
#define SRQ_STATUS		1
#define NDAC_STATUS		2
#define SYS_CONT_STATUS		3
#define ACT_CONT_STATUS		4
#define TALKER_STATUS		5
#define LISTENER_STATUS 	6
#define CURRENT_BUS_ADDRESS	7

/*  definitions for hpib_status_wait argument */

#define WAIT_FOR_SRQ		1
#define WAIT_FOR_CONTROL	4
#define WAIT_FOR_TALKER		5
#define WAIT_FOR_LISTENER	6

#ifdef __hp9000s300
/* maximum for length parameter of hpib_send_cmnd */

#define MAX_HPIB_COMMANDS	122

/* interrupt definitions for io_on_interrupt() */

#define DCL	0x00001	/* device clear             */
#define IFC	0x00002	/* interface clear asserted */
#define GET	0x00004	/* group execution trigger  */
#define LTN	0x00008	/* lintener addressed       */
#define TLK	0x00010	/* talker addressed         */
#define SRQ	0x00020	/* SRQ asserted             */
#define REN	0x00040	/* remote enable            */
#define TCT	0x00080	/* controller in charge     */
#define PPOLL	0x00100	/* parallel poll            */


/* extended interrupt definitions */

#define PPCC	0x00200	/* parallel configuration change        */
#define EOI	0x00400	/* EOI received                         */
#define SPAS	0x00800	/* requesting service and serial polled */
#define HSERR	0x02000	/* handshake error                      */
#define UUC	0x04000	/* unrecognized universal command       */
#define SECA	0x08000	/* secondary command while addressed    */
#define UAC	0x10000	/* unrecognized addressed command       */
#define MAC	0x20000	/* my address change			*/


/* GPIO interrupt definitions */

#define SIE0	  1
#define SIE1	  2
#define EIR	  4
#define RDY	  8

/* Parallel interrupt definitions */
#define NERROR  0x4
#define SELECT  0x2
#define PE      0x1

/* HP-IB addressing group bases */
#define	LAG_BASE   	0x20	 /*  listener address base  */
#define	TAG_BASE   	0x40	 /*  talker address base    */
#define	SCG_BASE   	0x60	 /*  secondary address base */

/* HP-IB command equates */
#define	GTL         0x01	/*  go to local            */
#define	SDC         0x04	/*  selective device clear */
#define	PPC         0x05	/*  ppoll configure        */
#define	GETR        0x08	/*  group execute trigger  */
#define	TCTL        0x09	/*  take control           */
#define	LLO         0x11	/*  local lockout          */
#define	DCLR        0x14	/*  device clear           */
#define	PPU         0x15	/*  ppoll unconfigure      */
#define	SPE         0x18	/*  spoll enable           */
#define	SPD         0x19	/*  spoll disable          */
#define	UNL         0x3F	/*  unlisten               */
#define	UNT         0x5F	/*  untalk                 */

#define RSV_MASK    0x40	 /* SRQ is set by this bit (in spoll byte) */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
/* maximum for length parameter of hpib_send_cmnd */

#define MAX_HPIB_COMMANDS	100

#define REN	(1 <<  0)
#define SRQ	(1 <<  1)
#define NDAC	(1 <<  2)
#define SC	(1 <<  3)
#define TCT	(1 <<  4)
#define TLK	(1 <<  5)
#define LTN	(1 <<  6)
#define ADDRESS	(1 <<  7)
#define PPOLL	(1 <<  8)
#define GET	(1 <<  9)
#define DCL	(1 << 10)
#define IFC	(1 << 11)
#define EIR	(1 << 12)
#define SIE0	(0)
#define SIE1	(0)
#endif /* __hp9000s800 */


#endif /* _INCLUDE_HPUX_SOURCE */
#endif /* _MACHINE_DVIO_INCLUDED */
