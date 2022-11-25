/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/mf.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:14:37 $
 */
/* HPUX_ID: @(#)mf.c	55.1		88/12/23 */

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

/*****************************************************************************
  Unix Minifloppy Driver    (dlb) 
 *****************************************************************************/

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/errno.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../wsio/timeout.h" 
#include "../s200io/bootrom.h" 
#include "../graf/ite_scroll.h"
#include "../s200io/mf.h"
#include "../h/uio.h"

#define NUMSECTORS 1056
#define SECTORSHIFT 8
#define SECTORSIZE 256

struct buf rmfbuf;
int opncnt[2] = {0,0};
extern char ndrives;

/****************************************************************************
  mfstrategy -	do actual I/O (synchronous, but for now..).
 ****************************************************************************/

mf_strategy(bp)
register struct buf *bp;
{
	register daddr_t nsects;
	register int ercod, mfdrive;

	if (bpcheck(bp, NUMSECTORS, SECTORSHIFT, 0))
		return;

	/* assumption: by this point all addresses and sizes are even
	   multiples of the sector size, and block size is a multiple
	   of sector size
	 */

 	mfdrive = minor(bp->b_dev); 	   /* set the drive # */

	nsects = bp->b_bcount >> SECTORSHIFT;

	spl0();  /* XXX */
	rloop:
	if (bp->b_flags&B_READ)
		ercod = flpymread(bp->b_un.b_addr, bp->b_un2.b_sectno,
			nsects,mfdrive);
	else 
		ercod = flpymwrite(bp->b_un.b_addr, bp->b_un2.b_sectno,
			nsects,mfdrive);
	if (ercod == 8080) {
	   	printf("POSSIBLE DISASTER!! Minifloppy media changed on drive %d\n",mfdrive);
	   	goto rloop;
	}
	if (ercod != 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
	 	printf("Minifloppy error #%d, drive = %d, sector = %d\n" 
			,ercod,mfdrive,bp->b_un2.b_sectno); 
	}
 	bp->b_resid -= bp->b_bcount;
	iodone(bp);
}

mf_open(dev)
dev_t dev;
{
/*  This I/O request will not work if the "flpymread" routine is
    interruptable -- so be carefull.  */

	char dumbuf[256];
	register int mdev = dev & 1;

	if (minor(dev) > ndrives) {
		return(ENXIO);
	}
	if (opncnt[mdev] == -1)  /* check if initialize in progress ? */
		return(EBUSY);

	if (opncnt[mdev]++ == 0) { /* clear the media change bit */
		flpymread(dumbuf, 0, 1, mdev);
		if (mdev == 0)
			ite_scroller(RIGHTDRIVE, "  fd.0 *");
		else
			ite_scroller(LEFTDRIVE, "  fd.1 *");
	}
	return(0);
}

mf_close(dev)
dev_t dev;
{
	register int mdev = dev & 1;

	if (minor(dev) > ndrives)
		return;
	opncnt[mdev] = 0;
	if (mdev == 0)
		ite_scroller(RIGHTDRIVE, "  fd.0  ");
	else
		ite_scroller(LEFTDRIVE, "  fd.1  ");
}

/****************************************************************************
  mfread -	Read chars from disk.
 ****************************************************************************/

mf_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	return(physio(mf_strategy, &rmfbuf, dev, B_READ, minphys, uio));
}

/****************************************************************************
  mfwrite -	Write chars to disk
 ****************************************************************************/

mf_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	return(physio(mf_strategy, &rmfbuf, dev, B_WRITE, minphys, uio));
}

/****************************************************************************
  mfioctl -	Initialize the floppy disc
 ****************************************************************************/

mf_ioctl(dev, cmd, addr, flag)
dev_t dev;
{
	register struct MF_init_track *mfi = (struct MF_init_track *)addr;
 	int mfdrive = minor(dev); 	   /* set the drive # */
	register opencnt;

	/* must be exculsively opened for initialization */
	opencnt = opncnt[mfdrive];
	if ((opencnt != -1) && (opencnt != 1)) 
		return(EBUSY);

	opncnt[mfdrive] = -1;

	switch (cmd) {

	case MFINITS: 
		if (sysflags & BIT_MAP_PRESENT)
			mfi->crt_ptr = (char *)0;
		mfi->error =
		    flpyminit(mfi->phy_track_num, mfi->log_sectbl, mfi->crt_ptr, mfdrive);

/* the kernel will not return the control buffer if a non-zero return */
		return(0);
	default:
		return(EINVAL);
	}
}
 
/*  entry for asmb routine to check if signal sent */
int
mf_issig()
{
	struct proc *p = u.u_procp; /* XXX */

	if (ISSIG(p))
		return(1);
	return(0);
}

/****************************************************************************
  mfinit -	One time init.
 ****************************************************************************/
#define IFLPY 0x445000

extern int fintrupt();

int (*mf_saved_dev_init)();

mf_init()
{
	if (ndrives >= 0) {
		ite_scroller(RIGHTDRIVE, "  fd.0  ");
		isrlink(fintrupt, 2, IFLPY, 0x08, 0x08, 0, 0);
		f_pwr_on();
	}
	if (ndrives > 0)
		ite_scroller(LEFTDRIVE, "  fd.1  ");

	/* call the next init procedure in the chain */
	(*mf_saved_dev_init)();
}

/*
**  code for converting a dev_t to a boot ROM msus
*/

struct msus (*mf_saved_msus_for_boot)();

struct msus
mf_msus_for_boot(blocked, dev)
char blocked;
dev_t dev;
{
	extern struct bdevsw bdevsw[];
	extern struct cdevsw cdevsw[];
	extern char ndrives;
	struct msus my_msus;
	register int maj = major(dev);

	/* check if open routine for this dev is me ? (mf driver) */
	if ((blocked && (maj >= nblkdev || bdevsw[maj].d_open != mf_open)) ||
	    (!blocked && (maj >= nchrdev || cdevsw[maj].d_open != mf_open)))
		return ((*mf_saved_msus_for_boot)(blocked, dev));

	/* force illegal msus first */
	my_msus.dir_format   /* :3 */ = 0;
	my_msus.device_type  /* :5 */ = 31;	/* illegal */
	my_msus.vol 	     /* :4 */ = 0;
	my_msus.unit	     /* :4 */ = 0;
	my_msus.sc	     /* :8 */ = 0;
	my_msus.ba	     /* :8 */ = 0;

	/* check if any floppy drives ? */
	if (ndrives < 0)
		return (my_msus);

	switch (minor(dev)) {
	case 0:
		break;
	case 1:
		if (ndrives > 0)	
			break;
	default:
		return (my_msus);
	}
	my_msus.device_type  /* :5 */ = 0;	/* legal */
	my_msus.unit	     /* :4 */ = minor(dev);
	my_msus.sc	     /* :8 */ = 0xff; 	/* special for mini-floppy */
	my_msus.ba	     /* :8 */ = 0xff; 	/* special for mini-floppy */
	return (my_msus);
}

/*
**  one-time linking code
*/
mf_link()
{
	extern int (*dev_init)();
	extern struct msus (*msus_for_boot)();

	mf_saved_dev_init = dev_init;
	dev_init = mf_init;

	mf_saved_msus_for_boot = msus_for_boot;
	msus_for_boot = mf_msus_for_boot;
}
