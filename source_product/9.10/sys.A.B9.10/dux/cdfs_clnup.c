/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/cdfs_clnup.c,v $
 * $Revision: 1.4.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:39:36 $
 */

/* HPUX_ID: @(#)cdfs_clnup.c	54.3		88/12/02 */

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
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/conf.h"
#include "../h/vfs.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../h/kern_sem.h"



/*
 * Perform all the necessary cleanup of cdnodes including:
 *
 *	1) Local cdnodes referenced by the remote site
 *
 *	2) Remote cdnodes referenced by this site
 *
 *	3) Remote cdnodes refering to devices at the remote site.
 */
cdno_cleanup(site)
register site_t site;
{
	register struct cdnode *cdp;
	register struct vnode *vp;
	register int numref;	/*the number of references of a various type*/
	int err = 0;

	/*traverse the list of cdnodes and cleanup anything held by a remote
	 *site*/
	for (cdp = cdnode; cdp < cdnodeNCDNODE; cdp++)
	{
		vp = CDTOV(cdp);

		/*  If no refernences, no need to do anything		*/
		if(vp->v_count == 0)
			continue;

		/*first check for a lock held by the site on the cdnode*/
		if ((cdp->cd_flag & CDLOCKED) && (cdp->cd_cdlocksite == site))
			cdunlock(cdp);
		/*determine if the site has the file open*/
		if ((numref = getsitecount(&cdp->cd_opensites,site)) > 0)
		{
			/*close the file as appropriate*/
			/*update the sitecounts*/
			updatesitecount(&cdp->cd_opensites, site, -numref);
			/* Throw out all but one reference */
			SPINLOCK(v_count_lock);
			vp->v_count -= numref-1;
			SPINUNLOCK(v_count_lock);
			/*and release the last reference*/
			VN_RELE(vp);
		}
		/*Is anyone referencing this file for anything else?*/
		if ((numref = getsitecount(&cdp->cd_refsites,site)) > 0)
		{
			updatesitecount(&(cdp->cd_refsites),site,-numref);
			SPINLOCK(v_count_lock);
			vp->v_count -= numref-1;
			SPINUNLOCK(v_count_lock);
			VN_RELE(vp);
		}
		/*determine if anyone is execing to this file*/
		if ((vp->v_type == VREG) &&
			(numref = getsitecount(&cdp->cd_execsites,site)) > 0)
		{
			/*
			** dectext cld send an updated count (w/ DM_SLEEP)
			** to the server if this were a remote cdnode, however,
			** since we own the sitemap, we are the server and
			** therefore, we won't do the send or sleep.
			*/
			dectext(vp, numref);
		}
#ifdef LOCAL_DISC
		/* If the cdnode is on a device on the remote site, unhash it
		 * and change the site to site 0, so subsequent I/O operations
		 * will result in errors.  Also, nullify
		 * the fs and the mount pointers since we should never
		 * reference them (If we do it is an error).
		 */
		if (remotecdp(cdp) && devsite(cdp->cd_dev) == site)
		{
			/*
			** A check for a locked cdnode must
			** be done here and the err code must be set
			** if the cdnode is locked because we cannot
			** sleep in cleanup waiting for another site
			** to do something as that other site might
			** crash and we can't clean it up if we're
			** sleeping!
			*/
			if (cdp->cd_flag & CDLOCKED) {
				err=1;
				continue;
			}
			cdlock(cdp);
			cdp->cd_dev = mkbrmtdev(0,0);
			cdp->cd_fs = NULL;
			cdp->cd_dfs = NULL;
			cdp->cd_mount = NULL;
			/*remove it from the hash queue*/
			cdp->cd_back->cd_forw = cdp->cd_forw;
			cdp->cd_forw->cd_back = cdp->cd_back;
			/*and put it on a queue consisting only of itself
			 *(like a virgin cdnode)
			 */
			cdp->cd_forw = cdp;
			cdp->cd_back = cdp;
			cdunlock(cdp);
		}
#endif LOCAL_DISC
	}    /* end of for (cdnodes) loop */
	return(err);
}
