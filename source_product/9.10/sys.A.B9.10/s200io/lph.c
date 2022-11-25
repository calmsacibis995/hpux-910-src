/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/lph.c,v $
 * $Revision: 1.7.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:09:38 $
 */
/* HPUX_ID: @(#)lph.c	52.1		88/04/19 */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
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

/*
	    	    HPIB line printer
    asynchronous driver for HP amigo and non amigo printers.
		  2602 is not supported

		   Hewlett-Packard Company
		      May. 25, 1983
		    (mod by DLB 8-84)


*/
/*
   _____________________________________________________________
  |                                                             |
  |Product       Name or                          Interface/    |
  |Number        Description                      Switches      |
  |_____________________________________________________________|
  |                                                             |
  |2225A         ThinkJet                         HP-IB/DUMB    |
  |                                                             |
  |2227B         QuietJet Plus Workstation        HP-IB/DUMB    |
  |                                                A 00000000   |
  |                                                B 00000100   |
  |                                                             |
  |2227B         QuietJet Plus Workstation        HP-IB/AMIGO   |
  |                                                A 10000000   |
  |                                                B 00000100   |
  |                                                             |
  |2235D         RuggedWriter                     HP-IB/DUMB    |
  |                                                A 00000000   |
  |                                                B 0000010000 |
  |                                                             |
  |2235D         RuggedWriter                     HP-IB/AMIGO   |
  |                                                A 00000000   |
  |                                                B 0000010010 |
  |                                                             |
  |2932A         Dot matrix, 200 cps              HP-IB/AMIGO   |
  |   Opt. 046                                                  |
  |                                                SECONDARY    |
  |                                                COMMANDS on  |
  |                                                SUPPORT MODE |
  |                                                2932         |
  |                                                             |
  |2934A         Dot matrix, 200 cps              HP-IB/AMIGO   |
  |   Opt. 046                                                  |
  |                                                SECONDARY    *
  |                                                COMMANDS on  *
  |                                                SUPPORT MODE *
  |                                                2934         *
  |                                                             |
  |3630A         PaintJet: 180 DPI Color InkJet   HP-IB/DUMB    |
  |   Opt. 002                                                  |
  |                                                A 0000001    *
  |                                                             |
  |3630A         PaintJet: 180 DPI Color InkJet   HP-IB/AMIGO   |
  |   Opt. 002                                                  |
  |                                                A 0001001    *
  |                                                             |
  |C1202A                                         HP-IB/DUMB    |
  |                                                A XXXXXXX    *
  |                                                             |
  |C1602A        B-Size PaintJet                  HP-IB/AMIGO   |
  |                                                A XXXXXXX    *
  |_____________________________________________________________|
*/

#include "../h/param.h"
#include "../h/file.h" /* For FWRITE */
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../h/proc.h"
#include "../wsio/tryrec.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../wsio/iobuf.h"
#include "../s200io/lp.h"
#include "../h/uio.h"
#include "../h/lprio.h"

/* #define LP_DEBUG 1      Added flag for turning on debugging info */
#ifdef  LP_DEBUG

#define LP_DEBUG1(a)		printf(a)
#define LP_DEBUG2(a,b)		printf(a,b)
#define LP_DEBUG3(a,b,c)	printf(a,b,c)
#define LP_DEBUG4(a,b,c,d)	printf(a,b,c,d)
#define LP_DEBUG5(a,b,c,d,e)	printf(a,b,c,d,e)
#define LP_DEBUG6(a,b,c,d,e,f)	printf(a,b,c,d,e,f)

#define	aIO	31
#define	bIO	32
#define	cIO	33
#define	dIO	34
#define	eIO	35
#define	fIO	36
#define	gIO	37
#define	hIO	38
#define	iIO	39
#define	jIO	13
#define	kIO	41
#define	lIO	42
#define	mIO	43
#define	nIO	44
#define	oIO	45

#else	/* LP_DEBUG */

#define LP_DEBUG1(a)
#define LP_DEBUG2(a,b)
#define LP_DEBUG3(a,b,c)
#define LP_DEBUG4(a,b,c,d)
#define LP_DEBUG5(a,b,c,d,e)
#define LP_DEBUG6(a,b,c,d,e,f)

#define	aIO	EIO
#define	bIO	EIO
#define	cIO	EIO
#define	dIO	EIO
#define	eIO	EIO
#define	fIO	EIO
#define	gIO	EIO
#define	hIO	EIO
#define	iIO	EIO
#define	jIO	EIO
#define	kIO	EIO
#define	lIO	EIO
#define	mIO	EIO
#define	nIO	EIO
#define	oIO	EIO

#endif	/* LP_DEBUG */

#define	TWOSECONDS	HZ*2	/* two seconds for wait routine */
#define	TENSECONDS	HZ*10	/* ten seconds for wait routine */
/*	................................................ */

#define RAW 	0x1	/* raw access only -- no line buffering */
#define NOCR	0x2	/* for printers that do not have '\r' only */
			/*  capability */
#define CAP	0x4	/* for upper case only printers */
#define NO_EJT	0x8	/* No page eject on open and close */
#define NO_TO	0x10	/* No timeouts for this device */

#define LP_OPEN  0x1	/* bit position in t_flag */
#define LP_LOCK  0x2	/* bit position in t_flag */
#define LP_ERROR 0x4	/* bit position in t_flag */
#define LP_RESET 0x8	/* bit position in t_flag */
#define LP_ROPEN 0x10	/* bit position in t_flag */

struct PR_stat {
	dev_t		t_dev;
	struct PR_bufs *t_bufs;		/* buf, iobuf, input buf and */
					/* output buf */
	int	t_ccc;			/* current input position in */
					/* the line */
	int	t_mcc;			/* current output position in */
					/* the line */
	int	t_mlc;			/* current line count on page */
	char	t_flag, t_kind;		/* open etc. flag, type of */
					/* printer */
	struct lprio 	t_lprio;	/* printer defaults */
	int	t_rec_size;		/* holds the recode size to */
					/* transfer to a printer */
} lp_st[LPMAX];

#define		COUNT	io_s2

static struct lprio lpr_defaults = {
	4,		/* indentation		*/
	132,		/* columns per line	*/
	66,		/* lines per page	*/
	PASSTHRU,	/* backspace handling	*/
	0,		/* ejects on open 	*/
	1,		/* ejects on close 	*/
	COOKED_MODE	/* raw mode is cleared = cooked mode */
};

static struct lprio lpr_defaults_raw  = {
	4,		/* indentation		*/
	132,		/* columns per line	*/
	66,		/* lines per page	*/
	PASSTHRU,	/* backspace handling	*/
	0,		/* ejects on open 	*/
	1,		/* ejects on close 	*/
	RAW_MODE        /* raw mode is set 	*/
};

/* IDENTIFY BYTES for amigo printers (byte1 << 8 + byte2) */
char 		PR_ID2225A[3] = {32,18,0};
char            PR_ID2227B[3] = {32,13,0}; /* 2235 also returns this */
char		PR_ID2608[3]  = {32,1 ,0};
char		PR_ID2631A[3] = {32,2,0};  /* byte1 = 32, byte2 = 2 */
char		PR_ID2631B[3] = {32,9 ,0};
char		PR_ID2673[3]  = {32,11,0};
char 		PR_ID2932[3]  = {32,16,0};
char 		PR_ID2934[3]  = {32,17,0};
char		PR_ID3630A[3] = {32,14,0};
char		PR_ID1602A[3] = {32,32,0};


/* internal printer buffers */
int		PR_DUMBSIZE	= 132;
int		PR_2225SIZE	= 132;
int             PR_2227SIZE	= 132;
int		PR_2608SIZE	= 132;
int		PR_2631SIZE	= 132;
int		PR_2673SIZE	= 80;
int		PR_2932SIZE	= 132;
int		PR_3630SIZE	= 80;
int		PR_1602SIZE	= 105;

/* maximum record size to transfer in one write to the interface driver */
int		REC_DUMBSIZE	= 132;
int		REC_2225SIZE	= 32;
int             REC_2227SIZE	= 32;
int		REC_2608SIZE	= 32;
int		REC_2631SIZE	= 7;
int		REC_2673SIZE	= 32;
int		REC_2932SIZE	= 32;
int		REC_3630SIZE	= 32;
int		REC_1602SIZE	= 32;

int	SHORTTIME_1 = SHORTTIME;
int	LONGTIME_1  = LONGTIME;
int	XFRTIME_1   = XFRTIME;

#define	START_TIME_IF(proc, ticks, minor_num)		\
{							\
	if ((minor(minor_num) & NO_TO) == 0){		\
		START_TIME(proc, ticks);		\
	}						\
}
#define	END_TIME_IF(ticks, minor_num)			\
{							\
	if ((minor(minor_num) & NO_TO) == 0){		\
		END_TIME;				\
	}						\
}	

extern HPIB_identify();
extern HPIB_clear_wopp();	/* HPIB clear without ppoll */
int PR_timeout();
int PR_timeout2();
int PR_transfer();

/*
 * The two routines amg_wait and amg_sleep are used to provide
 * a wait/sleep.  This is used to delay processing so a printer
 * with problem (off line or out of paper) does not use up a
 * lot of the cpu.
 */

amg_sleep(bp)
struct buf *bp;
{
	wakeup((caddr_t)&bp->b_sc);
}

amg_wait(bp,timeout_count)
struct buf *bp;
int    timeout_count;
{
	int x;

	x = spl6();
	timeout(amg_sleep,bp,timeout_count,0);
	sleep(&bp->b_sc,PRIBIO+1);
	splx(x);
}

/*
 * This routine get the status from the printer.
 *
 * The features that this routine provides are:
 *
 * 1)	For printer of the 2673, 2631 and 2225 catagory:
 *
 *	a)	Primary status is obtained.
 *
 *	b)	If primary status is "Ready to Send Data",
 *		return this as the status.
 *
 *	c)	If primary status is "Ready to Receive Data" or
 *		"Read Status", then get the secondary status.
 *
 * x)	For the 2608 NEEDS TO BE FILLED IN BY THE PERSON THAT
 *	MAKES THE EVALUATION/CHANGES TO THIS SECTION.
 *
 *
 *	Status is collected as bytes.  The status is placed in the array
 *	"stat".  The status is returned as a int with the statement
 *	"return(*(int *)stat);".
 */


/*  I own the select code at this point
 *  I cannot return stat as a pointer since
 *  an isr can occur and ruin the stack info
 */
lpget_stat(bp)
register struct buf *bp;
{
	char stat[4];
	char data;

	*(int *)stat = 0;	
	switch (bp->prt_kind) {

	case P_2225:
	case P_2227:
	case P_2631:
	case P_2673:
	case P_2932:
	case P_3630:
	case P_1602:
		(*bp->b_sc->iosw->iod_mesg)(bp, 0, PR_SEC_DSJ, &stat[0],1);
		if (stat[0] != PR_SDS)
			(*bp->b_sc->iosw->iod_mesg)(bp, 0, PR_SEC_RSTA, &stat[1],1);
		return(*(int *)stat);

	case P_2608:
		(*bp->b_sc->iosw->iod_mesg)(bp, 0, PR_SEC_DSJ, &stat[0],1);
		if ((stat[0] == PR_ATTEN) || (stat[0] == PR_ATT_PAR)){
			data = 1;
			(*bp->b_sc->iosw->iod_mesg)(bp,T_WRITE,PR_SEC_STRD,
			&data,1);
			(*bp->b_sc->iosw->iod_mesg)(bp,0,0, &stat[1],1);
			if (stat[1] & PR_I_OPSTAT) {
				/* read the operator status */
				data = 2;
				(*bp->b_sc->iosw->iod_mesg)(bp,T_WRITE,
				PR_SEC_STRD, &data,1);
				(*bp->b_sc->iosw->iod_mesg)(bp,0,0, &stat[2],1);
			}
				
		}
		return(*(int *)stat);
	}
	return(*(int *)stat);
}


lpdo_stat(bp)
struct buf *bp;
{

	*(int *)bp->b_un.b_addr = lpget_stat(bp);
	return 0;  /* no parrallel poll */
}

lpRT_stat(bp)
struct buf *bp;
{
	bp->b_action2 = lpdo_stat;
	bp->b_clock_ticks = 5 * HZ;
	HPIB_utility(bp);
}


do_mask_2631(bp)
struct buf *bp;
{
	char mask;

	mask = PR_M_PAPER + PR_M_POWER + PR_M_STATUS + PR_M_RFD;
	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE, PR_SEC_MASK, (char *)&mask, 1);
	return 0;
}

set_mask_2631(bp)
struct buf *bp;
{
	bp->b_action2 = do_mask_2631;
	bp->b_clock_ticks = 5 * HZ;
	HPIB_utility(bp);
}


/*
 *	This routine will perform a clear operation.
 *
 *	Note:	For the 2227B the clear operation is performed by
 *		writing the clear sequence.  This is done because
 *		The selected device clear command (in the clear
 *		routine in ti9914.c) sometimes causes the 2227B
 *		to lock up.  This usually occurs when a lot of output
 *		is still in the internal buffer of the 2227B and a
 *		clear is sent to it.
 *
 *		The define "PR_SEC_DSJ" (a listen operation) has the
 *		same bit pattern as the clear command pattern, so
 *		it is used as the clear command.
 */

lpdo_clear(bp)
struct buf *bp;
{
	char	data;
	/* Clear the device, set the printer to a defined state. */
	switch (bp->prt_kind) {

	case P_2225:
	case P_2227:
	case P_3630:
	case P_1602:
		data = 0;
                (*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE, PR_SEC_DSJ, &data, 1);
		break;
	case P_2608:
	case P_2631:
	case P_2673:
	case P_2932:
		(*bp->b_sc->iosw->iod_clear_wopp)(bp);
		break;
	default:
		bp->b_error = aIO;
		break;
	}
}

lpRT_clear(bp)
struct buf *bp;
{
	bp->b_action2 = lpdo_clear;
	bp->b_clock_ticks = 5 * HZ;
	HPIB_utility(bp);
}

/*
 *  The 2608, 2673 and 2631 are amigo printers and will response to
 *  a universal identify command.
 *  The 9876, 2671, however, do not and there is a time
 *  out mechanizim in the low level drivers that takes care of the
 *  problem without hanging the bus and reports an error.
 */
PR_identify(tp)
register struct PR_stat *tp;
{
	char *identify = &tp->t_bufs->r_inbuf[0];
	register struct buf *bp = &tp->t_bufs->r_bpbuf;

	identify[0] = -1; /* force illegal -- just in case */
	if (PR_control(HPIB_identify, bp, identify, 2)) {
		tp->t_rec_size = REC_DUMBSIZE;
		bp->prt_kind   = P_DUMB;	/* error return. */
		bp->b_resid    = PR_DUMBSIZE;	/* Assume a dumb */
						/* printer */
	} else {	
		identify[2] = 0;
		if ((strcmp(identify, PR_ID2631A) == 0) ||
		    (strcmp(identify, PR_ID2631B) == 0)) {
			tp->t_rec_size = REC_2631SIZE;
			bp->prt_kind   = P_2631;
			bp->b_resid    = PR_2631SIZE;
		}
		else if ((strcmp(identify, PR_ID2932) == 0) ||
		         (strcmp(identify, PR_ID2934) == 0)) {
			tp->t_rec_size = REC_2932SIZE;
			bp->prt_kind   = P_2932;
			bp->b_resid    = PR_2932SIZE;
		}
		else if (strcmp(identify, PR_ID2608) == 0) {
			tp->t_rec_size = REC_2608SIZE;
			bp->prt_kind   = P_2608;
			bp->b_resid    = PR_2608SIZE;
		}
		else if (strcmp(identify, PR_ID2225A) == 0) {
			tp->t_rec_size = REC_2225SIZE;
			bp->prt_kind   = P_2225;
			bp->b_resid    = PR_2225SIZE;
		}
		else if (strcmp(identify, PR_ID2227B) == 0) {
			tp->t_rec_size = REC_2227SIZE;
			bp->prt_kind   = P_2227;
			bp->b_resid    = PR_2227SIZE;
		}
		else if (strcmp(identify, PR_ID2673) == 0) {
			tp->t_rec_size = REC_2673SIZE;
			bp->prt_kind   = P_2673;
			bp->b_resid    = PR_2673SIZE;
		}
		else if (strcmp(identify, PR_ID3630A) == 0) {
			tp->t_rec_size = REC_3630SIZE;
			bp->prt_kind   = P_3630;
			bp->b_resid    = PR_3630SIZE;
		}
		else if (strcmp(identify, PR_ID1602A) == 0) {
			tp->t_rec_size = REC_1602SIZE;
			bp->prt_kind   = P_1602;
			bp->b_resid    = PR_1602SIZE;
		}
		else {
			switch (identify[0]) {
			case 32:
				printf("lph: dev = 0x%x is unknown AMIGO printer!\n",
					bp->b_dev);
				break;
			case 33:
				printf("lph: dev = 0x%x is a CIPER printer--wrong driver!\n",
					bp->b_dev);
				break;

			default:
	    			printf("lph: dev = 0x%x is not a printer!\n",
					bp->b_dev);
				break;
			}
			return(bIO);
		}
	}
	return 0;
}

lp_open(dev, flag)	
register dev_t dev;
register flag;
{
	register struct PR_stat 	*tp = &lp_st[0];
	register struct buf 		*bp;
	register struct isc_table_type	*sc;
	register			error = 0;
	register int 			n;

	int	i;
	int	first_time;
	char	*identify;
        struct proc *p = u.u_procp; /* XXX */

	if (!(flag & FWRITE)) 		/* error out if not write */
		return(EINVAL);
	n = m_selcode(dev);
	if (n > 31)
		return(EINVAL);
	if (dev & RAW) {
		/* must only have RAW minor or the RAW and NO_TO */
		/* minor bits set in least 8 bits, this is for */
		/* protection of only last close will call close */

		if ((m_flags(dev) & (~NO_TO)) != RAW)
			return(ENXIO);
	}
	if ((sc = isc_table[n]) == NULL) /* error out if no card*/
		return(ENXIO);

	if (sc->card_type != HP98625 &&
	    sc->card_type != HP98624 &&
	    sc->card_type != INTERNAL_HPIB)
		return(ENXIO);

	for (n = LPMAX;; tp++) { /* find entry in tp for this printer */
		if ((((tp->t_dev ^ dev) & M_DEVMASK) == 0) || tp->t_dev == NULL)
			break;
		if (--n <= 0)
			return(ENXIO); /* too many printers */
	}
	/* only one active open allowed at a time */
	if (tp->t_flag & LP_LOCK)
		return(EBUSY);

	/* disallow multiple opens */
	if (tp->t_flag & (LP_OPEN|LP_ROPEN))
		return(EBUSY);

	tp->t_flag |= LP_LOCK; /* prevent interference while getting buf */
	tp->t_flag &= ~LP_ERROR; /* clear out any errors */

	bp = geteblk(sizeof(struct PR_bufs)); /* allocate needed buffers */

	tp->t_flag &= ~LP_LOCK; /* and unlock the open */

	if (dev & RAW)  {
		tp->t_flag |= LP_ROPEN; /* mark the printer raw open */
		tp->t_lprio.raw_mode = RAW_MODE; /* Set raw mode flag */
		}
	else
		tp->t_flag |= LP_OPEN; /* mark the printer normal open */

	/* Get the address of the buffers and clear them */

	tp->t_bufs = (struct PR_bufs *)bp->b_un.b_addr;
	bzero((caddr_t)tp->t_bufs, sizeof(struct PR_bufs));

	tp->t_bufs->r_getebuf = bp; 	/* save for brelse() */

	bp = &tp->t_bufs->r_bpbuf;	/* build buf for this driver */
	bp->b_spaddr = tp->t_bufs->r_getebuf->b_spaddr;
	bp->b_sc = sc;
	bp->b_ba = m_busaddr(dev);
	bp->b_queue = &tp->t_bufs->r_iobuf;
	bp->b_dev = tp->t_dev = dev;

	/* if reset, force to unknown printer type */
	if (tp->t_flag & LP_RESET) {
 		tp->t_kind = 0;
		tp->t_flag &= ~LP_RESET;
	}

	bp->prt_kind = tp->t_kind;
	if (error = PR_identify(tp)){ 	/* what kind of printer */
		LP_DEBUG2("identify error is %d\n",error);
		goto errout;
	}
	LP_DEBUG2("identify is %d\n",bp->prt_kind);
	/*
	   2225 printer takes approx. 6 siconds to do device clear.
	   Hence, it is not done by the time I do DSJ and returns
	   buffer full. Thats why I am not doing device clear for
	   2225 printer. (cil)
	*/
	identify = &tp->t_bufs->r_inbuf[0];
	if ((bp->prt_kind != P_DUMB && bp->prt_kind != P_2225)) { /* amigo */
		if (error = PR_control(lpRT_clear, bp, identify, 0)){
			LP_DEBUG2("clear error is %d\n",error);
			goto errout;
		}
		if ((bp->prt_kind == P_2631) || (bp->prt_kind == P_2932)){
			if (error = PR_control(set_mask_2631, bp, identify, 4)){
				LP_DEBUG2("set_mask error is %d\n",error);
				goto errout;
			}
		}
/*
 * This section of the open routine get the status from the printer.
 *
 * The features that this routine provides are:
 *
 * 1)	The driver will wait for a off-line, paper out
 *	or carriage stall condition to be corrected.
 *
 * 2)	The driver will wait ten seconds before checking to see
 *	if the printer is on line.
 *
 * 3)	The driver will wait ten seconds before checking to see
 *	if the printer has paper loaded.
 *
 * 4)	The driver will wait ten seconds before checking to see
 *	if the printer is ready for data.  This is how off line
 *	is handled for printers that do not return off line status.
 *
 *
 *	NOTE: There is a special case for the first time getting
 *	      status.  If it is the first time, do not wait ten
 *	      seconds.  The reason is because the Rugged Writer
 *	      (2235D) returns not ready for data status the first
 *	      time status is obtained.  This prevents the ten
 *	      second wait in this case.  The concern is that the
 *	      user might think something is wrong if the printer
 *	      waits ten seconds before printing.
 */
		/*
		 * Read the status of the printer. If it is online
		 * and ready for data then open the printer and set
		 * the flag, otherwise try again if it is off line,
		 * out of paper or not ready for data.
		 */
	first_time = 0;
lp_open_wait1:
		if (error = PR_control(lpRT_stat, bp, identify, 4)){
			goto errout;
		}
		if (is_err1(tp->t_bufs->r_inbuf[0], bp->prt_kind)){
			if (error = is_err2(tp->t_bufs->r_inbuf[1], bp->prt_kind)){
				if (((error >= 2) && (error <= 4))){
					if (first_time == 0){
						first_time = 1;
						amg_wait(bp,(int) TWOSECONDS);
						goto lp_open_wait1;
					}
					if (ISSIG(p)){
						error = EINTR;
						LP_DEBUG1("EINTR error\n");
						goto errout;
					}
					amg_wait(bp,(int) TENSECONDS);
					if (ISSIG(p)){
						LP_DEBUG1("EINTR error\n");
						error = EINTR;
						goto errout;
					}
					goto lp_open_wait1;

				}
				else{
					error = cIO;
					LP_DEBUG2("open is_err2 error is %d\n",error);
					goto errout;
				}
				
			}
		}
	}
	if (bp->prt_kind != tp->t_kind) { /* only on 1st open of this printer */
	   	if (!tp->t_lprio.raw_mode)
			tp->t_lprio = lpr_defaults;
           	else
			tp->t_lprio = lpr_defaults_raw;

		tp->t_kind = bp->prt_kind;
		tp->t_lprio.col = bp->b_resid; 	/* default columns !!!!!! */
		tp->t_lprio.ind = tp->t_lprio.col>80?4:0; /* indent columns */
	}
	tp->t_mlc = 0; /* No leading page eject on open */
	tp->t_ccc = tp->t_lprio.ind; /* only necessary if FF is not done */
	tp->t_mcc = 0; /* clean out unposted buf due to error */

	/* Print opening form feeds */
	if (((tp->t_dev & NO_EJT) == 0) && (!tp->t_lprio.raw_mode)) {
		try
			for (i=0; i < tp->t_lprio.open_ej; i++)
                                lp_tof(tp,'\f'); /* do conditional FF */
				
		recover {
			error = escapecode;
			LP_DEBUG2("lp_open ff error is %d\n",error);
			goto errout;
		}
	}
	return(0);
errout:
	brelse(tp->t_bufs->r_getebuf);
	tp->t_flag &= ~(LP_ROPEN|LP_OPEN);
	tp->t_lprio.raw_mode = COOKED_MODE;
	if (error != EINTR)
 		tp->t_flag |= LP_RESET; /* if error, force reset */
	return(error);
}


lp_close (dev, flag)         /* close hpib printer */
register dev_t dev;
register int flag;
{
	struct PR_stat *tp = &lp_st[0];
	register n;
	int i;

	for (n = LPMAX;; tp++) { /* find entry in tp for this printer */
		if (((tp->t_dev ^ dev) & M_DEVMASK) == 0)
			break;
		if (--n <= 0)
			panic("lph: could not find dev_t on close");
	}

	/* Print close page ejects */
	if (((tp->t_dev & NO_EJT) == 0) && (!tp->t_lprio.raw_mode)) {
	   for (i=0; i < tp->t_lprio.close_ej; i++)
                lp_tof(tp,'\f'); /* do conditional FF */

        }

	/* super kludge for raw printer access */
	if (dev & RAW) {
		tp->t_flag &= ~LP_ROPEN;
	} else {
		if (!tp->t_lprio.raw_mode) {
		/* not RAW, check if previous error */
			if ((tp->t_flag & LP_ERROR) == 0) {
				try
					/* flush buffer */
					lpoutput(tp, -1);
				recover {
					/* if error, force reset */
 					tp->t_flag |= LP_RESET;
					}
 			} else {
 				tp->t_flag |= LP_RESET; /*error, force reset */
              		}
		}
		tp->t_flag &= ~LP_OPEN;
	}
	if (tp->t_flag & (LP_OPEN|LP_ROPEN))
		return;
	brelse(tp->t_bufs->r_getebuf);
}

lp_write(dev, uio)
register dev_t dev;
struct uio *uio;
{
	register char *cp;
	struct PR_stat *tp = &lp_st[0];
	struct buf *bp;
	register int n, maxbufsz;
	int error = 0;
	register struct proc *p = u.u_procp; /* XXX */
	
	for (n = LPMAX;; tp++) { /* find entry in tp for this printer */
		if (((tp->t_dev ^ dev) & M_DEVMASK) == 0)
			break;
		if (--n <= 0)
			panic("lph: could not find dev = %x", dev);
	}
	bp = &tp->t_bufs->r_bpbuf;
	maxbufsz = PR_INBUF;
	/* kludge for 2225 & 2634 in dumb mode */
	if (tp->t_lprio.raw_mode) {
		try
			maxbufsz = min(PR_INBUF, 256);
			lpoutput(tp, -1); /* flush buffer */
		recover {
			error = escapecode;
			tp->t_flag |= LP_ERROR;
			return(error);
		}
	}
	try
		while (n = min(maxbufsz, (unsigned)uio->uio_resid)) {
			cp = &tp->t_bufs->r_inbuf[0];
			if (error = uiomove(cp, n, UIO_WRITE, uio))
				break;
	                if (tp->t_lprio.raw_mode) {
				if (ISSIG(p))
					escape(EINTR);
				if (PR_control2(PR_transfer, bp, cp, n, tp))
					escape(dIO);
			} else {
				do
					lpoutput(tp, *cp++);
				while (--n);
			}
		}
	recover {
			error = escapecode;
	}
	if ((error != 0) && (error != EINTR))
		tp->t_flag |= LP_ERROR;
	else
		tp->t_flag &= ~LP_ERROR;

	return(error);
}


lp_ioctl(dev, cmd, data, flag)
	dev_t dev;
	register struct lprio *data;
{
	struct PR_stat *tp = &lp_st[0];
	register n;

	for (n = LPMAX;; tp++) { /* find entry in tp for this printer */
		if (((tp->t_dev ^ dev) & M_DEVMASK) == 0)
			break;
		if (--n <= 0)
			panic("lph: could not find dev");
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
		if (data->col == 0) {
			tp->t_flag |= LP_RESET;
			break;
		}
		tp->t_lprio.col = min((unsigned)data->col, PR_PSIZE);
		n = tp->t_lprio.ind;
		tp->t_lprio.ind = min((unsigned)data->ind, tp->t_lprio.col);
		if (tp->t_ccc == n)
			tp->t_ccc = tp->t_lprio.ind;
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

lpoutput(tp, c)
register struct PR_stat *tp;
register c;
{
	register unsigned char *outbufp = (unsigned char *)&tp->t_bufs->r_outbuf[0];
	struct buf *bp = &tp->t_bufs->r_bpbuf;
	register struct proc *p = u.u_procp; /* XXX */

	if (c == -1) { /* buffer flush request */
#ifdef LP_DEBUG
/*
		if (tp->t_mcc) {*/ /* any chars in output buffer ? */
/*
		printf("Flush case of lpoutput\n");
		printf("Number of chars in output buffer= %d\n",tp->t_mcc);
		printf("Current input position in line= %d\n",tp->t_ccc);
		printf("Current line count = %d\n",tp->t_mlc);
		}
		else {
		printf("Flush case of lpoutput\n");
		printf("No chars in output buffer = %d\n",tp->t_mcc);
		}
*/
#endif
		if (tp->t_mcc) { /* any chars in output buffer ? */
			outbufp[tp->t_mcc++] = '\r'; /* to prevent hang? */
		c = tp->t_mcc;
			tp->t_mcc = 0;
			if (ISSIG(p))
				escape(EINTR);
			if (PR_control2(PR_transfer, bp, outbufp, c, tp))
				escape(eIO);
			return;
		}
		return;
	}

	if(tp->t_dev&CAP) {
		if(c>='a' && c<='z')
			c += 'A'-'a'; else
		switch(c) {
		case '{':
			c = '(';
			goto esc;
		case '}':
			c = ')';
			goto esc;
		case '`':
			c = '\'';
			goto esc;
		case '|':
			c = '!';
			goto esc;
		case '~':
			c = '^';
		esc:
			lpoutput(tp, c);
			tp->t_ccc--;
			c = '-'; /* overwrite char with '-' */
		}
	}
	switch(c) {
	case '\t':
		tp->t_ccc=((tp->t_ccc+8-tp->t_lprio.ind) & ~7)+tp->t_lprio.ind;
		return;
	case '\n':
		tp->t_mlc++;
 		if ((tp->t_lprio.line > 0) && (tp->t_mlc >= tp->t_lprio.line))
			c = '\f'; /* do form feed instead of newline */
	case '\f':
		/* check if any lines or non-white space chars on page */
		if (tp->t_mlc || tp->t_mcc || (tp->t_dev & NO_EJT)) {
			outbufp[tp->t_mcc++] = '\r';
			outbufp[tp->t_mcc++] = c;
			if (c == '\f') {  /* always trail '\f' with '\r' */
				outbufp[tp->t_mcc++] = '\r';
				tp->t_mlc = 0;
			}
			if (ISSIG(p)) {
				tp->t_mcc = 0;
				escape(EINTR);
			}
			if (PR_control2(PR_transfer, bp, outbufp, tp->t_mcc, tp)) {
				tp->t_mcc = 0;
				escape(fIO);
			}
		}
		tp->t_mcc = 0;
	case '\r': /* special case -- treat like a lot of backspaces */
		tp->t_ccc = tp->t_lprio.ind; /* set the indent */
		return;
	case '\b': /* special case because some printers can't handle */
		if (tp->t_ccc > tp->t_lprio.ind) { /* decrement col counter */
			tp->t_ccc--;
			if (tp->t_lprio.bksp == PASSTHRU) {
				if ((tp->t_mcc >= PR_INBUF) &&
				   (tp->t_mcc > 0)) {
				   /* flush buffer */
				   lpoutput(tp, -1);
				   tp->t_ccc = tp->t_lprio.ind;
				   tp->t_mcc = 0;
				   }

				outbufp[tp->t_mcc++] = '\b';
				if (tp->t_ccc <= tp->t_lprio.col)
				   tp->t_mcc = tp->t_ccc;
                        }
		}
		return;
	case ' ':
		tp->t_ccc++;
		return;
	default:
#ifdef LP_DEBUG
/*
		printf("Default case of lpoutput\n");
		printf("Number of chars in output buffer= %d\n",tp->t_mcc);
		printf("Current input position in line= %d\n",tp->t_ccc);
*/
#endif
		if(tp->t_ccc < tp->t_mcc) { /* check if backspaced !! */
			if (tp->t_dev & NOCR) {
				tp->t_ccc++;
				return; /* eat chars after '\b' */
			} /* else output CR & space's up until current col */
			outbufp[tp->t_mcc++] = '\r';
			if (ISSIG(p)) {
				tp->t_mcc = 0;
				escape(EINTR);
			}
			if (PR_control2(PR_transfer, bp, outbufp, tp->t_mcc, tp)) {
				tp->t_mcc = 0;
				escape(gIO);
			}
			tp->t_mcc = 0;
		}
		if(tp->t_ccc < tp->t_lprio.col) { /* eat chars past max col */
			while(tp->t_ccc > tp->t_mcc) { /* pad spaces until */
				outbufp[tp->t_mcc++] = ' ';
			}
			outbufp[tp->t_mcc++] = c;
		}
		tp->t_ccc++;
	}
}

/* Do a top-of-form */
lp_tof(tp,c)
register struct PR_stat *tp;
register c;
{
register unsigned char *outbufp = (unsigned char *)&tp->t_bufs->r_outbuf[0];

outbufp[tp->t_mcc++] = '\r';
outbufp[tp->t_mcc++] = '\f';
outbufp[tp->t_mcc++] = '\r';
}

/* Keep the queue going after a software trigger */
PR_dumb(bp)
register struct buf *bp;
{
	register int i;

	PR_transfer(bp);
	do {
		i = selcode_dequeue(bp->b_sc);
		i += dma_dequeue();
	}
	while (i);
}

enum states {
	start=0,command,stat1,intm1,stat2,get1,tfr,report,get3,
	timedout, timedout2, defaul};


/*
 *  Run this driver at interrupt level 0 so that the serial card won't
 *  drop charcters
 */
PR_intm(bp)
register struct buf *bp;
{
	struct iobuf *iob;
	int x;

	iob = bp->b_queue;

	/* find  out the interrupt level */
	x = spl6();
	splx(x);

	if (x != bp->b_sc->int_lvl) {
		x = splsx (1);
		PR_transfer(bp);
		splsx(x);
	}
	else {
		sw_trigger(&iob->intloc, PR_dumb, bp, 0, 1);
	}
}

/*
 *   The reason behind the huge amount of code in the state machine
 *   is that, I can not use messages to do stat on the printers since
 *   sometimes they refuse the respond in a short perid of time. I need
 *   to keep the timeouts tight arround these opperations to prevent
 *   the system from hanging up when daemon is in use and the printer
 *   has a problem. The solution to the problem as implemented is to
 *   drive the state machine by interrupts completely. That is why
 *   transfers are used instead of messages. (cil)
 */


PR_transfer(bp)
register struct buf *bp;
{
	register struct iobuf *iob;
	register int x,i;
	register int dsjbyte;
	struct drv_table_type *isw;


	iob = bp->b_queue;

	try
	START_FSM;
	switch((enum states) iob->b_state) {

	case start:
		iob->b_state = (int)command;
		/*
		 *  store the number of data bytes to be transfered
		 *  until the actual data transfer
		 */
		iob->COUNT = bp->b_bcount;
		get_selcode(bp,PR_intm);
		break;

	case command:
		if (bp->prt_kind != P_DUMB) { /* amigo -- read the status */
			isw = bp->b_sc->iosw;
			iob->b_xaddr = &bp->prt_stat;
			iob->b_xcount = 1;
			bp->b_resid = bp->b_bcount = 1;
			bp->b_flags |= B_READ;
			START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
			(*isw->iod_preamb)(bp,PR_SEC_DSJ);
			END_TIME_IF(SHORTTIME_1, bp->b_dev);
			iob->b_state = (int)stat1;
			START_TIME_IF(PR_timeout, XFRTIME_1, bp->b_dev);
			(*isw->iod_tfr)(MAX_OVERLAP,bp,PR_intm);
			break;

	case stat1:
			END_TIME_IF(XFRTIME_1, bp->b_dev);
			START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
			(*bp->b_sc->iosw->iod_postamb)(bp);
			END_TIME_IF(SHORTTIME_1, bp->b_dev);
			bp->b_flags &= ~B_READ;
			i = is_err1(bp->prt_stat, bp->prt_kind);
			if (i == 1) {
				bp->b_error = hIO;
				escape(iIO);
			}
			else if (i == 2) { /* read the second DSJ byte */
				isw = bp->b_sc->iosw;
				if (bp->prt_kind == P_2608) {
					bp->prt_stat = 1;
					iob->b_xaddr = &bp->prt_stat;
					iob->b_xcount = 1;
					bp->b_resid = bp->b_bcount = 1;
					bp->b_flags &= ~B_READ;
					START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
					(*isw->iod_preamb)(bp,PR_SEC_STRD);
					END_TIME_IF(SHORTTIME_1, bp->b_dev);
					iob->b_state = (int)intm1;
					START_TIME_IF(PR_timeout, XFRTIME_1, bp->b_dev);
					(*isw->iod_tfr)(MAX_OVERLAP,bp,PR_intm);
					break;

	case intm1:
					END_TIME_IF(XFRTIME_1, bp->b_dev);
					START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
					(*bp->b_sc->iosw->iod_postamb)(bp);
					END_TIME_IF(SHORTTIME_1, bp->b_dev);
					isw = bp->b_sc->iosw;
					iob->b_xaddr = &bp->prt_stat;
					iob->b_xcount = 1;
					bp->b_resid = bp->b_bcount = 1;
					bp->b_flags |= B_READ;
					START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
					(*isw->iod_preamb)(bp,0);
					END_TIME_IF(SHORTTIME_1, bp->b_dev);
					iob->b_state = (int)stat2;
					START_TIME_IF(PR_timeout, XFRTIME_1, bp->b_dev);
					(*isw->iod_tfr)(MAX_OVERLAP,bp,PR_intm);
					break;
				}
				else {  /* 2631 or 2673 */
					isw = bp->b_sc->iosw;
					iob->b_xaddr = &bp->prt_stat;
					iob->b_xcount = 1;
					bp->b_resid = bp->b_bcount = 1;
					bp->b_flags |= B_READ;
					START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
					(*isw->iod_preamb)(bp,PR_SEC_RSTA);
					END_TIME_IF(SHORTTIME_1, bp->b_dev);
					iob->b_state = (int)stat2;
					START_TIME_IF(PR_timeout, XFRTIME_1, bp->b_dev);
					(*isw->iod_tfr)(MAX_OVERLAP,bp,PR_intm);
					break;
				}
		
	case stat2:
				END_TIME_IF(XFRTIME_1, bp->b_dev);
				START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
				(*bp->b_sc->iosw->iod_postamb)(bp);
				END_TIME_IF(SHORTTIME_1, bp->b_dev);
				bp->b_flags &= ~B_READ;
				i = is_err2(bp->prt_stat, bp->prt_kind);
				if (i == 1) {
					bp->b_error = jIO;
					escape(kIO);
				}
				if (i == 3){ /* paper out condition */
					bp->b_error = ENOSPC;
					escape(bp->b_error);
				}
				if (i == 4){ /* off line condition */
					bp->b_error = EBUSY;
					escape(bp->b_error);
				}
				else if (i == 2) { /* wait for ppoll response */
					iob->b_state = (int)get1;
					START_TIME_IF(PR_timeout2, LONGTIME_1, bp->b_dev);
					/*
					 *  Keep the interrupt level at that of
					 *  the card so I will not be interr-
					 *  upted before I drop selcode
					 */
					x = splx(bp->b_sc->int_lvl);
					/*
					 *  Can not use HPIB_ppoll since it
					 *  takes 150 MS on 2608A and 2631
					 */
					HPIB_ppoll_int(bp,PR_intm,1);
					drop_selcode(bp);
					splx(x);
					break;
		case get1:
					/* only for smart printers */
					END_TIME_IF(LONGTIME_1, bp->b_dev);
					iob->b_state = (int)command;
					get_selcode(bp,PR_intm);
					break;
				}
				/* otherwise do not wait for ppoll and drop to tfr */
			}
		}

	case tfr:
			isw = bp->b_sc->iosw;
			iob->b_xaddr = bp->b_un.b_addr;
			/* restore the actual number of data bytes */
			bp->b_resid = bp->b_bcount = iob->COUNT;
			iob->b_xcount = bp->b_bcount;
			bp->b_flags &= ~B_READ;
			START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
			if (bp->prt_kind != P_DUMB) /* only amigo */
				(*isw->iod_preamb)(bp,PR_SEC_DATA);
			else
				(*isw->iod_preamb)(bp,0);
			END_TIME_IF(SHORTTIME_1, bp->b_dev);
			/*
			   allow a longer timeout in overlap mode because
			   it doesn't hang the system.
			*/
			iob->b_state = (int)report;
			START_TIME_IF(PR_timeout, XFRTIME_1, bp->b_dev);
			(*isw->iod_tfr)(MAX_OVERLAP,bp,PR_intm);
			break;

	case report:
		END_TIME_IF(XFRTIME_1, bp->b_dev);
		START_TIME_IF(PR_timeout, SHORTTIME_1, bp->b_dev);
		(*bp->b_sc->iosw->iod_postamb)(bp);
		END_TIME_IF(SHORTTIME_1, bp->b_dev);

	case get3:
		iob->b_state = (int)defaul;
		drop_selcode(bp);
		queuedone(bp);
		break;

	case timedout:
		bp->b_error = lIO;
		escape(TIMED_OUT);

	case timedout2:
		bp->b_error = ETXTBSY;
		escape(ETXTBSY);

	default:
		printf("lph: Unrecognized PR state = %d\n", iob->b_state);

	}
	END_FSM;
	recover {
		ABORT_TIME;
		if (!((bp->b_error == ENOSPC) || (bp->b_error == EBUSY))){
		/*
		 *  In case there is an io operation in progress then
		 *  this wipes out DMA's nose. Also it there is a wait
		 *  on ppoll this will clear that too
		 */
			try
				HPIB_ppoll_clear(bp);
				(*bp->b_sc->iosw->iod_abort)(bp);
			recover{}
			if (bp->b_error != ETXTBSY){
				bp->b_error = mIO;
			}
		}
		bp->b_flags |= B_ERROR;
		drop_selcode(bp);
		queuedone(bp);
		iob->b_state = (int)defaul;
	}
}


PR_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,PR_dumb,0,1,timedout);
}

PR_timeout2(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,PR_dumb,0,1,timedout2);
}


/*
 * Decode the error condition of the printer.
 * return 4 if the printer is off line.
 * return 3 if the printer is out of paper or carriage stall.
 * return 2 if the printer is not ready for data only.
 * return 1 if the printer has fatal problem.  i.e. power failure
 * return 0 if everything is fine.
 */
/*
 * NOTE:	The 2608 section needs to be updated to this
 */

is_err2(c, prt_kind)
char c;
register int prt_kind;
{
	register int flag;

	flag = 1;
	switch (prt_kind) {

	case P_2227:
	case P_2631:
	case P_2673:
	case P_2932:
	case P_3630:
	case P_1602:
		{
			     if   (c & PR_I_POWER )  flag = 1;	/* power      */
								/* fail.      */
			else if (!(c & PR_I_ONLINE)) flag = 4;	/* off line.  */
			else if (  c & PR_I_PAPER  ) flag = 3;	/* out of     */
								/* paper or   */
								/* carriage   */
								/* stall.     */
			else if (!(c & PR_I_RFD   )) flag = 2;	/* not ready  */
								/* for data.  */
			else if ((c & PR_I_ONLINE) && (!(c & PR_I_PAPER)) &&
			         (c & PR_I_RFD   ) && (!(c & PR_I_POWER)))
					flag = 0;	/* online, has paper, */
							/* ready for data and */
							/* no power fail      */
			break;
		}
	case P_2225:
		{
			if ((c & PR_I_OOP) || (c & PR_I_CMD)) {/* error */}
			else if (!(c & PR_I_BUFF) && (c & PR_I_BUFE)) flag = 0;
			else if (!(c & PR_I_BUFE) || (c & PR_I_BUFF)) flag = 2;
			break;
		}
	case P_2608:
		{
			if((c & PR_I_POW) || (!(c & PR_I_LINE))) {/* error */}
			else if(c & PR_I_OPSTAT) {
				flag = 2;
			} else flag = 0;
			break;
		}
	default:
		printf("lph: stat on unknown printer = %d\n", prt_kind);
	}
	return flag;
}

/*
 * Decode the first byte of DSJ.
 * return 0 if the printer is ready for data.  NOTE: for some of
 *          the printers, this state will not be returned because
 *          we want to force the reading of secondary status
 *	    because some printers do not specify read secondary
 *	    status if the printer is off line.
 * return 1 if there is a real error.
 * return 2 if second DSJ byte should be read.
 */

is_err1(st,prt_kind)
char st;
register int prt_kind;
{
	register int flag;

	flag = 1;
	switch (prt_kind) {

	case P_2225:
	case P_2227:
	case P_2631:
	case P_2673:
	case P_2932:
	case P_3630:
	case P_1602:
		{
			switch (st){
			case PR_RFDATA:
				flag = 2;
				break;
			case PR_SDS:
				/* lp has data to send */
				break;
			case PR_RIOSTAT:
				/* read second byte of DSJ */
				flag = 2;
				break;
			}
			break;
		}
	case P_2608:
		{
			switch (st){
			case PR_RFDATA:
				flag = 0;
				break;
			case PR_ATTEN:
			case PR_ATT_PAR:
				flag = 2;
				break;
			}
			break;
		}
	default:
		printf("lph: stat on unknown printer = %d\n", prt_kind);
	}
	return flag;
}


/* This is the key interface routine to the low level drivers. */
PR_control(proc, bp, bufr, len)
register int (*proc)();
register struct buf *bp;
caddr_t  bufr;
int	 len;
{

	/* make sure the buffer is available before we use it */
	acquire_buf(bp);

	bp->b_un.b_addr = bufr;
	bp->b_bcount = len;
	bp->b_action = proc;

	enqueue(bp->b_queue, bp);

	iowait(bp);

	release_buf(bp);

	bp->b_flags = 0;

	u.u_error = 0; /* !@#$%^& */
	return(bp->b_error);
}

PR_control2(proc, bp, bufr, len, tp)
register int (*proc)();
register struct buf *bp;
caddr_t  bufr;
int	 len;
struct	 PR_stat *tp;
{
	caddr_t	local_bufr;
	caddr_t	new_buf;
	int	error;
	int	local_rec_size;
	int	local_len;
	int	new_len;

	local_rec_size = tp->t_rec_size;
	local_len      = len;
	local_bufr     = bufr;

	while (local_len > 0){
		if (local_len > local_rec_size){
			new_len = local_rec_size;
		}else{
			new_len = local_len;
		}

		if (error = PR_controlw(proc, bp, local_bufr, new_len)){
			return(error);
		}

		local_bufr = local_bufr + local_rec_size;
		local_len  = local_len  - local_rec_size;
	}
	return(error);
}
/*
 * This routine is similar to the routine PR_control.
 * PR_control is used to do most of the work.
 *
 * The features that this routine provides over
 * PR_control are:
 *
 * 1)	The driver will wait for a off-line, paper out
 *	or carriage stall condition to be corrected.
 *
 * 2)	The driver will wait ten seconds before checking to see
 *	if the printer is on line.
 *
 * 3)	The driver will wait ten seconds before checking to see
 *	if the printer has paper loaded.
 *
 * 4)	The driver will wait two seconds before checking to see
 *	if the printer has corrected the carriage stall.
 *
 *	NOTE:	The carriage stall is reported as a paper out
 *		condition.  Because of this, the first wait
 *		for a paper out condition will be two seconds.
 *		The other waits will be ten seconds.
 *
 *		A off line condition is specified as EBUSY.
 *
 *		A alternate off line condition is specified as
 *		ETXTBSY.  ETXTBSY means that a ppoll timed out
 *		while checking the secondary status.  This is
 *		usually caused by a printer being off line and
 *		the not ready for data condition being used to
 *		indicate it.  The Rugged Writer 480 always
 *		on line, even if it is not.  Because of this
 *		the not ready for data status is used to provide
 *		the information to cause a delay waiting for
 *		the condition to be corrected.
 *
 *		A paper out condition is specified as ENOSPC
 *		(EBUSY and ENOSPC were chosen as the off line
 *		and paper out indicators for no particular
 *		reason.  They look good at the time.  I am
 *		including this incase someone needs to change
 *		them in the future).
 *
 */
PR_controlw(proc, bp, bufr, len)
register int (*proc)();
register struct buf *bp;
caddr_t  bufr;
int	 len;
{
	int	error;
	struct	proc *p = u.u_procp; /* XXX */
	int	timeout_count;
	int	first_time;

	first_time = TWOSECONDS;

PR_controlw_wait1:
						/* Go do the I/O */
	error = PR_control(proc, bp, bufr, len);
						/* Check for timeout */
						/* or paper out or   */
						/* ppoll timeout.    */
	if ((error == ENOSPC) || (error == EBUSY) || (error == ETXTBSY)){
		if (ISSIG(p)){
			return(EINTR);
		}
		if ((first_time == TWOSECONDS) && (error == ENOSPC)){
			timeout_count = TWOSECONDS;
			first_time    = TENSECONDS;
		}else{
			timeout_count = TENSECONDS;
		}
		if (error == ETXTBSY){
			timeout_count = TWOSECONDS;
		}
		amg_wait(bp,timeout_count);
		if (ISSIG(p)){
			return(EINTR);
		}
		goto PR_controlw_wait1;
	}
	return(error);
}
