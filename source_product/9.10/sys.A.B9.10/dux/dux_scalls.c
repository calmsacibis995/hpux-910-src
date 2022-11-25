/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_scalls.c,v $
 * $Revision: 1.5.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:42:29 $
 */

/* HPUX_ID: @(#)dux_scalls.c	55.1		88/12/23 */

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

/* Miscellaneous DUX system calls */
#include "../h/param.h"
#include "../h/user.h"
#include "../h/ustat.h"
#include "../h/buf.h"
#include "../ufs/fs.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/unsp.h"
#include "../dux/dmmsgtype.h"
#include "../dux/cct.h"
#include "../dux/dux_dev.h"
#include "../dux/dux_lookup.h"
#include "../h/mount.h"
#include "../dux/lookupops.h"
#include "../dux/lookupmsgs.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"

/* Global sync */

global_sync()
{
        dm_message request, response;

        request = dm_alloc(DM_EMPTY, WAIT);
        response = dm_send (request, DM_REPEATABLE|DM_INTERRUPTABLE,
		DM_LSYNC, DM_CLUSTERCAST, DM_EMPTY, NULL, NULL, 0, 0,
		NULL, NULL, NULL);
	lsync();
	dm_wait(request);
        dm_release(request,0);
        dm_release(response,1);
}

/*serve the sync request*/

/*ARGSUSED*/
servelsync(request)
dm_message request;
{
	lsync();
        dm_quick_reply(0);
}

/* ustat system call */
struct ustat_request		/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;
};

struct ustat_reply		/*DUX MESSAGE STRUCTURE*/
{
	struct ustat ustat;
};

struct fstatfs_request 		/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;
	ino_t i_num;
};

dux_ustat(dev,cbuf)
dev_t dev;
char *cbuf;
{
	dm_message request,reply;
	struct ustat_reply *up;

	request = dm_alloc(sizeof(struct ustat_request), WAIT);
	(DM_CONVERT(request, struct ustat_request))->dev = dev;
        reply = dm_send (request, DM_REPEATABLE|DM_SLEEP|DM_RELEASE_REQUEST,
		DM_USTAT, devsite(dev), sizeof(struct ustat_reply),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	u.u_error = DM_RETURN_CODE(reply);
	if (!u.u_error)
	{
		up = DM_CONVERT(reply, struct ustat_reply);
		if(copyout(&up->ustat, cbuf, sizeof(struct ustat)))
			u.u_error = EFAULT;
	}
	dm_release(reply,0);
}

dux_ustat_serve(request)
dm_message request;
{
	register dev_t dev =
		localdev((DM_CONVERT(request, struct ustat_request))->dev);
	register struct mount *mp;

	mp = getmp(dev);
	if (mp && (mp->m_flag & MINUSE))
	{
	   if (mp->m_vfsp->vfs_mtype == MOUNT_UFS) {
		register struct fs *fp;
		dm_message reply;
		register struct ustat *up;

#ifdef USTAT_KLUDGE
		/*
		 * this is to conform to XPG2 semantics
		 * the free inode count must be correct
		 * synchronously!
		 */
		flush_all_inactive();
#endif

		fp = mp->m_bufp->b_un.b_fs;
		reply = dm_alloc(sizeof (struct ustat_reply), WAIT);
		up = &(DM_CONVERT(reply, struct ustat_reply))->ustat;
		up->f_tfree = fp->fs_cstotal.cs_nffree +
			(fp->fs_cstotal.cs_nbfree << fp->fs_fragshift);
		up->f_tinode = fp->fs_cstotal.cs_nifree;
		bcopy(fp->fs_fname,up->f_fname,sizeof(up->f_fname));
		bcopy(fp->fs_fpack,up->f_fpack,sizeof(up->f_fpack));
		up->f_blksize = fp->fs_fsize;
		dm_reply(reply,0,0, NULL, NULL, NULL);
		return;
	   } else if (mp->m_vfsp->vfs_mtype == MOUNT_CDFS) {
		
		register struct cdfs *cdfsp;
		dm_message reply;
		register struct ustat *up;

		cdfsp = (struct cdfs *) mp->m_bufp->b_un.b_fs;
		reply = dm_alloc(sizeof (struct ustat_reply), WAIT);
		up = &(DM_CONVERT(reply, struct ustat_reply))->ustat;
		up->f_tfree = 0;
		up->f_tinode = 0;
		up->f_blksize = cdfsp->cdfs_lbsize;
		switch (cdfsp->cdfs_magic) {
			case CDFS_MAGIC_HSG:
			   bcopy("CDROM", up->f_fname, sizeof(up->f_fname));
			   break;

			case CDFS_MAGIC_ISO:
			   bcopy("CD001", up->f_fname, sizeof(up->f_fname));
			   break;

			default:
			   bcopy("?????", up->f_fname, sizeof(up->f_fname));
			   break;
		}
		up->f_fpack[0] = '\000';
		dm_reply(reply,0,0, NULL, NULL, NULL);
		return;
	   }
	}
	dm_quick_reply(EINVAL);
}




dux_fstatfs(vfsp, sbp)
struct vfs *vfsp;
struct statfs *sbp;
{
	dm_message request,reply;
	struct statfs_reply *sp;
	register struct mount *mp;
	register struct fstatfs_request *rp;
	int error;

	request = dm_alloc(sizeof(struct fstatfs_request), WAIT);
	rp = DM_CONVERT(request, struct fstatfs_request);
	mp = (struct mount *)vfsp->vfs_data;
	rp->dev = mp->m_dev;
        reply = dm_send (request, DM_REPEATABLE|DM_SLEEP|DM_RELEASE_REQUEST,
		DM_FSTATFS, devsite(mp->m_dev), sizeof(struct statfs_reply),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	if (!error) {
		sp = DM_CONVERT(reply, struct statfs_reply);
		bcopy(&sp->statfs, sbp, sizeof(struct statfs));
	}
	dm_release(reply,0);
	return(error);
}

dux_fstatfs_serve(request)
dm_message request;
{
        register error;
        dm_message reply;
        struct statfs_reply *sp;
	register struct mount *mp;
	register struct fstatfs_request *rp =
		(DM_CONVERT(request, struct fstatfs_request));

	mp = getmp(localdev(rp->dev));
        if (mp == NULL) /*shouldn't happen*/
        {
                printf("getmp failed:  dux_fstatfs_serve\n");
                dm_quick_reply (ENOENT);
                return;
        }

        reply=dm_alloc(sizeof(struct statfs_reply), WAIT);
        sp = DM_CONVERT(reply, struct statfs_reply);
        error = VFS_STATFS(mp->m_vfsp, &sp->statfs);
        dm_reply(reply, 0, error, NULL, NULL, NULL);
}

/*
 *  mysite system call.  If clustered, returns the value of my_site.
 *  Otherwise, returns 0.
 */

extern site_t my_site;

mysite()
{
      u.u_r.r_val1 = my_site;
}
