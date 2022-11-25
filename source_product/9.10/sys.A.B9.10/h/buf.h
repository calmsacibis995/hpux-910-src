/* /* $Header: buf.h,v 1.37.83.4 94/09/16 10:43:16 dkm Exp $ */

#ifndef _SYS_BUF_INCLUDED /* allows multiple inclusion */
#define _SYS_BUF_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL_BUILD
#include "../h/spinlock.h"
#else /* ! _KERNEL_BUILD */
#include <sys/spinlock.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/time.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <sys/time.h>
#endif /* _KERNEL_BUILD */
#ifdef	_LVM
	/* This is temporary until OSF RW lock routine are supported */
#ifdef _KERNEL_BUILD
#include "../h/rw_lock.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/rw_lock.h>
#endif /* _KERNEL_BUILD */
#endif	/* _LVM */

#ifdef FSD_KI
#define	B_unknown		0x0000	/* generic not known	*/
#define B_IFMT(B)	((B)&0xf)	/* v_type (see vnode.h) */

/* methods of allocating fragments as buffers */
#define B_BESTFIT		0x0000
#define	B_WORSTFIT		0x0001
#ifdef QUOTA
/* This flag is set in frag_fit and checked in alloc so that when chkdq */
/* is called, the temporary allocation is allowed even if over quota */
#define	B_DQ_FORCE		0x0002
#endif /* QUOTA */

/* buffer(bp) contains this type of data that may go to disk */
#define B_TYPE(B)	(((B)&0xf0)>>4)
#define	B_inode			0x0010 	/* inode		*/
#define	B_dir			0x0020 	/* directory		*/
#define	B_cylgrp		0x0030 	/* cylinder group info	*/
#define	B_sblock		0x0040 	/* super block		*/
#define	B_indbk			0x0050 	/* indirect block	*/
#define B_data			0x0060 	/* File data block	*/
#define B_fifo			0x0070 	/* File fifo block	*/
#define B_args			0x0080 	/* Process args		*/
#define B_swpbf			0x0090 	/* Process swap buffer	*/
#define B_phybf			0x00A0 	/* Process physical I/O	*/
#define B_pagebf		0x00B0 	/* Process page buffer  */

/* buffers are allocated by these routines */
#define B_ROUTINE(B)	(((B)&0xff00)>>8)
#define B_rds_strat		0x0100
#define B_dux_rwip		0x0200
#define B_dux_fifo_read		0x0300
#define B_dux_fifo_write	0x0400
#define B_servestratread	0x0500
#define B_servestratwrite	0x0600
#define B_rwvp			0x0700
#define B_exec_i		0x0800
#define B_realloccg		0x0900
#define B_ifree			0x0A00
#define B_bmap			0x0B00
#define B_dirmakedirect		0x0C00
#define B_blkatoff		0x0D00
#define B_iget			0x0E00
#define B_iupdat		0x0F00
#define B_itrunc		0x1000
#define B_indirtrunc		0x1100
#define B_ufs_mountroot		0x1200
#define B_mountfs		0x1300
#define B_unmount		0x1400
#define B_sbupdate		0x1500
#define B_fs_size		0x1600
#define B_rwip			0x1700
#define B_fifo_read		0x1800
#define B_fifo_write		0x1900
#define B_ufs_bread		0x1A00
#define B_free			0x1B00
#define B_dux_grow		0x1C00
#define B_unsp_rw		0x1D00
#define B_fragextend		0x1E00
#define B_alloccg		0x1F00
#define B_ialloccg		0x2000
#define B_swap			0x2100	/* Not used */
#define B_physio		0x2200
#define B_geteblk		0x2300
#define B_allocbuf		0x2400
#define B_spec_rdwr		0x2500
#define B_dux_rwcdp		0x2600
#define B_cdfs_rd		0x2700

#define B_ufs_close		0x2800
#define B_inactive		0x2900

#define B_mp_physio		0x2A00
#define B_vhand                 0x2B00
#define B_swapout              	0x2C00
#define B_devswap_pageout	0x2D00
#define B_devswap_pagein 	0x2E00
#define B_vfs_pageout		0x2F00
#define B_vfs_pagein		0x3000
#define B_vfdswapo		0x3100
#define B_vfdswapi		0x3200
#define B_vfs_alloc_hole	0x3300
#endif /* FSD_KI */

#define ASSERT_BC_LOCKED  T_SPINLOCK(buf_hash_lock)
#define ASSERT_BC_UNLOCKED TBAR_SPINLOCK(buf_hash_lock)
#define	BC_LOCK	spinlock(buf_hash_lock)
#define	BC_UNLOCK spinunlock(buf_hash_lock)
#define	BC_SLEEP_UNLOCK	\
	{spinlock(sched_lock); BC_UNLOCK;}

/*
 * The header for buffers in the buffer pool and otherwise used
 * to describe a block i/o request is given here.  The routines
 * which manipulate these things are given in vfs_bio.c.
 *
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * hashed into a chain by <dev,blkno> so it can be located in the cache,
 * and (usually) on (one of several) queues.  These lists are circular and
 * doubly linked for easy removal.
 *
 * For the dynamic buffer cache, there is also a singularly linked list
 * of all buffer headers.  Buffer headers can be dynamically added and
 * deleted from the pool, and this list is used to find all buffers in the
 * pool.
 *
 * There are currently three queues for buffers:
 *	one for buffers which must be kept permanently (super blocks).  This
 *              is not currently used by anyone.
 * 	one for buffers containing ``useful'' information (the cache)
 *	one for buffers containing ``non-useful'' information
 *		(and empty buffers, pushed onto the front)
 *
 * The latter two queues contain the buffers which are available for
 * reallocation, are kept in lru order.  When not on one of these queues,
 * the buffers are ``checked out'' to drivers which use the available list
 * pointers to keep track of them in their i/o active queues.
 */
struct buf
{
	long	b_flags;		/* too much goes here to describe */
	struct	buf *b_forw, *b_back;	/* hash chain (2 way street) */
	struct	buf *av_forw, *av_back;	/* position on free list if not BUSY */
#ifdef _WSIO
	struct  buf *b_blockf, **b_blockb;/* associated vnode */
#endif
#define	b_actf	av_forw			/* alternate names for driver queue */
#define	b_actl	av_back			/*    head - isn't history wonderful */
	long	b_bcount;		/* transfer count */
	long	b_bufsize;		/* size of allocated buffer */
#ifdef __hp9000s800
#define b_last_front	b_bufsize	/* used only in header headers */
#endif /* __hp9000s800 */
#define	b_active b_bcount		/* driver queue head: drive active */
	short	b_error;		/* returned after I/O */
#ifdef FSD_KI
	u_short	b_queuelen;		/* queue length on disc (+long align bp)*/
#endif /* FSD_KI */
	dev_t	b_dev;			/* major+minor device name */
	union {
	    caddr_t b_addr;		/* low order core address */
					/* (space in b_spaddr) */
	    int	*b_words;		/* words for clearing */
	    struct fs *b_filsys;	/* superblocks */
#define	b_fs	b_filsys
	    struct csum *b_cs;		/* superblock summary information */
	    struct cg *b_cg;		/* cylinder group block */
	    struct dinode *b_dino;	/* ilist */
	    daddr_t *b_daddr;		/* indirect block */
	} b_un;

#ifndef _WSIO
	long	b_resid;		/* words not transferred after error */
#endif /* !_WSIO */

#ifdef	__hp9000s300
#define	paddr(X)	(paddr_t)(X->b_un.b_addr)
#endif /* __hp9000s300 */

#ifdef _WSIO
	struct	isc_table_type *b_sc;	/* select code pointer */
#endif /* _WSIO */

	int	(*b_action)();		/* only WSIO:next activity to perform */

#ifdef _WSIO
	int	(*b_action2)();		/* (see HPIB_utility for use) */
	int	b_clock_ticks;		/* (see HPIB_utility for use) */
	struct	iobuf *b_queue;		/* whatever IO queue its on */

	long	b_s2;			/* scratch area */
	char	b_s0;			/* scratch area */
	char	b_s1;			/* scratch area */
	char	b_s3;			/* scratch area */
	char	b_ba;			/* bus address */
	char	b_s4;			/* scratch area */
	char	b_s5;			/* scratch area */
	short	b_s6;			/* scratch area */
#ifdef	REQ_MERGING
	struct buf *mio_next;		/* Merged I/O requests queue */
	struct buf *mio_last;
	int	b_s9;			/* driver scratch area */
#else
	int	b_s7;			/* driver scratch area */
	int	b_s8;			/* driver scratch area */
	void	*b_s9;			/* driver scratch area */
#endif
#endif /* _WSIO */

	union {				/* only for _WSIO */
		daddr_t b_sectno;	/* sector number on device */
		daddr_t b_byteno;	/* byte number on device (raw only) */
	} b_un2;

#ifdef _WSIO
	unsigned int b_resid;		/* words not transferred after error */
	daddr_t	b_offset;		/* byte offset on device */
#endif /* _WSIO */

#define	b_errcnt b_resid		/* while i/o in progress: # retries */
	daddr_t	b_blkno;		/* block # on device */
#ifndef _WSIO
	struct timeval b_start;		/* request start time	*/
#endif /* not _WSIO */
	int	*b_pcnt;		/* decr in biodone(), wakeup on < 0 */
	struct  proc *b_proc;		/* proc doing physical or swap I/O */
	int	(*b_iodone)();		/* function called by iodone */
	struct	vnode *b_vp;		/* vnode associated with block */
	struct	region	*b_rp;		/* region associated with block */
	int	b_pfcent;		/* center page when swapping cluster */
	space_t	b_spaddr;		/* space of b_un.b_addr */
	int	b_prio;			/* priority of process initiating */
#ifndef _WSIO
#define b_front_cnt b_pfcent		/* used only in header headers */
	daddr_t	b_offset;		/* byte offset on device */
					/* also used as scratch */
	long	b_s2;			/* scratch area */
#endif /* !_WSIO */
	int access_cnt;			/* hits since in buffer cache */
	struct timeval tv;		/* HP REVISIT.  not used on WSIO */
#ifdef _WSIO
	struct bf *which_list;		/* which priority partition */
#else  /* _WSIO */
	int which_list;			/* on LRU, AGE, or VIRT? */
#endif /* _WSIO */
	int b_wanted;			/* MP-safe wanted flag */
#ifdef FSD_KI
	struct	ki_timeval	b_timeval_eq;	/* request enqueue time */
	struct	ki_timeval	b_timeval_qs;	/* request queuestart time */
	u_short b_bptype;		/* buffer type */
	site_t	b_site;			/* site(cnode) that allocated this bp */
	pid_t	b_upid;			/* pid that last used this bp */
	pid_t	b_apid;			/* pid that last allocated this bp */
#endif /* FSD_KI */
#ifdef	NSYNC
					/* fields used by modified syncer */
	long	io_tv;			/* latest block access time (in sec.) */
	long	sync_time;		/* latest syncer active time */
#endif	/* NSYNC */
	u_short b_flags2;		/* more flags */
#ifdef _WSIO
	u_short b_ord_refcnt;		/* count of ordered write pointers */
	struct buf *b_ord_infra;	/* writes to schedule from biodone() */
	struct buf *b_ord_inter;	/* writes to schedule from biodone() */
	struct proc *b_ord_proc;	/* process of current transaction */
	struct buf *b_nexthdr;		/* linked list of all buffer headers */
	u_int       b_timestamp;	/* used to "age" and "steal" buffers */
	u_int b_enq_ticks;		/* boot ticks at enqueue time */
#endif /* _WSIO */
	long    b_virtsize;             /* size of allocated virtual space. */
#ifdef _LVM
	union	{	/* these fields reserved _solely_ for device driver */
		long	longvalue;
		void	*pointvalue;
	} b_driver_un_1, b_driver_un_2;

#ifdef	OSDEBUG
	int	b_lvmdb;		/* for LVM debug only */
#endif
	rw_lock_data_t	b_lock;
#endif /* _LVM */
};

#ifdef __hp9000s800
/* Dux flags */
#define B2_DUXCANTFREE 	0x10
#define B2_DUXFLAGS	B2_DUXCANTFREE
#endif

#ifdef _WSIO
#define	BQUEUES		2		/* number of free buffer queues */
					/* types of queues follow */
#define	BQ_AGE		0		/* lru, useful buffers */
#define	BQ_EMPTY	1		/* rubbish */
struct bf {
        struct buf      *bf_bp;
        struct bf       *bf_next;       /* Circularly linked */
};

#define bufqhead buf
extern struct bufqhead bfreelist[BQUEUES];      /* heads of available lists */

/* Ordered write transaction types: */
#define B2_LINKDATA	0x01
#define B2_UNLINKDATA	0x02
#define B2_LINKMETA	0x04
#define B2_UNLINKMETA	0x08
#define B2_ORDMASK 	(B2_LINKDATA|B2_UNLINKDATA|B2_LINKMETA|B2_UNLINKMETA)

#else
#define	BQUEUES		5		/* number of free buffer queues */
					/* types of queues follow */
#define	BQ_LOCKED	0		/* super-blocks &c */
#define	BQ_LRU		1		/* lru, useful buffers */
#define	BQ_AGE		2		/* rubbish */
#define	BQ_EMPTY	3		/* buffer headers with no memory */
#define BQ_VIRT		4		/* header (only) remembers dev,blkno */

extern struct buf bfreelist[BQUEUES];   /* heads of available lists */
extern u_int pref_size, virt_size;
extern int bfree_cnts[BQUEUES];
#endif /* !_WSIO */

/*
 * These flags are kept in b_flags.
 * They are in numerical order to show the gaps (if any).
 * First come the flags common to both platforms, followed
 * by the platform-dependent flags.
 */
/* B_HWRELOC is 0 for now to avoid driver changes */
#define	B_HWRELOC	0x00000000	/* relocate/rewrite block - LVM */
#define	B_WRITE		0x00000000	/* non-read pseudo-flag */

#define	B_READ		0x00000001	/* read when I/O occurs */
#define	B_DONE		0x00000002	/* transaction finished */
#define	B_ERROR		0x00000004	/* transaction aborted */
#define	B_BUSY		0x00000008	/* not on av_forw/back list */

#define	B_PHYS		0x00000010	/* physical IO */
#define B_END_OF_DATA   0x00000020      /* let physio know transfer is done */
#define	B_WANTED	0x00000040	/* issue wakeup when BUSY goes off */
#define	B_NDELAY	0x00000080	/* don't retry on failures */

#define	B_ASYNC		0x00000100	/* don't wait for I/O completion */
#define	B_DELWRI	0x00000200	/* write at exit of avail list */
#define	B_ORDWRI	0x00000400	/* flags ordered asynchronous writes */
#define B_REWRITE       0x00000800      /* re-writing buffer - call bdwrite */

#define B_PRIVATE	0x00001000	/* private, not part of buffers - LVM */
#define	B_WRITEV	0x00002000	/* verification of writes  - LVM */
#define	B_PFTIMEOUT	0x00004000	/* power failure time out - LVM */
#define	B_CACHE		0x00008000	/* did bread find us in the cache ? */

#define	B_INVAL		0x00010000	/* does not contain valid info  */
#define B_FSYSIO	0x00020000	/* buffer from b{read,write}	*/
#define	B_CALL		0x00040000	/* call b_iodone from iodone */
#define	B_NOCACHE	0x00080000	/* don't cache block when released */

#define B_RAW		0x00100000	/* raw interface (LVM/disc3/autoch) */
#define B_NETBUF	0x00200000	/* buffer in network pool */
#define B_DUX_REM_REQ	0x00400000	/* Buffer is for a remote dux reqest */
#define B_SYNC		0x00800000	/* buffer write is synchronous */

/*
 * Platform-dependent flags
 *	These are I/O flags (WS) and DataPair flags (800)
 */
#ifdef _WSIO
#define	B_PAGEOUT	0x01000000	/* a buffer header, not a buffer */
#define	B_DIL		0x02000000	/* mark it as a DIL buffer */
#define	B_SCRACH1	0x04000000	/* scratch flags for drivers use */
#define	B_SCRACH2	0x08000000	/* scratch flags for drivers use */

#define	B_SCRACH3	0x10000000	/* scratch flags for drivers use */
#define	B_SCRACH4	0x20000000	/* scratch flags for drivers use */
#define	B_SCRACH5	0x40000000	/* scratch flags for drivers use */
#define	B_SCRACH6	0x80000000	/* scratch flags for drivers use */
#ifdef REQ_MERGING
#define B_MERGE_IO B_SCRACH6
#endif
#endif /* _WSIO */

#ifndef _WSIO
#define	B_HEAD		0x08000000	/* a buffer header, not a buffer */
#define B_LOCAL		0x10000000	/* DataPair */
#define B_REIMAGE_WRITE	0x20000000	/* DataPair */
#define B_MIRIODONE	0x40000000	/* DataPair */
#define B_MIRCALL	0x80000000	/* DataPair */
#endif /* !_WSIO */

/*
 * Disksort steals some of the buffer members and bits for use in
 * the disk queue headers, which are never used for I/O and are never
 * owned by the buffer cache.
 */
#define b_cylin         b_resid         /* cylinder number, for sorting */
#define b_run_count     b_pfcent        /* length of current run */
#define B_DSORTDQ       B_NDELAY        /* driver uses our dequeue() */
/*

 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
struct bufhd
{
	long	b_flags;		/* see defines below */
	struct	buf *b_forw, *b_back;	/* fwd/bkwd pointer in chain */
	int	b_checkdup;		/* modification "time" stamp */
};

#ifdef _KERNEL
extern	int BUFHSZ;
extern 	int BUFMASK;			/* must be a power of 2 - 1 */

#define RND	(MAXBSIZE/DEV_BSIZE)

#define BC_DEFAULT      0x1	/* Use default seraching. */
#define BC_PHYSICAL     0x2	/* Only return buffers with physical space. */

#define	BUFHASH(dvp, dblkno)	\
 	((struct buf *)&bufhash[((u_int)(dvp)+(((int)(dblkno))/RND))&(BUFMASK)])

#ifdef _WSIO
extern  struct  buf *dbc_hdr;   /* pointer to first buffer header in pool */
extern  int     dbc_nbuf; 	/* minimum number of buffer headers. */
extern  int     dbc_bufpages;   /* minimum number of memory pages in the */
				/* buffer pool */
extern  u_int	dbc_stealavg;	/* Average number of pages stolen from cache */
extern  u_int   dbc_vhandcredit;/* Number of pages voluntarily given up */
#else /* _WSIO */
extern	struct	buf *buf;	/* the buffer pool itself */
extern	char	*buffers;
extern	struct	buf *bclnlist;	/* head of cleaned page list */
#endif /* _WSIO */
extern	int	nbuf;		/* actual number of buffer headers */
extern	int	bufpages;	/* number of memory pages in the buffer pool */
extern  int     orignbuf;	/* number of buffer headers requested */
extern  int     origbufpages;   /* number of memory pages requested for */
				/* buffer pool.  */
extern	struct	buf *swbuf;	/* swap I/O headers */
extern	int	nswbuf;
extern	struct	bufhd *bufhash;	/* heads of hash lists */
extern	struct	buf bswlist;		/* head of free swap header list */

#ifdef __hp9000s800
extern	struct	buf *pageoutbp;
#endif /* __hp9000s800 */


struct	buf *alloc();
struct	buf *realloccg();
struct	buf *baddr();
struct	buf *getblk();
struct	buf *geteblk();
struct	buf *bread();
struct	buf *breada();
struct	vnode *devtovp();

#ifndef _WSIO
/* getnewbuf() with BC_DEFAULT grabs empty buffers first, while BC_PHYSICAL
 * starts with AGED and LRU buffer.  See getnewbuf2().
 */
struct	buf *getnewbuf2();
#define getnewbuf(a)    getnewbuf2(a, BC_DEFAULT)
#else /* _WSIO */
struct  buf *getnewbuf();
#endif /* _WSIO */

extern	space_t bvtospace();

unsigned minphys();

#ifdef _WSIO
#define bclearprio(bp) ((bp)->b_prio = NZERO)
#define bsetprio(bp) ((bp)->b_prio = (u.u_procp->p_flag&SRTPROC) ? \
					0 : u.u_procp->p_nice)
#else /* _WSIO */
#define bclearprio(bp) ((bp)->b_prio = RTPRIO_MAX+1)
#define bsetprio(bp) ((bp)->b_prio = (u.u_procp->p_flag&SRTPROC) ? \
					u.u_procp->p_rtpri : RTPRIO_MAX+1)
#endif /* _WSIO */

#endif /* _KERNEL */

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	bremhash(bp) { \
	(bp)->b_back->b_forw = (bp)->b_forw; \
	(bp)->b_forw->b_back = (bp)->b_back; \
}
#define	binshash(bp, dp) { \
	(bp)->b_forw = (dp)->b_forw; \
	(bp)->b_back = (dp); \
	(dp)->b_forw->b_back = (bp); \
	(dp)->b_forw = (bp); \
}

#ifndef _WSIO
/*
 * Insq/Remq for the buffer free lists.
 */
#define	bremfree(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
	(bp)->av_forw = (bp)->av_back = 0; \
	if (((bp)->which_list == BQ_LRU) || ((bp)->which_list == BQ_AGE)) \
		bfree_cnts[(bp)->which_list] -= (bp)->b_bufsize; \
	else \
		bfree_cnts[(bp)->which_list]--; \
}

#define	binsheadfree(bp, list) { \
	(&bfreelist[list])->av_forw->av_back = (bp); \
	(bp)->av_forw = (&bfreelist[list])->av_forw; \
	(&bfreelist[list])->av_forw = (bp); \
	(bp)->av_back = (&bfreelist[list]); \
	if ((list == BQ_LRU) || (list == BQ_AGE)) \
		bfree_cnts[list] += (bp)->b_bufsize; \
	else \
		bfree_cnts[list]++; \
	(bp)->which_list = list; \
}

#define binstailfree(bp, list) { \
	(&bfreelist[list])->av_back->av_forw = (bp); \
	(bp)->av_back = (&bfreelist[list])->av_back; \
	(&bfreelist[list])->av_back = (bp); \
	(bp)->av_forw = (&bfreelist[list]); \
	if ((list == BQ_LRU) || (list == BQ_AGE)) \
		bfree_cnts[list] += (bp)->b_bufsize; \
	else \
		bfree_cnts[list]++; \
	(bp)->which_list = list; \
}
#endif /* ! _WSIO */

/*
 * Take a buffer off the free list it's on and
 * mark it as being use (B_BUSY) by a device.
 */
#ifdef _WSIO
#define notavail(bp) { \
	bremfree(bp); \
	(bp)->b_flags |= B_BUSY; \
}
#else /* _WSIO */
#define	notavail(bp) { \
	ASSERT_BC_LOCKED; \
	bremfree(bp); \
	(bp)->b_flags |= B_BUSY; \
}
#endif /* _WSIO */

#define	iodone	biodone
#define	iowait	biowait

/*
 * Zero out a buffer's data portion.
 */

#ifdef __hp9000s800
#define	clrbuf(bp) { \
	bzero((caddr_t) bp->b_un.b_addr, (unsigned) bp->b_bcount); \
	bp->b_resid = 0; \
}
#endif  /* __hp9000s800) */

#ifdef __hp9000s300
#define	clrbuf(bp) { \
	blkclr(bp->b_un.b_addr, bp->b_bcount); \
	bp->b_resid = 0; \
}
#endif	/* __hp9000s300 */

#ifdef _KERNEL
extern int fs_async;
#ifndef _WSIO
#define BXWRITE(BP) { \
	if (fs_async) \
		bdwrite(BP); \
	else { \
		(BP)->b_flags |= B_SYNC; \
		bwrite(BP); \
	} \
}
#endif /* ! _WSIO */
#endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_BUF_INCLUDED */
