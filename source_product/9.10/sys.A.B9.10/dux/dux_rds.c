/* $Header: dux_rds.c,v 1.7.83.4 93/12/15 12:01:57 marshall Exp $ */
/* HPUX_ID: @(#)dux_rds.c	55.1		88/12/23 */

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
#endif   /* _SYS_STDSYMS_INCLUDED  */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/errno.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/vnode.h"
#include "../h/swap.h"
#include "../ufs/inode.h"
#include "../dux/sitemap.h"
#include "../h/devices.h"
#include "../dux/dux_rdX.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../h/kern_sem.h"


/*  rds_opend duplicates the code for opend, except that the requesting
 *  site ID is passed to alloc_dev() instead of my_site.
 */

/*ARGSUSED*/
rds_opend(dev, mode, omode, src)
dev_t  dev;
u_int  mode, omode;
site_t src;
{ 
#ifdef FULLDUX
	register int        error, maj;
	register dtaddr_t   dtp;
	register dm_message msg;

	dtp = alloc_dev(dev, mode, src);

	maj = major(dev);

	if (dtp == NULL)
	{	error = EIO;
	}
	else if (setjmp(&u.u_qsave))
	{
		/* MP - reacquire semaphores */
		error = u.u_error;
		if (error == 0)
			error = EINTR;
	}
	else if(mode == IFBLK)
		if(maj < nblkdev)
		{ 
#ifdef _WSIO
			if(bdevsw[maj].d_flags & C_ALLCLOSES)
				dtp->dt_flags |= D_ALLCLOSES;
#endif /* _WSIO */
			error = (*bdevsw[maj].d_open)(dev, omode);
		}
		else
			error = ENXIO;
	else
		if(maj < nchrdev)
		{
#ifdef _WSIO
			if(cdevsw[maj].d_flags & C_ALLCLOSES)
				dtp->dt_flags |= D_ALLCLOSES;
#endif /* _WSIO */
			error = (*cdevsw[maj].d_open)(dev, omode);
		}
		else
			error = ENXIO;

	if(error != 0)
	{ 
		updatesitecount(&(dtp->dt_map), src, -1);
		if(gettotalsites(&(dtp->dt_map)) == 0)
			dev_rele(dtp);

		msg = NULL;
	}
	else
	{ 
		msg = dm_alloc(RDS_SIZE(0), WAIT);
		DM_CONVERT(msg, rds_t)->rds_index = devindex(dev, mode);
	}

	dm_reply(msg, 0, error, NULL, NULL, NULL);
#else
	panic("rds_opend: remote devices not supported");
#endif
}

/*ARGSUSED*/
rds_closed(dev, mode, flag, site)
dev_t  dev;
u_int  mode, flag;
site_t site;
{ 
#ifdef FULLDUX
	register int error;

	error = closed(dev, mode, flag, site);
	dm_reply(NULL, 0, error, NULL, NULL, NULL);
#else
	panic("rds_closed: remote devices not supported");
#endif
}

/*ARGSUSED*/
rds_rw(req, dev, rdu, bp)
register int   req;
register dev_t dev;
register rdu_t *rdu;
struct buf     *bp;
{ 
#ifdef FULLDUX
	struct uio uio;
	struct iovec   iovec;
	rds_t          *rpl;
	register int   error = ENXIO;
	dm_message     resp;

	if(req == DMNDR_READ)
		if(rdu->rdu_uio.uio_resid <= DMRD_IOSIZE)
		{ 
			resp = dm_alloc(RDS_SIZE(rdu->rdu_uio.uio_resid), WAIT);
			bp   = NULL;
		}
		else
		{ 
			resp = dm_alloc(RDS_SIZE(0), WAIT);
			bp   = geteblk(rdu->rdu_uio.uio_resid);
		}
	else
		resp = dm_alloc(RDS_SIZE(0), WAIT);

	rpl = DM_CONVERT(resp, rds_t);

	uio.uio_offset = rdu->rdu_uio.uio_offset;
	uio.uio_resid  = rdu->rdu_uio.uio_resid;
	uio.uio_fpflags = rdu->rdu_uio.uio_fpflags;
	uio.uio_seg = UIOSEG_KERNEL;
	uio.uio_iovcnt = 1;
	uio.uio_iov    = &iovec;

	iovec.iov_len  = uio.uio_resid;
	if(uio.uio_resid > DMRD_IOSIZE)
		iovec.iov_base = bp->b_un.b_addr;
	else
		iovec.iov_base = (req == DMNDR_READ ? rpl->rds_iodata : rdu->rdu_iodata);

	if (setjmp(&u.u_qsave))
	{
		/* MP - reacquire semas */
		if ((u.u_procp->p_flag & SOUSIG) == 0)
		{
			error = 0;
			if (uio.uio_resid == rdu->rdu_uio.uio_resid)
			{
				u.u_eosys = RESTARTSYS;
				if (bp)
				{
					brelse (bp);
					bp = NULL;
				}
			}
		}
		else
		{
			brelse (bp);
			dm_reply(resp, DM_LONGJMP, EINTR, NULL, NULL, NULL);
			return;
		}
	}
	else if(req == DMNDR_WRITE)
		error = (*cdevsw[major(dev)].d_write)(dev, &uio);
	else
		error = (*cdevsw[major(dev)].d_read)(dev, &uio);

	rpl->rds_resid  = uio.uio_resid;

	if(req == DMNDR_WRITE || bp == NULL)
		dm_reply(resp, 0, error, NULL, NULL, NULL);
	else
		dm_reply(resp, DM_REPLY_BUF, error, bp, bp->b_bcount, 0);
#else
	panic("rds_rw: remote devices not supported");
#endif
}

/*ARGSUSED*/
rds_strat(dev, rdu, rbp, site, opcode)
register dev_t dev;
register rdu_t *rdu;
struct buf     *rbp;
int opcode;
{ 
	register struct buf *bp;
	dm_message resp;
	struct vnode *devvp;
	swpdbd_t swdbd;
	int adjust;
	sv_sema_t ss;

	resp = dm_alloc(RDS_SIZE(0), WAIT);

	if(rdu->rdu_strat.flags & B_FSYSIO)
	{
		devvp = devtovp(dev);
		if(rdu->rdu_strat.flags & B_READ)
		{ 
#ifdef	FSD_KI
			bp = bread(devvp, rdu->rdu_strat.blkno, rdu->rdu_strat.bcount,
				B_rds_strat|B_unknown);
#else	FSD_KI
			bp = bread(devvp, rdu->rdu_strat.blkno, rdu->rdu_strat.bcount);
#endif	FSD_KI
			DM_CONVERT(resp, rds_t)->rds_resid = bp->b_resid;
			dm_reply(resp, DM_REPLY_BUF, u.u_error, bp, bp->b_bcount, 0);
		}
		else
		{ 
#ifdef	FSD_KI
			bp = getblk(devvp, rdu->rdu_strat.blkno, rdu->rdu_strat.bcount,
				B_rds_strat|B_unknown);
#else	FSD_KI
			bp = getblk(devvp, rdu->rdu_strat.blkno, rdu->rdu_strat.bcount);
#endif	FSD_KI
			/*  eventually we could optimize by trading blocks      */
			bcopy(rbp->b_un.b_addr, bp->b_un.b_addr, rdu->rdu_strat.bcount);
			/*  The request buffer is currently released automatically */

			if(rdu->rdu_strat.flags & B_DELWRI)
				bdwrite(bp);
			else
			{ 
				bp->b_flags |= (rdu->rdu_strat.flags & B_ASYNC);
				bwrite(bp);
			}

			DM_CONVERT(resp, rds_t)->rds_resid = bp->b_resid;
			dm_reply(resp, 0, u.u_error, NULL, NULL, NULL);
		}
		VN_RELE(devvp);
	}
	else {
		/*
		 *  Do I/O directly to the driver.  Set the flags which can
		 *  be carried over, and do the usual checking of the device.
		 *  On the S800 we turn off B_PHYS because we are no
		 *  longer doing a real physio to/from user space.  I am
		 *  leaving it in for the S300 since there is a safeguard
		 *  in bpcheck() that looks for this flag for swap device
		 *  requests.  B_PAGET and B_PGIN are probably not necessary
		 *  either but it won't hurt to leave them in.
		 */

#define B_STRATMASK	(B_WRITE|B_READ|B_PHYS)

		/*
                 * need to set B_NETBUF so the swap driver will
                 * allocate a separate swap buffer for this request
                 */

		if(rdu->rdu_strat.flags & B_READ) {
			rbp = geteblk(rdu->rdu_strat.bcount);
                        rbp->b_flags |= B_NETBUF;
		}

		/*
		 * since the 300 has a different page size from
		 * the 800, we need to adjust the page index number
		 * the swapmap.
		 *
		 * since we currently only support an 800 server in
		 * a heterogenous cluster this will work.  If we 
 		 * ever go to supporting a 300 server in a heterogenous
		 * cluster and NBPG on the 800 remains smaller than that
		 * on the 300 the current swapmap algorithms will not work
		 * for an odd numbered page.
		 */
		if ((adjust=rdu->rdu_strat.nbpg/NBPG) > 1) {
			*(int *)(&swdbd) = rdu->rdu_strat.blkno;
			swdbd.dbd_swpmp = swdbd.dbd_swpmp * adjust;
			rdu->rdu_strat.blkno = *(int *)(&swdbd);	
		}
		rbp->b_flags |= rdu->rdu_strat.flags & B_STRATMASK;
		rbp->b_blkno  = rdu->rdu_strat.blkno;
		rbp->b_offset = rdu->rdu_strat.offset;
		rbp->b_dev    = dev;
		rbp->b_s2 = site;

		if (opcode == DMNDR_STRAT){
			PXSEMA(&vmsys_sema, &ss);
        		(*swapdev_vp->v_op->vn_strategy)(rbp);
			VXSEMA(&vmsys_sema, &ss);
		} else {
			p_io_sema(&ss);
			(*bdevsw[major(dev)].d_strategy)(rbp);
			v_io_sema(&ss);
		}

		biowait(rbp);

		/*
                 * clear B_NETBUF since this buffer really is
                 * not a networking buffer
                 */

                if(rdu->rdu_strat.flags & B_READ)
                        rbp->b_flags &= ~B_NETBUF;

		DM_CONVERT(resp, rds_t)->rds_resid = rbp->b_resid;
		if(rdu->rdu_strat.flags & B_READ)
			dm_reply(resp, DM_REPLY_BUF, u.u_error, rbp, rbp->b_bcount, 0);
		else
			dm_reply(resp, 0, u.u_error, NULL, NULL, NULL);
	}
	/*note tha the messages are automatically released*/
}

/*ARGSUSED*/
rds_ioctl(dev, rdu)
register dev_t dev;
register rdu_t *rdu;
{ 
#ifdef FULLDUX
	register dm_message resp;
	register u_int error, size, cmd;
#ifdef	__hp9000s300
	char data[EFFECTIVE_IOCPARM_MASK+1];
#else
	char data[IOCPARM_MASK+1];
#endif	/* hp9000s300 */

	cmd  = rdu->rdu_ioc.com;

	size = (cmd & ~(IOC_INOUT|IOC_VOID)) >> 16;

	resp = dm_alloc(RDS_SIZE((cmd & IOC_OUT) ? size : 0), WAIT);

	if(cmd & IOC_IN)
		if(size)
			bcopy(rdu->rdu_data, data, size);
		else
			*((caddr_t *) data) = *((caddr_t *) rdu->rdu_data);
	else
		if(cmd & IOC_VOID)
			*((caddr_t *) data) = *((caddr_t *) rdu->rdu_data);

	if (setjmp(&u.u_qsave))
	{
		dm_reply(resp, DM_LONGJMP, 0, NULL, NULL, NULL);
		/* MP - reacquire semaphores */
		return;
	}
	error = (*cdevsw[major(dev)].d_ioctl)
	    (dev, cmd, data, rdu->rdu_ioc.flag);

	if(error == 0 && (cmd & IOC_OUT) && size)
		bcopy(data, (DM_CONVERT(resp, rds_t))->rds_data, size);

	dm_reply(resp, 0, error, NULL, NULL, NULL);
#else
	panic("rds_ioctl: remote devices not supported");
#endif
}

/*ARGSUSED*/
rds_select(dev, which)
register dev_t dev;
int which;
{ 
#ifdef FULLDUX
	dm_quick_reply(cdevsw[major(dev)].d_select(dev, which)); 
#else
	panic("rds_select: remote devices not supported");
#endif
}

/*ARGSUSED*/
rds_serve(reqp)
dm_message reqp;
{ 
	register rdu_t      *rdu;
	register dtaddr_t   dtp;
	register int        opcode;
	register dm_message resp;
	register site_t     site;
	dev_t dtpdev;	

	opcode = DM_OP_CODE(reqp);
	site   = DM_SOURCE(reqp);
	rdu    = DM_CONVERT(reqp, rdu_t);

    /*  If opcode == DMNDR_OPEND, then the destination site is passed as as
     *  open  parameter.  Otherwise,  it  should  be  decoded  from the dev
     *  number.  The following line of code determines  which  one  it  is,
     *  then  compares  it  to my_site.  If not equal, an error is returned
     *  immediately.
     */

	if(opcode == DMNDR_OPEND) {
		if (rdu->rdu_open.site != my_site)
			goto out;
	}
	else if (opcode != DMNDR_STRAT) {
		if (devsite(rdu->rdu_dev) != my_site)
			goto out;
	}

    /*  If  opening  a  device,  branch  to  the  routine  early  an  quit.
     *  Otherwise, make sure the device is already open here.
     */

	if(opcode == DMNDR_OPEND)
	{ 
		rds_opend(rdu->rdu_dev, rdu->rdu_open.mode, rdu->rdu_open.flag, site);
		return;
	}

	/* 
	 * do not check for the "swap device" since it is actually a vnode
	 * unless we are going through the buffer cache.  (not currently
	 * implemented)
	 */

	if (( opcode != DMNDR_STRAT) || (rdu->rdu_strat.flags & B_FSYSIO)) {
		dtp = deventry(rdu->rdu_dev);
		if(dtp == NULL || dtp->dt_flags == 0)
			goto out;
		dtpdev = dtp->dt_dev;
	}
	else
		dtpdev = NULL;

	switch(opcode)
	{ 
	case DMNDR_CLOSE  :
		rds_closed(dtp->dt_dev, dtp->dt_mode, rdu->rdu_flag, rdu->rdu_origsite);
		break;

	case DMNDR_READ   :
		rds_rw(DMNDR_READ, dtp->dt_dev, rdu, NULL);
		break;

	case DMNDR_WRITE  :
		rds_rw(DMNDR_WRITE, dtp->dt_dev, rdu, DM_BUF(reqp));
		break;

	case DMNDR_IOCTL  :
		rds_ioctl(dtp->dt_dev, rdu);
		break;

	case DMNDR_SELECT :
		rds_select(dtp->dt_dev, rdu->rdu_which);
		break;

	case DMNDR_STRAT  :
		rds_strat(dtpdev, rdu, DM_BUF(reqp), site, opcode);
		break;
	}

	return;     /* To avoid error handling routines */

out:

	switch(opcode)
	{ 
	case DMNDR_STRAT  :     
		resp = dm_alloc(RDS_SIZE(0), WAIT);
		DM_CONVERT(resp, rds_t)->rds_resid = rdu->rdu_strat.bcount;
		break;

	case DMNDR_WRITE  :
	case DMNDR_READ   :     
		resp = dm_alloc(RDS_SIZE(0), WAIT);
		DM_CONVERT(resp, rds_t)->rds_resid = rdu->rdu_uio.uio_resid;
		break;

	case DMNDR_IOCTL  :
	case DMNDR_OPEND  :
	case DMNDR_CLOSE  :     
		resp = NULL;
		break;

	case DMNDR_SELECT :     
		dm_quick_reply(0);      /* Return false */
		return;
	}

	dm_reply(resp, 0, ENXIO, NULL, NULL, NULL);
}
