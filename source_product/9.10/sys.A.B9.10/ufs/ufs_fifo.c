/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_fifo.c,v $
 * $Revision: 1.20.83.6 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:21:42 $
 */

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



#ifdef MP
#include "../machine/reg.h"
#include "../machine/inline.h"
#endif /* MP */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/mount.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../ufs/fs.h"
#include "../h/kernel.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA
#include "../h/kern_sem.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../dux/dux_dev.h"

extern struct fileops vnodefops;

struct vnode *pipevp;	/*where to create pipes*/

/*
 * The sys-pipe entry.
 * Allocate an inode on the root device.
 * Allocate 2 file structures.
 * Put it all together with flags.
 */
pipe()
{
	register struct inode *ip;
	register struct file *rf, *wf;
	int r;
	extern struct inode *ss_pipe();
	extern struct inode *pipe_send();

	/*for DUX, the fds are allocated before the inode, since they are
	 *easier to undo*/
	PSEMA(&filesys_sema);

	rf = falloc();
	if(rf == NULL) {
		VSEMA(&filesys_sema);
		return;
	}
	r = u.u_rval1;
	wf = falloc();
	if(wf == NULL) {
		rf->f_count = 0;
		uffree(r);
		crfree(u.u_cred);
		VSEMA(&filesys_sema);
		return;
	}
	u.u_rval2 = u.u_rval1;
	u.u_rval1 = r;
	if (pipevp->v_fstype == VDUX)
	{
		ip = pipe_send(pipevp);
	}
	else
	{
		ip = ss_pipe(pipevp);
	}
	if (ip == NULL)
	{
		rf->f_count = wf->f_count = 0;
		uffree(r);
		uffree(u.u_rval2);
		crfree(u.u_cred); /* for rf */
		crfree(u.u_cred); /* for wf */
		VSEMA(&filesys_sema);
		return;
	}
	wf->f_flag = FWRITE;
	wf->f_type = DTYPE_VNODE;
	wf->f_ops = &vnodefops;
	wf->f_data = (caddr_t)ITOV(ip);
	rf->f_flag = FREAD;
	rf->f_type = DTYPE_VNODE;
	rf->f_ops = &vnodefops;
	rf->f_data = (caddr_t)ITOV(ip);
	ip->i_flag |= IREF;
	if (ilocked(ip))
		iunlock(ip);
	VSEMA(&filesys_sema);
}

/*
 *allocate an inode using pipe device pip
 */
struct inode *
ss_pipe(pvp)
struct vnode *pvp;
{
	register struct inode *ip;
	struct vnode *vp;
	struct mount *mp;

	use_dev_vp(pvp);
	if (pvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error= EROFS;
		drop_dev_vp(pvp);
		return(NULL);
	}
	ip = ialloc(VTOI(pvp), 0, IFIFO);
	if(ip == NULL)
	{
		drop_dev_vp(pvp);
		return (NULL);
	}
	updatesitecount(&(ip->i_opensites),u.u_site,2);	/*2 file descs*/
	updatesitecount(&(ip->i_writesites),u.u_site,1);
	updatesitecount(&(ip->i_fifordsites),u.u_site,1);
	ip->i_flag |= IREF;
	mp = (VTOI(pvp))->i_mount;
	vp = (ITOV(ip));
	VN_INIT (vp, mp->m_vfsp, IFTOVT(ip->i_mode), ip->i_rdev);
	VN_HOLD(vp);
	ip->i_uid = u.u_uid;
	ip->i_gid = u.u_gid;
	ip->i_mode = IFIFO;
	ip->i_frcnt = 1;
	ip->i_fwcnt = 1;
#ifdef QUOTA
	ip->i_dquot = getinoquota(ip);
#endif
	imark(ip, IACC|IUPD|ICHG);
	drop_dev_vp(pvp);
	return (ip);
}


/*
 * Open a pipe
 * Check read and write counts, delay as necessary
 */

openp(ip, mode)
register struct inode *ip;
register mode;
{
	if (mode&FREAD) {
		if (ip->i_frcnt++ == 0) {
			if ((ip->i_fifosize < PIPSIZ) 
			   && (ip->i_fwcnt != 0)) fselwakeup(&ip->i_fselw);
			if (ip->i_fifosize > 0) fselwakeup(&ip->i_fselr);
			wakeup((caddr_t)&ip->i_frcnt);
		}
	}
	if (mode&FWRITE) {
#ifdef	POSIX
		if (mode&(FNDELAY|FNBLOCK) && ip->i_frcnt == 0) {
#else	not POSIX
		if (mode&FNDELAY && ip->i_frcnt == 0) {
#endif	POSIX
			return(ENXIO);
		}
		if (ip->i_fwcnt++ == 0) {
			if ((ip->i_fifosize < PIPSIZ)
			   && (ip->i_frcnt != 0)) fselwakeup(&ip->i_fselw);
			wakeup((caddr_t)&ip->i_fwcnt);
		}
	}
	openp_wait(ip,mode);
	return(0);
}

/*
 *wait for readers or writers on the pipe as appropriate.  If sync I/O
 *is required, this should be done at the server, otherwise at the US.
 */
openp_wait(ip,mode)
register struct inode *ip;
register int mode;
{
restart:	/*we need a restart because sync I/O might suddenly be needed*/
	if (ip->i_flag&ISYNC && remoteip(ip)) 
		openp_wait_send(ip,mode);
	else if (!u.u_nsp || ip->i_flag & ISYNC) /* sleep at SS only if ISYNC */
	{
		if (mode&FREAD) {
			if (ip->i_fwcnt == 0) {
#ifdef	POSIX
				if (mode&(FNDELAY|FNBLOCK) || ip->i_fifosize)
#else	not POSIX
				if (mode&FNDELAY || ip->i_fifosize)
#endif	POSIX
					return;
				sleep(&ip->i_fwcnt, PPIPE);
				goto restart;
			}
		}
		if (mode&FWRITE) {
			if (ip->i_frcnt == 0)
			{
				sleep(&ip->i_frcnt, PPIPE);
				goto restart;
			}
		}
	}
}

/*
 * Close a pipe
 * Update counts and cleanup
 */

closep(ip, mode)
register struct inode *ip;
register mode;
{
	register i;
	daddr_t bn;
	register int size;
	register struct fs *fs;
	struct inode tip;
	register struct inode *nip;

	if (mode&FREAD) {
		if ((--ip->i_frcnt == 0) && (ip->i_fflag&IFIW)) {
			ip->i_fflag &= ~IFIW;
			wakeup((caddr_t)&ip->i_fwcnt);
		}
	}
	if (mode&FWRITE) {
		if ((--ip->i_fwcnt == 0) && (ip->i_fflag&IFIR)) {
			ip->i_fflag &= ~IFIR;
			wakeup((caddr_t)&ip->i_frcnt);
		}
	}

	/*
	 * This is a bug fix to make our pipe code 
	 * compatible with Berkeley (with respect to
	 * how select works).  The idea is that if the
	 * pipe is closed those processes waiting for 
	 * reading or writing should be awakened.
	 */
	fselwakeup(&ip->i_fselw);
	fselwakeup(&ip->i_fselr);
	if ((ip->i_frcnt == 0) && (ip->i_fwcnt == 0)) {
		if (remoteip(ip))
		{
			dux_fifo_invalidate(ip);
		}
		else
		{
		fs = ip->i_fs;
		tip = *ip;
		for (i = NDADDR - 1; i >= 0; i--) 
			ip->i_db[i] = 0;
		ip->i_size = 0;
		ip->i_blocks = 0;
		ip->i_frptr = 0;
		ip->i_fwptr = 0;
		ip->i_fflag = 0;
		/*
		 * POSIX compatibility: do not mark inode as changed,
		 * unless we are actually discarding data.
		 */
		if (ip->i_fifosize != 0) {
			imark(ip, IUPD|ICHG);
			ip->i_fifosize = 0;
		}
		iupdat(ip, 0, 0);
		nip = &tip;
		for(i = NDADDR - 1; i >= 0; i--) {
			bn = nip->i_db[i];
			if (bn == 0)
				continue;
			nip->i_db[i] = 0;
			size = (off_t) blksize(fs, nip, i);
			if (size == fs->fs_bsize)
			    bpurge(nip->i_devvp, (daddr_t) fsbtodb(fs, bn));
			free(nip, bn, size);
#ifdef QUOTA
                        (void) chkdq(nip, -btodb(size), 0);
#endif QUOTA
		}
	}
	}
}

fselqueue(fsel)
	struct i_select *fsel;
{
	register struct proc *p;

	if ((p = fsel->i_selp) && (p->p_wchan == (caddr_t) &selwait))
		fsel->i_selflag |= FSEL_COLL;
	else
		fsel->i_selp = u.u_procp;
}

fselwakeup(fsel)
	struct i_select *fsel;
{
	if (fsel->i_selp) {
		selwakeup(fsel->i_selp, fsel->i_selflag & FSEL_COLL);
		fsel->i_selp = 0;
		fsel->i_selflag &= ~FSEL_COLL;
	}
}

#ifndef MP
#define SETCURPRI(pri, dummy) curpri=pri
#endif MP

fifo_read(ip, uio)
	register struct inode *ip;
	register struct uio *uio;
{

    int x, err=0;
    int cantwait  = uio->uio_fpflags&(FNDELAY|FNBLOCK);
    int posixflavor = cantwait&FNBLOCK;
    struct vnode *devvp = ip->i_devvp;
    register u_int reg_temp;

    /* resid > 0 at entry */

#ifdef POSIX
    /* yuch. no-writer case win's over would-block case.  */
    if (cantwait && ip->i_fifosize == 0 && ip->i_fwcnt > 0)
	return posixflavor ? EAGAIN : 0;
#else
    /* yuch. no-writer case win's over would-block case.  */
    if (cantwait && ip->i_fifosize == 0 && ip->i_fwcnt > 0)
	return 0;
#endif /* POSIX */


    /* if the file buffer is empty,  wait for something
     * to read.
     */
    
    while (ip->i_fifosize == 0) {
	/* poor form to sleep if writer has vanished */
	if (ip->i_fwcnt == 0)
	    break;
	ip->i_fflag |= IFIR;
	iunlock(ip);
	drop_dev_dvp(devvp);
	sleep((caddr_t)&ip->i_frcnt, PPIPE);
	use_dev_dvp(devvp);
	ilock(ip);
    }

    /* at this point, we have bytes in the buffer, 
     * or else got an error. */
    if (ip->i_fifosize == 0)
	return 0;


    uio->uio_offset = ip->i_frptr;
    if (err = cirq_read (ip, uio, ip->i_fifosize, PIPSIZ, &x))
	return err;
    
    ip->i_fifosize -= x;
    ip->i_frptr = uio->uio_offset;


    /* we have gorged on bytes. give the writer a chance to run again. */
    fselwakeup ((caddr_t) &ip->i_fselw);
    if (ip->i_fflag&IFIW) {
	ip->i_fflag &= ~IFIW;
	SETCURPRI(PPIPE, reg_temp); /* awaken, but don't schedule crock */
	wakeup ((caddr_t)&ip->i_fwcnt);
    }

    return err;
}

fifo_write(ip, uio)
	register struct inode *ip;
	register struct uio *uio;
{

    int x, err;
    int cantwait  = uio->uio_fpflags&(FNDELAY|FNBLOCK);
    int posixflavor = cantwait&FNBLOCK;
    int deficit  =  (ip->i_fifosize + uio->uio_resid) - PIPSIZ;
    int need;	/* bytes of space needed to make this write atomically */
    struct vnode *devvp = ip->i_devvp;
    register u_int reg_temp;

    if (cantwait && deficit > 0) {
	/* uh-oh----no room to complete write atomically w/o blocking,
	 * as requested.
	 * HPUX specifies similar behavior for both flavors of noblock:
	 * if request size > PIPSIZ, and there is room in the pipe buffer,
	 * write what we can, otherwise write nothing.
	 */
#ifdef POSIX
	if ((uio->uio_resid <= PIPSIZ || ip->i_fifosize == PIPSIZ))
	    if (posixflavor)
		return EAGAIN;
	    else
		return 0;
#else
	if ((uio->uio_resid <= PIPSIZ || ip->i_fifosize == PIPSIZ))
	    return 0;
#endif
    }

    err=0;
    while (uio->uio_resid && ip->i_frcnt) {

	need = ip->i_fifosize + uio->uio_resid;

	if (need <= PIPSIZ || uio->uio_resid > PIPSIZ) {
	    uio->uio_offset = ip->i_fwptr;
	    if (err = cirq_write (ip, uio, ip->i_fifosize, PIPSIZ, &x))
		break;
	    /* don't forget to adjust the file buffer pointers */
	    ip->i_fifosize += x;
	    ip->i_fwptr = uio->uio_offset;
	}

	/* best wake up the reader if we are going to sleep */
	if (ip->i_fifosize) {
	    fselwakeup ((caddr_t)&ip->i_fselr);
	    if (ip->i_fflag&IFIR) {
		ip->i_fflag &= ~IFIR;
		SETCURPRI(PPIPE, reg_temp);
		wakeup ((caddr_t)&ip->i_frcnt);
	    }
	}
	if (uio->uio_resid) {
	    if(cantwait)
		break;
	    ip->i_fflag |= IFIW;
	    iunlock(ip);
	    drop_dev_dvp(devvp);
	    sleep((caddr_t)&ip->i_fwcnt, PPIPE);
	    use_dev_dvp(devvp);
	    ilock(ip);
	}
    }

    if (ip->i_frcnt == 0) {
	if (!u.u_nsp)		/* if we are not servicing a remote request */
	psignal (u.u_procp, SIGPIPE);
	err = EPIPE;		/* wins over EFAULT */
    }
    return err;
}
#undef SETCURPRI


/* circular queue read and write
 *
 * ip          -- pointer to a locked inode of a regular file for buffer
 * cirq_size   -- total number of data bytes currently in this cirque
 * cirq_lim    -- how large this cirq can grow before wrapping around
 * retcnt      -- number of bytes read or written
 *
 * RETURN VALUE: 0 on success, else a system error code.
 * SIDE EFFECTS: update inode change/modified/access times.
 *               cirq_write'ing may cause additional db's to be allocated if growing.
 *
 * REMARKS:      ufs fifo read and write are implemented on top of this.
 */
int
cirq_read (ip, uio, cirq_size, cirq_lim, retcnt)
	register struct inode *ip;
	register struct uio *uio;
	int cirq_size;
	int cirq_lim;
	int *retcnt;	/* return # bytes transferred */
{
    int amount_to_go = uio->uio_resid;
    int size_r1;	/* size region 1 */
    int size_r2; 	/* and region 2 */
    int rdoff, wroff;	/* reader and writer offsets from start of file */
    register int a;
    int err, cnt=0, x;

    if (cirq_size > cirq_lim || uio->uio_offset > cirq_lim)
	panic("cirq_read");

    rdoff = uio->uio_offset;
    if (rdoff == cirq_lim)
	rdoff = 0;

    wroff = rdoff + cirq_size;
    if (wroff >= cirq_lim)
	wroff -= cirq_lim;

    if (wroff > rdoff) {
	size_r1 = wroff - rdoff;
	size_r2 = 0;
    } else {
	size_r1 = cirq_lim - rdoff;
	size_r2 = wroff;
    }

    if (amount_to_go > cirq_size)
	amount_to_go = cirq_size;

    /* copy into region 1 --- start offset to physical end of cirq */
    if ((a = MIN(amount_to_go, size_r1)) >0) {
	uio->uio_offset = rdoff;
	if (err = ifreg_read (ip, uio, a, &x))
	    return err;
	cnt += x;
	amount_to_go -= x;
    }

    /* copy into region 2 --- offset 0 to logical beginning of cirq */
    if ((a = MIN(amount_to_go, size_r2)) >0) {
	uio->uio_offset = 0;
	if (err = ifreg_read (ip, uio, a, &x))
	    return err;
	cnt += x;
    }
    if (cnt)
	imark(ip, IACC);

    *retcnt = cnt;
    return 0;
}


int
cirq_write (ip, uio, cirq_size, cirq_lim, retcnt)
	register struct inode *ip;
	register struct uio *uio;
	int cirq_size;
	int cirq_lim;
	int *retcnt;	/* return # bytes transferred */
{

    int cirq_free = cirq_lim - cirq_size;
    int amount_to_go = uio->uio_resid;
    int size_r1;	/* size region 1 */
    int size_r2; 	/* and region 2 */
    int rdoff, wroff;	/* reader and writer offsets from start of file */
    register int a;
    int err, cnt=0, x;

    if (cirq_size > cirq_lim || uio->uio_offset > cirq_lim)
	panic("cirq_write");

    wroff = uio->uio_offset;
    if (wroff == cirq_lim)
	wroff = 0;

    rdoff = wroff - cirq_size;
    if (rdoff < 0)
	rdoff += cirq_lim;

    if (rdoff > wroff) {
	size_r1 = rdoff - wroff;
	size_r2 = 0;
    } else {
	size_r1 = cirq_lim - wroff;
	size_r2 = rdoff;
    }
	
    if (amount_to_go > cirq_free)
	amount_to_go = cirq_free;

    /* copy into region 1 --- start offset to physical end of cirq */
    if ((a = MIN(amount_to_go, size_r1)) >0) {
	uio->uio_offset = wroff;
	if (err = ifreg_write (ip, uio, a, &x))
	    return err;
	cnt += x;
	amount_to_go -= x;
    }

    /* copy into region 2 --- offset 0 to logical beginning of cirq */
    if ((a = MIN(amount_to_go, size_r2)) >0) {
	uio->uio_offset = 0;
	if (err = ifreg_write (ip, uio, a, &x))
	    return err;
	cnt += x;
    }
    if (cnt)
	imark(ip, IUPD|ICHG);

    *retcnt = cnt;
    return 0;
}

#ifdef FSD_KI
#define FSD_KI_AUXARGS , B_fifo_read|B_data
#else
#define FSD_KI_AUXARGS
#endif

/*
 * pipe-specific file-read.  could be made a little more general
 * by allowing frags and reads of blocks which have not been written yet.
 */

int
ifreg_read (ip, uio, cnt, retcnt)
	register struct inode *ip;
	register struct uio *uio;
	int cnt, *retcnt;
{
    register struct fs *fs;
    register struct buf *bp; 
    int lblk;			/* logical blkno */
    int lblk_ofst;		/* offset within "" */
    int readsize;
    int nblk;			/* # lblks to iterate over */
    int bytes_to_go;
    int err;
    int bsize;			/* synonym for fs->fs_bsize */
    daddr_t blkno;		/* device block # */

    fs = ip->i_fs;
    lblk_ofst = blkoff(fs, uio->uio_offset);
    nblk = lblkno(fs, blkroundup(fs, cnt +lblk_ofst));
    bsize  = fs->fs_bsize;

    bytes_to_go = cnt;
    err = 0;

    while(nblk--) {

	lblk = lblkno(fs, uio->uio_offset);

	blkno = fsbtodb(fs, bmap(ip, lblk, B_READ, bsize, 0, 0, 0));

	/* could assert that blkno is nonzero at this point. */
	if (blkno == 0)
	    panic("ifreg_read");

	bp = (struct buf *) 
	    bread(ip->i_devvp, blkno, bsize FSD_KI_AUXARGS);

	if (bp->b_flags&B_ERROR)
	    err = geterror(bp);
	else {
	    readsize = bsize - lblk_ofst;
	    if (readsize > bytes_to_go)
		readsize = bytes_to_go;
	    err = uiomove(bp->b_un.b_addr+lblk_ofst, readsize,
			  UIO_READ, uio);
	    /* but clear B_DELWRI if we emptied it  */
	}
	brelse (bp);

	if (err)
	    break;

	lblk_ofst = 0;
	bytes_to_go -= readsize;
    }

    *retcnt = cnt - bytes_to_go;
    return err;
}
#undef FSD_KI_AUXARGS

#ifdef FSD_KI
#define FSD_KI_AUXARGS , B_fifo_write|B_data
#else
#define FSD_KI_AUXARGS
#endif

/*
 * pipe-specific file-write.  could be made a little more general 
 * by allowing frags.
 */
int
ifreg_write (ip, uio, cnt, retcnt)
	register struct inode *ip;
	register struct uio *uio;
	int cnt, *retcnt;
{
    register struct fs *fs;
    register struct buf *bp; 
    int lblk;			/* logical blkno */
    int lblk_ofst;		/* offset within "" */
    int writesize;
    int nblk;			/* # lblks to iterate over */
    int bytes_to_go;
    int err;
    int bsize;			/* synonym for fs->fs_bsize */
    daddr_t blkno;		/* device block # */
    daddr_t blkend;		/* end of allocated block */

    fs = ip->i_fs;
    lblk_ofst = blkoff(fs, uio->uio_offset);
    nblk = lblkno(fs, blkroundup(fs, cnt +lblk_ofst));
    bsize  = fs->fs_bsize;

    bytes_to_go = cnt;
    err = 0;

    while(nblk--) {

	lblk = lblkno(fs, uio->uio_offset);

	blkno = fsbtodb(fs, bmap(ip, lblk, B_WRITE, bsize, 0, 0, 0));
	if (u.u_error)    /* includes ENOSPC and EDQUOT */
	    return(u.u_error);

	if (blkno <= 0)
	    return ENOSPC;

	/* might have grown the file */
	blkend = (lblk+1) << fs->fs_bshift;
	if (blkend > ip->i_size)
	    ip->i_size = blkend;

	/* could just getblk if nothing valid in buf */
	bp = (struct buf *) 
	    bread(ip->i_devvp, blkno, bsize FSD_KI_AUXARGS);

	if (bp->b_flags&B_ERROR)
	    err = geterror(bp);
	else {
	    writesize = bsize - lblk_ofst;
	    if (writesize > bytes_to_go)
		writesize = bytes_to_go;
	    err = uiomove(bp->b_un.b_addr+lblk_ofst, writesize,
			  UIO_WRITE, uio);
	}
	if (err) {
	    brelse (bp);
	    break;
	} else
	    bdwrite(bp);

	lblk_ofst = 0;
	bytes_to_go -= writesize;
    }

    *retcnt = cnt - bytes_to_go;
    return err;
}

