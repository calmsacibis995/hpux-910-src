/*
 * @@(#)region.h: $Revision: 1.11.83.4 $ $Date: 93/09/17 18:33:16 $
 * $Locker:  $
 */

#ifndef _REGION_INCLUDED
#define _REGION_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/sema.h"
#include "../h/vfd.h"
#include "../h/dbd.h"
#include "../machine/hdl_region.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sema.h>
#include <sys/vfd.h>
#include <sys/dbd.h>
#include <machine/hdl_region.h>
#endif /* _KERNEL_BUILD */

/*
 * Per region descriptor.  One is allocated for
 * every active region in the system.  Beware if you add
 * data elements here:  Dupreg may need to copy them.
 */

typedef	struct	region	{
	ushort	r_flags;
	ushort	r_type;		/* type of region */
	size_t	r_pgsz;		/* size in pages */
	size_t	r_nvalid;	/* number of valid pages in region */
	size_t	r_swnvalid;	/* resident set size of swapped region */
				/* (r_nvalid value when region swapped) */
	size_t	r_swalloc;	/* for RF_SWLAZY, # pgs actually allocated */
	ushort	r_refcnt;	/* number of users pointing at region */
	size_t	r_off;		/* offset into vnode (page aligned) */
	ushort	r_incore;	/* number of users pointing at region */
	short	r_mlockcnt;	/* number of processes that locked this */
				/* region in memory. */
	int	r_dbd;		/* dbd for vfd's when swapped */
	struct vnode *r_fstore;	/* pointer to vnode where blocks come from */
	struct vnode *r_bstore;	/* pointer to vnode where blocks go */
	struct region  *r_forw;	/* links for list of all regions */
	struct region  *r_back;
	short	r_zomb;		/* set by xinval to indicate text bad */
	struct region		/* hash for region */
		*r_hchain;				
	union {
	    struct old_aout {
		u_int r_ubyte;	  /* byte off in fstore (for old a.out) */
		u_int r_ubytelen; /* byte len in fstore (for old a.out) */
	    } r_byt;
	    struct mmf {
		struct ucred *r_ummfcred; /* credentials for MMF */
		u_long r_filler1;	  /* unused */
	    } r_mmf;
	} r_un;
	vm_sema_t r_lock;	/* region lock */
	vm_sema_t r_mlock;	/* wait for region to be locked in memory */
	int  r_poip;            /* number of page I/Os in progress 
                                 *
                                 *  NOTE: must hold the region lock and the
                                 *  sleep_lock to increment the r_poip
                                 *  field (start an I/O). Must hold
                                 *  the sleep_lock to decrement.   
                                 */	
	struct broot		/* Root of btree of vfd/dbd's */
		*r_root;
	unsigned long r_key;	/* Each region contains chunk and one key */
	chunk_t *r_chunk;
	struct region *r_next;	/* links for regions sharing pages */
	struct region *r_prev;
	struct pregion *r_pregs;/* list of pregions pointing to this region */
	struct hdlregion	/* HDL fields in region */
		r_hdl;
} reg_t;

#define r_byte		r_un.r_byt.r_ubyte
#define r_bytelen	r_un.r_byt.r_ubytelen
#define r_mmfcred	r_un.r_mmf.r_ummfcred;

/*
 * Region flags
 */
#define	RF_NOFREE	0x0001	/* Don't free region on last detach */
#define RF_ALLOC	0x0004	/* region is not on free list */
#define	RF_MLOCKING	0x0008	/* set when locking region in memory */
				/* wake up processes waiting on r_mlock */
				/* when resetting this flag. */
#define RF_ZOMB         0x0010  /* set in xinval when a text turns bad */
#define RF_UNALIGNED	0x0020	/* Region is an unaligned view of vnode */
				/* (support old a.out) */
#define RF_SWLAZY	0x0040	/* Don't allocate all swap space up front */
#define RF_WANTLOCK	0x0080	/* someone else wants to lock this reg, */
				/* so wakeup(rp) them. CHANGE FOR MP*/
#define RF_HASHED	0x0100  /* region is hashed (fstore, byte) */
#define RF_EVERSWP	0x0200  /* set if region has ever been swapped */
#define RF_NOWSWP	0x0400  /* set if region is now swapped */
#define RF_DAEMON	0x0800  /* set if region is for a kernel daemon */
#define RF_UNMAP	0x1000	/* MMF region is being unmapped */
#define RF_IOMAP	0x2000	/* region is an iomap(7) region */

/*
 * Logical index from region offset to vnode offset in bytes.
 */
#define vnodindx(RP, PGINDX) (ptob(PGINDX + (RP)->r_off))

/*
 * Region types
 */
#define	RT_UNUSED	0	/* Region not being used.	*/
#define	RT_PRIVATE	1	/* Private (non-shared) region. */
#define RT_SHARED	2	/* Shared region */

#ifdef _KERNEL


extern vm_lock_t rlistlock;	/* region list lock */
extern reg_t	region[];	/* Global array of regions */
extern reg_t	regactive;	/* List of active regions */

void		reginit();	/* Initialize the region table. */
void		hdl_reginit();	/*  HDL initialization */
reg_t		*allocreg();	/* region allocator */
void		freereg();	/* region free routine */
reg_t		*dupreg();	/* Duplicate region (fork). */
int		growreg();	/* Grow region. */
int		loadreg();	/* Load region from file. */
int		mapreg();	/* Map region to 413 file. */



reg_t		*vnodereg();	/* return the region associated with vnode */
#endif /* _KERNEL */

/*	The page table entries are followed by a list of disk block
 *	descriptors which give the location on disk where a
 *	copy of the corresponding page is found.
 */

#define	reglock(RP)	{ \
				if (!(ON_ISTACK)) { \
					VASSERT(u.u_procp->p_reglocks<255); \
					u.u_procp->p_reglocks++; \
				} \
				vm_psema(&(RP)->r_lock, PZERO); \
			}

#define	creglock(RP)	( \
				vm_cpsema(&(RP)->r_lock) \
				? \
				((!(ON_ISTACK))?(u.u_procp->p_reglocks++,1):1)\
				: \
				0 \
			)

#define	regrele(RP)	{ \
				VASSERT(vm_valusema(&(RP)->r_lock)<=0); \
				vm_vsema(&(RP)->r_lock, 0); \
				if (!(ON_ISTACK)) { \
					VASSERT(u.u_procp->p_reglocks>0); \
					u.u_procp->p_reglocks--; \
				} \
			}

# define	rlstlock(context)	SPINLOCK_USAV(rlistlock, context)
# define	rlstunlock(context)				\
			{ VASSERT(vm_valulock(rlistlock) <= 0); \
			SPINUNLOCK_USAV(rlistlock, context); }
#ifdef	LATER
# define	rlstlock(context)	{u.u_procp->p_flag |= SLOCK; \
				 SPINLOCK_USAV(rlistlock, context);}
# define	rlstunlock(context)	{VASSERT(vm_valulock(rlistlock) <= 0); \
				 SPINUNLOCK_USAV(rlistlock, context); \
				 u.u_procp->p_flag &= ~SLOCK;}
#endif	/* LATER */

#endif /* _REGION_INCLUDED */
