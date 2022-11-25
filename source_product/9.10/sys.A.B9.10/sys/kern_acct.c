/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_acct.c,v $
 * $Revision: 1.25.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:05:04 $
 */

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
#ifdef hp9000s800
#include "../h/proc.h"
#endif /* hp9000s800 */
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../ufs/inode.h"
#include "../h/kernel.h"
#include "../h/acct.h"
#include "../h/uio.h"
#include "../h/proc.h"
#include "../h/kern_sem.h"

#include "../h/file.h"

/*
 * SHOULD REPLACE THIS WITH A DRIVER THAT CAN BE READ TO SIMPLIFY.
 */
struct 	vnode *acctp;
struct 	vnode *savacctp;

#ifdef	MP
sysacct()
{
	PSEMA(&filesys_sema);
	_sysacct();
	VSEMA(&filesys_sema);
}
_sysacct()	
/*
 * Perform process accounting functions.
 */
#else
sysacct()
#endif MP
{
	struct vnode *vp;
	register struct a {
		char	*fname;
	} *uap = (struct a *)u.u_ap;
	static aclock = 0;
#define	DONE	goto unlock

	if (suser()) {
		if (aclock) {
			u.u_error = EBUSY;
			return;
		}
		aclock++;
		if (savacctp)
			vp = savacctp;
		else
			vp = acctp;
		if (uap->fname==NULL) {
			if (vp) {
				/* must lock inode because someone
				 * else could have this inode locked
				 * at this moment.
				 */
/* bug fixes: the acctp should be set to NULL before ip is released */
				acctp = NULL;
				savacctp = NULL;
				VOP_CLOSE(vp, FWRITE, u.u_cred);
				VN_RELE(vp);
			}
			DONE;
		}

		if (vp) {		/*  enabling when already enabled? */
			u.u_error = EBUSY;
			DONE;
		}
		/*
		 * The call to vn_open() does a number of necessary
		 * things including setting up DUX synchronization and
		 * checking for specific errors like ETXTBUSY and EROFS.
		 * Unfortunately it also attempts to invoke the driver
		 * or fifo code if the file specified is a special file
		 * (an error condition only - and a rather unlikely one
		 * given that this call is generally only made by the
		 * well-behaved accounting commands.  We set the FNDELAY
		 * flag to avoid blocking on an open of a device file or
		 * fifo.  It is possible that this call will cause
		 * side-effects (eg.  tty affiliation) different from
		 * the System V behavior in such an error case.  It
		 * would be possible to precisely duplicate the System V
		 * behavior by setting up a sepatate entry in the
		 * lookup_ops table and duplicating much of the open and
		 * close code.  The ROI of doing so appears minimal.  We
		 * also must translate a few specific errors from open
		 * attempts on special files.
		 */
#ifdef hp9000s800
		/*
		 * We set u.u_r.r_val1 to -1 so that do_open_network_kludge()
		 * doesn't mangle a file ptr (most likely file[0]).
		 */
		u.u_r.r_val1 = -1;
#endif /* hp9000s800 */
		u.u_error = vn_open(uap->fname, UIOSEG_USER,
				    FWRITE|FNDELAY, 0, &vp);
#ifdef hp9000s800
		/*
		 * Reset u.u_r.r_val1.
		 */
		u.u_r.r_val1 = 0;
#endif /* hp9000s800 */

		/*
		 * Translate the following errors that could be caused
		 * by special files to the System V error
		 */
		switch (u.u_error) {
		case EIO:
		case ENXIO:
		case EAGAIN:
		case EBUSY:
		case ENODEV:
		case EISDIR:
		case EINVAL:
			u.u_error = EACCES;
			break;
		}

		if (u.u_error)
			DONE;
		if(vp->v_type != VREG) {
			u.u_error = EACCES;
			VN_RELE(vp);
			VOP_CLOSE(vp, FWRITE, u.u_cred);
			DONE;
		}


		acctp = vp;
unlock:		aclock = 0;
	}
}

/* stop accounting when < acctsuspend % free space left */
extern	int acctsuspend;
/* resume when free space risen to > acctresume % */
extern	int acctresume;

struct	acct acctbuf;
#ifdef	MP
acct(exit_status)
int exit_status;
{
	sv_sema_t acctSS;
	/*	Called only from exit(); */
	PXSEMA(&filesys_sema, &acctSS);
	_acct(exit_status);
	VXSEMA(&filesys_sema, &acctSS);
}
_acct(exit_status)

/*
 * On exit, write a record on the accounting file.
 */
#else
acct(exit_status)
#endif MP
int exit_status;
{
	register int i;
	register struct vnode *vp;
	struct statfs sb;
	register struct acct *ap = &acctbuf;

	int save_limit;
#ifdef FSS
	extern int fss_state;
#endif

checksavacctp:
	if (savacctp) {
		/*
		 * Make sure the vnode doesn't go away (due to a call to
		 * sysacct()) or that someone else doesn't resume accounting
		 * (in this same code) while we're trying to do the STATFS.
		 * If that happens, we have to start over.
		 */
		VN_HOLD(vp = savacctp);
		(void)VFS_STATFS(vp->v_vfsp, &sb);
		VN_RELE(vp);
		if (vp != savacctp)
			goto checksavacctp;
		if (sb.f_bavail > (acctresume * sb.f_blocks / 100)) {
			acctp = savacctp;
			savacctp = NULL;
			printf("Accounting resumed\n");
		}
	}
checkacctp:

	if ((vp = acctp) == NULL)
		return;
	/*
	 * Make sure the vnode doesn't go away (due to a call to
	 * sysacct()) while we're trying to statfs or write to it, or
	 * that someone else doesn't suspend accounting (in this same
	 * code) while we're trying to do the STATFS.
	 */
	VN_HOLD(vp);

	(void)VFS_STATFS(vp->v_vfsp, &sb);
	if (vp != acctp) {
		VN_RELE(vp);
		goto checkacctp;
	}
	if (sb.f_bavail <= (acctsuspend * sb.f_blocks / 100)) {
		savacctp = acctp;
		acctp = NULL;
		VN_RELE(vp);
		printf("Accounting suspended\n");
		return;
	}

	for (i = 0; i < sizeof (ap->ac_comm); i++)
		ap->ac_comm[i] = u.u_comm[i];
#ifdef hp9000s800
	/* time quantities are in ticks (not seconds) */
	ap->ac_utime = compress((long)tvtoticks(&u.u_procp->p_utime));
	ap->ac_stime = compress((long)tvtoticks(&u.u_procp->p_stime));
#else /* __hp9000s300 */
	ap->ac_utime = compress(u.u_procp->p_uticks);
	ap->ac_stime = compress(u.u_procp->p_sticks);
#endif /* __hp9000s300 */
	ap->ac_etime = compress(lbolt - u.u_ticks);
	ap->ac_btime = u.u_procp->p_start;
	ap->ac_uid = u.u_ruid;
	ap->ac_gid = u.u_rgid;
	ap->ac_mem =
		compress(u.u_ru.ru_ixrss + u.u_ru.ru_idrss + u.u_ru.ru_isrss);
	ap->ac_io = compress((long)u.u_ru.ru_ioch);
	ap->ac_rw = compress((long)(u.u_ru.ru_inblock + u.u_ru.ru_oublock));
	ap->ac_stat = exit_status;
	if (u.u_procp->p_ttyp)
		ap->ac_tty = u.u_procp->p_ttyd;
	else
		ap->ac_tty = NODEV;
	ap->ac_flag = u.u_acflag;
#ifdef FSS
#if (MIN_FSID < 0) || (MAX_FSID > 15)
		ERROR!  ERROR!  ERROR!  ERROR!  ERROR!  ERROR!
		The AT&T fair share scheduler puts the fair share
		group ID in the middle 4 bits (ugh) of the flag in
		the accounting record.  In order to fit without
		bashing the other bits in the flag the fs_id field
		must fit in 4 bits.  For some reason, this is no
		longer the case.  Fix it.
#endif
	ap->ac_flag |= u.u_procp->p_fss->fs_id << 2;
#endif /* FSS */
	/* System V code checks for write errors, and if any occur,
	 * truncates the file back to its size from before the write.
	 * Various differences in the BSD, VFS, and DUX system make
	 * this difficult, if not impossible, to do reliably.  The
	 * truncation is of limited value anyhow, because:
	 *	- I/O errors are not usually caught due to buffering
	 *	- various other mechanisms generally prevent hitting
	 *	  full filesys or file too big
	 *	- the accounting commands ignore a partial record
	 *	  at the end of the file
	 */

	/* limit of 5000 blocks is compatible with Sys V accounting */
	save_limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = 5000;
	(void) vn_rdwr(UIO_WRITE, vp, (caddr_t)ap, sizeof(acctbuf), 0,
		UIOSEG_KERNEL, IO_UNIT|IO_APPEND, (int *)0, 0);
	VN_RELE(vp);
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = save_limit;
}

/*
 * Produce a pseudo-floating point representation
 * with 3 bits base-8 exponent, 13 bits fraction.
 */
compress(t)
	register long t;
{
	register exp = 0, round = 0;

	while (t >= 8192) {
		exp++;
		round = t&04;
		t >>= 3;
	}
	if (round) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return ((exp<<13) + t);
}
