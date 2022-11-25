/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/dilio.h,v $
 * $Revision: 1.4.83.5 $	$Author: drew $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/13 12:00:16 $
 */
/* @(#) $Revision: 1.4.83.5 $ */       
/*
**	dilio.h	Device I/O Library header for kernel
*/
#ifndef _SYS_DILIO_INCLUDED /* allows multiple inclusion */
#define _SYS_DILIO_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/* stuff kept in buf structure */
#define	pass_data1		b_bufsize	/* put temp parameter info */
#define	pass_data2		b_pfcent	/* put temp parameter info */
#define	return_data		b_bufsize	/* put temp return info */
#define ppoll_data		b_s1	/* put termination reason stuff here */
#define	dil_packet		b_s2

/* stuff kept in iobuf structure */
#define	read_pattern		io_nreg	/* hold read terminating pattern */
#define term_reason		io_s0	/* put termination reason stuff here */
#define	activity_timeout	io_s1	/* for use during any activity */
#define	dil_state		io_s2	/* keep dil info */
#define	dil_timeout		timeo	/* for all timeouts */

/* buf.h uses 0x40000000 for B_DIL to show that this is a dil buffer */

/* These flags are kept in dil_state.  */
#define D_CHAN_LOCKED	0x001	/* this process has the channel locked */
#define D_16_BIT 	0x002	/* transfer width is 16 bit */
#define D_RAW_CHAN 	0x004	/* this is a raw open */
#define D_MAPPED 	0x008	/* this was mapped into user space */


/* transfer information for select code in dil_state */
#define EIR_CONTROL     0x010   /* do(not) terminate transfers on EIR */
#define READ_PATTERN	0x040
#define EOI_CONTROL	0x080
#define USE_DMA		0x100
#define USE_INTR	0x200
#define PC_HANDSHK      0x400
#define NO_NACK		0x1000  /* for centronics, it indicates handshake
				   mode */

/* parity control in iob->b_flags */
#define PARITY_CONTROL	B_SCRACH1

/* dma control information in iob->b_flags */
#define ACTIVE_DMA	B_SCRACH4
#define RESERVED_DMA	B_SCRACH5
#define LOCKED_DMA	B_PAGEOUT


#define NO_ADDRESS	31	/* invalid address for the bus*/

#define DILPRI	PZERO+3

/* iov generic structure */
struct diliovec {
	int	*buffer;
	int	count;
	int	command;
};


/*
**	info for 98622 card 
*/

/* memory layout of the card */
struct GPIO {				/* address */
	unsigned char ready;		/*   0	   */
	unsigned char id_reg;		/*   1 	   */
	unsigned char intr_mask;	/*   2 	   */
	unsigned char intr_control;	/*   3 	   */
	union {				/*   4,5   */
		struct {
			unsigned char upper;
			unsigned char lower;
		} byte;
		unsigned short word;
	} data;
	unsigned char nop1;
	unsigned char p_status;		/*   7	   */
};

#define set_pctl ready
#define	reset_gpio id_reg
#define	intr_status intr_control
#define p_control p_status

/* (addr 0) ready for data status */

#define GP_READY	0x01

/* (addr 2) ready/external interrupt control assignments */
#define	GP_EIR1 	0x01
#define	GP_EIR0 	0x00
#define	GP_RDYEN1	0x02
#define	GP_RDYEN0	0x00

/* (addr 3) interrupt/dma control assignments */
#define	GP_DMA0		0x01
#define	GP_DMA1		0x02
#define	GP_WORD		0x04
#define	GP_BURST	0x08
#define	GP_ENAB		0x80

/* (addr 7) status register */
#define	GP_INT_EIR	0x04
#define	GP_PSTS		0x08


#define NO_TIMEOUT	B_SCRACH3


#define RAW_CHAN_ONLY   if (!(state & D_RAW_CHAN)) { \
				bp->b_error = ENOTTY; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define ACT_CTLR_ONLY   if (!(sc->state & ACTIVE_CONTROLLER)) { \
				bp->b_error = EIO; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define SYS_CTLR_ONLY   if (!(sc->state & SYSTEM_CONTROLLER)) { \
				bp->b_error = EIO; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define DIL_START_TIME(proc)                                             \
			if (!(iob->b_flags & NO_TIMEOUT)) {              \
				START_TIME(proc, iob->activity_timeout); \
			}  

/* sc->intr_wait and sc->intr_event info */
#define WAIT_CTLR	0x0001
#define WAIT_SRQ	0x0002
#define WAIT_TALK	0x0004
#define WAIT_LISTEN	0x0008
#define WAIT_READ	0x0020
#define WAIT_WRITE	0x0040
#define	WAIT_MASK	0x006f

/* user interrupt event info */

#define	INTR_DCL	0x0000100
#define	INTR_IFC	0x0000200
#define	INTR_GET	0x0000400
#define	INTR_LISTEN	0x0000800
#define	INTR_TALK	0x0001000
#define	INTR_SRQ	0x0002000
#define	INTR_REN	0x0004000
#define	INTR_CTLR	0x0008000
#define	INTR_PPOLL	0x0010000

/* extended interrupt definitions */

#define INTR_PPCC	0x0020000 /* parallel configuration change        */
#define INTR_EOI	0x0040000 /* EOI received                         */
#define INTR_SPAS	0x0080000 /* requesting service and serial polled */
/* 			0x0100000    currently unused                     */
#define INTR_ERR	0x0200000 /* handshake error                      */
#define INTR_UUC	0x0400000 /* unrecognized universal command       */
#define INTR_APT	0x0800000 /* secondary command while addressed    */
#define INTR_UAC	0x1000000 /* unrecognized addressed command       */
#define INTR_MAC	0x2000000 /* my address change interrupt	  */

#define	INTR_MASK	0x3ffff00

#define	INTR_GPIO_EIR	0x00400
#define	INTR_GPIO_RDY	0x00800

/* parallel printer interface interrupt definitions */
#define INTR_PARALLEL_STROBE            0x2000
#define INTR_PARALLEL_BUSY              0x1000
#define INTR_PARALLEL_ERROR             0x0400
#define INTR_PARALLEL_SELECT            0x0200
#define INTR_PARALLEL_PERROR            0x0100

/* bit mask of allowable user interrupts */
#define INTR_PARALLEL_MASK      (                               \
                                  INTR_PARALLEL_STROBE  |       \
                                  INTR_PARALLEL_BUSY    |       \
                                  INTR_PARALLEL_ERROR   |       \
                                  INTR_PARALLEL_SELECT  |       \
                                  INTR_PARALLEL_PERROR          \
                                )


#define NO_RUPT_HANDLER	B_SCRACH2

/* dil info packet */
struct dil_info {
	struct buf * dil_forw;
	struct buf * dil_back;
	struct buf * event_forw;
	struct buf * event_back;
	int eid;
	int (*handler)();
	int event;
	int cause;
	int ppl_mask;
	struct proc *dil_procp;
	int (*dil_action)();
	int (*dil_timeout_proc)();
	int intr_wait;
	int register_data;
 	int hpibio_addr;
 	int hpibio_cnt;
 	int full_tfr_count;
	int open_flags;
	caddr_t map_addr;
	pid_t locking_pid;
	pid_t last_pid;
	char cmd_buf[128];
	struct buf *rel_bp_on_close;
};

#endif /* _SYS_DILIO_INCLUDED */
