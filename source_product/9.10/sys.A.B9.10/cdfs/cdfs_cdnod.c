/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/cdfs/RCS/cdfs_cdnod.c,v $
 * $Revision: 1.6.83.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/09/14 19:34:59 $
 */

/* HPUX_ID: @(#)cdfs_knode.c	54.5		88/12/08 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988, 1988 Hewlett-Packard Company.
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


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../nfs/nfs.h"
#include "../h/vfs.h"
#include "../h/uio.h"
#include "../h/vnode.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../h/vfs.h"
#include "../h/mount.h"
#include "../h/kernel.h"
#include "../h/conf.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../dux/dmmsgtype.h"
#include "../h/kern_sem.h"

#define	CDNOHSZ	63
#if	((CDNOHSZ&(CDNOHSZ-1)) == 0)
#define	CDNOHASH(dev,cdno)	(((dev)+(cdno))&(CDNOHSZ-1))
#else
#define	CDNOHASH(dev,cdno)	(((unsigned)((dev)+(cdno)))%CDNOHSZ)
#endif

extern struct buf *cdfs_readb(); 
extern struct vnodeops cdfs_vnodeops;	/* vnode operations for cdfs */

/*
 * Cdnode hash LRU chains:
 *
 *
 *    +------------------------------<-------------------------------------+
 *    |   +---------+          +----------+                +----------+    |
 *    +-> | head[0] | -------> | forw ptr | --->      <--- | forw ptr | -->+
 *        +---------+          +----------+      ...       +----------+
 *    +-- | head[1] | <------- | back ptr | <---      <--- | back ptr | <--+
 *    |   +---------+          +----------+                +----------+    |
 *    +------------------------------>-------------------------------------+
 *
 *
 *    hash chain header          cdnode          ...          cdnode
 */
union cdhead {				/* cdnode LRU cache */
	union  cdhead *cdh_head[2];
	struct cdnode *cdh_chain[2];
} cdhead[CDNOHSZ];

struct cdnode *cdfreeh, **cdfreet;


/******************************************************************************
 * cdinit()
 *
 * Initialize hash links for cdnodes and build cdnode free list.
 * This routine must be called at the boot up time.
 *
 * Global variables used: cdnode, cdnodeNCDNODE.
 *
 */

cdhinit()
{
	register int i;
	register struct cdnode *cdp;
	register union  cdhead *cdh = cdhead;
	register int	table_size;

        /*
         * Allocate space for "cdnode"s and zero out the space before we
         * start.  "cdnode" marks the beginning of "cdnode" array while
         * cdnodeNCDNODE marks the end.
         */
	table_size = sizeof(struct cdnode)*ncdnode;
	cdnode = (struct cdnode *) kmalloc(table_size, M_CDNODE, M_NOWAIT);
	cdnodeNCDNODE = (struct cdnode *) ((int) cdnode + table_size);
	bzero((caddr_t)cdnode, table_size);

	cdp = cdnode;

        /*
         * Intialize the headers for hash chains.
         */
	for (i = CDNOHSZ; --i >= 0; cdh++) {
		cdh->cdh_head[0] = cdh;
		cdh->cdh_head[1] = cdh;
	}

        /*
         * Now it's time to initialize all "cdnode"s and put them in the
         * free list.  We put a cdnode to free list and add the others in
         * a loop.
         */
	cdfreeh = cdp;
	cdfreet = &cdp->cd_freef;
	cdp->cd_freeb = &cdfreeh;
	cdp->cd_forw = cdp;
	cdp->cd_back = cdp;

        /*
         * The vnode associated with this cdnode in part of the cdnode.  We
         * initialize the vnode part of the cdnode.
         */
	cdp->cd_vnode.v_data = (caddr_t)cdp;
	cdp->cd_vnode.v_op = &cdfs_vnodeops;
	cdp->cd_vnode.v_fstype = VCDFS;
	cdp->cd_flag = CDBUFVALID|CDPAGEVALID;

	for (i = ncdnode; --i > 0; ) {
		++cdp;
		cdp->cd_forw = cdp;
		cdp->cd_back = cdp;
		*cdfreet = cdp;
		cdp->cd_freeb = cdfreet;
		cdfreet = &cdp->cd_freef;
		cdp->cd_vnode.v_data = (caddr_t)cdp;
		cdp->cd_vnode.v_op = &cdfs_vnodeops;
		cdp->cd_vnode.v_fstype = VCDFS;
		cdp->cd_flag = CDBUFVALID|CDPAGEVALID;
	}
	cdp->cd_freef = NULL;
}

/******************************************************************************
 * cdfind()
 *
 * Search an cdnode (given device and cdnode number in core.
 * Return the cdnode pointer if found.  Else, return NULL.
 *
 */
struct cdnode *
cdfind(dev, cdno)
	dev_t dev;
	cdno_t cdno;
{
	register struct cdnode *cdp;
	register union  cdhead *cdh;

        /*
         * Find the head of the hash chain and follow the chain to find it.
         */
	cdh = &cdhead[CDNOHASH(dev, cdno)];
	for (cdp = cdh->cdh_chain[0]; cdp != (struct cdnode *)cdh; 
	     cdp = cdp->cd_forw)
	{
		if ((cdno==cdp->cd_num) && (dev==cdp->cd_dev))
			return (cdp);
	}
	return ((struct cdnode *)0);
}

/*****************************************************************************
 * ecdget()
 *
 * ecdget() is called when a cdnode is needed.  If availabe, insert it into
 * the chain specified.  If no chain specified (cdh == NULL), it is in a
 * chain by itself and cannot be find through hash table.
 *
 */
struct cdnode *
ecdget(cdh)
register union  cdhead *cdh;
{
	register struct cdnode *cdp;
	register struct cdnode *cdq;

        /*
         * If there is no free cdnode available, look through name cache
         * to see if we can free one by removing a cache that tight up
         * a cdnode.  Since we share purge with the other file systems
         * we might not have a free cdnode after a purge.  Performance
         * can be improved by change dnlc_purge1() to allow specified
         * the kind of node (cdnode or inode) needed.  This should be
         * done as soon as timing is appropriate. Sigh :-(
         */
	while (cdfreeh == NULL) {
		if (dnlc_purge1() == 0) {	/* XXX */
			break;
		}
	}


	/*
         * If we still do not have one, look through the buffer cache and
         * see if we can get one by free up buffers.
	 */
	while (cdfreeh == NULL) {
		if (bufvpfree() == 0) {
			break;
		}
	}


        /*
         * If we run out of cdnode, we print appropriate message and return
         * error to users.  Kernel auto configuration should help in this
         * area. :-(
         */
	if ((cdp = cdfreeh) == NULL) {
		tablefull("cdnode");
		u.u_error = ENFILE;
		return(NULL);
	}

        /*
         * Ho! Ho! we got one.  Yank it out of the free list.
         */
	if (cdq = cdp->cd_freef)
		cdq->cd_freeb = &cdfreeh;
	cdfreeh = cdq;
	cdp->cd_freef = NULL;
	cdp->cd_freeb = NULL;

	/*
	 * Now to take cdnode off the hash chain it was on
	 * (initially, or after an iflush, it is on a "hash chain"
	 * consisting entirely of itself, and pointed to by no-one,
	 * but that doesn't matter), and put it on the chain for
	 * its new (cdno, dev) pair
	 */

	/*
	 * The following lines take the place of the vax remque
	 * instruction:  remque(cdp);
	 */
	cdp->cd_back->cd_forw = cdp->cd_forw;
	cdp->cd_forw->cd_back = cdp->cd_back;

	if (cdh) {
                /*
                 * We are giving a hash chain header.  Insert the cdnode
                 * on the beginning of the chain. (Between the header and
                 * the first node.  (see picture of how chain list above the
                 * definition of struct cdhead.)
                 */
		cdp->cd_back = (struct cdnode *) cdh;
		cdp->cd_forw = cdh->cdh_chain[0];
		cdh->cdh_head[0]->cdh_head[1] = (union cdhead *) cdp;
		cdh->cdh_chain[0] = cdp;
	} else {
                /*
                 * We intend to be solitude.
                 */
		cdp->cd_back = cdp;
		cdp->cd_forw = cdp;
	}

	return (cdp);
}

/******************************************************************************
 * cdeget()
 *
 * Given device, cdnode_number pair, return the locked cdnode identified.
 * If it is in core, it is easy.  Else, get an empty cnode and filled in
 * the info we have, such as, cdnode number, device number, reference count,
 * file system pointer, flags and initialize the directory search offset.
 *
 * Note: If the cdnode is not in core, the newly allocated one will be returned
 *       with a reference count of zero.  It is the responsibility of the
 *       caller to up the count.  On the other hand, if it is in core, we
 *       will up the count.
 */
struct cdnode *
cdeget(dev,cdfs,cdno)
	dev_t dev;
	struct cdfs *cdfs;
	cdno_t cdno;
{
	register struct cdnode *cdp;
	register union  cdhead *cdh;
	register struct cdnode *cdq;

        /*
         * First we need to find out if the cdnode interested is in core or
         * not.  This is done by search through the hash chain that the cdnode
         * should be in (if exist).
         */
loop:
	cdh = &cdhead[CDNOHASH(dev, cdno)];
	for (cdp = cdh->cdh_chain[0]; cdp != (struct cdnode *)cdh; 
	     cdp = cdp->cd_forw)
	{
                /*
                 * A cdnode is identified by device, cdnode number pair.
                 */
		if ((cdno == cdp->cd_num) && (dev == cdp->cd_dev)) {
                        /*
                         * We found one.
                         */
			if ((cdp->cd_flag&CDLOCKED) != 0) {
                                /*
                                 * If someone else has it locked, set the
                                 * alarm and go to sleep.  When woken up,
                                 * re-start the search to avoid race
                                 * condition.
                                 */
				cdp->cd_flag |= CDWANT;
				sleep((caddr_t)cdp, PINOD);
				goto loop;
			}

			/*
			 * If cdnode is on free list, remove it.
			 */
			if ((cdp->cd_flag & CDREF) == 0) {
				if (cdq = cdp->cd_freef)
					cdq->cd_freeb = cdp->cd_freeb;
				else
					cdfreet = cdp->cd_freeb;
				*cdp->cd_freeb = cdq;
				cdp->cd_freef = NULL;
				cdp->cd_freeb = NULL;
			}
			/*
			 * mark cdnode locked and referenced and return it.
			 */
			cdp->cd_flag |= CDLOCKED | CDREF;
			cdp->cd_cdlocksite = u.u_site;
                        /*
                         * Up the reference count of the vnode.
                         */
			VN_HOLD(CDTOV(cdp));
			return(cdp);
		}
	}

        /*
         * Well, the cdnode we are looking for is not in core. We need a
         * new one.
         */
	cdp = ecdget(cdh);
	if (cdp == NULL)
		return (NULL);


        /*
         * Now, we fill in all the information we know about this cdnode.  The
         * rest of the infomation should be filled in by the caller.
         */
	cdp->cd_flag = CDLOCKED | CDREF;
	cdp->cd_cdlocksite = u.u_site;

	cdp->cd_dev = dev;

        /*
         * If this cdnode is gotten from someone eles, we down the reference
         * count of the device.
         */
	if (cdp->cd_devvp)
		VN_RELE(cdp->cd_devvp);

        /*
         * This is a new reference to the device, we call devtovp to up the
         * the reference count. (If no vnode for the device exists, devtovp()
         * will make one.
         */
	cdp->cd_devvp = devtovp(dev);

	cdp->cd_diroff = 0;
	cdp->cd_fs = cdfs;
	cdp->cd_dfs = 0;

	cdp->cd_num = cdno;
	cdp->cd_nextr = 0;	/* for read ahead. */
	return (cdp);
}

/******************************************************************************
 * cdget()
 *
 * cdget returns a lock cdnode given the cdnode number and device.  If the
 * cdnode requested is not in core, we get a new one and filled it with the
 * information needed from the disk.
 *
 * If we cannot get one due to resources limitation, we return NULL and
 * u.u_error has the error number.
 *
 * Panic: if the device is not mounted.  (Cannot happen)
 *        if the mount point return is inconsistant.  (Cannot happen)
 *        if the mounted file system has bad magic number (Cannot happen)
 */
struct cdnode *
cdget(dev, cdfs, cdno, pcdnop, cddirp)
	dev_t			dev;
	register struct cdfs	*cdfs;
	cdno_t 			cdno; 
	cdno_t			*pcdnop;
	register struct cddir	*cddirp;
{
	register struct cdnode	*cdp;
	register struct mount	*mp;
	register struct vnode	*vp;
	struct	 cdxar_iso	*cdip;
	struct	 cdxar_hsg	*cdhp;
	struct	 buf		*bp=0;
	char	 *cp1;
	u_int	 i;

        /*
         * First we need to get the mount point.  Then we can get the file
         * system structure we need.
         */
	mp = getmp(dev);
	if (mp == NULL) {               /* Mount point not found.  Bad news.  */
		panic("cdget: bad dev");
	}

	if (cdfs != (struct cdfs *) mp->m_bufp->b_un.b_fs) /* inconsistant.   */
		panic("cdget: bad fs");

        /*
         * Now we use device and cdnode number to get the cdnode.  If this
         * cdnode is in core, we will return to caller immediately.  If we
         * fail to get the cdnode, we return NULL right now.  If the cdnode
         * returned by cdeget() is newly allocated, we have more work to do.
         */
	cdp = cdeget(dev,cdfs,cdno);
	if (cdp == NULL || CDTOV(cdp)->v_count >= 1)
		return (cdp);

        /*
         * If the directory record of the file we are interested in, we need
         * to read in from the disk.  Most of the time, caller will pass it
         * in.
         */
	if (cddirp == 0) { /* cddir is not passed in */
	   bp = cdfs_readb(cdp->cd_devvp, (u_int)cdno, sizeof(struct cddir));
	
	   /*
	    * Check I/O errors
	    */
	   if (bp == (struct buf *)0) {
		/*
		 * the cdnode doesn't contain anything useful, so it would
		 * be misleading to leave it on its hash chain.
		 * 'cdput' will take care of putting it back on the free list.
		 */
   
		/*
		 * The following lines take the place of the vax remque
	 	 * instruction:  remque(cdp);
	 	 */
		cdp->cd_back->cd_forw = cdp->cd_forw;
		cdp->cd_forw->cd_back = cdp->cd_back;
		cdp->cd_forw = cdp;
		cdp->cd_back = cdp;

		/*
		 * we also loose its cd_number, just in case (as cdput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary .. kre
		 * (probably the two methods are interchangable)
		 */
		cdp->cd_num = 0;
		cdp->cd_pnum = 0;
		cdunlock(cdp);
		cdp->cd_flag &= CDBUFVALID|CDPAGEVALID;

                /*
                 * put it back to free list.
                 */
		if (cdfreeh) {
			*cdfreet = cdp;
			cdp->cd_freeb = cdfreet;
		} else {
			cdfreeh = cdp;
			cdp->cd_freeb = &cdfreeh;
		}
		cdp->cd_freef = NULL;
		cdfreet = &cdp->cd_freef;
		return(NULL);
	   }
	   cddirp = (struct cddir *)bp->b_un.b_addr;
	}
	cdp->cd_mount = mp;
	if ((cdp->cd_fs->cdfs_magic) == CDFS_MAGIC_ISO) {
		cdp->cd_format = ISO9660FS;
	} else if ((cdp->cd_fs->cdfs_magic) == CDFS_MAGIC_HSG) {
                /*
                 * convert the HSG into uniform format.  Fortunately, there
                 * are only a few differnece. (flags and timezone)
                 */
		cdp->cd_format = HSGFS;
		cddirp->cdd_flag = (u_char)cddirp->cdd_timezone;
		cddirp->cdd_timezone = 0;
	} else {
		panic("cdget: bad magic number");
	}

        /*
         * Fill in the data from directory entry.  Since some compilers
         * have alignment problem. bcopy() is used to copy some fields.
         */
	bcopy (cddirp->cdd_size, (caddr_t)&(cdp->cd_size), sizeof(u_int));
	cdp->cd_seq = cddirp->cdd_vol_seq;
	bcopy (cddirp->cdd_loc, (caddr_t)&(cdp->cd_loc), sizeof(u_int));
	cdp->cd_fusize = cddirp->cdd_unit_size;
	cdp->cd_fugsize = cddirp->cdd_lg_size;
	cdp->cd_xarlen = cddirp->cdd_xar_len;
	cdp->cd_fflag = cddirp->cdd_flag;		/*this is not cd_flag;*/
	cdp->cd_ftype = cdp->cd_fflag & CDFDIR ? CDFDIR : CDFREG;
	seconds(cddirp->cdd_year, cddirp->cdd_month, cddirp->cdd_day,
		cddirp->cdd_hour, cddirp->cdd_minute, cddirp->cdd_second,
		cddirp->cdd_timezone, &(cdp->cd_record_t));


        /*
         * Fix the vnode part of the cdnode.
         */
	vp = CDTOV(cdp);
	VN_INIT(vp, mp->m_vfsp, CDFTOVT(cdp->cd_ftype), 0);
	if (cdno == cdfs->cdfs_rootcdno) {
		vp->v_flag |= VROOT;
		cdp->cd_pnum=cdno;
	} else {
                /*
                 * When ever we can, save cdnode number of the parent
                 * directory.  To find this piece of information is very
                 * expensive.  Fortunately, it is available most of the
                 * time.
                 */
		if (pcdnop) 
			cdp->cd_pnum = *pcdnop;
		else
	  		cdp->cd_pnum = 0;
	}

	vp->v_op = &cdfs_vnodeops;
	vp->v_fstype = VCDFS;

	if (bp) 
		brelse(bp);	/* release the buffer. */

	/*
         * Now we need to get the information from XAR if exists.
         */
	if (cdp->cd_xarlen != 0) {
		bp = cdfs_readb(cdp->cd_devvp,
				(u_int)(cdp->cd_loc << cdfs->cdfs_lbshift),  
				sizeof(struct cdxar_iso));
		if (bp) {
		   cdhp = (struct cdxar_hsg *) bp->b_un.b_addr;
		   cdip = (struct cdxar_iso *) cdhp;

		   if(cdp->cd_format == HSGFS) {
			cp1 = (char *) (cdhp->xar_eff_year);
			bcopy(cp1, (caddr_t)cdp->cd_eff_year,
			      YEAR_DIGIT+MONTH_DIGIT+
			      DAY_DIGIT+ HOUR_DIGIT+MINUTE_DIGIT+SECOND_DIGIT);
			cdp->cd_eff_zone[0] = '0';
			cp1 = (char *) (cdhp->xar_exp_year);
			bcopy(cp1, (caddr_t)cdp->cd_exp_year,
			      YEAR_DIGIT+MONTH_DIGIT+
			      DAY_DIGIT+ HOUR_DIGIT+MINUTE_DIGIT+SECOND_DIGIT);
			cdp->cd_exp_zone[0] = '0';
			cp1 = (char *) (cdhp->xar_mod_year);
			bcopy(cp1, (caddr_t)cdp->cd_mod_year,
			      YEAR_DIGIT+MONTH_DIGIT+
			      DAY_DIGIT+ HOUR_DIGIT+MINUTE_DIGIT+SECOND_DIGIT);
			cdp->cd_mod_zone[0] = '0';
			cp1 = (char *) (cdhp->xar_create_year);
			bcopy(cp1, (caddr_t)cdp->cd_create_year,
			      YEAR_DIGIT+MONTH_DIGIT+
			      DAY_DIGIT+ HOUR_DIGIT+MINUTE_DIGIT+SECOND_DIGIT);
			cdp->cd_create_zone[0] = '0';
		   } else {
			cp1 = (char *) (cdhp->xar_create_year);
			bcopy(cp1, (caddr_t)cdp->cd_create_year,
			      (YEAR_DIGIT+MONTH_DIGIT+
			      DAY_DIGIT+ HOUR_DIGIT+MINUTE_DIGIT+SECOND_DIGIT +
			      ZONE_DIGIT)*3);

		   }
		   cdp->cd_uid = cdip->xar_uid;
		   cdp->cd_gid = cdip->xar_gid;
		   i = ~(cdip->xar_perm);

		   cdp->cd_mode = 0;
		   
                   /*
                    * The format of permission is very different from UFS,
                    * we convert it now to save the grief later.
                    */
		   if (i&0x10) 
			cdp->cd_mode |= CDREAD;
		   if (i&0x40) 
			cdp->cd_mode |= CDEXEC;
		   if (i&0x100) 
			cdp->cd_mode |= (CDREAD>>3);
		   if (i&0x400) 
			cdp->cd_mode |= (CDEXEC>>3);
		   if (i&0x1000) 
			cdp->cd_mode |= (CDREAD>>6);
		   if (i&0x4000) 
			cdp->cd_mode |= (CDEXEC>>6);

		   brelse(bp);
		}
	} else {
                /*
                 * No XAR, use default.
                 */
		cdp->cd_uid = -1;		/*don't care*/
		cdp->cd_gid = -1;		/*don't care*/

		cdp->cd_mode = (CDREAD|CDEXEC) | ((CDREAD|CDEXEC)>>3) |
			     ((CDREAD|CDEXEC)>>6);

		/*default to readable and executable */
		
	}
	
	if (cdp->cd_fflag & CDROM_IS_DIR)	/*  set directory bit for  */
		cdp->cd_mode |= 0040000;	/*  stat(2)'s use          */
	else
		cdp->cd_mode |= 0100000;

	return (cdp);
}

/*
 * Unlock cdnode and vrele associated vnode
 */
cdput(cdp)
	register struct cdnode *cdp;
{

	if ((cdp->cd_flag & CDLOCKED) == 0)
		panic("cdput");
	cdunlock(cdp);
	VN_RELE(CDTOV(cdp));
}

/*****************************************************************************
 * cddrop()
 *
 * Drop cdnode without going through the normal chain of unlocking
 * and releasing. Used by NFS in the case of the CDROM is swapped while
 * mounted by the NFS client.
 */
cddrop(cdp)
	register struct cdnode *cdp;
{
	register struct vnode *vp = &cdp->cd_vnode;

	if ((cdp->cd_flag & CDLOCKED) == 0)
		panic("cddrop");

	CDUNLOCK(cdp);
	SPINLOCK(v_count_lock);
	--vp->v_count;
	SPINUNLOCK(v_count_lock);

	if (vp->v_count == 0) {
		cdp->cd_flag = CDBUFVALID|CDPAGEVALID;

		/*
		 * Put the inode back on the end of the free list.
		 */
		if (cdfreeh) {
			*cdfreet = cdp;
			cdp->cd_freeb = cdfreet;
		} else {
			cdfreeh = cdp;
			cdp->cd_freeb = &cdfreeh;
		}

		cdp->cd_freef = NULL;
		cdfreet = &cdp->cd_freef;
	}
}

/*
 * Vnode is no longer referenced, write the cdnode out and if necessary,
 * truncate and deallocate the file.
 */
cdinactive(cdp)
	register struct cdnode *cdp;
{
	if (cdp->cd_flag & CDLOCKED)
		panic("cdfs_inactive");

	cdp->cd_flag &= CDBUFVALID|CDPAGEVALID;

	/*
	 * Put the cdnode on the end of the free list.
	 * Possibly in some cases it would be better to
	 * put the cdnode at the head of the free list,
	 */
	if (cdfreeh) {
		*cdfreet = cdp;
		cdp->cd_freeb = cdfreet;
	} else {
		cdfreeh = cdp;
		cdp->cd_freeb = &cdfreeh;
	}
	cdp->cd_freef = NULL;
	cdfreet = &cdp->cd_freef;
}


/****************************************************************************
 * cdflush()
 *
 * remove any cdnodes in the cdnode cache belonging to dev.  Used when the
 * CD-ROM is un-mounted.
 *
 * There should not be any active ones, return error if any are found.
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the cdnode table here anyway, we might as well get the
 * extra benefit.
 *
 * this is called from sumount()/sys3.c when dev is being unmounted
 */
cdflush(dev)
	dev_t dev;
{
	register struct cdnode *cdp;
	register open = 0;

	for (cdp = cdnode; cdp < cdnodeNCDNODE; cdp++) {

		if (cdp->cd_dev == dev) {

                        /*
                         * If any cdnode is still being reference, we need to
                         * stop and return error.  Users should get a "device
                         * busy" message.
                         */
			if (cdp->cd_flag & CDREF) {

				/*
				 * If this is a DUX vnode, the count may
				 * be due to having buffers in core
				 * so invalidate them
				 */
				mpurge(CDTOV(cdp));

				if ((CDTOV(cdp))->v_fstype == VDUX_CDFS) {
					binval(CDTOV(cdp));
				}

                                /*
                                 * If after all these pushup, it is still
                                 * reference, we return error.
                                 */
				if (cdp->cd_flag & CDREF) {
					return(-1);
				}
			}
			/*
	 		 * The following lines take the place of the
 	 		 * vax remque instruction:  remque(ip);
 	 		 */
			cdp->cd_back->cd_forw = cdp->cd_forw;
			cdp->cd_forw->cd_back = cdp->cd_back;
			cdp->cd_forw = cdp;
			cdp->cd_back = cdp;
			/*
			 * as cd_count == 0, the cdnode was on the free
			 * list already, just leave it there, it will
			 * fall off the bottom eventually. We could
			 * perhaps move it to the head of the free
			 * list, but as umounts are done so
			 * infrequently, we would gain very little,
			 * while making the code bigger.
			 */

		} else if ((cdp->cd_flag & CDREF) && (cdp->cd_rdev == dev)) {
                        /*
                         * Reference for the access through special device.
                         */
			open++;
		}
	}
	return (open);
}

/*
 * Lock an cdnode. If its already locked, set the WANT bit and sleep.
 */
cdlock(cdp)
	register struct cdnode *cdp;
{

	CDLOCK(cdp);
}

/*
 * Unlock an cdnode.  If WANT bit is on, wakeup.
 */
cdunlock(cdp)
	register struct cdnode *cdp;
{

	if (!(cdp->cd_flag & CDLOCKED)) {
		panic("cdunlock");
	}
	CDUNLOCK(cdp);
}


/*****************************************************************************
 * cdfs_readb()
 *
 * Given the vnode of the device and the address (offset) in bytes and the
 * number of the bytes requested, cdfs_readb() reads in the section into
 * a buffer.  A pointer to the buffer header is returned.
 */
struct buf *
cdfs_readb(devvp, offset, size)
struct vnode *devvp;
u_int	offset, size;
{
	struct	uio	auio;
	struct	iovec	aiov;
	struct	buf	*bp;

	register struct	uio	*auiop = &auio;
	register struct	iovec	*aiovp = &aiov;

	
	bp = geteblk(size);

        /*
         * Prepare for a uio structure so cdfs_rd and read in the section
         * requested.
         */
	auiop->uio_iov = aiovp;
	aiovp->iov_base = bp->b_un.b_addr;
	aiovp->iov_len = size;
	auiop->uio_iovcnt = 1;
	auiop->uio_offset = offset;
	auiop->uio_seg = UIOSEG_KERNEL;
	auiop->uio_resid = aiovp->iov_len;	/* byte count */
	auiop->uio_fpflags = 0;
	cdfs_rd(devvp, devvp, auiop);		/* read */
	if (auiop->uio_resid) {			/*read incomplete return error*/
		brelse(bp);
		bp = (struct buf *) 0;
	}
	return(bp);
}

/*
 * Check mode permission on cdnode.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, always return error.
 * The mode is shifted to select the owner/group/other fields.
 * If the owner of file is -1 (no owner), always grant the permissions.
 * Permission is granted by return 0.  Else, return EACESS.
 * The super user is granted all read and write permissions,
 * and is granted exec permissions if any exec bits are set in
 * cdp->cd_mode.
 */
#define CDANY_EXEC	(CDEXEC | CDEXEC >> 3 | CDEXEC >> 6)
cdaccess(cdp, m)
	register struct cdnode *cdp;
	register int m;
{
	register gid_t *gp;
	register int uid;

        /*
         * Sorry, good try.  You cannot write to ROM.
         */
	if (m & CDWRITE) 
		return(EROFS);

	/*
	 * No uid / no perm ==> -1 uid and r-xr-xr-x permissions
	 */
	if ((int)cdp->cd_uid == -1)
		return(0);

        /*
         * Super-user doesn't have super-privilige over execution permission.
         * If you're the super-user, you always get read access for files and
         * search permission for directories..  (Note: For directories,
         * execution bits mean gives search permissions.
         * Of course, if any of the execution bits is set, super-user can
         * execute it.
         */
	uid=u.u_uid;
        if ((uid == 0) &&                   /* super user                     */
            (!(m & CDEXEC) ||               /* Not asking for exec permission */
             (cdp->cd_ftype & CDFDIR) ||    /* If a directory, grant search   */
             (cdp->cd_mode & CDANY_EXEC)))  /* Any of the exec bit set        */
        {
                return (0);                 /* "Yes, you have my permisson"   */
        }

        /*
         * Access check is based on only one of owner, group, public.
         * If not owner, then check group.
         * If not a member of the group, then
         * check public access.
         */
	if (uid != cdp->cd_uid) {
		m >>= 3;
		if (u.u_gid == cdp->cd_gid)
			goto found;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (cdp->cd_gid == *gp)
				goto found;
		m >>= 3;
	}
found:
	if ((cdp->cd_mode & m) == m)
		return (0);
	u.u_error = EACCES;
	return (EACCES);
}
