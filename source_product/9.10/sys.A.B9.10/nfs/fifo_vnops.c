/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/fifo_vnops.c,v $
 * $Revision: 1.8.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:07:04 $
 */

/* SystemV-compatible FIFO implementation */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/time.h"
#ifdef hpux
#include "../h/resource.h"
#endif hpux
#include "../h/proc.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/file.h"
#include "../h/errno.h"
#include "../h/signal.h"
#include "../ufs/inode.h"
#include "../nfs/fifo.h"
#include "../nfs/snode.h"
#include "../nfs/fifonode.h"
#ifdef POSIX
#include "../h/unistd.h"
#endif POSIX

#ifndef hpux
#include "../krpc/lockmgr.h"
#endif hpux

#ifdef NS_QA
#define SANITY		/* do sanity checks */
#endif NS_QA

#include "../h/kern_sem.h"
#ifdef MP
#include "../machine/cpu.h"
#include "../machine/reg.h"
#include "../machine/inline.h"
#endif

static struct fifo_bufhdr *fifo_bufalloc();
static struct fifo_bufhdr *fifo_buffree();

int fifo_open();
int fifo_close();
int fifo_rdwr();
int fifo_select();
int fifo_getattr();
int fifo_inactive();
int fifo_invalop();
int fifo_notdir();		/* return ENOTDIR instead of EINVAL */
#ifdef notdef
int fifo_badop();
#endif notdef
#ifdef POSIX
int fifo_pathconf();
extern int spec_pathconf();
#endif POSIX

extern int spec_setattr();
extern int spec_access();
extern int spec_link();
extern int spec_fsync();
extern int spec_lockctl();
extern int spec_lockf();
extern int spec_fid();
#ifdef ACLS
extern int spec_setacl();
extern int spec_getacl();
#endif ACLS

struct vnodeops fifo_vnodeops = {
	fifo_open,
	fifo_close,
	fifo_rdwr,
	fifo_invalop,	/* ioctl() */
	fifo_select,
	fifo_getattr,
	spec_setattr,
	spec_access,
	fifo_notdir,	/* lookup() */
	fifo_notdir,	/* create() */
	fifo_notdir,	/* remove() */
	spec_link,
	fifo_notdir,	/* rename() */
	fifo_notdir,	/* mkdir() */
	fifo_notdir,	/* rmdir() */
	fifo_notdir,	/* readdir() */
	fifo_notdir,	/* symlink() */
	fifo_invalop,	/* readlink() */
	spec_fsync,
	fifo_inactive,
	fifo_invalop,	/* bmap() */
	fifo_invalop,	/* strategy() */
	fifo_invalop,	/* bread() */
	fifo_invalop,	/* brelse() */
	fifo_invalop,	/* pathsend() */
#ifdef ACLS
	spec_setacl,
	spec_getacl,
#endif ACLS
#ifdef POSIX
	fifo_pathconf,
	fifo_pathconf,
#endif POSIX
	spec_lockctl,
	spec_lockf,
	spec_fid,
	fifo_invalop,	/* fscntl() */
        fifo_invalop,   /* prefill() */
        fifo_invalop,   /* pagein() */
        fifo_invalop,   /* pageout() */
        NULL,           /* dbddealloc() */
        NULL,           /* dbddup() */
};


/*
 * open a fifo -- sleep until there are at least one reader & one writer
 */
/*ARGSUSED*/
int
fifo_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	register struct fifonode *fp;

	/*
	 * Setjmp in case open is interrupted.
	 * If it is, close and return error.
	 */
	if (setjmp(&u.u_qsave)) {
		PSEMA(&filesys_sema);
		(void) fifo_close(*vpp, flag & FMASK, cred);
		return (EINTR);
	}
	fp = VTOF(*vpp);

	if (flag & FREAD) {
		if (fp->fn_rcnt++ == 0)
			/* if any writers waiting, wake them up */
			wakeup((caddr_t) &fp->fn_rcnt);
	}

	if (flag & FWRITE) {
#ifdef POSIX
		if ((flag & (FNDELAY|FNBLOCK)) && (fp->fn_rcnt == 0))
#else !POSIX
		if ((flag & FNDELAY) && (fp->fn_rcnt == 0))
#endif POSIX
			return (ENXIO);
		if (fp->fn_wcnt++ == 0)
			/* if any readers waiting, wake them up */
			wakeup((caddr_t) &fp->fn_wcnt);
	}

	if (flag & FREAD) {
		while (fp->fn_wcnt == 0) {
			/* if no delay, or data in fifo, open is complete */
#ifdef POSIX
			if ((flag & (FNDELAY|FNBLOCK)) || fp->fn_size)
#else POSIX
			if ((flag & FNDELAY) || fp->fn_size)
#endif POSIX
				return (0);
			(void) sleep((caddr_t) &fp->fn_wcnt, PPIPE);
		}
	}

	if (flag & FWRITE) {
		while (fp->fn_rcnt == 0)
			(void) sleep((caddr_t) &fp->fn_rcnt, PPIPE);
	}
	return (0);
}

/*
 * close a fifo
 * On final close, all buffered data goes away
 */
/*ARGSUSED*/
int
fifo_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct fifonode *fp;
	register struct fifo_bufhdr *bp;
#ifdef MP
        register u_int reg_temp;
#endif

	fp = VTOF(vp);

	if (flag & FREAD) {
		if (--fp->fn_rcnt == 0) {
			if (fp->fn_flag & FIFO_WBLK) {
				fp->fn_flag &= ~FIFO_WBLK;
				wakeup((caddr_t) &fp->fn_wcnt);
			}
#ifdef notdef
			/* wake up any sleeping exception select()s */
			if (fp->fn_xsel) {
#ifdef MP
                                SETCURPRI(PPIPE, reg_temp);
#else
				curpri = PPIPE;
#endif
				selwakeup(fp->fn_xsel, fp->fn_flag&FIFO_XCOLL);
				fp->fn_flag &= ~FIFO_XCOLL;
				fp->fn_xsel = (struct proc *)0;
			}
#endif notdef
		}
	}

	if (flag & FWRITE) {
		if ((--fp->fn_wcnt == 0) && (fp->fn_flag & FIFO_RBLK)) {
			fp->fn_flag &= ~FIFO_RBLK;
			wakeup((caddr_t) &fp->fn_rcnt);
		}
	}

	if ((fp->fn_rcnt == 0) && (fp->fn_wcnt == 0)) {
		/* free all buffers associated with this fifo */
		for (bp = fp->fn_buf; bp != NULL; ) {
			bp = fifo_buffree(bp, fp);
		}

		/* update times only if there were bytes flushed from fifo */
		if (fp->fn_size != 0)
			FIFOMARK(fp, SUPD|SCHG);

		fp->fn_buf = (struct fifo_bufhdr *) NULL;
		fp->fn_rptr = 0;
		fp->fn_wptr = 0;
		fp->fn_size = 0;
	}
	return (0);
}


/*
 * read/write a fifo
 */
/*ARGSUSED*/
int
fifo_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct fifonode *fp;
	register struct fifo_bufhdr *bp;
	register u_int count;
	register int off;
	register unsigned i;
	register int rval = 0;
	int ocnt = uiop->uio_resid;	/* save original request size */
        register u_int reg_temp;

#ifdef SANITY
	if ((ioflag & IO_APPEND) == 0)
		printf("fifo_rdwr: no append flag\n");
	if (uiop->uio_offset != 0)
		printf("fifo_rdwr: non-zero offset: %d\n", uiop->uio_offset);
#endif SANITY

	fp = VTOF(vp);
	FIFOLOCK(fp);

	if (rw == UIO_WRITE) {				/* UIO_WRITE */
		/*
		 * PIPE_BUF: max number of bytes buffered per open pipe
		 * PIPE_MAX: max size of single write to a pipe
		 *
		 * If the count is less than PIPE_BUF, it must occur
		 * atomically.  If it does not currently fit in the
		 * kernel pipe buffer, either sleep or return EWOULDBLOCK,
		 * depending on FNDELAY  (library routine translates).
		 *
		 * If the count is greater than PIPE_BUF, it will be
		 * non-atomic (FNDELAY clear).  If FNDELAY is set,
		 * write as much as will fit into the kernel pipe buffer
		 * and return the number of bytes written.
		 *
		 * If the count is greater than PIPE_MAX, return EINVAL.
		 */
		if ((unsigned)uiop->uio_resid > PIPE_MAX) {
			rval = EINVAL;
			goto rdwrdone;
		}
	
		while (count = uiop->uio_resid) {
			if (fp->fn_rcnt == 0) {
				/* no readers anymore! */
				psignal(u.u_procp, SIGPIPE);
				rval = EPIPE;
				goto rdwrdone;
			}
			if ((count + fp->fn_size) > PIPE_BUF) {
#ifdef hpux 
				/* 
				 * HP-UX has file flags passed in, don't
				 * need special flag for NDELAY.
				 */
#ifdef POSIX
				if (uiop->uio_fpflags & (FNDELAY|FNBLOCK)) {
#else POSIX
				if (uiop->uio_fpflags & FNDELAY) {
#endif POSIX
#else !hpux
				if (ioflag & IO_NDELAY) {	/* NO DELAY */
#endif hpux
					if (count <= PIPE_BUF) {
						/*
						 * Write will be satisfied
						 * atomically, later.
						 */
#ifdef hpux
						/*
						 * Match the behaviour of
						 * the local code.
						 */
#ifdef POSIX
						if ( uiop->uio_fpflags&FNBLOCK )
						    rval = EAGAIN;
						else
						    rval = 0;
#else POSIX
						rval = 0;
#endif POSIX
#else hpux
						rval = EWOULDBLOCK;
#endif hpux
						goto rdwrdone;
					} else if (fp->fn_size >= PIPE_BUF) {
					    /*
					     * Write will never be atomic.
					     * At this point, it cannot even be
					     * partial.   However, some portion
					     * of the write may already have
					     * succeeded.  If so, uio_resid
					     * reflects this.
					     */
						if (ocnt == uiop->uio_resid)
#ifdef hpux
						    /*
						     * Match the behaviour of
						     * the local code.
						     */
#ifdef POSIX
						    if ( uiop->uio_fpflags&FNBLOCK )
							rval = EAGAIN;
						    else
						    	rval = 0;
#else POSIX
						    rval = 0;
#endif POSIX
#else hpux
						    rval = EWOULDBLOCK;
#endif hpux
						goto rdwrdone;
					}
				} else {			/* DELAY */
					if ( (count <= PIPE_BUF) ||
					    (fp->fn_size >= PIPE_BUF) ) {
				/*
				 * Sleep until there is room for this request.
				 * On wakeup, go back to the top of the loop.
				 */
						fp->fn_flag |= FIFO_WBLK;
						FIFOUNLOCK(fp);
						(void) sleep((caddr_t)
						    &fp->fn_wcnt, PPIPE);
						FIFOLOCK(fp);
						goto wrloop;
					}
				}
				/* at this point, can do a partial write */
				count = PIPE_BUF - fp->fn_size;
			}
			/*
			 * Can write 'count' bytes to pipe now.   Make sure
			 * there is enough space in the allocated buffer list.
			 * If not, try to allocate more.
			 * If allocation does not succeed immediately, go back
			 * to the  top of the loop to make sure everything is
			 * still cool.
			 */

#ifdef SANITY
			if ((fp->fn_wptr - fp->fn_rptr) != fp->fn_size)
			    printf("fifo_write: ptr mismatch...size:%d  wptr:%d  rptr:%d\n",
				fp->fn_size, fp->fn_wptr, fp->fn_rptr);

			if (fp->fn_rptr > PIPE_BSZ)
			    printf("fifo_write: rptr too big...rptr:%d\n",
				fp->fn_rptr);
			if (fp->fn_wptr > (fp->fn_nbuf * PIPE_BSZ))
			    printf("fifo_write: wptr too big...wptr:%d  nbuf:%d\n",
				fp->fn_wptr, fp->fn_nbuf);
#endif SANITY

			while (((fp->fn_nbuf * PIPE_BSZ) - fp->fn_wptr)
			    < count) {
				if ((bp = fifo_bufalloc(fp)) == NULL) {
					goto wrloop;	/* fifonode unlocked */
				}
				/* new buffer...tack it on the of the list */
				bp->fb_next = (struct fifo_bufhdr *) NULL;
				if (fp->fn_buf == (struct fifo_bufhdr *) NULL) {
					fp->fn_buf = bp;
				} else {
					fp->fn_bufend->fb_next = bp;
				}
				fp->fn_bufend = bp;
			}
			/*
			 * There is now enough space to write 'count' bytes.
			 * Find append point and copy new data.
			 */
			bp = fp->fn_buf;
			for (off = fp->fn_wptr; off >= PIPE_BSZ;
			    off -= PIPE_BSZ)
				bp = bp->fb_next;
		
			while (count) {
				i = PIPE_BSZ - off;
				i = MIN(count, i);
				if (rval =
				    uiomove(&bp->fb_data[off], (int) i,
				    UIO_WRITE, uiop)){
					/* error during copy from user space */
					/* NOTE:LEAVE ALLOCATED BUFS FOR NOW */
					goto rdwrdone;
				}
				fp->fn_size += i;
				fp->fn_wptr += i;
				count -= i;
				off = 0;
				bp = bp->fb_next;
			}
			FIFOMARK(fp, SUPD|SCHG);	/* update mod times */

			/* wake up any sleeping readers */
			if (fp->fn_flag & FIFO_RBLK) {
				fp->fn_flag &= ~FIFO_RBLK;
#ifdef MP
                                SETCURPRI(PPIPE, reg_temp);
#else
				curpri = PPIPE;
#endif
				wakeup((caddr_t) &fp->fn_rcnt);
			}

			/* wake up any sleeping read selectors */
			if (fp->fn_rsel) {
#ifdef MP
                                SETCURPRI(PPIPE, reg_temp);
#else
				curpri = PPIPE;
#endif
				selwakeup(fp->fn_rsel, fp->fn_flag&FIFO_RCOLL);
				fp->fn_flag &= ~FIFO_RCOLL;
				fp->fn_rsel = (struct proc *)0;
			}

wrloop: 		/* bottom of write 'while' loop */
			continue;
		}

	} else {					/* UIO_READ */
		/*
		 * Handle zero-length reads specially here
		 */
		if ((count = uiop->uio_resid) == 0) {
			goto rdwrdone;
		}
		while ((i = fp->fn_size) == 0) {
			if (fp->fn_wcnt == 0) {
				/* no data in pipe and no writers...(EOF) */
				goto rdwrdone;
			}
			/*
			 * No data in pipe, but writer is there;
			 * if No-Delay, return EWOULDBLOCK
			 * NOTE: for HP-UX match local behavior and return 0.
			 */
#ifdef hpux
			/* See note above */
			if (uiop->uio_fpflags & FNDELAY) {
				rval = 0;
				goto rdwrdone;
			}
#ifdef POSIX
			if (uiop->uio_fpflags & FNBLOCK) {
				rval = EAGAIN;
				goto rdwrdone;
			}
#endif POSIX
#else !hpux
			if (ioflag & IO_NDELAY) {
				rval = EWOULDBLOCK;
				goto rdwrdone;
			}
#endif hpux
			fp->fn_flag |= FIFO_RBLK;
			FIFOUNLOCK(fp);
			(void) sleep((caddr_t) &fp->fn_rcnt, PPIPE);
			FIFOLOCK(fp);
			/* loop to make sure there is still a writer */
		}

#ifdef SANITY
		if ((fp->fn_wptr - fp->fn_rptr) != fp->fn_size)
			printf("fifo_read: ptr mismatch...size:%d  wptr:%d  rptr:%d\n",
			    fp->fn_size, fp->fn_wptr, fp->fn_rptr);

		if (fp->fn_rptr > PIPE_BSZ)
			printf("fifo_read: rptr too big...rptr:%d\n",
			    fp->fn_rptr);

		if (fp->fn_wptr > (fp->fn_nbuf * PIPE_BSZ))
			printf("fifo_read: wptr too big...wptr:%d  nbuf:%d\n",
			    fp->fn_wptr, fp->fn_nbuf);
#endif SANITY

		/*
		 * Get offset into first buffer at which to start getting data.
		 * Truncate read, if necessary, to amount of data available.
		 */
		off = fp->fn_rptr;
		bp = fp->fn_buf;
		count = MIN(count, i);	/* smaller of pipe size and read size */

		while (count) {
			i = PIPE_BSZ - off;
			i = MIN(count, i);
			if (rval =
			    uiomove(&bp->fb_data[off], (int)i, UIO_READ, uiop)){
				goto rdwrdone;
			}
			fp->fn_size -= i;
			fp->fn_rptr += i;
			count -= i;
			off = 0;

#ifdef SANITY
			if (fp->fn_rptr > PIPE_BSZ)
				printf("fifo_read: rptr after uiomove too big...rptr:%d\n",
				    fp->fn_rptr);
#endif SANITY

			if (fp->fn_rptr == PIPE_BSZ) {
				fp->fn_rptr = 0;
				bp = fifo_buffree(bp, fp);
				fp->fn_buf = bp;
				fp->fn_wptr -= PIPE_BSZ;
			}
			/*
			 * At this point, if fp->fn_size is zero, there may be
			 * an allocated, but unused, buffer.  [In this case,
			 * fp->fn_rptr == fp->fn_wptr != 0.]
			 * NOTE: FOR NOW, LEAVE THIS EXTRA BUFFER ALLOCATED.
			 * NOTE: fifo_buffree() CAN'T HANDLE A BUFFER NOT 1ST.
			 */
		}
    
		FIFOMARK(fp, SACC);	/* update the access times */

		/* wake up any sleeping writers */
		if (fp->fn_flag & FIFO_WBLK) {
			fp->fn_flag &= ~FIFO_WBLK;
#ifdef MP
                        SETCURPRI(PPIPE, reg_temp);
#else
			curpri = PPIPE;
#endif
			wakeup((caddr_t) &fp->fn_wcnt);
		}

		/* wake up any sleeping write selectors */
		if (fp->fn_wsel) {
#ifdef MP
                        SETCURPRI(PPIPE, reg_temp);
#else
			curpri = PPIPE;
#endif
			selwakeup(fp->fn_wsel, fp->fn_flag&FIFO_WCOLL);
			fp->fn_flag &= ~FIFO_WCOLL;
			fp->fn_wsel = (struct proc *)0;
		}
	}		/* end of UIO_READ code */

rdwrdone:
	FIFOUNLOCK(fp);
	uiop->uio_offset = 0;		/* guarantee that f_offset stays 0 */
	return (rval);
}

int
fifo_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	register int error;
	register struct snode *sp;

	sp = VTOS(vp);
	error = VOP_GETATTR(sp->s_realvp, vap, cred, VSYNC);
	if (!error) {
		/* set current times from snode, even if older than vnode */
		vap->va_atime = sp->s_atime;
		vap->va_mtime = sp->s_mtime;
		vap->va_ctime = sp->s_ctime;

		/* size should reflect the number of unread bytes in pipe */
		vap->va_size = (VTOF(vp))->fn_size;
		vap->va_blocksize = PIPE_BUF;
	}
	return (error);
}

/*
 * test for fifo selections
 */
/*ARGSUSED*/
int
fifo_select(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct fifonode *fp;

	fp = VTOF(vp);

	switch (flag) {
	case FREAD:
		if (fp->fn_size != 0)		/* anything to read? */
			return (1);
#ifdef hpux
		if (fp->fn_wcnt <= 0 )		/* any writers left? */
			return (1);
#endif hpux
		if (fp->fn_rsel && fp->fn_rsel->p_wchan == (caddr_t)&selwait)
			fp->fn_flag |= FIFO_RCOLL;
		else
			fp->fn_rsel = u.u_procp;
		break;

	case FWRITE:
		/* is there room to write? (and are there any readers?) */
		if ((fp->fn_size < PIPE_BUF) && (fp->fn_rcnt > 0))
			return (1);
		if (fp->fn_wsel && fp->fn_wsel->p_wchan == (caddr_t)&selwait)
			fp->fn_flag |= FIFO_WCOLL;
		else
			fp->fn_wsel = u.u_procp;
		break;

#ifdef notdef
	case 0:
		if (fp->fn_rcnt == 0)		/* no readers anymore? */
			return (1);		/* exceptional condition */
		if (fp->fn_xsel && fp->fn_xsel->p_wchan == (caddr_t)&selwait)
			fp->fn_flag |= FIFO_XCOLL;
		else
			fp->fn_xsel = u.u_procp;
		break;
#endif notdef
	}
	return (0);
}

int
fifo_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	/* !!! Problem - spec_fsync calls VOP_GETATTR and can goes to
	   sleep.  If another process does VN_HOLD/VN_RELE the v_count
	   will equals to 0 and fifo_inactive will be called to free
	   up the snode.  When VOP_GETATTR wakes up and call kmem_free
	   - panic - the snode has already been freed.  The fix is:
	   v_count++ before spec_fsync and v_count-- when return from
	   spec_fsync so that no other process will call fifo_inactive
	   while it is sleeping.  Additional check is also added to make 
	   sure v_count is still 0 when we return so that no other
	   process does VN_RELE and calls fifo_inactive again.  
	 */
	SPINLOCK(v_count_lock);
	vp->v_count++;
	SPINUNLOCK(v_count_lock);
	(void) spec_fsync(vp, cred, 0);
	SPINLOCK(v_count_lock);
	vp->v_count--;
	SPINUNLOCK(v_count_lock);
	if (vp->v_count > 0) return(0);
	sunsave(VTOS(vp));
	kmem_free((caddr_t)VTOF(vp), (u_int)sizeof (struct fifonode));
	return (0);
}

int
fifo_invalop()
{
	u.u_error = EINVAL;
	return (EINVAL);
}

/*
 * Same as fifo_invalop(), but returns ENOTDIR for directory type operations.
 */
int
fifo_notdir()
{
	u.u_error = ENOTDIR;
	return (ENOTDIR);
}

#ifdef notdef
int
fifo_badop()
{
	panic("fifo_badop\n");
}
#endif notdef


/*
 * construct a fifonode that can masquerade as an snode
 */
struct snode *
fifosp(vp)
	struct vnode *vp;
{
	register struct fifonode *fp;
	struct vattr va;

	fp = (struct fifonode *)kmem_alloc((u_int)sizeof (*fp));
	bzero((caddr_t)fp, sizeof (*fp));
	FTOV(fp)->v_op = &fifo_vnodeops;
	FTOV(fp)->v_fstype = VNFS_FIFO;

	/* init the times in the snode to those in the vnode */
	(void) VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
	FTOS(fp)->s_atime = va.va_atime;
	FTOS(fp)->s_mtime = va.va_mtime;
	FTOS(fp)->s_ctime = va.va_ctime;
	return (FTOS(fp));
}

/*
 * allocate a buffer for a fifo
 * return NULL if had to sleep
 */
static struct fifo_bufhdr *
fifo_bufalloc(fp)
	register struct fifonode *fp;
{
	register struct fifo_bufhdr *bp;

	if (fifo_alloc >= PIPE_MNB) {
		/*
		 * Impose a system-wide maximum on buffered data in pipes.
		 * NOTE: This could lead to deadlock!
		 */
		FIFOUNLOCK(fp);
		(void) sleep((caddr_t) &fifo_alloc, PPIPE);
		FIFOLOCK(fp);
		return ((struct fifo_bufhdr *)NULL);
	}

	/* the call to kmem_alloc() might sleep, so leave fifonode locked */

	fifo_alloc += FIFO_BUFFER_SIZE;
	bp = (struct fifo_bufhdr *)kmem_alloc((u_int)FIFO_BUFFER_SIZE);
	fp->fn_nbuf++;
	return ((struct fifo_bufhdr *) bp);
}


/*
 * deallocate a fifo buffer
 */
static struct fifo_bufhdr *
fifo_buffree(bp, fp)
	struct fifo_bufhdr *bp;
	struct fifonode *fp;
{
	register struct fifo_bufhdr *nbp;
        register u_int reg_temp;

	fp->fn_nbuf--;

	/*
	 * NOTE: THE FOLLOWING ONLY WORKS IF THE FREED BUFFER WAS THE 1ST ONE.
	 */
	if (fp->fn_bufend == bp) {
		fp->fn_bufend = (struct fifo_bufhdr *) NULL;
		nbp = (struct fifo_bufhdr *) NULL;
	} else
		nbp = bp->fb_next;

	kmem_free((caddr_t)bp, (u_int)FIFO_BUFFER_SIZE);

	if (fifo_alloc >= PIPE_MNB) {
#ifdef MP
                SETCURPRI(PPIPE, reg_temp);
#else
		curpri = PPIPE;
#endif
		wakeup((caddr_t) &fifo_alloc);
	}
	fifo_alloc -= FIFO_BUFFER_SIZE;

	return (nbp);
}

#ifdef POSIX
/*
 * Special case handling with NFS fifos.  Since one of the calls of pathconf
 * is the size of the pipe buffer, we have to return a correct value
 * for that.  Everything else goes to the real vnode pointers.
 */

int
fifo_pathconf(vp, name, resultp, cred)
struct vnode *vp;
int	name;
int	*resultp;
struct ucred *cred;
{
	switch(name) {

	case _PC_PIPE_BUF:	/* Max atomic write to pipe.  See fifo_vnops */
		*resultp = PIPE_BUF;
		break;
	default:
		return ( spec_pathconf( vp, name, resultp, cred ));
	
	}

	return ( 0 );
}

#endif POSIX
