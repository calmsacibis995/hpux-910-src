/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_lockf.c,v $
 * $Revision: 1.32.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:22:07 $
 */

/* HPUX_ID: @(#)ufs_lockf.c	55.1		88/12/23 */

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

/*
 * file lock routines
 * John Bass, PO Box 1223, San Luis Obispo, CA 93401
 * original design spring 1976, CalPoly, San Luis Obispo
 * Deadlock idea from Ed Grudzien at Basys April 1980
 * extensions fall 1980, Onyx Systems Inc., San Jose
 */


#include "../h/param.h"
#include "../h/systm.h"
#include "../dux/nsp.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/unistd.h"

#include "../h/errno.h"
#include "../machine/reg.h"
#include "../h/kern_sem.h"

#ifdef SYSCALLTRACE
int lockfdbg;
#endif SYSCALLTRACE


#include "../h/conf.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"

/* region types */
#define S_BEFORE 010
#define S_START  020
#define S_MIDDLE 030
#define S_END    040
#define S_AFTER  050
#define E_BEFORE 001
#define E_START  002
#define E_MIDDLE 003
#define E_END    004
#define E_AFTER  005

extern int nflocks;

/*
 * lockf -- handles syscall requests
 */

lockf()
{
	struct file *fp;
	/*
	 * define order and type of syscall args
	 */
	register struct a {
		int	fdes;
		int	flag;
		off_t	size;
	} *uap = (struct a *)u.u_ap;
	off_t	offset;
	off_t   LB, UB;

#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("lockf(fdes %d, flag 0x%x, size 0x%x)\n",
			uap->fdes, uap->flag, uap->size);
#endif SYSCALLTRACE
	/*
	 * check for valid open file
	 */
	fp = getf(uap->fdes);
	if(fp == NULL) return(-1);

	/* in case fp is a socket or any other non-inode type */
	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = EINVAL;
		return(-1);
	}


	/* check and make sure not trying to lock a file opened
	 * for reading (unless backward compatibility mode set)
	 */
	if (((uap->flag == F_LOCK) || (uap->flag == F_TLOCK))
	&& (fp->f_flag & FREAD)
	&& (!(fp->f_flag & FWRITE))
	&& (!in_privgrp(PRIV_LOCKRDONLY, 0))) {
	        u.u_error = EBADF;
		return(-1);
	}


	/* For pipes we always treat the offset as 0.  This is because seeks
	 * are not permitted on a fifo, thus if a lock is done followed by
	 * a read or a write, it would not be possible to seek back to 0 to
	 * unlock the pipe.
	 * DTS DSDe400731
	 */
	if (((struct vnode *)(fp->f_data))->v_type == VFIFO)
		offset = 0;
	else
		offset = fp->f_offset;
	/*
	 * Check for valid command
	 * We assume that F_TEST is the largest and
	 * F_ULOCK is the lowest here.
	 */
	if ((uap->flag > F_TEST) || (uap->flag < F_ULOCK) || (offset > MAX_LOCK_SIZE)) {
		u.u_error = EINVAL;
		return(-1);
	}

	/*
	 * validate ranges
	 */
	if (uap->size < 0) {
		/*
		** Note: UB is the first byte not
		** included in the lock. If size<0
		** then the current offset is not
		** included.
		*/
		UB = offset;
		LB = UB + uap->size;
		if (LB < 0) {
			u.u_error = EINVAL;
			return(-1);
		}
	}
	else {
		/*
		** do the work for size>=0
		*/
		LB = offset;
		UB = LB + uap->size;
		if ((UB <= 0) || (uap->size == 0) || (UB > MAX_LOCK_SIZE))
			UB = MAX_LOCK_SIZE;
	}
	/* catch the hole that LB = UB due to both >= MAX_LOCK_SIZE */
	if (LB == UB) {
		u.u_error = EINVAL;
		return(-1);
	}

	PSEMA(&filesys_sema);
	/*
	 * With NFS3_2, the special cases for diskless and UFS get
	 * moved into file system specific code in dux_lockf() and ufs_lockf().
	 * in dux_vnops.c and ufs_vnops.c, respectively.  Of course, other
	 * file systems can now be add support for lockf more easily now.
	 * NOTE: It is up to the VOP routines to return a valid errno!
	 */
	u.u_error = VOP_LOCKF((struct vnode *)fp->f_data, uap->flag, 
				uap->size, fp->f_cred, fp, LB, UB);

        if (!u.u_error && (uap->flag != F_TEST) && (uap->flag != F_ULOCK)){
		char *pop = getpp(uap->fdes);
		/*
		 * If any locking is attempted and succeeded, mark file locked
		 * to force unlock on close.  Also, since the SVID
		 * specifies that the FIRST close releases all locks,
		 * mark process to reduce the search overhead in
		 * vno_lockrelease() (in kern_dscrp.c).
		 */
		*pop |= UF_FDLOCK;
		SPINLOCK(sched_lock);
		u.u_procp->p_flag2 |= SLKDONE;
		SPINUNLOCK(sched_lock);
	}

	VSEMA(&filesys_sema);
	return(-1);

}


insert_lock(ip,LB,UB,function,locktype)
struct inode *ip;
off_t LB,UB;
enum lockf_type function;
short locktype;
{
	register struct locklist *cl, *nl, *tmp, *found_one;
	int inserted = 0;

	found_one = NULL;
	nl = (struct locklist *)&ip->i_locklist;	/* note that nl starts
							as address in inode */
	cl = (struct locklist *)ip->i_locklist;
	
	do {
		if (cl == NULL) {
			/* at end of lock list, just add lock and done */
#ifdef SYSCALLTRACE
			if (lockfdbg)
				printf("insert_lock: adding lock at end of list.\n");
#endif SYSCALLTRACE
			/* first lock added; tell the other sites */
			if(lockadd(nl, LB, UB, function, locktype, ip) == 0 && !remoteip(ip)) {
				isynclock(ip);
				checksync(ip);
				isyncunlock(ip);
			}
			return;
		}
		if (is_my_lock(cl) != 0) 
			switch (find_lock_range(cl,LB,UB)) {
			case (S_BEFORE | E_BEFORE):
				/* add this lock */
#ifdef SYSCALLTRACE
		if (lockfdbg)
		    printf("lockf: adding, before existing locks\n");
#endif SYSCALLTRACE
				lockadd(nl, LB, UB, function, locktype, ip);
				return;
			case (S_BEFORE | E_START):
			case (S_BEFORE | E_MIDDLE):
				if (cl->ll_type == locktype)
					cl->ll_start = LB;
				else {
					if (lockadd(nl, LB, UB, function, locktype, ip) == 0)
						cl->ll_start = UB;
				}
				return;
			case (S_BEFORE | E_END):
				cl->ll_type = locktype;
				cl->ll_start = LB;
				return;
			case (S_BEFORE | E_AFTER):
				cl->ll_type = locktype;
				cl->ll_start = LB;
				cl->ll_end = UB;
		/* delete all the locks covered in this region -- set insert
		   to do that later */
				inserted++;
				break;
			case (S_START | E_START):
			case (S_END | E_END):
				cl->ll_type = locktype;
				return;
			case (S_START | E_MIDDLE):
				if (cl->ll_type != locktype) {
					if (lockadd(nl, LB, UB, function, locktype, ip) == 0)
						cl->ll_start = UB;
				}
				return;
			case (S_START | E_END):
				cl->ll_type = locktype;
				return;
			case (S_START | E_AFTER):
				cl->ll_type = locktype;
				cl->ll_end = UB;
				/* take care of locks in this region */
				inserted++;
				break;
			case (S_MIDDLE | E_MIDDLE):
				/* need to make sure there are at least 2
				   locks available */
				if (cl->ll_type != locktype) {
					if (lockadd(nl,cl->ll_start,LB, function, cl->ll_type, ip) == 0)
					{
						if (lockadd(nl->ll_link,LB,UB,function,locktype, ip) == 0)
							cl->ll_start = UB;
						else {
							tmp = nl->ll_link;
							nl->ll_link = cl;
							lockfree(tmp);
						}
					}
				}
				return;
			case (S_MIDDLE | E_END):
				if (cl->ll_type != locktype) {
					if (lockadd(cl, LB, UB, function, locktype, ip) == 0) {
						cl->ll_end = LB;
						cl = cl->ll_link;
					}
				}
				return;
			case (S_MIDDLE | E_AFTER):
			case (S_END | E_AFTER):
				if (cl->ll_type != locktype) {
					if (lockadd(cl, LB, UB, function, locktype, ip) == 0){
						cl->ll_end = LB;
						cl = cl->ll_link;
					}
					else
						return;
				}
				else
					cl->ll_end = UB;
				inserted ++;
				/* fix up the list - resolve locks covered in
				   this region */
				break;
			case (S_AFTER | E_AFTER):
				/* keep looking for the right position to put
				   this lock */
				found_one = cl;
				break;
			default:
#ifdef SYSCALLTRACE
		if (lockfdbg)
		    printf("default case; case is %x\n",find_lock_range(cl,LB,UB));
#endif SYSCALLTRACE
				break;

			}	/* switch */
		nl = cl;
	} while (((cl = cl->ll_link) != NULL) && (inserted == 0));

	if (inserted)
		compress_lock_list(nl,LB,UB);
	else {
#ifdef SYSCALLTRACE
		if (lockfdbg)
		    printf("Dropped out of the loop; call lockadd\n");
#endif SYSCALLTRACE
		if (found_one != NULL)
			lockadd(found_one, LB, UB, function, locktype, ip);
		else
			lockadd(nl, LB, UB, function, locktype, ip);
	}

}  /* insert_lock */


/******************************************************************
   find_lock_range sets the type of span specified by LB & UB with respect
   to the lock element pointed to by cl.  There are 5 regions:

   S_BEFORE   S_START   S_MIDDLE   S_END   S_AFTER
     010	020	  030	   040	    050
   E_BEFORE   E_START   E_MIDDLE   E_END   E_AFTER
     001	002	  003	   004	    005

   relative to the already locked section.  The type is 2 octal digits,
   the 8's digit is the start type and the 1's the end type.

   e.g.  if the relationship between the requested & existing locks are:
  		LB		UB
	---------|------|--------|-------|-------
		       start		end

	then find_lock_range will return (S_BEFORE | E_MIDDLE)

***************************************************************/
int find_lock_range(cl,LB,UB)
struct locklist *cl;
int LB,UB;
{
	int i=0;

	if (LB < cl->ll_start)
		i = S_BEFORE;
	else if (LB == cl->ll_start)
		i = S_START;
	else if (LB < cl->ll_end)
		i = S_MIDDLE;
	else if (LB == cl->ll_end)
		i = S_END;
	else	i = S_AFTER;

	if (UB < cl->ll_start)
		i |= E_BEFORE;
	else if (UB == cl->ll_start)
		i |= E_START;
	else if (UB < cl->ll_end)
		i |= E_MIDDLE;
	else if (UB == cl->ll_end)
		i |= E_END;
	else i |= E_AFTER;

	return (i);
}


compress_lock_list(cl,LB,UB)
struct locklist *cl;
off_t LB,UB;
{
	struct locklist *nl,*tmp;

	nl = cl;
	while ((cl = cl->ll_link) != NULL) {
		if ((is_my_lock(cl) != 0) && (cl->ll_start <= UB)) {
			switch (find_lock_range(cl,LB,UB)) {
			case (S_BEFORE | E_START):
			case (S_BEFORE | E_MIDDLE):
			case (S_START | E_MIDDLE):
				cl->ll_start = UB;
				nl = cl;
				break;

			/* delete lock in the following cases */
			case (S_BEFORE | E_END):
			case (S_BEFORE | E_AFTER):
			case (S_START | E_END):
			case (S_START | E_AFTER):
				tmp = cl;
				nl->ll_link = cl->ll_link;
				lockfree(tmp);
				if (nl->ll_link == NULL)
					return;
				cl = nl;
				break;
			default:
				panic("compress_lock_list");
				break;

			}  /*switch*/
		} 	   /* if */
	}	/*while*/
}  /* compress_lock_list */


delete_lock(ip, LB, UB, function)
struct	inode *ip;
off_t 	LB,UB;
enum lockf_type function;
{
	register struct locklist *cl, *nl, *tmp;

#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("delete_lock(ip 0x%x, LB 0x%x, UB 0x%x, function 0x%x)\n",ip, LB, UB, function);
#endif SYSCALLTRACE

	/*
	 * starting at list head scan
	 * for locks in the range by
	 * this process.
	 * Note addr is pointer in inode
	 */
	cl = (struct locklist *)&ip->i_locklist;
	while(nl = cl->ll_link) {
		/*
		 * if not by this process skip to next lock
		 */
		if(is_my_lock(nl) == 0) {
			cl = nl;
			continue;
		}

		/* if totally out of bound, skip to next lock */
		if ((nl->ll_start >= UB) || (LB >= nl->ll_end)) {
			cl = nl;
			if (cl != NULL)
				continue;
			else {
				return;
			}
		}
		switch (find_lock_range(nl,LB,UB)) {
		case (S_BEFORE | E_MIDDLE):
		case (S_START | E_MIDDLE):
			nl->ll_start = UB;
			check_lock_sleep(nl);
			break;
		case (S_BEFORE | E_END):
		case (S_BEFORE | E_AFTER):
		case (S_START | E_END):
		case (S_START | E_AFTER):
			tmp = nl;
			cl->ll_link = nl->ll_link;
			lockfree(tmp);
			if (cl->ll_link == NULL)
				return;
			break;
		case (S_MIDDLE | E_MIDDLE):
			/* DELETE middle section , but need one more lock
			   to keep the 2 sections still locked */
			if (lockadd(nl,UB,nl->ll_end,function,nl->ll_type, ip) == 0)  {
				nl->ll_end = LB;
				check_lock_sleep(nl);
			}
			else {
				u.u_prelock = nl;  /* indicate an error */
				return;  /* give up */
			}
			break;
		case (S_MIDDLE | E_END):
			nl->ll_end = LB;
			check_lock_sleep(nl);
			break;
		case (S_MIDDLE | E_AFTER):
			nl->ll_end = LB;
			check_lock_sleep(nl);
			/* continue looking for locks covered by upper
 			   limit of unlock range */
			cl=nl;
			break;
		default:
#ifdef SYSCALLTRACE
		if (lockfdbg)
		    printf("default case; case is %x\n",find_lock_range(cl,LB,UB));
#endif SYSCALLTRACE
			cl=nl;
		}	/* switch */
	}	/* while */


	if(!remoteip(ip) && ip->i_locklist == NULL) {	
	/*  The last lock removed, tell the other sites */
		isynclock(ip);
		checksync(ip);
		isyncunlock(ip);
	}

}  /* delete_lock */

check_lock_sleep(nl)
struct locklist *nl;
{		
			/*
			 * if some one is sleeping on this lock
			 * do a wakeup, we may free the region
			 * being slept on
			 */
			if(nl->ll_flags & IWANT) {
#ifdef SYSCALLTRACE
				if (lockfdbg)
					printf("delete_lock: waking up processes sleeping on 0x%x\n", nl);
#endif SYSCALLTRACE
				nl->ll_flags &= ~IWANT;
				wakeup(nl);
			}
			/*
			 * Check to see if we need to notify the NFS lock
			 * manager that this lock may have become free
			 * Note that all that the LM really needs to know
			 * is what file, since it will try the whole list.
			 * However, we give it as accurate information as
			 * possible here. See nfs_fcntl()  and
			 * free_lock_with_lm() in nfs_server.c
			 */
			if ( nl->ll_flags & NFS_WANTS_LOCK ) {

			    nl->ll_flags &= ~NFS_WANTS_LOCK;
			    NFSCALL(NFS_INFORMLM)( ITOV(nl->ll_ip), 
				nl->ll_start, nl->ll_end, 
      				((nl->ll_flags & L_REMOTE) ? nl->ll_pid :
							nl->ll_proc->p_pid));
			}
}


/*
 * locked -- routine to scan locks and check for a locked condition
 *           flag     = F_ULOCK =0, F_LOCK  =1, F_TLOCK  =2, F_TEST =3,
 *		        F_GETLK =5, F_SETLK =6, F_SETLKW =7
 *           function = L_LOCKF =0, L_READ  =1, L_WRITE  =2, L_COPEN =3,
 *		        L_FCNTL =4
 *           l_type   = F_RDLCK =1, F_WRLCK =1, F_UNLCK  =3
 */

locked(flag, ip, LB, UB, function, flock, l_type, fpflags)
	register struct inode *ip;
	off_t   LB, UB;
	enum lockf_type function;
	struct flock *flock;
	short l_type;
	int fpflags;
{
	int remote = 0;	/* temp hack for RFA pullout */
	register struct locklist *nl = ip->i_locklist;
	u.u_error = 0;

#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("locked(flag %d, ip 0x%x, LB 0x%x, UB 0x%x, function 0x%x, flock 0x%x, l_type 0x%x, fpflags 0x%x, remote 0x%x)\n",
		flag, ip, LB, UB, function, flock, l_type, fpflags, remote);
#endif SYSCALLTRACE

	/*
	 * scan list while locks are in requested range
	 */
	while(nl != NULL) {

		/*
		 * skip locks for this process
		 * and those out of range
		 */
		if( is_my_lock(nl) != 0 ||
		    LB >= nl->ll_end || nl->ll_start >= UB) {
			nl = nl->ll_link;
			continue;
		}

		/*
		 * must have found lock by another process
		 */
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("locked: found lock by another process\n");
#endif SYSCALLTRACE
		/* if function is copen(), then
		      if enforcement mode lock, return EAGAIN
		   Note: enforcement is on if IENFMT is set and
                         the group execute bit is cleared
	   	 */
		if (function == L_COPEN) {
		    if (   (ip->i_mode&IENFMT)
		        && ((ip->i_mode&IFMT) == IFREG)
		        && !(ip->i_mode&(IEXEC>>3)))
		    {
			u.u_error = EAGAIN;
		    }

		    return (u.u_error);
		}

		/* if function is lockf and request is to test only,
		 * set error and exit
		 */
		if(flag == F_TEST || flag == F_TLOCK) {
#ifdef SYSCALLTRACE
			if (lockfdbg)
				printf("locked: F_TEST or F_TLOCK, exiting\n");
#endif SYSCALLTRACE
			u.u_error = EACCES;
			return(1);
		}

		/* if this is fcntl call, check if lock type requested is 
		 * ok to be applied to the region found.
		 * if request is to test only, update flock struct and exit.
		 */
		if (function == L_FCNTL) {
		    if (((nl->ll_type == F_RDLCK) && (l_type == F_WRLCK))
		      || (nl->ll_type == F_WRLCK)) {
			if (flag == F_SETLK) {
				u.u_error = EACCES;
				return(u.u_error);
			}
			else if (flag == F_GETLK) {
				/* flag == F_GETLK */
				flock->l_type = nl->ll_type;
				flock->l_whence = 0; 
				flock->l_start = nl->ll_start;
				if (nl->ll_end == MAX_LOCK_SIZE)
		  			flock->l_len = 0;
				else
					flock->l_len = nl->ll_end - nl->ll_start;
      				if (nl->ll_flags & L_REMOTE)
					flock->l_pid = nl->ll_pid;
      				else
					flock->l_pid = nl->ll_proc->p_pid;
				return(1);
			}
			/* else F_SETLKW: let it pass through to block */ 
		    }
		    else
		    {
			nl = nl->ll_link;
			continue; /* look for other locks */	
		    }	
		}

		/*
		 * If this is a read call and the lock is a write lock,
		 * or if this is a write call, check for normal file
		 * type and a file mode of IENFMT to see
		 * if lock is enforced or advisory only.
		 * if there is mandatory lock on file, then
		 *	if O_NDELAY is set, the read/write function will
		 *	  return EAGAIN;
		 *	else the read/write call will sleep until the 
		 *	  blocking record lock is removed.
		 * Note: enforcement is on if IENFMT is set and the group
		 * execute bit is cleared.
		 * 
		 * if lock is advisory only, return (NULL).  It is ok to
		 *	read/write an advisory lock region.
		 */
		if (function == L_READ || function == L_WRITE)
		{
		    if ((function == L_READ) && (nl->ll_type != F_WRLCK))
		    {
			nl = nl->ll_link;
			continue; /* look for other locks */
		    }

		    if (   (ip->i_mode&IENFMT)
		        && ((ip->i_mode&IFMT) == IFREG)
		        && !(ip->i_mode&(IEXEC>>3)))
		    {
#ifdef	POSIX
			if (fpflags & (FNDELAY|FNBLOCK)) {
#else	not POSIX
			if (fpflags & FNDELAY) {
#endif	POSIX
				u.u_error = EAGAIN;
				return(u.u_error);
			}
			else
			{
#ifdef SYSCALLTRACE
				if (lockfdbg)
					printf("locked: sleep - enf O_NDELAY not set\n");
#endif SYSCALLTRACE
			}
		    }
		    else {
#ifdef SYSCALLTRACE
				if (lockfdbg)
					printf("locked: non enf mode - return 0 from locked\n");
#endif SYSCALLTRACE
				nl = nl->ll_link;
				continue; /* look for other locks */
		    }
		}		/* not read or write function */

		/*
		 * otherwise, if this is the lockf system call or fcntl 
		 * (F_SETLKW) then go ahead and sleep.
		 */
		else if ( flag != F_LOCK && flag != F_SETLKW 
			&& flag != F_SETLK_NFSCALLBACK ) {
			nl = nl->ll_link;
			continue; /* look for other locks */
		}

		/*
		 * will need to sleep on lock, check for deadlock first
		 * abort on error
		 */

		/*  Set up p_dlchan for the distributed deadlock hunt	*/
		u.u_procp->p_dlchan = (char *) nl;
		lockinc(nl);
		if(deadlock(nl) != NULL) {
			lockdec(nl);
			u.u_procp->p_dlchan = NULL;
			return(1);
		}

		/*
		 * Check if the lock has been freed while
		 * this process was checking deadlock.  If
		 * not, post want flag to get awoken then
		 * sleep till lock is released.
		 */
  		if (nl->ll_flags & ILBUSY) { 
			/*
			 * F_SETLK_NFSCALLBACK says that it should be a blocking
			 * lock, but the caller doesn't want to block, so
			 * instead we make note of it want will call him 
			 * back when the lock becomes free so we can
			 * wake him up.  This is added to support the NFS 3.2
			 * lock manager.  See nfs_fcntl() in nfs_server.c
			 * Note that we intentionally wait until AFTER the
			 * deadlock check because this is still in effect
			 * a blocking request.
			 */
			if (flag == F_SETLK_NFSCALLBACK) {
				lockdec(nl);
				u.u_procp->p_dlchan = NULL;
				u.u_error = EACCES;
				nl->ll_flags |= NFS_WANTS_LOCK;
				return(u.u_error);
			}
			nl->ll_flags |= IWANT;

			/* Guard against an interrupted system call.  
			 * We setjmp() and then sleep.  If we are 
			 * interrupted we will return here, cleanup 
			 * and set EINTR.
			 */
			if (setjmp(&u.u_qsave)) {
				lockdec(nl);
				u.u_procp->p_dlchan = NULL;
				u.u_error = EINTR;
				PSEMA(&filesys_sema);
				return (u.u_error);
			}
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("locked: going to sleep on 0x%x\n", nl);
#endif SYSCALLTRACE
			sleep( (caddr_t)nl, PSLEP);
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("locked: wakeup on lock 0x%x\n", nl);
#endif SYSCALLTRACE
  		} 

		/*
		 * set scan back to begining to catch
		 * any new areas locked
		 * or a partial delete
		 */
		/*  Reset the dlchan before hunting again */
		lockdec(nl);
		u.u_procp->p_dlchan = NULL;
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("locked: starting scan again\n");
#endif SYSCALLTRACE
		nl = ip->i_locklist;
		/*
		 * abort if any errors
		 */
		if(u.u_error)
			return(1);

	}  /* while loop  */

	return(0); /* no blocking locks were found */

}  /* locked */


/*
 * unlock -- called by close to release all locks for this process
 */

unlock(ip)
register struct inode *ip;
{
	register struct locklist *nl;
	register struct locklist *cl;
	register int ret_code=0;

#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("unlock(ip 0x%x)\n", ip);
#endif SYSCALLTRACE

	if((cl = (struct locklist *)&ip->i_locklist)->ll_link) {
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("unlock(ip 0x%x, cl 0x%x)\n", ip, cl);
#endif SYSCALLTRACE
		while( (nl = cl->ll_link) != NULL) {
			if(is_my_lock(nl) != 0) {
				cl->ll_link = nl->ll_link;
				lockfree(nl);
				ret_code = 1;
			}
			else cl = nl;
		}
	}

	return(ret_code);
}

/*
 * lockalloc -- allocates free list, returns free lock items
 */
struct locklist *
lockalloc(function)
enum lockf_type function;
{
	register struct locklist *fl = &locklist[0];
	register struct locklist *nl;

#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("lockalloc()\n");
#endif SYSCALLTRACE
	/*
	 * if first entry has never been used
	 * link the locklist table into the freelist
	 */
	if(fl->ll_proc == NULL) {
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("lockalloc: initializing locklist\n");
#endif SYSCALLTRACE
		fl->ll_proc = &proc[0];
		for(nl= &locklist[1]; nl < &locklist[nflocks]; nl++) {
			/*
			 * Make sure we don't have a bad info to start with;
			 */
			nl->ll_flags = 0;
			nl->ll_count = 1;	/*lockfree will decrement*/
			lockfree(nl);
		}
	}
	/*
	 * if all the locks are used error exit
	 */
	if( (nl=fl->ll_link) == NULL) {
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("lockalloc: out of locks\n");
#endif SYSCALLTRACE
		if (function == L_LOCKF)
			u.u_error = EDEADLK;
		else
			u.u_error = ENOLCK;	
		return(NULL);
	}
	/*
	 * return the next lock on the list
	 */
	fl->ll_link = nl->ll_link;
	nl->ll_link = NULL;
	VASSERT(nl->ll_count == 0);
	nl->ll_count = 1;
	return(nl);
}

/*
 * lockfree -- returns a lock item to the free list
 */

lockfree(lp)
register struct locklist *lp;
{
#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("lockfree(lp 0x%x)\n", lp);
#endif SYSCALLTRACE
	/*
	 * if some process is sleeping on this lock
	 * wake them up
	 */
	if(lp->ll_flags & IWANT) {
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("lockfree: waking up processes\n");
#endif SYSCALLTRACE
		lp->ll_flags &= ~IWANT;
		wakeup(lp);
	}
	/*
	 * Check to see if we need to notify the NFS lock
	 * manager that this lock may have become free
	 * Note that all that the LM really needs to know
	 * is what file, since it will try the whole list.
	 * However, we give it as accurate information as
	 * possible here. See nfs_fcntl()  and
	 * free_lock_with_lm() in nfs_server.c
	 */
	if ( lp->ll_flags & NFS_WANTS_LOCK ) {
		lp->ll_flags &= ~NFS_WANTS_LOCK;
		NFSCALL(NFS_INFORMLM)( ITOV(lp->ll_ip), 
				lp->ll_start, lp->ll_end,
      				((lp->ll_flags & L_REMOTE) ? lp->ll_pid :
							lp->ll_proc->p_pid));
	}
	lp->ll_ip = NULL;
	/*
	 * add the lock into the free list
	 */
	lp->ll_flags &= ~L_REMOTE;   /* Turn off the remote flag */
  	lp->ll_flags &= ~ILBUSY;     /* Turn off the lock busy flag */
	lp->ll_proc = &proc[0];	     /* clear the lock pointer */
	lockdec(lp);
}

/*
 * lockadd -- routine to add item to list
 */
lockadd(cl,LB,UB,function,locktype,ip)
register struct locklist *cl;
off_t LB,UB;
enum lockf_type function;
short locktype;
struct inode *ip;
{
	register struct locklist *nl;

#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("lockadd(cl 0x%x, LB 0x%x, UB 0x%x)\n", cl, LB, UB);
#endif SYSCALLTRACE
	/*
	 * get a lock, return if none available
	 */
	nl = lockalloc(function);
	if(nl == NULL) {
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf("lockadd: no locks available\n");
#endif SYSCALLTRACE
		return(1);
	}

	/*
	 * link the new entry into list at current spot
	 * fill in the data from the args
	 */
	nl->ll_link = cl->ll_link;
	cl->ll_link = nl;
	if(u.u_nsp != NULL) {
	    nl->ll_flags |= L_REMOTE;
	    nl->ll_pid    = u.u_nsp->nsp_pid;
	    nl->ll_psite  = u.u_nsp->nsp_site;
#ifdef SYSCALLTRACE
	if (lockfdbg)
	printf("lockadd: remote lock (lpid 0x%x, lsite 0x%x)\n", 
		nl->ll_pid,  nl->ll_psite);
#endif SYSCALLTRACE
	}
	else
	    nl->ll_proc = u.u_procp;

	nl->ll_start = LB;
	nl->ll_end = UB;
	nl->ll_type = locktype;
   	nl->ll_flags |= ILBUSY;     /* set the lock busy flag */
	nl->ll_ip = ip;
	return(0);
}

lockinc(lp)
register struct locklist *lp;
{
	lp->ll_count++;
}

lockdec(lp)
register struct locklist *lp;
{
	register struct locklist *fl;
	
	VASSERT(lp->ll_count > 0);
	if (--(lp->ll_count) == 0)
	{
		fl = &locklist[0];

		lp->ll_link = fl->ll_link;   /* add at the head of the list */
		fl->ll_link = lp;
	}
}

deadlock(lp)
  register struct locklist *lp;		/*  Blocking lock		*/
{ 
	register short  Rpid;		/*  The requesting process	*/
	register site_t Rsite;		/*  The requesting site		*/
	short  pid;			/*  pid of current process	*/
	site_t site;			/*  site of current process	*/
	site_t faddr;			/*  A forwarding address	*/
	extern site_t my_site;
	struct proc *pfind();
	short  ret_code;
	struct locklist *dd_procstat();
	extern struct locklist *dd_nspstat();
#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("deadlock(lp 0x%x)\n", lp);
#endif SYSCALLTRACE

    	/* Save the requestor's pid and site */
    	if(u.u_nsp == NULL) {
      		Rpid  = u.u_procp->p_pid;
		Rsite = my_site;
      	}
    	else {
		Rpid  = u.u_nsp->nsp_pid;
		Rsite = u.u_nsp->nsp_site;
      	}

  local_lock:
    	if(lp->ll_flags & L_REMOTE) { 
		pid  = lp->ll_pid;
		site = lp->ll_psite;
      	}
    	else { 
		pid  = lp->ll_proc->p_pid;
		site = my_site;
      	}

  next_loop:
    	/*  Since the locked() function will never call us with our own lock,
     	 *  there is a deadlock if pid and site ever identify the process that
     	 *  made the request.
     	 */

    	if(pid == Rpid && site == Rsite) {
		 lp = (struct locklist *) u.u_procp->p_dlchan;
#ifdef SYSCALLTRACE
	if (lockfdbg) {
		printf("deadlock: deadlock found\n");
		printf("deadlock: u.u_procp is 0x%x; p_dlchan is 0x%x,lock owner is 0x%x\n", 
			u.u_procp, u.u_procp->p_dlchan, lp->ll_proc);
	}
#endif SYSCALLTRACE
		u.u_error = EDEADLK;
		return((int) lp);
      	}
    
    	/* local current lock holder */  
    	if(site == my_site) {
      		if((lp = dd_procstat(pfind(pid), &faddr)) == NULL) { 
			SPINUNLOCK(sched_lock); /* pfind locks it */
			/* no lock held by this process, check the 
			 * forwarding address 
			 */
			if(faddr != 0)
				goto forwarding; 
#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("deadlock: no deadlock found\n");
#endif SYSCALLTRACE
	  		return(NULL);
		}
      		else {
         		/* process the holder of the lock, A is sleeping on; 
		 	 * if B -> deadlock
		 	 */
			SPINUNLOCK(sched_lock); /* pfind, in if, locks it */
	 		goto local_lock;
      		}
	} /* local lock holder */
    	
	else {  /* remote current lock holder */  
      		ret_code = Rdd_procstat(&pid, &site, &faddr); 
      		if(ret_code == 0) {
#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("deadlock: no deadlock found\n", lp);
#endif SYSCALLTRACE
	  		return(NULL);
		}
      		else
			if(faddr == 0)
	  			goto next_loop;
	}  /* remote lock holder */

  forwarding:
    	if(faddr == my_site) { 
		if((lp = dd_nspstat(pid, site)) != NULL)
			goto local_lock;
#ifdef SYSCALLTRACE
	if (lockfdbg)
		printf("deadlock: no deadlock found\n", lp);
#endif SYSCALLTRACE
		return(NULL);
      	}
    	else {
      		ret_code = Rdd_nspstat(&pid, &site, faddr); 
      		if(ret_code != 0)
	  		goto next_loop;
      		return(NULL);
     	}
}  /* deadlock */



/*  Find out the status of a process WRT locks.  Is the current holder
 *  deadlock hunting?  If the return value is not NULL, it is the 
 *  address of a lock.  Otherwise, the variable faddr will be set to 
 *  the forwarding address if the follow flag is set
 */

struct locklist *
dd_procstat(procp, faddr)
  register struct proc *procp;		/*  A pointer to the process	*/
  site_t *faddr;			/*  A slot to return the FA	*/
  /* p_dlchan is the process deadlock channel. It holds the address of a
   * lock this process will sleep on, if deadlock is not encountered, or
   * is already sleeping on, if deadlock has not been encountered.
   */
  { if(procp->p_dlchan == NULL)
      *faddr = procp->p_faddr;
    else
      *faddr = 0;

    return((struct locklist *) procp->p_dlchan);
  }



/*
 * check_for_locks(ip, pid) -- returns true or false depending on
 * whether the given pid has any locks currently on the given inode.
 * This is very similar to the Dux routine is_my_lock() (see above)
 * but checks for the given pid instead of for the current pid.
 */

int
check_for_locks(ip, pid)
struct inode *ip;
short pid;
{
	struct locklist *lp;

	for ( lp = ip->i_locklist; lp != NULL ; lp = lp->ll_link ) {
		if ( (u.u_nsp != NULL) ) {
		    if ( ((lp->ll_flags & L_REMOTE) != 0) &&
			(u.u_nsp->nsp_site == lp->ll_psite) &&
			(u.u_nsp->nsp_pid ==  pid ) )
				return(1);
			else
				continue;
		}
		/* ll_proc is a union of proc and site+cnode_pid
		   therefore, only if it's local (NON DUX) it 
		   represent a proc pointer
		*/
		if ((lp->ll_flags & L_REMOTE) == 0)
		if ( pid == lp->ll_proc->p_pid )
			return (1);
	}
	return(0);
}



/*  is_my_lock() summarizes the convoluted logic need to determine if a
 *  lock is owned either by the current process or the remote process this
 *  NSP is serving.
 *  Note: a lock can be remote and the u.u_nsp == NULL -> the server is 
 *        processing an existing client lock.  
 */

int
is_my_lock(lp)
  register struct locklist *lp;
  { if(u.u_nsp == NULL)
      if((lp->ll_flags & L_REMOTE) == 0 && lp->ll_proc == u.u_procp)
	return(1);
      else
	return(0);

    if((lp->ll_flags & L_REMOTE) && u.u_nsp->nsp_pid == lp->ll_pid && u.u_nsp->nsp_site == lp->ll_psite)
	return(1);
      else
	return(0);
  }


/*
 * check_for_nfs_callbacks(ip) -- returns true or false depending on
 * whether any of the locks currently on the given inode are marked
 * that NFS wants them.
 */

int
check_for_nfs_callbacks(ip, pid)
struct inode *ip;
short pid;
{
	struct locklist *lp;

	for ( lp = ip->i_locklist; lp != NULL ; lp = lp->ll_link ) {
		if ( lp->ll_flags & NFS_WANTS_LOCK ) {
			return (1);
		}
	}
	return(0);
}
