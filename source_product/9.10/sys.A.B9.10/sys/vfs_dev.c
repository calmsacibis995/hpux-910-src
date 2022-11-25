/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_dev.c,v $
 * $Revision: 1.10.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/18 13:45:11 $
 */

/* HPUX_ID: @(#)vfs_dev.c	55.1		88/12/23 */

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

/*	@(#)vfs_dev.c 1.1 86/02/03 SMI	*/
/*      NFSSRC @(#)vfs_dev.c	2.1 86/04/15 */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/time.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/user.h"
#include "../ufs/inode.h"
#include "../h/mount.h"
#include "../dux/dux_dev.h"
#include "../dux/sitemap.h"
#include "../h/malloc.h"

#include "../h/kern_sem.h"
#ifdef	MP
extern	lock_t *devvp_lock;
#endif

/*
 * Convert a dev into a vnode pointer suitable for bio.
 */

struct dev_vnode {
	struct vnode dv_vnode;
	struct dev_vnode *dv_link;
	ushort dv_usecount;	/* this and next for mirroring */
	ushort dv_flags;
	struct sitemap dv_sites;
} *dev_vnode_headp = (struct dev_vnode *) 0;

#define DV_MWANT  1	/* mirror or reboot wants to block this device */
#define DV_PWANT  2	/* proc wants to use device */
#define DV_MBLOCK 4	/* mirror or reboot has blocked this device */

/* where various entities sleep */
#define dv_mirsleep	dv_usecount
#define dv_procsleep	dv_flags

/*
 * These routines implement disk freezing and unfreezing.
 * Freezing a disk means to block all access to it at a point
 * where the disk is consistent as far as fsck is concerned.
 * Because a single system call might write the disk several times,
 * freezing requires knowing when processes are in the middle
 * of such a transaction, and waiting until they are finished,
 * meanwhile blocking other processes who want to start a
 * transaction.
 * 
 * Freezing is used by disk mirroring when taking half of a disk
 * offline or unconfiguring it.  It is also used by reboot to
 * force the root file system into a consistent state.  
 * These modules call freeze_dev() to freeze the disk.  
 * Freeze_dev() allows current processes to finish
 * with the device, but prevents any new ones from starting.  When
 * all current processes are finished, freeze_dev() returns.
 * The mirror driver calls unfreeze_dev() when it's finished.
 *
 * Freeze_dev looks at dv_usecount, a new field in the device
 * vnode for the disk being frozen.  This counts the number of processes
 * in the middle of a file system transaction.  The count is maintained
 * by use_dev() and drop_dev().  These two routines frame most
 * ufs_* (* == rdwr, create, etc) routines in ufs_vnops.c.
 * Freeze_dev blocks new processes from passing through use_dev, and
 * waits for existing ones to come through drop_dev, using
 * WANTED/LOCKED bits and sleep/wakeup in the usual way.
 *
 * Because a process might try to lock the same device twice,
 * we keep track of the number of devices locked in u.u_devsused.
 * When a process calls use_dev(), and the freezer has set the
 * WANTED bit, normally, that process sleeps, but if its u.u_devsused
 * is > 0, it is allowed to proceed to prevent possible deadlock.
 * This would not work if a process ever tried to lock two different
 * devices at once (we couldn't guarantee that the disk would ever
 * get frozen), but in fact, this never happens.
 *
 * To keep this simpler, we allow only one block_dev() call at a time.
 * This restriction is enforced by the mirror driver.
 *
 * There are various subtleties here.  We don't block EVERY access to the
 * disk, because the mirror driver itself needs to call update.  So,
 * for example, ufs_inactive is not blocked at the top level; instead,
 * there is code that allows the routine to be executed in the case
 * where only an iupdat() is done, but not in the case where an itrunc()
 * occurs; update() can cause the first to happen, but not the second.
 */

int freezing;
struct proc *freeze_proc;
#define FWANT 1
#define FLOCK 2

/*
 * if device blocked, wait.
 * else inc usecount to register our presence.
 * To avoid deadlock, allow freezer to avoid sleeping.
 */

dvp_sleep(dvp)
	struct dev_vnode *dvp;
{
	ushort	flags;

	do {
		dvp->dv_flags |= DV_PWANT;
                sleep(&dvp->dv_procsleep, PINOD);
		flags = dvp->dv_flags;
	} while ( (u.u_procp != freeze_proc)	&&
		  ((flags & DV_MBLOCK)		||
		   ((flags & DV_MWANT) && u.u_devsused == 0 )));
}

/*
 * Make sure dvp is just a local variable, otherwise USE_DEV() will
 * incur extra overhead everytime dvp is evaluated. IE. don't do something
 * like USE_DEV(&a->b->c->dvp);
 */
#define USE_DEV(dvp)						\
{								\
	ushort	ud_flags;					\
	ud_flags = (dvp)->dv_flags;				\
	if ( (u.u_procp != freeze_proc)	&&			\
	     ((ud_flags & DV_MBLOCK)		||		\
	     ((ud_flags & DV_MWANT) && u.u_devsused == 0 ))) {	\
			dvp_sleep(dvp);				\
	}							\
        ++(dvp)->dv_usecount;					\
	++u.u_devsused;						\
	INC_SITECOUNT(&(dvp)->dv_sites,u.u_site);		\
}
void
use_dev_dvp(dvp)
	struct dev_vnode *dvp;
{
	USE_DEV(dvp);
}

void
use_dev_vp(vp)
	struct vnode *vp;
{
	struct dev_vnode *dvp = (struct dev_vnode *)(VTOI(vp)->i_devvp);
	USE_DEV(dvp);
}
/*
 * Make sure dvp is just a local variable, otherwise DROP_DEV() will
 * incur extra overhead everytime dvp is evaluated. IE. don't do something
 * like DROP_DEV(&a->b->c->dvp);
 */

#define DROP_DEV(dvp)							\
{									\
	if ((dvp)->dv_usecount == 0)					\
		panic("DROP_DEV ERROR");				\
	--(dvp)->dv_usecount;						\
	--u.u_devsused;							\
	DEC_SITECOUNT(&(dvp)->dv_sites,u.u_site);			\
	if ((dvp)->dv_usecount==0 && ((dvp)->dv_flags & DV_MWANT))	\
                wakeup( (caddr_t)&(dvp)->dv_mirsleep);			\
}

void
drop_dev_dvp(dvp)
	struct dev_vnode *dvp;
{
	DROP_DEV(dvp);
}

void
drop_dev_vp(vp)
	struct vnode *vp;
{
	struct dev_vnode *dvp = (struct dev_vnode *)(VTOI(vp)->i_devvp);
	DROP_DEV(dvp);
}

/*cleanup any remote references after a site failure*/
dev_cleanup(site)
site_t site;
{
	register struct dev_vnode *dvp;
	
	for (dvp = dev_vnode_headp; dvp; dvp = dvp->dv_link) {
		while (getsitecount(&dvp->dv_sites,site))
			drop_dev_dvp(dvp);
	}
}

/*
 * block this device and wait for users to finish with it.
 */

void
freeze_dev(dvp)
	struct dev_vnode *dvp;
{
	if (!DBG_dev_ok(dvp))
		return;

	/* only one process can freeze at a time */
	while (freezing & FLOCK)
	{
		freezing |= FWANT;
		sleep(&freezing, PRIBIO);
	}
	freezing |= FLOCK;
	freeze_proc = u.u_procp;

	/* request a device block */
	dvp->dv_flags |= DV_MWANT;

	/* wait for processes using this device to finish */
	while (dvp->dv_usecount != 0)
                sleep(&dvp->dv_mirsleep, PINOD);

	dvp->dv_flags &= ~DV_MWANT;
	dvp->dv_flags |= DV_MBLOCK;

}

#ifdef NEVER_CALLED
/*
 * same as block_dev, but blocks multiple devices atomically.
 * arg is a NULL-terminated array of dev_vnode ptrs.
 */
void
freeze_mdev(dvpp)
	struct dev_vnode **dvpp;
{
	struct dev_vnode **tdvpp;
	struct dev_vnode *dvp;
	caddr_t sleepaddr;

	/* only one process can freeze at a time */
	while (freezing & FLOCK)
	{
		freezing |= FWANT;
		sleep(&freezing, PRIBIO);
	}
	freezing |= FLOCK;
	freeze_proc = u.u_procp;
loop:
	sleepaddr = (caddr_t) 0;
	for (tdvpp = dvpp; (dvp = *tdvpp) != NULL; ++tdvpp) {
		dvp->dv_flags |= DV_MWANT;
		if (dvp->dv_usecount != 0)
			sleepaddr = (caddr_t) &dvp->dv_mirsleep;
        }
	if (sleepaddr) {
		sleep(sleepaddr, PINOD);
		goto loop;
	}
	for (tdvpp = dvpp; (dvp = *tdvpp) != NULL; ++tdvpp) {
		dvp->dv_flags &= ~DV_MWANT;
		dvp->dv_flags |= DV_MBLOCK;
	}
}
#endif /* NEVER_CALLED */

/*
 * same as block_dev, but blocks all devices atomically.
 * This is used for a reboot.
 */
void
freeze_alldevs()
{
	struct dev_vnode *dvp;
	caddr_t sleepaddr;
	register struct mount *mp;
	register struct mounthead *mhp;
	extern struct vnode *devtovp();

	/* only one process can freeze at a time */
	while (freezing & FLOCK)
	{
		freezing |= FWANT;
		sleep(&freezing, PRIBIO);
	}
	freezing |= FLOCK;
	freeze_proc = u.u_procp;
loop:
	sleepaddr = (caddr_t) 0;
	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
	{
		for (mp = mhp->m_hforw; mp != (struct mount *)mhp;
		     mp = mp->m_hforw)
		{
			if ((mp->m_flag & MINUSE) == 0 || mp->m_dev == NODEV ||
			    bdevrmt(mp->m_dev))
				continue;
			dvp = (struct dev_vnode*)devtovp(mp->m_dev);
			/*
			 *devtovp increments the reference count on the device
			 *vnode.  We don't really want that here so we release
			 *it immediately.  Actually, since this function is only
			 *called before reboot, it isn't necessary, but we might
			 *as well be clean, in case someone finds another use
			 *for this routine
			 */
			VN_RELE(dvp);
			dvp->dv_flags |= DV_MWANT;
			if (dvp->dv_usecount != 0)
				sleepaddr = (caddr_t)&dvp->dv_mirsleep;
		}
        }
	if (sleepaddr) {
		sleep(sleepaddr, PINOD);
		goto loop;
	}
	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
	{
		for (mp = mhp->m_hforw; mp != (struct mount *)mhp;
		     mp = mp->m_hforw)
		{
			if ((mp->m_flag & MINUSE) == 0 || mp->m_dev == NODEV)
				continue;
			dvp = (struct dev_vnode *)devtovp(mp->m_dev);
			/*
			 *devtovp increments the reference count on the device
			 *vnode.  We don't really want that here so we release
			 *it immediately.  Actually, since this function is only
			 *called before reboot, it isn't necessary, but we might
			 *as well be clean, in case someone finds another use
			 *for this routine
			 */
			VN_RELE(dvp);
			dvp->dv_flags &= ~DV_MWANT;
			dvp->dv_flags |= DV_MBLOCK;
		}
	}
}

/*
 * unblock this device and wake up anyone waiting for it
 */
void
unfreeze_dev(dvp)
	struct dev_vnode *dvp;
{
	dvp->dv_flags &= ~DV_MBLOCK;
        if (dvp->dv_flags & DV_PWANT) {
                dvp->dv_flags &= ~DV_PWANT;
                wakeup( (caddr_t)&dvp->dv_procsleep);
        }
	/* wake up other procs waiting to freeze */
	freezing &= ~FLOCK;
	if (freezing & FWANT)
	{
		freezing &= ~FWANT;
		wakeup( (caddr_t)&freezing);
	}
	freeze_proc = NULL;
}

/*
 * a routine for debugging only; should be removed when finished.
 * takes a dev, ensures that it's real before using it.
 */
DBG_dev_ok(dvp)
	struct dev_vnode *dvp;
{
	struct dev_vnode *tdvp;

	for (tdvp = dev_vnode_headp; tdvp; tdvp = tdvp->dv_link) 
		if (tdvp == dvp)
			return 1;
	printf("ERROR: BAD DEVICE VNODE PTR 0x%x\n", dvp);
	panic("DBG_dev_ok");
	return 0;	
}

devtovp_badop()
{
	panic("devtovp_badop");
}

/*ARGSUSED*/
int
devtovp_strategy(bp)
	struct buf *bp;
{
	sv_sema_t save_sema;

	p_io_sema(&save_sema);
	(*bdevsw[major(bp->b_vp->v_rdev)].d_strategy)(bp);
	v_io_sema(&save_sema);
	return(0);
}

/*ARGSUSED*/
int
devtovp_inactive(vp)
	struct vnode *vp;
{
	/* could free the vnode here */
	return(0);
}

int 
devtovp_open(dev)
	dev_t dev;
{
	sv_sema_t save_sema;
	int ret;

	p_io_sema(&save_sema);
	ret = ((*bdevsw[major(dev)].d_open)(dev));
	v_io_sema(&save_sema);
	return ret;
}

int
devtovp_close(dev)
	dev_t dev;
{
	sv_sema_t save_sema;
	int ret;

	p_io_sema(&save_sema);
	ret = ((*bdevsw[major(dev)].d_close)(dev));
	v_io_sema(&save_sema);
	return ret;
}

struct vnodeops dev_vnode_ops = {
	devtovp_open,
	devtovp_close,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_inactive,
	devtovp_badop,
	devtovp_strategy,
	devtovp_badop,		/* bread() */
	devtovp_badop,		/* brelse() */
	devtovp_badop,		/* pathsend() */
#ifdef ACLS
	devtovp_badop,		/* setacl() */
	devtovp_badop,		/* getacl() */
#endif ACLS
#ifdef POSIX
	devtovp_badop,		/* pathconf() */
	devtovp_badop,		/* fpathconf() */
#endif POSIX
	devtovp_badop,		/* lockctl() */
	devtovp_badop,		/* lockf() */
	devtovp_badop,		/* fid() */
};

struct vnode *
devtovp(dev)
	dev_t dev;
{
	register struct dev_vnode *dvp, *dvp2;
	register struct dev_vnode *endvp;

	SPINLOCK(devvp_lock);
	for (dvp = dev_vnode_headp; dvp; dvp = dvp->dv_link) {
		if (dvp->dv_vnode.v_rdev == dev) {
			SPINUNLOCK(devvp_lock);
			VN_HOLD(&dvp->dv_vnode);
			return (&dvp->dv_vnode);
		}
	}
	SPINUNLOCK(devvp_lock); /* alloc may sleep */
	/* devtovp() can be called  under interrupt
	 * Otherwise M_WAITOK is fine. Since we are normally
	 * NOT on the istack and therefore call
	 * with M_WAITOK which will not fail, panic
	 * on failure.
	 */
	MALLOC(dvp, struct dev_vnode *, sizeof(struct dev_vnode), M_DYNAMIC,ON_ISTACK?M_NOWAIT:M_WAITOK);
	if (dvp == NULL)
	  panic("devtovp: no memory available");
	bzero((caddr_t)dvp, sizeof(struct dev_vnode));
	dvp->dv_vnode.v_count = 1;
	dvp->dv_vnode.v_op = &dev_vnode_ops;
	dvp->dv_vnode.v_fstype = VDEV_VN;
	dvp->dv_vnode.v_rdev = dev;
	endvp = (struct dev_vnode *)0;
	SPINLOCK(devvp_lock);
	for (dvp2 = dev_vnode_headp; dvp2; dvp2 = dvp2->dv_link)
		endvp = dvp2;
	if (endvp != (struct dev_vnode *)0) {
		endvp->dv_link = dvp;
	} else {
		dev_vnode_headp = dvp;
	}
	SPINUNLOCK(devvp_lock);
	return (&dvp->dv_vnode);
}

