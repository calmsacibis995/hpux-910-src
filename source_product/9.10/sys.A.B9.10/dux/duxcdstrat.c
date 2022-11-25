/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/duxcdstrat.c,v $
 * $Revision: 1.3.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:43:19 $
 */

/* HPUX_ID: @(#)duxcdstrat.c	54.4		88/12/05 */
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

/*
 *duxcdstrat.c
 *
 *This file contains the code for sending and processing remote KFS strategy
 *requests. 
 *
 *This file contains both the code for the requesting site and the serving
 *site.
 *
 *note:  Block operations directly to the disc (as opposed to the file system)
 *use the regular (local) block I/O.
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/systm.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../rpc/types.h"
#include "../h/mount.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_dev.h"
#include "../dux/dux_lookup.h"
#include "../dux/lookupmsgs.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"


struct buf *cdfs_read();
/*
 *The following routines serve the appropriate requests on the serving site.
 *Both the read and write are subject to the following requirements:
 *	-The requested block is contained within a single disc block.
 *	-Asynchronous I/O is being used on the file (so lockf checks need
 *		not be made, and so the function can be used with pipes).
 */

/*
 *This function will perform a read on the serving site.  The steps are:
 *
 *	1) Find the cdnode.
 *
 *	2) Figure out the block that must be read (there can be no more than
 *		one)
 *
 *	3) Read the block (from disc or core and with a possible readahead)
 *
 *	4) Send the block back.  It may be that only part of the block
 *		has been requested in which case only that part should
 *		be sent.
 */
servestratreadcd(sreqp)
register struct duxstratreq *sreqp;
{
	register struct cdnode *cdp;
	dm_message response;
	register struct duxstratresp *srespp;
	register long offset = 0;
	register long length = sreqp->ns_count;
	dev_t dev = localdev(sreqp->ns_dev);
	struct buf *bp;
	long resid;

	/*look up and lock the cdnode*/
	cdp = cdfind(dev,sreqp->ns_cdno);
	if (cdp == NULL)
	{
		printf ("cdfind failed: servestratread\n");
		dm_quick_reply(EIO);
		return;
	}
	cdlock(cdp);

	offset = sreqp->ns_offset; 
	bp = cdfs_read (cdp, offset, length);

	resid = bp->b_resid;
	/*release the vnode*/
	/*send back the reply*/
	if (bp->b_flags & B_ERROR)
	{
		cdunlock(cdp);
		dm_quick_reply(geterror(bp));
		brelse(bp);
		return;
	}
	response = dm_alloc(sizeof(struct duxstratresp),WAIT);
	srespp = DM_CONVERT(response,struct duxstratresp);
	srespp->ns_resid = resid;
	srespp->ns_flags = 0;
	cdunlock(cdp);
	dm_reply(response,DM_REPLY_BUF,0,bp,length,0);
	/*reply will release bp*/
}
struct buf *
cdfs_read(cdp, offset, size)
struct cdnode *cdp;
u_int	offset, size;
{
	struct	uio	auio;
	struct	iovec	aiov;
	register struct	uio	*auiop = &auio;
	register struct	iovec	*aiovp = &aiov;
	struct	buf	*bp;

	
	bp = geteblk(size);
	auiop->uio_iov = aiovp;
	aiovp->iov_base = bp->b_un.b_addr;
	aiovp->iov_len = size;
	auiop->uio_iovcnt = 1;
	auiop->uio_offset = offset;
	auiop->uio_seg = UIOSEG_KERNEL;
	auiop->uio_resid = aiovp->iov_len;	/*byte count*/
	auiop->uio_fpflags = 0;
	if(cdfs_rd(cdp->cd_devvp, CDTOV(cdp), auiop)) {	/*read*/
		bp->b_flags |= B_ERROR;
	}
	return(bp);
}
