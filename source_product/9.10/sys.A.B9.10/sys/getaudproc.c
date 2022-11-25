/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/getaudproc.c,v $
 * $Revision: 1.5.83.3 $        $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 20:04:32 $
 */

/* HPUX_ID: @(#)getaudproc.c	55.1		88/12/23 */

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

#ifdef AUDIT

/******************************************************************
*
*	getaudproc - get the audit process flag
*
*	RETURN VALUE
*
*	    Upon successful completion, the audit process flag is returned.
*	    Otherwise, a -1 returned.
*
******************************************************************/

#include "../h/param.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/audit.h"

int
getaudproc()
{
	if(!suser()) {
		u.u_error = EPERM;
		return(-1);
	}

	u.u_r.r_val1 = u.u_audproc;
  	return(0);
}


/***********************************************************************
*
*  	setaudproc - set or clear the audit process flag
*
*	RETURN VALUE
*
*           Upon successful completion, a value of 0 is returned.
*           Otherwise, a -1 is returned.
*  
***********************************************************************/

int
setaudproc()
{
	int audproc;
	register struct a {
		int	audproc;
	} *uap;

	if (!suser()) {
			u.u_error = EPERM;
			return(-1);
		}

	uap = (struct a *)u.u_ap;
        audproc = uap->audproc;
	if ( audproc != 1 && audproc != 0) {
		u.u_error = EINVAL;
		return(-1);
	}

/*	set the audit process flag into the u area    */

	u.u_audproc = audproc;
	return(0);
}
#endif AUDIT
