/*
 * @(#)iobuf.h: $Revision: 1.4.83.3 $ $Date: 93/09/17 20:30:16 $
 * $Locker:  $
 */

/* @(#) $Revision: 1.4.83.3 $ */       

#ifndef _SYS_IOBUF_INCLUDED /* allows multiple inclusion */
#define _SYS_IOBUF_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _WSIO

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../machine/timeout.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <sys/timeout.h>
#endif /* _KERNEL_BUILD */

/*
 * Each block device has a iobuf, which contains private state stuff
 * and 2 list heads: the b_forw/b_back list, which is doubly linked
 * and has all the buffers currently associated with that major
 * device; and the d_actf/d_actl list, which is private to the
 * device but in fact is always used for the head and tail
 * of the I/O queue for the device.
 * Various routines in bio.c look at b_forw/b_back
 * (notice they are the same as in the buf structure)
 * but the rest is private to each device driver.
 */
struct iobuf
{
	long	b_flags;		/* see buf.h */
	struct	buf *b_forw;		/* first buffer for this dev */
	struct	buf *b_back;		/* last buffer for this dev */
	struct	buf *b_actf;		/* head of I/O queue */
	struct 	buf *b_actl;		/* tail of I/O queue */
	int	b_queuelen;		/* current queue length to device */
/***	struct	iostat	*io_stp;	/* unit I/O statistics */
	struct  timeout timeo;		/* for timeouts */
	struct  sw_intloc intloc;	/* for soft trigger on timeouts */
	int (**markstack)();		/* for timeout escapes */
	char	timeflag;		/* timeout has occurred */
	char	b_active;		/* busy flag */
	char	b_errcnt;		/* error count (for recovery) */
	char	b_state;		/* state for FSMs */
	char	io_s0;			/* space for drivers to leave things */
	char	in_fsm;			/* indicates flow in FSM for timeout */
	caddr_t	b_xaddr;		/* transfer address */
	long	b_xcount;		/* transfer count */
	long	b_headpos;		/* head position for scheduling */
	struct eblock	*io_erec;	/* error record */
	int	io_nreg;		/* number of registers to log on errors */
	physadr	io_addr;		/* csr address */
	int	io_s1;			/* space for drivers to leave things */
	int	io_s2;			/* space for drivers to leave things */
	int	io_s3;			/* space for drivers to leave things */
};
/*                       f f b f b dev           */
#define tabinit(dv)	{0,0,0,0,0,makedev(dv,0)} 

#define NDEVREG	(sizeof(struct device)/sizeof(int))

#define	B_ONCE	0x01	/* flag for once only driver operations */
/* #define	B_TAPE	0x02	/* this is a magtape (no bdwrite) */
#define	B_TIME	0x04	/* for timeout use */

/* The following macros are used to assure correct form of the finite
   state machines with respect to timeout, which must be of exactly
   the correct (and same) form in all the FSM's in the system.  This
   also permits tuning just this one place in case of bugs or 
   optimizations.  */

#define START_FSM	{						\
			iob->markstack = NULL;				\
			iob->timeflag = FALSE;				\
			iob->in_fsm = TRUE;				\
			if (!(iob->intloc.proc > (int (*)()) 1 )) {

#define END_FSM		{						\
			iob->in_fsm = FALSE;				\
			if (iob->timeflag) {				\
				iob->timeflag = FALSE;			\
				escape(TIMED_OUT);			\
			}						\
			}						\
			}						\
			}

#define START_TIME(proc, ticks)						\
			{						\
			timeout(proc, bp, ticks, &iob->timeo);		\
			}

#define END_TIME	{						\
			clear_timeout(&iob->timeo);			\
			if (iob->timeflag) {				\
				iob->timeflag = FALSE;			\
				escape(TIMED_OUT);			\
			}						\
			}

#define ABORT_TIME	{						\
			clear_timeout(&iob->timeo);			\
			iob->timeflag = FALSE;				\
			}
extern int (*fhs_timeout_proc)();

#define TIMEOUT_BODY(loc, prc, hw, sw, sta)				\
{									\
	register struct iobuf *iob = bp->b_queue;			\
	int flag_timeout();						\
									\
	if (bp->b_sc->state & FHS_ACTIVE) {				\
		bp->b_sc->state |= FHS_TIMED_OUT;			\
		iob->timeflag = TRUE;					\
		if (iob->markstack != NULL)				\
			*iob->markstack = fhs_timeout_proc;		\
		if (!iob->in_fsm) {					\
			iob->b_state = (int)sta;			\
			sw_trigger(&loc, prc, bp, hw, sw);		\
		}							\
	} else {							\
		if (!iob->in_fsm) {					\
			iob->b_state = (int)sta;			\
			sw_trigger(&loc, prc, bp, hw, sw);		\
		} else {						\
			if (iob->markstack == NULL)			\
				iob->timeflag = TRUE;			\
			else						\
				*iob->markstack = flag_timeout;		\
		}							\
	}								\
}

#endif /* defined(__hp9000s300) || defined(_WSIO) */
#endif /* ! _SYS_IOBUF_INCLUDED */
