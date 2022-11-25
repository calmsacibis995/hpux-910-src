/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/duxcd_vnop.c,v $
 * $Revision: 1.6.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:43:12 $
 */

/* HPUX_ID: @(#)duxcd_vnop.c	54.8		88/12/12 */
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


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/kernel.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../h/dir.h"
#include "../dux/dux_dev.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_lookup.h"
#include "../h/mount.h"

#include "../dux/dux_hooks.h"	/* Needed for configurability - daveg */

#include "../dux/lookupmsgs.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../h/pathname.h"


int duxcd_strategy();
extern duxcd_not_apply();
#define duxcd_open	duxcd_inval
extern duxcd_close();
extern duxcd_rdwr();
#define duxcd_ioctl	duxcd_inval
extern duxcd_ioctl();
#define duxcd_select	duxcd_inval
/*extern duxcd_select(); */
#define duxcd_create	duxcd_inval	/*NOTE: need change for local mount*/
extern cdfs_access();
#define duxcd_access	cdfs_access	/*This won't quite work, but for now..*/
/*extern duxcd_access();*/
#define duxcd_lookup	duxcd_inval
#define duxcd_remove	duxcd_inval
extern	duxcd_getattr();
#define duxcd_setattr	duxcd_inval
#define duxcd_link	duxcd_inval
#define duxcd_rename	duxcd_inval
#define duxcd_mkdir	duxcd_inval
#define duxcd_rmdir	duxcd_inval
#define duxcd_readlink	duxcd_inval
extern duxcd_readdir();
#define duxcd_symlink	duxcd_inval
extern cdfs_fsync();
#define duxcd_fsync	cdfs_fsync
extern cdfs_inactive();
#define duxcd_inactive	cdfs_inactive
extern int cdfs_bmap();
#define duxcd_bmap	cdfs_bmap
#define duxcd_bread	duxcd_inval
#define duxcd_brelse	duxcd_inval
extern	duxcd_fpathconf();
extern	duxcd_pathsend();
#ifdef ACLS
#define duxcd_getacl	duxcd_not_apply
#define duxcd_setacl	duxcd_not_apply
#endif
#ifdef	POSIX
#define duxcd_pathconf	duxcd_inval
#endif	POSIX
#define duxcd_lockctl duxcd_not_apply
#define duxcd_lockf duxcd_not_apply
#define duxcd_fid duxcd_not_apply
extern duxcd_fsctl();
extern struct buf *cdfs_readb();
extern int cdfs_pagein();
extern int vfs_prefill();
extern int vfs_pageout();

/*
** The dux_strategy() routine is replaced by the dux_nop() routine
** for configurability purposes. If the kernel is configured with
** DUX in, then the duxcd_vnodeops location corresponding to dux_strategy
** is filled in at boot time. - daveg (See dux_hooks.c)
*/
extern dux_nop();

struct vnodeops duxcd_vnodeops =
{
	duxcd_open,
	duxcd_close,
	duxcd_rdwr,
	duxcd_ioctl,
	duxcd_select,
	duxcd_getattr,
	duxcd_setattr,
	duxcd_access,
	duxcd_lookup,
	duxcd_create,
	duxcd_remove,
	duxcd_link,
	duxcd_rename,
	duxcd_mkdir,
	duxcd_rmdir,
	duxcd_readdir,
	duxcd_symlink,
	duxcd_readlink,
	duxcd_fsync,
	duxcd_inactive,
	duxcd_bmap,
	duxcd_strategy,
	duxcd_bread,
	duxcd_brelse,
	duxcd_pathsend,
#ifdef ACLS
	duxcd_setacl,
	duxcd_getacl,
#endif
#ifdef	POSIX
	duxcd_pathconf,
	duxcd_fpathconf,
#endif	POSIX
	duxcd_lockctl,
	duxcd_lockf,
	duxcd_fid,
	duxcd_fsctl,
        vfs_prefill,
        cdfs_pagein,
        vfs_pageout,
        NULL,
        NULL,
};

duxcd_not_apply()
{
	return(EINVAL);
}

duxcd_inval()
{
	panic ("Unimplemented dux cdrom vnode operation");
}

int duxcd_strategy(bp)
struct buf *bp;
{
	DUXCALL(DUX_STRATEGY)(bp);
}

int
duxcd_fpathconf(vp, name, resultp, cred)
struct vnode *vp;
int name;
int	*resultp;
struct ucred *cred;
{
	DUXCALL(DUX_FPATHCONF)(vp, name, resultp, cred);
}

int
duxcd_getattr(vp, vap, cred, vsync)
struct vnode *vp;
struct vattr *vap;
struct ucred *cred;
enum vsync vsync;
{
	DUXCALL(DUX_GETATTR)(vp, vap, cred, vsync);
}

duxcd_pathsend(vpp,pnp,followlink,nlinkp,dirvpp,compvpp,opcode,dependent)
	struct vnode **vpp;
	struct pathname *pnp;
	enum symfollow followlink;
	int *nlinkp;
	struct vnode **dirvpp;
	struct vnode **compvpp;
	int opcode;
	caddr_t dependent;
{
	DUXCALL(DUX_PATHSEND)(vpp,pnp,followlink,nlinkp,dirvpp,compvpp,opcode,dependent);
}



/* READ/WRITE CALLS */

#define	FILE_CHUNK	4
#define	FILE_CHUNK_SHIFT	2
int
dux_rwcdp(cdp, uio, rw, transform)
	register struct cdnode *cdp;
	register struct uio *uio;
	enum uio_rw rw;
	int transform;
{
	u_int	size;
	u_int	bn;
	u_int	on, n;
	int	nextread;
	int	diff;
	struct vnode	*vp;
	struct buf	*bp, *tmpbp=0;
	char	*cp1, *cp2;

	if (rw != UIO_READ) {
		return(EROFS);
	}
	if (uio->uio_resid == 0)
		return (0);
	/* If the serving site has died, return an error */
	if (devsite(cdp->cd_dev) == 0)
		return (DM_CANNOT_DELIVER);
	vp = CDTOV(cdp);

	u.u_error = 0;
	do {
		dxcd_bmap(cdp, uio->uio_offset, &bn, &size, &on);
		n = MIN(uio->uio_resid, size - on);
		diff = cdp->cd_size - uio->uio_offset;
		if (diff <= 0) return(0);
		if (diff < n) n = diff;
		nextread = cdp->cd_nextr;
		cdp->cd_nextr = rablock;
		if (rablock && (nextread == rablock)) {
			bp = breada(vp, bn, size,
#ifdef	FSD_KI
				rablock, rasize, B_dux_rwcdp | B_data );
#else 	not FSD_KI
				rablock, rasize);
#endif	FSD_KI
		}
		else
#ifdef	FSD_KI
			bp = bread(vp, bn, size, B_dux_rwcdp|B_data);
#else	not FSD_KI
			bp = bread(vp, bn, size);
#endif	FSD_KI
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}
		n = MIN(n, bp->b_bcount - bp->b_resid);
		
		cp1 = bp->b_un.b_addr+on;
		if (transform == 1) {
		    tmpbp = geteblk(n);
		    bcopy(cp1, (cp2=tmpbp->b_un.b_addr), n);
		    cddir_to_dir(cdp, uio->uio_offset, cp2, n);
		    cp1 = cp2;
		}
		u.u_error = uiomove(cp1, n, rw, uio);
		if(tmpbp) brelse(tmpbp);
		brelse(bp);
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);
	return (u.u_error);
}

dxcd_bmap(cdp, offset, bn, size, on)
struct cdnode	*cdp;
u_int	offset;
u_int	*bn;
u_int	*size;
u_int	*on;
{
	register u_int	bsize, bmask;
	register u_int	nextchunk;
	register u_int	fsize = cdp->cd_size;

	bsize = cdp->cd_fs->cdfs_sec_size << FILE_CHUNK_SHIFT;
	bmask = bsize - 1;
	*on = offset & bmask;
	nextchunk = offset & ~bmask;
	*bn = nextchunk >> DEV_BSHIFT;
	nextchunk += bsize;
	if (nextchunk > fsize) {
		rablock=0;
		rasize=0;
		*size = fsize & bmask;
	}
	else {
		*size = bsize;
		rablock = nextchunk >> DEV_BSHIFT;
		nextchunk += bsize;
		if (nextchunk > fsize) {
			rasize = fsize & bmask;
		}
		else {
			rasize = bsize;
		}
	}
}

/*ARGSUSED*/
duxcd_rdwr(vp, uiop, rw, ioflag, cred)
struct vnode	*vp;
struct uio	*uiop;
enum uio_rw	rw;
int	ioflag;
struct ucred *cred;
{
	return(dux_rwcdp(VTOCD(vp), uiop, rw, 0));
}


/* CLOSE CALL */

/*ARGSUSED*/
int
duxcd_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
{
	closecd_send (VTOCD(vp), flag);
	return (0);
}
/*
 *Send a close message.
 *This is separated out from closing devices, to permit calling of this
 *function if a device open fails.
 */
closecd_send(cdp, flag)
	register struct cdnode *cdp;
	int flag;
{
	dm_message request, reply;
	register struct closereq *rp;

	request = dm_alloc(sizeof (struct closereq), WAIT);
	rp = DM_CONVERT(request, struct closereq);
	rp->dev = cdp->cd_dev;
	rp->cdnumber = cdp->cd_num;
	rp->fstype = MOUNT_CDFS;
	rp->flag = flag;
	rp->timeflag = 0;
	reply = dm_send (request, DM_SLEEP|DM_RELEASE_REQUEST,
		DM_CLOSE, devsite (cdp->cd_dev), DM_EMPTY,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	dm_release(reply,0);
}


closecd_serve(rp)
struct closereq *rp;
{
	register struct cdnode *cdp;

	cdp = cdfind (localdev(rp->dev), rp->cdnumber);
	if (cdp == NULL)	/*can't happen*/
	{
		printf("cdfind failed:  closecd_serve\n");
		dm_quick_reply (ENOENT);
		return;
	}
	updatesitecount (&(cdp->cd_opensites), u.u_site, -1);
	cdlock(cdp);
	cdput(cdp);
	dm_quick_reply (0);
}


/* close an cdnode on the server when the client's cdnode table */
/* is full */
close_send_no_cdno(dev, cdnum, flag)
	dev_t dev;
	cdno_t cdnum;
	int flag;
{
	dm_message request, reply;
	register struct closereq *rp;

	request = dm_alloc(sizeof(struct closereq), WAIT);
	rp = DM_CONVERT(request, struct closereq);
	rp->dev = dev;
	rp->cdnumber = cdnum;
	rp->fstype = MOUNT_CDFS;
	rp->flag = flag;
	rp->timeflag = 0;
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST,
		DM_CLOSE, devsite(dev), sizeof(struct closerepl),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	dm_release(reply, 0);
}


/*ARGSUSED*/
duxcd_readdir(vp, uiop, cred)
struct vnode *vp;
register struct uio *uiop;
struct ucred *cred;
{
	register struct iovec *iovp;
	register int count;

	iovp = uiop->uio_iov;
	count = iovp->iov_len;
	if ((uiop->uio_iovcnt != 1) || (count < DIRBLKSIZ) ||
	    (uiop->uio_offset & (DIRBLKSIZ -1)))
		return (EINVAL);
	count &= ~(DIRBLKSIZ - 1);
	uiop->uio_resid -= iovp->iov_len - count;
	iovp->iov_len = count;
	return (dux_rwcdp(VTOCD(vp), uiop, 0, 1));
}

	
/*ARGSUSED*/
duxcd_fsctl(vp, command, uiop, cred)
struct	vnode	*vp;
register int	command;
struct uio	*uiop;
int	cred;
{
	register struct	cdfs	*cdfsp;
	register struct cdnode	*cdp;
	register char *infop;
	int	datalen;
	int	error = 0;
	struct	buf *bp;
	dm_message request;
	dm_message reply;
	register struct fsctl_request *fstrqp;
	register struct fsctl_reply *fstrep;

	cdp = VTOCD(vp);
	cdfsp = cdp->cd_fs;

	switch (command) {
	case CDFS_DIR_REC:
	   {
		request = dm_alloc(sizeof(struct fsctl_request),WAIT);
		fstrqp = DM_CONVERT(request, struct fsctl_request);
		bp = geteblk (MAX_CDDIR_RECLEN);
		fstrqp->fstype = MOUNT_CDFS;
		fstrqp->command = command;
		fstrqp->dev = cdp->cd_dev;
		fstrqp->un_fs.cdnumber = cdp->cd_num;
		reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST|
				DM_REPLY_BUF|DM_INTERRUPTABLE,
			DM_FSCTL, devsite(cdp->cd_dev), 
			sizeof(struct fsctl_reply), NULL, bp, 
			MAX_CDDIR_RECLEN, 0, NULL, NULL, NULL);

		if (error = DM_RETURN_CODE(reply)) {
			brelse(bp);
			return(error);
		}
		fstrep = DM_CONVERT(reply, struct fsctl_reply);
		error=uiomove(bp->b_un.b_addr, fstrep->bytes_transfered, 
			UIO_READ, uiop);
		brelse(bp);
		dm_release(reply,0);
		return(error);
	   }
	case CDFS_XAR:
	   {
		int	which_blk;
		int	left = cdlbtooff(cdp->cd_fs, cdp->cd_xarlen);

		if (left == 0) return(ENOENT);

		request = dm_alloc(sizeof(struct fsctl_request),WAIT);
		fstrqp = DM_CONVERT(request, struct fsctl_request);

		bp = geteblk (cdp->cd_fs->cdfs_sec_size<<FILE_CHUNK_SHIFT);

		fstrqp->fstype = MOUNT_CDFS;
		fstrqp->command = command;
		fstrqp->dev = cdp->cd_dev;
		fstrqp->un_fs.cdnumber = cdp->cd_num;
		for(which_blk = 0; (error == 0) && (left > 0); which_blk++) {
		   fstrqp->arg1 = which_blk;

		   reply = dm_send(request,
			DM_SLEEP| DM_REPLY_BUF| DM_INTERRUPTABLE, 
			DM_FSCTL, devsite(cdp->cd_dev), 
			sizeof(struct fsctl_reply), NULL, bp, 
			sizeof(struct cddir), 0, NULL, NULL, NULL);

		   if (error = DM_RETURN_CODE(reply))  break;

		   fstrep = DM_CONVERT(reply, struct fsctl_reply);
		   error=uiomove(bp->b_un.b_addr, fstrep->bytes_transfered, 
			UIO_READ, uiop);
		   left -= fstrep->bytes_transfered;
		   dm_release(reply,0);
		   if(error) break;
		}
		brelse(bp);
		dm_release(request,0);
		return(error);
	   }
	case CDFS_AFID:
		infop = cdfsp->cdfs_abstract;
		break;
	case CDFS_BFID:
		infop = cdfsp->cdfs_bibliographic;
		break;
	case CDFS_CFID:
		infop = cdfsp->cdfs_copyright;
		break;
	case CDFS_VOL_ID:
		infop = cdfsp->cdfs_vol_id;
		break;
	case CDFS_VOL_SET_ID:
		infop = cdfsp->cdfs_vol_set_id;
		break;
	default:
		return(EINVAL);
	}
	if((datalen= cdfsnmlen(infop)) == 1) {
		return(ENOENT);
	}
	return(uiomove(infop, datalen, UIO_READ, uiop));
}

fsctlcd_serve(fstrqp)
register struct fsctl_request *fstrqp;
{
	register struct buf *bp;
	register struct cdnode *cdp;
	register struct cdfs *cdfsp;
	dev_t dev = localdev(fstrqp->dev);
	struct fsctl_reply *fstrep;
	u_int bytes;
	int	error;
	dm_message reply;
	u_int	bsize, bshift;

	cdp = cdfind(dev, fstrqp->un_fs.cdnumber);
	if (cdp == NULL) {
		printf("cdfind failed: fsctlcd_serve\n");
		dm_quick_reply(EIO);
		return;
	}
	cdfsp = cdp->cd_fs;
	switch(fstrqp->command) {
	case CDFS_DIR_REC:
	{
		struct cdnode *dcdp;
		int tmpcdno;
		if(cdp->cd_pnum == 0) {
			if(error = fill_pnum(cdp)) {
				dm_quick_reply(error);
				return;
			}
		}

		dcdp = cdget(cdp->cd_dev, cdfsp, cdp->cd_pnum, 0, 0);
		if(dcdp == 0) {
			dm_quick_reply(u.u_error);
			return;
		}

		bp = geteblk(MAX_CDDIR_RECLEN); 
		if (CDTOV(cdp) -> v_type == VDIR) {
		   dcdp->cd_diroff = 0;
		   while((error = get_dir_rec(dcdp, bp->b_un.b_addr, 
			  &tmpcdno, 0)) == 0) {
			if (tmpcdno == cdp->cd_num) break;
			if (dcdp->cd_diroff == 0) {
				printf("cdfs_fsctl: error in getting dir. rec\n");
				error=EINVAL;
				break;
			}
		   }
			
		}
		else {
		   dcdp->cd_diroff = cdp->cd_num - 
				  cdlbtooff(cdfsp, 
					    (dcdp->cd_loc+dcdp->cd_xarlen));
		   error = get_dir_rec(dcdp, bp->b_un.b_addr, &tmpcdno, 1);
		}
		cdunlock(dcdp);
		VN_RELE(CDTOV(dcdp));
		if (error) {
			brelse(bp);
			dm_quick_reply(error);
			return;
		}
		bytes = ((struct cddir *)bp->b_un.b_addr)->cdd_reclen;
/*debug. kao*/	if((bytes > MAX_CDDIR_RECLEN) || (bytes < sizeof(struct min_cddir))) panic ("fsctlcd_serve: bad cddir reclen");
		break;
	}
	case CDFS_XAR:
	{
		u_int	offset;
		int	xarlen;
		bshift = cdfsp->cdfs_lsshift+FILE_CHUNK_SHIFT;
		bsize = 1 << bshift;
		xarlen = cdlbtooff(cdfsp, cdp->cd_xarlen);
		offset = (fstrqp->arg1) << bshift;
		bytes = min(bsize, xarlen-offset);
		bp = cdfs_readb(cdp->cd_devvp, 
				cdlbtooff(cdfsp, cdp->cd_loc) + offset, 
				bytes);
		if (bp == 0) {
			dm_quick_reply(EIO);
			return;
		}
		break;
	}
	default:
		panic("fsctlcd_serve: illegal command");
	}
	reply = dm_alloc(sizeof(struct fsctl_reply),WAIT);
	fstrep = DM_CONVERT(reply, struct fsctl_reply);
	fstrep->bytes_transfered =  bytes;
	dm_reply(reply, DM_REPLY_BUF, 0, bp, bytes, 0);
}

duxcd_pseudo_inactive(vp)
struct vnode *vp;
{
register struct cdnode *cdp;
extern struct cdnode *cdfreeh, **cdfreet;

	cdp = VTOCD(vp);
	cdp->cd_num = 0;
	cdp->cd_flag = IBUFVALID|IPAGEVALID;
	/*put the inode on the free list.*/
	if (cdfreeh) {
		*cdfreet = cdp;
		cdp->cd_freeb = cdfreet;
	} else {
		cdfreeh = cdp;
		cdp->cd_freeb = &cdfreeh;
	}
	cdp->cd_freef = NULL;
	cdfreet = &cdp->cd_freef;
	return (0);
}

