/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/hpibio.c,v $
 * $Revision: 1.3.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:29:23 $
 */
/* HPUX_ID: @(#)hpibio.c	55.1		88/12/23 */

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

#include "../sio/dvio.h"
#undef	DCL
#undef	GET
#undef	TCT
#undef	PPC
#undef	LLO
#undef	SPE
#undef	UNL
#undef	UNT

#ifdef __hp9000s300
#include "../machine/pte.h"
#endif
#include "../h/param.h"
#include "../h/buf.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vm.h"
#include "../wsio/iobuf.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/vfs.h"
#include "../wsio/dil.h"
#include "../wsio/dilio.h"
#include "../h/vnode.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"

/* XXX move to wsio/dilio.h */
#define PREAMB_SENT     0x800

int dil_io_lock();
int dil_lock_timeout();
int dil_utility();
int dil_utility_timeout();
int dil_util();
int dil_util_timeout();

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hpib_io()
{
	register struct a {
		int	fdes;
		struct	iodetail *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iodetail aiov[MAXIOV];		/* XXX */
	int error;

	if (uap->iovcnt <= 0 || uap->iovcnt > MAXIOV) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = (struct iovec *)aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iodetail)));
	if (u.u_error)
		return;
	hpib_uio(&auio);
	error = copyout((caddr_t)aiov, (caddr_t)uap->iovp,
	    (unsigned)(uap->iovcnt * sizeof (struct iodetail)));
	if (error)
		u.u_error = error;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hpib_uio(uio)
register struct uio *uio;
{
	struct a {
		int	fdes;
	};
	register struct file *fp;
	register struct iodetail *iov;
	register struct vnode *vp;
	register struct inode *ip;
	int i, count;
	int reading = 0;
	int writing = 0;
	int type;
	dev_t dev;

	uio->uio_resid = 0;
	iov = (struct iodetail *)uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->count < 0) {
			u.u_error = EINVAL;
			return;
		}
		if ((iov->mode & HPIBATN) == 0)
			if (iov->mode & HPIBREAD)
				reading = 1;
			else
				writing = 1;
		uio->uio_resid += iov->count;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	count = uio->uio_resid;
	if (count < 1)
		return;

	GETF(fp, ((struct a *)u.u_ap)->fdes);
	if (reading && ((fp->f_flag & FREAD) == 0)) {
		u.u_error = EBADF;
		return;
	}
	if (writing && ((fp->f_flag & FWRITE) == 0)) {
		u.u_error = EBADF;
		return;
	}

	/* offset has no meaning for hpib */
	uio->uio_offset = fp->f_offset = 0;

	if ((u.u_procp->p_flag&SOUSIG) == 0 && setjmp(&u.u_qsave)) {
		if (uio->uio_resid == count)
			u.u_eosys = RESTARTSYS;
	} else {
		vp = (struct vnode *)fp->f_data;
		/*
		 * if write make sure filesystem is writable
		 */
		if (writing && (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
			u.u_error = EROFS;
			return;
		}
		ip = VTOI(vp);
		type = ip->i_mode & IFMT;
		dev = (dev_t) ip->i_rdev;

		/* must be a RAW HPIB DIL device file */
		if ((type != IFCHR) || 
		    (major(dev) != 21) || 
		    (m_busaddr(dev) != 0x1f)) {
			u.u_error = ENOTTY;
			return;
		}

		if (reading) imark(ip, IACC);
		if (writing) imark(ip, IUPD | ICHG);
		u.u_error = do_hpib_io(ip->i_rdev, uio);
	}
	u.u_r.r_val1 = 0;
	fp->f_offset = 0;
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hpibio_physio(bp, uio)
struct buf *bp;
struct uio *uio;
{
	register struct iodetail *iov;
	struct dil_info *info = (struct dil_info *)bp->dil_packet;
	register int iovcnt;
	caddr_t addr;
	int error;


	iov = (struct iodetail *)uio->uio_iov;
	iovcnt = uio->uio_iovcnt;

	/* allocate enough space for all the buffers */
	addr = (caddr_t)sys_memall(uio->uio_resid);
	if (addr == NULL)
		return ENOMEM;
 	info->hpibio_addr = (int)addr;
 	info->hpibio_cnt = uio->uio_resid;

	while (iovcnt--) {
		/* skip over space for read vectors */
		if ((iov->mode & HPIBREAD) == 0)
			if (error = copyin(iov->buf, addr, iov->count))
				return error;
		addr += iov->count;
		iov++;
	}
	return 0;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:  iod_send_cmd via dil_util (dil_action) via enqueue
**		iod_ctlr_status via dil_util (dil_action) via enqueue (b_action)
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/

do_hpib_io(dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct buf *bp;
	register struct iobuf *iob;
	register struct dil_info *info;
	struct iodetail *iovec;	/* vector of vectors */
	register int iolen;	/* count of vectors */
	register int length;
	register char mode;
	register char pattern;
	struct isc_table_type *sc;
	struct iodetail *this_iovec;
	int ccount;
	int error, x;
	int did_lock = 0;
	caddr_t addr, startaddr;
	int dilsigmask = (1 << (SIGDIL - 1));

	this_iovec = 0;

	bp = (struct buf *)u.u_fp->f_buf; 	/* get buffer */
	if (shared_buf_check(bp))
		return(bp->b_error);

	if ((u.u_procp->p_sigmask & dilsigmask) == 0)
		u.u_procp->p_sigmask |= dilsigmask;
	else
		dilsigmask = 0;

	acquire_buf(bp);

	info = (struct dil_info *) bp->dil_packet;
	info->dil_procp = u.u_procp;
	info->last_pid = u.u_procp->p_pid;
	info->full_tfr_count = uio->uio_resid;
	sc = (struct isc_table_type *)bp->b_sc;

	iob = bp->b_queue;      /* get dil iobuf*/
	/* mark the buffer */
	bp->b_flags &= ~(B_DONE | B_ERROR);
	bp->b_flags |= B_DIL;
	bp->b_error = 0;        /* clear errors */
	bp->b_spaddr = KERNELSPACE;

	if (error = hpibio_physio(bp, uio)) {
		release_buf(bp);
		u.u_procp->p_sigmask &= ~dilsigmask;
		return error;
	}

	
	/* lock the bus */
	x = spl6();
	if ((sc->state & LOCKED) && 
	    (sc->locking_pid == info->last_pid)) {
		splx(x);
		did_lock = 0;
	} else {
		splx(x);
		info->dil_timeout_proc = dil_lock_timeout;
		bp->b_action = dil_io_lock;
		enqueue(iob, bp);

		/* wait for this request to complete */
		if (error = hpibio_iowait(bp))
			goto err_exit;
		did_lock = 1;
	}

	iovec = (struct iodetail *)uio->uio_iov;
	iolen = uio->uio_iovcnt;
 	addr = (caddr_t)info->hpibio_addr;

	while(iolen--) {	/* transfer all the vectors */
		mode = iovec->mode;
		length = iovec->count;
		pattern = iovec->terminator;
		this_iovec = iovec;
		iovec++;	/* go to next vector */
		if (length < 1)
			continue;

		if (mode & HPIBREAD) {	/* read transfer */
			/* turn on eol control */
			if (mode & HPIBCHAR) {
				iob->dil_state |= READ_PATTERN;
				iob->read_pattern = pattern & 0xff;
			} else {
				iob->dil_state &= ~READ_PATTERN;
				iob->read_pattern = 0;
			}

			this_iovec->count = 0;
			startaddr = addr;
			iob->dil_state &= ~PREAMB_SENT;

			while (length > 0) {
				/* mark the buffer */
				bp->b_error = 0;
				bp->b_flags &= ~(B_DONE | B_ERROR);
				bp->b_flags |= B_DIL | B_READ;
				bp->b_un.b_addr = addr;

				/* get the count for this transfer */
				bp->b_bcount = length;
				minphys(bp, 0);
				ccount = bp->b_bcount;

				/* do the transfer */
				hpib_strategy(bp, uio);
				if (error = hpibio_iowait(bp))
					goto err_exit;

				/* adjust our stuff */
				ccount -= bp->b_resid;
				addr += ccount;
				this_iovec->count += ccount;
				length -= ccount;

				/* did we have an error? */
				if (bp->b_flags&B_ERROR)
					goto err_exit;

				/* are we done? */
				if (bp->b_resid || bp->b_flags & B_END_OF_DATA) {
				        addr += bp->b_resid;
					break;
				}
			}

			if (error = copyout(startaddr, this_iovec->buf, this_iovec->count))
				break;

			/* turn off eol control */
			if (mode & HPIBCHAR) {
				iob->dil_state &= ~READ_PATTERN;
				iob->read_pattern = 0;
			}
		}
		else {	/* write transfer */
			if (mode & HPIBATN) {
				/*
				** get our current bus controller status
				*/
				if (sc->card_type == HP98625) {
					/* simon so lets do it the hard way */
					info->dil_action = 
						sc->iosw->iod_ctlr_status;
					info->dil_timeout_proc = 
						dil_utility_timeout;
					bp->b_action = dil_utility;
					enqueue(iob, bp);

					if (error = hpibio_iowait(bp))
						break;
				} else 
					(*sc->iosw->iod_ctlr_status)(bp);
				if ((sc->state & ACTIVE_CONTROLLER) == 0) {
					error = EIO;
					bp->b_error = EIO;
					bp->b_flags |= B_ERROR;
					break;
				}
				bp->pass_data1 = length;      /* length */
				bp->pass_data2 = (int)addr;
				info->dil_action = sc->iosw->iod_send_cmd;
				info->dil_timeout_proc = dil_util_timeout;
				bp->b_action = dil_util;
				enqueue(iob, bp);
				if (error = hpibio_iowait(bp))
					break;
				addr += length;
			} else {
				if (mode & HPIBEOI)
					iob->dil_state |= EOI_CONTROL;
				else
					iob->dil_state &= ~EOI_CONTROL;

				iob->dil_state &= ~PREAMB_SENT;

				this_iovec->count = 0;

				while (length > 0) {
					/* mark the buffer */
					bp->b_error = 0;
					bp->b_flags &= ~(B_READ | B_DONE | B_ERROR);
					bp->b_flags |= B_DIL;
					bp->b_un.b_addr = addr;

					/* get the count for this transfer */
					bp->b_bcount = length;
					minphys(bp, 0);
					ccount = bp->b_bcount;

					/* do the transfer */
					hpib_strategy(bp, uio);
					if (error = hpibio_iowait(bp))
						goto err_exit;

					/* adjust our stuff */
					ccount -= bp->b_resid;
					addr += ccount;
					this_iovec->count += ccount;
					length -= ccount;

					/* are we done? */
					if (bp->b_resid || (bp->b_flags&B_ERROR))
						goto err_exit;
				}

				if (mode & HPIBEOI)
					iob->dil_state &= ~EOI_CONTROL;
			}
		}
	}

err_exit:

 	sys_memfree((caddr_t)info->hpibio_addr, info->hpibio_cnt);

	if ((error) && (this_iovec)) {
		this_iovec->count = -1;
	}


	if (did_lock) {
		x = spl6();
		if (sc->owner == bp) {
			iob->dil_state &= ~D_CHAN_LOCKED;
			info->locking_pid = 0;	
			sc->state &= ~LOCKED;
			sc->locking_pid = 0;
			drop_selcode(sc->owner);
			wakeup((caddr_t) &bp->b_queue);
			dil_dequeue(bp);
		}
		splx(x);
	}
	bp->b_flags |= B_DONE;

	u.u_procp->p_sigmask &= ~dilsigmask;
	release_buf(bp);
	return error;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hpibio_iowait(bp)
register struct buf *bp;
{
	register struct dil_info *dil_info = (struct dil_info *) bp->dil_packet;
	int x;

	/* wait for this request to complete */
	x = spl6();
	if (setjmp(&u.u_qsave)) {
		/*
		** Request has recieved a signal cleanup and return.
		*/
		if (!(bp->b_flags & B_DONE))

			(*dil_info->dil_timeout_proc)(bp);
		u.u_eosys = EOSYS_NORESTART;
		bp->b_error = EINTR;
	} else
		while ((bp->b_flags & B_DONE) == 0)
			sleep((caddr_t) &bp->b_flags, DILPRI);
	splx(x);

	if (bp->b_flags & B_ERROR)
		return(bp->b_error);

	bp->b_flags &= ~B_DONE;
	return 0;
}
