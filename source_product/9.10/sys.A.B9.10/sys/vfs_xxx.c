/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_xxx.c,v $
 * $Revision: 1.6.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/15 12:19:28 $
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

/*	@(#)vfs_xxx.c 1.1 85/05/30 SMI; from UCB 4.7 83/06/21	*/

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../ufs/inode.h"
#include "../h/kernel.h"
#include "../h/kern_sem.h"

#ifdef __hp9000s300	/* Compatibility routines. */

#include "../dux/lookupops.h"

/*
 * Oh, how backwards compatibility is ugly!!!
 */

#define dev_t short
#define translate_dev_t(dev)   ((major(dev) << 8) + minor(dev))


struct	ostat {
	dev_t	ost_dev;
	u_short	ost_ino;
	u_short ost_mode;
	short  	ost_nlink;
	short  	ost_uid;
	short  	ost_gid;
	dev_t	ost_rdev;
	int	ost_size;
	int	ost_atime;
	int	ost_mtime;
	int	ost_ctime;
};

/*
 * The old fstat system call.
 */
ofstat()
{
	register struct a {
		int	fd;
		struct ostat *sb;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	extern struct file *getinode();
	int type;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	type = fp->f_type;
	if (type != DTYPE_VNODE) {
		u.u_error = EINVAL;
		return;
	}

	u.u_error = ostat1((struct vnode *)fp->f_data, uap->sb);
}

/*
 * Old stat system call.  This version follows links.
 */
ostat()
{
	struct vnode *vp;
	register struct a {
		char	*fname;
		struct ostat *sb;
	} *uap;

	uap = (struct a *)u.u_ap;
	/* This function could be more efficient for the remote case by
	 * use of an ostat vnode operation, but since this is presumably
	 * a little used compatibility routine, why bother.
	 */
	u.u_error =
	    lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_LOOKUP);
	if (u.u_error)
		return;
	u.u_error = ostat1(vp, uap->sb);
	update_duxref(vp, -1, 0);
	VN_RELE(vp);
}

int
ostat1(vp, ub)
	register struct vnode *vp;
	struct ostat *ub;
{
	struct ostat ds;
	struct vattr vattr;
	register int error;

	error = VOP_GETATTR(vp, &vattr, u.u_cred, VSYNC);
	if (error)
		return(error);
	/*
	 * Copy from inode table
	 */
	ds.ost_dev = (dev_t)translate_dev_t(vattr.va_fsid);
	ds.ost_ino = (short)vattr.va_nodeid;
	ds.ost_mode = (u_short)vattr.va_mode;
	ds.ost_nlink = vattr.va_nlink;
	ds.ost_uid = (short)vattr.va_uid;
	ds.ost_gid = (short)vattr.va_gid;
	ds.ost_rdev = (dev_t)translate_dev_t(vattr.va_rdev);
	ds.ost_size = (int)vattr.va_size;
	ds.ost_atime = (int)vattr.va_atime.tv_sec;
	ds.ost_mtime = (int)vattr.va_mtime.tv_sec;
	ds.ost_ctime = (int)vattr.va_atime.tv_sec;
	return (copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)));
}

#endif /* hp9000s200 */

/*
 * Set IUPD and IACC times on file.
 * Can't set ICHG.
 */
outime()
{
	register struct a {
		char	*fname;
		time_t	*tptr;
	} *uap = (struct a *)u.u_ap;
	struct vattr vattr;
	time_t tv[2];

	if (uap->tptr != NULL) {
		u.u_error = copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof (tv));
		if (u.u_error)
			return;
	}
	vattr_null(&vattr);
	if (uap->tptr == NULL) {
		vattr.va_atime.tv_sec = time.tv_sec;
		vattr.va_atime.tv_usec = 0;
		vattr.va_mtime.tv_sec = time.tv_sec;
		vattr.va_mtime.tv_usec = 0;
	} else {
	vattr.va_atime.tv_sec = tv[0];
	vattr.va_atime.tv_usec = 0;
	vattr.va_mtime.tv_sec = tv[1];
	vattr.va_mtime.tv_usec = 0;
	}

	PSEMA(&filesys_sema);
	/* THIS IS A PROBLEM !!!! */
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr, 
	   uap->tptr == NULL);
	VSEMA(&filesys_sema);
}


#ifdef __hp9000s300 /* Backward compatibility on S300 */

/* give an error on attempts by old object files to read directories */
oread()
{
	struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	};
	register struct file *fp;
	struct vattr vattr;
	
	fp = getf(((struct a *)u.u_ap)->fdes);
	if (fp && fp->f_type == DTYPE_VNODE) {
		VOP_GETATTR((struct vnode*)fp->f_data, &vattr, u.u_cred,VASYNC);
		if (vattr.va_type == VDIR) {
			nosys();
		} else
			read();
	} else
		read();
}
#endif /* hp9000s200 */
