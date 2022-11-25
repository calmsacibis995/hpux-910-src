/* @(#) $Revision: 1.18.83.4 $ */      
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/pty.h,v $
 * $Revision: 1.18.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:32:53 $
 */
#ifndef _SYS_PTY_INCLUDED /* allows multiple inclusion */
#define _SYS_PTY_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/ptyio.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ptyio.h>
#endif /* _KERNEL_BUILD */

#define	TRUE		1	
#define	FALSE		0	
#define	TRAPTERMIO	1
#ifdef __hp9000s300
#define	TRAPRW		0
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#define TRAPNONTERMIO	2		/* non-termio ioctl trapped */
#define TRAPOPENCLOSE	0		/* open or close trapped */
#endif /* __hp9000s800 */
#define	PSEL_COLL	1		/* Select Collision Flag */

struct pty_select {
	struct proc *pty_selp;
	short pty_selflag;
};

struct pty_info {
	struct pty_select pty_selr;
	struct pty_select pty_selw;
	struct pty_select pty_sele;
	char exclusive;			/* ptm is being used exclusively */
	char ptmsleep;			/* ptm is asleep waiting for data
					   to read */
	struct proc *u_procp;		/* Pointer to ptm proc */
	char trapbusy;			/* pts is busy trapping an
					   ioctl/open/close */
	char trapwait;			/* pts is sleeping waiting for turn
					   to trap ioctl/open/close */
#ifdef __hp9000s300
	char trapcomplete;		/* pts is sleeping, waiting for ptm to 
					   read trapped ioctl/open/close */
#endif /* __hp9000s300 */
#ifdef __hp9000s800
	char trapnoshake;		/* ptm has not completed handshake of
					   a trapped ioctl/open/close */
	char trappending;		/* pts is waiting for ptm to handshake
					   a trapped ioctl/open/close */
#endif /* __hp9000s800 */
	char tioctrap;			/* Tell ptm about open, close, ioctl */
	char tiocsigmode;		/* signal handling mode */
	char tioctty;			/* termio processing enable */
	char tiocpkt;			/* TIOCPKT enabled */
	char tiocmonitor;		/* TIOCMONITOR enabled */
	char tiocremote;		/* TIOCREMOTE enabled */
	unsigned char pktbyte;		/* TIOCPKT byte value */
	char sendpktbyte;		/* Flag TIOCPKT byte to be sent */
#ifdef __hp9000s300
	struct request_info r_info;
#endif /* __hp9000s300 */
#ifdef __hp9000s800
	struct request_info trapinfo;	/* trapped pts open, close, ioctl */
	unsigned int termio_ioctl;	/* Ioctl request is a termio request */
#endif /* __hp9000s800 */
	char ioctl_buf[EFFECTIVE_IOCPARM_MASK+1]; /* buffer to hold pts ioctl */
	u_long pty_state;		/* pty status */
	u_long fileopen;		/* Number of open slave PTY FDs */
	void (* callout)();		/* call routine when output ready */
	u_char vhangup;
};

/* pty states */

#define PWOPEN		00001
#define PISOPEN		00002
#define PSPRES		00004
#define PMPRES		00010
#define PBLOCK   	00020
#define PUNBLOCK 	00040
#define PWMASTER 	00100		/* Flag to signify that a the master
					 * side of the pty is open.
					 */

#define MPTYCLONE       (makedev(16, (-1))) 	/* major/minor for clone device */
#endif /* _SYS_PTY_INCLUDED */
