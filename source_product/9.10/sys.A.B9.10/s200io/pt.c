/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/pt.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:16:51 $
 */
/* HPUX_ID: @(#)pt.c	55.1		88/12/23 */

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
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../wsio/tryrec.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../wsio/iobuf.h"
#include "../h/uio.h"

/* misc. constants */
#define		PT_SIZE	1024
#define 	PTMAX 16


#define STIMEOUT	HZ/4	/* short timeout for pre/postamble */
#define LTIMEOUT	HZ*900	/* long timeout for transfers */

/* storage local to driver */
struct PT_bufs {
	struct buf	*r_getebuf;	/* pointer for brelse */
	struct buf	r_bpbuf;	/* buf for io request */
	struct iobuf    r_iobuf;	/* iobuf for hpib stuff */
	char		r_inbuf[PT_SIZE]; /* raw input buffer from user land */
};

struct PT_stat {
	long		t_flags;	/* busy, wanted flag */
	dev_t		t_dev;
	struct PT_bufs *t_bufs;		/* buf, iobuf, user buf */
} pt_st[PTMAX];

pt_open(dev, flag)	
register dev_t dev;
int flag;
{
	register struct PT_stat 	*tp = &pt_st[0];
	register struct buf 		*bp;
	register struct isc_table_type	*sc;
	register			error = 0;
	register int 			n;

	if (m_flags(dev)) {
printf("\nptopen: new type minor number (0xSSBB00) required\n\n"); /* temp */
		return(ENXIO);
	}
	if (m_selcode(dev) > 31) 
		return (ENXIO);
	if ((sc = isc_table[m_selcode(dev)]) == NULL) /* error out if no card*/
		return(ENXIO);

	if (sc->card_type != HP98625 &&
	    sc->card_type != HP98624 &&
	    sc->card_type != INTERNAL_HPIB) 
		return(ENXIO);

	for (n = PTMAX;; tp++) { /* find entry in tp for this printer */
		if ((((tp->t_dev ^ dev) & M_DEVMASK) == 0) || tp->t_dev == NULL)
			break;
		if (--n <= 0)
			return(ENXIO); /* too many plotters */
	}
	if (tp->t_bufs == NULL) { /* allocate buffer if not allocated */
		acquire_PTbuf(tp); /* make sure another open not in progress */
		bp = geteblk(sizeof(struct PT_bufs)); /* allocate buffers */
		tp->t_bufs = (struct PT_bufs *)bp->b_un.b_addr; /*get address*/
		bzero((caddr_t)tp->t_bufs, sizeof(struct PT_bufs)); /* clear */
		tp->t_bufs->r_getebuf = bp; 	/* save for brelse() */
		bp = &tp->t_bufs->r_bpbuf;	/* build buf for this driver */
		bp->b_spaddr = tp->t_bufs->r_getebuf->b_spaddr;
		bp->b_sc = sc;
		bp->b_ba = m_busaddr(dev);
		bp->b_queue = &tp->t_bufs->r_iobuf;
		bp->b_dev = tp->t_dev = dev;
		release_PTbuf(tp); /* now buffer is available */
	}
	return(0);
}


pt_close (dev, flag)         /* close hpib printer */
register dev_t dev;
register int flag;
{
	struct PT_stat *tp = &pt_st[0];
	register n;

	for (n = PTMAX;; tp++) { /* find entry in tp for this plotter */
		if (tp->t_dev == dev)
			break;
		if (--n <= 0)
			panic("pt: could not find dev");
	}
	if (tp->t_bufs != NULL) {
		brelse(tp->t_bufs->r_getebuf);
		tp->t_bufs = NULL;
	}
	tp->t_flags = 0;
}

int PT_transfer();

pt_read(dev, uio)
register dev_t dev;
struct uio *uio;
{
	register char *cp;
	struct PT_stat *tp = &pt_st[0];
	register int n, error = 0;
	
	for (n = PTMAX;; tp++) { /* find entry in tp for this printer */
		if (tp->t_dev == dev)
			break;
		if (--n <= 0)
			panic("pt: could not find dev");
	}
	acquire_PTbuf(tp); /* make sure buffer is available */
	cp = &tp->t_bufs->r_inbuf[0];
	try
		n = min(PT_SIZE, (unsigned)uio->uio_resid);
		if (error = PT_control(PT_transfer, tp, n, B_READ))
			escape(1);
		if (error = uiomove(cp, n - tp->t_bufs->r_bpbuf.b_bcount,
			UIO_READ, uio))
				escape(1);
	recover { 
		if (error == 0)
			error = EIO;
	}
	release_PTbuf(tp); /* now buffer is available */
	return(error);
}

pt_write(dev, uio)                
register dev_t dev;
struct uio *uio;
{
	register char *cp;
	struct PT_stat *tp = &pt_st[0];
	register int n;
	int error = 0;
	
	for (n = PTMAX;; tp++) { /* find entry in tp for this printer */
		if (tp->t_dev == dev)
			break;
		if (--n <= 0)
			panic("pt: could not find dev");
	}
	acquire_PTbuf(tp); /* make sure buffer is available */
	cp = &tp->t_bufs->r_inbuf[0];
	try
		while (n = min(PT_SIZE, (unsigned)uio->uio_resid)) {
			if (error = uiomove(cp, n, UIO_WRITE, uio))
				break;
			if (error = PT_control(PT_transfer, tp, n, B_WRITE))
				break;
		}
	recover { 
		if (error == 0)
			error = EIO;
	}
	release_PTbuf(tp); /* now buffer is available */
	return(error);
}

/*
 *   keep the queue going after a software trigger so that other 
 *   processes won't hang 
 */ 
PT_dumb(bp)
register struct buf *bp;
{
	register int i;

	PT_transfer(bp);
	do {
		i = selcode_dequeue(bp->b_sc);
		i += dma_dequeue();
	}
	while (i);
}

enum states { start=0,tfr,report,timedout, defaul};

/*
 *  Run this driver at interupt level 0 so that the serial card 
 *  won't drop charcters.
 */
PT_intm(bp)
register struct buf *bp;
{
	struct iobuf *iob;
	int x;

	iob = bp->b_queue;

	/* find out the interupt level */
	x = spl6();
	splx(x);

	/*
	 *  incase interupt level is 0 just call the state machine 
	 *  otherwise do software trigger
	 */

	if (x != bp->b_sc->int_lvl) {
		x = splsx(1);
		PT_transfer(bp);
		splsx(x);
	}
	else {
		sw_trigger(&iob->intloc, PT_dumb, bp, 0, 1);
	}
}

PT_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,PT_dumb,0,1,timedout);
}


PT_transfer(bp)
register struct buf *bp;
{
	register struct iobuf *iob;

	iob = bp->b_queue;

	try
	    START_FSM;
	    switch((enum states) iob->b_state) {

	case start:
		iob->b_state = (int)tfr;
		get_selcode(bp,PT_intm);
		break;

	case tfr: 

		iob->b_xaddr = bp->b_un.b_addr;
		bp->b_resid = iob->b_xcount = bp->b_bcount;
		START_TIME(PT_timeout, STIMEOUT);
		(*bp->b_sc->iosw->iod_preamb)(bp,0);
		END_TIME;
		iob->b_state = (int)report;
		START_TIME(PT_timeout, LTIMEOUT);
		(*bp->b_sc->iosw->iod_tfr)(MAX_OVERLAP,bp,PT_intm);
		break;

	case report:
		/* number of bytes left to be trasfered */
		bp->b_bcount = bp->b_sc->resid;
		END_TIME;
		START_TIME(PT_timeout, STIMEOUT);
		(*bp->b_sc->iosw->iod_postamb)(bp);
		END_TIME;
		iob->b_state = (int)defaul;
		drop_selcode(bp);
		queuedone(bp);
		break;

	case timedout:
		escape(TIMED_OUT);

	default:
		printf("Unrecognized PT state\n");

	}
	END_FSM;
	recover {
		iob->b_state = (int)defaul;
		ABORT_TIME;
		/*
		 *  In case there is an io operation in progress then
		 *  this whipes out DMA's nose. Also it there is a wait
		 *  on ppoll this will clear that too 
		 */
		try
			HPIB_ppoll_clear(bp);
			(*bp->b_sc->iosw->iod_abort)(bp);
		recover {}
		bp->b_error = EIO;
		drop_selcode(bp);
		queuedone(bp);
	}

}

/* This is the key interface routine to the low level drivers. */
PT_control(proc, tp, len, mode)
register int (*proc)();
register struct PT_stat *tp;
{
	register struct buf *bp = &tp->t_bufs->r_bpbuf;

	if (bp->b_flags & B_BUSY)
		panic ("pt bug -- bp should not be busy!");
	bp->b_flags = B_BUSY;
	if (mode)
		bp->b_flags |= B_READ;
	else 
		bp->b_flags &= ~B_READ;
	bp->b_error = 0;
	bp->b_un.b_addr = (caddr_t)&tp->t_bufs->r_inbuf[0];
	bp->b_bcount = len;
	bp->b_action = proc;

	enqueue(bp->b_queue, bp);

	iowait(bp);

	bp->b_flags = 0; /* take out when above panic is removed */

	u.u_error = 0; /* !@#$%^& */ 
	return(bp->b_error);
}

acquire_PTbuf(tp)
register struct PT_stat *tp;
{
	int s;
	s = spl6();
	while (tp->t_flags&B_BUSY) {
		tp->t_flags |= B_WANTED;
		sleep((caddr_t)tp, PRIBIO+1);
	}
	tp->t_flags = B_BUSY;
	splx(s);
}

release_PTbuf(tp)
register struct PT_stat *tp;
{
	int s;
	s = spl6();
	if (tp->t_flags&B_WANTED)
		wakeup((caddr_t)tp);
	splx(s);
	tp->t_flags &= ~(B_BUSY|B_WANTED);
}
