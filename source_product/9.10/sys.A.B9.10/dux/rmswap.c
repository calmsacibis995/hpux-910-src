/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/rmswap.c,v $
 * $Revision: 1.10.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/15 12:02:11 $
 */

/* HPUX_ID: @(#)rmswap.c	55.1		88/12/23 */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include  "../h/param.h"
#include  "../h/user.h"
#include  "../h/kernel.h"
#ifdef __hp9000s300
#include  "../wsio/timeout.h"
#include  "../s200io/lnatypes.h"
#undef timeout
#endif
#include  "../h/map.h"
#include  "../dux/dmmsgtype.h"
#include  "../h/errno.h"
#include  "../dux/dm.h"
#include  "../dux/cct.h"
#include  "../dux/rmswap.h"
#include  "../h/proc.h"
#include  "../h/conf.h"
#include  "../h/vnode.h"
#include  "../ufs/inode.h"
#include  "../ufs/fs.h"
#include  "../h/buf.h"
#include  "../h/swap.h"
#include  "../h/errno.h"

extern int res_swap_wait;
extern dev_t	swapdev1;
extern vm_sema_t       swapwait;       /* waiters for swap space */
extern proc_t          *swapwold;      /* head of "waiting for swap" list */
extern int swapspc_max;
int        vm_debug = 0;

/*
 * send a request for a chunk of swap space to the server
 */
dux_swaprequest()
{
	register dm_message reqp;
	register dm_message resp;
	struct dux_vmmesg *resap;
	int error;
	extern struct cct clustab[];

	/* get the chunk from the remote chunk pool */
	/*allocate a message*/
	reqp = dm_alloc(sizeof(struct dux_vmmesg), WAIT);

	/*send the message synchronously and get back the result*/
	resp = dm_send(reqp, DM_SLEEP|DM_RELEASE_REQUEST, DM_CHUNKALLOC,
			swap_site, sizeof(struct dux_vmmesg),
			NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	switch (error = DM_RETURN_CODE(resp)) {
	case 0:
		if (DM_SOURCE(resp) != swap_site )
                        panic("dux_swaprequest : invalid response data");

                /*convert to the dux_vmmesg structure*/
                resap = DM_CONVERT(resp,struct dux_vmmesg);

                swaplock();

                /* get a chunk from our local swap table */
                if (dux_swapadd(resap->swaptab)) {
                        swapunlock();
                        dm_release(resp,0);
                        return(1);
                }
		else {
                        swapunlock();
                        return_chunk(resap->swaptab);
                        dm_release(resp,0);
                        return(0);
                }

	case EACCES:
		printf("dux_swaprequest: GCSP shortage on swap server cluster node %d\n", swap_site);
		/* fall through to ENOSPC */

	case ENOSPC:
		dm_release(resp,0);
		return(0);

	case EIO:
		if (!(clustab[swap_site].status & CL_IS_MEMBER)) {
			printf("dux_swaprequest: swap server %d is not a valid cluster node\n", swap_site);
			dm_release(resp,0);
			return(0);
		} else {
			panic("dux_swaprequest: EIO");
			break;	/* not reached */
		}

	default:
		printf("dux_swaprequest : errorno : %d, ", error);
                panic("dux_swaprequest : error responded");
	}
	/*NOTREACHED*/
}

/*
 * Add new swap chunk into swaptab[].
 */
dux_swapadd(index)
int index;
{
	int	swi;
	int	i, k;
	struct swaptab *st;
	struct swdevt *swdev;
	unsigned int    spl_level;

	/*
	 * Find a free entry in the swap file table.
	 * Check to see if the new entry duplicates an
	 * existing entry.  If so, this is an error unless
	 * the existing entry is being deleted.  In this
	 * case, just clear the INDEL flag and the swap
	 * file will be used again.
	 */
	swi = -1;
	for (i = 0;  i < maxswapchunks;  i++) {
		if (swaptab[i].st_swpmp == NULL) {
               	         if (swi == -1)
               	                 swi = i;
			 break;
		}
	}

	/*
	 * If no free entry is available, give an error
         * return.
         */
        if (swi < 0) {
               	u.u_error = ENOSPC;
               	return(0);
        }

	st = &swaptab[swi];

        /*
	 * Allocate space for the use count array.
         * One counter for each page of swap.
         */
        st->st_swpmp = (struct swapmap *) kmalloc(NPGCHUNK*sizeof(swpm_t), M_SWAPMAP, KALLOC_NOWAIT);

	if (st->st_swpmp == NULL) {
		st->st_fsp = 0;
		st->st_dev = 0;
		st->st_vnode = 0;
		u.u_error= ENOSPC;
		return(0);
	}

	for (k = 0; k < NPGCHUNK; k++) {
		st->st_swpmp[k].sm_ucnt = 0;
		st->st_swpmp[k].sm_next = k+1;
	}

	st->st_swpmp[NPGCHUNK-1].sm_next = -1;
	st->st_nfpgs = NPGCHUNK;
        st->st_free = 0;
        st->st_site = my_site;

	/*
         * initialize even though no local swap so that
         * swfree1 does not dereference zero
         */
        st->st_dev = &swdevt[0];

        /*
	 * store the value of the server swap table entry that
	 * we are using.
	 */
        st->st_union.st_swptab = index;
	rswaplock();
	swapspc_cnt += NPGCHUNK;
	swapspc_max += NPGCHUNK;
	rswapunlock();
	swdev = &swdevt[0];
	if (swdev->sw_head == -1)
		swdev->sw_head = swi;
	else
		swaptab[swdev->sw_tail].st_next = swi;

	swdev->sw_tail = swi;
	swdev->sw_nfpgs += NPGCHUNK;
	swaptab[swdev->sw_tail].st_next = -1;


	/*
	 * Clearing the flags allows swalloc to find it
         * Wakeup if somebody is waiting for swap space
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

/*
 * This routine is run by a kernel NSP to service remote calls to
 * allocate a chunk from the shared swap pool.  It tries to allocate
 * a chunk large enough to accomodate the requested size.  Returns
 * frame no. and size of the chunks and updates chmap.
 */
serve_chunkalloc(reqp)
register dm_message reqp;
{
	register char error;
	register swpt_t *st;
	register dm_message resp;
	register struct dux_vmmesg *resap;
	struct swpdbd swappg;
        /*allocate a dux_vmmesg structure for the response*/
        resp = dm_alloc(sizeof(struct dux_vmmesg), WAIT);

	/*convert the response to a dux_vmmesg structure*/
        resap = DM_CONVERT(resp,struct dux_vmmesg);

	error = 0;
retry:
        rswaplock();
        if ((swapspc_cnt - NPGCHUNK) < 0) {
                rswapunlock();
                if (add_swap()) {
                    goto retry;
                }
		else {
                        error = ENOSPC;
                        goto done;
                }
        }
	else {
		swapspc_cnt -= NPGCHUNK;
	}
	VASSERT(swapspc_cnt >= 0);
	rswapunlock();

	swaplock();
	if (get_swap(NPGCHUNK, &swappg, 1)) {
		st = &swaptab[swappg.dbd_swptb];
		st->st_site = DM_SOURCE(reqp);
		if (st->st_dev)
                	st->st_dev->sw_nfpgs -= NPGCHUNK;
                else
                        st->st_fsp->fsw_nfpgs -= NPGCHUNK;

		/* make sure the serve doesn't try to use this chunk */
		st->st_nfpgs = 0;

		resap->swaptab = swappg.dbd_swptb;
		swapunlock();
	} else {
		error = ENOSPC;
		swapunlock();
		rswaplock();
		swapspc_cnt += NPGCHUNK;
		VASSERT(swapspc_cnt <= swapspc_max);
		rswapunlock();
	}
done:
	dm_reply(resp, 0, error, NULL, NULL, NULL);
}

return_chunk(index)
	register int index;
{
	register struct dux_vmmesg *reqfp;
	register dm_message reqp, resp;
	register char error;

	/*allocate a request message*/
	reqp = dm_alloc(sizeof( struct dux_vmmesg), WAIT);

	/*convert to the network swap message structure*/
	reqfp = DM_CONVERT(reqp,struct dux_vmmesg);

	reqfp->swaptab = index;

	/*send the message synchronously and get back the result*/
	resp = dm_send(reqp, DM_SLEEP | DM_RELEASE_REQUEST,
			DM_CHUNKFREE, swap_site, DM_EMPTY,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	/*check for error*/
	if ( error = DM_RETURN_CODE(resp)) {
		printf("errorno : %d ", error);
		panic("return_chunk: error responded");

	}

	dm_release(resp,0);
}

/*
** called by chunk release for swapless clients to contact
** the swap server and return a chunk of swap space.
*/
dux_swapcontract(st)
	register swpt_t *st;
{
	int i;

	/* invalidate swap table entry */

	/* if releasing this chunk would make swapspc_cnt<0
	 * then don't do it now
	*/
 	rswaplock();
        if ((swapspc_cnt - NPGCHUNK) < 0) {
    		rswapunlock();
		return(0);
	}
	else {
		swapspc_cnt -= NPGCHUNK;
		swapspc_max -= NPGCHUNK;
	}
	VASSERT(swapspc_cnt >= 0);
        VASSERT(swapspc_cnt <= swapspc_max);
	rswapunlock();

	return_chunk(st->st_union.st_swptab);

	swaplock();
	st->st_dev->sw_nfpgs -= NPGCHUNK;
	unlink_swptb(st);
	/* can unlock here. st_nfpgs is already 0 so noone will
	 * use this entry. will not be seen as free until st_swpmp
	 * is set to 0.
	*/
	swapunlock();
	kfree(st->st_swpmp, M_SWAPMAP);
        st->st_nfpgs = 0;
        st->st_dev = 0;
        st->st_vnode = 0;
	st->st_free = 0;
        st->st_site = -1;
        st->st_swpmp = 0;

	return(1);
}

/*
** This routine is run by a kernel NSP to service remote calls
** to free a chunk to the common swap pool.
*/
serve_chunkfree(reqp)
	register dm_message reqp;
{
	register swpt_t *st;
	register char error;
	register struct dux_vmmesg *reqfp;

	if (vm_debug & DFUNC)
		printf("serve_chunkfree entered :\n");

	/*convert the request to a dux_vmmesg structure*/
	reqfp = DM_CONVERT(reqp,struct dux_vmmesg);

	error = 0;
	/*consistancy check */
	st = &swaptab[reqfp->swaptab];
	if (st >= swapMAXSWAPTAB) {
		error = ERANGE;

	} else if ((!st->st_dev && !st->st_fsp) || (!st->st_swpmp) ||
                     (st->st_site != DM_SOURCE(reqp))) {

		error = EINVAL;
	} else
		/* invalidate corresponding chunk map entry */
		release_swaptab(st);

	/* send the reply back*/
	dm_reply(NULL, 0, error, NULL, NULL, NULL);
}

/*
** This is the clean up code called by crash  recovery routines to
** clean up swap resources allocated to a crashed site.
*/
swap_cleanup(site)
	register site_t site;
{
	struct swaptab *st;

	for (st = &swaptab[0]; st < swapMAXSWAPTAB; st++)
		if (st->st_site == site)
			release_swaptab(st);
	/* return 0 so recovery code knows we we're successful */
	return(0);
}

/*
** release a swaptab entry. used by serve_chunkfree and swap_cleanup
** to get invalidate swap chunks.
*/
release_swaptab(st)
struct swaptab *st;
{
	rswaplock();
	swapspc_cnt += NPGCHUNK;
	VASSERT(swapspc_cnt <= swapspc_max);
	rswapunlock();
	swaplock();
	if (st->st_dev)
		st->st_dev->sw_nfpgs += NPGCHUNK;
	else
		st->st_fsp->fsw_nfpgs += NPGCHUNK;
	swaprem(st);
	swapunlock();
}
