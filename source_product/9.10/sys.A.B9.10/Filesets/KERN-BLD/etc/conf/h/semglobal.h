/*
 * @(#)semglobal.h: $Revision: 1.9.83.4 $ $Date: 93/09/17 18:34:24 $
 * $Locker:  $
 */

#ifndef	_SYS_SEMGLOBAL_INCLUDED /* allows multiple inclusion */
#define	_SYS_SEMGLOBAL_INCLUDED

#ifdef _KERNEL

/*
 *   File:
 *	semglobal.h
 *
 *   Purpose:
 *	Maintains configuration of kernel semaphores and spinlocks.
 *
 *   Description:
 *	Every shared kernel data structure is protected by either a semaphore
 *	or a spinlock.  The set of data structures protected by a single
 *	semaphore (or spinlock) is defined to be a "protection class".  Every
 *	shared kernel data structure is a member of one protection class.
 *	Semaphores, in addition, also have "priority" (priority to which
 *	a process is to be promoted while possessing the semaphoe) and "order"
 *	(ordering of semaphores used in deadlock detection).  The definitions
 *	of "protection class", "priority", and "order" are maintained in this
 *	file.
 *
 */

/*** PRIORITIES ***/

#define KERNEL_SEMA_PRI		PSWP		/* kernel_sema.		   */
#define VM_SEMA_PRI		PSWP		/* all VM structures       */
#define FILESYS_SEMA_PRI	PRIBIO		/* all FS structures       */
#define IO_SEMA_PRI		PRIBIO		/* all IO structures       */
#define PROC_PRI		PSWP		/* proc table priority.    */
#define SYSV_PRI		PRIBIO		/* all SysV structs        */

/*** RESOURCE CLASSES ***/

#define KERNEL_SEMA_ORDER	200		/* kernel_sema.		   */
#define VM_SEMA_ORDER		210		/* vmsys_sema 		   */
#define FILESYS_SEMA_ORDER	220		/* filesys_sema.	   */
#define PROC_ORDER		260		/* proc structure.	   */
#define IO_SEMA_ORDER		300		/* io_sema		   */
#define SYSV_SEMA_ORDER		300		/* sysVmsg_sema, sysVsem_sema*/

/** Start of Beta-Class orders **/

#define	SHMID_LOCK_ORDER	80
#define	VNODE_LOCK_ORDER	90
#define	VAS_VA_LOCK_ORDER	100
#define	IOMAP_VAS_SEMA_ORDER	110
#define	REG_R_MLOCK_ORDER	120
#define	REG_R_LOCK_ORDER	130
#define	PFD_LOCK_ORDER		140
#define	SWAP_LOCK_ORDER		150
#define	VHAND_SEMA_ORDER	0		/* really psync? */
#define	REG_R_WAIT_ORDER	0		/* ? used ? */

/** Start of Spinlock orders **/

#define	LAST_HELD_ORDER			900	/* none grabbed after this */
#define	NO_ORDER			-1

#define HDL_LOCK_ORDER		NO_ORDER	/* 300 only */

#define	CRASH_MONARCH_LOCK_ORDER	5
#define	SPL_LOCK_ORDER			10
#define	RSWAP_LOCK_LOCK_ORDER		20
#define LOST_PAGE_LIST_LOCK_ORDER	30
#define	KMEMLOCK_ORDER			40
#define RLISTLOCK_ORDER			40
#define PLISTLOCK_ORDER			40
#define	PFDAT_HASH_LOCK_ORDER		50
#define	PFDAT_LOCK_LOCK_ORDER		60
#define	ISR_MEMLIST_LOCK_ORDER		60
#define	PDIR_LOCK_ORDER			70
#define ACTIVEPROC_LOCK_ORDER           600
#define	SCHED_LOCK_ORDER		700
#define	PROC_LOCK_ORDER			800
#define TEXT_HASH_LOCK_ORDER		SEMAPHORE_LOCK_ORDER-1
#define	RMAP_LOCK_ORDER			LAST_HELD_ORDER-1
#define	CALLOUT_LOCK_ORDER		LAST_HELD_ORDER-1
#define	FILE_TABLE_LOCK_ORDER		LAST_HELD_ORDER-1
#define	DEVVP_LOCK_ORDER		LAST_HELD_ORDER-1
#define	BIODONE_LOCK_ORDER		LAST_HELD_ORDER-1
#define	BBUSY_LOCK_ORDER		LAST_HELD_ORDER-1
#define	V_COUNT_LOCK_ORDER		LAST_HELD_ORDER-1
#define	BUF_HASH_LOCK_ORDER		LAST_HELD_ORDER-1
#define	SEMAPHORE_LOG_LOCK_ORDER	LAST_HELD_ORDER-1
#define	LOCK_INIT_LOCK_ORDER		LAST_HELD_ORDER-1
#define	SEMAPHORE_LOCK_ORDER		LAST_HELD_ORDER-1
#define	IOSERV_LOCK_ORDER		LAST_HELD_ORDER-1
#define	IO_TREE_LOCK_ORDER		LAST_HELD_ORDER-1	/* 800 only */
#define VMSYS_LOCK_ORDER		LAST_HELD_ORDER-1
#define CRED_LOCK_ORDER			LAST_HELD_ORDER-1
#define NETISR_LOCK_ORDER		LAST_HELD_ORDER-2
#define NTIMO_LOCK_ORDER		LAST_HELD_ORDER-2
#define BSDSKTS_LOCK_ORDER		LAST_HELD_ORDER-1
#define BUF_VIRT_LOCK_ORDER             LAST_HELD_ORDER-1
#define BUF_PHYS_LOCK_ORDER             LAST_HELD_ORDER-1
#define LPMC_LOG_LOCK_ORDER		LAST_HELD_ORDER
#define ITMR_SYNC_LOCK_ORDER		LAST_HELD_ORDER
#define PDCE_PROC_LOCK_ORDER		LAST_HELD_ORDER
#define PFAIL_CNTR_LOCK_ORDER		LAST_HELD_ORDER
#define CLIST_LOCK_ORDER		LAST_HELD_ORDER

/*** PROTECTION CLASSES

protection_class pdirlock =
{
	pdir;
	htbl[];			<h/vm_machdep.c>
}

protection_class swaplock =
{
	swaptab;		<sys/vm_swalloc.c>
	swapnew;
}

protection_class rlistlock =
{
	rfree;			<sys/vm_region.c>
	region[];
}

protection_class plistlock =
{
	prpfree;			<sys/vm_region.c>
	p_pregion;			<h/proc.h>
}

protection_class kernel_sema =
{
	< everything else for now >
}

***/

#endif /* _KERNEL */

#endif /* _SYS_SEMGLOBAL_INCLUDED */
