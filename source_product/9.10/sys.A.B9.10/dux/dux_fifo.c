/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_fifo.c,v $
 * $Revision: 1.9.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:40:46 $
 */
/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
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

#include "../h/types.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/systm.h"
#include "../h/vnode.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../dux/dux_dev.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/lookupops.h"
#include "../dux/dux_lookup.h"
#include "../dux/duxfs.h"
#include "../h/kern_sem.h"

extern struct vnode *pipevp;

#ifdef MP
#include "../machine/reg.h"
#include "../machine/inline.h"
#include "../machine/cpu.h"
#endif

/* pipe request message */
struct pipereq {		/*DUX MESSAGE STRUCTURE*/
	dev_t dev;
	ino_t inumber;
	struct packed_ucred cred;
};

/* pipe reply message */

struct piperepl {		/*DUX MESSAGE STRUCTURE*/
	struct remino rmtip;
};


/*
 * pipe_send() -- pipe packetize routine... set up and send pipe 
 *  message, wait for reply.
 * when reply comes in, set up inode and return pointer to it.
 */
struct inode *
pipe_send(pvp)
struct vnode *pvp;
{
	register struct inode *pip = VTOI(pvp);
	register dm_message reqp, replp;
	register struct pipereq *sreqp;
	register struct piperepl *sreplp;
	register struct inode *ip = NULL;
	struct inode *stprmi();
	ino_t inum;
	int error;

	reqp = dm_alloc(sizeof(struct pipereq), WAIT);
	sreqp = DM_CONVERT(reqp, struct pipereq);
	sreqp->dev = pip->i_dev;
	sreqp->inumber = pip->i_number;
	pack_ucred(u.u_cred, &sreqp->cred);

	replp = dm_send(reqp, DM_SLEEP|DM_RELEASE_REQUEST, DM_PIPE,
		devsite(pip->i_dev), sizeof(struct piperepl),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	if (u.u_error = DM_RETURN_CODE(replp))
	{
		dm_release(replp,0);
		return (NULL);
	}

	sreplp = DM_CONVERT(replp, struct piperepl);
	ip = stprmi(&(sreplp->rmtip),NULL);
	if (ip == NULL) {
		error = u.u_error;
		inum = (sreplp->rmtip).ino;
		dm_release(replp, 0);
		close_send_no_ino(pip->i_dev, inum, FREAD);
		close_send_no_ino(pip->i_dev, inum, FWRITE);
		u.u_error = error;
		return(NULL);
	}
	dm_release(replp, 0);
	ip->i_frcnt = 1;
	ip->i_fwcnt = 1;
	VN_HOLD(ITOV(ip));	/* need reference count of 2 for fifo */

	return(ip);
}

/*
 * pipe_recv -- pipe depacketize routine.  This routine is called by
 * an nsp which is servicing a remote request.  This routine calls the 
 * serving site version of pipe (ss_pipe()), and then sends back the
 * reply message.
 */

pipe_recv(dmpt)
	register dm_message dmpt;
{
	register struct inode *ip;
	register struct inode *pip;
	register struct pipereq *sreqp = DM_CONVERT(dmpt, struct pipereq);
	register dm_message replp;
	register struct piperepl *sreplp;
	struct inode *ss_pipe();
	struct ucred ucred;

	pip = ifind(localdev(sreqp->dev),sreqp->inumber);
	if (pip == NULL)
	{
		dm_quick_reply(ENOENT);
		return;
	}
	unpack_ucred(&sreqp->cred, &ucred);
	u.u_cred = &ucred;
	if ((ip = ss_pipe(ITOV(pip))) == NULL)
	{
			/* send back error reply message */
		restore_ulookup();	/*to restore the ucred*/
		dm_quick_reply(u.u_error);
		return;
	}
	iunlock(ip);
	restore_ulookup();	/*to restore the ucred*/
	/* send back successful reply message */
	replp = dm_alloc(sizeof(struct piperepl), WAIT);
	sreplp = DM_CONVERT(replp, struct piperepl);
	pkrmi(ip, &sreplp->rmtip);
	dm_reply(replp, 0, u.u_error, NULL, NULL, NULL);
}

/* wait for fifo open request message */

struct openwreq {	/*DUX MESSAGE STRUCTURE*/
	dev_t dev;
	ino_t inumber;
	int mode;
};
/*
 *Send an openp wait request.  This will only occur if the original
 *open occurred when sync I/O was not required, ans sync I/O was required
 *before the open could complete
 */
openp_wait_send(ip,mode)
register struct inode *ip;
int mode;
{
	register dm_message reqp, replp;
	register struct openwreq *sreqp;

	reqp = dm_alloc(sizeof(struct openwreq), WAIT);
	sreqp = DM_CONVERT(reqp, struct openwreq);
	sreqp->dev = ip->i_dev;
	sreqp->inumber = ip->i_number;
	sreqp->mode = mode;

	replp = dm_send(reqp, DM_SLEEP|DM_RELEASE_REQUEST|DM_INTERRUPTABLE, DM_OPENPW,
		devsite(ip->i_dev), DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);

	u.u_error = DM_RETURN_CODE(replp);
	dm_release(replp, 0);
}

openp_wait_recv(dmpt)
	register dm_message dmpt;
{
	register struct inode *ip;
	register struct openwreq *sreqp = DM_CONVERT(dmpt, struct openwreq);

	ip = ifind(localdev(sreqp->dev),sreqp->inumber);
	if (ip == NULL)
	{
		dm_quick_reply(ENOENT);
		return;
	}
	openp_wait(ip,sreqp->mode);
	dm_quick_reply(u.u_error);
}

#ifdef FULLDUX
/*
 *change the disc where pipes should be put.  Do this by specifying the
 *root directory of the mounted device.
 */
pipenode(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{
	struct vnode *vp;

	if (!suser())
		return;
	PSEMA(&filesys_sema);
        u.u_error =
	    lookupname(uap->dirnamep, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_LOOKUP);
	if (u.u_error){
		VSEMA(&filesys_sema);
		return;
	}
	if (vp->v_fstype != VUFS && vp->v_fstype != VDUX)
	{
		u.u_error = EINVAL;
		update_duxref(pipevp,-1,0);
		VN_RELE(vp);
		VSEMA(&filesys_sema);
		return;
	}
	update_duxref(pipevp,-1,0);
	VN_RELE(pipevp);
	pipevp = vp;
	VSEMA(&filesys_sema);
}
#endif	FULLDUX

dux_fifo_invalidate(ip)
register struct inode *ip;
{
	/* Just invalidate the fifo pages here */
	invalip(ip);
	ip->i_size = 0;
	ip->i_blocks = 0;
	ip->i_fifosize = 0;
	ip->i_frptr = 0;
	ip->i_fwptr = 0;
	ip->i_fflag = 0;
}

dux_fifo_read(ip, uio, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	register int ioflag;
{
	struct vnode *vp;
	struct buf *bp;
	struct duxfs *dfs;
	daddr_t lbn, bn;
	register int n, on;
	int size;
	long bsize;
	int error = 0;
	register u_int reg_temp;

	if (uio->uio_resid == 0)
		return (0);
	vp = ITOV(ip);
	dfs = ip->i_dfs;
	bsize = dfs->dfs_bsize;
restart:
	if (ip->i_flag & ISYNC)
		return (syncio(ip, uio, UIO_READ, ioflag));
	if (ip->i_fifosize == 0) {
		if (ip->i_fwcnt == 0)
			return(0);
#ifdef POSIX
		if (uio->uio_fpflags&FNBLOCK)
			return(EAGAIN);
#endif
		if (uio->uio_fpflags&FNDELAY)
			return(0);
		ip->i_fflag |= IFIR;
		iunlock(ip);
		drop_dev_vp(vp);
		sleep((caddr_t)&ip->i_frcnt, PPIPE);
		use_dev_vp(vp);
		ilock(ip);
		/* we may need to use sync I/O so recheck */
		goto restart;
	}
	uio->uio_offset = ip->i_frptr;
	do {
		int diff;

		/*make sure the site hasn't gone away while we were sleeping*/
		if (devsite(ip->i_dev) == 0)
		{
			return (DM_CANNOT_DELIVER);
		}
		/*make sure incore buffers are valid*/
		if (!(ip->i_flag&IBUFVALID))
		{
			invalip(ip);
			ip->i_flag |= IBUFVALID;
		}
		lbn = uio->uio_offset / bsize;
		on = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);
		diff = ip->i_fifosize;
		if (diff <= 0)
			break;
		if (diff < n)
			n = diff;
		dux_bmap(vp, lbn, NULL, &bn);
		size = dux_blksize(dfs, ip, lbn);
#ifdef	FSD_KI
		bp = bread(vp, bn, size, 
			B_dux_fifo_read|B_data);
#else	FSD_KI
		bp = bread(vp, bn, size);
#endif	FSD_KI
		if (bp->b_flags & B_CACHE)
			imark(ip, IACC);
		ip->i_lastr = lbn;
		n = MIN(n, size - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return(EIO);
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, UIO_READ, uio);
	
		ip->i_fifosize -= n;
		if (uio->uio_offset >= PIPSIZ)
			uio->uio_offset = 0;
			if ((on+n) == bsize &&
		   	    ip->i_fifosize < (PIPSIZ-bsize)&&
			    (bp->b_flags & B_DELWRI)) {
				bp->b_flags &= ~B_DELWRI;
				clear_delwri(bp);
			}
		brelse(bp);
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);

	if (ip->i_fifosize)
		ip->i_frptr = uio->uio_offset;
	else
		ip->i_frptr = ip->i_fwptr = 0;
	fselwakeup(&ip->i_fselw);
	if (ip->i_fflag&IFIW) {
		ip->i_fflag &= ~IFIW;
#ifdef MP
		SETCURPRI(PPIPE, reg_temp);
#else
		curpri = PPIPE;
#endif
		wakeup((caddr_t)&ip->i_fwcnt);
	}

	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
	return (error);
}

dux_fifo_write(ip, uio, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	register int ioflag;
{
	struct vnode *vp;
	struct buf *bp;
	struct duxfs *dfs;
	daddr_t lbn, bn;
	register int n, on;
	int size;
	long bsize;
	int error = 0;
	unsigned int usave;
	register u_int reg_temp;

	if (uio->uio_resid == 0)
		return (0);

	dfs = ip->i_dfs;
	bsize = dfs->dfs_bsize;
	vp = ITOV(ip);

	if (uio->uio_fpflags&FNDELAY) {
		if (ip->i_fifosize >= PIPSIZ) /* no room */
			return(0);
		if ((uio->uio_resid <= PIPSIZ) /* must be atomic */
		&& ((uio->uio_resid+ip->i_fifosize) > PIPSIZ))
			return(0);
	}
#ifdef 	POSIX
	if (uio->uio_fpflags&FNBLOCK) {
		if (ip->i_fifosize >= PIPSIZ) { /* no room */
			return(EAGAIN);
		}
		if ((uio->uio_resid <= PIPSIZ) /* must be atomic */
		&& ((uio->uio_resid+ip->i_fifosize) > PIPSIZ)) {
			return(EAGAIN);
		}
	}
#endif 	POSIX
floop:
	if (ip->i_flag & ISYNC)
	{
		error = syncio(ip, uio, UIO_WRITE, ioflag);
		if (error == EPIPE)	/*signal the process*/
			psignal(u.u_procp, SIGPIPE);
		return (error);
	}
	usave = 0;
	while ((uio->uio_resid+ip->i_fifosize) > PIPSIZ) {
		if (ip->i_frcnt == 0)
			break;
		if ((uio->uio_resid > PIPSIZ) && (ip->i_fifosize < PIPSIZ)) {
			usave = uio->uio_resid;
			uio->uio_resid = PIPSIZ - ip->i_fifosize;
			usave -= uio->uio_resid;
			break;
		}
		ip->i_fflag |= IFIW;
		iunlock(ip);
		drop_dev_vp(vp);
		sleep((caddr_t)&ip->i_fwcnt, PPIPE);
		use_dev_vp(vp);
		ilock(ip);
		if (ip->i_flag & ISYNC)
		{
			error = syncio(ip, uio, UIO_WRITE, ioflag);
			if (error == EPIPE)	/*signal the process*/
				psignal(u.u_procp, SIGPIPE);
			return (error);
		}
	}
	if (ip->i_frcnt == 0) {
		u.u_error = EPIPE;
		psignal(u.u_procp, SIGPIPE);
		return(u.u_error);
	}
	uio->uio_offset = ip->i_fwptr;

	do {
		/*make sure the site hasn't gone away while we were sleeping*/
		if (devsite(ip->i_dev) == 0)
		{
			return (DM_CANNOT_DELIVER);
		}
		/*make sure incore buffers are valid*/
		if (!(ip->i_flag&IBUFVALID))
		{
			invalip(ip);
			ip->i_flag |= IBUFVALID;
		}
		lbn = uio->uio_offset / bsize;
		on = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);

		dux_bmap(vp, lbn, NULL, &bn);
		ip->i_fifosize += n;
		if (uio->uio_offset + n > ip->i_size)
		{
			u.u_error = dux_grow(ip, uio->uio_offset + n);
			if (u.u_error)
				return (u.u_error);
		}
		size = dux_blksize(dfs, ip, lbn);

		if (n == bsize) 
#ifdef	FSD_KI
			bp = getblk(vp, bn, size, 
				B_dux_fifo_write|B_data);
#else	FSD_KI
			bp = getblk(vp, bn, size);
#endif	FSD_KI
		else if (on==0 && (ip->i_fifosize-n) <= (PIPSIZ-bsize))
#ifdef	FSD_KI
			bp = getblk(vp, bn, size, 
				B_dux_fifo_write|B_data);
#else	FSD_KI
			bp = getblk(vp, bn, size);
#endif	FSD_KI
		else
#ifdef	FSD_KI
			bp = bread(vp, bn, size, 
				B_dux_fifo_write|B_data);
#else	FSD_KI
			bp = bread(vp, bn, size);
#endif	FSD_KI

		n = MIN(n, size - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}
		u.u_error =
		    uiomove(bp->b_un.b_addr+on, n, UIO_WRITE, uio);

		bdwrite(bp);

		if (uio->uio_offset == PIPSIZ)
			uio->uio_offset = 0;

		imark(ip, IUPD|ICHG);

	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);

	ip->i_fwptr = uio->uio_offset;
	fselwakeup(&ip->i_fselr);
	if (ip->i_fflag&IFIR) {
		ip->i_fflag &= ~IFIR;
#ifdef MP
		SETCURPRI(PPIPE, reg_temp);
#else
		curpri = PPIPE;
#endif
		wakeup((caddr_t)&ip->i_frcnt);
	}
	if (u.u_error == 0 && usave != 0) {
		uio->uio_resid = usave;
#ifdef POSIX
		if (!(uio->uio_fpflags&(FNBLOCK|FNDELAY)))
#endif POSIX
		goto floop;
	}

	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
	return (error);
}
