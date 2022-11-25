/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/audswitch.c,v $
 * $Revision: 1.4.83.3 $        $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 20:03:50 $
 */
/* HPUX_ID: @(#)audswitch.c	55.1		88/12/23 */

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

/***********************************************************************

  	audswitch - suspends or resumes auditing on the current process

	SYNOPSIS

		#include <sys/audit.h>

		int audswitch (aflag)
		int aflag;

	DESCRIPTION

       	    Audswitch suspends or resumes auditing on the current
	    process.  This call is restricted to users with appropriate
            privilege.  One of the following two flags must be used:

            AUD_SUSPEND     Suspend auditing on the current process.
            AUD_RESUME      Resume auditing on the current process.

	RETURN VALUE

            Upon successful completion, a value of 0 is returned.
            Otherwise, a -1 is returned.
  
***********************************************************************/

#ifdef AUDIT
#include "../h/types.h"
#include "../h/signal.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/audit.h"

audswitch()
{

	register struct a {
		int	sus_aud;
	} *uap;


	/* if not a privileged user return EPERM */

	if (!suser())
	        return;


	/* if the input parameter is AUD_RESUME or AUD_SUSPEND */
	/* set the audit suspension flag in the u-area */
	/* otherwise return EINVAL */

	uap = (struct a *)u.u_ap;
	if ((uap->sus_aud == AUD_RESUME) || (uap->sus_aud == AUD_SUSPEND))
		u.u_audsusp = uap->sus_aud;
	else 
		u.u_error = EINVAL;
	return;
}
#endif AUDIT
