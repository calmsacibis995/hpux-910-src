/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ciper.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:10:33 $
 */
/* HPUX_ID: @(#)ciper.c	55.1		88/12/23 */

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
#include "../h/file.h" /* For FWRITE */
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../wsio/tryrec.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../wsio/iobuf.h"
#include "../h/uio.h"
#include "../h/lprio.h"
#include "../s200io/ciper.h"

struct dev_clear device_clear0 = {
	4,1,0,
	4,0,DEV_CLR,
	0,1,1,0,
	0				/* do not clear buffers */
};

struct cip_config cip_configure = {
	4,1,0,
	4,0,CIP_CONF,
	0,1,1,0,
	1
};

struct cip_generic rep_dev_stat = {
	4,1,0,
	4,0,DEV_STAT,
	0,1,1,0
};

struct write_header ciper_block = {
	4,1,0,		/* packet header */
	4,0,CIP_WR,	/* record header */
	0,1,1,0,
	6,0,1		/* block header */
};

static struct lprio lpr_defaults = {
	4,		/* indentation		*/
	132,		/* columns per line 	*/
	66,		/* lines per page	*/
	PASSTHRU,	/* backspace handling 	*/
	0,		/* ejects on open	*/
	1,		/* ejects on close	*/
	COOKED_MODE	/* raw_mode is cleared  */
};

static struct lprio lpr_defaults_raw  = {
	4,		/* indentation		*/
	132,		/* columns per line 	*/
	66,		/* lines per page	*/
	PASSTHRU,	/* backspace handling 	*/
	0,		/* ejects on open	*/
	1,		/* ejects on close	*/
	RAW_MODE	/* raw_mode is cleared  */
};

struct CIP_stat {
	dev_t	t_dev;
	struct CIP_bufs *t_bufs;	/* buf, iobuf, input buf, output buf */
	unsigned short	t_ccc;		/* current input position in line */
	unsigned short	t_mcc;		/* current output position in line */
	unsigned short	t_mlc;		/* current line count on page */
	unsigned short	t_flag;			/* open, used etc. flag */
	unsigned short	t_rec_rdy_cnt;	/* receive ready count */
	unsigned short	t_send_recno;	/* send record number */
	char *		t_curbufp;	/* current output buffer pointer */
	char *		t_topbufp;	/* top o buffer pointer */
	struct lprio 	t_lprio;	/* printer defaults */
} cip_st[CIPMAX];

extern HPIB_identify();
int CIP_timeout();
int CIP_transfer();
int cip_do_SAB();

enum states
{	start=0,command,get1,tfr,checkit,oh_oh,ok_ok,report,get3,
	timedout, defaul
};

/* to be used to remove timeouts */
/*
#define START_TIME(a,c)
#define END_TIME
*/

ciper_open(dev, flag)	
register dev_t dev;
register flag;
{
	register struct CIP_stat 	*tp = &cip_st[0];
	register struct buf 		*bp;
	register struct isc_table_type	*sc;
	register int error = 0;
	register int n;
	register int i;
	register struct write_header *addr;


	if (!(flag & FWRITE)) 		/* error out if not write */
		return(EINVAL);
	n = m_selcode(dev);
	if (n > 31) {
		return(EINVAL);
	}
	if ((sc = isc_table[n]) == NULL) /* error out if no card*/
		{
		printf("Error out if no card\n");
		return(ENXIO);
		}

	if (sc->card_type != HP98625 &&
	    sc->card_type != HP98624 &&
	    sc->card_type != INTERNAL_HPIB) {
		printf("Error out if card not HPIB\n");
		return(ENXIO);
	}

	/* if driver opened raw, no other flag bits can be set */
	if ((dev & RAW) && (m_flags(dev) != RAW))
		{
		printf("Error out if open is raw and other bits set\n");
		return(ENXIO);
		}

	for (n = CIPMAX;; tp++) { /* find entry in tp for this printer */
		if ((((tp->t_dev ^ dev) & M_DEVMASK) == 0) || tp->t_dev == NULL)
			break;
		if (--n <= 0) {
			printf("Too many printers\n");
			return(ENXIO); /* too many printers */
			}
	}

	if (tp->t_flag & CIP_LOCK)	/* lock open call until buffer gotten */
		{
	        printf("CIP LOCK set\n");
		return(ENXIO);
		}

	/* Disallow multiple opens */
	if (tp->t_flag & (CIP_OPEN|CIP_ROPEN))  
		{
	        printf("Attempting multiple opens\n");
		return(ENXIO);
		}

	/* only on 1st open of this printer */
	if (tp->t_dev == NULL || (tp->t_flag & CIP_RESET)) {
		if ((dev & RAW) || (tp->t_lprio.raw_mode))
			tp->t_lprio = lpr_defaults_raw;
		else
			tp->t_lprio = lpr_defaults;
			
		tp->t_flag &= ~CIP_RESET;
	}
	tp->t_ccc = tp->t_lprio.ind;
	tp->t_mcc = 0;
	if ((tp->t_dev & (NO_EJT)) == 0)	/* true if flag not set or */
		tp->t_mlc = 0;			/* this is first open      */

	tp->t_flag |= CIP_LOCK; /* prevent interference while getting buf */
	tp->t_flag &= ~CIP_ERROR; /* clear out any errors */
	bp = geteblk(sizeof(struct CIP_bufs)); /* allocate needed buffers */

	if (dev & RAW) { 
		tp->t_flag |= CIP_ROPEN; /* mark the printer raw open */
		tp->t_lprio.raw_mode = RAW_MODE;
        }
	else
		tp->t_flag |= CIP_OPEN; /* mark the printer normal open */
	tp->t_flag &= ~CIP_LOCK; /* and unlock the open */

	tp->t_bufs = (struct CIP_bufs *)bp->b_un.b_addr; /* get the address of buffers */
	bzero((caddr_t)tp->t_bufs, sizeof(struct CIP_bufs)); /* clear them */
	tp->t_bufs->r_getebuf = bp; 	/* save for brelse() */

	bp = &tp->t_bufs->r_bpbuf;	/* build buf for this driver */
	bp->b_spaddr = tp->t_bufs->r_getebuf->b_spaddr;
	bp->b_sc = sc;
	bp->b_ba = m_busaddr(dev);
	bp->b_queue = &tp->t_bufs->r_iobuf;
	bp->b_dev = tp->t_dev = dev;
	tp->t_curbufp = &tp->t_bufs->r_outbuf[sizeof (struct write_header)];
	tp->t_topbufp = &tp->t_bufs->r_outbuf[CIP_PSIZE];
	addr = (struct write_header *)&tp->t_bufs->r_outbuf[0];
	*addr = ciper_block;
	if (error = CIP_identify(tp))	/* what kind of printer */
		goto errout;

	/* send a device clear */
	try
		tp->t_flag &= ~CIP_PWF;		/* powerfail ok in here */
		send_device_clear(tp);
		cip_send(tp, &cip_configure, sizeof (cip_configure));
		obtain_ciper_status(tp);
		tp->t_flag |= CIP_PWF;		/* cannot tolerate powerfail */

		/* Print opening form feeds */
		if (((tp->t_dev & (RAW|NO_EJT)) == 0)	/* dev is set by now */
		   && (!tp->t_lprio.raw_mode))
		      	for (i=0; i < tp->t_lprio.open_ej; i++)
				cip_tof(tp, '\f');
	recover {
		error = EIO;
		goto errout;
	}
	return(0);
errout:
	brelse(tp->t_bufs->r_getebuf);
	tp->t_flag &= ~(CIP_OPEN|CIP_ROPEN);
	tp->t_lprio.raw_mode = COOKED_MODE;
	return(error);
}

ciper_close (dev, flag)
register dev_t dev;
register int flag;
{
	register struct CIP_stat *tp = &cip_st[0];
	register n;
	int i;

	for (n = CIPMAX;; tp++) { /* find entry in tp for this printer */
		if (((tp->t_dev ^ dev) & M_DEVMASK) == 0)
			break;
		if (--n <= 0)
			panic("cip: could not find dev");
	}

	if ((tp->t_flag & CIP_ERROR) == 0) {
		try
			if (((tp->t_dev & (RAW|NO_EJT)) == 0) &&
		   	   (!tp->t_lprio.raw_mode))

                        /* print closing page ejects */
			   for (i=0; i < tp->t_lprio.close_ej; i++) 
				cip_tof(tp, '\f');

			cip_send_block(tp);
			while (tp->t_rec_rdy_cnt <=0)
				cip_receive(tp, &tp->t_bufs->r_cipbuf,
						sizeof (union cip_recs));
		recover {}
	}
	if (dev & RAW) 
		tp->t_flag &= ~CIP_ROPEN;

	else
		tp->t_flag &= ~CIP_OPEN;

	if (tp->t_flag & (CIP_OPEN|CIP_ROPEN)) return;
	brelse(tp->t_bufs->r_getebuf);
}

ciper_write(dev, uio)                
register dev_t dev;
register struct uio *uio;
{
	register char *cp;
	register struct CIP_stat *tp = &cip_st[0];
	register int n;
	register int error = 0;

	for (n = CIPMAX;; tp++) { /* find entry in tp for this printer */
		if (((tp->t_dev ^ dev) & M_DEVMASK) == 0)
			break;
		if (--n <= 0)
			panic("cip: could not find dev");
	}

	if (tp->t_flag & CIP_ERROR) return(EIO);
	try
		while (n = min(CIP_INBUF, (unsigned)uio->uio_resid)) {
			cp = &tp->t_bufs->r_inbuf[0];
			if (error = uiomove(cp, n, UIO_WRITE, uio))
				break;
			if ((dev & RAW) || (tp->t_lprio.raw_mode))
			{	do
					cip_putchar(tp,*cp++);
				while (--n);
				tp->t_mcc = 0;
			}
			else
			{	do
					cipoutput(tp, *cp++);
				while (--n);
			}
		}
	recover { 
		error = EIO;
	}
	if (error)
		tp->t_flag |= CIP_ERROR;
	else
		tp->t_flag &= ~CIP_ERROR;
	return(error);
}


ciper_ioctl(dev, cmd, data, flag)
dev_t dev;
register struct lprio *data;
{
	struct CIP_stat *tp = &cip_st[0];
	register n;

	for (n = CIPMAX;; tp++) { /* find entry in tp for this printer */
		if (((tp->t_dev ^ dev) & M_DEVMASK) == 0)
			break;
		if (--n <= 0)
			panic("cip: could not find dev");
	}

	switch(cmd) {
	case LPRGET:
		data->ind = tp->t_lprio.ind;
		data->col = tp->t_lprio.col;
		data->line = tp->t_lprio.line;
		data->bksp = tp->t_lprio.bksp;
		data->open_ej = tp->t_lprio.open_ej;
		data->close_ej = tp->t_lprio.close_ej;
		data->raw_mode = tp->t_lprio.raw_mode;
		break;
	case LPRSET:
		if (data->col == 0)
		{	tp->t_flag |= CIP_RESET;
			break;
		}
		n = tp->t_lprio.ind;
		tp->t_lprio.ind = min((unsigned)data->ind, CIP_PSIZE);
		if (tp->t_ccc == n)
			tp->t_ccc = tp->t_lprio.ind;
		tp->t_lprio.col = min((unsigned)data->col, CIP_PSIZE);
		tp->t_lprio.line = data->line;
		tp->t_lprio.bksp = data->bksp;
		tp->t_lprio.open_ej = data->open_ej;
		tp->t_lprio.close_ej = data->close_ej;
		tp->t_lprio.raw_mode = data->raw_mode;
		break;
	default:
		return(EINVAL);
	}
	return(0);
}

CIP_identify(tp)
register struct CIP_stat *tp;
{
	register char *identify = (char *)(&tp->t_bufs->r_cipbuf);
	register struct buf *bp = &tp->t_bufs->r_bpbuf;

	bp->b_clock_ticks = XFRTIME;
	if ((CIP_control(HPIB_identify, tp, 2, identify, 0) == 0) &&
	    (*identify++ == 0x21 && *identify == 1))
		return 0;
	else return(EIO);
}

send_device_clear(tp)
register struct CIP_stat *tp;
{
	register struct buf *bp = &tp->t_bufs->r_bpbuf;
	register union cip_recs *cip_record = &tp->t_bufs->r_cipbuf;
	register struct dev_clear *dc;

	/* send SAB secondary */
	if (CIP_control(cip_do_SAB, tp, 1, cip_record, 0) != 0)
		escape(1);

	/* set receive ready count to one so write won't hang */
	tp->t_rec_rdy_cnt = 1;

	/* send device clear */
	cip_send(tp, &device_clear0, sizeof (struct dev_clear));

	do {
		/* receive clear response */
		cip_receive(tp,cip_record,sizeof(union cip_recs));
	} while ( cip_record->clear_response.rh.code != CLR_RSP );

	tp->t_rec_rdy_cnt = 0;
	tp->t_send_recno = 0;
}

obtain_ciper_status(tp)
register struct CIP_stat *tp;
{
	register struct buf *bp = &tp->t_bufs->r_bpbuf;
	register union cip_recs *cip_record = &tp->t_bufs->r_cipbuf;

	do {
		/* send report device status */
		cip_send(tp, &rep_dev_stat, sizeof (rep_dev_stat));

		/* receive device status report */
		cip_receive(tp,cip_record,sizeof(union cip_recs));
	} while ( cip_record->device_status.rh.code != DEV_STAT ||
		 !(cip_record->device_status.dstat[0] & CIP_ON_LINE) ||
		 (cip_record->device_status.dstat[1] & CIP_ST_PWF));
}
/* Do a top-of-form */
cip_tof(tp, c)
register struct CIP_stat *tp;
register c;
{
	cip_putchar(tp,'\r');
	cip_putchar(tp,c);
	tp->t_mlc = 0;
	tp->t_mcc = 0;
	tp->t_ccc = tp->t_lprio.ind; /* set the indent */
}


cipoutput(tp, c)
register struct CIP_stat *tp;
register c;
{
	switch(c) {
	case '\t':
		tp->t_ccc=((tp->t_ccc+8-tp->t_lprio.ind) & ~7)+tp->t_lprio.ind;
		return;
	case '\n':
		tp->t_mlc++;
 		if ((tp->t_lprio.line > 0) && (tp->t_mlc >= tp->t_lprio.line))
			c = '\f'; /* do form feed instead of newline */
	case '\f':
		/* check if any lines on page */
		if (tp->t_mlc || 
		    tp->t_mcc > tp->t_lprio.ind || 
		    tp->t_ccc > tp->t_lprio.ind)
		{	cip_putchar(tp,'\r');
			cip_putchar(tp,c);
			if (c == '\f')
				tp->t_mlc = 0;
		}
		tp->t_mcc = 0;
	case '\r': /* special case -- treat like a lot of backspaces */
		tp->t_ccc = tp->t_lprio.ind; /* set the indent */
		return;
	case '\b': /* special case because some printers can't handle */
		if (tp->t_ccc > tp->t_lprio.ind) { /* decrement col counter */
			tp->t_ccc--;
                	if (tp->t_lprio.bksp == PASSTHRU) {
				cip_putchar(tp,'\b');
				tp->t_mcc = tp->t_ccc;
			}
                }
		return;
	case ' ':
		tp->t_ccc++;
		return;
	default:
		if(tp->t_ccc < tp->t_mcc) { /* check if backspaced !! */
			cip_putchar(tp,'\r');
			tp->t_mcc = 0;
		}
		if(tp->t_ccc < tp->t_lprio.col) { /* eat chars past max col */
			while(tp->t_ccc > tp->t_mcc) { /* pad spaces until */
				cip_putchar(tp,' ');
			}
			cip_putchar(tp,c);
		}
		tp->t_ccc++;
	}
}

cip_putchar(tp,c)
register struct CIP_stat *tp;
char c;
{
	*tp->t_curbufp++ = c;
	tp->t_mcc++;
	if (tp->t_curbufp >= tp->t_topbufp)
		cip_send_block(tp);
}

cip_sleep(bp)
struct buf *bp;
{
	wakeup((caddr_t)&bp->b_sc);
}

cip_send_block(tp)
register struct CIP_stat *tp;
{
	register struct buf *bp = &tp->t_bufs->r_bpbuf;
	register int x;

tryloop:
	try
		cip_send(tp, tp->t_bufs->r_outbuf,
				tp->t_curbufp-tp->t_bufs->r_outbuf);
	recover
	{	/* the identify here is used to see if the device is still
		   on line. if it is sleep and try again else escape
		   if escapecode is 3 then powerfail has been indicated by
		   the device status report--escape in this case also */
		if (escapecode == 3) escape(1);
		if (CIP_identify(tp))
		{	tp->t_curbufp = &tp->t_bufs->r_outbuf[sizeof (struct write_header)];
			escape(1);
		}
		else 
		{
			x = spl6();
			timeout(cip_sleep,bp,LONGTIME,0);
			sleep(&bp->b_sc,PRIBIO);
			splx(x);
			goto tryloop;
		}
	}
	tp->t_curbufp = &tp->t_bufs->r_outbuf[sizeof (struct write_header)];
}


cip_send(tp,crec,len)
register struct CIP_stat *tp;
register union cip_recs *crec;
{
	register struct buf *bp = &tp->t_bufs->r_bpbuf;

	crec->gen.rh.record_num = tp->t_send_recno;
	while (tp->t_rec_rdy_cnt <=0)
		cip_receive(tp, &tp->t_bufs->r_cipbuf, sizeof(struct cip_recs));

	if (CIP_control(CIP_transfer, tp, len, crec, 1) != 0)
		escape(1);

	tp->t_rec_rdy_cnt -= 1;
	tp->t_send_recno += 1;
}

cip_receive(tp, crec, len)
register struct CIP_stat *tp;
register union cip_recs *crec;
{
	register struct buf *bp = &tp->t_bufs->r_bpbuf;
	register int top;

	if (CIP_control(CIP_transfer, tp, len, crec, 0) != 0)
		escape(1);
	switch (crec->gen.rh.code) {
	case REC_RDY:
		if (bp->COUNT != SIZEOF_REC_RDY)
			escape(2);
		tp->t_rec_rdy_cnt += crec->receive_ready.num_recs;
		break;
	case CLR_RSP:
		if (bp->COUNT != sizeof (struct clear_resp))
			escape(2);
		top = MIN(crec->clear_response.rec_buf_size*16,
					 CIP_PSIZE);
		tp->t_topbufp = &tp->t_bufs->r_outbuf[top];
		break;
	case DEV_STAT:
		if (bp->COUNT != sizeof (struct dev_stat))
			escape(2);
/*		printf("device status received 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			crec->device_status.dstat[0],
			crec->device_status.dstat[1],
			crec->device_status.dstat[2],
			crec->device_status.dstat[3],
			crec->device_status.dstat[4],
			crec->device_status.dstat[5]);	*/
		if ( crec->device_status.dstat[4] != 0 ||
			crec->device_status.dstat[5] != 0)
			escape(3);
		if ((crec->device_status.dstat[1] & CIP_ST_PWF) &&
		    (tp->t_flag & CIP_PWF))
			escape(3);
		break;
	default:
/*		printf("CIP: unrecognized record received type %d\n",
			crec->gen.rh.code);	*/
		escape(3);
	}
}

SAB_dumb(bp)
register struct buf *bp;
{
	register int i;

	cip_do_SAB(bp);
	do {
		i = selcode_dequeue(bp->b_sc);
		i += dma_dequeue();
	}
	while (i); 
}

/* timeout routine for SAB state machine */
SAB_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,SAB_dumb,0,1,timedout);
}

/* this mini state machine is used to send the SAB command to the printer */
cip_do_SAB(bp)
register struct buf *bp;
{
	register struct iobuf *iob;
	register struct drv_table_type *isw;

	iob = bp->b_queue;
	isw = bp->b_sc->iosw;

	try
	START_FSM;
	switch (iob->b_state) {

	case start:
		iob->b_state = (int)tfr;
		get_selcode(bp,cip_do_SAB);
		break;

	case tfr:
		iob->b_xaddr = &bp->DSJ_BYTE;
		iob->b_xcount = 1;
		bp->b_resid = bp->b_bcount = 1;
		bp->b_flags |= B_READ;

		START_TIME(SAB_timeout,SHORTTIME);
		(*isw->iod_preamb)(bp,CIP_SEC_SAB);
		END_TIME

		iob->b_state = (int)report;
		START_TIME(SAB_timeout,XFRTIME);
		(*isw->iod_tfr)(MAX_OVERLAP,bp,cip_do_SAB);
		break;
	
	case report:
		END_TIME
		START_TIME(SAB_timeout,SHORTTIME);
		(*isw->iod_postamb)(bp);
		END_TIME

		bp->b_flags &= ~B_READ;
		START_TIME(SAB_timeout,SHORTTIME);
		(*isw->iod_preamb)(bp,CIP_SEC_SQC);
		END_TIME
		START_TIME(SAB_timeout,SHORTTIME);
		(*isw->iod_postamb)(bp);
		END_TIME
		iob->b_state = (int)defaul;
		drop_selcode(bp);
		queuedone(bp);
		break;

	case timedout:
		escape(TIMED_OUT);

	default:
		panic("Unrecognized CIP state");

	}
	END_FSM;
	recover {
		iob->b_state = (int)defaul;
		try
			HPIB_ppoll_clear(bp);
			(*isw->iod_abort)(bp);
		recover{}
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		drop_selcode(bp);
		queuedone(bp);
	}
	return 0;
}

/* Keep the queue going after a software trigger */
CIP_dumb(bp)
register struct buf *bp;
{
	register int i;

	CIP_transfer(bp);
	do {
		i = selcode_dequeue(bp->b_sc);
		i += dma_dequeue();
	}
	while (i); 
}

CIP_transfer(bp)
register struct buf *bp;
{
	register struct iobuf *iob;
	register int x,i;
	register int dsjbyte;
	register int secondary;
	register struct drv_table_type *isw;


	iob = bp->b_queue;
	isw = bp->b_sc->iosw;

	try
	START_FSM;
	switch((enum states) iob->b_state) {

	case start:
		iob->CIP_RW = bp->BP_RW;
		iob->b_state = (int)command;
		get_selcode(bp,CIP_transfer);
		break;

	case command: 
		/*
		 * send ERD/RWT and wait for ppoll int
		 */
		START_TIME(CIP_timeout,LONGTIME);
		if (iob->CIP_RW == 0) secondary = CIP_SEC_ERD;
		else secondary = CIP_SEC_RWT;
		(*isw->iod_preamb)(bp,secondary);
		END_TIME
		START_TIME(CIP_timeout,SHORTTIME);
		(*isw->iod_postamb)(bp);
		END_TIME
		iob->b_state = (int)get1;
		START_TIME(CIP_timeout,LONGTIME);
		x = splx(bp->b_sc->int_lvl);
		HPIB_ppoll_int(bp,CIP_transfer,1);
		drop_selcode(bp);
		splx(x);
		break;

	case get1:
		END_TIME
		iob->b_state = (int)tfr;
		get_selcode(bp,CIP_transfer);
		break;

	case tfr:
		if (iob->CIP_RW == 0)
			bp->b_flags |= B_READ;
		else
			bp->b_flags &= ~B_READ;
		START_TIME(CIP_timeout,SHORTTIME);
		(*isw->iod_preamb)(bp,CIP_SEC_WRD);
		END_TIME

		/* set up for transfer */
		iob->b_xaddr = bp->b_un.b_addr;
		bp->b_resid = bp->b_bcount;
		iob->b_xcount = bp->b_bcount;
		/* 
		   allow a longer timeout in overlap mode because
		   it doesn't hang the system.
		*/
		iob->b_state = (int)checkit;
		START_TIME(CIP_timeout,XFRTIME);
		(*isw->iod_tfr)(MAX_OVERLAP,bp,CIP_transfer);
		break;

	case checkit:	/* send DSJ */
		END_TIME

		/* save away the number of bytes transfered */
		bp->COUNT = bp->b_bcount - bp->b_sc->resid;

		START_TIME(CIP_timeout,SHORTTIME);
		(*isw->iod_postamb)(bp);
		END_TIME
		iob->b_xaddr = &bp->DSJ_BYTE;
		iob->b_xcount = 1;
		bp->b_resid = bp->b_bcount = 1;
		bp->b_flags |= B_READ;
		START_TIME(CIP_timeout,SHORTTIME);
		(*isw->iod_preamb)(bp,CIP_SEC_DSJ);
		END_TIME
		iob->b_state = (int)report;
		START_TIME(CIP_timeout,XFRTIME);
		(*isw->iod_tfr)(MAX_OVERLAP,bp,CIP_transfer);
		break;

	case report:	/* check response--if error reported
			   send SAB else send SQC */
		END_TIME
		START_TIME(CIP_timeout,SHORTTIME);
		(*isw->iod_postamb)(bp);
		END_TIME
		iob->DSJ_SAVE = bp->DSJ_BYTE;
		if (bp->DSJ_BYTE)
		{	
			START_TIME(CIP_timeout,SHORTTIME);
			(*isw->iod_preamb)(bp,CIP_SEC_SAB);
			END_TIME
			iob->b_state = (int)ok_ok;
			START_TIME(CIP_timeout,XFRTIME);
			(*isw->iod_tfr)(MAX_OVERLAP,bp,CIP_transfer);
			break;
		} else {	

		/* do nothing--fall through and send SQC */

		}

	case ok_ok:
		/* if an error was detected and SAB was sent then send
		   a postamble
		*/
		bp->DSJ_BYTE = iob->DSJ_SAVE;
		if (bp->DSJ_BYTE)
		{
			END_TIME
			START_TIME(CIP_timeout,SHORTTIME);
			(*isw->iod_postamb)(bp);
			END_TIME
		}
		bp->b_flags &= ~B_READ;
		START_TIME(CIP_timeout,SHORTTIME);
		(*isw->iod_preamb)(bp,CIP_SEC_SQC);
		END_TIME
		START_TIME(CIP_timeout,SHORTTIME);
		(*isw->iod_postamb)(bp);
		END_TIME
		iob->b_state = (int)defaul;
		drop_selcode(bp);
		queuedone(bp);
		break;

	case timedout:
		escape(TIMED_OUT);

	default:
		panic("Unrecognized CIP state");

	}
	END_FSM;
	recover {
		iob->b_state = (int)defaul;
		try
			HPIB_ppoll_clear(bp);
			(*isw->iod_abort)(bp);
		recover{}
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		drop_selcode(bp);
		queuedone(bp);
	}
}


CIP_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,CIP_dumb,0,1,timedout);
}

/* This is the key interface routine to the low level drivers. */
CIP_control(proc, tp, len, addr, rw)
register int (*proc)();
register struct CIP_stat *tp;
char *addr;
{
	register struct buf *bp = &tp->t_bufs->r_bpbuf;

	/* make sure the buffer is available before we use it */
	acquire_buf(bp);

	bp->b_error = 0;
	bp->b_un.b_addr = addr;
	bp->b_bcount = len;
	bp->b_action = proc;
	bp->BP_RW = rw;
	enqueue(bp->b_queue, bp);
	iowait(bp);
	release_buf(bp);
	bp->b_flags = 0;
	u.u_error = 0; /* !@#$%^& */ 
	return(bp->b_error);
}
