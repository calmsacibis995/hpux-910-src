/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_dscrp.c,v $
 * $Revision: 1.58.83.5 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/31 12:01:24 $
 */

/* HPUX_ID: @(#)kern_dscrp.c	55.1		88/12/23 */

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
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/netfunc.h"
#include "../h/stat.h"

#ifdef __hp9000s300
#include "../h/ttold.h"
#endif

#include "../h/ioctl.h"
#include "../h/netfunc.h"

#ifdef SAR
#include "../h/sar.h"
#endif

#ifdef __hp9000s300
#include "../machine/reg.h"	/* for R1 value */
#endif

#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI

#include "../h/kern_sem.h"

#ifdef SYSCALLTRACE
int  lockfdbg;
#endif SYSCALLTRACE


#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_dev.h"
#include "../h/unistd.h"


/*
 * Descriptor management.
 */

/*
 * TODO:
 *	increase NOFILE
 *	eliminate u.u_error side effects
 */

/*
 * System calls on descriptors.
 */

#ifdef __hp9000s300
getdtablesize()
{

	u.u_r.r_val1 = u.u_maxof;
}

getdopt()
{

}

setdopt()
{

}
#endif /* __hp9000s300 */

dup()
{
	register struct a {
		int	i;
	} *uap = (struct a *) u.u_ap;
	struct file *fp;
	int j;

	fp = getf(uap->i);
	if (fp == 0)
		return;
	j = ufalloc(0);
	if (j < 0)
		return;
	dupit(j, fp, getp(uap->i));
}

dup2()
{
	register struct a {
		int	i, j;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp, *jfp;
	register int k;

	fp = getf(uap->i);
	if (fp == NULL)
		return;
	if (uap->j < 0 || uap->j >= u.u_maxof) {
		u.u_error = EBADF;
		return;
	}
	u.u_r.r_val1 = uap->j;
	if (uap->i == uap->j)
		return;
	if ((jfp = getf(uap->j)) != NULL) {
		closef(jfp);
		/*
		 * Ignore all errors from closef().
		 * Even if an error occurred (e.g. an error
		 * from nfs_close()), closef() will have
		 * freed up the file table entry so we might
		 * as well as go ahead.  These semantics are
		 * identical to a close() system call.
		 * This fixes DSDe400020. -byb 2/7/88
		 */
		 u.u_error = 0;
	} else {
		/*
		 * getf will have set u.u_error to EBADF because 
		 * uap->j was not an allocated file descriptor.  As
		 * we are about to allocate it, we can ignore this
		 * error.
		 */
		u.u_error = 0;
		k = ufalloc(uap->j);
		if (k < 0)
			return;
		if (k != uap->j) {
			uffree(k);
			return;
		}
	}

	dupit(uap->j, fp, getp(uap->i));
}

dupit(fd, fp, flags)
	int fd;
	register struct file *fp;
	register int flags;
{
	register struct file **fpp;
	register char *cp;

	fpp = getfp(fd);
	*fpp = fp;

	/* The duplicated descriptor should remain open across exec() */

	cp = getpp(fd);
	*cp = flags & ~UF_EXCLOSE;
	SPINLOCK(file_table_lock);
	fp->f_count++;
	SPINUNLOCK(file_table_lock);
}

/*
 * The file control system call.
 */
fcntl()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		int	cmd;
		union	{
			int val;
			struct flock *lockdes;
		} arg;
	} *uap;
	register i;
	register char *pop;
	struct flock flock;
	struct vnode *vp;
	off_t LB,UB;

	uap = (struct a *)u.u_ap;
	fp = getf(uap->fdes);
	if (fp == NULL)
		return;
	pop = getpp(uap->fdes);
	switch(uap->cmd) {
	case F_DUPFD:
		i = uap->arg.val;
		if (i < 0 || i >= u.u_maxof ) {
			u.u_error = EINVAL;
			return;
		}
		if ((i = ufalloc(i)) < 0)
			return;
		/*
		 * Bug fix to make us Bell compatible.
		 * Clear the close on exec flag of the
		 * new file discriptor.
		 */

		dupit(i, fp, (*pop) &~ UF_EXCLOSE);
		break;

	case F_GETFD:
		u.u_r.r_val1 = *pop & 1;
		break;

	case F_SETFD:
		*pop = (*pop &~ 1) | (uap->arg.val & 1);
		break;

	case F_GETFL:
		u.u_r.r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		SPINLOCK(file_table_lock);
#ifdef POSIX
		if ((uap->arg.val&(FNDELAY|FNBLOCK)) == (FNDELAY|FNBLOCK)) {
			u.u_error = EINVAL; /* can't have it both ways */
			SPINUNLOCK(file_table_lock);
			return;
		}
#endif
		fp->f_flag &= FCNTLCANT;
		fp->f_flag |= (uap->arg.val-FOPEN) &~ FCNTLCANT;
		SPINUNLOCK(file_table_lock);
		break;

	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:

	   switch (fp->f_type) {

	   case DTYPE_VNODE:
		if (copyin(uap->arg.lockdes, &flock, sizeof(struct flock))) {
			u.u_error = EFAULT;
			return;
		}

		PSEMA(&filesys_sema);
		vp =(struct vnode *) fp->f_data;
		LB = flock.l_start;
		if (flock.l_whence == 1)
			LB += fp->f_offset;
		/*
		 * With support for NFS and other file systems, we don't
		 * know for sure that we have an inode.  Therefore we have
		 * to do a GETATTR() to get the current file size.
		 */
		else if (flock.l_whence == 2) {
			struct vattr va;
			if((u.u_error = VOP_GETATTR(vp, &va, fp->f_cred,VSYNC)))
				goto fs_return;
			LB += va.va_size;
		}
		else if (flock.l_whence != 0)  {
			u.u_error = EINVAL;
			goto fs_return;
		}
		if (LB > MAX_LOCK_SIZE)
		  	LB = MAX_LOCK_SIZE;
		else
			if (LB < 0)  { 
				u.u_error = EINVAL;
				goto fs_return;
			}
		if (flock.l_len == 0)
			UB = MAX_LOCK_SIZE;
		else {
			UB = LB + flock.l_len;
			if (UB > MAX_LOCK_SIZE)
				UB = MAX_LOCK_SIZE;
			if ((UB < 0) || (UB < LB) || (UB == LB)) {
				u.u_error = EINVAL;
				goto fs_return;
			}
		}

		/* check validity of lock type */
		switch(flock.l_type) {
		case F_RDLCK:
		case F_WRLCK:
		case F_UNLCK:
			break;
		default:
			u.u_error = EINVAL;
			goto fs_return;
		}

		/* check access permissions */
		if (uap->cmd == F_SETLK || uap->cmd == F_SETLKW) {
			if ((flock.l_type == F_RDLCK &&
		     	        (fp->f_flag&FREAD) == 0) ||
		  	    (flock.l_type == F_WRLCK &&
		     	        (fp->f_flag&FWRITE) ==0))
			{
				u.u_error = EBADF;
				goto fs_return;
			}
		}

		/*
		 * Farm out the request to the different file systems to
		 * handle.  Then, if it was a GETLK request copy it back
		 * to user land, assuming the VOP calls updated flock.
		 */
		u.u_error = VOP_LOCKCTL( vp, &flock, uap->cmd, fp->f_cred,
					 fp, LB, UB);
		if ( (uap->cmd == F_GETLK) && !u.u_error )
			if( copyout( &flock, uap->arg.lockdes,
					sizeof( struct flock)))
				u.u_error = EFAULT;

		if (!u.u_error && (uap->cmd != F_GETLK) &&
		    ( flock.l_type != F_UNLCK ) ) {
			/*
			 * If any locking is attempted and succeeded, mark
			 * file locked to force unlock on close.  Also, since
			 * the SVID specifies that the FIRST close releases all
			 * locks, mark process to reduce the search overhead in
			 * vno_lockrelease() (in kern_dscrp.c).
			 */
			*pop |= UF_FDLOCK;
			SPINLOCK(sched_lock);
			u.u_procp->p_flag2 |= SLKDONE;
			SPINUNLOCK(sched_lock);
		}
		goto fs_return;

	   fs_return:
		VSEMA(&filesys_sema);
		break;

	   default:
		u.u_error = EINVAL;

	   }   /* switch (fp->f_type) */
           break;

	default:
		u.u_error = EINVAL;
	}
}

close()
{
	register struct a {
		int	i;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	fp = getf(uap->i);
	if (fp == 0)
		return;
	closef(fp);
	/*
	 * We need to clear the flags AFTER the closef() because
	 * vno_lockrelease (added as part of NFS 3.2 features) uses the
	 * PerOpenFile flags to help save time when searching to see if
	 * locks have been done.
	 */
	uffree(uap->i);
	/* WHAT IF u.u_error ? */
}

fstat ()
{
	extern struct fileops lancops;
	register struct file *fp;
	register struct a {
		int	fdes;
		struct	stat *sb;
	} *uap;
	struct stat ub;

	bzero (&ub, sizeof(struct stat));
	uap = (struct a *)u.u_ap;
	fp = getf(uap->fdes);
	if (fp == 0)
		return;
	switch (fp->f_type) {

	case DTYPE_VNODE:
		/* We will worry about this locking later. HD. */
		PSEMA(&filesys_sema);
		u.u_error = vno_stat((struct vnode *)fp->f_data, &ub, fp->f_cred,FOLLOW_LINK);
		VSEMA(&filesys_sema);
		break;

	case DTYPE_LLA:
		/* open lan device -- it really ought to work */
		u.u_error = EINVAL;
		break;
		
	case DTYPE_SOCKET:
		u.u_error = NETCALL(NET_SOO_STAT)((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/*NOTREACHED*/
	}
	if (u.u_error == 0) {
		u.u_error = copyout((caddr_t)&ub, (caddr_t)uap->sb,
		    sizeof (ub));
	}
}

/*
 * Allocate a user file descriptor.
 */
int
ufalloc(i)
	register int i;
{
	register int j, index;

	if (i >= u.u_maxof) {
		u.u_error = EMFILE;
		return(-1);
	}
	/* 
	 * Must calculate
	 * here which is the lowest file chunk the requested file descriptor
	 * could fall into, and make sure that the ofile pointers between
	 * u.u_ofilep[0] and this requested chunk all have been kmem_alloc'd
	 * and initialized, because we don't want any holes in the u_ofilep
	 * array. 
	 */
	index = (unsigned)i >> SFDSHIFT;
	for (j=0; j<index; j++) {
		if (u.u_ofilep[j] == NULL) {
			u.u_ofilep[j] = 
			(struct ofile_t *)kmem_alloc(sizeof(struct ofile_t));

			bzero((caddr_t)u.u_ofilep[j], sizeof(struct ofile_t));
		}
	}
	/* 
	 * Look at the first indirect file descriptor chunk, or index, 
	 * to find the lowest file chunk which the file descriptor 
	 * number requested by the caller could fall into. 
	 */
	for (j = index; j < NFDCHUNKS(u.u_maxof) && i < u.u_maxof; j++) {
		if (u.u_ofilep[j] == NULL) {
			u.u_ofilep[j] = 
			(struct ofile_t *)kmem_alloc(sizeof(struct ofile_t));

			bzero((caddr_t)u.u_ofilep[j], sizeof(struct ofile_t));

			u.u_r.r_val1 = i;
			u.u_ofilep[j]->pofile[i & SFDMASK] = 0;
			u.u_highestfd = i;
			return(i);
		}
		for (;i < u.u_maxof && i < (j+1)<<SFDSHIFT; i++) {
			if (u.u_ofilep[j]->ofile[i & SFDMASK] == NULL) {
			/* Found slot */

				u.u_r.r_val1 = i;
				u.u_ofilep[j]->pofile[i & SFDMASK] = 0;

				if (u.u_highestfd < i)
					u.u_highestfd = i;

				return(i);
			}
		}
	}
	u.u_error = EMFILE;
	return(-1);
}

/*
 * Frees a slot in the user's file descriptor table.
 */
int
uffree(i)
	register int i;
{
	register int j, index, startindex;

	if (i > u.u_highestfd) /* larger than highest open file */
		return(-1);

	index = (unsigned)i >> SFDSHIFT;

	if (u.u_ofilep[index] == NULL) 
	  /* This file descriptor has already been freed */
	  return(-1);
	
	j = i & SFDMASK;
	if (u.u_ofilep[index]->ofile[j] == NULL)
	  return(-1); /* already freed */
	
	u.u_ofilep[index]->ofile[j] = NULL;
	u.u_ofilep[index]->pofile[j] = 0;

	if (u.u_highestfd == i) {
	/*
	 * Just closed the highest file descriptor -- must find next highest.
	 */
	    startindex = index;
	    for (;index >= 0;index--) {
		if (index == startindex)
		  j = (i & SFDMASK) - 1;
		else
		  j = SFDCHUNK - 1;
		for (;j >=0 && u.u_ofilep[index]->ofile[j] == NULL; j--)
		  ;
		if (j < 0) {
		    /* nothing in this chunk -- kmem_free */
		    kmem_free((caddr_t)u.u_ofilep[index],sizeof(struct ofile_t));
		    u.u_ofilep[index] = NULL;
		} else {
		    u.u_highestfd = (index << SFDSHIFT) + j;
		    break;
		}
	    }
	    if (u.u_highestfd == i)
	      u.u_highestfd = -1;
	}
	return(0);
}

/*
 * Returns 1 if the number of slots available in the user's file descriptor
* is >= need, 0 otherwise.
 */
int
ufavail(need)
	int need;
{
	register int i, j, k, avail = 0;

	if (u.u_maxof - u.u_highestfd > need)
		return(1);

	for (i=0, k=0; k<NFDCHUNKS(u.u_maxof) && i<u.u_maxof && avail < need;k++)
		if (u.u_ofilep[k] != NULL)
			for (j=0;j<SFDCHUNK && i<u.u_maxof && avail < need;j++, i++)
				if (u.u_ofilep[k]->ofile[j] == NULL)
					avail++;
		else {
			if (i + SFDCHUNK >= u.u_maxof)
			/* 
			 * reached maximum file descriptor slot for this process
			 * avail = avail + SFDCHUNK - (i + SFDCHUNK - u.u_maxof)
			 */
				avail += u.u_maxof - i; 
			else 
				avail += SFDCHUNK;

			i += SFDCHUNK;
		}
	return((avail >= need));
}

struct	file *lastf;
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct file *
falloc()
{
	register struct file *fp, **fpp;
	register i;

	i = ufalloc(0);
	if (i < 0)
		return (NULL);
	/*	This spinlock protects "lastf" too */
	SPINLOCK(file_table_lock);
	if (lastf == 0)
		lastf = file;

	/* We have added a padding to the end of file table to reserve
	   slots for the super-user when the file table has been filled.
	   So, this first loop searches from lastf to the begining of 
	   the part of the table that is reserved for the super-user. */
	   
	for (fp = lastf; fp < file_reserve; fp++)
		if (fp->f_count == 0)
			goto slot;
	for (fp = file; fp < lastf; fp++)
		if (fp->f_count == 0)
			goto slot;
	/* If we've gotten this far, the regular part of the table is full 
	   and if we're the super-user, we may use the end of the file 
	   table. */
	if (suser()) {
	    for (fp = file_reserve; fp < fileNFILE; fp++)
	      if (fp->f_count == 0)
		  goto slot;
	}
	  
	SPINUNLOCK(file_table_lock);
	tablefull("file");
#ifdef SAR
	fileovf++;
#endif 
	uffree(i);
	u.u_error = ENFILE;
	return (NULL);
slot:
	fpp = getfp(i);
#ifndef _WSIO
	*fpp = fp;
#else /* _WSIO */
	*fpp = u.u_fp = fp;
#endif /* _WSIO */
	fp->f_count = 1;
	fp->f_data = 0;
	fp->f_offset = 0;

	/* If we've just used one of the file table entries reserved for 
	   the super-user when the table is full, we don't want to set 
	   lastf to be an entry in the reserved portion of the table.  If
	   we did, then the next time through a normal user might be able to 
	   allocate one of the reserved slots (for loop #2 above) */
	if (fp < file_reserve)
	  lastf = fp + 1;
	SPINUNLOCK(file_table_lock);
	crhold(u.u_cred);
	fp->f_cred = u.u_cred;
	return (fp);
}

/*
 * Convert a user supplied file descriptor into a pointer
 * to a file structure.
 * WARNING!  This function will set u.u_error to EBADF if it is passed
 * a file descriptor which does not correspond to a currently opened file.
 * If the calling routine wishes to return immediately with an error if
 * the file descriptor is not valid, this is all right.  If the calling
 * routine looping all possible valid file descriptors and does not care if
 * some of them are invalid, the calling routine should either reset 
 * u.u_error to zero after getf(), or save u.u_error before calling getf()
 * and restore it afterwards, or use the function getf_no_ue instead.
 * getf_no_ue() is identical to getf(), except it will not set u.u_error.
 */
struct file *
getf(f)
	register int f;
{
	int index;
	struct file *fp;

	if (u.u_highestfd < 0 || (unsigned)f > u.u_highestfd) {
#ifdef _WSIO
		u.u_fp = NULL;
#endif /* _WSIO */
		u.u_error = EBADF;
		return(NULL);
	}

	index = (unsigned)f >> SFDSHIFT;
	if (index < NFDCHUNKS(u.u_maxof))
		if ((u.u_ofilep[index]) != NULL)
#ifdef _WSIO
			if ((fp = u.u_fp = u.u_ofilep[index]->ofile[(unsigned)f & SFDMASK]) != NULL)
#else
			if ((fp = u.u_ofilep[index]->ofile[(unsigned)f & SFDMASK]) != NULL)
#endif /* _WSIO */
			  return(fp);

	/* 
	   either index >= NFDCHUNKS(u.u_maxof), so too large to be legal; or
	   the indirect pointer for index, u.u_ofilep[index - 1] is NULL; or
	   the file pointer in the indirect pointer array is NULL 
	*/
	   
#ifdef _WSIO
	u.u_fp = NULL;
#endif /* _WSIO */
	u.u_error = EBADF;
	return(NULL);
}

/*
 * getf_no_ue() is identical to getf(), except it will not set u_error
 * if it is passed an invalid file descriptor.  For use when looping
 * through all possible file descriptors.  If you are checking for a
 * single file and intend to return with error if the file descriptor
 * is not valid, use getf().
 */
struct file *
getf_no_ue(f)
	register int f;
{
	int index;
	struct file *fp;

	if ((unsigned)f > u.u_highestfd) {
#ifdef _WSIO
		u.u_fp = NULL;
#endif /* _WSIO */
		return(NULL);
	}

	index = (unsigned)f >> SFDSHIFT;
	if (index < NFDCHUNKS(u.u_maxof))
		if ((u.u_ofilep[index]) != NULL)
#ifdef _WSIO
			if ((fp = u.u_fp = u.u_ofilep[index]->ofile[(unsigned)f & SFDMASK]) != NULL)
#else /* not _WSIO */
			if ((fp = u.u_ofilep[index]->ofile[(unsigned)f & SFDMASK]) != NULL)
#endif /* _WSIO */
				return(fp);

	/* 
	   either index >= NFDCHUNKS(u.u_maxof), so too large to be legal; or
	   the indirect pointer for index, u.u_ofilep[index] is NULL; or
	   the file pointer in the indirect pointer array is NULL 
	*/
	   
#ifdef _WSIO
	u.u_fp = NULL;
#endif /* _WSIO */
	return(NULL);
}

/*
 * getfp returns a pointer to location u.u_ofile.ofile[f].  Should be
 * used when you need to modify u.u_ofile to make it point to something else.
 */
struct file **
getfp(f)
	register int f;
{
	int index;
	struct file **fpp;

	if ((unsigned)(f) > u.u_highestfd) {
	/* f bigger than largest open fd for this process */
		u.u_error = EBADF;
		return(NULL);
	}


	index = (unsigned)f >> SFDSHIFT;
	if (index < NFDCHUNKS(u.u_maxof))
		if ((u.u_ofilep[index]) != NULL)
			if ((fpp = &u.u_ofilep[index]->ofile[(unsigned)f & SFDMASK]) != NULL)
				return(fpp);

	/* 
	   either index >= NFDCHUNKS(u.u_maxof), so too large to be legal; or
	   the indirect pointer for index, u.u_ofilep[index - 1] is NULL; or
	   the file pointer in the indirect pointer array is NULL 
	*/
	   
	u.u_error = EBADF;
	return(NULL);
}

/*
 * getp returns the contents of u.u_ofile.pofile[f].  Should be used when
 * you need to examine u.u_ofile.pofile[f].  
 * macro.
 */
char
getp(f)
	register int f;
{
	int index;

	if ((unsigned)(f) > u.u_highestfd) {
	/* f bigger than largest open fd for this process */
		u.u_error = EBADF;
		return(-1);
	}

	index = (unsigned)f >> SFDSHIFT;
	if (index < NFDCHUNKS(u.u_maxof))
		return(u.u_ofilep[index]->pofile[(unsigned)f & SFDMASK]);

	/* 
	   index >= NFDCHUNKS(u.u_maxof)
	*/
	u.u_error = EBADF;
	return(-1);
}

/*
 * getpp returns a pointer to location u.u_ofile.pofile[f].  Should be used
 * when you need to modify u.u_ofile.pofile[f].  
 * GETPP macro.
 */
char *
getpp(f)
	register int f;
{
	int index;

	if ((unsigned)(f) > u.u_highestfd) {
	/* f bigger than largest open fd for this process */
		u.u_error = EBADF;
		return(NULL);
	}

	index = (unsigned)f >> SFDSHIFT;
	if (index < NFDCHUNKS(u.u_maxof))
		return(&u.u_ofilep[index]->pofile[(unsigned)f & SFDMASK]);

	/* 
	   index >= NFDCHUNKS(u.u_maxof)
	*/
	u.u_error = EBADF;
	return(NULL);
}

#ifdef _WSIO
extern int dil_exit();
#endif /* _WSIO */

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 * If last reference not going away, but no more
 * references except in message queues, run a
 * garbage collect.  This would better be done by
 * forcing a gc() to happen sometime soon, rather
 * than running one each time.
 */
closef(fp)
	register struct file *fp;
{
	sv_sema_t closef_ss;
#ifdef _WSIO
	register struct vnode *vp;
	register dev_t dev;
#endif /* _WSIO */

	if (fp == NULL)
		return;
	SPINLOCK(file_table_lock);

	if (fp->f_count > 1) {
		fp->f_count--;
		SPINUNLOCK(file_table_lock);
#ifdef _WSIO
		dil_exit(fp);
		vp = (struct vnode *)fp->f_data;
#endif /* _WSIO */
		if (fp->f_type == DTYPE_VNODE) {
			PXSEMA(&filesys_sema, &closef_ss);
		    /*
		     * Instead of calling the file system specific routines,
		     * call the more generic vno_lockrelease().  That
		     * routine will essentially do an unlock [0,infinity)
		     * having the same effect as the unlock() routine 
		     * called by the local code here, but working with
		     * NFS as well as UFS and DUX.
		     */
		    (void) vno_lockrelease(fp);
			VXSEMA(&filesys_sema, &closef_ss);
		}   /*  fp->f_type == DTYPE_VNODE */

#ifdef _WSIO
		if (fp->f_type == DTYPE_VNODE) {
		    if (vp->v_fstype == VDUX || vp->v_fstype == VUFS) {
			dev = vp->v_rdev;
			switch(vp->v_type) {
			case VBLK:
				if (bdevsw[major(dev)].d_flags & C_EVERYCLOSE)
					(*bdevsw[major(dev)].d_close)(dev, fp->f_flag);
				break;
			case VCHR:
				if (cdevsw[major(dev)].d_flags & C_EVERYCLOSE)
					(*cdevsw[major(dev)].d_close)(dev, fp->f_flag);
				break;
			default:
				break;
			}
		    }
		}
#endif /* _WSIO */

#ifdef FSD_KI
		KI_closef(fp);
#endif /* FSD_KI */
		return;
	} else
		SPINUNLOCK(file_table_lock);
#ifdef _WSIO
	u.u_fp = fp;
#endif /* _WSIO */
	if (fp->f_count < 1)
		panic ("closef: count < 1");
#ifdef FSD_KI
	fp->f_count = 0;
	KI_closef(fp);
	fp->f_count = 1;
#endif /* FSD_KI */
	(*fp->f_ops->fo_close)(fp);
	crfree(fp->f_cred);
	fp->f_cred = NULL;
	fp->f_count = 0;
}


/*
 * This routine is called for every file close in order to implement the 
 * SVID 'feature' that the FIRST close of a descriptor that refers to
 * a locked object causes all the locks to be released for that object.
 * It is called primarily by closef().
 *
 * NOTE:  Sun calls vno_lockrelease() from outside of closef() because
 *  they have some places in the kernel that shouldn't be doing unlocks. 
 *  We don't.  The criteria chosen for who calls vno_lockrelease() is
 *  basically any routine that was doing an unlock() that was not obviously
 *  an UFS or DUX only routine.
 *
 * THE FOLLOWING IS SUN'S NOTE:
 * NOTE: If the SVID ever changes to hold locks until the LAST close,
 *       then this routine might be moved to closef() [note that the
 *       window system calls closef() directly for file descriptors
 *       that is has dup'ed internally....such descriptors may or may
 *       not count towards holding a lock]
 *
 * TODO: The record-lock flag should be in the u-area.
 */

int
vno_lockrelease(fp)
        register struct file *fp;
{
	register struct vnode *vp;
	register struct file *ufp;
	register int i;
	register int locked;
	struct flock ld;
	off_t LB, UB;
	char *pp;
        /*
         * Only do extra work if the process has done record-locking.
         */
        if (u.u_procp->p_flag2 & SLKDONE) {

#ifdef _WSIO
		struct file *savefp;
#endif /* _WSIO */
                locked = 0;             /* innocent until proven guilty */
		SPINLOCK(sched_lock);
                u.u_procp->p_flag2 &= ~SLKDONE; /* reset process flag */
		SPINUNLOCK(sched_lock);
                vp = (struct vnode *)fp->f_data;
                /*
                 * Check all open files to see if there's a lock
                 * possibly held for this vnode.
		 *
		 * On the Series 200/300/700 we need to save away u.u_fp and
		 * restore it because the subsequent getf()'s will destroy
		 * it and end up confusing the DIL drivers.
                 */
#ifdef _WSIO
		savefp = u.u_fp;
#endif /* _WSIO */
                for (i = u.u_highestfd; i>= 0; i-- ) {
			if (((ufp = getf(i)) != NULL) && 
				(((pp = getpp(i)) != NULL) && (*pp & UF_FDLOCK))) {

                                /* the current file has an active lock */
                                if ((struct vnode *)ufp->f_data == vp) {

                                        /* release this lock */
                                        locked = 1;     /* (later) */
                                        *pp &= ~UF_FDLOCK;
                                } else {

                                        /* another file is locked */
					SPINLOCK(sched_lock);
                                        u.u_procp->p_flag2 |= SLKDONE;
					SPINUNLOCK(sched_lock);
                                }
                        }
                } /*for all files*/
#ifdef _WSIO
		u.u_fp = savefp;
#endif /* _WSIO */
		/*
		 * getf will set u_error to EBADF if called for a file
		 * descriptor which is not open.  Since we are looping
		 * through all possible file descriptors 0 - highestfd
		 * the chances of hitting a non-used file descriptor
		 * are good, and we can ignore this error.
		 */
		u.u_error = 0;

                /*
                 * If 'locked' is set, release any locks that this process
                 * is holding on this file.  If record-locking on any other
                 * files was detected, the process was marked (SLKDONE) to
                 * run thru this loop again at the next file close.
                 */
                 if (locked) {
		    ld.l_type = F_UNLCK;    /* set to unlock entire file */
		    ld.l_whence = 0;        /* unlock from start of file */
		    ld.l_start = 0;
		    ld.l_len = 0;           /* do entire file */
		    LB = 0;
		    UB = MAX_LOCK_SIZE;		/* NOTE: Defined in fcntl() */

		    (void) VOP_LOCKCTL(vp, &ld, F_SETLK, u.u_cred, fp, LB, UB);
                 }
        }
        return (0);
}
