/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/tty_subr.c,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:22:26 $
 */
/* HPUX_ID: @(#)tty_subr.c	55.1		88/12/23 */

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
 * This is an extremely large #ifdef.
 * The else clause begins with
 * \n#else\tnot hp9000s200.
 */


/* UNISRC_ID: @(#)clist.c	4.1	83/04/07  */

#include "../h/param.h"
#include "../h/tty.h"

/* get a character from the clist  p  */
getc(p)
register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;

	s = spl6();
	if (p->c_cc > 0) {

		/* decrement the count in the cblock */
		p->c_cc--;
		bp = p->c_cf;
		c = bp->c_data[bp->c_first++]&0377;

		/* put cblock on cfreelist if empty */
		if (bp->c_first == bp->c_last) {
			if ((p->c_cf = bp->c_next) == NULL)
				p->c_cl = NULL;
			bp->c_next = cfreelist.c_next;
			cfreelist.c_next = bp;
		}
	} else
		c = -1;
	splx(s);
	return(c);
}

/* put character  c  in the clist  p  */
putc(c, p)
register struct clist *p;
{
	register struct cblock *bp, *obp;
	register s;

	s = spl6();
	if ((bp = p->c_cl) == NULL || bp->c_last == cfreelist.c_size) {
		obp = bp;
		if ((bp = cfreelist.c_next) == NULL) {
			splx(s);
			return(-1);
		}
		cfreelist.c_next = bp->c_next;
		bp->c_next = NULL;
		bp->c_first = bp->c_last = 0;
		if (obp == NULL)
			p->c_cf = bp;
		else
			obp->c_next = bp;
		p->c_cl = bp;
	}
	bp->c_data[bp->c_last++] = c;
	p->c_cc++;
	splx(s);
	return(0);
}

/* get a cblock from the cfreelist */
struct cblock *
getcf()
{
	register struct cblock *bp;
	register int s;

	s = spl6();
	if ((bp = cfreelist.c_next) != NULL) {
		cfreelist.c_next = bp->c_next;
		bp->c_next = NULL;
		bp->c_first = 0;
		bp->c_last = cfreelist.c_size;
	}
	splx(s);
	return(bp);
}

/* put free cblock  bp  back into cfreelist */
putcf(bp)
register struct cblock *bp;
{
	register int s;

	s = spl6();
	bp->c_next = cfreelist.c_next;
	cfreelist.c_next = bp;
	splx(s);
}

/* remove last cblock from the clist  p  */
struct cblock *
getcb(p)
register struct clist *p;
{
	register struct cblock *bp;
	register int s;

	s = spl6();
	if ((bp = p->c_cf) != NULL) {
		p->c_cc -= bp->c_last - bp->c_first;
		if ((p->c_cf = bp->c_next) == NULL)
			p->c_cl = NULL;
	}
	splx(s);
	return(bp);
}

/* add cblock  bp  to clist  p  and update clist count word   c_cc  */
putcb(bp, p)
register struct cblock *bp;
register struct clist *p;
{
	register struct cblock *obp;
	register int s;

	s = spl6();
	if ((obp = p->c_cl) == NULL)
		p->c_cf = bp;
	else
		obp->c_next = bp;
	p->c_cl = bp;
	bp->c_next = NULL;
	p->c_cc += bp->c_last - bp->c_first;
	splx(s);
	return(0);
}

