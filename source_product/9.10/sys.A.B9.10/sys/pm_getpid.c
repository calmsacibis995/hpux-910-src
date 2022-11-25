/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/pm_getpid.c,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:09:17 $
 */

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
#include "../h/user.h"
#include "../h/proc.h"

extern pid_t mpid;

extern int dux_getnewpid();
extern int dskless_initialized;

nodux_getnewpid()
{
	register short phx1,phx2;
	register sid_t phx3;

	/* search for a unique mpid */
	for(mpid++;;mpid++){
		if (mpid >= MAXPID){
			mpid = 100;
			continue;
		}
#ifndef BSDJOBCTL
		phx1 = pidhash[PIDHASH(mpid)];  
		while (phx1 != 0){
			if (proc[phx1].p_pid == mpid)     
				break;
			phx1 = proc[phx1].p_idhash;
		}
		/*
		 * If phx index is zero then we did not 
		 * find the mpid in the list. Therefore it is
		 * unique.
		 */
		if (phx1 == 0)
			break;
#else
		phx1 = pidhash[PIDHASH(mpid)];  
		phx2 = pgrphash[PGRPHASH(mpid)];  
		phx3 = sidhash[SIDHASH(mpid)];  
		while ((phx1 != 0) || (phx2 != 0) || (phx3 != 0)){
			if (proc[phx1].p_pid == mpid)     
				break;
			if (proc[phx2].p_pgrp == mpid)
				break;
			if (proc[phx3].p_sid == mpid)
				break;

			phx1 = proc[phx1].p_idhash;
			phx2 = proc[phx2].p_pgrphx;
			phx3 = proc[phx3].p_sidhx;
		}
		/*
		 * If all the phx indexes are zero then we did not 
		 * find the mpid in either list. Therefore it is
		 * unique.
		 */
		if ((phx1 == 0) && (phx2 == 0) && (phx3 == 0))
			break;
#endif
	}
	return(mpid);
}

int
getnewpid()
{
	if (dskless_initialized)
		return(dux_getnewpid());
	else
		return(nodux_getnewpid());
}
