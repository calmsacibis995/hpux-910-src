/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/float.c,v $
 * $Revision: 1.6.84.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/20 11:08:40 $
 */
/* HPUX_ID: @(#)float.c	55.1		88/12/23 */

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
#include "../s200/vmparam.h"
#include "../s200/pte.h"

#define NUM_FLOAT_REGS 8
#define CHARV(x)  (* (char *)  (x))		/* The char at address x */
#define SHORTV(x) (* (short *) (x))		/* The short at address x  */
#define INTV(x)   (* (int *)   (x))		/* The int at address x  */
#define FLOATV(x) (* (float *) (x))		/* The float at address x  */


/* base address of float card */
caddr_t float_base = (char *) (0x5c0000 + LOG_IO_OFFSET);
#ifdef NEVER_CALLED
struct pte *float_ptes;
#endif /* NEVER_CALLED */

static int int0 = 0;

float_init()
{
	register int i;
	struct pte proto;
	register int pfnum;
	extern int float_present;

	/*
	 * Why int0?
	 *
	 * If we just say x=0, then the c compiler will generate a clr.l x.
	 * This is fine, except that a 68000 clr reads before it writes!
	 * This can cause some things to hang if they aren't readable.
	 * Hence we assign int0 to it instead.
	 * Fortunately, our compiler doesn't realize that int0
	 * is really zero and emit it into a clr.l.
	 */

	float_present = 0;
	if (testr(float_base,1) == 1) 
		printf("HP98635A Floating Point Math Card ignored -- this card is no longer supported\n");
}

#ifdef NEVER_CALLED
float_reset()
{
	register int i;
	/*
	** Make the card exit its idle loop
	** and initialize the control register
	*/
	CHARV(float_base+1) = 1;
	INTV(float_base+0x4540) = int0;

	/* Zero the float registers  */
	for (i=0; i<NUM_FLOAT_REGS; i++)
		INTV(float_base+0x44e0+i*4) = int0;

}


float_attach(p)
struct proc *p;
{
	register unsigned int *dest_pte, *src_pte;
	unsigned int *dest_pte_saved;
	register int i;

	src_pte  = (unsigned int *) float_ptes;
#ifdef LATER
	dest_pte = dest_pte_saved = 
		(unsigned int *) vtopte(p, ((u_int) float_area) >> PGSHIFT);
#else
	panic("float attach");
#endif LATER
	for (i=0; i < FLOAT; i++)
		*dest_pte++ = *src_pte++;
	purge_tlb_user();
}
#endif /* NEVER_CALLED */
