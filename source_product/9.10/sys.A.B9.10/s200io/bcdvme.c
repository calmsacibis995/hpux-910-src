/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/bcdvme.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:10:00 $
 */

/* HPUX_ID: @(#)bcdvme.c	55.1		88/12/23 */

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
 *****************************************************************
 * VME driver 
 * @(#)vme.c	1.24 - 4/2/87
 * Hal Prince
 * BCD R&D
 * October 1985
 *****************************************************************
 */


#include "../h/types.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../wsio/intrpt.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/sysmacros.h"
#include "../s200io/vme.h"
#include "../h/utsname.h"

/*
 * things used by other files 
 */

int vme_open(), 
    vme_close(), 
    vme_read(), 
    vme_write(),
    vme_ioctl(), 
    vme_link();

/*
 *****************************************************************
 * bits in the card registers, access to registers
 *****************************************************************
 */
  
/* names of the VME registers */
#define RID		1	
#define RINTSTATUS	3
#define RVMESTATUS	7
#define WRESET		1
#define WINTCNTRL	3
#define WHIADDR		5
#define WVMECNTRL	7

/* RID contains the VME ID */
#define VMEID 		17 

/* RINTSTATUS has interrupt info */
#define IEN		0x80		/* enabled */
#define IRQ		0x40		/* requested */
#define VMELEVEL(x)	((~(x))&07)	/* vme int level */
#define DIOLEVEL(x)	((((x)>>4)&03)+3) /* hpux int level */

/* RVMESTATUS has VME bus status */
#define BCLR 		01		/* 0 true, 1 false */
#define BGIN 		02		/* 0 true, 1 false */

/* WRESET resets card with any write */

/* WVMECNTRL requests bus, has address modifier */
#define BR 		01		/* bus request */
#define BBSY 		02		/* bus busy */
/* set address modifier */
#define AMSET(reg,am)	((reg) = ((reg)&3) + ((am)<<2))
/*
 * WHIADDR has the top 8 (of 24) address bits. 
 * This selects a 64K block of the VME address space.
 * The 98646A maps this block to the DIO addresses
 * occupied by the second select code (98646A takes 2 select codes).
 */
#define MOFFSET		0x10000		/* offset of VME map from card addr */
#define MSIZE		0x10000		/* size of VME map */
#define HIADDR(x)	(((x)>>16)&0xff)
#define LOADDR(x)	((x)&0xffff)

/* WINTCNTRL handles interrupts */
#define IACK		01		/* acknowledging interrupt */
/*      IEN		0x80		/* defined above */

/*
 * Note that we cannot do bit sets and bit clears to the write
 * registers, because the corresponding read registers interfere.
 * Therefore, we keep a copy of the write registers in the vme_od
 * table, set and clear the bits there, then write the whole byte
 * to the register.
 */

/*
 *****************************************************************
 * data structure for device status
 *****************************************************************
 */

#define MAXVME 8		/* max number of vme cards */

static
struct vme_od {
	char sel_code;		/* select code */
	char iocount;		/* count of procs doing i/o */
	char opencount;		/* count of procs with this device open */
	char strategy;		/* when to release bus */
	char synflags;		/* open, granted, locking */
	char wvmecntrl;		/* copy of WVMECNTRL reg */
	char wintcntrl;		/* copy of WINTCNTRL reg */
	char *card_addr;	/* card address */
} vme_od[MAXVME];

/* release strategy values in <vme.h> */

/* synflag values */
#define VGRANTED	01		/* bus is granted */
#define VLOCKED		02 		/* mutual exclusion */
#define VWANTED		04 		/* want mutex code */
#define VGHOSTI	       010		/* want ghost interrupt */

/*
 *****************************************************************
 * stuff relating to vmeio structures, define in vme.h
 *****************************************************************
 */

/*
 * timeout:
 * UHZ -- how often (per second) user time units occur
 *	(i.e., user time in 1/UHZ secs)
 * Since the VME can be busy for any amount of time, we
 * go into a sleep-then-check loop where the sleep time
 * doubles each cycle.  MINDELAY is the initial sleep, and
 * MAXDELAY the max.  MAXDELAY/MINDELAY should be a power of 2.
 * Both delays are in 1/HZ sec ticks
 */
#define UHZ		100		/* user unit is .01 secs */
#define MINDELAY	1		/* wait 1 tick at first */
#define MAXDELAY	128		/* about every 2 seconds */

/* default parameters */
#define DEFTIMEOUT	0
#define DEFADDMOD	0x39		/* 24-bit, user, data */
#define DEFIOFLAGS	0
#define DEFSTRATEGY     VRWD

/*
 *****************************************************************
 * Interrupt stuff
 * procp wants signal when interrupt from statusid comes on odp
 *****************************************************************
 */
#define INTTAB		10		/* max outstanding requests */
struct inttab {
	struct proc *procp;		
	struct vme_od *odp;
	int statusid;
	unsigned short readsid;
	char signal;
	char state;
} inttab[INTTAB];

/* states: NEW -> WANTI -> GOTI [ -> READI ] -> out of table */
#define NEW	0		/* not yet valid */
#define WANTI	1		/* ready for interrupt */
#define GOTI	2		/* interrupt occurred, signal sent */
#define READI	3		/* VMEREADI occurred */
		
/* delay until next interrupt if this one can't get bus */
#define GHOSTDELAY	HZ		/* 1 sec */

/* stuff for isrlink */
#define	INTLEVEL	3		/* required interrupt level */
#define INTMASK		(IEN|IRQ)	/* look at these bits */
#define INTRESULT	(IEN|IRQ)	/* expect this result */

/*
 *****************************************************************
 * miscellaneous stuff
 *****************************************************************
 */
/* name of card for boot message */
char VMENAME[] = "HP98646A";

/* debugging */
#ifdef VMEDEBUG
#define VDEBUG _IO('V',999)
static int debug = 1;
#endif

/* sleep priority -- must be interruptable (> PZERO) */ 
#define PVME		PSLEP

/* modes for lookup routines */
#define LOOKUP	1
#define CREATE	2

/* what sort of copy for xcopy */
#define	NOFIX	0	/* auto-increment both addresses */
#define FIXFROM	1	/* from address held fixed */
#define FIXTO	2	/* to address held fixed */

/* block shifting and masking -- must match routines in vmeas.s */
#define BLKSHIFT	6
#define BLKMASK		077


/*
 * A few words on synchronization:
 * There are three problem areas.
 * (1) When a process is negotiating for the bus, others must
 * leave it alone.  This is done with a VLOCKED around the
 * code in vme_get_bus.  (2) When a process is doing i/o, the same situation
 * occurs.  This is done with VLOCKED and iocount in vme_rw.  VLOCKED
 * enforces a one-proc-only rule inside the i/o code, and
 * iocount keeps track of how many procs are waiting between the
 * vme_get_bus and the io code in vme_rw.  Without iocount, 
 * the following can occur: proc A enters vme_rw, 
 * gets the bus, starts doing i/o.  proc B enters vme_rw,
 * gets the bus, and sleeps on the VLOCKED.  proc A finishes
 * its i/o, and wakes up proc B.  before proc B restarts,
 * proc C gets into and out of vme_rw, dropping the
 * bus on the way out.  proc C shouldn't have dropped the
 * bus, but without iocount, it can't tell if anyone is there.
 * (3) The interrupt routine must look at the inttab, and
 * it must know whether the bus is granted or not.  It must
 * also acquire the bus if it isn't granted.  All code that
 * manipulates inttab does spl3 to prevent interrupts when
 * the table is being examined and changed.  The code that
 * drops VGRANTED (vme_release_bus) does the same.  The code
 * that sets VGRANTED (vme_get_bus) sets VLOCKED when trying to
 * get the bus.  Therefore, the rule for the interrupt routine
 * is: a) if VGRANTED is true, it can use the bus.  b) if VGRANTED
 * is false, and VLOCKED is false, it can try to get the bus
 * without interfering with the interrupted routine.  c) if
 * VGRANTED is false and VLOCKED is true, then vme_get_bus is
 * in progress, and the isr must not touch anything.  In case
 * (c), it schedules a retry later.
 */

/*
 *****************************************************************
 * Lookup routines for status table entries and
 * per-open data area. 
 * Also interrupt structures.
 *****************************************************************
 */

/*
 * Given the major/minor device number, find the
 * entry in vme_od describing that device.
 * Returns NULL if not found.
 */
static struct vme_od *
find_od(dev)
	register dev_t dev; {
	register sc;
	register struct vme_od *odp;

	sc = m_selcode(dev);
	for (odp = vme_od; odp < &vme_od[MAXVME] && odp->sel_code; odp++) 
		if (odp->sel_code == sc)
			return odp;
	return NULL;

}

/*
 * Get the vmeio structure for this file descriptor.
 * This is stuck in a per-open buffer hidden in the
 * file structure.
 * LOOKUP means return the default if none there.
 * CREATE means allocate a new one in that case.
 * The getebuf always succeeds (sleeps until buffer available).
 */
static struct vmeio def_io = {
		DEFTIMEOUT,
		DEFADDMOD,
		DEFIOFLAGS,
		DEFSTRATEGY
};

static struct vmeio *
get_vmeio(mode) {
	register struct buf *bp;

	/* assign buffer if not there and creating */
	if (u.u_fp->f_buf == NULL && mode == CREATE) 
		u.u_fp->f_buf = (caddr_t) geteblk(sizeof(struct vmeio));

	/* return it if there */
	if (bp = (struct buf *) u.u_fp->f_buf) 
		return (struct vmeio *) bp->b_un.b_addr;

	/* otherwise use the default */
	return &def_io;

}

/*
 * Get the interrupt entry for this process/statusid/odp 
 * combination.  CREATE means make a new one if none found.
 * Returns NULL if none found or table full.
 * goti means that the state must be GOTI; otherwise we don't care.
 */
static
struct inttab *
get_inttab(odp, sid, mode, goti)
	register sid;
	register struct vme_od *odp; {
	register struct inttab *ip, *freep;

	freep = NULL;
	for (ip = inttab; ip < &inttab[INTTAB]; ip++) {
		if (!ip->procp) {
			freep = ip;
			break;
		}
		if (ip->procp != u.u_procp)
			continue;
		if (ip->odp != odp)
			continue;
		if (goti && ip->state != GOTI)
			continue;
		if (sid == ALLDEVICES 
		    || ip->statusid == ALLDEVICES 
		    || sid == ip->statusid) {
#if VMEDEBUG
			if (debug)
				printf("get_int found entry %x\n", ip);
#endif
			return ip;
		}
	}
	if (mode == CREATE && freep) {
		freep->procp = u.u_procp;
		freep->odp = odp;
		freep->state = NEW;
		freep->statusid = sid;
#ifdef VMEDEBUG
		if (debug)
			printf("get_int new entry %x\n", freep);
#endif
		return freep;
	}
	return NULL;

}

/*
 *****************************************************************
 * Routines that link the driver into the kernel at run time.
 *****************************************************************
 */

/*
 * vme_link links our make_entry into the make_entry chain.
 */
extern int (*make_entry)();
static int (*saved_make_entry)();
static int bcdvme_make_entry();
static bcdvme_init();

/*
 * make_entry puts this table into the isc_table; then vme_init
 * is called for each VME card there.
 */
static
struct drv_table_type bcdvme_iosw = {
	bcdvme_init,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL
};


vme_link(){

	saved_make_entry = make_entry;
	make_entry = bcdvme_make_entry;

}

/*
 * Our make_entry is called with each ID.  If it belongs to us,
 * we initialize a status entry (vme_od) and the isc_table.
 * Otherwise we just call the next make_entry.
 */
static
bcdvme_make_entry(id, isc)
	int id;
	register struct isc_table_type *isc; {
	register struct vme_od *odp;
	static int bcdvme_isr();

	/* is it our card? */
	if (id != VMEID
#ifdef VMEDEBUG
		&& id != 127 /* prototype ID */
#endif
			)
		return (*saved_make_entry)(id, isc);
	
	/* find vacant slot in open device table */
	for (odp = vme_od; odp < &vme_od[MAXVME]; odp++)
		if (!odp->sel_code)
			break;
	    
	/* if last slot, table full */
	if (odp == &vme_od[MAXVME])
		return io_inform(VMENAME, isc, -2,
			      " ignored; too many of this interface");

	/* claim select code */
	odp->sel_code = isc->my_isc;
	odp->card_addr = (char *) isc->card_ptr;
#ifdef VMEDEBUG
	if (debug)
		printf("vme card addr %x\n", odp->card_addr); 
#endif
	isc->iosw = &bcdvme_iosw;
	isc->card_type = HP98646;


	/* Link in interrupt routine. */
	if (isc->int_lvl == INTLEVEL) 
		isrlink(bcdvme_isr, INTLEVEL, &odp->card_addr[RINTSTATUS],
			INTMASK, INTRESULT, 0, odp);

	return io_inform(VMENAME, isc, INTLEVEL);

}
		
/*
 * Reset the VME card.
 * Called once per card, but we just initialize all cards
 * on the first call.
 */
/*ARGSUSED*/
static
bcdvme_init(sc){ 
	register struct vme_od *odp;
	static char firstcall = TRUE;
	
	if (firstcall)
	for (odp = vme_od; odp < &vme_od[MAXVME] && odp->sel_code; odp++) 
		bcdvme_reset(odp);
	firstcall = FALSE;
	
}

/* 
 *****************************************************************
 * Low-level operations on the card.
 * Reset card, get bus, release bus.
 *****************************************************************
 */

/*
 * Reset card by writing anything to reset register.
 */
static
bcdvme_reset(odp)
	register struct vme_od *odp; {

	odp->card_addr[WRESET] = 0;
	odp->wvmecntrl = 0;
	odp->wintcntrl = 0;

}

/*
 * Get control of the VME.
 * Sets VGRANTED if successful.
 * Returns error code; 0 means success (no error).
 */
static
vme_get_bus(odp)
	register struct vme_od *odp; {
	register char *rstatus, *wcntrl;
	register granted, error; 
	int timeleft;

	/* already granted? */
	if (odp->synflags & VGRANTED)
		return 0;

	/*
	 * If not VGRANTED, but VLOCKED, it's because
	 * another process is trying to get the bus.
	 * We wait for exclusive access.
	 */
	while (odp->synflags & VLOCKED) {
		odp->synflags |= VWANTED;
#if VMEDEBUG
		if (debug)
			printf("get_bus sleeping on lock\n");
#endif
		if (sleep((caddr_t)&odp->synflags, PVME|PCATCH)) 
			return EINTR;
	}

	/*
	 * The process ahead of us may have already gotten
	 * the bus, so we check this first.
	 */
	if (odp->synflags & VGRANTED)
		return 0;

	odp->synflags |= VLOCKED;
#ifdef VMEDEBUG
	if (debug)
		printf("get_bus locked\n");
#endif

	/*
	 * clear BBSY (cautionary measure; should be clear already)
	 * set BR
	 * set BBSY
	 * wait for BGIN to drop (to go back to 1)
	 * drop BR
	 */
		
	rstatus = &odp->card_addr[RVMESTATUS];
	wcntrl = &odp->card_addr[WVMECNTRL];

	*wcntrl = (odp->wvmecntrl &= ~BBSY);
	*wcntrl = (odp->wvmecntrl |= BR);

	/*
	 * Wait for VME to drop BGIN.
	 * One quick check, then start the delaying loop.
	 * We delay MINDELAY at first, then double the
	 * delay each time, until the user timeout expires,
	 * or forever if the user timeout is 0.
	 * There is a MAXDELAY to prevent overflows and
	 * to allow checks for signals.
	 * The idea is that we have no idea how long the
	 * VME will be busy, so we check often at first,
	 * then settle into a less frequent check.
	 */
	error = 0;
	granted = TRUE;
	if (*rstatus & BGIN) {
	    timeleft = get_vmeio(LOOKUP)->timeout;
	    while (*rstatus & BGIN) {

		    /* update timeleft */
		    if (timeleft) {
			    timeleft -= MINDELAY;
			    if (timeleft <= 0) {
				    granted = FALSE;
				    error = EBUSY;
				    break;
			    }
		    }
	     }

	}
	/*
	  POSSIBLE GLITCH:
		WE GIVE UP
		BGIN ARRIVES
		WE DROP BR
		DO WE THEN PASS BGOUT?
	*/
#ifdef VMEDEBUG
	if (debug)
		printf("get_bus granted: %d\n", granted);
#endif

	/* don't set VGRANTED until bus useable by isr */
	if (granted) 
		*wcntrl = (odp->wvmecntrl |= BBSY);
	*wcntrl = (odp->wvmecntrl &= ~BR);

	if (granted) 
		odp->synflags |= VGRANTED;
	

	odp->synflags &= ~VLOCKED;
	if (odp->synflags & VWANTED) {
		odp->synflags &= ~VWANTED;
		wakeup(&odp->synflags);
	}

	return error;
}

/* 
 * Release the bus if no one is using it.
 * We first make sure bus is VGRANTED and that no one is doing i/o.
 */
static
vme_release_bus(odp)
	register struct vme_od *odp; {
	register char *wcntrl;
	register x;

	if (!(odp->synflags & VGRANTED) || odp->iocount)
		return;

	wcntrl = &odp->card_addr[WVMECNTRL];
	x = spl3();

	/*
	 * clear BR (cautionary; should be clear already)
	 * clear BBSY
	 * for safety, don't do both in the same instruction
	 */
	*wcntrl = (odp->wvmecntrl &= ~BR);
	*wcntrl = (odp->wvmecntrl &= ~BBSY);

	odp->synflags &= ~VGRANTED;
	splx(x);
#ifdef VMEDEBUG
	if (debug)
		printf("release bus\n");
#endif

}

/*
 * Delay the given number of ticks.
 * sleep returns 0 if aroused by wakeup, 1 if by interrupt.
 * If by interrupt, we must cancel the wakeup call.
 * Sleeping on procp+2 is a little wierd, but it's a per-process
 * address not currently used elsewhere.
 */
static
delay(ticks)
	int ticks; {
	register x, ret;
	register caddr_t sleepaddr;
	
#ifdef VMEDEBUG
	if (debug)
		printf("delay %d ticks\n", ticks);
#endif
	sleepaddr = ((caddr_t) u.u_procp) + 2;

	/* must prevent wakeup until we sleep */
	x = spl7();
	timeout(wakeup, sleepaddr, ticks, NULL);
	ret = sleep(sleepaddr, PVME|PCATCH);
	if (ret)
		untimeout(wakeup, sleepaddr);
	splx(x);

	return ret;

}

/*
 *****************************************************************
 * High-level exported routines.
 * open/close/read/write/ioctl.
 *****************************************************************
 */

/*ARGSUSED*/
vme_open(dev, mode)
	dev_t dev;
	int mode; {
	register struct vme_od *odp;

	/*
	 * Be sure minor device number describes a device
	 * in our table.
	 */
	if ((odp=find_od(dev)) == NULL)
		return ENXIO;
	
	/* show open, init timeout, strategy */
	if (!odp->opencount) {
#ifdef VMEDEBUG
		if (debug)
			printf("first open for %x\n", odp->card_addr);
#endif
		/* first open */
		/* selcode, card_addr set at boot time */
		odp->strategy = VRWD;
		odp->synflags = 0;
		odp->iocount = 0;
	}
	++odp->opencount;
	u.u_fp->f_buf = NULL;

	return 0;
}

/*
 * All closes come here, not just last one.
 */
/*ARGSUSED*/
vme_close(dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct vme_od *odp;
	register struct buf *bp;
	register error, x;

	bp = (struct buf *)u.u_fp->f_buf;

	/* find entry */
	if ((odp=find_od(dev)) == NULL)
		panic("VME close: no entry");

	/* release bus if last close */
	if (--odp->opencount == 0)
		vme_release_bus(odp);
	
	/* drop buffer if had one */
	if (bp) {
		brelse(bp);
	        u.u_fp->f_buf = NULL;
	}	

	/* release interrupt table entries */
	x = spl3();
	error = rel_int(odp, ALLDEVICES);
	if (!error)
		checkienable(odp);
	splx(x);

#ifdef VMEDEBUG
	if (debug)
		printf("vme_close on %x: %d opens\n", 
			odp->card_addr, odp->opencount);
#endif
	return 0;
}

/*ARGSUSED*/
vme_ioctl(dev, cmd, vmeio, flag)
	dev_t dev; 
	register struct vmeio *vmeio; {
	register struct vme_od *odp;
	register struct vmeio *iob;

	/* find entry */
	if ((odp=find_od(dev)) == NULL)
		panic("VME ioctl: no entry");

	switch(cmd) {
	
#ifdef VMEDEBUG
	  case VDEBUG:
		debug = !debug;
		break;
#endif

	  case VMESET:
		return vmeset(odp, vmeio);

	  case VMEGET:
		vmeio->strategy = odp->strategy;
		iob = get_vmeio(LOOKUP);
		vmeio->timeout = iob->timeout;
		vmeio->addmod = iob->addmod;
		vmeio->ioflags = iob->ioflags;
		break;

	  case VMEWANTI:
		return vmewanti(odp, (struct vmeint *) vmeio);

	  case VMEREADI:
		return vmereadi(odp, (struct vmeint *) vmeio);

	  case VMEENDI:
		return vmeendi(odp, (struct vmeint *) vmeio);

	  default:
		return EINVAL;

	}
	return 0;

}

vme_read(dev, uio)
	dev_t dev;
	struct uio *uio; {

	return vme_rw(dev, uio, UIO_READ);

}

vme_write(dev, uio)
	dev_t dev;
	struct uio *uio; {

	return vme_rw(dev, uio, UIO_WRITE);

}

vme_buserror()
{
	longjmp(&u.u_psave);
}

/*
 *****************************************************************
 * read/write
 *****************************************************************
 */

static
vme_rw(dev, uio, dir) 
	dev_t dev;
	register struct uio *uio; 
	enum uio_rw dir; {
	register struct vme_od *odp; 
	register struct vmeio *vmeio;
	char *map_base;
	register addr, count, to_edge, error; 
	u.u_error = 0;

	/* find entry */
	if ((odp=find_od(dev)) == NULL)
		panic("VME rw: no entry");

	/* get bus */
	error = vme_get_bus(odp);
	if (error)
		return error;

	++odp->iocount;
	/*
	 * The move from user area to io space
	 * or vice versa can cause a process switch,
	 * because of virtual memory.
	 * Therefore, we must ensure that the io code
	 * has only one user at a time in it; otherwise,
	 * one process might change the hi address bits
	 * in the middle of another proc's io.
	 */
	while (odp->synflags & VLOCKED) {
		odp->synflags |= VWANTED;
		if(sleep((caddr_t)&odp->iocount, PVME|PCATCH)) {
			--odp->iocount;
			return EINTR;
		}
	}
	odp->synflags |= VLOCKED;

	vmeio = get_vmeio(LOOKUP);
	odp->card_addr[WVMECNTRL] = AMSET(odp->wvmecntrl, vmeio->addmod);
	map_base = odp->card_addr + MOFFSET;

	/* set probe_addr so a fault will come back here */
	u.u_probe_addr = (int)vme_buserror;
	if (setjmp(&u.u_psave))
		error = EFAULT;
	else 
	while (uio->uio_resid) {
		addr = (int) uio->uio_offset;
		odp->card_addr[WHIADDR] = HIADDR(addr);
		addr = LOADDR(addr);
		to_edge = MSIZE - addr;	
		count = (to_edge < uio->uio_resid? to_edge: uio->uio_resid);
#ifdef VMEDEBUG
		if (debug)
			printf("uiomove(%x, %d, %d, uio, %d)\n", map_base+addr,
				count, dir, vmeio->ioflags);
#endif
		uiomove(map_base + addr, count, dir, uio, vmeio->ioflags);
		if (u.u_error) {
			error = u.u_error;
			break;
		}
	}
	u.u_probe_addr = NULL;

	odp->synflags &= ~VLOCKED;
	if (odp->synflags & VWANTED) {
		odp->synflags &= ~VWANTED;
		wakeup(&odp->iocount);
	}
	--odp->iocount;
    
	switch (odp->strategy) {
	  case VRWD:
		vme_release_bus(odp);
		break;
	  case VROR:
		if (!(odp->card_addr[RVMESTATUS] & BCLR))
			vme_release_bus(odp);
		break;
	}
	return error;
}


/*
 * Moving data from user area to VME and vice versa.
 * uiomove() is much like the kernel's; differences are
 * (a) we don't look at segflg (always the same)
 * (b) we take extra param ioflags for VBYTE and VFIXADDR
 * (c) we call xcopy (understands ioflags) instead of copyin/copyout
 * (d) we don't return an error code; caller must catch bus errors.
 */

static
uiomove(cp, n, rw, uio, iof)
	register caddr_t cp;
	register int n;
	enum uio_rw rw;
	register struct uio *uio; {
	register struct iovec *iov;
	u_int cnt;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		if (rw == UIO_READ)
			xcopy(cp, iov->iov_base, TRUE, cnt, iof);
		else
			xcopy(iov->iov_base, cp, FALSE, cnt, iof);

		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
}

/*
 * arrays of copy routines for xcopy, indexed by 
 * NOFIX/FIXFROM/FIXTO 
 */
int xbytai(), xbytfix1(), xbytfix2();
int xwdai(), xwdfix1(), xwdfix2();
int xblkai(), xblkfix1(), xblkfix2();
int (*xbyt[3])() = { xbytai, xbytfix1, xbytfix2 };	/* 1 byte */
int (*xwd[3])() = { xwdai, xwdfix1, xwdfix2 };		/* 2 byte */
int (*xblk[3])() = { xblkai, xblkfix1, xblkfix2 };	/* 64 byte */

/*
 * xcopy copies a block of data.  It decides whether to use byte,
 * word, or 64-byte block moves based on VBYTE, the addresses, 
 * and the count.  It implements FIXADDR also.
 */
static
xcopy(from, to, readvme, count, ioflags)
	register char *from, *to;
	register count; {
	register fix, bcount;

	if (count <= 0) return;

	/* load source and destination function code registers */
	loadfc(readvme);

	/* set fix to index the right subroutine */
	if (ioflags & VFIXADDR)
		if (readvme)
			fix = FIXFROM;
		else 	fix = FIXTO;
	else	fix = NOFIX;
#ifdef VMEDEBUG
	if (debug)
		printf("fix %d\n", fix);
#endif

	/* xfer bytes if user wants to */
	if (ioflags & VBYTE) {
#ifdef VMEDEBUG
		if (debug)
			printf("forced byte xfer %x, %x, %d\n", 
				from, to, count);
#endif
		(*xbyt[fix])(from, to, count);
		return;
	}

	/* xfer words if user wants to */
	if (ioflags & VWORD) {
		if (((int)to & 1) || ((int)from & 1) || ((int)count & 1)) {
			u.u_error = EINVAL;
			return;
		}
#ifdef VMEDEBUG
		if (debug)
			printf("forced word xfer %x, %x, %d\n", 
				from, to, count);
#endif
		(*xwd[fix])(from, to, count);
		return;
	}

	/* get to address even */
	if ((int)to & 1) {
#ifdef VMEDEBUG
		if (debug)
			printf("xfer 1 byte\n");
#endif
		(*xbyt[fix])(from, to, 1);
		if (--count <= 0) return;
		if (fix != FIXFROM) ++from;
		if (fix != FIXTO) ++to;
	}

	/* if from now odd, must use byte xfer */
	if ((int)from & 1) {
#ifdef VMEDEBUG
		if (debug)
			printf("to/from mismatch\n");
#endif
		(*xbyt[fix])(from, to, count);
		return;
	}

	/* to and from both even, so can use big moves if count ok */
	/* try 64-byte blocks */
	if (bcount = (count >> BLKSHIFT)) {
#ifdef VMEDEBUG
		if (debug)
			printf("blk xfer %x, %x, %d\n", from, to, bcount);
#endif
		(*xblk[fix])(from, to, bcount);
	}
	
	/* nothing left over => we're done */
	if (!(count &= BLKMASK))
		return;

	/* update from and to */
	bcount <<= BLKSHIFT;
	if (fix != FIXFROM) from += bcount;	
	if (fix != FIXTO) to += bcount;	
	
	/* leftover in words, then maybe last byte */
	bcount = count & 1;
	count >>= 1;
	if (count) {
#ifdef VMEDEBUG
		if (debug)
			printf("word xfer %x, %x, %d\n", from, to, count);
#endif
		(*xwd[fix])(from, to, count);
	}

	/* may be one byte at the end */
	if (bcount) {
		count <<= 1;
		if (fix != FIXFROM) from += count;	
		if (fix != FIXTO) to += count;	
#ifdef VMEDEBUG
		if (debug)
			printf("byte xfer %x, %x, %d\n", from, to, bcount);
#endif
		(*xbyt[fix])(from, to, bcount);
	}

}

/*
 *****************************************************************
 * ioctl subroutines
 *****************************************************************
 */

static
vmeset(odp, vmeio)
	register struct vmeio *vmeio;
	register struct vme_od *odp; {
	register struct vmeio *iob;
	register s;

	/* check for bad values */
	if (vmeio->ioflags & ~(VBYTE|VFIXADDR|VWORD))
		return EINVAL;
	if ((s=vmeio->strategy) != VROR && s != VRWD && s != VROCLOSE)
		return EINVAL;

	/* strategy is per device, not per process */
	switch(odp->strategy = s) {

	  case VROR:
		if (!(odp->card_addr[RVMESTATUS] & BCLR))
			vme_release_bus(odp);
		break;

	  case VRWD:
		vme_release_bus(odp);
		break;

	}

	/* alloc iob only if not default settings or user already has iob */
	if (u.u_fp->f_buf
	 || vmeio->addmod != DEFADDMOD
	 || vmeio->ioflags != DEFIOFLAGS
	 || vmeio->timeout != DEFTIMEOUT) {
		iob = get_vmeio(CREATE);
		iob->addmod = vmeio->addmod;
		iob->ioflags = vmeio->ioflags;
		iob->timeout = vmeio->timeout;
	}
	return 0;

}


/*
 *****************************************************************
 * Interrupt support routines
 *****************************************************************
 */

/*
 * User wants an interrupt.
 * If there's space in the table, and the request doesn't
 * conflict with any other outstanding ones for this process,
 * then make an entry.
 */
static
vmewanti(odp, vmeint)
	register struct vme_od *odp;
	register struct vmeint *vmeint; {
	register struct inttab *ip;
	register error, x;

#ifdef VMEDEBUG
	if (debug)
		printf("WANTI sig %d statusid %d\n", 
			vmeint->signal, vmeint->statusid);
#endif
	error = 0;
	x = spl3();

	/* signal must be meaningful (NSIG is not) */
	if (vmeint->signal <= 0 || vmeint->signal >= NSIG) {
		error = EINVAL;
		goto end;
	}

	/* get a table entry */
	ip = get_inttab(odp, vmeint->statusid, CREATE, FALSE);

	/* out of table space? */
	if (ip == NULL) {
		error = EBUSY;
		goto end;
	}

	/* clash with other request? */
	if (ip->state != NEW) {
		error = EINVAL;
		goto end;
	}
	
	/* take the entry */
	ip->signal = vmeint->signal;
	ip->state = WANTI;

	/* may need to enable interrupts */
	checkienable(odp);

end:
	splx(x);
#ifdef VMEDEBUG
	if (debug && error) 
		printf("WANTI error %d\n", error);
#endif
	return error;

}

/*
 * Read the statusid returned by the interrupt being processed.
 */
static
vmereadi(odp, vmeint)
	register struct vme_od *odp;
	register struct vmeint *vmeint; {
	register struct inttab *ip;
	register error, x;

#ifdef VMEDEBUG
	if (debug)
		printf("READI on statusid %d\n", vmeint->statusid);
#endif
	error = 0;
	x = spl3();

	ip = get_inttab(odp, vmeint->statusid, LOOKUP, TRUE);
	if (ip) {
		vmeint->statusid = ip->readsid;
		ip->state = READI;
	} 
	else	error = EINVAL;

	splx(x);
#ifdef VMEDEBUG
	if (debug && error) 
		printf("READI error %d\n", error);
#endif
	return error;
}

/*
 * End or cancel the interrupt described by statusid.
 */
static
vmeendi(odp, vmeint)
	register struct vme_od *odp;
	register struct vmeint *vmeint; {
	register x, error;

#ifdef VMEDEBUG
	if (debug)
		printf("ENDI on statusid %d\n", vmeint->statusid);
#endif
	x = spl3();

	error = rel_int(odp, vmeint->statusid);
	if (!error)
		checkienable(odp);

	splx(x);
	return error;

}

/*
 * Release the interrupt request on odp described by statusid.
 * There may be more than one.
 * EINVAL if no request exists.
 */
static 
rel_int(odp, statusid)
	register struct vme_od *odp; {
	register struct inttab *ip;
	register error;

	error = EINVAL;
	for (ip = inttab; ip < &inttab[INTTAB]; ip++) {
		if (ip->procp != u.u_procp)
			continue;
		if (ip->odp != odp)
			continue;
		if (ip->statusid == ALLDEVICES
		   || statusid == ALLDEVICES
		   || statusid == ip->statusid) {
#ifdef VMEDEBUG
		if (debug)
			printf("release entry %x\n", ip);
#endif
			ip->procp = NULL;
			error = 0;
		}
	}
	return error;
}

/*
 * Re-enable interrupts on the given device (odp)
 * IF any process wants them AND no process is still
 * handling the last interrupt.
 * Otherwise, turn them off.
 */
static
checkienable(odp)
	register struct vme_od *odp; {
	register struct inttab *ip;
	register wanted, other;

	other = wanted = 0;
	for (ip = inttab; ip < &inttab[INTTAB]; ip++) {
		if (ip->odp != odp || !ip->procp)
			continue;
		if (ip->state == WANTI)
			++wanted;
		else	++other;
	}
#ifdef VMEDEBUG
	if (debug)
		printf("checkienable: %d wanted, %d other, %d result\n", 
			wanted, other,
			wanted && !other);
#endif
	odp->synflags &= ~VGHOSTI;
	if (wanted && !other)
		odp->card_addr[WINTCNTRL] = (odp->wintcntrl |= IEN);
	else	odp->card_addr[WINTCNTRL] = (odp->wintcntrl &= ~IEN);

}

/*
 * VME interrupt service routine.
 * Disable interrupts.
 * Acquire the bus.
 * Read the statusid.
 * Release the bus if it wasn't already acquired.
 * Signal all procs with requests, noting statusid in case they 
 * do a VMEREADI, and set their state to GOTI.
 * We must be very careful to leave the bus in the same state
 * that we found it.  We assume VGRANTED tells the truth, and that,
 * if not VLOCKED, we can diddle the bits and get
 * the bus.  
 * The routine normally leaves interrupts off; the user's ENDI
 * will reenable them if needed (or the ghosti() call).  One
 * exception: what if there is an interrupt, but no one to
 * service it?  We print a console message and reenable anyway;
 * maybe reading the statusid turned the device off, or maybe
 * we'll get a lot of console messages.
 */
bcdvme_isr(isrib)
	register struct interrupt *isrib; {
	register struct vme_od *odp;
	register struct inttab *ip;
	register char *card;
	register statusid;
	register sentsig;
	int ghosti();

#ifdef VMEDEBUG
	if (debug)
	printf("BANG on %x\n", ((struct vme_od *)isrib->temp)->card_addr);
#endif

	odp = (struct vme_od *) isrib->temp;
	card = odp->card_addr;

	/* disable interrupts */
	card[WINTCNTRL] = (odp->wintcntrl &= ~IEN);

	/*
	 * If bus granted, great.
	 * Otherwise, if locked, keep hands off
	 * and retry later.  If not locked, can
	 * try to get bus, retrying later on failure.
	 */
	if (!(odp->synflags & VGRANTED)) {
		if (odp->synflags & VLOCKED) {
			odp->synflags |= VGHOSTI;
			timeout(ghosti, odp, GHOSTDELAY, NULL);
			return;
		}
		/* request bus */
		card[WVMECNTRL] = (odp->wvmecntrl &= ~BBSY);
		card[WVMECNTRL] = (odp->wvmecntrl |= BR);
		/* 25 microsecond busy loop */
		snooze(25);	
		/* see if controller granted request */
		if (card[RVMESTATUS] & BGIN) {
			/* NOT granted */
			odp->synflags |= VGHOSTI;
			timeout(ghosti, odp, GHOSTDELAY, NULL);
			return;
		}
		/* IS granted */
		card[WVMECNTRL] = (odp->wvmecntrl |= BBSY);
		card[WVMECNTRL] = (odp->wvmecntrl &= ~BR);
	}
	
	/* read the statusid */
	statusid = read_status(odp); 

	/* drop the bus if we had to acquire it */
	if (!(odp->synflags & VGRANTED)) {
		/* keep separate instructions */
		card[WVMECNTRL] = (odp->wvmecntrl &= ~BR);
		card[WVMECNTRL] = (odp->wvmecntrl &= ~BBSY);
	}

	/*
	 * statusid -1 means that something's wrong;
	 * the card showed no interrupt or the device
	 * didn't put out a statusid.  We just print
	 * a warning and hope it goes away.
	 * Re-arming interrupts is probably dangerous,
	 * but what else can we do?
	 */
	if (statusid == -1) {
		badint(odp, "no statusid");
		return;
	}

	/* signal waiting processes */
	sentsig = FALSE;
	for (ip = inttab; ip < &inttab[INTTAB]; ip++)
		if (ip->procp && ip->odp == odp	&& ip->state == WANTI
		 && (ip->statusid == ALLDEVICES || ip->statusid == statusid)) {
#ifdef VMEDEBUG
			if (debug)
				printf("signal %d to proc %d\n", 
					ip->signal, ip->procp->p_pid); 
#endif
			ip->readsid = statusid;
			ip->state = GOTI;
			psignal(ip->procp, ip->signal);
			sentsig = TRUE;
		}

	/* warn if no one wanted it */
	if (!sentsig)
		badint(odp, "unrequested (statusid 0x%x)", statusid);

}

/*
 * ghosti -- retry the interrupt.
 * Always called from a timeout after the isr couldn't
 * get the bus, and wants to try again.
 * We don't call the isr directly, but just reenable
 * interrupts and let the hardware take over.  
 * VGHOSTI says that we are allowed to do this;
 * if this flag is off, we disappear.
 */ 
ghosti(odp)
	struct vme_od *odp; {
	
#ifdef VMEDEBUG
	if (debug)
		printf("ghosti\n");
#endif
	if (odp->synflags & VGHOSTI)
		odp->card_addr[WINTCNTRL] = (odp->wintcntrl |= IEN);

}

/*
 * badint -- print a message and reenable.  Only called
 * when weird interrupt occurs.  If happens repeatedly,
 * can lock up machine until bad device quieted.
 */
badint(odp, msg, arg)
	register struct vme_od *odp;
	char *msg; {

	printf("bad VME interrupt, sel code %d: ", odp->sel_code);
	printf(msg, arg);
	printf("\n");

	odp->card_addr[WINTCNTRL] = (odp->wintcntrl |= IEN);

}

/*
 * Read the statusid for the given interrupt level.
 * Returns -1 if problems (device doesn't respond
 * or card shows no interrupt).
 * card is the address of the bottom select code.
 * One problem is that the read could produce a bus error if
 * the device doesn't respond.  We need the PROBE flag and
 * the current process's u_psave to handle this. 
 * Since we are called from an interrupt routine, we
 * mustn't leave any traces behind us.
 * With the older card, the level bits read 0 for level 7
 * and 7 for no interrupt; otherwise they tell the truth.
 * With the newer card, which this driver handles, the level
 * bits are the complement of the true level; if the result
 * of the complement is 0, there is no interrupt.
 */
static
read_status(odp)
	register struct vme_od *odp; {
	register char *card;
	register wasprobe, statusid, intlevel;
	label_t savelabel;

	/* get interrupt level */
	card = odp->card_addr;
	intlevel = VMELEVEL(card[RINTSTATUS]);
#ifdef VMEDEBUG
	if (debug)
		printf("read_status sees level %d\n", intlevel);
#endif
	if (intlevel == 0)
		return -1; 

	/* set up for possible bus error */
	wasprobe = u.u_probe_addr;
	savelabel = u.u_psave;

	u.u_probe_addr = (int)vme_buserror;

	/* try to read from the bus */
	card[WINTCNTRL] = (odp->wintcntrl |= IACK);
	if (setjmp(&u.u_psave))
		statusid = -1;
	else    statusid = *((unsigned short *)(card+MOFFSET+(intlevel<<1)));
	card[WINTCNTRL] = (odp->wintcntrl &= ~IACK);

	/* undo bus error setup */
	u.u_probe_addr = wasprobe; 
	u.u_psave = savelabel;

	return statusid;

}
