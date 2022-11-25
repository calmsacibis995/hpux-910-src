/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_swalloc.c,v $
 * $Revision: 1.13.83.6 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/08 12:49:54 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/vm.h"
#include "../h/sysmacros.h"
#include "../h/sysinfo.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/conf.h"
#include "../h/var.h"
#include "../h/buf.h"
#include "../h/pfdat.h"
#include "../h/vfd.h"
#include "../h/sema.h"
#include "../h/vas.h"
#include "../h/proc.h"
#include "../h/swap.h"
#include "../h/systm.h"
#include "../h/debug.h"
#include "../h/vnode.h"
#include "../h/malloc.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../dux/cct.h"

#define DPPSHFT (PGSHIFT-DEV_BSHIFT)

int res_swap_wait = 0;
int res_mem_wait = 0;
int swapspc_max;
extern u_int my_site_status;
extern site_t my_site;
extern int swapmem_on;
extern int swapmem_max;
extern int sys_mem;
extern int sysmem_max;
extern int mainentered;
extern int minswapchunks;
struct vnode *allocate_fs_space();
struct swaptab *swapMAXSWAPTAB;
extern int noswap;

vm_lock_t	rswap_lock;	/* waiters for swap space */

#ifdef NEVER_USED
vm_sema_t	swapwait;	/* waiters for swap space */

proc_t		*swapwold;	/* head of "waiting for swap" list */
proc_t		*swapwnew = 0;	/* tail of "waiting for swap" list */
#endif /* NEVER_USED */

#ifdef OSDEBUG
check_swpmp(st, index)
struct swaptab *st;
int index;
{
        int i;

        for(i = st->st_free; i != -1; i = st->st_swpmp[i].sm_next) {
                VASSERT(i != index);
        }
}
#endif /* OSDEBUG */

int
get_swapspace(size, swappg)
int size;
swpdbd_t *swappg;
{
	swpt_t *st;

	swaplock();
	if (my_site_status & CCT_SLWS) {
		/*
                 * try an get swap space
                 */
			
                while(!get_swchunk(swdevt[0].sw_head,size,swappg,0)) {
			swapunlock();

			/* 
			 * send a request to server
			 */
			VASSERT(size != 1);
			if (dux_swaprequest()) {
				swaplock();
			}
			else
				return(0);
		}
		st = &swaptab[swappg->dbd_swptb];
		st->st_dev->sw_nfpgs -= size;
			
	}
	else {
		if (!get_swap(size, swappg, 0)) {
			swapunlock();
			VASSERT(size != 1);
			return(0);
		}
		else {
			/*
	                 * decrement free page count
       	                 */

			st = &swaptab[swappg->dbd_swptb];
                       	if (st->st_dev)
                       		st->st_dev->sw_nfpgs -= size;
               		else
                               	st->st_fsp->fsw_nfpgs -= size;
		}

	}
	VASSERT(vm_valusema(&swap_lock) <= 0);
	return(1);
}

/*	Allocate swap file space.
 */
swalloc(rp, idx, size)
reg_t	*rp;		/* Region containing pages */
int	idx;		/* starting page number */
int	size;		/* Number of pages of swap needed.    */
{
	register int	i;
	register short	smpi;
	register swpdbd_t	*dbd;
	swpt_t *st;
	swpdbd_t swappg;

	VASSERT(vm_valusema(&rp->r_lock) <= 0);


	/*      
	 * Get the swap space from the highest priority device
         * or file system.
	 *	
	 * get_swapspace() returns true if able to allocate swap space.
	 */
	VASSERT(size <=  NPGCHUNK);

	if (!get_swapspace(size, &swappg))
		return(0);

	/*
	 * swappg.dbd_swptb contains the index into swaptab[]
 	 * swappg.dbd_dwpmp contains the index into swapmap[]
 	 */

	st = &swaptab[swappg.dbd_swptb];
	smpi = swappg.dbd_swpmp;

	/*
	 * Initialize the swap use counts for each page
	 * and set up the disk block descriptors (dbd's).
	 */

	for(i = 0;  i < size;  i++) {
		dbd = (swpdbd_t *)FINDDBD(rp, idx + i);
		VASSERT(st->st_swpmp[smpi+i].sm_ucnt == 0);
#ifdef OSDEBUG
		check_swpmp(st, smpi+i);
#endif
		st->st_swpmp[smpi+i].sm_ucnt = 1;
		dbd->dbd_type = DBD_BSTORE;
		dbd->dbd_swptb = swappg.dbd_swptb;
		dbd->dbd_swpmp = smpi + i;
	}

	st->st_site = my_site;
	st->st_nfpgs -= size;
	st->st_flags &= ~ST_FREE;
	VASSERT(st->st_nfpgs >= 0);

	swapunlock();
	return(1);
}

/*
 *	Return true if calling swfree1() would actually free the swapmap 
 *	entry dbd.  These are freed by swfree1() when their use count 
 *	goes to zero.
 */
int swfree_willfree(dbd)
	swpdbd_t *dbd;  /* Ptr to disk block descriptor */
{
	short smpi;
	swpt_t *st;

	st = &swaptab[dbd->dbd_swptb];
	smpi = dbd->dbd_swpmp;

	return (st->st_swpmp[smpi].sm_ucnt == 1);
}

/*
 * Free one page of swap and return the resulting use count.
 */

void swfree1(dbd)
swpdbd_t *dbd;	/* Ptr to disk block descriptor for	*/
		/* block to be removed.			*/
{
	register short	smpi;
	register swpt_t	*st;

	swaplock();

	st = &swaptab[dbd->dbd_swptb];
	smpi = dbd->dbd_swpmp;

	VASSERT(st->st_swpmp[smpi].sm_ucnt != 0);

	/*	
	 * Decrement the use count for this page.  If it goes
	 * to zero, then free the page.  If anyone is waiting
	 * for swap space, wake them up.
	 */
	if (--st->st_swpmp[smpi].sm_ucnt != 0) {
		swapunlock();
		return;
	}

	/*
	 * Increment free page counters.
	 */
	st->st_nfpgs += 1;
	if (st->st_dev)
		st->st_dev->sw_nfpgs++;
	else
		st->st_fsp->fsw_nfpgs++;

	/*
	 * Insert this back in the free list by scanning
	 * back to find the first free entry.
	 */
	for (smpi = dbd->dbd_swpmp - 1; smpi >=  0; smpi--) {
		if (st->st_swpmp[smpi].sm_ucnt == 0)
			break;
	}; 

	/*
	 * if we are the first free entry, then adjust
	 * pointers, otherwise insert it into the list.
	 */
	if (smpi == -1) {
		st->st_swpmp[dbd->dbd_swpmp].sm_next = st->st_free;
		st->st_free = dbd->dbd_swpmp;
	}
	else {		
#ifdef OSDEBUG
		short smpidbg = st->st_swpmp[smpi].sm_next;
		VASSERT(st->st_swpmp[smpi].sm_ucnt == 0);
		if ((smpidbg != -1) && (st->st_swpmp[smpidbg].sm_ucnt != 0))
			panic("swfree1: free list corrupted");
#endif
		st->st_swpmp[dbd->dbd_swpmp].sm_next = 
				st->st_swpmp[smpi].sm_next;
		st->st_swpmp[smpi].sm_next = dbd->dbd_swpmp;
	}

	/*
	 * If all the pages on the device are now free, and
	 * the file is marked for deletion, release swap.
	 */
	if ((st->st_flags & ST_INDEL) && st->st_nfpgs == NPGCHUNK)
		(void)swaprem(st);

	swapunlock();
}


#ifdef OSDEBUG
/*
 * Find the use count for a block of swap.
 */
swpuse(dbd)
swpdbd_t *dbd;
{
	register swpt_t	*st;
	register int cnt;

	st = &swaptab[dbd->dbd_swptb];

	swaplock();
	cnt = st->st_swpmp[dbd->dbd_swpmp].sm_ucnt;
	swapunlock();

	return(cnt);
}
#endif /* OSDEBUG */


/*
 * Increment the use count for a block of swap.
 */
swpinc(dbd, nm)
swpdbd_t *dbd;
char *nm;
{
	register swpt_t	*st;

	VASSERT(vm_valusema(&swap_lock) <= 0);

	st = &swaptab[dbd->dbd_swptb];
#ifdef OSDEBUG
	check_swpmp(st, dbd->dbd_swpmp);
#endif
	if (st->st_swpmp[dbd->dbd_swpmp].sm_ucnt >= MAXSUSE) {
		printf("%s - swpuse count overflow\n", nm);
		return(0);
	} 
	
	if (st->st_swpmp[dbd->dbd_swpmp].sm_ucnt == 0) {
	    /* If this panic happens, someone is probably trying to share 
	     * swap space for a shared page, and is incorrectly assuming
	     * that space has already been allocated. 
	     */
	    panic("swap allocation error: swpinc() called with sm_ucnt == 0");
	}

	st->st_swpmp[dbd->dbd_swpmp].sm_ucnt++;
	return(1);
}


/*	
 * Add a new swap space into swaptab[].
 */
swapadd(swdev, swfs, nentries, name)
swdev_t	 *swdev;
fswdev_t *swfs;
uint nentries;	
char *name;
{
	register int	first_swi;
	register swpt_t	*st;
	register int	first;
	register u_int  context;
	int	npages;
	int	start;
	int	prev_swi;
	int	swi;
	int	i, k;
	uint	j;
	unsigned int	spl_level;
	struct vnode	*devvp;
	struct vnode	*sw_vnode;
	void memunreserve();
	unsigned int sleep_lock();

	/* 
	 * Find a free entry in the swap file table.
	 * Check to see if the new entry duplicates an
	 * existing entry.  If so, this is an error unless
	 * the existing entry is being deleted.  In this
	 * case, just clear the INDEL flag and the swap
	 * file will be used again.
	 */

	st = (swpt_t *) 0;
	first = 1;
	start = 0;

	/*
         * Open the swap file.
         */
	if (swdev) {
        	devvp = devtovp(swdev->sw_dev);
        	u.u_error = 0;
        	u.u_error = (*devvp->v_op->vn_open)(devvp->v_rdev);
        	if (u.u_error)
               		return(0);
	}

	/*
         * make sure we have enough memory to back up
         * the swap kernel structures.
         *
         *			XXX  WARNING WARNING
         *	the accounting for steal_swap/return_swap() is confused;
         *	e.g. kdrele (called by kfree_unused coalescing code) calls 
         *	return_swap and lockmemunreserve, which also calls return_swap
         *	but doesn't call memunreserve
         */
	npages = btorp(nentries * NPGCHUNK * sizeof(swpm_t));
        if (steal_swap((reg_t *)0, npages, 1) == 0) {
                u.u_error = ENOMEM;
		return(0);
	}
	
	if (cmemreserve((unsigned int)npages) == 0) {
                return_swap(npages);
                u.u_error = ENOMEM;
		return(0);
        }

	prev_swi = -1;
	for (j = 0; j < nentries; j++) {
		swi = -1;
		for (i = start;  i < maxswapchunks;  i++) {
			if (swaptab[i].st_swpmp == NULL) {
               		         if (swi == -1)
               		                 swi = i;
				 break;
			}
		}
		start = i + 1;
		

		/*      
		 * If no free entry is available, give an error
                 * return.
                 */
        	if (swi < 0) {
			if (swdev) {
				if (swdev->sw_enable == 0) {
					printf("Unable to add all swap for device: %s.  Increase the tunable parameter maxswapchunks by %d and re-configure your system.\n", name, nentries-j);
				}
			}
			else {
				if (swfs->fsw_enable == 0) {
					printf("Unable to add all file system swap from: %s.  Increase the tunable parameter maxswapchunks by %d and re-configure your system.\n", name, nentries-j);
				}
			}

			swi = prev_swi;
			break;
		}

		/*
		 * remember the first swaptab[] entry we got.
		 */
		if (first) {
			first = 0;
			first_swi = swi;
		}
		else
			st->st_next = swi;

		st = &swaptab[swi];

 		/*
                 * Allocate space for the use count array.
                 * One counter for each page of swap.
                 */
                i = NPGCHUNK * sizeof(swpm_t);
                st->st_swpmp = 
			(swpm_t *)kmalloc(i, M_SWAPMAP, M_NOWAIT | M_NORESV);
 
                if (st->st_swpmp == NULL)
                        panic ("swapadd(): kalloc failed");
			
		if (swdev) {

			/*
			 * Initialize the new entry.
		 	 */
			st->st_dev = swdev;
			st->st_fsp = 0;
			st->st_vnode = devvp;
			st->st_union.st_start = swdev->sw_start;
		}
		else {
			/*
			 * get file system chunk
			 */
			st->st_dev =  0;
                        st->st_fsp = swfs;
			sw_vnode = allocate_fs_space(st->st_fsp);
			if (sw_vnode) {
				st->st_vnode = sw_vnode;
                        	st->st_union.st_start = 0;
			}
			else {
                        	kfree(st->st_swpmp, M_SWAPMAP);
				i = btorp((nentries - j)*NPGCHUNK * sizeof(swpm_t));
				return_swap(i);		/*  XXX see comment above  */
				memunreserve(i);

                        	st->st_swpmp = 0;
				st->st_fsp = 0;
				st->st_next = 0;
				swi = prev_swi;
				goto cont;
			}
		}

		for (k = 0; k < NPGCHUNK; k++) { 
			st->st_swpmp[k].sm_ucnt = 0;
			st->st_swpmp[k].sm_next = k+1;
		} 

		st->st_swpmp[NPGCHUNK-1].sm_next = -1;
		st->st_nfpgs = NPGCHUNK;
                st->st_free = 0;
                st->st_site = -1;
		prev_swi = swi;
	}
	
	if (j != nentries) {
		i = btorp((nentries - j)*NPGCHUNK * sizeof(swpm_t));
                memunreserve(i);
                return_swap(i);		/*  XXX see comment above  */
        }
cont:
	if (j == 0) {
		u.u_error = ENOSPC;
                return(0);
	}

	/*
         * If this is a new device or file system, set up swdevt
         * to point to the first and last swaptab[] entry.
         *
         * If we are adding additional blocks for an exisiting
         * device or file system, just modify the last entry
         * pointer in swdevt[] or fswdevt[]
         */

	if (swdev) {

		if (swdev->sw_head == -1)
			swdev->sw_head = first_swi;
		else 
			swaptab[swdev->sw_tail].st_next = first_swi;

		swdev->sw_tail = swi;
		swdev->sw_nfpgs += j * NPGCHUNK;
		swaptab[swdev->sw_tail].st_next = -1;

	}
	else {
		if (swfs->fsw_head == -1)
                        swfs->fsw_head = first_swi;
                else
                        swaptab[swfs->fsw_tail].st_next = first_swi;

                swfs->fsw_tail = swi;
                swfs->fsw_nfpgs += j * NPGCHUNK;
		swaptab[swfs->fsw_tail].st_next = -1;

	}
	SPINLOCK_USAV(rswap_lock, context);
	swapspc_cnt += j * NPGCHUNK;
        swapspc_max += j * NPGCHUNK;
	SPINUNLOCK_USAV(rswap_lock, context);

	/*
	 * Clearing the flags allows swalloc to find it.
         */
        st->st_flags = 0;

	/*
	 * Wakeup anyone waiting to reserve swap space.
	 */
	spl_level = sleep_lock();
	if (res_swap_wait) {
		res_swap_wait = 0;
		wakeup((caddr_t)&res_swap_wait);
	}
	sleep_unlock(spl_level);

	return(1);
}

#ifdef LATER
	/* The only routine that calls this routine is realswapoff()
	 * which is not called and not compiled.  So I ifdef'd out this
	 * routine to save space.  cwb 5/27/92
	 */

/*	Delete a swap file.
 */

swapdel(swdev, swfs)
swdev_t *swdev;	/* Device to delete.			*/
fswdev_t *swfs;	/* Low block number of area to delete.	*/
{
	register int	i;
	register int	start;
	register int	maxpgs;
	register int	nentries;

	/* 
	 * Set the delete flag.  Clean up its pages.
	 * The file will be removed by swfree1 when
	 * all of the pages are freed.
	 */
	swaplock();

	if (swdev) {
		swdev->sw_enable = 0;
		start = swdev->sw_head;
	}
        else {
		swfs->fsw_enable = 0;
		start = swfs->fsw_head;
	}
        for (i = start; i > 0; i = swaptab[i].st_next) {
                swaptab[i].st_flags |= ST_INDEL;
	}

	if (swdev) {
		nentries = swdev->sw_nblks / swchunk;
		maxpgs = NPGCHUNK * nentries;
		if(swdev->sw_nfpgs < maxpgs) {
			swapunlock();
			if (!freeswap(swdev, (fswdev_t *)0))
				return(0);
		}
	}
	else {
		maxpgs = NPGCHUNK * swfs->fsw_allocated;
		if (swfs->fsw_nfpgs < maxpgs) {
                        swapunlock();
                        if (!freeswap((swdev_t *)0, swfs))
				return(0);
                }
	}

	for (i = start; i > 0; i = swaptab[i].st_next) {
			(void)swaprem(&swaptab[i]);
        }
	return(1);
}
#endif /* LATER */

/*
 * take swaptab entry out of the list
 */

unlink_swptb(st)
swpt_t *st;
{
	swpt_t *swptb;
	swpt_t *swptmp;
	int index;

	if (st->st_dev) {
		swptb = &swaptab[st->st_dev->sw_head];
		if (st == swptb) {
			st->st_dev->sw_head = st->st_next;
                        if (st->st_next == -1)
                                st->st_dev->sw_tail = -1;
			return;
		}
	}
	else {
		swptb = &swaptab[st->st_fsp->fsw_head];
		if (st == swptb) {
			st->st_fsp->fsw_head = st->st_next;
                        if (st->st_next == -1)
                                st->st_fsp->fsw_tail = -1;
			return;
		}
	}

	for(index = swptb->st_next; index != -1; index = swptb->st_next) {	
		swptmp = &swaptab[index];
		if (st == swptmp) {
			swptb->st_next = st->st_next;
                          /* if we just removed the tail, update tail index */
                        if (swptb->st_next == -1) {
                                if (st->st_dev)
                                    st->st_dev->sw_tail = swptb - swaptab;
                                else
                                    st->st_fsp->fsw_tail = swptb - swaptab;
                        }
			return;
		}
		swptb = &swaptab[index];
	}
	panic("unlink_swptb: entry not in chain");
}
/*
 * Clear a swap chunk from swaptab[].
 */

swaprem(st)
swpt_t *st;
{
	register int	i;
        register u_int  context;
	extern void use_dev_vp();
	extern void drop_dev_vp();

	VASSERT(vm_valusema(&swap_lock) <= 0);

	if (st->st_swpmp == NULL) 
		return(0);

	if (st->st_dev) { 
		/*
		 * nfpgs was cleared so no one would use this chunk
                 * until it was unlinked.  since this is a device
                 * chunk, st_nfpgs should be NPGCHUNK
		 */
		st->st_nfpgs = NPGCHUNK;
	}
	else {
		/*
		 * This is a file system chunk. We must keep at least
		 * fsw_min file system chunks around.
		 */
		if ((st->st_fsp->fsw_allocated-1) >= st->st_fsp->fsw_min) {

			/*
			 * remove swaptab entry from linked list
                         * if the space has not been commited yet.
			 */
                        SPINLOCK_USAV(rswap_lock, context);
			VASSERT(swapspc_cnt >= 0);
			if ((swapspc_cnt - NPGCHUNK) < 0) {
                                SPINUNLOCK_USAV(rswap_lock, context);
				st->st_nfpgs = NPGCHUNK;
				st->st_site = my_site;
				st->st_free = 0;
				return(0);
			}
			swapspc_cnt -= NPGCHUNK;
			swapspc_max -= NPGCHUNK;
			VASSERT(swapspc_cnt >=0);
			VASSERT(swapspc_cnt <= swapspc_max);
                        SPINUNLOCK_USAV(rswap_lock, context);
			st->st_fsp->fsw_nfpgs -= NPGCHUNK;
			unlink_swptb(st);
			use_dev_vp(st->st_vnode);
			ilock(VTOI(st->st_vnode));
			iput(VTOI(st->st_vnode));
			drop_dev_vp(st->st_vnode);
			st->st_fsp->fsw_allocated -= 1;

			kfree(st->st_swpmp, M_SWAPMAP);		/*  XXX need memunreserve?  */

        		st->st_swpmp = 0;
			st->st_nfpgs = 0;
        		st->st_fsp = 0;
			st->st_vnode = 0;
		}
		else {
			/* 
			 * can't release this file system chunk right now
			 */
			st->st_nfpgs = NPGCHUNK;
			st->st_free = 0;
			st->st_site = -1;
			return(0);
		}
	}

	/*
	 * Release the space used by the use count array.
	 */

	st->st_free = 0;
	st->st_site = -1;

	return(1);
}

#ifdef LATER
	/* The only routine that calls this routine is swapdel()
	 * which is not called and not compiled.  So I ifdef'd out this
	 * routine to save space.  cwb 5/27/92
	 */

/*
 * Try to free up swap space on the swap device being deleted.
 * Look at every region for pages which are swapped to the
 * device we want to delete.  Read in these pages and delete
 * the swap.
 * %%% Original source unlocked the region list while swapping
 *      in pages, then locked and continued.  This means that
 *	the region pointers might not be valid, an obvious bug.
 *	The question is, will the retry strategy used to avoid
 *	this problem cause performance problems?
 */
freeswap(swdev, swfs)
swdev_t *swdev;
fswdev_t *swfs;
{
	register reg_t	*rp;
	register swpdbd_t	*dbd;
	register int	start;
	register int	j;
	register int	i;
        register u_int  context;
	extern void vfdswapi();

    retry:
	rlstlock(context);
	for (rp = regactive.r_forw; rp != &regactive; rp = rp->r_forw) {
		if (!creglock(rp))
			continue;

		/* if the regions vfd's are swapped out, swap them in */
		if (rp->r_dbd) {
			rlstunlock(context);
			vfdswapi(rp);
			goto retry;
		}

		/*
		 * Loop through all the pages of the region.
		 */
		if (swdev)
			start = swdev->sw_head;
		else
			start = swfs->fsw_head;

		for (i = 0; i < rp->r_pgsz; i++) {
			dbd = (swpdbd_t *)FINDDBD(rp, i);
			if (dbd->dbd_type == DBD_BSTORE) {
				for (j=start; j>0; j=swaptab[i].st_next) {
					if (dbd->dbd_swptb == j) {
						rlstunlock(context);
						unswap(rp, j);
						goto retry;
					}
        			}
			}
		}
		regrele(rp);
	}
	rlstunlock(context);
	return (0);
}
#endif /* LATER */

/*
 * Free up the swap block being used by the indicated page.
 * The region is locked when we are called.
 * This code roughly parallels vfault().
 */
/*ARGSUSED*/
#ifdef LATER
unswap(rp, i)
reg_t	*rp;
int i;
{
    	vfd_t	*vfd = FINDVFD(rp, i);
    	swpdbd_t	*dbd = (swpdbd_t *)FINDDBD(rp, i);
    	pfd_t	*pfd;

	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	/*	
	 * If a copy of the page is in core, then just
	 * release the copy on swap.
	 */

	if (vfd>pgm.pg_v) {
top:
		pfd = &pfdat[vfd->pgm.pg_pfn];
		pfdatlock(pfd);
		if (PAGEINHASH(pfd))
			pageremove(pfd);
		pfdatunlock(pfd);
		(void)swfree1(dbd);
		dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff11;
		return;
	}
	if (vfdmemall(rp, i, 1)) {
		goto top;
	}

	/*	Read in the page from swap and then free up the swap.
	 */

	Qswap(dbd, rp->r_space, ptova(rp, i), 1, B_READ);
	(void)swfree1(dbd);
	dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff12;
	pfd = &pfdat[vfd->pgm.pg_pfn];
	pdeunlock(vfd->pgm.pg_pfn);
}
#endif

/*
 * Search swap use counters looking for size contiguous free pages.
 * Returns the page number found sucess, -1 on failure.
 */
swapfind(indx, size)
short indx;
int size;
{
	register int p;
	register int i;
	register int save;
	register int start;
	register int end;
	swpt_t *st;

	save = -1;
	st = &swaptab[indx];
	VASSERT(st->st_free != -1);
	p = st->st_free;
	end = NPGCHUNK - size;

	if (p > end)
		return(-1);

	if (size == 1) {
		VASSERT(st->st_swpmp[p].sm_ucnt == 0);
		st->st_free = st->st_swpmp[p].sm_next;
           	return(p);

	}

Retry:
	VASSERT(st->st_swpmp[p].sm_ucnt == 0);
	i = 1;
	start = p;
	   
	while (i < size) {
	   	if (st->st_swpmp[p].sm_next == p+1) {
			VASSERT(st->st_swpmp[p+1].sm_ucnt == 0);
		  	i++;
			p++;
	  	}
	      	else {
		   	save = p;
		   	p = st->st_swpmp[p].sm_next;
		   	if ((p == -1) || (p > end))
		   		return(-1);
		   	else
				goto Retry;
              	}
	} 

	if (save != -1)
		st->st_swpmp[save].sm_next = st->st_swpmp[p].sm_next;
	else
		st->st_free = st->st_swpmp[p].sm_next;

	return(start);
}

/*
 * Find a continguous run of pages on some swap device.  Don't try
 * very hard to get the space.
 */
swcontig(dbd, size)
swpdbd_t *dbd;
int size;
{
   register short smpi;
   register int i;
   swpdbd_t swappg;
   swpt_t *st;

	/*
	 * get_swap returns the swaptab[] entry and the
	 * swapmap[] index for the first page allocated.
	 */

	if (!get_swapspace(size, &swappg)) 
		return(0);

	st = &swaptab[swappg.dbd_swptb];
	smpi = swappg.dbd_swpmp;

	/*
	 * If we got the swap space, set up the dbd entry pointed
	 * to with the first block of the contiguous space.
	 */

	for (i = 0; i < size; i++) {
#ifdef OSDEBUG
		check_swpmp(st, smpi+i);
#endif
		st->st_swpmp[smpi + i].sm_ucnt = 1;
	}

	st->st_site = my_site;
        st->st_nfpgs -= size;
        st->st_flags &= ~ST_FREE;
        VASSERT(st->st_nfpgs >= 0);

	dbd->dbd_swptb =swappg.dbd_swptb;
	dbd->dbd_swpmp = smpi;
	dbd->dbd_type = DBD_BSTORE;

	swapunlock();
	return(1);
}

struct vnode *
allocate_fs_space(fswp)
fswdev_t *fswp;
{
	register struct inode *ip, *rp;
	register int numblks, nbfree;
	register int fs_bsize;
	register int i;
	struct fs *fs;
	sv_sema_t ss;


	if ((fswp->fsw_limit) && (fswp->fsw_allocated >= fswp->fsw_limit)) {
		if (fs_swap_debug)
			printf("File system \"%s\" has exceeded swap limit: allocated %d limit %d\n", fswp->fsw_mntpoint, fswp->fsw_allocated, fswp->fsw_limit);
		return(0);
	}

	rp = VTOI(fswp->fsw_vnode);
	if (rp == NULL) {
		if (fs_swap_debug)
			printf("File system \"%s\": root inode is null\n", fswp->fsw_mntpoint);
		return(0);
	}

	fs = rp->i_fs;
	fs_bsize = fs->fs_bsize;

	PXSEMA(&filesys_sema, &ss);
	use_dev_vp(ITOV(rp));
        ip = ialloc(rp, 0, IFREG);

	if (ip == NULL) {
		drop_dev_vp(ITOV(rp));
		VXSEMA(&filesys_sema, &ss);
		return(0);
	}

	/* dont exceed minfree for this file system */
	nbfree = fs->fs_cstotal.cs_nbfree -
                (((fs->fs_dsize * fs->fs_minfree)/(100*fs_bsize))
		+ fswp->fsw_reserve);

        if (nbfree <= ((swchunk*DEV_BSIZE)/fs_bsize)) {
		if (fs_swap_debug)
			printf("Cant allocate swap space from file system \"%s\": minfree exceeded\n", fswp->fsw_mntpoint);
                iput(ip);
                drop_dev_vp(ITOV(rp));	
		VXSEMA(&filesys_sema, &ss);
		return(0);
	}

	ip->i_mode = IFREG;

	/* 
	 * how many blocks does it take to equal (swchunk * DEV_BSIZE)
	 * bytes.  There had better not be any space lost in the division
	 */
	numblks = (swchunk * DEV_BSIZE) / fs_bsize;
	for (i=0; i<numblks; i++ ) {
		int bn;
		int rbn;
		if ((bn = bmap(ip, (daddr_t)i, B_WRITE, fs_bsize, (int *)0, 
				(daddr_t *)0, (int *)0)) <= (daddr_t)0) {

			if (fs_swap_debug)
				printf("allocate_fsw_space: bmap failed 0\n");
                        iput(ip);
			drop_dev_vp(ITOV(rp));
			VXSEMA(&filesys_sema, &ss);
			return(0);
		}
		rbn = fsbtodb(ip->i_fs, bn);	
		bpurge(ip->i_devvp, (daddr_t)rbn);
		ip->i_size += fs_bsize;
	}
	iunlock(ip);
	drop_dev_vp(ITOV(rp));
	fswp->fsw_allocated++;
	VXSEMA(&filesys_sema, &ss);
	return (ITOV(ip));
}

extern struct fswdevt *fsp[];

/* 
 * find the first chunk with free space
 */
get_swchunk(fchunk, size, swappg, getnew)
int fchunk;
int size;
swpdbd_t *swappg;
int getnew;
{
	int i;
	int page;
	
 	i = fchunk;
	for (;;) {
		if (i == -1) break;

		/* If we must have a new chunk skip forward
                 * until we find one
                 */
		if ((getnew) && (swaptab[i].st_site != -1))
			goto next;

		if (size <= swaptab[i].st_nfpgs) {
			page = swapfind(i, size);
			if (page != -1) {
				swappg->dbd_swptb = i;
			    	swappg->dbd_swpmp = page;
				return(1);
		        }
                }

	next:
		i = swaptab[i].st_next;
	}
	return(0);
}

/*
 * find the highest priority device or file system with space 
 * available.
 */
get_swap(size, swappg, getnew)
int size;
swpdbd_t *swappg;
int getnew;
{
	/* don't forget error checking */
	swdev_t *start_dev;
	swdev_t *curr_dev;
	fswdev_t *start_fs;
	fswdev_t *curr_fs;
	int pri, maxpri;
	int allocate=0;

	if(maxdev_pri > maxfs_pri)
		maxpri = maxdev_pri;
	else
		maxpri = maxfs_pri;

	for (pri = 0; pri <= maxpri; pri++) {
		if (swdev_pri[pri].first == 0)
			goto check_fs;

		start_dev = swdev_pri[pri].curr;
		curr_dev = start_dev;
		do {
		   if (!curr_dev->sw_enable) {
			curr_dev = curr_dev->sw_next;
			continue;
		   }
		   if (size <= curr_dev->sw_nfpgs) {
		      if (get_swchunk(curr_dev->sw_head,size,swappg,getnew)) {
				swdev_pri[pri].curr = curr_dev->sw_next;
				return(1);
	              }
		   }
		   curr_dev = curr_dev->sw_next;
		}  while (curr_dev != start_dev);


 check_fs:
                if (swfs_pri[pri].first == 0)
                        continue;

		start_fs = swfs_pri[pri].curr;
		curr_fs = start_fs;

                do {
                   if (!curr_fs->fsw_enable) {
                  	 curr_fs = curr_fs->fsw_next;
                         continue;
                   }

		retry1:
	           if (size <= curr_fs->fsw_nfpgs) {
		      if (get_swchunk(curr_fs->fsw_head,size,swappg,getnew)) {
                               	swfs_pri[pri].curr = curr_fs->fsw_next;
                               	return(1);
		      }
                   }
		   if (allocate) {
		      if (swapadd((swdev_t *)0, curr_fs, 1, (char *)0)) {
				goto retry1;
		      }
		   }
                   curr_fs = curr_fs->fsw_next;

                } while (curr_fs != start_fs);
		if (!allocate) {
	 		allocate = 1;
			goto check_fs;
		}
		else {
			allocate = 0;
		}
	}
	
	return(0);
}
/*
 * Allocate another chunk of swap space from the highest priority
 * file system.
 */
add_swap()
{
	int pri;
	fswdev_t *start_fs;
        fswdev_t *curr_fs;

	if (my_site_status & CCT_SLWS) {
		if (!dux_swaprequest())
			return(0);
		else
			return(1);
	}
	else {
		swaplock();
		for (pri = 0; pri <= maxfs_pri; pri++) {
			if (swfs_pri[pri].first == 0)
				continue;
			start_fs = swfs_pri[pri].curr;
                	curr_fs = start_fs;

                	do {
				if (!curr_fs->fsw_enable) {
                                	curr_fs = curr_fs->fsw_next;
                                	continue;
                        	}
				if (swapadd((swdev_t *)0, curr_fs,1,(char *)0)){
					swapunlock();
                                        return(1);
                                }
				curr_fs = curr_fs->fsw_next;

                	} while (curr_fs != start_fs);
		}
		swapunlock();
		return(0);
	}
}
#ifdef NEVER_CALLED
return_swap_to_fs()
{
	 swpt_t *st;
	struct inode *tinode;

	if (fs_swap_debug)
		printf("return_swap_to_fs\n");
	for (st = &swaptab[0]; st < &swaptab[maxswapchunks]; st++) {	
		if (st->st_fsp != 0) {
			if (fs_swap_debug)
				printf("return_swap_to_fs vnode\n");
			tinode = VTOI(st->st_vnode);
			use_dev_vp(st->st_vnode);
                        ilock(tinode);
                        iput(tinode);
			drop_dev_vp(st->st_vnode);
		}
	}
}
#endif /* NEVER_CALLED */

int res_swapmemcnt = 0;
/*
 * Reserve swap space.
 * If nowait is set we do not go to sleep waiting for swap space.
 *
 * rswaplock()  is a spinlock for the swap counters.
 * swaplock()  is a semaphore for the swap structures.
 */
reserve_swap(rp, npages, flag)
reg_t *rp;
int npages;
int flag;
{
        register u_int context;
	unsigned int	spl_level;
	unsigned int sleep_lock();
	int nowait = (flag&SWAP_NOWAIT);
	int nosleep = (flag&SWAP_NOSLEEP);

	if (!npages) 
		return(1);

	if (noswap) 
		return(1);

	VASSERT(rp != NULL);

retry:
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(!owns_spinlock(rswap_lock));
        SPINLOCK_USAV(rswap_lock, context);
	VASSERT(swapspc_cnt >= 0);
	if (swapspc_cnt - npages < 0) {
                if (swapmem_cnt - npages < 0) {
                        SPINUNLOCK_USAV(rswap_lock, context);

			/*
			 * Could sleep waiting for file system swap
			 */
			if (nosleep)
				return(0);

			regrele(rp);
                        if (add_swap()) {
                                reglock(rp);
				goto retry;
			}
			else {
				/*
				 * Go to sleep until someone frees up
				 * space.
				 */
				if (nowait) {
					reglock(rp);
					return(0);
				}

				spl_level = sleep_lock();
				res_swap_wait++;
				sleep_then_unlock((caddr_t)&res_swap_wait, 
					PMEM, spl_level);
				reglock(rp);
				goto retry;
			}
		}
                else {
			/*
			 * swapmem_cnt is shared by the swapper and 
                         * kdalloc().
			 *
                         * res_swapmemcnt tracks memory only used by
			 * the swapper.  we need a second count so
			 * we know how much space we can migrate
			 * in steal_swap().
			 */
                        swapmem_cnt -= npages;
			res_swapmemcnt += npages;
		}
        } 
	else {
                swapspc_cnt -= npages;
	}
        SPINUNLOCK_USAV(rswap_lock, context);
	return(1);
}

/*
 * migrate reserved swap from system memory to actual swap space
 */
migrate_swap(npages, nowait)
int npages;
int nowait;
{
        register u_int context;
	if (!npages) 
		return(1);
        SPINLOCK_USAV(rswap_lock, context);
	VASSERT(swapspc_cnt >= 0);
retry:
	if (swapspc_cnt - npages < 0) {
                SPINUNLOCK_USAV(rswap_lock, context);
		if (nowait)
			return(0);

		printf("migrate_swap: add_swap for %x\n",npages);
        	if (add_swap()) {
                        SPINLOCK_USAV(rswap_lock, context);
			goto retry;
		}
		else
			return(0);
        } 
        swapspc_cnt -= npages;
	VASSERT(swapspc_cnt >= 0);
        swapmem_cnt += npages;
        res_swapmemcnt -= npages;
        SPINUNLOCK_USAV(rswap_lock, context);
	return(1);
}

/*
 * update swap count
 */

release_swap(npages)
int npages;
{
	int extra;
        register int context;
	unsigned int	spl_level;
	unsigned int sleep_lock();

	if (noswap) 
		return;

        SPINLOCK_USAV(rswap_lock, context);
        swapmem_cnt += npages;
	extra = swapmem_cnt - swapmem_max;
	if (extra > 0) {
		VASSERT(swapspc_cnt >= 0);
		swapspc_cnt += extra;
		VASSERT(swapspc_cnt <= swapspc_max);
        	swapmem_cnt -= extra;
	}
	if (res_swapmemcnt - npages < 0)
		res_swapmemcnt = 0;
	else
		res_swapmemcnt -= npages;

	/*
	 * Wakeup anyone waiting to reserve swap space.
	 */
	spl_level = sleep_lock();
	if (res_swap_wait) {
		res_swap_wait = 0;
		wakeup((caddr_t)&res_swap_wait);
	}
	sleep_unlock(spl_level);
        SPINUNLOCK_USAV(rswap_lock, context);
}

/*
 * steal memory from swapper
 */
steal_swap(rp, npages, nowait)
reg_t *rp;
int npages;
int nowait;
{
        register u_int context;
	unsigned int	spl_level;
	unsigned int sleep_lock();

	/* 
	 * If we are not using memory for "swap" space
	 * then return.
	 */
	if (!swapmem_on)
                return(1);

	if (!npages) 
		return(1);

#ifdef OSDEBUG
	if (mainentered && ON_ISTACK)
		VASSERT(nowait);
	if (rp)
		VASSERT(vm_valusema(&rp->r_lock) <= 0);
#endif OSDEBUG
	
	/*
         * don't reserve any space if we have already in procmemreserve()
         */
        if (!ON_ISTACK) {
                register struct proc *p = u.u_procp;

		if (p->p_swpresv != -1) {
                        if (npages > p->p_swpresv)
                                panic("steal_swap: reservation overrun");
                        p->p_swpresv -= npages;
                        return(1);
                }
        }

retry:
        SPINLOCK_USAV(rswap_lock, context);
        if (swapmem_cnt - npages < 0) {
		/*
                 * Since the kernel dynamically allocates kernel structures,
                 * the system could deadlock if we have locked down
                 * all available memory through the localable_mem()
                 * interface.
                 */

                if (sys_mem - npages > 0) {
                        sys_mem -= npages;
                        SPINUNLOCK_USAV(rswap_lock, context);
                        return(1);
                }
		if ((res_swapmemcnt-npages) < 0) {
                        SPINUNLOCK_USAV(rswap_lock, context);
                        return(0);
                }
                SPINUNLOCK_USAV(rswap_lock, context);
                while (!migrate_swap(npages, nowait)) {
			if (nowait)
				return(0);
			else {
                		printf("steal_swap():  kernel approaching potential deadlock.  Possible fix is to gen kernel with a larger value of unlockable memory\n");
				/*
				 * Go to sleep until I can get real memory. 
				 */
				if (rp != NULL)
					regrele(rp);
				spl_level = sleep_lock();
				res_mem_wait++;
				sleep_then_unlock((caddr_t)&res_mem_wait, 
					PMEM, spl_level);
				if (rp != NULL)
					reglock(rp);
				goto retry;
			}
		}
		goto retry;
        }
	VASSERT(swapmem_cnt - npages >= 0);
        swapmem_cnt -= npages;
        SPINUNLOCK_USAV(rswap_lock, context);
        return(1);
}

return_swap(npages)
int npages;
{
	int extra;
        register u_int context;
	unsigned int	spl_level;
	unsigned int sleep_lock();
	/* 

	 * If we are not using memory for "swap" space
	 * then return.
	 */
	if (!swapmem_on)
                return(1);

	if (!ON_ISTACK) {
                register struct proc *p = u.u_procp;
		if (p->p_swpresv != -1) {
               		p->p_swpresv += npages;
                	return(1);
        	}
	}

        SPINLOCK_USAV(rswap_lock, context);
	extra = (sys_mem + npages) - (maxmem - swapmem_max);

        if (extra <= 0) {
                sys_mem += npages;
		/*
		 * Wakeup anyone waiting for real memory.
		 */
		spl_level = sleep_lock();
		if (res_mem_wait) {
			res_mem_wait = 0;
			wakeup((caddr_t)&res_mem_wait);
		}
		sleep_unlock(spl_level);
	}
        else {
                sys_mem = sys_mem + npages - extra;
		swapmem_cnt += extra;
		extra = swapmem_cnt - swapmem_max;
		if (extra > 0) {
			VASSERT(swapspc_cnt >= 0);
			swapspc_cnt += extra;
			VASSERT(swapspc_cnt <= swapspc_max);
			swapmem_cnt -= extra;
		}
		if (res_swapmemcnt - npages < 0)
			res_swapmemcnt = 0;
		else
			res_swapmemcnt -= npages;
		/*
		 * Since we incremented swapmem_cnt we can wakeup
		 * anyone sleeping waiting for real memory or to 
		 * reserve swap space. 
		 */
		spl_level = sleep_lock();
		if (res_mem_wait) {
			res_mem_wait = 0;
			wakeup((caddr_t)&res_mem_wait);
		}
		if (res_swap_wait) {
			res_swap_wait = 0;
			wakeup((caddr_t)&res_swap_wait);
		}
		sleep_unlock(spl_level);
	}
        SPINUNLOCK_USAV(rswap_lock, context);
	return ( 0 );
}


/*
 * chunk_release() is called from lsync to release unused chunks 
 * from the swap table.
 */

chunk_release(chkall)
	int chkall;	/* if chkall == TRUE then check all chunks */
                        /* else check file system chunks only      */
{
	struct swaptab *st;
	int count,ret;

	/*
	 * count how many chunks are currently in use.
	 * we need this to be certain that we keep minswapchunks
	 * chunks around.
	 */
	count = 0;
	for (st = &swaptab[0]; st < swapMAXSWAPTAB; st++) {
		if (st->st_site == my_site || st->st_site == -1)
			count++;
	}

	if (count <= minswapchunks)	/* no need to bother */
		return;

	for (st = &swaptab[0]; st < swapMAXSWAPTAB; st++) {

		if (!st->st_swpmp )
			continue;

		if (st->st_site != my_site && st->st_site != -1)
			continue;

		swaplock();

		if (!chkall && (int)st->st_dev) {   /* ignore device chunks, */
			swapunlock();	            /* if appropriate	    */
			continue;
		}

		if ((st->st_nfpgs != NPGCHUNK)) {
			swapunlock();
			continue;
		}

		/* Chunk is releasable only if it is not referenced lately */
		if (!(st->st_flags & ST_FREE)) {
			st->st_flags |= ST_FREE;
			swapunlock();
			continue;
		}
  
		/* set st_nfpgs to zero so no one tries to use this chunk */
		st->st_nfpgs = 0;
		swapunlock();


		/* Contract swap map. If fails for some reason, reset
		 * number of free pages so can still use it. We will
		 * try again later.
		 */
		
		if (my_site_status & CCT_SLWS) 
			ret = dux_swapcontract(st);

		else 
			ret = swapcontract(st);

		if (ret) {
			/* if there is only min chunks left, quit */
			if (--count <= minswapchunks)
				break;
		}
		else {
			swaplock();
			st->st_nfpgs = NPGCHUNK;
			st->st_flags &= ~ST_FREE;
			swapunlock();
		}
	}
}


swapcontract(st)
	register swpt_t *st;
{
	register int error;

	/*validity check*/
	if ((st >= swapMAXSWAPTAB)
		|| (!st->st_dev && !st->st_fsp) 
		|| (!st->st_swpmp))
		panic("chunk_release : invalid swaptab entry");

	/* invalidate corresponding swapmap entry */
	swaplock();
	error = swaprem(st);
	swapunlock();
	return(error);

}
