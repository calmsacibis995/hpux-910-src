/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_lookup.c,v $
 * $Revision: 1.16.83.7 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/03/09 15:44:19 $
 */

/* HPUX_ID: @(#)vfs_lookup.c	55.1		88/12/23 */

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

/*	@(#)vfs_lookup.c 1.1 86/02/03 SMI       */
/*      NFSSRC @(#)vfs_lookup.c	2.1 86/04/15 */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/dir.h"
#include "../h/pathname.h"
#ifdef SAR
#include "../h/sar.h"
#endif 
#include "../dux/duxparam.h"

#ifdef AUDIT
#include "../h/audit.h"			/* auditing support */
#include "../h/acl.h"			/* auditing support */
struct audit_filename *get_pn_info();	/* auditing support */
#include "../dux/dux_dev.h"
#include "../dux/lookupops.h"
extern site_t my_site;
#endif AUDIT

#include "../h/kern_sem.h"

extern int strcmp();
extern int schar();

#ifdef FSD_KI
#include "../h/ki_calls.h"
#include "../h/ki.h"
#endif /* FSD_KI */


/*
 * lookup the user file name,
 * Handle allocation and freeing of pathname buffer, return error.
 */
lookupname(fnamep, seg, followlink, dirvpp, compvpp, opcode, dependent)
	char *fnamep;			/* user pathname */
	int seg;			/* addr space that name is in */
	enum symfollow followlink;	/* follow sym links */
	struct vnode **dirvpp;		/* ret for ptr to parent dir vnode */
	struct vnode **compvpp;		/* ret for ptr to component vnode */
	int opcode;			/* function being performed */
	caddr_t dependent;		/* opcode dependent structure */
{
	struct pathname lookpn;
	register int error;

	KPREEMPTPOINT();

	error = pn_get(fnamep, seg, &lookpn);

	KPREEMPTPOINT();
	if (error)
		return (error);

	error = lookuppn(&lookpn, followlink, dirvpp, compvpp,
		opcode, dependent);
	pn_free(&lookpn);
	return (error);
}


#ifdef FILE_TRACE
int (*log_path)() = NULL;   /*  will be set by strace_link if configed in  */
#endif

	
/*
 * Starting at current directory, translate pathname pnp to end.
 * Leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 *
 * This is the central routine in pathname translation and handles
 * multiple components in pathnames, separating them at /'s.  It also
 * implements mounted file systems and processes symbolic links.
 */
lookuppn(pnp, followlink, dirvpp, compvpp, opcode, dependent)
	register struct pathname *pnp;		/* pathaname to lookup */
	enum symfollow followlink;		/* (don't) follow sym links */
	struct vnode **dirvpp;			/* ptr for parent vnode */
	struct vnode **compvpp;			/* ptr for entry vnode */
	int opcode;				/* function being performed */
	caddr_t dependent;			/* opcode dependent structure */
{
	struct vnode *vp;			/* current directory vp */
	struct vnode *vp2;			/* current directory vp */
	int nlink = 0;
	register int error;
	sv_sema_t ss;
#ifdef AUDIT
	struct audit_filename *pathptr = (struct audit_filename *)0;
	int secondcall = 0;
	int saverror;
	int savenlink;
	struct vnode *savedirvpp;
	struct vnode *savecompvpp;
#ifdef	FSD_KI
	struct timeval	starttime;

	/* stamp bp with start time */
	KI_getprectime(&starttime);
#endif /* FSD_KI */

	if (AUDITEVERON()) {
		pathptr = get_pn_info(pnp);
	}
#endif AUDIT


	/*
	 * start at current directory.
	 */
	vp = u.u_cdir;
	VN_HOLD(vp);
	PXSEMA(&filesys_sema, &ss);
	rootcheck(pnp,&vp);
again:
	error = locallookuppn(pnp, followlink, dirvpp, compvpp, &vp, &nlink);
	if (error == EPATHREMOTE)
	{
#ifdef	FSD_KI
		char ki_savepath[KI_MAXPATH];
		if (ki_kernelenable[KI_LOCALLOOKUPPN])
		{
			strncpy(ki_savepath, pnp->pn_buf, KI_MAXPATH);
		} else
		{
			ki_savepath[0] = '\0';
		}
#endif /* FSD_KI */
		/* vp is ONLY guaranteed to be valid from locallookuppn
		 * if error is EPATHREMOTE
		 */
	        vp2 = vp;
	        VN_HOLD(vp2);
		do
		{
			pn_skipslash(pnp);
			error = VOP_PATHSEND(&vp,pnp,followlink,&nlink,
				dirvpp, compvpp, opcode,dependent);
#ifdef AUDIT
			/* check to see if this information needs to be saved */
			if (pathptr != NULL)  /* auditing is on */
			{
			/* UGLY kludge -- if we are on a diskless node
		 	 * and have sent over a lookup op, then the vnode
		 	 * info does not exist on this node at all. Rather
		 	 * than penalize all lookup ops whether auditing is
		 	 * on or not and because the decision on what to
		 	 * return is made by each lookup op, we will send
		 	 * another lookup request and have the information
		 	 * returned. If the error was EOPCOMPLETE or one of
		 	 * the opcodes given below we know we do not have a
		 	 * valid vnode.
		 	 */
	
			if (error == EOPCOMPLETE && opcode != LKUP_LOOKUP
				&& opcode != LKUP_OPEN && opcode != LKUP_CREATE
				&& opcode != LKUP_EXEC && opcode != LKUP_NORECORD)
			  {
				secondcall = 1;
				saverror = error;
				savenlink = nlink;
				if (dirvpp) {
					savedirvpp = *dirvpp;
				}
				if (compvpp) {
					savecompvpp = *compvpp;
				}
				VN_HOLD(vp);
				error = VOP_PATHSEND(&vp,pnp,followlink,&nlink,
					dirvpp, compvpp, LKUP_LOOKUP, 0);
				nlink = savenlink;
			  }
			}
#endif AUDIT
		}
		while (error == EPATHREMOTE && vp && pathisremote(vp));
		/* Note, that EPATHREMOTE was relative to the serving site
		 * Thus, if we fall through to here, but the error is
		 * EPATHREMOTE, we must actually continue the search
		 * locally
		 */
		VN_RELE(vp2);
		if (error == EPATHREMOTE) {
			goto again;
		}
#ifdef	FSD_KI
		KI_locallookuppn(ki_savepath, 
                	(compvpp) ? *compvpp : NULL,  /* dont dereference NULL pointer */
			error, 
			&starttime);
#endif /* FSD_KI */
	}
#ifdef AUDIT
	if ((pathptr != NULL) && AUDITEVERON() && (error == 0 || error == EOPCOMPLETE))
	{
	   if (compvpp && *compvpp != (struct vnode *)0 && *compvpp != (struct vnode *)-1)
		save_vp_info(*compvpp,pathptr);
	   else if (dirvpp && *dirvpp != (struct vnode *)0 && *dirvpp != (struct vnode *)-1)
		save_vp_info(*dirvpp,pathptr);
	   if (secondcall) {
		if (compvpp) {
			if (*compvpp) {
				update_duxref(*compvpp,-1,0);
				VN_RELE(*compvpp);
			}
			*compvpp = savecompvpp;
		}
		if (dirvpp) {
			if (*dirvpp) {
				update_duxref(*dirvpp,-1,0);
				VN_RELE(*dirvpp);
			}
			*dirvpp = savedirvpp;
		}
		error = saverror;
	   }
	}
#endif AUDIT
#ifdef FILE_TRACE	   
	if (compvpp && *compvpp != (struct vnode *)0 && *compvpp != (struct vnode *)-1)
	    if (log_path && !error && ((*compvpp)->v_flag & VTRACE))
		(*log_path)(u.u_procp->p_pid, *compvpp, pnp->pn_path);
#endif
	VXSEMA(&filesys_sema, &ss);
	return (error);
}

locallookuppn(pnp, followlink, dirvpp, compvpp, vpp, nlinkp)
	register struct pathname *pnp;		/* pathaname to lookup */
	enum symfollow followlink;		/* (don't) follow sym links */
	struct vnode **dirvpp;			/* ptr for parent vnode */
	struct vnode **compvpp;			/* ptr for entry vnode */
	struct vnode **vpp;			/* pointer for current vnode */
	int *nlinkp;				/* symlink count */
{
	struct vnode *vp;			/* current directory vp */
	struct vnode *cvp;			/* current component vp */
	struct vnode *tvp;			/* non-reg temp ptr */
	register struct vfs *vfsp;		/* ptr to vfs for mount indir */
	char component[MAXNAMLEN+1];		/* buffer for component */
	register int error;
	register int nlink;
	label_t lqsave;
	int     parent;
#ifdef	FSD_KI
	struct timeval	starttime;

	/* stamp bp with enqueue time */
	KI_getprectime(&starttime);
#endif /* FSD_KI */
	nlink = *nlinkp;
	vp = *vpp;
	cvp = (struct vnode *)0;
#ifdef SAR
	sysnami++;
#endif

begin:
	if (pathisremote(vp))
	{
		*nlinkp = nlink;
		*vpp = vp;
		return (EPATHREMOTE);
	}

next:
	/*
	 * Make sure we have a directory.  However, for an NFS vnode
	 * this need not be an error case if the remote operating
	 * system supports lookup operations on non-dir files (e.g.
	 * Apollo extended names).  The penalty here is that
	 * for NFS one more lookup would be necessary to detect a
	 * genuine ENOTDIR error.
	 */
	if (vp->v_type != VDIR && vp->v_fstype != VNFS) {
		error = ENOTDIR;
		goto bad;
	}
	/*
	 * Process the next component of the pathname.
	 */
	error = (*pn_getcomponent)(pnp, component);
	if (error)
		goto bad;

	/*
	 * Check for degenerate name (e.g. / or "")
	 * which is a way of talking about a directory,
	 * e.g. "/." or ".".
	 */
	if (component[0] == 0) {
		/*
		 * If the caller was interested in the parent then
		 * return an error since we don't have the real parent
		 */
		if (dirvpp != (struct vnode **)0) {
			VN_RELE(vp);
			return(EISDIR);
		}
		(void) pn_set(pnp, ".");
		if (compvpp != (struct vnode **)0) {
			*compvpp = vp;
		} else {
			VN_RELE(vp);
		}
		return(0);
	}

	/*
	 * Handle parent directory: two special cases.
	 * 1. If at root directory (e.g. after chroot)
	 *    then ignore it so can't get out.
	 * 2. If this vnode is the root of a mounted
	 *    file system, then replace it with the
	 *    vnode which was mounted on so we take the
	 *    parent directory in the other file system.
	 */
	parent = is_parent_dir(component);
	cvp = vp;
	VN_HOLD(cvp);
	if (parent) {
checkforroot:
		if ((vp == u.u_rdir) || (vp == rootdir)) {
		        VN_RELE(cvp);
			cvp = vp;
			VN_HOLD(cvp);
			goto skip;
		}
		if (vp->v_flag & VROOT) {
		        VN_RELE(cvp);
			cvp = vp;
			vp = vp->v_vfsp->vfs_vnodecovered;
			VN_HOLD(vp);
			if (pathisremote(vp))
			{
				*nlinkp = nlink;
				*vpp = vp;
				/*Backup over .. so it will be transmitted*/
				pn_backup_comp(pnp);
				VN_RELE(cvp);
				return (EPATHREMOTE);
			}
			goto checkforroot;
		}
	}

	/*
	 * Perform a lookup in the current directory.
	 */
#ifdef FILE_TRACE
	if (log_path && (vp->v_flag & VTRACE))
		(*log_path)(u.u_procp->p_pid, vp, pnp->pn_path);
#endif
	error = VOP_LOOKUP(vp, component, &tvp, u.u_cred, cvp);
	VN_RELE(cvp);
	cvp = tvp;
	if (error) {
		cvp = (struct vnode *)0;
		/*
		 * The following pn_skipslash makes us ignore
		 * trailing slashes on the create path.  This allows
		 * us to be compatible with System V and be POSIX
		 * conformant.
		 */
		pn_skipslash(pnp);
		/*
		 * On error, if more pathname or if caller was not interested
		 * in the parent directory then hard error.
		 */
		if (pn_pathleft(pnp) || dirvpp == (struct vnode **)0
			|| error == EACCES)
			goto bad;
#ifdef	FSD_KI
		KI_locallookuppn(pnp->pn_buf, cvp, error, &starttime);
#endif /* FSD_KI */
		(void) pn_set(pnp, component);
		*dirvpp = vp;
		if (compvpp != (struct vnode **)0) {
			*compvpp = (struct vnode *)0;
		}
		return (0);
	}

	/*
	 * If this vnode is mounted on, then we
	 * transparently indirect to the vnode which 
	 * is the root of the mounted file system.
	 * Before we do this we must check that an unmount is not
	 * in progress on this vnode. This maintains the fs status
	 * quo while a possibly lengthy unmount is going on.
	 */
	lqsave = u.u_qsave;
mloop:
	while (vfsp = cvp->v_vfsmountedhere) {
                if (setjmp(&u.u_qsave)) {       
                        error = EINTR;
                        u.u_qsave = lqsave;
			PSEMA(&filesys_sema);
                        goto bad;
                }

		while (vfsp->vfs_flag & VFS_MLOCK) {
			vfsp->vfs_flag |= VFS_MWAIT;
			sleep((caddr_t)vfsp, PVFS);
			u.u_qsave = lqsave;
			goto mloop;
		}
		u.u_qsave = lqsave;
		vfsp->vfs_icount++;
		error = VFS_ROOT(cvp->v_vfsmountedhere, &tvp, component);
		vfsp->vfs_icount--;
		if (error)
			goto bad;
		VN_RELE(cvp);
		cvp = tvp;
		if (pathisremote(cvp))
		{
			*nlinkp = nlink;
			*vpp = cvp;
			if ((pn_pathleft(pnp) == 0) && 
			    (dirvpp != (struct vnode **) 0)) {
				*dirvpp = vp;
			}
			else {
				VN_RELE(vp);
			}
			return (EPATHREMOTE);
		}
	}

	/*
	 * If we hit a symbolic link and there is more path to be
	 * translated or this operation does not wish to apply
	 * to a link, then place the contents of the link at the
	 * front of the remaining pathname.
	 */
	if (cvp->v_type == VLNK &&
	    ((followlink == FOLLOW_LINK) || pn_pathleft(pnp))) {
		struct pathname linkpath;

		nlink++;
		if (nlink > MAXSYMLINKS) {
			error = ELOOP;
			goto bad;
		}
		error = getsymlink(cvp, &linkpath);
		if (error)
			goto bad;
		if (pn_pathleft(&linkpath) == 0)
/*Again SUN is making assumption of "." means current directory, this need to
  be fixed when other file systems are involved.*/
			(void) pn_set(&linkpath, ".");
		error = pn_combine(pnp, &linkpath);	/* linkpath before pn */
		pn_free(&linkpath);
		if (error)
			goto bad;
		VN_RELE(cvp);
		cvp = (struct vnode *)0;
		rootcheck(pnp,&vp);	/*check for starting with '/' */
		goto begin;
	}

skip:
	/*
	 * skip over slashes from end of last component
	 */
	pn_skipslash(pnp);
	/*
	 * Skip to next component of the pathname.
	 * If no more components, return last directory (if wanted)  and
	 * last component (if wanted).
	 */
	if (pn_pathleft(pnp) == 0) {
#ifdef	FSD_KI
		KI_locallookuppn(pnp->pn_buf, cvp, 0, &starttime);
#endif /* FSD_KI */
		(void) pn_set(pnp, component);
		if (dirvpp != (struct vnode **)0) {
			/*
			 * check that we have the real parent and not
			 * an alias of the last component
			 */
			*dirvpp = vp;
		} else {
			VN_RELE(vp);
		}
		if (compvpp != (struct vnode **)0) {
			*compvpp = cvp;
		} else {
			VN_RELE(cvp);
		}
		return (0);
	
       }

	/*
	 * Searched through another level of directory:
	 * release previous directory handle and save new (result
	 * of lookup) as current directory.
	 */
	VN_RELE(vp);
	vp = cvp;
	cvp = (struct vnode *)0;
	goto next;

bad:
#ifdef	FSD_KI
	KI_locallookuppn(pnp->pn_buf, (struct vnode *)0, error, &starttime);
#endif /* FSD_KI */
	/*
	 * Error. Release vnodes and return.
	 */
	if (cvp)
		VN_RELE(cvp);
	VN_RELE(vp);
	return (error);
}

/*
 * Each time we begin a new name interpretation (e.g.
 * when first called and after each symbolic link is
 * substituted), we allow the search to start at the
 * root directory if the name starts with a '/', otherwise
 * continuing from the current directory.
 */
rootcheck(pnp,vpp)
	register struct pathname *pnp;		/* pathaname to lookup */
	register struct vnode **vpp;		/* pointer for current vnode */
{
	if (pn_peekchar(pnp) == '/') {
		VN_RELE(*vpp);
		pn_skipslash(pnp);
		if (u.u_rdir)
			*vpp = u.u_rdir;
		else
			*vpp = rootdir;
		VN_HOLD(*vpp);
	}
}

/*
 * Gets symbolic link into pathname.
 */
static int
getsymlink(vp, pnp)
	struct vnode *vp;
	struct pathname *pnp;
{
	struct iovec aiov;
	struct uio auio;
	register int error;

	pn_alloc(pnp);
	aiov.iov_base = pnp->pn_buf;
	aiov.iov_len = MAXPATHLEN;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_seg = UIOSEG_KERNEL;
	auio.uio_resid = MAXPATHLEN;
	error = VOP_READLINK(vp, &auio, u.u_cred);
	if (error)
		pn_free(pnp);
	pnp->pn_pathlen = MAXPATHLEN - auio.uio_resid;
	return (error);
}


/* This routine should be a macro to call different routines
 * for different file systems.  Until SUN put this routine in
 * NFS protocol, this routine will serve the purpose for UNIX
 * system.
 */
is_parent_dir(name)
char *name;
{
        register char *cp;
        cp = name;
        if ((*cp++ == '.') && (*cp++ == '.') && 
	    ((*cp == '\000') || ((*cp++ == SDOCHAR) && (*cp == '\000'))))
        {
                return(1);
        }
        else {
                return(0);
        }
}

