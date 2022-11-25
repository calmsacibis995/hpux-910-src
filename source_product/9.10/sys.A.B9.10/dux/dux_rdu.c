/* $Header: dux_rdu.c,v 1.10.83.4 93/10/20 10:38:57 rpc Exp $ */
/* HPUX_ID: @(#)dux_rdu.c	55.1		88/12/23 */

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
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../dux/dux_rdX.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/unsp.h"
extern site_t swap_site;

static int rdu_intr();

/*  Do an opend for a remote device dev located at site.
 *  Modifies the dev field as a side effect.
 */
/*ARGSUSED*/
#ifdef FULLDUX
int rdu_opend(dev, mode, site, omode)
register dev_t  *dev;
u_int  mode, omode;
site_t site;
{ 
	register rdu_t *rdu;
    register int   error;
    register dm_message msg;

    msg = dm_alloc(RDU_SIZE(0), WAIT);
    rdu = DM_CONVERT(msg, rdu_t);

    rdu->rdu_dev        = *dev;
    rdu->rdu_open.site  = site;
    rdu->rdu_open.flag  = omode;
    rdu->rdu_open.mode = mode;

    msg = dm_send(msg, DM_SLEEP|DM_RELEASE_REQUEST|DM_INTERRUPTABLE,
		  DMNDR_OPEND, site,
	       RDS_SIZE(0), NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if((error = DM_RETURN_CODE(msg)) == 0)
	{ 
		register int index = DM_CONVERT(msg, rds_t)->rds_index;
	if(mode == IFBLK)       /*  Translate the dev number  */
	  *dev = mkbrmtdev(site, index);
	else
	  *dev = mkcrmtdev(site, index);
      }

    dm_release(msg, 0);
    return(error);
}

/*ARGSUSED*/
int rdu_closed(dev, flag)
dev_t dev;
int flag;
{ 
	register rdu_t *rdu;
    register int error = 0;
    register dm_message msg;

    msg = dm_alloc(RDU_SIZE(0), WAIT);
    rdu = DM_CONVERT(msg, rdu_t);

    rdu->rdu_dev  = dev;
    rdu->rdu_flag = flag;

    /*  If we are an nsp (u.u_site != 0), send the site number of the requestor
     *  as the origiantor of the close.  Otherwise, we started the close.
     */
    rdu->rdu_origsite = (u.u_site ? u.u_site : my_site);

    msg = dm_send(msg, DM_SLEEP|DM_RELEASE_REQUEST|DM_INTERRUPTABLE,
		  DMNDR_CLOSE, devsite(dev), DM_EMPTY,
		  NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    error = DM_RETURN_CODE(msg);

    dm_release(msg, 0);
    return(error);
}
#endif /* FULLDUX */

/*ARGSUSED*/
rdu_strategy(bp, swap)
register struct buf *bp;
int swap;
{ 
    register rdu_t *rdu;
    register dm_message msg;
    extern int freemem;
    site_t sitenum;
    int dmflags;

    msg = dm_alloc(RDU_SIZE(0), WAIT);
    rdu = DM_CONVERT(msg, rdu_t);

    if (swap)
	sitenum = swap_site;
    else
	sitenum = devsite(rdu->rdu_dev);

    rdu->rdu_strat.nbpg   = NBPG;
    rdu->rdu_dev          = bp->b_dev;
    rdu->rdu_strat.flags  = bp->b_flags;
    rdu->rdu_strat.bcount = bp->b_bcount;
    rdu->rdu_strat.blkno  = bp->b_blkno;
    rdu->rdu_strat.offset = bp->b_offset;
    
    if(bp->b_flags & B_READ)
    {	/* Reading */
	dmflags = DM_FUNC | DM_REPEATABLE | DM_REPLY_BUF;
    }
    else
    {	/* Writing */
	dmflags = DM_FUNC | DM_REPEATABLE | DM_REQUEST_BUF;

	/* If this is a swapping/paging request and we are out of memory,
	 * then make this an out of band message.
	 */
        if ((swap) && (freemem == 0))
	    dmflags |= DM_URGENT;
    }

    (void) dm_send(msg, dmflags, DMNDR_STRAT, sitenum, RDS_SIZE(0),
		   rdu_intr, bp, bp->b_bcount, 0, NULL, NULL, NULL);
}

/*ARGSUSED*/
int rdu_read(dev, uio)
dev_t dev;
register struct uio *uio;
{ 
#ifdef FULLDUX
	register dm_message msg;
    register int        error,
			iosize = uio->uio_resid;
    register site_t     site   = devsite(dev);

    /*  If the request is less  than  DMRD_IOSIZE,  allocate  and  send  a
     *  message - the data will be contained in the reply
     */

    if(iosize <= DMRD_IOSIZE)
	{ 
		register rdu_t *rdu;
	register rds_t *rds;

	msg = dm_alloc(RDU_SIZE(0), WAIT);
	rdu = DM_CONVERT(msg, rdu_t);

	rdu->rdu_dev = dev;
	rdu->rdu_uio.uio_offset = uio->uio_offset;
	rdu->rdu_uio.uio_resid  = iosize;
	rdu->rdu_uio.uio_fpflags = uio->uio_fpflags;

	msg = dm_send(msg, DM_SLEEP | DM_RELEASE_REQUEST | DM_INTERRUPTABLE,
		      DMNDR_READ,
		      site, RDS_SIZE(iosize), NULL, NULL, NULL, NULL,
			NULL, NULL, NULL);

	if((error = DM_RETURN_CODE(msg)) == 0)
		{ 
			rds = DM_CONVERT(msg, rds_t);
	    error = uiomove(rds->rds_iodata, iosize - rds->rds_resid,
				 UIO_READ, uio);
	  }

	dm_release(msg, 0);
	return(error);
      }

    /*  If the request is larger the DMRD_IOSIZE but less  than  MAXDUXBSIZE,
     *  send a message - the data will be returned in a disk buffer.
     */

    if(iosize <= MAXDUXBSIZE)
	{ 
		register struct buf *bp;

	msg = dm_alloc(RDU_SIZE(0), WAIT);
	bp  = geteblk(iosize);

		{ 
			register rdu_t *rdu = DM_CONVERT(msg, rdu_t);

	  rdu->rdu_dev = dev;
	  rdu->rdu_uio.uio_offset = uio->uio_offset;
	  rdu->rdu_uio.uio_resid  = iosize;
	  rdu->rdu_uio.uio_fpflags = uio->uio_fpflags;
	}

	msg = dm_send(msg,
		      DM_SLEEP|DM_REPLY_BUF|
		      DM_RELEASE_REQUEST|DM_INTERRUPTABLE,
		      DMNDR_READ, site, RDS_SIZE(0),
		      NULL,
		      bp, iosize, 0,
		      NULL, NULL, NULL);

	if((error = DM_RETURN_CODE(msg)) == 0)
		{ 
			register rds_t *rds = DM_CONVERT(msg, rds_t);
	    error = uiomove(bp->b_un.b_addr, iosize - rds->rds_resid,
				UIO_READ, uio);
	  }

	dm_release(msg, 1);
	return(error);
      }

    /*  If the request is larger than MAXDUXBSIZE, send a special  request  to
     *  start  up  a user NSP.  Continue to read data until an error occurs
     *  or the request is satisfied.  The first message back tells how many
     *  bytes to expect and has an 8K buffer with it.
     */

	{ 
		register struct buf       *bp;
      register dm_message       rpl;
      register struct rdX_bigio *bigio;
      register int 		left2read;

      msg   = dm_alloc(sizeof(struct unsp_message), WAIT);
      bigio = DM_CONVERT(msg, struct rdX_bigio);
      bp    = geteblk(MAXDUXBSIZE);

      /*  Try to establish the UNSP - if something bombs, return error  */

      bigio->rdX_um_id  = -1;
      bigio->rdX_dev    = dev;
      bigio->rdX_size   = iosize;
      bigio->rdX_offset = uio->uio_offset;
      bigio->rdX_fpflags = uio->uio_fpflags;
      bigio->rdX_sousig = u.u_procp->p_flag & SOUSIG;

      rpl = dm_send(msg, DM_SLEEP | DM_REPLY_BUF | DM_INTERRUPTABLE,
		    DMNDR_BIGREAD, site, sizeof(struct unsp_message),
		    NULL,
		    bp, MAXDUXBSIZE, 0,
		    NULL, NULL, NULL);

      if((error = DM_RETURN_CODE(rpl)))
		{ 
			dm_release(rpl, 1);           /*  Note: also releases buffer  */
	  dm_release(msg, 0);
	  return(error);
	}

      bigio->rdX_um_id = DM_CONVERT(rpl, struct rdX_bigio)->rdX_um_id;
      left2read	       = DM_CONVERT(rpl, struct rdX_bigio)->rdX_size;
      dm_release(rpl, 0);

      error = 0;
      while(error == 0 && left2read > 0)
		{ 
			error = uiomove(bp->b_un.b_addr, MIN(left2read, MAXDUXBSIZE), UIO_READ, uio);

	  left2read -= MAXDUXBSIZE;

	  if(left2read > 0)
	    if(error)
{ /*  If the UNSP is waiting (i.e., there's still  data  to  be
		 *  sent), send a death message
		 */
		dm_send(msg, DM_RELEASE_REPLY, DMNDR_BIGIOFAIL, site,
				sizeof(struct unsp_message),
			 	NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	      }
	    else
				{ 
				   rpl = dm_send(msg,
				   DM_SLEEP | DM_REPLY_BUF | DM_INTERRUPTABLE,
			           DMNDR_BIGREAD,
			           site, sizeof(struct unsp_message),
			           NULL,
			           bp, MAXDUXBSIZE, 0, 
				   NULL, NULL, NULL);

		error = DM_RETURN_CODE(rpl);
		dm_release(rpl, 0);
	      }
	}

      dm_release(msg, 0);
      brelse(bp);
      return(error);
    }
#else
	panic("rdu_read: remote devices not supported");
#endif
}

/*ARGSUSED*/
int rdu_write(dev, uio)
dev_t dev;
register struct uio *uio;
{ 
#ifdef FULLDUX
	register dm_message msg;
    register int        error,
			iosize = uio->uio_resid;
    register site_t     site   = devsite(dev);

    /*  If the request is less  than  DMRD_IOSIZE,  allocate  and  send  a
     *  message containing the data
     */

    if(iosize <= DMRD_IOSIZE)
	{ 
		register rdu_t *rdu;
	register rds_t *rds;

	msg = dm_alloc(RDU_SIZE(iosize), WAIT);
	rdu = DM_CONVERT(msg, rdu_t);

	rdu->rdu_dev = dev;
	rdu->rdu_uio.uio_offset = uio->uio_offset;
	rdu->rdu_uio.uio_resid  = iosize;
	rdu->rdu_uio.uio_fpflags = uio->uio_fpflags;

	if((error = uiomove(rdu->rdu_iodata, iosize, UIO_WRITE, uio)))
	  goto write_done;

	msg   = dm_send(msg, DM_SLEEP | DM_RELEASE_REQUEST | DM_INTERRUPTABLE,
			DMNDR_WRITE,
			site, RDS_SIZE(0), NULL, NULL, NULL, NULL,
			NULL, NULL, NULL);

	if((error = DM_RETURN_CODE(msg)) == 0)
		{ 
			register int resid = DM_CONVERT(msg, rds_t)->rds_resid;
	    uio->uio_resid   = resid;
	    uio->uio_offset -= resid;
	  }

write_done:
	dm_release(msg, 0);
	return(error);
      }

    /*  If the request is larger the DMRD_IOSIZE but less  than  MAXDUXBSIZE,
     *  send a message - the data will be returned in a disk buffer.
     */

    if(iosize <= MAXDUXBSIZE)
	{ 
		register struct buf *bp  = geteblk(iosize);
	register u_int offset    = uio->uio_offset;

	if((error = uiomove(bp->b_un.b_addr, iosize, UIO_WRITE, uio)))
		{ 
			brelse(bp);
	    return(error);
	  }

	msg = dm_alloc(RDU_SIZE(0), WAIT);

		{ 
			register rdu_t *rdu = DM_CONVERT(msg, rdu_t);

	  rdu->rdu_dev = dev;
	  rdu->rdu_uio.uio_offset = offset;
	  rdu->rdu_uio.uio_resid  = iosize;
	  rdu->rdu_uio.uio_fpflags = uio->uio_fpflags;
	}

	msg   = dm_send(msg,
			DM_SLEEP|DM_RELEASE_REQUEST|
			DM_REQUEST_BUF|DM_INTERRUPTABLE,
			DMNDR_WRITE, site, RDS_SIZE(0),
			NULL,
			bp, iosize, 0,
			NULL, NULL, NULL);

	if((error = DM_RETURN_CODE(msg)) == 0)
		{ 
			register int resid = DM_CONVERT(msg, rds_t)->rds_resid;
	    uio->uio_resid   = resid;
	    uio->uio_offset -= resid;
	  }

	dm_release(msg, 0);
	return(error);
      }

    /*  If the request is larger than MAXDUXBSIZE, send a special  request  to
     *  start  up a user NSP.  Continue to write data until an error occurs
     *  or the request is satisfied.  The request message is reused for the
     *  next  request, so the UNSP id is already set each time through.  If
     *  the data transfer completes OK, the rdX_size field of the  response
     *  indicates the residual bytes from the actual write.
     */

	{ 
		register struct buf           *bp = geteblk(MAXDUXBSIZE);
      register        dm_message    rpl;
      register struct rdX_bigio     *bigio;
      register u_int                offset = uio->uio_offset;
      register int		    left2send;

      if((error = uiomove(bp->b_un.b_addr, MAXDUXBSIZE, UIO_WRITE, uio)))
		{ 
			brelse(bp);
	  return(error);
	}

      msg   = dm_alloc(sizeof(struct unsp_message), WAIT);
      bigio = DM_CONVERT(msg, struct rdX_bigio);

      /*  Try to establish the UNSP - if something bombs, return error  */

      bigio->rdX_um_id  = -1;
      bigio->rdX_dev    = dev;
      bigio->rdX_size   = iosize;
      bigio->rdX_offset = offset;
      bigio->rdX_fpflags = uio->uio_fpflags;
      bigio->rdX_sousig = u.u_procp->p_flag & SOUSIG;

      rpl = dm_send(msg, DM_SLEEP | DM_REQUEST_BUF | DM_INTERRUPTABLE,
		    DMNDR_BIGWRITE, site, sizeof(struct unsp_message),
		    NULL,
		    bp, MAXDUXBSIZE, 0,
		    NULL, NULL, NULL);

      if((error = DM_RETURN_CODE(rpl)) == 0)
	bigio->rdX_um_id  = DM_CONVERT(rpl, struct rdX_bigio)->rdX_um_id;

      for(left2send = iosize - MAXDUXBSIZE; error == 0 && left2send > 0; left2send -= MAXDUXBSIZE)
		{ 
			register int send_bytes = MIN(left2send, MAXDUXBSIZE);
	  dm_release(rpl, 0);
	  error = uiomove(bp->b_un.b_addr, send_bytes, UIO_WRITE, uio);

	  if(error)
	    /*  We know that UNSP is waiting - send death message       */
	    dm_send(msg, DM_RELEASE_REPLY, DMNDR_BIGIOFAIL, site,
			    sizeof(struct unsp_message),
			    NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	  else
			{ 
				rpl = dm_send(msg,
				      DM_SLEEP|DM_REQUEST_BUF|DM_INTERRUPTABLE,
			              DMNDR_BIGWRITE, site,
				      sizeof(struct unsp_message),
			    	      NULL, bp, send_bytes, 0,
				      NULL, NULL, NULL);

	      error = DM_RETURN_CODE(rpl);
	    }
	}

      if(error == 0)    /*  Transfer completed OK - reply has bytes written */
		{ 
			register int written = DM_CONVERT(rpl, struct rdX_bigio)->rdX_size;
	  uio->uio_resid   = iosize - written;
	}

      dm_release(rpl, 0);
      dm_release(msg, 0);
      brelse(bp);
      return(error);
    }
#else
	panic("rdu_write: remote devices not supported");
#endif
}

/*ARGSUSED*/
rdu_ioctl(dev, cmd, data, flag)
dev_t dev;
register int cmd;
register caddr_t data;
int flag;
{ 
#ifdef FULLDUX
	register rdu_t *rdu;
    register dm_message msg;
    register int size, error;

    size = (cmd & ~(IOC_INOUT|IOC_VOID)) >> 16;
/*
    msg = dm_alloc(RDU_SIZE((cmd & IOC_IN) ? size : 0), WAIT);
*/
    if(  ((cmd & IOC_IN) == IOC_IN) && (size == 0) )
    	msg = dm_alloc( RDU_SIZE( sizeof(u_int) ), WAIT);
    else
    	msg = dm_alloc( RDU_SIZE( size ), WAIT);

    rdu = DM_CONVERT(msg, rdu_t);

    if(cmd & IOC_IN)
      if(size)
	bcopy(data, rdu->rdu_data, size);
      else
	*((caddr_t *) rdu->rdu_data) = *((caddr_t *) data);
    else
      if(cmd & IOC_VOID)
	*((caddr_t *) rdu->rdu_data) = *((caddr_t *) data);

    rdu->rdu_dev      = dev;
    rdu->rdu_ioc.com  = cmd;
    rdu->rdu_ioc.flag = flag;

    msg = dm_send(msg, DM_SLEEP | DM_RELEASE_REQUEST | DM_INTERRUPTABLE,
		  DMNDR_IOCTL,
		  devsite(dev),
		  RDS_SIZE((cmd & IOC_OUT) ? size : sizeof(u_int)),
		  NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    error = DM_RETURN_CODE(msg);

    if(error == 0 && (cmd & IOC_OUT) && size)
      bcopy(DM_CONVERT(msg, rds_t)->rds_data, data, size);

    dm_release(msg, 0);

    return(error);
#else
	panic("rdu_ioctl: remote devices not supported");
#endif
}

/*ARGSUSED*/
rdu_select(dev, which)
dev_t dev;
int which;
{ 
#ifdef FULLDUX
	register rdu_t *rdu;
    register dm_message msg;
    register int selected;

    msg = dm_alloc(RDU_SIZE(0), WAIT);
    rdu = DM_CONVERT(msg, rdu_t);

    rdu->rdu_dev   = dev;
    rdu->rdu_which = which;

    msg = dm_send(msg, DM_SLEEP|DM_RELEASE_REQUEST|DM_INTERRUPTABLE,
		  DMNDR_SELECT,
		  devsite(dev), RDS_SIZE(0),
		  NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    selected = DM_RETURN_CODE(msg);
    dm_release(msg, 0);
    return(selected);
#else
	panic("rdu_select: remote devices not supported");
#endif
}

static int 
rdu_intr(reqp, resp)
register dm_message reqp, resp;
{ 
	register rdu_t *reqst;
    register rds_t *respn;
    register struct buf *bp;

    reqst = DM_CONVERT(reqp, rdu_t);
    respn = DM_CONVERT(resp, rds_t);

    bp = DM_BUF(reqst->rdu_strat.flags & B_READ ? resp : reqp);

    if((bp->b_error = DM_RETURN_CODE(resp)))           /*an error occurred*/
      bp->b_flags |= B_ERROR;

    /*  Always set the resid to what came back				*/
    bp->b_resid  = respn->rds_resid;

    /*complete the I/O*/
    iodone(bp);
    dm_release(reqp,0);
    dm_release(resp,0);
}
