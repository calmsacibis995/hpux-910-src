/* @(#) $Revision: 1.68.83.6 $ */

#ifndef _SYS_PARAM_INCLUDED /* allows multiple inclusion */
#define _SYS_PARAM_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

/*
 * Kernel Type Declarations
 */
#ifdef _KERNEL_BUILD
#ifndef LOCORE
#include "../h/types.h"
#endif /* not LOCORE */
#include "../machine/param.h"
#include "../h/time.h"
#ifdef __hp9000s800
#include "../machine/spl.h"
#endif /* __hp9000s800 */
#else /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <machine/param.h>
#include <sys/time.h>
#ifdef __hp9000s800
#include <machine/spl.h>
#endif /* __hp9000s800 */
#endif /* _KERNEL_BUILD */

/*
 * Machine-independent constants
 */

#ifdef _WSIO
/* defines for the front panel led lights */
#define led0            0x01
#define led1            0x02
#define led2            0x04
#define led3            0x08
#define KERN_OK_LED     0x10
#define DISK_DRV_LED    0x20
#define LAN_RCV_LED     0x40
#define LAN_XMIT_LED    0x80
#endif /* _WSIO */

#define	MSWAPX	15		/* pseudo mount table index for swapdev */

#define MAXCOMLEN 14		/* size of process name buffer for accounting */
#define	NOFILE	60		/* max open files per process */
#define MAX_LOCK_SIZE (long)(0x7fffffff) /* max lockable offset in file */

#define	MAXPID	30000		/* max process id */
#define	MAXUID	60000		/* max user id */
#define	MAXLINK	0x7fff		/* max # links to a file */
#ifndef MAX_CNODE		/* see also MAXCNODE in <cluster.h> */
#define MAX_CNODE 255           /* max cnode id */
#endif

#define MAXHOSTNAMELEN 64	/* max length of hostname */

#ifdef __hp9000s800
#if NBPG==4096
#define SSIZE	2		/* initial stack size/NBPG */
#define SINCR	2		/* increment of stack/NBPG */
#else /* NBPG == 4096 */
#define SSIZE	4		/* initial stack size/NBPG */
#define SINCR	4		/* increment of stack/NBPG */
#endif /* NBPG==4096 */

#define UAREA 0x7ffe6000                /* start of u area */
	/* KSTACKADDR is the base of the kernel stack, actually IN Uarea */
#define KSTACKADDR (UAREA+sizeof(struct user)-sizeof(double))  
#define TOPKSTACK (UAREA+UPAGES*NBPG)    /* top of kernel stack */

#define USRSTACK 0x7b033000   /* Start of user stack */
#define USRSTACKMAX UAREA     /* Top of user stack */

#define	CANBSIZ	256		/* max size of typewriter line	*/

#define	HZ	CLK_TCK		/* Ticks/second of the clock (in kernel land) */

#endif /* __hp9000s800 */

#ifdef __hp9000s300

/*
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack.  LOWPAGES and HIGHPAGES are
 * the number of pages from the beginning of the P0 region to the
 * beginning of the text and from the beginning of the P1 region to the
 * beginning of the stack respectively.
 */

#define	USRTEXT		0

#define UPAGES         1    /* pages for mapping struct user */
#define KSTACK_PAGES   4    /* pages of kernel stack (must be >= 2) */
#define FLOAT          5    /* pages of float card area */
#define DRAGON_PAGES  32
#define GAP1_PAGES    (216 - KSTACK_PAGES)
#define GAP2_PAGES    (3 - UPAGES)
#define HIGHPAGES  (DRAGON_PAGES+GAP1_PAGES+KSTACK_PAGES+UPAGES+GAP2_PAGES+FLOAT)
#define USRSTACK   (caddr_t) (-(HIGHPAGES*NBPG))
#define UAREA  (USRSTACK+((DRAGON_PAGES+GAP1_PAGES+KSTACK_PAGES)*NBPG))
#define FLOAT_AREA ((caddr_t) (-(FLOAT*NBPG)))
#define DRAGON_AREA     (USRSTACK)

#define KSTACK_RESERVE 3    /* # of reserve pages in kernel stack private pool */

#define	CANBSIZ	512		/* max size of typewriter line	*/

#define	HZ	CLK_TCK		/* Ticks/second of the clock (in kernel land) */

#endif /* __hp9000s300 */


/*
 * Maximum values (in seconds) for alarms, interval timers, and timeouts
 */
#define	MAX_ALARM	((unsigned long)(0x7fffffff / HZ))
#define	MAX_VTALARM	((unsigned long)0xffffffff)
#define	MAX_VT_ALARM	(MAX_VTALARM)	/* please use MAX_VTALARM instead */
#define	MAX_PROF	((unsigned long)0xffffffff)

#ifdef _KERNEL
/*
 * Note that the value of ARG_MAX, defined in <limits.h>, in shared source,
 * tracks this value as NCARGS-2.  If you change NCARGS, please also update
 * the value of ARG_MAX
 *
 * Also NOTE that NCARGS needs to be a multiple of NBPW.
 */
#endif /* _KERNEL */
#define	NCARGS	20480		/* # characters in exec arglist */

/*
 * priorities
 * should not be altered too much
 */

#define	PMASK	0xff
#define	PCATCH	0x100

#ifdef __hp9000s800
#define PRTBASE 0
#define PTIMESHARE 128
#endif /* __hp9000s800 */

/*
 * Priorities stronger (smaller number) than (or equal to) PZERO
 * are not signalable.
 */

#ifdef __hp9000s300
#define PRTBASE 0
#define PTIMESHARE 128
#endif /* __hp9000s300 */


#define	PSWP	(0+PTIMESHARE)
#define PMEM	(0+PTIMESHARE)
#define PRIRWLOCK (5+PTIMESHARE)
#define PRIBETA	(6+PTIMESHARE)
#define PRIALPHA (7+PTIMESHARE)
#define PRISYNC	(8+PTIMESHARE)
#define	PINOD	(10+PTIMESHARE)
#define	PRIBIO	(20+PTIMESHARE)
#define	PRIUBA	(24+PTIMESHARE)
#ifdef __hp9000s800
#define	PLLIO	(24+PTIMESHARE)
#endif /* __hp9000s800 */
#define	PZERO	(25+PTIMESHARE)
#define IPCPRI  (25+PTIMESHARE)

/*
 * Priorities weaker (bigger number) than PZERO are signalable.
 */

#define	PPIPE	(26+PTIMESHARE)
#define PVFS	(27+PTIMESHARE)
#define	PWAIT	(30+PTIMESHARE)
#define	PLOCK	(35+PTIMESHARE)
#define	PSLEP	(40+PTIMESHARE)
#define	PUSER	(50+PTIMESHARE)

#define PMAX_TIMESHARE	(127+PTIMESHARE)

/*
 * Special argument to changepri(), meaning "the appropriate non-real-time
 * priority".
 */
#define PNOT_REALTIME	(-1)

/*
 * Used in nice(2) calculations.
 */
#define NZERO 20

/*
 * fundamental constants of the implementation--
 * cannot be changed easily
 */

#define	NBPW	sizeof(int)	/* number of bytes in an integer */
#define NBTSPW	(NBBY*NBPW)	/* number of bits in an integer */

#define BSIZE   DEV_BSIZE
#define BSHIFT  DEV_BSHIFT

/*
 * NINDIR is the number of indirects in a file system block.
 */
#define	NINDIR(fs)	((fs)->fs_nindir)
/*
 * INOPB is the number of inodes in a secondary storage block.
 */
#define	INOPB(fs)	((fs)->fs_inopb)
#define INOPF(fs)       ((fs)->fs_inopb >> (fs)->fs_fragshift)
#ifndef NULL
#define	NULL	0
#endif
#define	CMASK	0		/* default mask for file creation */
#define	CDLIMIT	0x1FFFFFFF	/* default max write address */
#define	NODEV	(dev_t)(-1)
#define	SWDEF	(dev_t)(-2)
/* The root inode is the root of the file system.
 * Inode 0 can't be used for normal purposes and
 * historically bad blocks were linked to inode 1,
 * thus the root inode is 2. (inode 1 is no longer used for
 * this purpose, however numerous dump tapes make this
 * assumption, so we are stuck with it)
 * The lost+found directory is given the next available
 * inode when it is created by ``mkfs''.
 */
#define	ROOTINO		((ino_t)2)	/* i number of all roots */
#define LOSTFOUNDINO    (ROOTINO + 1)
#ifdef __hp9000s300
#define	SUPERBOFF	512	/* byte offset of the super block */
#define	DIRSIZ	14		/* max characters per directory */
#define	NICINOD	100		/* number of superblock inodes */
#endif /* __hp9000s300 */
#ifdef __hp9000s800
/*
 * Decode privilege level for HP-PA.
 */
#define PC_PRIV_MASK	3
#define PC_PRIV_KERN	0
#define PC_PRIV_USER	3

#define USERMODE(pc)	(((pc) & PC_PRIV_MASK) != PC_PRIV_KERN)

#define	lobyte(X)	(((unsigned char *)&X)[1])
#define	hibyte(X)	(((unsigned char *)&X)[0])
#define	loword(X)	(((ushort *)&X)[1])
#define	hiword(X)	(((ushort *)&X)[0])
#endif /* __hp9000s800 */

#ifdef __hp9000s300
#define USERMODE(ps)    (((ps) & PS_S) == 0)  /* check for user mode */
#define BASEPRI(ps)     (((ps) & PS_IPL) == 0)    /* check for int level 0 */
#endif /* __hp9000s300 */

#define	NGROUPS	20		/* max number groups */
#define	NOGROUP	((gid_t) -1)	/* marker for empty group set member */
/*
 * Signals.  Only include if defining _KERNEL.
 */
#ifdef _KERNEL
#ifdef _KERNEL_BUILD
#include "../h/signal.h"
#else /* ! _KERNEL_BUILD */
#include <sys/signal.h>
#endif /* _KERNEL_BUILD */
#endif /* _KERNEL */

#ifdef __hp9000s800
#ifdef MP
/* There are places where we would like to know if we have a possible
 * signal that we might have to handle.  If so, we have to grab the
 * kernel semaphore.  Note, this is a hint, and does not include the
 * call to issig...
 */
#define	ISSIG_MP(p) \
	(((p)->p_cursig) || \
	((p)->p_sig && ((p)->p_flag&STRC || \
	 ((p)->p_sig &~ (p)->p_sigmask))) )
#endif

#define	ISSIG(p) \
	((p)->p_sig && ((p)->p_flag&STRC || \
	 ((p)->p_sig &~ (p)->p_sigmask)) && issig())
#endif /* __hp9000s800 */

/*
 * Fundamental constants of the implementation.
 */
#ifndef NBBY
#define	NBBY	8		/* number of bits in a byte */
 				/* NOTE: this is also defined	*/
 				/* in fs.h (filsys.h).  So if	*/
 				/* NBBY gets changed, change it	*/
 				/* in fs.h (filsys.h) also	*/
#endif /* NBBY */

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of at most MAXBSIZE units,
 * with smaller units (fragments) only in the last direct block.
 * MAXBSIZE primarily determines the size of buffers in the buffer
 * pool. It may be made larger without any effect on existing
 * file systems; however making it smaller make make some file
 * systems unmountable.
 *
 * Note that the blocked devices are assumed to have DEV_BSIZE
 * "sectors" and that fragments must be some multiple of this size.
 * Block devices are read in BLKDEV_IOSIZE units. This number must
 * be a power of two and in the range of
 *	DEV_BSIZE <= BLKDEV_IOSIZE <= MAXBSIZE
 * This size has no effect upon the file system, but is usually set
 * to the block size of the root file system, so as to maximize the
 * speed of ``fsck''.
 */
#define	MAXBSIZE	65536
#ifndef	DEV_BSIZE
#define	DEV_BSIZE	1024
#define	DEV_BSHIFT	10      	/* log2(DEV_BSIZE) */
#define DEV_BMASK	(DEV_BSIZE-1)	/* For doing modulo functions */
#endif /* DEV_BSIZE */

#define BLKDEV_IOSIZE	2048
#define	BLKDEV_IOSHIFT	11
#define BLKDEV_IOMASK	(BLKDEV_IOSIZE-1)

#define	btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)

#define btodbup(bytes)			/* same as btodb but round up */ \
	(((unsigned)(bytes) + (DEV_BSIZE-1)) >> DEV_BSHIFT)

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and will be after we
 * add an entry to cdevsw for that purpose.  For now though
 * just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * MAXPATHLEN defines the longest permissable path length
 * after expanding symbolic links. It is used to allocate
 * a temporary buffer from the buffer pool in which to do the
 * name expansion, hence should be a power of two, and must
 * be less than or equal to MAXBSIZE.
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name. It should be set high
 * enough to allow all legitimate uses, but halt infinite loops
 * reasonably quickly.
 */
#define MAXPATHLEN	1024
#define MAXSYMLINKS	20

/*
 * bit map related macros
 */
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

/*
 * bit map related macros done on a word basis
 */
#define wsetbit(a,i)	((a)[(i)/NBTSPW] |= 1<<((i)%NBTSPW))
#define wclrbit(a,i)	((a)[(i)/NBTSPW] &= ~(1<<((i)%NBTSPW)))
#define wisset(a,i)	((a)[(i)/NBTSPW] & (1<<((i)%NBTSPW)))
#define wisclr(a,i)	(((a)[(i)/NBTSPW] & (1<<((i)%NBTSPW))) == 0)

/*
 * Macros for fast min/max.
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

/*
 * Macros for counting and rounding.
 */
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

/*
 * Definitions for asychronous kernel preemption.
 * Kernel-specific functionality, so only try
 * to include kpreempt.h if we are defining _KERNEL.
 */
#ifdef _KERNEL_BUILD
#include "../h/kpreempt.h"
#endif /* _KERNEL_BUILD */

#ifdef _UNSUPPORTED

	/* 
	 * NOTE: The following header file contains information specific
	 * to the internals of the HP-UX implementation. The contents of 
	 * this header file are subject to change without notice. Such
	 * changes may affect source code, object code, or binary
	 * compatibility between releases of HP-UX. Code which uses 
	 * the symbols contained within this header file is inherently
	 * non-portable (even between HP-UX implementations).
	*/
#ifdef _KERNEL_BUILD
#	include "../h/_param.h"
#else  /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_param.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_PARAM_INCLUDED */
