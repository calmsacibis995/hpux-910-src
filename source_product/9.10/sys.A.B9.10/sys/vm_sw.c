/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_sw.c,v $
 * $Revision: 1.25.83.8 $        $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/15 12:19:34 $
 */


/*	vm_sw.c	6.1	83/07/29	*/
/*	matches 1.15.3.14		*/

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/pregion.h"
#include "../h/vnode.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/kern_sem.h"
#include "../h/swap.h"
#include "../h/debug.h"
#include "../h/vmmac.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../h/pathname.h"
#include "../dux/cct.h"
#include "../dux/lookupops.h"
#ifdef AUDIT
#include "../h/audit.h"
#endif AUDIT

extern site_t my_site;
extern int swapmem_on;
#ifdef __hp9000s800	/* used to be NOPAGING, active on s700, s800, not on s300 */
extern int noswapping;
#endif
#ifdef NEVER_USED
struct  buf rswbuf;
#endif /* NEVER_USED */
/*
 * Indirect driver for multi-controller paging.
 */

/*ARGSUSED*/
sw_strategy(bp)
struct buf *bp;
{
   panic("should never get here");
}

devswap_strategy(bp)
	struct buf *bp;
{
	register int size;
	register int swpage;
	register ushort swi, smpi;
	struct buf *tbp;
	struct buf savebp;
	swpt_t *st;
	struct inode *fsw_inode;
	struct inode *st_inode;
	register long save_flags;
        register int fs_bsize, rbn, bn, bnrem;
        register int fsblk, fsblkrem;
	swpdbd_t swdbd;
	int nchunk;
	int once = 1;
	sv_sema_t old_sema;

	*(int *)(&swdbd) = bp->b_blkno;
	swi = swdbd.dbd_swptb;
	smpi = swdbd.dbd_swpmp;
	st = &swaptab[swi];

	/*
	 * if this is a swapless client, send the request to the dux
	 * swap driver.  Store the index of the servers swaptab entry
	 * into the request.
	 */

	if (my_site_status & CCT_SLWS) {
		swdbd.dbd_swptb = st->st_union.st_swptab;
		bp->b_blkno = *(int *)(&swdbd);
#ifdef FSD_KI
		bump_stats(makedev(brmtdev,0), (unsigned int)bp->b_bcount/NBPG,
			  (unsigned int)(bp->b_flags & B_READ), SWTYPE_LAN);
#endif
		(*bdevsw[brmtdev].d_strategy)(bp,1);
		return;
	}

	size = bp->b_bcount / NBPG;
	
        if (smpi+size > NPGCHUNK)
		panic("devswap_strategy: request exceeds swchunk");

	if ((swi >= maxswapchunks) || (st->st_swpmp == 0) || 
	   (st->st_dev == 0 && st->st_fsp == 0))
		panic("devswap_strategy: swaptab[] entry invalid");

	/*
	 * b_s2 holds the site id -- could change this from a panic to
	 * an error after it has been run a while
	 */

	if ((st->st_site != my_site) && (st->st_site != bp->b_s2))
		panic("bad swaptab[]"); 

        if (st->st_dev)
        {
		if (bp->b_flags & B_NETBUF){

			tbp = bswalloc();
			*tbp = *bp;
		}
        	else {
               		tbp = bp;
        	}
		
#ifdef OSDEBUG
		{
			long save_bcount = tbp->b_bcount;
			minphys(tbp);
			VASSERT(tbp->b_bcount == save_bcount);
		}
#endif

		if (st->st_dev->sw_enable == 0)
		   panic("devswap_strategy: device %x not enabled",st->st_dev);

		nchunk = swi - st->st_dev->sw_head;
		VASSERT(nchunk >= 0);
		swpage = smpi + (nchunk * NPGCHUNK);
		tbp->b_blkno = st->st_dev->sw_start + ptod(swpage);
		tbp->b_vp = st->st_vnode;
		tbp->b_dev = st->st_dev->sw_dev;
#ifdef _WSIO
		tbp->b_offset = tbp->b_blkno << DEV_BSHIFT;
#endif /* _WSIO */
#ifdef FSD_KI
		bump_stats(tbp->b_dev, (unsigned int)size, 
			(unsigned int)(tbp->b_flags & B_READ), SWTYPE_DEV);
#endif
		p_io_sema(&old_sema);
		(*bdevsw[major(tbp->b_dev)].d_strategy)(tbp);
		v_io_sema(&old_sema);

		if (bp->b_flags & B_NETBUF) 
			biowait(tbp);

		if (tbp->b_flags & B_ERROR)
			panic("swap device error");

		if (bp->b_flags & B_NETBUF) {
			bswfree(tbp);
			bp->b_resid = 0;
			biodone(bp);
		}
               	return;
        }
        else
	{
		save_flags = bp->b_flags;

		if (bp->b_flags & B_NETBUF){
			tbp = bswalloc();
                        *tbp = *bp;
                }
                else {
                        tbp = bp;
                }

		/*
                 * bn = the file system block number
                 * bnrem = the offset into the file system block
                 */

        	fsw_inode = VTOI(st->st_fsp->fsw_vnode);
        	fs_bsize =  fsw_inode->i_fs->fs_bsize;

                bn = (smpi * NBPG) / fs_bsize;
                bnrem = (smpi * NBPG) % fs_bsize;

		/*
                 * fsblkrem = 0 if the buffer to be written out does not
                 *              take up a fragment at the end.
                 */

		fsblk = (tbp->b_bcount + bnrem) / fs_bsize;
		fsblkrem = (tbp->b_bcount + bnrem) % fs_bsize;

		if (fsblkrem > 0)
			fsblk++;

		savebp = *tbp;
                tbp->b_dev = fsw_inode->i_dev;
                tbp->b_vp = st->st_vnode;

		if (fsblk > 1) {
			once = 0;
			tbp->b_bcount = fs_bsize - bnrem;	
		}
			
                st_inode = VTOI(st->st_vnode);

#ifdef FSD_KI
		bump_stats(tbp->b_dev, (unsigned int)size, 
			(unsigned int)(tbp->b_flags & B_READ), SWTYPE_FS);
#endif

		while (fsblk > 0) {

			if (!once){
				tbp->b_flags =  save_flags & ~B_CALL;
				tbp->b_iodone =  0;
			}
                	rbn = bmap(st_inode, (daddr_t)bn, B_READ, 0, (int *)0,
							(daddr_t *)0, (int *)0);
			tbp->b_blkno = fsbtodb(st_inode->i_fs, rbn)+
				(bnrem/DEV_BSIZE);
			bnrem = 0;
#ifdef _WSIO
			tbp->b_offset = tbp->b_blkno << DEV_BSHIFT;
#endif /* _WSIO */
			ilock(st_inode);
			p_io_sema(&old_sema);
			(*bdevsw[major(tbp->b_dev)].d_strategy)(tbp);
			v_io_sema(&old_sema);
			iunlock(st_inode);

			fsblk--;
			if ((!once) || (bp->b_flags & B_NETBUF)) {
				biowait(tbp);	
				tbp->b_un.b_addr += tbp->b_bcount;
				bn++;

				if ((fsblkrem > 0) && (fsblk == 1))
					tbp->b_bcount = fsblkrem;
				else
					tbp->b_bcount = fs_bsize;
			}

			if (tbp->b_flags & B_ERROR)
				panic("swap file2 error");
			if ((!once) || (bp->b_flags & B_NETBUF)) {
				if (tbp->b_resid != 0)
					panic("incomplete swap buf sent");
			}

		}
		if (bp->b_flags & B_NETBUF) {
			bswfree(tbp);
			bp->b_resid = 0;
			biodone(bp);
                	return;	
		}
		if (!once){
			*bp = savebp;
			bp->b_resid = 0;
			biodone(bp);
		}
		return;
        }
              
}
/*ARGSUSED*/
sw_read(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return(ENODEV); 
}

/*ARGSUSED*/
sw_write(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return(ENODEV);
}

fs_update(vp, lim)
	struct vnode *vp;
	int lim;
{
	fswdev_t *fswp;
	struct inode *fsw_inode;
	int ndevblk;
	int new_limit;
	int i, rem;
	
	fswp = &fswdevt[0];
	for (i = 0; i < nswapfs; i++, fswp++) {

	    if (*fswp->fsw_mntpoint == '\0' )
			continue;

	    if (fswp->fsw_vnode == vp) {
	    
		update_duxref(vp, -1, 0);
		VN_RELE(vp);

		if (!fswp->fsw_enable) {

	                /*
                         * If this file system is being
                         * disabled, try and re-enable.
                         */
                         if (!swapok((swdev_t *)0, fswp)) {
          	               u.u_error = EBUSY;
                               return(1);
                         }
		}
		fsw_inode = VTOI(fswp->fsw_vnode);
		ndevblk = (lim*fsw_inode->i_fs->fs_bsize) /
			  DEV_BSIZE;

		new_limit = ndevblk / swchunk; 
		rem = ndevblk % swchunk;

		if (rem > 0)
		    new_limit++;

		if (new_limit > fswp->fsw_limit) {
			fswp->fsw_limit = new_limit;
		}
		return(1);
	    }
	}
	return(0);
}	



/*
 * Check to see if the media is readonly - if it is, it's not worth
 * much as swap space :-)  Do this by reading the first block of the
 * swap area and then writing it back.	If this succeeds, we haven't
 * done any damage; if it fails, we can avoid a later "I/O error in
 * push" panic by disallowing the entry now.  If we can't write to the
 * device (for any reason) this routine returns a 1, else it returns 0 if
 * we can write to the disk.
 */

int
check_readonly(dev, start)
dev_t dev;
int start;
{
	struct vnode *dev_vp;
	struct buf *bp;
	int error;

#ifdef FULLDUX
#ifdef HPNSE
	if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD|FWRITE, 0, 0, 0)) {
#else  /* not HPNSE */
	if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD|FWRITE, 0)) {
#endif /* HPNSE */
#else  /* not FULLDUX */
#ifdef HPNSE
	if (opend(&dev, IFBLK | IF_MI_DEV, FREAD|FWRITE, (int *)0, 0, 0)) {
#else  /* not HPNSE */
	if (opend(&dev, IFBLK | IF_MI_DEV, FREAD|FWRITE, (int *)0)) {
#endif /* HPNSE */
#endif /* FULLDUX */
		return(1);
	}

	dev_vp = devtovp(dev);
	bp = bread(dev_vp, (daddr_t)start, MAXBSIZE, B_unknown);  
	bp->b_flags |= B_NOCACHE;
	error = ((bp->b_flags & B_ERROR) ? 1 : 0);

	if (!error) {
		u.u_error = 0;	/* clear any previous error */
		bwrite(bp);	/* writes and releases bp */
		if (u.u_error) {
			error = 1;
		}
	} else
		brelse(bp);

#ifdef FULLDUX
#ifdef	HPNSE
	(void) closed(dev, IFBLK, FREAD|FWRITE, my_site, 0);
#else  /* not HPNSE */
	(void) closed(dev, IFBLK, FREAD|FWRITE, my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef	HPNSE
	(void) closed(dev, IFBLK, FREAD|FWRITE, 0);
#else  /* not HPNSE */
	(void) closed(dev, IFBLK, FREAD|FWRITE);
#endif /* HPNSE */
#endif /* FULLDUX */


	return(error);
}

/*
 * Description:
 *      Print a descriptive message about the errors that can be returned
 *      by check_swap_conditions. 
 *
 * END_DESC
 */
print_swap_error(err)
int err;
{
	switch (err) {
	      case 0:	/* No error.  Should not be called with this */
		break;
	      case ENODEV:
		printf("ignored - can't open device\n");
		break;
	      case ENOSYS:
		printf("ignored - can't open file system\n");
		break;
	      case EEXIST:
		printf("ignored - file system exists\n");
		break;
	      case EIO:
		printf("ignored - can't read device\n");
		break;
	      case ENOSPC:
		printf("ignored - no room\n");
		break;
	      case EROFS:
		printf("ignored - can't write to device\n");
		break;
	      case EFAULT:
		printf("ignored - LIF header is inconsistent\n");
		break;
	      default:
		printf("ignored - can't configure swap on device\n");
	}
}

/*
 * Configure swap space and related parameters.
 */
swapconf()
{
#ifndef HPUXBOOT
	register int totalblks = 0;
	register struct swdevt *swp, *swp_scan;
	int snoozed = 0;
	int err;

/*
 * Define a constant for the number of bits to shift for converting a
 * DEV_BSIZE block size to a 512 block size.
 */
#define SHIFTDEVBSIZETO512 (DEV_BSHIFT - 9)

	/*
	 * Variable used to keep track of the number of swap devices enabled.
	 */
	nswdev = 0;
	printf("    Swap device table:  (start & size given in 512-byte blocks)\n");
	/*
	 * loop for auto-configuring and validating each entry
	 */
	for (swp = swdevt; swp < &swdevt[nswapdev] && swp->sw_dev != NODEV; swp++) {
		printf("        entry %d - ", swp - swdevt);

		/*
		 * sw_dev auto-configured to rootdev?
		 */
		if (swp->sw_dev == SWDEF) {
			swp->sw_dev = rootdev;
			printf("auto-configured on root device; ");
		} else {
			printf("major is %d, minor is 0x%x; ",
					major(swp->sw_dev), minor(swp->sw_dev));
		}
		/*
		 * on same device as a prior entry?
		 */
		for (swp_scan = swdevt; swp_scan < swp; swp_scan++)
			if (swp_scan->sw_dev == swp->sw_dev)
				break;
		if (swp_scan < swp) {
			printf("ignored - same device as entry %d\n",
							swp_scan - swdevt);
			continue;
		}

		/*
		 * If unable to open device, print to console & continue.
		 */
#ifndef _WSIO
#ifdef FULLDUX
#ifdef HPNSE
		if (opend(&swp->sw_dev, IFBLK | IF_MI_DEV, my_site,
			  FREAD|FWRITE, 0, 0, 0))
#else  /* not HPNSE */
		if (opend(&swp->sw_dev, IFBLK | IF_MI_DEV, my_site,
			  FREAD|FWRITE, 0))
#endif /* HPNSE */
#else  /* not FULLDUX */
#ifdef HPNSE
		if (opend(&swp->sw_dev, IFBLK | IF_MI_DEV, FREAD|FWRITE, 
			  (int *)0, 0, 0))
#else  /* not HPNSE */
		if (opend(&swp->sw_dev, IFBLK | IF_MI_DEV, FREAD|FWRITE, 
			  (int *)0))
#endif /* HPNSE */
#endif /* FULLDUX */
		{
			printf("ignored - can't open device\n");
			continue;
		}
#endif /* not _WSIO */

		/*
		 * Call to check_swap_conditions has side effects of changing
		 * the sw_start and sw_nblks fields of swp->.
		 */
try_again:
		if ((err=check_swap_conditions(swp,0)) != 0) {
			/*
			** Give primary swap device a second chance; the
			** alternative is to panic.
			*/
			if (err == ENODEV && swp == swdevt && !snoozed) {
				snoozed++;
#ifdef __hp9000s300
				snooze(30000000);
#else
				busywait(30000000);
#endif
				goto try_again;
			}
			print_swap_error(err);
			continue;
		}


		printf("start = %d, size = %d\n",
			swp->sw_start << SHIFTDEVBSIZETO512,
			swp->sw_nblks << SHIFTDEVBSIZETO512);
		totalblks += swp->sw_nblks;
	}

	/*
	 * If more than 1 swap entry is configured, print the total
	 */

	if (swp > swdevt + 1) {
		msg_printf("        Total available pre-configured swap: %d\n",
		       totalblks << SHIFTDEVBSIZETO512);
	}

#ifdef __hp9000s800	/* used to be NOPAGING, active on s700, s800, not on s300 */
	if (!noswapping) {
#endif
		if (!swapmem_on) {
			if (swdevt[0].sw_nblks <= 0) {
				panic("swapconf - no primary swap space configured");
			}
		}
#ifdef __hp9000s800	/* used to be NOPAGING, active on s700, s800, not on s300 */
	}
#endif
#endif HPUXBOOT
}


/*
 * System call swapon(name) enables swapping on device name,
 * which must be in the swdevt.  Return EBUSY
 * if already swapping on this device.
 */
swapon()
{
	 struct a {
		char	*name;
		uint     min_or_pri;
                uint     limit;
                uint     reserve;
                int     priority;
	} *uap;
	struct vnode *vp;
	dev_t dev;
	struct swdevt *sp;

        struct fswdevt *fswp;
        struct vnode *root_vp;
        struct inode *fsw_inode;
        struct pathname lookpn;
	int i;
	uint tmin;
	uint ndevblk;
	int rem;
	int pri;
        struct buf *bp;
        unsigned int len;
	extern char * strcpy();
	extern struct audit_filename * save_pn_info();


	uap = (struct a *)u.u_ap;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(uap->name);
	}
#endif /* AUDIT */

	if (!suser()) 
	        return;

	if (my_site_status & CCT_SLWS) {
		u.u_error = EINVAL;
		return;
	}
		
	vmemp_lock();		/* lock down the vm empire */
	u.u_error =
	    lookupname(uap->name, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_LOOKUP, (caddr_t)0);
	if (u.u_error)
		vmemp_return();

        if ((vp->v_type != VBLK) && (vp->v_type != VDIR)){
		update_duxref(vp, -1, 0);
		VN_RELE(vp);
                u.u_error = ENOTBLK;
		vmemp_return();
        };
	
	if (vp->v_type == VBLK) {
		int old_dev, swapon_and_new_entry;
		u_int mode;
		dev = old_dev = (dev_t)vp->v_rdev;
		/*
		 * If VMI_DEV flag is not set in the vnode, then call opend() 
		 * without the IF_MI_DEV flag to let it know (on a s800) 
		 * that the mapping from LU to MI has not yet occurred.
		 * This is because opend calls pre_open on the s800.  All 
                 * future calls to opend() 
		 * will need to pass in the IF_MI_DEV mode flag to let
		 * opend() know that the mapping has been done.
		 */
		if ((vp->v_flag & VMI_DEV) == 0) {
			mode = IFBLK;
		}
		else {
			mode = IFBLK | IF_MI_DEV;
		}
#ifdef FULLDUX
#ifdef HPNSE
		u.u_error = opend(&dev, mode, my_site, FREAD|FWRITE, 0, 0, 0);
#else  /* not HPNSE */
		u.u_error = opend(&dev, mode, my_site, FREAD|FWRITE, 0);
#endif /* HPNSE */
#else  /* not FULLDUX */
#ifdef HPNSE
		u.u_error = opend(&dev, mode, FREAD|FWRITE, (int *)0, 0, 0);
#else  /* not HPNSE */
		u.u_error = opend(&dev, mode, FREAD|FWRITE, (int *)0);
#endif /* HPNSE */
#endif /* FULLDUX */
		if (u.u_error) {
			update_duxref(vp, -1, 0);
			VN_RELE(vp);
			vmemp_return();
		}
		vp->v_flag |= VMI_DEV;

		update_duxref(vp, -1, 0);
		VN_RELE(vp);

		sp = &swdevt[0];

		for (i = 0; i < nswapdev; i++, sp++) {
			if (sp->sw_dev == dev) {
				if (sp->sw_enable) {
#ifdef FULLDUX
#ifdef  HPNSE
					closed(dev, IFBLK, FREAD|FWRITE, 
							my_site, 0);
#else  /* not HPNSE */
					closed(dev, IFBLK, FREAD|FWRITE, 
							my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef  HPNSE
					closed(dev, IFBLK, FREAD|FWRITE, 0);
#else  /* not HPNSE */
					closed(dev, IFBLK, FREAD|FWRITE); 
#endif /* HPNSE */
#endif /* FULLDUX */
					u.u_error = EALREADY;
					vmemp_return();
				}
				else {
					/* 
					 * See if this device has been 
					 * enabled previously by checking
					 * whether it has been linked into
					 * a priority chain.  If not break
					 * out so it will get enabled in
					 * swfree().
					 */
					if (sp->sw_next == 0)
						break;
					
					/*
					 * If this device is being
					 * disabled, try and re-enable.
					 */
					if (!swapok(sp, (fswdev_t *)0)){
#ifdef FULLDUX
#ifdef  HPNSE
						closed(dev, IFBLK, FREAD|FWRITE,
							my_site, 0);
#else  /* not HPNSE */
						closed(dev, IFBLK, FREAD|FWRITE,
							my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef  HPNSE
						closed(dev, IFBLK, FREAD|FWRITE,
							 0);
#else  /* not HPNSE */
						closed(dev, IFBLK,FREAD|FWRITE); 
#endif /* HPNSE */
#endif /* FULLDUX */
						u.u_error = EBUSY;
						vmemp_return();
					}
#ifdef FULLDUX
#ifdef  HPNSE
					closed(dev, IFBLK, FREAD|FWRITE, 
							my_site, 0);
#else  /* not HPNSE */
					closed(dev, IFBLK, FREAD|FWRITE, 
							my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef  HPNSE
					closed(dev, IFBLK, FREAD|FWRITE, 0);
#else  /* not HPNSE */
					closed(dev, IFBLK, FREAD|FWRITE); 
#endif /* HPNSE */
#endif /* FULLDUX */
					vmemp_return();
				}
			}
			else if (sp->sw_dev == NODEV) {
				old_dev = NODEV;
				sp->sw_dev = dev;
				break;
			}
		}
		if ( i >= nswapdev) {
                        u.u_error = ENOENT;
#ifdef FULLDUX
#ifdef  HPNSE
			closed(dev, IFBLK, FREAD|FWRITE, my_site, 0);
#else  /* not HPNSE */
			closed(dev, IFBLK, FREAD|FWRITE, my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef  HPNSE
			closed(dev, IFBLK, FREAD|FWRITE, 0);
#else  /* not HPNSE */
			closed(dev, IFBLK, FREAD|FWRITE); 
#endif /* HPNSE */
#endif /* FULLDUX */
		}
		else {
			pri = uap->min_or_pri; 
			if ((pri < 0) || (pri > NSWPRI))
				pri = 1;

			/*
			 * Set flag for check_swap_conditions() to tell it
			 * that the call is from swapon and that there was
			 * not an entry for this device in the swdevt (none
			 * was pre-configured from a dfile) if the swdevt
			 * entry had NODEV as the device prior to this call.
			 */
			swapon_and_new_entry = (old_dev == NODEV);
			u.u_error = check_swap_conditions(sp,
						  swapon_and_new_entry);
			if (u.u_error) {
				sp->sw_dev = old_dev;
#ifdef FULLDUX
#ifdef	HPNSE
				closed(dev, IFBLK, FREAD|FWRITE, my_site, 0);
#else  /* not HPNSE */
				closed(dev, IFBLK, FREAD|FWRITE, my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef	HPNSE
				closed(dev, IFBLK, FREAD|FWRITE, 0);
#else  /* not HPNSE */
				closed(dev, IFBLK, FREAD|FWRITE);
#endif /* HPNSE */
#endif /* FULLDUX */

				vmemp_return();
			}
			bp = geteblk(MAXPATHLEN);

			u.u_error =
			copyinstr(uap->name, bp->b_un.b_addr, MAXPATHLEN, &len);

			/* If an error occured in the call to copyinstr()
			 * OR if a call to swfree() fails, we cannot add this
			 * swap, so restore the original dev, if it was
			 * NODEV, then clear the fields that may have been
			 * set before the error occured that would be
			 * mistakenly used during a subsequent call to
			 * swapon() for a device not in swdevt yet, close
			 * the device, release the bp, and return.
			 */

			if (u.u_error ||
			    !(swfree(sp - swdevt, pri, bp->b_un.b_addr)))
			{
				if (old_dev == NODEV) {
					sp->sw_start = 0;
					sp->sw_nblks = 0;
				}
				sp->sw_dev = old_dev;
#ifdef FULLDUX
#ifdef	HPNSE
				closed(dev, IFBLK, FREAD|FWRITE, my_site, 0);
#else  /* not HPNSE */
				closed(dev, IFBLK, FREAD|FWRITE, my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef	HPNSE
				closed(dev, IFBLK, FREAD|FWRITE, 0);
#else  /* not HPNSE */
				closed(dev, IFBLK, FREAD|FWRITE);
#endif /* HPNSE */
#endif /* FULLDUX */
				brelse(bp);
				vmemp_return();
			}

			brelse(bp);
		}
		vmemp_return();
	}
	else {		/* vp->v_type == VDIR */
		if (vp->v_fstype != VUFS){
			update_duxref(vp, -1, 0);
			VN_RELE(vp);
			u.u_error = EOPNOTSUPP;
			vmemp_return();
         	};

		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			update_duxref(vp, -1, 0);
			VN_RELE(vp);
			u.u_error = EROFS;
			vmemp_return();
		}

		pn_get(uap->name, UIOSEG_USER, &lookpn);

		if (ufs_root(vp->v_vfsp, &root_vp, (char *)0)) {
		    printf("Couldn't find root of \"%s\"\n", lookpn.pn_path);
		    panic("vm_sw:  swapon to filesystem");
		}

		/* Check to see if we are already swapping to 
		 * this filesystem
		 */

		if (fs_update(root_vp, (int)uap->limit)) {
			pn_free(&lookpn);
                	u.u_error = EALREADY;
			vmemp_return();
		}

		update_duxref(vp, -1, 0);
		VN_RELE(vp);
	
		fswp = &fswdevt[0];
        	for (i = 0; i < nswapfs; i++, fswp++) {
			if (fswp->fsw_vnode == 0)
				break;
		}

		if ( i >= nswapfs) {
			pn_free(&lookpn);
                	u.u_error = ENOENT;
                	vmemp_return();
		}

		fswp->fsw_enable = 0;
		fswp->fsw_vnode = root_vp;
		fswp->fsw_allocated = 0;
		fswp->fsw_nfpgs = 0;
		fswp->fsw_min = 0;
		fswp->fsw_head = -1;
		fswp->fsw_tail = -1;

		fsw_inode = VTOI(root_vp);

		ndevblk = (uap->limit * fsw_inode->i_fs->fs_bsize) /
                          DEV_BSIZE;

                fswp->fsw_limit = ndevblk / swchunk;
                rem = ndevblk % swchunk;

                if (rem > 0)
                    fswp->fsw_limit++;

                fswp->fsw_reserve = uap->reserve;

		if (uap->min_or_pri) {

		    ndevblk = (uap->min_or_pri * fsw_inode->i_fs->fs_bsize) /
		 	       DEV_BSIZE;

		    tmin = ndevblk / swchunk;
		    rem = ndevblk % swchunk;

		    if (rem > 0)
		    	tmin++;

		    bp = geteblk(MAXPATHLEN); 

		    u.u_error = 
		    copyinstr(uap->name, bp->b_un.b_addr, MAXPATHLEN, &len);
		    if (u.u_error)
		    {
			    brelse(bp);
			    fswp->fsw_vnode = 0;
			    fswp->fsw_limit = 0;
			    fswp->fsw_reserve = 0;
			    pn_free(&lookpn);
			    vmemp_return();
		    }

		    swaplock();
                    if (!swapadd((swdev_t *)0, fswp, tmin, bp->b_un.b_addr)) {
                        printf("vm_sw:  minimum allocation unavailable from %s\n ", bp->b_un.b_addr);
			fswp->fsw_vnode = 0;
                	fswp->fsw_limit = 0;
                	fswp->fsw_reserve = 0;
			pn_free(&lookpn);
                        u.u_error = ENOSPC;
			swapunlock();
			brelse(bp);
                        vmemp_return();
                    }
		    swapunlock();
                    brelse(bp);
		    fswp->fsw_min = tmin;
                }

		fswp->fsw_enable = 1;
		fswp->fsw_priority = uap->priority;
		strcpy(fswp->fsw_mntpoint,lookpn.pn_path);
		adjust_swap_pri((swdev_t *)0, fswp);
		pn_free(&lookpn);
        }
	vmemp_unlock();
}

/*
 * BEGIN_DESC 
 *
 * check_swap_conditions(sw, swapon_and_new_entry)
 *
 * Return Value:
 *	0		Succeeded
 *      ENODEV		Can't open the device.
 *      ENOSYS		Can't find file system that was expected.
 *      EEXIST		Start value in swdevt is in the middle of the fs.
 *      EIO		Error in reading device.
 *      ENOSPC		Not enough room for the swap space requested.
 *      EROFS		Device is a read-only device.
 *      EFAULT		LIF header is inconsistent.
 *
 * Globals Referenced:
 *	rootdev, nblkdev
 *
 * Description:
 *	This routine has the side effects of possibly modifying the 
 *	sw->sw_start and sw->sw_nblks fields of the entry sw points to.
 *	This routine determines if there are any conflicts between
 *	information in the swdevt (either defined by the user in a dfile
 *      or gen file at config time or default values) and the existence of
 *	the device specified and whether it can be written to and whether
 *	there is a file system that overlaps the swap specification, or if
 * 	a fs was expected but was not present, or if the device is read only.
 *	The second parameter is used to flag the case where this is being
 *	called from swapon() and there was not an entry in the swdevt before
 *	the call to swapon().  This is used to allow the case where we want
 *	to call swapon for a new device and we want to swap after the file
 *	system if it exists, but use the whole device otherwise.  
 *
 * Algorithm:
 *      Save the start value so we can back out any changes if an error
 *	occurs.  Determine if the device exists and if so its size.  
 *	Adjust the size if it's a s700 with a boot area at the end.  
 *	Return an error if there is not a fs and one was expected, or
 *	if it does exist and a dfile specified swap to start in the middle
 *	of an existing fs.  If a dfile specified nblks, add the correct
 *	offset from the beginning of swap or from the end of swap.
 *
 * END_DESC
 */


int
check_swap_conditions(sw, swapon_and_new_entry)
swdev_t *sw;
int swapon_and_new_entry;  /* Swapon sets this if no config-time entry 
			    * existed, to allow swapping after a fs (if one
			    * exists) else swap on the whole device.
			    */
{
	sv_sema_t old_sema;
	/* 
	 * Save start value, sw->sw_start (that was set by caller) 
	 * and restore it later if an error occurs.
	 */
	int old_start=sw->sw_start;
	int maj, (*psize)();
	int devsize, nblks, limit, extra;
	int error = 0;
	int size_of_fs;
	int start, size;

	/*
	 * If unable to open device, return with error.
	 */
#ifdef FULLDUX
#ifdef HPNSE
        if (opend(&sw->sw_dev, IFBLK | IF_MI_DEV, my_site, FREAD|FWRITE, 
		  0, 0, 0)) {
#else  /* not HPNSE */
        if (opend(&sw->sw_dev, IFBLK | IF_MI_DEV, my_site, FREAD|FWRITE, 0)) {
#endif /* HPNSE */
#else  /* not FULLDUX */
#ifdef HPNSE
        if (opend(&sw->sw_dev, IFBLK | IF_MI_DEV, FREAD|FWRITE, (int *)0,0,0)) {
#else  /* not HPNSE */
        if (opend(&sw->sw_dev, IFBLK | IF_MI_DEV, FREAD|FWRITE, (int *)0)) {
#endif /* HPNSE */
#endif /* FULLDUX */
		sw->sw_start = old_start;
		return (ENODEV);
	}

	/*
	 * if absolute device size unknown, leave it alone!!!
	 */
	if ((maj = major(sw->sw_dev)) >= nblkdev ||
	    (psize = bdevsw[maj].d_psize) == NULL) {
		error = ENODEV;
		goto bad;
	}
	p_io_sema(&old_sema);
	if ((devsize = (*psize)(sw->sw_dev)) <= 0) {
		v_io_sema(&old_sema);
		error = ENOSPC;
		goto bad;
	}
	v_io_sema(&old_sema);

	/*
	 * Determine the file system size.  If an error occurs while
	 * attempting to read the superblock, return EIO, since we
	 * cannot determine if there is a fs or what its size is.
	 * A -1 returned means no fs, and a positive value is the fs size.
	 */

	if ((size_of_fs = fs_size(sw->sw_dev)) == 0) {
	        error = EIO;
		goto bad;
	}

        /* 
	 * if lif entry for swap exists, get start and size info,
         * else, treat as a disk w/o boot area at the end.
         */
	if (read_my_lif(sw->sw_dev, &start, &size)) {
		/* 
		 * Check if LIF header information is consistent.  
		 * If swap doesn't directly follow the file system, then
		 * return an error since this is currently an unexpected
		 * situation.  Mkboot currently places the swap area
		 * directly after the fs on a s700, so this error could
		 * only occur if the user modifies the lif header.
		 */
		if ((size_of_fs > 0) && (start != size_of_fs)) {
			error = EFAULT;
			goto bad;
		}

		/* 
		 * "Chop off" the boot area if it exists by using 
		 * the end of the swap area as the end of device. 
		 */
		devsize = (start + size);  
	}

	/*
	 * If sw->sw_start is negative (only occurs on s300 and s700 and
	 * means swap after the file system), and no file system exists, 
	 * return an error.  Else if the parameter swapon_and_new_entry !=0
	 * (this is a flag that means that the call came from swapon() and
	 * there was no existing entry in the swdevt at the time of the call),
	 * then if a file system exists, start swap after it.  The value of
	 * sw->sw_start will be 0 in this case, so if there is no fs then
	 * swap will begin at the start of the device.
	 */

	if (sw->sw_start < 0) {
	        if ((sw->sw_start = size_of_fs) <= 0) {
			error = ENOSYS;  /* expected fs but none there */
			goto bad;
		}
	} else if (swapon_and_new_entry) {  /* swapon and NODEV */
		if (size_of_fs > 0) {
			sw->sw_start = size_of_fs;
		} 
	}
        /*
         * If a file system exists where swap is to be configured,
         * return an error unless this is the primary swap device,
         * in which case we will print out a warning, but configure it
         * anyway (after 15 seconds), so that they do not panic.
         */
	if (size_of_fs > sw->sw_start) {  
                if (sw == swdevt) {
                        printf("\n\n\n\n\n\n\n\n\n\n\n");
                        printf("                           ************\n");
                        printf("                           * WARNING! *\n");
                        printf("                           ************\n\n");
                        printf("A file system exists on your primary swap ");
                        printf("device and it will\n");
                        printf("be overwritten by swap.  ");
                        printf("Power down your system or remove the\n");
                        printf("device within 15 seconds if ");
                        printf("you do not want this file system\n");
                        printf("to be overwritten.\n\n");
#ifdef __hp9000s300
                        snooze(15000000);  /* wait 15 seconds */
			fs_clobber(sw->sw_dev);
			printf("        Swap ");
#else
                        busywait(15000000);  /* wait 15 seconds */
#endif
                } else {
			error = EEXIST;  /* start is in middle of fs */
			goto bad;
                }
	}

	/*
	 * if not enough room, say so
	 */
	if ((nblks = devsize - sw->sw_start) <= 0) {
		error = ENOSPC;
		goto bad;
	}

	/*
	 * if readonly media, ignore this entry
	 */
	if (check_readonly(sw->sw_dev, sw->sw_start)) {
		error = EROFS;
		goto bad;
	}

	/*
	 * is there a size limit to enforce?
	 * This is used for limiting the swap area on the disk so that
	 * the remaining space can be used for raw disk io (s300, s700).
	 */
	if (sw->sw_nblks != 0) {
		if ((limit = sw->sw_nblks) < 0)
			limit = -limit;
		if ((extra = nblks - limit) > 0) {
			nblks = limit;
			if (sw->sw_nblks < 0) {
				sw->sw_start += extra;
				VASSERT(sw->sw_start < devsize);
			}
		}
	}
	sw->sw_nblks = nblks;
	/*
	 * Fall through
	 */
bad:
	if (error) { 
		sw->sw_start = old_start;
	}
#ifdef FULLDUX
#ifdef  HPNSE
	closed(sw->sw_dev, IFBLK, FREAD|FWRITE, my_site, 0);
#else  /* not HPNSE */
	closed(sw->sw_dev, IFBLK, FREAD|FWRITE, my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef  HPNSE
	closed(sw->sw_dev, IFBLK, FREAD|FWRITE, 0);
#else  /* not HPNSE */
	closed(sw->sw_dev, IFBLK, FREAD|FWRITE); 
#endif /* HPNSE */
#endif /* FULLDUX */
	return (error);
}


#ifdef INSTALL 
clear_swap(start)
	int start;
{
	int i;
	int npg;

	for (i=start; i != -1; i=swaptab[i].st_next) {
                kdfree(swaptab[i].st_swpmp, M_SWAPMAP);
                swaptab[i].st_swpmp = 0;
                swaptab[i].st_nfpgs = 0;
                swaptab[i].st_dev = 0;
                swaptab[i].st_vnode = 0;
                swaptab[i].st_site = -1;
		swapspc_cnt -= (NPGCHUNK - 1);
		if (swapspc_cnt < 0)
			swapspc_cnt = 0;
	}
}
/*
 * System call swapoff(name) enables the install kernel to
 * remove a device that is currently being swapped to.
 * 
 * This system call should NOT be used by anyone else because
 * it does not correctly clean up after itself.  All this
 * call does is clear out all swap tables.  Therefore any
 * thing that was potentially swapped out is lost.
 *
 * THIS ROUTINE SHOULD NOT BE USED.  There are a couple of known
 * problems.  One is that the device should be closed and the other
 * is with the way swapon walks through the swap dev table looking
 * for open slots.
 */
swapoff()
{
	struct a {
		char *name;
	} *uap;
	struct vnode *vp;
	dev_t dev;
        struct swdevt *sp;
	int i;

	uap = (struct a *)u.u_ap;
	if (!suser()) return;
	u.u_error = lookupname(uap->name, UIOSEG_USER, FOLLOW_LINK,
                              (struct vnode **)0, &vp, LKUP_LOOKUP, (caddr_t)0);
	if (u.u_error) {
		update_duxref(vp, -1, 0);
		VN_RELE(vp);
		u.u_error = EINVAL;
		return;	
	}

	if ((vp->v_type != VBLK)) {
		update_duxref(vp, -1, 0);
		VN_RELE(vp);
                u.u_error = EINVAL;
		return;
        };


	if ((vp->v_flag & VMI_DEV) == 0) {
		u_int   mode = IFBLK;
		dev = (dev_t)vp->v_rdev;
		u.u_error = pre_open(&(vp->v_rdev), &mode, (int *)0);
                if (u.u_error) {
			update_duxref(vp, -1, 0);
			VN_RELE(vp);
                	return;
		}
                vp->v_flag |= VMI_DEV;
	}

	update_duxref(vp, -1, 0);
        VN_RELE(vp);
        if (major(dev) >= nblkdev) {
        	u.u_error = ENXIO;
        }

        sp = &swdevt[0];

        for (i = 0; i < nswapdev; i++, sp++) {
		if (sp->sw_dev == dev) {
			clear_swap(sp->sw_head);
			sp->sw_dev = NODEV;
			sp->sw_enable = 0;
			/*
			 *  Decrement counter for number of enabled
			 *  swap devices.
			 */
			nswdev--;
			sp->sw_start = 0;
			sp->sw_nblks = 0;
			sp->sw_nfpgs = 0;
			sp->sw_head = 0;
		}
	}
	if ( i > nswapdev)
        	u.u_error = ENOENT;
	return;


}
#endif /* INSTALL */
#ifdef NEVER_USED
int argblks = 0;
#endif /* NEVER_USED */
/*
 * Swfree(index) frees the index'th portion of the swap map.
 * Each of the nswdev devices provides 1/nswdev'th of the swap
 * space, which is laid out with blocks of swchunk pages circularly
 * among the devices.
 * If swfree() fails (because swapadd() fails), 0 is returned, else
 * non-zero is returned.
 */
swfree(index,pri,name)
	register int index;
	register int pri;
	char *name;
{
	swdev_t *sw;
	int nentries;
	int error;
	/* 
	 * Create the vnode for this swap device.
	 */
	vm_sema_state;		/* semaphore save state */
	
	vmemp_lockx();		/* lock down VM empire */
	sw = &swdevt[index];
	sw->sw_head = -1;
	sw->sw_tail = -1;
	sw->sw_priority = pri;
	adjust_swap_pri(sw, (fswdev_t *)NULL);

	nentries = sw->sw_nblks / swchunk;

	swaplock();
	if(error=swapadd(sw, (fswdev_t *)0, (uint)nentries, name)) {
		sw->sw_enable = 1;
		/*
		 *  Bump up counter for number of enabled swap devices
		 */
		nswdev++;
	}
	swapunlock();
	vmemp_unlockx();	/* free up VM empire */
	return(error);
}

/*
 * insert the device or file system in the right priority list.
 */
adjust_swap_pri(swdev, swfs)
	swdev_t *swdev;
	fswdev_t *swfs;
{
	swdev_t *tdev;
	fswdev_t *tfs;
	int pri;


	if (swdev) {

		pri = swdev->sw_priority;
		swdev_pri[pri].curr = swdev;
		tdev = swdev_pri[pri].first;

		if (tdev == 0){
			swdev_pri[pri].first = swdev;
			swdev->sw_next = swdev;
			if (pri>maxdev_pri) maxdev_pri = pri;
		}
		else{ 
			if(tdev->sw_next == tdev){
				tdev->sw_next = swdev;
				swdev->sw_next = tdev;
			}
			else {
				swdev->sw_next = tdev->sw_next;
				tdev->sw_next = swdev;
			}
		}

	}
	else {

		pri = swfs->fsw_priority;
		swfs_pri[pri].curr = swfs;
		tfs = swfs_pri[pri].first;

		if (tfs == 0){
			swfs_pri[pri].first = swfs;
			swfs->fsw_next = swfs;
			if (pri>maxfs_pri) maxfs_pri = pri;
		}
		else{ 
			tfs = swfs_pri[pri].first;

			if(tfs->fsw_next == tfs){
				tfs->fsw_next = swfs;
				swfs->fsw_next = tfs;
			}
			else {
				swfs->fsw_next = tfs->fsw_next;
				tfs->fsw_next = swfs;
			}
		}
	}
}

/*
 * check if device or file system is already being disabled
 */
swapok(swdev,swfs)
	swdev_t *swdev;
	fswdev_t *swfs;
{

	int i, start;
	/* check the the first swaptab entry for this
	 * device or fs to see if it is currently being
   	 * disabled if it is, walk through each swaptab
 	 * entry and clear the INDEL flag
	 */

	start = -1;
	if (swdev) {
		if (swaptab[swdev->sw_head].st_flags & ST_INDEL) {
			start = swdev->sw_head;
			swdev->sw_enable = 1;
		}
	}
	else {
		if ((swfs->fsw_head != -1) &&
                    (swaptab[swfs->fsw_head].st_flags & ST_INDEL)) {
			start = swfs->fsw_head;
			swfs->fsw_enable = 1;
		}
	}
		

	if (start >= 0) {
		for (i = start; i > 0; i = swaptab[i].st_next){
			swaptab[i].st_flags &= ~ST_INDEL;
		}
		return(1);
	}
	return(0);	
}
#ifdef LATER 
	/* This routine needs to be fixed before being compiled !
	 * The lookupname call has the incorrect number of args.
	 * This routine is currently unused.  Removed for kernel size
	 * reduction 5/27/92 cwb.
	 */
/*
 * Disable a device or file system from swapping.
 */
realswapoff()
{
	 struct a {
		char	*name;
	} *uap;
	int i;
	struct vnode *vp;
	struct vnode *root_vp;
	dev_t dev;
	swdev_t *sp;
	fswdev_t *fswp;

	uap = (struct a *)u.u_ap;
	if (!suser()) return;
	u.u_error =
	    lookupname(uap->name, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	if ((vp->v_type != VBLK) && (vp->v_type != VREG)){
		u.u_error = EINVAL;
		VN_RELE(vp);
		return;
	}
	if (vp->v_type == VBLK) {
		dev = (dev_t)vp->v_rdev;
		VN_RELE(vp);
		if (major(dev) >= nblkdev) {
			u.u_error = ENXIO;
			return;
		}
		/*
	 	* Search starting at second table entry,
	 	* since first (primary swap area) is freed at boot.
	 	*/
		sp = &swdevt[0];
		for (i = 0; i < nswapdev; i++, sp++) {
			if (sp->sw_dev == dev) {
				sp->sw_enable = 0;
				/*
				 *  Decrement counter for number of enabled
				 *  swap devices.
				 */
				nswdev--;
				swapdel(sp, (fswdev_t *)0);
				return;
			}
		}
		if ( i >= nswapdev) {
                        u.u_error = ENODEV;
                        return;
                }
	}
	else {
		if (ufs_root(vp->v_vfsp, &root_vp, (char *)0)) {
                    panic("swapon to filesystem");
                }
                update_duxref(vp, -1, 0);
                VN_RELE(vp);

                fswp = &fswdevt[0];
                for (i = 0; i < nswapfs; i++, fswp++) {
                        if (fswp->fsw_vnode == root_vp) {
				fswp->fsw_enable = 0;
                                swapdel((swdev_t *)0, fswp);
                                return;
			}
                }
		if ( i >= nswapfs) {
                        u.u_error = ENOENT;
                        return;
                }
	}
}
#endif /* LATER */

#ifdef FSD_KI
extern struct swap_stats swap_stats[];

static
bump_stats(device, npages, direction, type)
dev_t device;
unsigned npages, direction, type;
{
	struct swap_stats *swp;

	swp = swap_stats;

	/*
	 * note that this loop without bounds checking assumes that swapoff
	 * is not enabled
	 */

	/*
	 * an entry on the swap_stats table is empty if the type field is 0.
	 * Note that device == 0 is a valid device number.
	 */
	while (swp->device != device && swp->type) {
		swp++;
	}

	if (swp->type == 0) {		/* this is an empty array element */
		swp->device = device;	/* so fill it in. */
		swp->type = type;
	}

	if (direction)		/* true for reading */
		swp->pages_in += npages;
	else
		swp->pages_out += npages;
}
#endif
