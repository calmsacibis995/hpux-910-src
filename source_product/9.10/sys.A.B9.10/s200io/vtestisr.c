/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/vtestisr.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:20:24 $
 */
/* HPUX_ID: @(#)vtestisr.c	55.1		88/12/23 */


/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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

#include "../h/types.h"
#include "../h/param.h"
#include "../s200io/vme2.h"
#include "../s200io/vtest.h"
extern struct active_vme_sc *vtest_sc;

/*-----------------------------------------------------------------------*/
vtest_isr()
/*-----------------------------------------------------------------------*/
/* This routine is in a separate file because of the labels in           */
/*   the assembly language BEGIN_ISR macro.  Only one BEGIN_ISR          */
/*   can appear in a file.  (Note: BEGIN_ISR & END_ISR are now obsolete) */

{
	struct active_vme_sc *my_ptr;
	int found;
	struct vmetestregs *cp;

/***	BEGIN_ISR;  ***/
	found = 0;
	/* crit? */
	if (vtest_sc == NULL)
		panic("vtest_isr: interrupt; no cards initialized");

	my_ptr = vtest_sc;
	while (my_ptr && !found) {
		if ((my_ptr->card_ptr->assert_intr & (CLEAR_INT0 | CLEAR_INT1 | CLEAR_INT2)) 
						== (CLEAR_INT0 | CLEAR_INT1 | CLEAR_INT2) )
			my_ptr = my_ptr->next;
		else	
			found = 1;
	}

	if (!found)
		panic("vtest_isr:  nobody interrupted");	/* or ignore?? */
	cp = my_ptr->card_ptr;

	cp->assert_intr |= (CLEAR_INT0 | CLEAR_INT1 | CLEAR_INT2);
	wakeup(&(cp->assert_intr));
/***	END_ISR;  ***/
}
