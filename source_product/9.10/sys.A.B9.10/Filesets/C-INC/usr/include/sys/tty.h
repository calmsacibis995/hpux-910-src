/* @(#) $Revision: 1.65.83.6 $ */      
#ifndef _SYS_TTY_INCLUDED /* allows multiple inclusion */
#define _SYS_TTY_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _HPUX_SOURCE

#define	BSDTOSTOP		00000001
#if defined(_POSIX_VDISABLE)
#undef _POSIX_VDISABLE
#endif /* defined(_POSIX_VDISABLE) */
#define _POSIX_VDISABLE		'\377'

/* set Posix job control TOSTOP flag depending on BSD setting */
#define SET_TOSTOP	if (tp->t_lmode & BSDTOSTOP) \
				tp->t_lflag |= TOSTOP; \
			else \
				tp->t_lflag &= ~TOSTOP;

/* set BSDTOSTOP flag depending on Posix TOSTOP flag */
#define SET_BSDTOSTOP	if (tp->t_lflag & TOSTOP) \
				tp->t_lmode |= BSDTOSTOP; \
			else \
				tp->t_lmode &= ~BSDTOSTOP;

#ifndef __hp9000s800
/*
 * A clist structure is the head of a linked list queue of characters.
 * The routines getc* and putc* manipulate these structures.
 */

struct clist {
	int	c_cc;		/* character count */
	struct cblock *c_cf;	/* pointer to first */
	struct cblock *c_cl;	/* pointer to last */
};


#define NCCS   16	/* duplicated from termios.h */
#define	NLDCC	17
#define VDSUSP  (NLDCC-1)

/* new modem stuff */
#define	NMTIMERS	6
#define	LINE_CLEARED	0
#define	LINE_SET	1
#define	VALID_RING	HZ/2	/* 1/2 second for now */

#ifdef __hp9000s300
#ifdef _KERNEL_BUILD
#include "../machine/timeout.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/timeout.h>
#endif /* _KERNEL_BUILD */

struct tty_driver {
	int	type;			/* Driver Number */
	int	(*open)();		/* open(dev, tp, flag) */
	int	(*close)();		/* close(dev, tp) */
	int	(*read)();		/* read(tp) */
	int	(*write)();		/* write(tp) */
	int	(*ioctl)();		/* ioctl(dev, tp, cmd, arg, mode) */
	int	(*select)();		/* select(tp) */
	int	(*kputchar)();		/* putchar(c); */
	int	(*wait)();		/* wait(tp); */
	struct tty * (*pwr_init)();	/* pwr_init(&tp, addr, id, il, &cs,
						&pci_total); */
	int	(*who_init)();		/* who_init(tp, tty_number, sc)
						For console tty_number == -1 */
	struct tty_driver *next;	/* Pointer to next one in list */
};
#endif /* __hp9000s300 */

/*
 * A tty structure is needed for each UNIX character device that
 * is used for normal terminal IO.
 */

struct tty {
	struct	clist t_rawq;	/* raw input queue */
	struct	clist t_canq;	/* canonical queue */
	struct	clist t_outq;	/* output queue */
	struct cblock *t_buf;	/* buffer pointer */
	int	(* t_proc)();	/* routine for device functions */
	struct	proc *t_rsel;	/* tty */
	struct	proc *t_wsel;	/* tty */
	dev_t   t_dev;          /* device */
	u_long	t_iflag;	/* input modes */
	u_long	t_oflag;	/* output modes */
	u_long	t_cflag;	/* control modes */
	u_long	t_lflag;	/* line discipline modes */
	u_long  t_state;        /* internal state */
	int	t_lmode;	/* BSD local mode word */
	short	t_pgrp;		/* process group name */
	char	t_line;		/* line discipline */
	char	t_delct;	/* delimiter count */
	char	t_col;		/* current column */
	char	t_row;		/* current row */
	unsigned char   t_cc[NLDCC];      /* settable control chars */
	int	*utility;	/* driver dependent info */
	struct proc *t_pidprc;	/* proc for system asynchronous I/O */
	struct proc *t_cproc;   /* pointer to proc structure of controlling
				   process */	/* TEMP for 6.5->7.0 PS */
	struct proc *t_cttyhp;	/* list of processes sharing controlling tty */
#ifdef __hp9000s300
	int	t_slcnl;	/* shadow registers: line control */
	int	t_smcnl;	/*		     modem control */
	int	t_sicnl;	/* 		     interrupt control */
	struct  timeout timeo;	/* for timouts/delays */
	char	*t_card_ptr;	/* Address of card */
	int	t_int_lvl;	/* Interrupt Level of the Card */
	struct tty_driver *t_drvtype;	/* Driver Type */
	ushort	*t_inbuf;	/* Input character buffer */
	ushort	*t_in_head;	/* Head of character queue */
	ushort	*t_in_tail;	/* Tail of character queue */
	ushort	t_in_count;	/* Number of characters in queue */
	char	t_int_flag;	/* More Flags */
	char	t_rcv_flag;	/* Pending interrupt for receive */
	char	t_xmt_flag;	/* Pending interrupt for transmit */
	char	t_open_type;	/* type of open that owns the card */
	struct sw_intloc rcv_intloc;	/* Software trigger receive structure */
	struct sw_intloc xmit_intloc; /* Software trigger transmit structure */
	struct sw_intloc tttimeo_intloc; /* Software trigger ttimeo structure */
	struct sw_intloc ttrstrt_intloc; /* Software trigger ttstrt structure */
	struct  timeout t_open;	/* for modem timeout stuff */
	struct  timeout t_DCD;	/* for modem timeout stuff */
	char	modem_status;	/* last state of modem input lines */
	char	tty_count;	/* keep number of direct opens */
	char	cul_count;	/* keep number of call out opens */
	char	ttyd_count;	/* keep number of call in opens */
	long	t_time;		/* Keep track of elasped time for RI */
	unsigned short	timers[NMTIMERS];  /* modem specific timers */
	unsigned char t_hardware;	/* hardware capabilities */	
	u_char	t_tx_limit;	/* Number of chars to send after XOFF */
	u_char	t_tx_count;	/* Number of chars left to send */
	u_char	t_isc_num;	/* select code for this interface */
	unsigned short	t_ws_row;	/* Terminal size fields */
	unsigned short	t_ws_col;
	unsigned short	t_ws_xpixel;
	unsigned short	t_ws_ypixel;
	ushort  t_hw_flow_ctl;  /* Hardware flow control states */
	ushort  t_dvr_fc;	/* Driver specific flow control */
	u_char	t_open_sema;	/* Open/close semaphore */
#endif /* __hp9000s300 */
};

#    define _T_cc_enabled(c, index)	\
	((c == tp->t_cc[index]) && (c != (cc_t)_POSIX_VDISABLE))

#ifdef __hp9000s300
/* Minor device bits */
#define CCITT_MODE       0x0002
#define SIMPLE_MODE      0x0000
#define DEV_TTY          0x0004		/* direct connect */
#define DEV_CUL          0x0001		/* call out */
#define DEV_TTYD         0x0000		/* call in */

/* Hardware capability bits */
#define	FIFO_MODE		0x01
#define	FIFOS_ENABLED		0x02
#define	HARDWARE_HANDSHAKE	0x04
#define	HARDWARE_HNDSHK_ENABLED	0x08
#define	HIGHSPEED_CLOCK		0x10
#define	HIGHSPEED_CLOCK_ENABLED	0x20
#define	HIDDENMODE		0x40
#define	HIDDENMODE_ENABLED	0x80

#define	HWFC_MASK	3	/* CTSXON and RTSXOFF only */
#define	DFLOW		1	/* Driver specific flow control */

#endif /* __hp9000s300 */

/*
 * The structure of a clist block
 */
#define	CBSIZE	26		/* number of chars in a clist block */

struct cblock {
	struct cblock *c_next;
	char	c_first;
	char	c_last;
	char	c_data[CBSIZE];
};

#ifdef _KERNEL
extern struct cblock * getcb();
extern struct cblock * getcf();
extern struct clist ttnulq;

struct chead {
	struct cblock *c_next;
	int	c_size;
};
struct chead cfreelist;
extern struct cblock *cfree;
#ifdef	HPUXTTY
extern int nclist;
int cfreecount;
#else	/* HPUXTTY */
int nclist;
#endif	/* HPUXTTY */
#endif


#define	TTIPRI	PZERO+3
#define	TTOPRI	PZERO+4


/* limits */
extern int ttlowat[], tthiwat[];
#define	TTYHOG	512	/* min(TTYHOG-TTXOHI,card_buffer_size) is the number of
			** character times that the 628 card can be locked out
			** by the kernel without dropping characters.
			*/

#ifdef __hp9000s300
#define	PARAM_INT 	8	/* ran out of room when changing params */
#endif /* __hp9000s300 */

#define	TTXOLO	60
#define	TTXOHI	180


#ifdef HPUXTTY
#define	TTHIWAT(tp)	tthiwat[(tp)->t_cflag&CBAUD]
#define	TTLOWAT(tp)	ttlowat[(tp)->t_cflag&CBAUD]
#endif /* HPUXTTY */

/* Hardware bits */
#define	DONE	0200
#define	IENABLE	0100
#define	OVERRUN	040000
#define	FRERROR	020000
#define	PERROR	010000
#define	MODEMSTATUS 0100000

/* Internal state */
#define	TIMEOUT	0000000001		/* Delay timeout in progress */
#define	WOPEN	0000000002		/* Waiting for open to complete */
#define	ISOPEN	0000000004		/* Device is open */
#define TS_ISOPEN ISOPEN
#define	TBLOCK	0000000010
#define	CARR_ON	0000000020		/* Software copy of carrier-present */
#define	BUSY	0000000040		/* Output in progress */
#define	OASLP	0000000100		/* Wakeup when output done */
#define	IASLP	0000000200		/* Wakeup when input done */
#define	TTSTOP	0000000400		/* Output stopped by ctl-s */
#ifdef __hp9000s300
#define ASYNC   0000001000              /* tty in async i/o mode */
#else
#define EXTPROC 01000                   /* External processing */
#endif

#define	TACT	0000002000
#define ESC     0000004000              /* Last char escape */
#define	RTO	0000010000
#define	TTIOW	0000020000
#define	TTXON	0000040000
#define	TTXOFF	0000100000

#ifdef __hp9000s300
#define	NBIO		 0000200000	/* Non-Blocking I/O enabled */
#define	PDIOLCK		 0000400000   /* Disable pdi output during ioctl wait */
#define	RCOLL		 0001000000   /* collision in read select */
#define	WCOLL		 0002000000   /* collision in write select */
#define CCITT   	 0004000000   /* Device open for CCITT modem control */
#define SIMPLE  	 0010000000   /* Device open for Simple modem control */
#define TTYD    	 0020000000   /* Device open/pending for TTYD access */
#define CUL     	 0040000000   /* Device open/pending for CUL  access */
#define TTY     	 0100000000   /* Device open for TTY access */
#define TTYD_NDELAY 	 0200000000   /* Non-blocking TTYD open sleazed thru */
#define TTYD_PENDING 	 0400000000   /* CCITT ttyd open is pending */
#define WAIT_FOR_RING 	01000000000   /* CCITT ttyd open is waiting for ring */
#define OPEN_TO_PENDING 02000000000   /* Open timeout is pending */
#define OPEN_TIMED_OUT  04000000000   /* Open has timed out */
#define CARRIER_TO_PENDING 010000000000  /* Loss of carrier timeout pending */
#define DELTA_CLOCAL       020000000000  /* Loss of carrier timeout pending */
/* NO MORE BITS */

/* t_open_type */
#define TTY_OPEN       0x01  /* A direct connect dev has this open */
#define CUL_OPEN       0x02  /* A call out dev has this open */
#define TTYD_OPEN      0x04  /* A call in dev has this open */
#endif /* __hp9000s300 */

#ifdef HPUXTTY
#define TS_RCOLL        0000200000      /* collision in read select */
#define TS_WCOLL        0000400000      /* collision in write select */
#ifdef	ASYNCIO
#define	TS_ASYNC	0001000000	/* tty in async i/o mode */
#endif	/* ASYNCIO */
#endif  /* HPUXTTY */

/* l_output status */
#define	CPRES	1

/* device commands */
#define	T_OUTPUT	0
#define	T_TIME		1
#define	T_SUSPEND	2
#define	T_RESUME	3
#define	T_BLOCK		4
#define	T_UNBLOCK	5
#define	T_RFLUSH	6
#define	T_WFLUSH	7
#define	T_BREAK		8

#define T_PARM		11
#define T_MODEM_STAT	100
#define T_MODEM_CNTL	101


#ifdef __hp9000s300
/*********************************************************************/
/* The following stuff should probably be in a different header file */

/* Supported TTY driver types */
#define	SIONULL		0	/* Dummy Driver for Run only system */
#define	SIO626		1	/* 98626 Driver */
#define	SIO628		2	/* 98628 Driver */
#define	SIO642		3	/* 98642 Driver */
#define	ITE200		4	/* Internal Terminal Emulator 9826/36/17 9920 */
#define	TTYDRIVS	5	/* Number of tty drivers possible */

/* Level Zero Interrupt Flags */
#define	PEND_TXINT	0x01	/* pending transfer interrupt */
#define	NMI_LCK		0x02	/* queuing routine is locked from NMI's */
#define	NMI_CNTL 	0x04	/* Control-Shift-PAUSE is pending from NMI */
#define	NMI_SHFT 	0x08	/* Shift-PAUSE is pending from NMI */
#define START_OF_RING 	0x10	/* Loss of carrier timeout pending */

#define	LINENO	0x1F		/* line number part of dev minor */
#define	TTYBUF_SIZE	256	/* Number of entries in TTY input buffer 
				** for delayed isr's
				*/
#endif /* __hp9000s300 */
#endif /* not hp9000s800 (68K) */

#ifdef __hp9000s800
/*
 * A tty structure is needed for each UNIX character device that
 * is used for normal terminal IO.
 */

#define	NCC	8

#ifdef _KERNEL_BUILD
#include "../h/clist.h"
#include "../h/modem.h"
#include "../h/blmodeio.h"
#include "../h/proc.h"
#ifdef _WSIO
#include "../machine/timeout.h"
#endif /* _WSIO */
#else /* ! _KERNEL_BUILD */
#include <sys/clist.h>
#include <sys/modem.h>
#include <sys/blmodeio.h>
#ifdef _WSIO
#include <sys/timeout.h>
#endif /* _WSIO */
#endif /* _KERNEL_BUILD */

#define	d_control	d_option1

#define	NLDCC	22

struct tty_blmode {		 	/* block mode fields of tty struct  */
	struct blmodeio user;	 	    /* Copy of user blmode settings */
	unsigned int trigger_checked  : 1;  /* 1st/2nd trig sent if defined */
	unsigned int second_trigger   : 1;  /* 2nd trigger sent, not 1st    */
	unsigned int timing_xfer      : 1;  /* Timer for term char started  */
	unsigned int receiving_xfer   : 1;  /* Looking for terminator char  */
	unsigned int xfer_received    : 1;  /* Terminator char seen         */
	unsigned int discarding_xfer  : 1;  /* Discarding until term seen   */
	unsigned int xfer_error       : 1;  /* Error occurred in transfer   */
};

struct tty {
	struct	clist t_rawq;	/* raw input queue */
	struct	clist t_canq;	/* canonical queue */
	struct	clist t_outq;	/* output queue */
	struct	ccblock t_tbuf;	/* tx control block */
	struct	ccblock t_rbuf;	/* rx control block */
	struct	proc *t_rsel;	/* process table entry of first process
				   to sleep during read select */
	struct	proc *t_wsel;	/* process table entry of first process
				   to sleep during write select */
	dev_t	t_dev;		/* device */
	u_long	t_iflag;	/* input modes */
	u_long	t_oflag;	/* output modes */
	u_long	t_cflag;	/* control modes */
	u_long	t_lflag;	/* line discipline modes */
	u_short	t_hp;		/* HP tty flag */
	u_long	t_state;	/* internal state */
	int	t_lmode;	/* BSD local mode word */
	short	t_pgrp;		/* process group name */
	char	t_line;		/* line discipline */
	u_char	t_delct;	/* delimiter count */
	u_char	t_col;		/* current column */
	u_char	t_row;		/* current row */
	u_char	t_rocount;	/* current count for rubout */
	u_char	t_rocol;	/* current column for rubout */
	u_char	t_stpcnt;	/* stop character count */
	u_char	t_delay;	/* delay */
	u_char	t_cc[NLDCC];	/* settable control chars */
	int 	*utility;	/* driver dependent info */
	u_long	t_tdstate;	/* flag for TTY-driver use */
	struct pidprc t_pidprc;
				/* extended PID for system asynchronous I/O */
#ifdef	DBG
	u_long	t_incnt;	/* input character count */
	u_long	t_outcnt;	/* output character count */
#endif	/* DBG */
	struct  tty_blmode t_blmode;	/* Block mode structure */
	u_char  t_readlen;	/* Length of read in bytes */
	struct  uio *nuio;
	u_char  tflag;
 	struct proc *t_cproc;	/* sesion leader of this controlling tty */
	int	(* t_proc)();	/* routine for device functions */
	int	xt_link;        /* index into link table */
	unsigned short	t_ws_row;	/* Terminal size fields */
	unsigned short	t_ws_col;
	unsigned short	t_ws_xpixel;
	unsigned short	t_ws_ypixel;
	u_char	*ttin_tab;
	int	reserve6;
	int	reserve7;
};

#define	QESC	0200		/* queue escape */

#define	TTIPRI	PZERO+3		/* input sleep priority */
#define	TTOPRI	PZERO+4		/* output sleep priority */

/* limits */

extern int ttlowat[], tthiwat[];
				/* water mark arrays */
#define	TTYHOG	768		/* maximum queue size */
#define	TTXOLO	60		/* low water mark */
#define	TTXOHI	180		/* high water mark */

#define	TTHIWAT(tp)	tthiwat[(tp)->t_cflag&CBAUD]
#define	TTLOWAT(tp)	ttlowat[(tp)->t_cflag&CBAUD]

/* Internal state */

#define	TIMEOUT		0000000001	/* delay timeout in progress */
#define	ASYNC		0000000002	/* async i/o mode */
#define	NBIO		0000000004	/* non-blocking write mode */
#define	TBLOCK		0000000010	/* input requested to be stopped */
#define	CARR_ON		0000000020	/* software copy of carrier-present */
#define	BROKEN		0000000040	/* hardware is broken (diagnostics) */
#define	OASLP		0000000100	/* wakeup when output done */
#define	IASLP		0000000200	/* wakeup when input done */
#define	TTSTOP		0000000400	/* output stopped by ctl-s */
#define	EXTPROC		0000001000	/* external processing */
#define	TACT		0000002000	/* timeout active */
#define	CLESC		0000004000	/* last char escape */
#define	RTO		0000010000	/* raw timeout in progress */
#define	TTIOW		0000020000	/* waiting for output to drain */
#define	TTXON		0000040000	/* input not stopped */
#define	TTXOFF		0000100000	/* input stopped */
#define	RCOLL		0000200000	/* collision in read select */
#define WCOLL		0000400000	/* collision in write select */
#define DFLOW		0001000000	/* driver flow control */
#define	IOPOK		0002000000	/* it is OK to process I/O */
#define	OFULL		0004000000	/* output blocked on water mark */

/* TTY-Driver state */

#define	TTOUT		0000000001	/* TTY output data ready */

#ifdef	DBG
extern int dbgasio;
#endif	/* DBG */


/* Additional control character indices */
#define VDSUSP  (NLDCC-1)
#define	VDELAY	VDSUSP

#define NMI_LCK         0x02    /* queuing routine is locked from NMI's */
#define NMI_CNTL        0x04    /* Control-Shift-PAUSE is pending from NMI */
#define NMI_SHFT        0x08    /* Shift-PAUSE is pending from NMI */

#endif /* __hp9000s800 */

/* get pointer to tty structure for this process's controlling tty */
#define CONTROL_TTY(xu)		((xu).u_procp->p_ttyp)

/* get pointer to proc structure tty's controlling process*/
#define CONTROL_PROC(ttyp)	(ttyp->t_cproc)

/* get process group id of current process */
#define GROUP_ID(xu)	((xu).u_procp->p_pgrp)

/* is current process a group leader */
#define GROUP_LEADER(xu)	((xu).u_procp->p_pid == GROUP_ID(xu))

/* get process group id of foreground process group of tty */
/* if none, then PGID_NOT_SET */
#define FOREGROUND_ID(xtp)	((xtp)->t_pgrp)

/* get session id of current process */
#define SESSION_ID( xu) 	(xu).u_procp->p_sid

/* is current process a session leader */
#define SESSION_LEADER(xu)	((xu).u_procp->p_pid == SESSION_ID(xu))

/* get session id of session leader of tty specified
(assumed to have a controlling terminal) 0 = no session leader */
#define SESSION_LEADER_ID(ttyp)	(CONTROL_PROC(ttyp) \
				? CONTROL_PROC(ttyp)->p_sid : 0)

/* Have a controlling terminal but don't have a session leader */
#define ORPHANED(xu)   (orphaned(GROUP_ID(xu), SESSION_ID(xu), NULL))

/* is current process in background relative to tty */
#define BACKGROUND(xu, xtp)	((GROUP_ID(xu) != FOREGROUND_ID(xtp)) \
			&& FOREGROUND_ID(xtp) && GROUP_ID(xu))

/* is current process in foreground relative to tty */
#define FOREGROUND(xu, xtp)	(!BACKGROUND(xu, xtp))

/* make current process the controlling process for the tty */
#define SET_SESSION_LEADER( xu, xttyp)  CONTROL_TTY(xu) = xttyp; \
				(xu).u_procp->p_ttyd = xttyp->t_dev; \
				FOREGROUND_ID(xttyp) = GROUP_ID(xu); \
				CONTROL_PROC(xttyp) = (xu).u_procp;

/* make the controlling tty of the current process not a controlling terminal */
#define FREE_CTTY(xttyp)	CONTROL_PROC(xttyp) = NULL; \
					FOREGROUND_ID(xttyp) = 0;

/* return either EAGAIN or EWOULDBLOCK */
#define IOBLOCKED(xuio) (((xuio)->uio_fpflags & FNBLOCK) ? EAGAIN : EWOULDBLOCK)

/* is write blocked */
#define WRITE_BLOCKED(xuio, xtp) 	((xuio->uio_fpflags & FNBLOCK) || \
					(xtp->t_state & NBIO))

/* Input char is special only if match and it is not equal to _POSIX_VDISABLE */
#define SPECIAL_CHAR( xc, xc1)  (((xc) == (xc1)) && ((xc) != \
				(unsigned char) _POSIX_VDISABLE))

/* get input baud rate code */
#define GET_IN_BAUD(xv)		(((xv)&CINBAUD) >> CINBAUDSHIFT)

/* set input baud rate code */
#define SET_IN_BAUD(xv)		(((xv)&CBAUD) << CINBAUDSHIFT)

/* did our controlling tty change sessions */
#define CONTROL_TTY_CHANGED(ux) (SESSION_ID(ux) != \
				SESSION_LEADER_ID((ux).u_procp->p_ttyp))

#endif /* _HPUX_SOURCE */
#endif /* ! _SYS_TTY_INCLUDED */
