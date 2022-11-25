/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/tp.c,v $
 * $Revision: 1.5.84.6 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/21 10:23:11 $
 */
/* HPUX_ID: @(#)	55.1	89/01/05	*/
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


/* #define TPDEBUG  /* Define for DEBUG */

#include "../h/param.h"
#include "../h/buf.h"
#include "../wsio/timeout.h"
#include "../wsio/iobuf.h"
#include "../h/systm.h"
#include "../s200io/dma.h"
#include "../wsio/hpibio.h"
#include "../h/ioctl.h"
#include "../h/mtio.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../wsio/tryrec.h"

#ifdef IOSCAN

#include "../h/libio.h"
#include "../wsio/dconfig.h"

int tp_config_identify();

extern int (*hpib_config_ident)();
#endif /* IOSCAN */

/* tunable parameters */
#define MAX_WRITE_RETRYS	18  /* Max Number of retries per write op */
#define MAX_READ_RETRYS		8  /* Max number of retries per read op  */

#define STATUS_SIZE 3		/* 3 (1 byte) status registers */

/* data needed for driver to function */
/* one of these are needed per active tape drive */
struct tp_status {
	long	tp_resid;
	char	tp_sbytes[STATUS_SIZE];	/* status result */
	char	tp_flag;		/* open */
	char	tp_data[STATUS_SIZE];	/* scratch area */
} tp_status;

/* tp_status flags */
#define TP_OPEN 	1
#define TP_EOT_FOUND	2

struct	buf tp_buf;	/* Only one buf per device */
struct iobuf tp_iobuf;	/* Only one iobuf per device */

#define SEC_DSJ		SCG_BASE+16

/* State info kept in (iobuf) b_flags */
#define T_WRITTEN 0x10000	/* True if last operation was a 'write' */
#define T_REWOUND 0x20000	/* True if last operation was a 'rewind' 
				 * or 'rewind_off_line */
#define T_EOF	  0x40000	/* True if last transfer encountered an EOF */
#define B_PPOLL_TIMEDOUT	B_SCRACH1

#define MT_READ MTNOP+1
#define MT_WRITE MTNOP+2

#define b_opcode b_s0		/* opcode for motion orders */
#define b_errcod b_s1
#define b_TO_state b_s3

/* Secondaries */
#define SEC_xDAT SCG_BASE+0
#define A_TSTAT	 SCG_BASE+1
#define A_TSAD   SCG_BASE+1
#define A_TBYTCT SCG_BASE+2
#define A_TEND   SCG_BASE+7

/* End Commands */
#define CLEAR_PP_RESP 	0x01	/* Clear Parallel Poll Response */
#define STOP_POLL	0x02	/* Inhibit PP response */
#define ENAB_POLL	0x04	/* Enable PP response  */
#define CLEAR_DSJ	0x010	/* Enable PP response  */

static char dsj;
#define	GET_DSJ	(*bp->b_sc->iosw->iod_mesg)(bp, 0, SEC_DSJ, &dsj, sizeof dsj)

/* the errors from tape, catagorized */
/* order sensitive, somewhat */
enum tp_errs {TE_FATAL,TE_OFFL,TE_BUSY,TE_RUN,TE_IOE,TE_EOF,TE_EOT,TE_BOT,TE_NONE};

/* define the actual orders for UNIX commands 
 * refer to mtio.h for actual mag_tape operations
 */
unsigned char cmdtab[MT_WRITE+1] =   /* add read and write */
	{0x06,0x0b,0x0c,0x09,0x0a,0x0d,0x0e,0,0x08,0x05};

/* "intersting" tape status bits */
#define IFC_BUSY 0001
#define UNT_BUSY 0002

int tape_transfer();

tp_strategy(bp)
register struct buf *bp;
{
	register struct iobuf *iob = &tp_iobuf;

	bp->b_sc = isc_table[m_selcode(bp->b_dev)];
	bp->b_ba = m_busaddr(bp->b_dev);
	bp->b_resid = bp->b_bcount;
	bp->b_action = tape_transfer;
	if (bp->b_flags & B_READ)	{
		bp->b_opcode = MT_READ;
		iob->b_errcnt = MAX_READ_RETRYS;
	}
	else	{
		bp->b_opcode = MT_WRITE;
		iob->b_errcnt = MAX_WRITE_RETRYS;
	}
	enqueue(iob, bp);
}

int RTtape_status();
extern HPIB_clear();
extern HPIB_identify();

tp_open(dev,flag)
dev_t dev;
int flag;
{
	short ident;
	char status[STATUS_SIZE];
	register struct isc_table_type *sc;
	register struct iobuf *iob = &tp_iobuf;
	register struct buf *bp = &tp_buf;
	register n;
	int error = 0;

	n = m_selcode(dev);	/* get select code */
	if (n > 31 || ((dev & MT_DENSITY_MASK) != MT_1600_BPI)) 
		return(EINVAL);
	if ((sc = isc_table[n]) == NULL) /* error out if no card*/
		return(ENXIO);


	if (tp_status.tp_flag&TP_OPEN) /* Exclusive Open per device */
		return(ENXIO);
	tp_status.tp_flag |= TP_OPEN;

	if (sc->card_type != HP98624 && sc->card_type != INTERNAL_HPIB) {
		error = ENXIO;
		goto errout;
	}
	bp->b_dev = dev;	/* store dev */
	bp->b_sc = sc;		/* temp until lower routine use dev_t */
	bp->b_ba = m_busaddr(dev);

	if(tape_control(HPIB_identify, (char *)&ident, sizeof ident) ||
		ident != 0x183) {
		error = ENXIO;
		goto errout;
	}

	if (tape_control(HPIB_clear, (char *)0, 0)) {
		error = ENXIO;
		goto errout;
	}

	/* ???? is this necessary */
	delay(2);

	tape_control(RTtape_status, status, STATUS_SIZE);
	if ((status[0] & 0x01) == 0) {
		error = EIO;
		goto errout;
	}
	if ((flag&FWRITE) && (status[0] & 0x04)) {
		/* Write Ring Missing / Write Protected */
		error = EIO;
		goto errout;
	}

	/* 7970 REQUIRES DMA - so let's reserve a channel */
	if (dma_reserve(sc) == 0) {
		error = EIO;
		goto errout;
	}

	iob->b_flags = 0;
	return(0);

errout:
	tp_status.tp_flag = 0;
	return(error);
}

tp_close(dev, flag)
dev_t dev;
int flag;
{
	register struct iobuf *iob = &tp_iobuf;

	if ((flag & FWRITE) && (iob->b_flags & T_WRITTEN)) {
		/* if file opened for 'WRITE', and last
		 * operation was a write, write 2
		 * EOF marks, then backspace one record.
		 */
		motion_control(MTWEOF,2);
		motion_control(MTBSR, 1);
	}
	if (!(dev & MT_NO_AUTO_REW))
		motion_control(MTREW,1);
	else	{
		if ((dev & MT_UCB_STYLE) == 0 && !(flag & FWRITE)
		    &&	(iob->b_flags & T_EOF) == 0) {
				/* not UCB, and not just past tape mark:
			   	go to past tape mark. */
				motion_control(MTFSF, 1);
		}
	}
	dma_unreserve(tp_buf.b_sc);
	tp_status.tp_flag = 0;
}

tape_control(proc, bfr, len)
int (*proc)();
register char *bfr;
register len;
{
	register char *ptr;
	register struct buf *bp = &tp_buf;

	acquire_buf(bp);

	bp->b_un.b_addr = (caddr_t)tp_status.tp_data;
	bp->b_bcount = len;

        bp->av_forw = NULL;
	bp->b_action = proc;

	enqueue(&tp_iobuf, bp);

	iowait(bp);

	ptr = tp_status.tp_data;
	while (len-- > 0) *bfr++ = *ptr++;
	release_buf(bp);
	return (bp->b_error);
}

int tape_motion();

motion_control(op, count)
int op;
long count;
{
	register struct buf *bp = &tp_buf;

	acquire_buf(bp);
        bp->av_forw = NULL;
	bp->b_resid = count;
	bp->b_action = tape_motion;
	bp->b_opcode = op;

	enqueue(&tp_iobuf, bp);

	iowait(bp);

	release_buf(bp);
	return (bp->b_resid);
}

tp_ioctl(dev,order,addr,flag)
dev_t dev;
int order;
caddr_t addr;
int flag;
{
	register struct tp_status *stt;
	register int op;
	register long count;
	struct mtop *mtop;
	register struct mtget *mtget;
	short ident;
	int err;
	struct io_hpib_ident *id_rec;

	stt = &tp_status;

	switch (order) {

	/*
	 * This ioctl is used by ioscan to read the hpib identify
	 * bytes from a device.
	 * If the identify worked then copy the id.
	 */
	case IO_HPIB_IDENT:
		err = tape_control(HPIB_identify, (char *)&ident, sizeof ident);
		if( !err )
		{
		     id_rec = (struct io_hpib_ident *)addr;
		     id_rec->ident = ident;
		     id_rec->dev_type = IOSCAN_HPIB_TAPE;  /* tape device */
		     return(0);
		}
		else
		     return(EIO);
		break;
	case MTIOCTOP:	/* control operation */
		mtop = (struct mtop *)addr;
		op = mtop->mt_op;
		if (op < 0 || op>MTNOP) 
			return(ENXIO);

		count = mtop->mt_count;

		if (op >= MTREW) count = 1;  /* nonsense to repeat these */
		if (motion_control(op, count)) /* error if count # 0 */
			return(EIO);
		return(0);
	case MTIOCGET:
		mtget = (struct mtget *)addr;
		mtget->mt_erreg = stt->tp_sbytes[0];
		mtget->mt_dsreg1 = stt->tp_sbytes[1];
		mtget->mt_dsreg2 = stt->tp_sbytes[2];
		mtget->mt_gstat = 0;
		mtget->mt_resid = stt->tp_resid;
		mtget->mt_type = MT_IS7970E;
		return(0);
	default:
		return(ENXIO);
	}
}

tp_read(dev, uio)
dev_t dev;
struct uio *uio;
{
 	/* If last operation was a rewind, send 'unit clear' before read */
 	if (tp_iobuf.b_flags & T_REWOUND)	{
  		tape_control(HPIB_clear, (char *)0, 0);
		tape_control(RTtape_status,&tp_status.tp_sbytes[0],STATUS_SIZE);
  		}
	return(physio(tp_strategy, &tp_buf, dev, B_READ, minphys, uio));
}

tp_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	return(physio(tp_strategy, &tp_buf, dev, B_WRITE, minphys, uio));
}

get_status(bp, status)
struct buf *bp;
register char *status;
{
	register int i;
	register char *p1;
	register char *p2;

	(*bp->b_sc->iosw->iod_mesg)(bp, 0, A_TSTAT, status, STATUS_SIZE);

	p2 = tp_status.tp_sbytes;
	for (i=0;i++ < STATUS_SIZE;) *p2++ = *status++;
}

/* Select unit 0, and get status */
static do_status(bp)
register struct buf *bp;
{
	send_cmd((bp), (m_unit(((bp)->b_dev))&3)+1);
	get_status(bp, bp->b_un.b_addr);
	return(1);
}

RTtape_status(bp)
register struct buf *bp;
{
	bp->b_action2 = do_status;
	bp->b_clock_ticks = 5 * HZ;
	HPIB_utility(bp);
}

enum tp_errs
tp_gstat(bp,opcode)
struct buf *bp;
int opcode;
{
	char status[STATUS_SIZE];

 	get_status(bp, status);

	switch (opcode) {
	case MT_READ:	status[0] &= ~0124;/* ignore ste,protect,BOT */
			break;
	case MT_WRITE:	
	case MTWEOF:	status[0] &= ~0100;  /* ignore BOT */
			break;
	case MTFSR:	/* ignore BOT, protect,errors */
	case MTFSF:	status[0] &= ~0126;
			break;
	case MTREW:
	case MTOFFL:
	case MTBSR:	/* ignore EOT, protect,errors */
	case MTBSF:	status[0] &= ~0066;
			break;
	default:	break;	/* pass everything else */
	}

	/* Ignore EOT except if writing, or in "Compatibility Mode" */
	if (!(bp->b_dev&MT_COMPAT_MODE) && (opcode!=MT_WRITE))
		status[0] &= ~040;

	/* order dependent */
	if (status[0]&0010)    /* bad opcode */      return(TE_FATAL);
	if ((status[0]&0001)==0) /* off line */      return(TE_OFFL);
	if (status[1]&0004)    /* rewinding */       return(TE_OFFL);
	if (status[0]&0004)    /* protect */	     return(TE_OFFL);
	if (status[1]&0020)    /* timing data */     return(TE_IOE);
	if (status[1]&0010)    /* runaway */         return(TE_RUN);
	if (status[2]&0020)    /* command pe */      return(TE_IOE);
	if (status[0]&0002)    /* multi track err */ return(TE_IOE);
	if (status[0]&0020)    /* single err */      return(TE_IOE);
	if (status[0]&0200)    /* EOF */ 	     return(TE_EOF);
	if (status[0]&0040)    /* EOT */ 	     return(TE_EOT);
	if (status[0]&0100)    /* LP */		     return(TE_BOT);
	if (status[1]&IFC_BUSY) /* intfc busy */     return(TE_BUSY);
	if (status[1]&UNT_BUSY) /* unit busy */      return(TE_BUSY);

	return (TE_NONE);
}

send_cmd(bp, cmd)
register struct buf *bp;
unsigned char cmd;
{
	char bfr[1];

	bfr[0] = cmd;
	bp->b_sc->iosw->iod_mesg(bp, T_WRITE + T_EOI, A_TSAD,
		bfr, 1);
}

enum m_states {m_start=0, m_setup, m_io_op1, m_io_op2, m_timedout, m_defaul};

tape_x_motion(bp)
register struct buf *bp;
{
	/* if timeout is called, you still need to dequeue after proceesing
	   the timeout */
	register int i;

	tape_motion(bp);
	do {
		i = selcode_dequeue(bp->b_sc);
		i += dma_dequeue();
	}
	while (i);
}

motion_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,tape_x_motion,bp->b_sc->int_lvl,0,m_timedout);
#ifdef TPDEBUG
	printf("tape motion timed out\n");
#endif
}

tape_motion(bp)
register struct buf *bp;
{
	enum tp_errs stat;
	register struct iobuf *iob = bp->b_queue;

retry:
	try 
		START_FSM;
		switch ((enum m_states)iob->b_state) {

		case m_start:
			iob->b_state = (int)m_setup;
			get_selcode(bp, tape_motion);
			break;

		case m_setup: {
			/* do unit select */ 

			START_TIME(motion_timeout, 1 * HZ);
			/* need to get dsj unconditionally */
			GET_DSJ;
			stat = tp_gstat(bp,bp->b_opcode);
			if (bp->b_opcode==MTNOP)	{
				bp->b_resid = 0;
				goto bailout;
				}

			if (dsj) {
				if ((stat != TE_NONE) &&
				    (stat != TE_EOF )) {
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
#ifdef TPDEBUG
				        if ((stat != TE_BOT ) &&
				            (stat != TE_EOT )) {
						printf("Tape setup error\n");
						tpprintf(bp);
					}
#endif
					goto bailout;
				}
			}
			}
			/* motions can take minutes, but each element is
			   short, thus time each separately */
			END_TIME;


		tp_reentry: {
			int x;

			if (bp->b_opcode==MTFSF || bp->b_opcode==MTBSF)
				x = 640*HZ;	/* 2400 ft @ 45ips */
			else	{
				if (bp->b_opcode==MTREW)
					x = 200*HZ;
				else
					x = 2*HZ;
			}
			START_TIME(motion_timeout, x);

			/* this is the re-entry point for the next item
			in a counted loop.  The use of a goto loop is 
			not desireable, but the alternatives are worse because
			of the break/re-enter on interrupt below.  */

			if ((stat==TE_BOT)&&(bp->b_opcode==MTREW))
					{ /* already at BOT */
					iob->b_flags &= ~(T_WRITTEN | T_EOF);
					iob->b_flags |= T_REWOUND;
					bp->b_resid = 0;
					goto bailout;
					}
			send_cmd(bp,cmdtab[bp->b_opcode]);
			iob->b_flags &= ~(T_REWOUND | T_WRITTEN | T_EOF);
			if (bp->b_opcode == MTREW) {
				iob->b_flags |= T_REWOUND;
				send_end_msg(bp,ENAB_POLL|CLEAR_PP_RESP|CLEAR_DSJ);
			}
			if (bp->b_opcode == MTOFFL) {
				iob->b_flags |= T_REWOUND;
				bp->b_resid = 0;
				goto bailout;
			}
			iob->b_state = (int)m_io_op1;
			HPIB_ppoll_drop_sc(bp, tape_motion, 1);
			break;
			}


		case m_io_op1:
			END_TIME;
			iob->b_state = (int)m_io_op2;
			get_selcode(bp,tape_motion);
			break;

		case m_io_op2:
			START_TIME(motion_timeout, 1*HZ);
			GET_DSJ;
			/* Always get status */
			stat = tp_gstat(bp,bp->b_opcode);
			if (dsj) {
				if (stat != TE_NONE) {
					if (stat == TE_EOF) {
						bp->b_flags |= B_ERROR;
						bp->b_error = EIO;
						goto bailout;
					}
					if (stat == TE_BOT &&
						(bp->b_opcode == MTBSR ||
						bp->b_opcode == MTBSF)) {
						--bp->b_resid;
						goto bailout;
					}
#ifdef TPDEBUG
					printf("Tape operation error\n");
					tpprintf(bp);
#endif
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
					goto bailout;
				}
			}
			END_TIME;
			if (--bp->b_resid != 0) goto tp_reentry;
		bailout:
			END_TIME;
			drop_selcode(bp);
			iob->b_state = (int)m_defaul;
			queuedone(bp);
			break;
		
		case m_timedout:
#ifdef TPDEBUG
			printf("tape motion timed out (from fsm).\n");
#endif
			escape(TIMED_OUT);

		default:
			panic("bad tape state");
		}
		END_FSM;
	recover {
		iob->b_state = (int)m_defaul;
		(*bp->b_sc->iosw->iod_abort)(bp);
		if (escapecode == TIMED_OUT) {
			HPIB_ppoll_clear(bp);
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
		ABORT_TIME;
		drop_selcode(bp);
		if (bp->b_error == 0) {
			iob->b_state = (int)m_start;
			goto retry;
		}
		else {
			iob->b_state = (int)m_defaul;
			queuedone(bp);
		}
	}
}
	
enum states {start=0, get_1, io_op1, io_op2, dma_check, io_done,
io_timedout, io_cleanup1, io_cleanup2, bksp_TO, bksp1, bksp2, timedout, defaul};

tp_x_timeout(bp)
register struct buf *bp;
{
	/* if timeout is called, you still need to dequeue after proceesing
	   the timeout */
	register int i;

	tape_transfer(bp);
	do {
		i = selcode_dequeue(bp->b_sc);
		i += dma_dequeue();
	}
	while (i);
}

tp_timeout(bp)
register struct buf *bp;
{
      TIMEOUT_BODY(iob->intloc,tp_x_timeout,bp->b_sc->int_lvl,0,bp->b_TO_state);
#ifdef TPDEBUG
      printf("tape timed out\n");
#endif
}

tape_transfer(bp)
register struct buf *bp;
{
	enum tp_errs stat;
	register struct iobuf *iob = bp->b_queue;
	static char errflag;
	static short	count;

retry:
	try 
		START_FSM;
		switch ((enum states)iob->b_state) {

		case start:
			iob->b_state = (int)get_1;
			get_selcode(bp,tape_transfer);
			break;
		
		case get_1:
		tp_restart:	/* entry from retry logic */
			{
			/* do unit select */ 
			int x;

			bp->b_TO_state = (int)timedout;
			START_TIME(tp_timeout, 1*HZ);
			GET_DSJ;
			/* need to dsj unconditionally */
			stat = tp_gstat(bp,bp->b_opcode);
			if (dsj != 0) {
				if (stat != TE_NONE &&
				    stat != TE_EOF) {
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
#ifdef TPDEBUG
				        if ((stat != TE_BOT ) &&
				            (stat != TE_EOT )) {
						printf("Tape setup error\n");
						tpprintf(bp);
					}
#endif
					goto nextbuf;
				}
			}
			END_TIME;
			}
			/* fall thru */

		case io_op1:
			/* precalculate as much as possible before sending the
			   command.  Needed to make timing.  Although
			   its not nice, we'll have to be piggish about the
			   resources to boot. */
			iob->b_state = (int)io_op2;
			get_dma(bp, tape_transfer);
			break;

		case io_op2:
			iob->b_xaddr = bp->b_un.b_addr;
			iob->b_xcount = bp->b_bcount;
			iob->b_flags &= ~(T_REWOUND | T_WRITTEN | T_EOF |
				B_PPOLL_TIMEDOUT);
			if ((bp->b_flags & B_READ) == 0)
				iob->b_flags |= T_WRITTEN;
/*******
 *   -> Old Comment <-
 *			send_cmd(bp,cmdtab[bp->b_opcode]);
 *   The book (such as it is to be trusted) says that one should wait
 *   for parallel poll here.  Doing this causes problems: can't make the
 *   requisite timing.  On the other hand, not doing it causes us to miss
 *   tape marks!
 *
 *   Solution: don't check the poll, just go on.  Timeout with a short
 *   timeout to check the activity.  If dma has started, just let it go.
 *   If it has not started, assume that the condition was a tape mark and
 *   wait for poll.  If the condition was a long gap, backspacing and retrying
 *   will make sure timing is made on the first retry.
 *
 *   This also permits the use of the highspeed transfer entry point, which
 *   keeps us from hanging up waiting for a DMA channel after the motion
 *   has started!
 *******/
			{
			char bfr[1];
			int x;

			/* make sure the short timeout can't get into
			 * the act until the transfer is actually off
			 * the ground; if it gets in too soon, the 
			 * io card will get goofed up.
			 */
			x = splx(bp->b_sc->int_lvl);
			bfr[0] = cmdtab[bp->b_opcode];
			bp->b_TO_state = (int)dma_check;
			START_TIME(tp_timeout, 1 * HZ);
			iob->b_state = (int)io_done;
			(*bp->b_sc->iosw->iod_hst)(bp, A_TSAD, SEC_xDAT,
				bfr, 1, tape_transfer);
			splx(x);
			}
			break;

		case dma_check:
			{
			struct dma *chan;
			unsigned short count;
		
			/* Do we own DMA? */
			if (bp->b_sc->dma_chan == NULL)	/* dma not owned */
				goto io_okay;
			else	
				chan = bp->b_sc->dma_chan->card;

			count = chan->dma_count;
			/* If DMA not armed check to see if interrupting */
			if ((chan->dma_status & DMA_ARM) == 0) 
				/* If we are getting a dma_isr just return
				 * from timeout and let dma_isr enter fsm
				 */
				if (chan->dma_status & DMA_INT) break;
				else goto io_okay;
		
			/* is dma moving yet? */
			snooze(20); /* assume 14 us/byte */
			if (count != chan->dma_count) { 
			/* DMA still active - allow one more timeout */
				bp->b_TO_state = (int)timedout; 
				START_TIME(tp_timeout, 2 * HZ);
				break;
				}
			}
			/* Apparantly nothing is going on - fall through.
			 * (It's normal to execute this code when reading EOF's
			 *  since no data is xfr'd - only a bit is set in CSR)
			 */

		dma_timedout:
			(*bp->b_sc->iosw->iod_abort)(bp);
			bp->b_TO_state = (int)timedout;
			START_TIME(tp_timeout, 5 * HZ);
			iob->b_state = (int)io_cleanup1;
			HPIB_ppoll_drop_sc(bp, tape_transfer, 1);
			break;

		case io_done:
		io_okay:
			END_TIME;
			bp->b_TO_state = (int)io_timedout;
			START_TIME(tp_timeout, 5 * HZ);
			(*bp->b_sc->iosw->iod_postamb)(bp);
			send_end_msg(bp, CLEAR_PP_RESP | STOP_POLL | CLEAR_DSJ);
			iob->b_state = (int)io_cleanup1;
			HPIB_ppoll_drop_sc(bp, tape_transfer, 1);
			/* wait for controller to say ready for DSJ 
			 * takes about 9 ms for read, about 6 ms for write
			 */
			break;

		case io_timedout:
			HPIB_ppoll_clear(bp);
			/* fall through; sc may have been busy; check below */

		case io_cleanup1:
			END_TIME;
			iob->b_state = (int)io_cleanup2;
			get_selcode(bp,tape_transfer);
			break;
		
		case io_cleanup2:
			/* done, but need to housekeep and get status */
			bp->b_TO_state = (int)timedout;
			START_TIME(tp_timeout, 5 * HZ);
			/* Get # bytes transfered */
			(*bp->b_sc->iosw->iod_mesg)(bp,0,A_TBYTCT, &count, 2);
			GET_DSJ;
			bp->b_errcod = (int)tp_gstat(bp,bp->b_opcode);
		decode_stat:
			switch ((enum tp_errs)bp->b_errcod) {
			case TE_NONE:
				goto completed;
			case TE_FATAL:  /* bad opcode, etc */
			case TE_BUSY:
#ifdef TPDEBUG
				printf("tp: Fatal error, no retry\n");
				tpprintf(bp);
#endif
				bp->b_flags |= B_ERROR;
				bp->b_error = EIO;
				goto nextbuf;
			case TE_RUN:
			case TE_OFFL:
#ifdef TPDEBUG
				printf(
				"tp: Tape dev = 0x%0x off-line/rewind/protect/runaway\n", bp->b_dev);
#endif
				bp->b_flags |= B_ERROR;
				bp->b_error = EIO;
				goto nextbuf;

			case TE_IOE:	/* simple error, retry */
#ifdef TPDEBUG
					printf("Tape Timing Error\n");
					tpprintf(bp);
#endif
				errflag=0;
				if (iob->b_errcnt-- <= 0) {
					/* Give up. Leave head positioned after 
					 * bad block, report error, and exit.
					 */
#ifdef TPDEBUG
					printf("Tape Retrys exceeded\n");
					tpprintf(bp);
#endif
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
					escape(1);
				}
				/* clear zapped unit select; 
				 * do it again; clear ppoll response 
				 */
				GET_DSJ;
				if (dsj) {
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
#ifdef TPDEBUG
					printf("Tape unit reset failed\n");
					tp_gstat(bp,bp->b_opcode);
					tpprintf(bp);
#endif
					escape(1);
				}
				break;

			case TE_EOT:
				/* Crossed EOT, backspace one 
				 * record and return error
				 */
				bp->b_flags |= B_ERROR;
				bp->b_error = EIO;
				if (bp->b_flags & B_READ)
					/* Do not reposition on reads */
					goto nextbuf;
				tp_status.tp_flag |= TP_EOT_FOUND;
				break;

			case TE_EOF:
				/* remember EOF was read for close */
				iob->b_flags |= T_EOF;
				goto nextbuf;

			case TE_BOT: 
				/* count as successful, catch next time */
				goto completed;

			default: panic("unrecognized tape error code");
			}
			bp->b_TO_state = (int)bksp_TO;
			send_cmd(bp,cmdtab[MTBSR]);
			iob->b_state = (int)bksp1;
			HPIB_ppoll_drop_sc(bp,tape_transfer,1);
			break;

		case bksp_TO:
			HPIB_ppoll_clear(bp);
			iob->b_flags |= B_PPOLL_TIMEDOUT;
		/* Backspace routine for retrying an unsuccessful write,
		 * or to backspace a record after passing the EOT.
		 */
		case bksp1:
			if (iob->b_flags&B_PPOLL_TIMEDOUT)
				if (HPIB_test_ppoll(bp, 1))
					iob->b_flags &= ~B_PPOLL_TIMEDOUT;
				else
					escape(TIMED_OUT);
			END_TIME;
			iob->b_state = (int)bksp2;
			get_selcode(bp,tape_transfer);
			break;
		
		case bksp2:
			bp->b_TO_state = (int)timedout;
			START_TIME(tp_timeout, 5 * HZ);
			/* backspace done , was it OK?
			   (also used for erase gap) */
			GET_DSJ;
			if (dsj) {
				if ((int)tp_gstat(bp,MTBSR) <= (int)TE_EOF) {
#ifdef TPDEBUG
					printf("Error during retry");
					tpprintf(bp);
#endif
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
					escape(1);
				}
			}
			if(tp_status.tp_flag & TP_EOT_FOUND)	{
				/* Backspaced Record: no longer past EOT */
				tp_status.tp_flag &= ~TP_EOT_FOUND;
				goto nextbuf;
				}
			if (((bp->b_flags & B_READ) == 0) && 
				(iob->b_errcnt<MAX_WRITE_RETRYS-4)
				    && (errflag==0)) {
				/* We are writing, backspace, write gap, retry
				 * write - for the first write, we make 4
				 * attempts to write record, after that we
				 * attempt write, backspace, write gap, then
				 * retry write for each failure.
				 */
				
				/* if erase gap should die on EOT, will it
				 * be possible to read the "last" record?
				 */
				errflag=1;
				send_cmd(bp,0x07 /* write gap */);
				iob->b_state = (int)bksp1;
				HPIB_ppoll_drop_sc(bp, tape_transfer, 1);
				break;
			}
			/* else we are reading, try again */
			END_TIME;
			goto tp_restart;

		case timedout:
			escape(TIMED_OUT);
		}
		goto end_fsm;

	completed:
		/* If read request less than record size set to 0 */
		if (bp->b_resid < count)	{
			tp_status.tp_resid = (long)count - bp->b_resid;
			bp->b_resid = 0;
			}
		else	{
			tp_status.tp_resid = 0;
			bp->b_resid -= count;
			}
	nextbuf:
		END_TIME;
		drop_selcode(bp);
		queuedone(bp);
	end_fsm:
		END_FSM;
	recover {
#ifdef TPDEBUG
		printf("tp: ESCAPED\n");
#endif
		iob->b_state = (int)defaul;
		ABORT_TIME;
		(*bp->b_sc->iosw->iod_abort)(bp);
		if (escapecode == TIMED_OUT) {
			HPIB_ppoll_clear(bp);
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
		drop_selcode(bp);
		if (bp->b_error == 0) {
			iob->b_state = (int)start;
			goto retry;
		}
		else {
			iob->b_state = (int)defaul;
			queuedone(bp);
		}
	}
}

#ifdef TPDEBUG
tpprintf(bp)
register struct buf *bp;
{
	register struct tp_status *stt; 
	
	stt = &tp_status;
	printf("Tape error: operation %d ",bp->b_opcode);
	printf("dev = 0x%x\n", bp->b_dev); 
        printf("three byte status = 0x%x, 0x%x, 0x%x\n",
		stt->tp_sbytes[0]&0xff,
		stt->tp_sbytes[1]&0xff,
		stt->tp_sbytes[2]&0xff);
}
#endif /* TPDEBUG */

send_end_msg(bp, end_cmd)
register struct buf *bp;
unsigned char end_cmd;
{
	/* Usually to clear ppoll */
	(*bp->b_sc->iosw->iod_mesg)(bp, 1, A_TEND, &end_cmd, 1);
}


#ifdef IOSCAN
tp_link()
{
   hpib_config_ident = tp_config_identify;
}

tp_config_identify(sinfo)
register struct ioctl_ident_hpib *sinfo;
{
    register struct isc_table_type *isc;
    short ident;
    dev_t sdev, bdev;
    int busaddr, err;
    register struct buf *bp = &tp_buf;

    if ((isc = isc_table[sinfo->sc]) == NULL) /* error out if no card*/
                return(ENODEV);

    if (isc->card_type != HP98624 && 
        isc->card_type != INTERNAL_HPIB &&
        isc->card_type != HP98625) 
                return ENODEV;

    bdev = 0x04000000 | (sinfo->sc << 16);

    for (busaddr = 0; busaddr < 8; busaddr++) {

        sdev = bdev | (busaddr << 8);

        bp->b_dev = sdev;    
        bp->b_sc = isc;      
        bp->b_ba = m_busaddr(sdev);

        err = tape_control(HPIB_identify, (char *)&ident, sizeof ident); 

        if (err) {
            sinfo->dev_info[busaddr].ident = -1;
            continue;
        }

        sinfo->dev_info[busaddr].ident = ident;

    }
    return 0;
}
#endif /* IOSCAN */
