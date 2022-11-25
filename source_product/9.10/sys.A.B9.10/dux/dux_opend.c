/* $Header: dux_opend.c,v 1.9.83.3 93/09/17 16:41:57 root Exp $ */
/* HPUX_ID: @(#)dux_opend.c     55.1            88/12/23 */

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

#ifndef _SYS_STDSYMS_INCLUDED
#    include "../h/stdsyms.h"
#endif /* _SYS_STDSYMS_INCLUDED  */

#ifdef hp9000s800
#define msg_printf	printf
#endif /* hp9000s800 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/conf.h"
#include "../h/kernel.h"
#include "../h/errno.h"
#include "../h/malloc.h"
#include "../dux/dux_dev.h"

#ifdef FULLDUX
#include "../dux/sitemap.h"
#include "../dux/dm.h"
#endif /* FULLDUX */

#include "../h/devices.h"
#include "../h/kern_sem.h"

#define clonesmajor(dev)        (cdevsw[major(dev)].d_flags & C_CLONESMAJOR)

/*
 * lookupdev() --
 *     find a device entry in the device hash table.  Returns 0 if
 *     not found.
 */
dtaddr_t
lookup_dev(dev, mode)
dev_t dev;
u_int mode;
{
    register dtaddr_t dt;

    /*
     * Simply search for a match.
     */
    for (dt = devhash[DEVHASH(dev)]; dt != (dtaddr_t)0; dt = dt->next)
    {
	if (dt->dt_dev == dev && dt->dt_mode == mode)
	    return dt;
    }

    return (dtaddr_t)0;
}

/*
 * alloc_dev() --
 *     allocate a device entry in the device hash table.  If the
 *     item is already in the table, its reference count is merely
 *     incremented.
 */
dtaddr_t
#ifdef  FULLDUX
alloc_dev(dev, mode, site)
register dev_t dev;
register u_int mode;
site_t   site;
#else  /* FULLDUX */
alloc_dev(dev, mode)
register dev_t dev;
register u_int mode;
#endif /* FULLDUX */
{
    register dtaddr_t dt;
    register dtaddr_t pdt;
    dtaddr_t ndt = (dtaddr_t)0;
    u_long hash = DEVHASH(dev);
    u_short id;

    /*
     * We wish to allocate a new dev.  If it already exists, simply
     * increment its reference count, if not, allocate a new entry
     * and add it to the hash table.
     *
     * Each element in the list must have a unique id.  We compute
     * the id value for a new element as we search the list.  This
     * id value is used by DUX to "index" into the device table.
     */
again:
    pdt = (dtaddr_t)0;
    id = 0;

    for (dt = devhash[hash]; dt != (dtaddr_t)0; dt = dt->next)
    {
	if (dt->dt_dev == dev && dt->dt_mode == mode)
	{
	    /*
	     * Found it!  increment the reference count and return.
	     */
#ifdef FULLDUX
	    updatesitecount(&(dt->dt_map), site, 1);
#else /* FULLDUX */
	    dt->dt_cnt++;
	    if (dt->dt_cnt <= 2 && mode == IFBLK)
	    {
		/*
		 * If the count is now 2, another process may be trying
		 * to close the device, and may be in the process of
		 * flushing the buffer cache.  We do not want to let
		 * anyone start putting things in the buffer cache while
		 * another process is trying to flush it, so we wait for
		 * the semaphore and then immediately release it.  If
		 * someone is not closing the device, we have not hurt
		 * anything by performing the extra semaphore operations
		 * here.
		 */
		b_psema(&dt->dt_sema);
		b_vsema(&dt->dt_sema);
	    }
#endif /* FULLDUX */

	    /*
	     * If ndt is not NULL, then we have MALLOCed space for
	     * a new entry already.  However, while we were asleep,
	     * someone added our entry to the table, so now we do not
	     * need that space.
	     */
	    if (ndt != (dtaddr_t)0)
		FREE(ndt, M_DYNAMIC);

	    return dt;
	}

	/*
	 * In case we need to add a new entry, we must make up a
	 * unique id for it.  We use the lowest number that is not
	 * already in use in this chain.  To make this possible,
	 * items are always inserted in sorted order (by their id)
	 * into the hash chain.
	 */
	if (id == dt->dt_id)
	{
	    id = dt->dt_id + 1;
	    pdt = dt;
	}
    }

    /*
     * We can only have DUX_DEVIDMASK elements per hash chain.
     */
    if (id > DUX_DEVIDMASK)
	return (dtaddr_t)0;

    /*
     * A new element must be added to the hash table.  We need to
     * MALLOC up space for one, and waiting is OK.  However, if we
     * wait, then we would have to re-hash to make sure someone did
     * not sneak in while we were asleep.  So, first we try to MALLOC
     * without waiting.  If that succeeds, then we can simply add the
     * item to the list.  Else, we MALLOC with WAITOK, and then go
     * back to the beginning.
     */
    if (ndt == (dtaddr_t)0)
    {
	MALLOC(ndt, dtaddr_t, sizeof *ndt, M_DYNAMIC, M_NOWAIT);

	/*
	 * If we did not get the memory we wanted, call MALLOC again
	 * but this time say we can wait.  Unforunately, in this case
	 * we have to re-search our hash table in case someone added
	 * our entry while we were sleeping.
	 */
	if (ndt == (dtaddr_t)0)
	{
	    MALLOC(ndt, dtaddr_t, sizeof *ndt, M_DYNAMIC, M_WAITOK);
	    goto again;
	}
    }

    ndt->dt_flags = D_INUSE;
    ndt->dt_dev = dev;
    ndt->dt_mode = mode;
    ndt->dt_id = id;
#ifdef FULLDUX
    clearmap(&(ndt->dt_map));
    updatesitecount(&(ndt->dt_map), site, 1);
#else
    ndt->dt_cnt = 1;
#endif
    b_initsema(&ndt->dt_sema, 1, DORDER, "device sema");

    /*
     * Link the new element into the list, in the correct spot as
     * computed above (directly after pdt, or at the head of the
     * list if pdt is 0).
     */
    if (pdt == (dtaddr_t)0)
    {
	ndt->next = devhash[hash];
	devhash[hash] = ndt;
    }
    else
    {
	ndt->next = pdt->next;
	pdt->next = ndt;
    }

    return ndt;
}

/*
 * dev_rele() --
 *    remove an entry from the device hash table.  We have to compute
 *    the hash index and search the hash table in order to remove an
 *    element.  We could have used a doubly linked list, but the hash
 *    chains are short enough that it is not worth the extra space and
 *    complexity that a doubly linked list would require.
 */
void
dev_rele(ddt)
dtaddr_t ddt;
{
    dtaddr_t dt, pdt;
    u_long hash = DEVHASH(ddt->dt_dev);

    dt = devhash[hash];
    pdt = (dtaddr_t)0;

#ifdef OSDEBUG
    if (ddt == (dtaddr_t)0)
    {
	panic("dev_rele: ddt is NULL");
	return;
    }

    if (ddt->dt_cnt < 0)
    {
	printf("ddt = 0x%08x, ddt->dt_cnt = %d\n", ddt, ddt->dt_cnt);
	panic("dev_rele: ddt->dt_cnt is < 0");
	return;
    }

    if (!(ddt->dt_flags & D_INUSE))
    {
	printf("ddt = 0x%08x, ddt->dt_flags = 0x%08\n",
	    ddt, ddt->dt_flags);
	panic("dev_rele: ddt is not D_INUSE");
	return;
    }
#endif /* OSDEBUG */

    /*
     * Search for this element in the hash chain so that we know
     * what the previous entry is.
     */
    while (dt != ddt && dt != (dtaddr_t)0)
    {
	pdt = dt;
	dt = dt->next;
    }

    /*
     * Item was not in hash table!
     */
    if (dt == (dtaddr_t)0)
    {
	printf("dev_rele: ddt = 0x%08x\n", ddt);
	panic("dev_rele: ddt not in table");
	return;
    }

    if (pdt == (dtaddr_t)0)
	devhash[hash] = ddt->next;
    else
	pdt->next = ddt->next;

#ifdef OSDEBUG
    ddt->dt_flags = 0;
    ddt->next = (dtaddr_t)0;
#endif
    FREE(ddt, M_DYNAMIC);
}

#ifdef FULLDUX
/*
 * opend() --
 *     opens the device dev at site of type mode (IFCHR or IFBLK).
 *     A slot in the devices table is allocated if the device
 *     <maj,min,mod> is not already open.  If the d_open routine
 *     fails, the entry is releases.
 */

int
opend(devp, mode, site, omode, minnump)
register dev_t	*devp;
u_int		mode;
register site_t site;
register u_int  omode;
int		*minnump;

#else  /* FULLDUX */

int
opend(devp, mode, omode, minnump)
register dev_t  *devp;
u_int		mode;
register u_int  omode;
int		*minnump;

#endif /* FULLDUX */
{
    register dtaddr_t   dt;
    register u_int	maj;
    register int	error;
    sv_sema_t		opendSS;
    sv_sema_t		sema_save;
    register dev_t	dev, newdev;

    PXSEMA(&filesys_sema, &opendSS);

#ifdef	FULLDUX
    if (site == 0)
	    site = my_site;
    if (site != my_site)
    {
	error = rdu_opend(devp, mode, site, omode, minnump);
	VXSEMA(&filesys_sema, &opendSS);
	return error;
    }
#endif /*FULLDUX*/

    /*
     * map lu to mgr_index and bind drivers, pre_open can change
     * the value of dev.
     */
    if ((error = pre_open(devp, &mode, minnump)) != 0) {
	VXSEMA(&filesys_sema, &opendSS);
	return(error);
    }

    /* pre_open can change the value of dev */
    dev = *devp;
    /* ensure that minnump points to ENTIRE device number */
    if (minnump)
        *minnump = dev;

#ifdef FULLDUX
    dt = alloc_dev(dev, mode, site);
#else  /* FULLDUX */
    dt = alloc_dev(dev, mode);
#endif /*FULLDUX*/
    if (dt == (dtaddr_t)0)
    {
	VXSEMA(&filesys_sema, &opendSS);
	return EIO;
    }

    maj = major(dev);

    if ((mode & IFMT) == IFBLK)
    {
	if (maj < nblkdev)
	{
#ifdef _WSIO
	    if (bdevsw[maj].d_flags & C_ALLCLOSES)
		dt->dt_flags |= D_ALLCLOSES;
#endif /* _WSIO */
	    p_io_sema(&sema_save);
	    error = (*bdevsw[maj].d_open)(dev, omode);
	    v_io_sema(&sema_save);
	}
	else
	    error = ENXIO;
    }
    else /* mode == IFCHR */
    {
	if (maj < nchrdev)
	{
	    if(cdevsw[maj].d_flags & C_ALLCLOSES)
	        dt->dt_flags |= D_ALLCLOSES;
	    p_io_sema(&sema_save);
	    {
		error = (*cdevsw[maj].d_open)(dev, omode, minnump);
	    }
	    v_io_sema(&sema_save);

	    /*
	     * If this was a multiple open device that changed
	     * our device to use a new device number,
	     * then we need to delete the old device entry
	     * from the device table and replace it with
	     * a new entry.
	     * Note: we pass back ENTIRE dev number
	     * through *minnump, even though driver passes
	     * only the minor number (unless clonesmajor is true).
	     */
	    if (minnump == NULL)
		newdev = dev;
	    else
	    if (clonesmajor(dev))
		newdev = *minnump;
	    else {
		newdev = makedev(major(dev), *minnump);
		/* pass ENTIRE dev number back to caller */
		*minnump = newdev;
	    }
	    if (!error && newdev != dev)
	    {
#ifdef FULLDUX
		updatesitecount(&(dt->dt_map), site, -1);
		if (gettotalsites(&(dt->dt_map)) == 0)
		    dev_rele(dt);

		dt = alloc_dev(newdev, mode, site);
#else  /* FULLDUX */
		if (--dt->dt_cnt == 0)
		    dev_rele(dt);

		dt = alloc_dev(newdev, mode);
#endif /* FULLDUX */
		if (dt == NULL)
		{
		    p_io_sema(&sema_save);
		    (*cdevsw[major(newdev)].d_close)(newdev, omode);
		    v_io_sema(&sema_save);
		    VXSEMA(&filesys_sema, &opendSS);
		    return ENXIO;
		}
		/* Make sure that D_ALLCLOSES is set if needed on the new
		 * device.
		 */
	        if(cdevsw[maj].d_flags & C_ALLCLOSES)
	            dt->dt_flags |= D_ALLCLOSES;
	    }
	}
	else
	    error = ENXIO;
    }

    if (error != 0)
    {
#ifdef FULLDUX
	updatesitecount(&(dt->dt_map), site, -1);
	if (gettotalsites(&(dt->dt_map)) == 0)
	    dev_rele(dt);
#else  /* FULLDUX */
	if (--dt->dt_cnt == 0)
	    dev_rele(dt);
#endif /* FULLDUX */
    }

    VXSEMA(&filesys_sema, &opendSS);
    return error;
}

#ifdef FULLDUX
/* If FULLDUX is around, closed will close a local device reference
 * from a possibly remote site
 */

int
closed(dev, mode, flags, site)
register dev_t  dev;
register u_int  mode, flags;
register site_t site;
#else  /* FULLDUX */

int
closed(dev, mode, flags)
register dev_t  dev;
register u_int  mode, flags;
#endif /* FULLDUX */
{
    register dtaddr_t dt;
    register u_int    error;
    struct vnode *dev_vp;
    int last_close;
    sv_sema_t closedSS;
    sv_sema_t sema_save;

    PXSEMA(&filesys_sema, &closedSS);
    if ((dt = lookup_dev(dev, mode)) == NULL) {
	VXSEMA(&filesys_sema, &closedSS);
	return ENXIO;
    }

    /*
     * If this is the last close, we cannot free the entry until the
     * very end of this routine.  So, if the count is currently 1, we
     * record this fact and defer the decrement of the count until
     * later.
     *
     * If this is not the last close, we decrement the count so that
     * subsequent closes will detect the last close properly (in case
     * we sleep).
     */
#ifdef FULLDUX
    if (gettotalsites(&(dt->dt_map)) == 1)
    {
	last_close = 1;
    }
    else
    {
	last_close = 0;
	updatesitecount(&(dt->dt_map), site, -1);
    }
#else  /* FULLDUX */
    if (dt->dt_cnt == 1)
    {
	last_close = 1;
    }
    else
    {
	last_close = 0;
	dt->dt_cnt--;
    }
#endif /* FULLDUX */

    /*
     * On last close of a block device (that isn't mounted)
     * we must invalidate any in core blocks, so that
     * we can, for instance, change floppy disks.
     */
    if (last_close && mode == IFBLK)
    {
	dev_vp = devtovp(dev);
	b_psema(&dt->dt_sema);
	bflush(dev_vp);
	binval(dev_vp);
	b_vsema(&dt->dt_sema);
	VN_RELE(dev_vp);
    }

    p_io_sema(&sema_save);

    if (last_close || (dt->dt_flags & D_ALLCLOSES))
    {
	if (mode == IFBLK)
	    error = (*bdevsw[major(dev)].d_close)(dev, flags);
	else
	{
	    {
		error = (*cdevsw[major(dev)].d_close)(dev, flags);
	    }
	}
    }
    v_io_sema(&sema_save);

    /*
     * Now that we are done sleeping, we can decrement the count if we
     * were performing a last close.  If the count is then 0, we release
     * the device.  The count is not necessarily 0, since someone could
     * have performed an open on the device while we were sleeping
     * trying to close it.
     */
    if (last_close)
    {
#ifdef FULLDUX
	updatesitecount(&(dt->dt_map), site, -1);
	if (gettotalsites(&(dt->dt_map)) == 0)
#else  /* FULLDUX */
	if (--dt->dt_cnt == 0)
#endif /* FULLDUX */
	{
	    dev_rele(dt);
	}
    }

    VXSEMA(&filesys_sema, &closedSS);

    return error;
}

/*
 * devindex() --
 *    Given a local dev_t and mode, compute the DEVINDEX part for the
 *    remote dev_t.
 */
int
devindex(dev, mode)
dev_t dev;
u_int mode;
{
    u_long hash = DEVHASH(dev);
    dtaddr_t dt = devhash[hash];

    for (dt = devhash[hash]; dt != (dtaddr_t)0; dt = dt->next)
    {
	if (dt->dt_dev == dev && dt->dt_mode == mode)
	{
	    /*
	     * For DUX, the devindex is the hash bucket number combined
	     * with the unique id of the element.
	     */
	    return DUX_MAKE_DEVHASH(hash, dt->dt_id);
	}
    }

    return -1;
}

/*
 * deventry() --
 *     return the device entry corresponding to a remote dux device.
 */
dtaddr_t
deventry(dev)
dev_t dev;
{
    u_short id = DUX_GET_DEVID(dev);
    dtaddr_t dt;

    for (dt = devhash[DUX_GET_DEVHASH(dev)]; dt != (dtaddr_t)0; dt = dt->next)
	if (dt->dt_id == id)
	    return dt;

    return (dtaddr_t)0;
}

/*
 * localdev() --
 *    map a remote dux device id to the corresponding dev_t on
 *    the local system.
 *
 * NOTE: we could have called deventry() here, but we have duplicated
 *       the code for the sake of efficiency.
 */
dev_t
localdev(dev)
dev_t dev;
{
    u_short id = DUX_GET_DEVID(dev);
    dtaddr_t dt;

    for (dt = devhash[DUX_GET_DEVHASH(dev)]; dt != (dtaddr_t)0; dt = dt->next)
	if (dt->dt_id == id)
	    return dt->dt_dev;

    return NODEV;
}
