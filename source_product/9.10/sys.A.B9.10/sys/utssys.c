/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/utssys.c,v $
 * $Revision: 1.17.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/16 10:04:28 $
 */
/* HPUX_ID: @(#)utssys.c	52.1		88/04/19 */

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


#include "../h/param.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdfs.h"
#include "../ufs/fs.h"
#include "../h/buf.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/utsname.h"
#include "../h/ustat.h"

#include "../h/conf.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../dux/dux_dev.h"
#include "../dux/cct.h"
extern site_t root_site, my_site;
#include "../h/kern_sem.h"

utssys()
{
    register i;
    register struct a {
	char	*cbuf;
	int	mv;	/* could be a dev_t, but int for arg passing */
	int	type;
    } *uap;
    extern char hostname[MAXHOSTNAMELEN+1];

    uap = (struct a *)u.u_ap;
    switch(uap->type) {
    case 0:		/* uname */

	/* 
	 * update utsname.machine and utsname.idnumber fields
	 * 68K:   9000/3xx            lanic address
	 * PA :   9000/8xx            hardware serial number
	 * added to prevent user spoofing of NetLS
	 */

	if (copyout(&utsname, uap->cbuf, sizeof(struct utsname)))
	    u.u_error = EFAULT;
	return;

    /* case 1 was umask */

    case 2:		/* ustat */
	PSEMA(&filesys_sema);
#ifdef LOCAL_DISC
	if (bdevrmt((dev_t)uap->mv)) {
	    dux_ustat((dev_t)uap->mv, uap->cbuf);
	    VSEMA(&filesys_sema);
	    return;
	}
#else /* LOCAL_DISC */
#ifndef FULLDUX
	if ((my_site_status & CCT_CLUSTERED) && (my_site != root_site))
#endif /* not FULLDUX */
	    if (bdevrmt((dev_t)uap->mv)) {
		dux_ustat((dev_t)uap->mv, uap->cbuf);
	        VSEMA(&filesys_sema);
		return;
	    }
#ifndef FULLDUX
	    else {
		dev_t  rmt_dev;

		rmt_dev = (dev_t)(mkbrmtdev(root_site,
					devindex(uap->mv, IFBLK)));
		dux_ustat(rmt_dev, uap->cbuf);
	        VSEMA(&filesys_sema);
		return;
	    }
#endif /* not FULLDUX */
#endif /* LOCAL_DISC */

	{
	    register struct mount *mp;

#if defined(hp9000s800) && !defined(_WSIO)
	    if (map_lu_to_mi((dev_t *)&uap->mv, IFBLK, 0) != 0) {
		u.u_error = EINVAL;
	        VSEMA(&filesys_sema);
		return;
	    }
#endif /* hp9000s800 */

	    mp = getmp((dev_t)uap->mv);
		
	    if (mp && (mp->m_flag & MINUSE)) {
		struct ustat ustat;
		/*
		**  instead of the following if/else, we should
		**  probably add an operation to the vnode layer  
		*/

		if (mp->m_vfsp->vfs_mtype == MOUNT_UFS) {
		    register struct fs *fp;
	
#ifdef USTAT_KLUDGE
		   /*
		    * this is to conform to XPG2 semantics
		    * the free inode count must be correct
		    * synchronously!
		    */
		    flush_all_inactive();
#endif /* USTAT_KLUDGE */

		    fp = mp->m_bufp->b_un.b_fs;
		    ustat.f_tfree = fp->fs_cstotal.cs_nffree +
			(fp->fs_cstotal.cs_nbfree << fp->fs_fragshift);
		    ustat.f_tinode = fp->fs_cstotal.cs_nifree;
		    bcopy(fp->fs_fname, ustat.f_fname, sizeof(ustat.f_fname));
		    bcopy(fp->fs_fpack, ustat.f_fpack, sizeof(ustat.f_fpack));
		    ustat.f_blksize = fp->fs_fsize;

		    if (copyout(&ustat, uap->cbuf, sizeof(struct ustat)))
			u.u_error = EFAULT;
	            VSEMA(&filesys_sema);
		    return;
		} else if (mp->m_vfsp->vfs_mtype == MOUNT_CDFS) {
			
		    register struct cdfs *cdfsp;

		    cdfsp = (struct cdfs *) mp->m_bufp->b_un.b_fs;
			
		    ustat.f_tfree = 0;
		    ustat.f_tinode = 0;
		    ustat.f_blksize = cdfsp->cdfs_lbsize;

		    switch (cdfsp->cdfs_magic) {
		    case CDFS_MAGIC_HSG:
			bcopy("CDROM", ustat.f_fname, sizeof(ustat.f_fname));
			break;
		    case CDFS_MAGIC_ISO:
			bcopy("CD001", ustat.f_fname, sizeof(ustat.f_fname));
			break;
		    default:
			bcopy("?????", ustat.f_fname, sizeof(ustat.f_fname));
			break;
		    }
		    ustat.f_fpack[0] = '\000';
		    if (copyout(&ustat, uap->cbuf, sizeof(struct ustat)))
					u.u_error = EFAULT;
	            VSEMA(&filesys_sema);
		    return;
		}	/*  should we return ENO_SUCH_FS here?  */
	    }
	}

	u.u_error = EINVAL;
	VSEMA(&filesys_sema);
	return;
    case 3:		/* setuname (set node name field) */
	if (suser()) {
	    char tmp[UTSLEN];
	    char c;

	    /* make sure we change nothing until we know it will succeed */
	    for(i=0; i<UTSLEN-1; i++) {
		c = fubyte(uap->cbuf++);
		if((int)c == -1) {
		    u.u_error = EFAULT;
		    return;
		}
		tmp[i] = c;
		if (c == '\0') break;
	    }
	    for (i++; i<UTSLEN-1; i++)
		tmp[i] = '\0';
	    for (i=0; i<UTSLEN-1; i++)
		utsname.nodename[i] = tmp[i];
	} else {
	    u.u_error = EPERM;
	    return;
	}
	return;
    case 4:		/*sethostname*/
	if (suser()) {
	    char tmp[MAXHOSTNAMELEN+1];
	    char c;
	    unsigned int len;

	    if (uap->mv <= 0)
		{
		u.u_error = EINVAL;
		return;
		}

	    len = (unsigned int) uap->mv;
	    if (len > MAXHOSTNAMELEN)
		len = MAXHOSTNAMELEN;
	    /* make sure we change nothing until we know it will succeed */
	    for(i=0; i<len; i++) {
		c = fubyte(uap->cbuf++);
		if((int)c == -1) {
		    u.u_error = EFAULT;
		    return;
		}
		tmp[i] = c;
		if (c == '\0') {
		    i++;
		    break;
		}
	    }
	    for (; i<=MAXHOSTNAMELEN; i++)
		tmp[i] = '\0';
	    for (i=0; i<=MAXHOSTNAMELEN; i++)
		hostname[i] = tmp[i];
	} else {
	    u.u_error = EPERM;
	    return;
	}
	return;
    case 5:		/*gethostname*/
	{
	    char tmp[MAXHOSTNAMELEN+1];
	    unsigned int len;

	    if (uap->mv <= 0)
		{
		u.u_error = EINVAL;
		return;
		}

	    len = (unsigned int)uap->mv;
	    if (len > MAXHOSTNAMELEN+1)
		len = MAXHOSTNAMELEN+1;
	    /*copy the name into the temporary buffer making it
	     *easy to append a NULL before copy out
	     */
	    for (i=0; i<len; i++)
		tmp[i] = hostname[i];
	    tmp[len-1] = '\0';
	    if (copyout(tmp, uap->cbuf, len))
		u.u_error = EFAULT;
	    return;
	}
    default:
	u.u_error = EFAULT;
    }
}
