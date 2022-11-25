/*
 * @(#)swap.h: $Revision: 1.8.83.6 $ $Date: 93/09/17 18:35:35 $
 * $Locker:  $
 */
#ifndef _SWAP_H
#define _SWAP_H

#ifdef _KERNEL_BUILD
#include "../h/sema.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sema.h>
#endif /* _KERNEL_BUILD */

int fs_swap_debug;

/*	The following structure contains the data describing a
 *	swap file.
 */


typedef struct swapmap {
	ushort	sm_ucnt;	/* number of users on this page	*/
	short	sm_next;	/* index of free swapmap[]	*/
} swpm_t;

typedef struct swaptab {
	short	st_free;	  /* index of 1st free swapmap[]*/
	short	st_next;	  /* index of next chunk for 	*/
				  /* same dev or fs		*/
	int	st_flags;	  /* flags defined below.	*/
	struct swdevt *st_dev;	  /* swap device.		*/
	struct fswdevt *st_fsp;	  /* swap file system.		*/
	struct vnode *st_vnode;	  /* dev or fs vnode		*/
				  /* system chunk		*/
	int	st_nfpgs;	  /* nbr of free pages on device*/
	struct swapmap *st_swpmp; /* ptr to swapmap[] array.	*/
	int 	st_site;	  /* site number (DUX)		*/
	union {
	int	st_start;	  /* starting addr on S300	*/
	int	st_swptab;	  /* server swaptab[] index	*/
	} st_union;
} swpt_t;

typedef struct fswdevt{
        struct fswdevt *fsw_next;	/* next fs w/ same pri	*/
        int fsw_enable;			/* enabled		*/
        int fsw_nfpgs;			/* # free pages		*/
        int fsw_allocated;		/* # of blocks allocated*/
        uint fsw_min;			/* min # preallocated	*/
        uint fsw_limit;			/* max # to allocate	*/
        uint fsw_reserve;		/* # to reserve		*/
        int fsw_priority;		/* priority		*/
        struct vnode *fsw_vnode;	/* file system vnode	*/
	short  fsw_head;		/* 1st swaptab[] entry	*/
	short  fsw_tail;		/* last swaptab[] entry	*/
        char fsw_mntpoint[256];		/* file system mount pt.*/
} fswdev_t;	

typedef struct devpri{
	struct swdevt *first;	/* first fs for a priority	*/
	struct swdevt *curr;	/* allocate from this fs first	*/
} devpri_t;

typedef struct fspri{
	struct fswdevt *first;	/* first fs for a priority	*/
	struct fswdevt *curr;	/* allocate from this fs first	*/
} fspri_t;


/*
 * This is an overlay structure for a regular dbd.
 * It MUST be the same size as a dbd.
 */
typedef struct swpdbd {
	uint dbd_type:4,
	     dbd_swptb:14,
	     dbd_swpmp:14;
} swpdbd_t;

/*
 * Info structure for swapfs system call.
 */
struct swapfs_info {
	int sw_priority;	/* Priority of this swap space */
	long sw_binuse;		/* Blocks used by swapping */
	long sw_bavail;		/* Blocks available for swapping */
	long sw_breserve;	/* Blocks reserved for filesystem */
	char sw_mntpoint[256];	/* argument to swapon() */
};

#define	NPGCHUNK ((int)(dtop(swchunk))) 	/* # pages per chunk	*/
#define NSWPRI 11		/* max # of priorities	*/
#define SYSMEMMAX 512		/* max pages of memory not "swap" */

#define ST_FREE         0x02    /* free the clients swapchunk   */
#define	ST_INDEL	0x01	/* This file is in the process 	*/
				/* of being deleted.  Don't	*/
				/* allocate from it.		*/
#ifdef _KERNEL
#define SWAP_NOWAIT     0x01
#define SWAP_NOSLEEP    0x02
#endif

#define swaplock()	vm_psema(&swap_lock, PZERO)
#define swapunlock()	vm_vsema(&swap_lock, 0)
#define rswaplock()	vm_spinlock(rswap_lock)
#define rswapunlock()	vm_spinunlock(rswap_lock)

struct buf *bswalloc();

/* change for uxgen GHAYS */
/*extern fswdev_t *fswdevt;
extern swpt_t *swaptab; */

extern fswdev_t fswdevt[];
extern swpt_t swaptab[];
extern devpri_t swdev_pri[];
extern fspri_t swfs_pri[];

extern nswapfs;
extern nswapdev;
extern swchunk;
extern maxswapchunks;
extern swapmem_cnt;
extern swapspc_cnt;
extern maxfs_pri;
extern maxdev_pri;
extern struct vnode *swapdev_vp;
extern struct swaptab *swapMAXSWAPTAB;
extern vm_sema_t swap_lock;      /* Lock for all swap entries   */
extern vm_lock_t rswap_lock;      /* Lock for reserveing swap */
extern int	swapwant; 	 /* Set non-zero if someone is	*/
				 /* waiting for swap space.	*/

extern swapphys_cnt;
extern vm_lock_t pswap_lock;    /* Lock for physical swap */

/*	The following struct is used by the sys3b system call.
 *	If the first argument to the sys3b system call is 3,
 *	then the call pertains to the swap file.  In this case,
 *	the second argument is a pointer to a structure of the
 *	following format which contains the parameters for the
 *	operation to be performed.
 */

typedef struct swapint {
	char	si_cmd;		/* One of the command codes	*/
				/* listed below.		*/
	char	*si_buf;	/* For an SI_LIST function, this*/
				/* is a pointer to a buffer of	*/
				/* sizeof(swpt_t)*NSWAPDEV bytes.*/
				/* For the other cases, it is a	*/
				/* pointer to a pathname of a	*/
				/* swap file.			*/
	int	si_swplo;	/* The first block number of the*/
				/* swap file.  Used only for	*/
				/* SI_ADD and SI_DEL.		*/
	int	si_nblks;	/* The size of the swap file in	*/
				/* blocks.  Used only for an	*/
				/* SI_ADD request.		*/
} swpi_t;

/*	The following are the possible values for si_cmd.
 */

#define	SI_LIST		0	/* List the currently active	*/
				/* swap files.			*/
#define	SI_ADD		1	/* Add a new swap file.		*/
#define	SI_DEL		2	/* Delete one of the currently	*/
				/* active swap files.		*/
struct swap_stats {
	dev_t	 device;	/* major/minor number of device */
	unsigned type;		/* type of swap device (see below) */
	unsigned pages_in;	/* number of pages swapped in on this device */
	unsigned pages_out;	/* number of pages swapped out on this device */
} ;

#define SWTYPE_DEV	0x1	/* raw disk swap dev */
#define SWTYPE_FS	0x2	/* file system swap device */
#define SWTYPE_LAN	0x4	/* diskless (lan) swap device */

#endif /* _SWAP_H */
