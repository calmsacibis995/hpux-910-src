/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/audwrite.c,v $
 * $Revision: 1.7.83.3 $        $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 20:03:57 $
 */
/* HPUX_ID: @(#)audwrite.c	55.2		88/12/27 */

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
#include "../h/audit.h"
#include "../h/types.h"
#include "../h/ipc.h"
#include "../h/sem.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errno.h"

/***********************************************************************
 * System Call: audwrite
 *
 *            - called by self-auditing processes to write the passed
 *              audit record into the current audit file. The call is
 *              restricted to super users.
 *
 *   The call returns a 0 if the write was successful or if the write
 *   is not necessary because
 *         -- auditing system is off or
 *         -- the process is not being audited or
 *
 *   Otherwise it returns a -1 and one of the following in u_error :
 *
 *         EPERM - the call is rejected; caller is not a super user.
 *         EINVAL - bad event number.
 *
 *   or other error numbers returned by the routines audwrite calls.
 *
 *   If the event of the record is not being audited, the call
 *   is a no-op and it returns a status of 0.
 *
 ***********************************************************************/

extern struct aud_event_tbl aud_event_table;
extern int audit_state_flag;

int
audwrite()
{
   register struct a {
      struct self_audit_rec *audrec_p;
   } *uap = (struct a *) u.u_ap;
   struct self_audit_rec audrec;

   ushort pf; /* 2-bit flag indicating if we are auditing successful
            and/or failed operations */
   register struct proc *p;
   struct audit_arg args[MAX_AUD_ARGS];
   short i;

   /* check to see if the person running the program is a super user */
   if (!suser())
      return(-1);

   /* check if the audit write is necessary*/
   if ( !(audit_state_flag) || !(u.u_audproc) )
      return(0);

   /* get the audit record */
   /* first copy the header */
   u.u_error = copyin((caddr_t)&(uap->audrec_p->aud_head),
               (caddr_t)&(audrec.aud_head),
               sizeof (struct audit_hdr));
   if (u.u_error)
      return(-1);
   /* then copy the body, using the length in the header */
   if (audrec.aud_head.ah_event == EN_IPC_GETMSG ||
       audrec.aud_head.ah_event == EN_IPC_PUTMSG) {
      if ((audrec.aud_head.ah_len) < 0 || 
	  (audrec.aud_head.ah_len > sizeof(audrec.aud_body.str_data))) {
		u.u_error = EINVAL;
		return(-1);
      }
      u.u_error = copyin ((caddr_t)&(uap->audrec_p->aud_body.str_data),
                  (caddr_t)&(audrec.aud_body.str_data),
                  audrec.aud_head.ah_len);
   } else {
      if ((audrec.aud_head.ah_len) < 0 || 
	  (audrec.aud_head.ah_len > sizeof(audrec.aud_body.text))) {
		u.u_error = EINVAL;
		return(-1);
      }
      u.u_error = copyin ((caddr_t)(uap->audrec_p->aud_body.text),
                  (caddr_t)(audrec.aud_body.text),
                  audrec.aud_head.ah_len);
   }
   if (u.u_error)
      return(-1);

   /* get the 2-bit pass/fail field for the event, to see */
   /* if we are auditing successful and/or failed operations */
   switch (audrec.aud_head.ah_event & ~01777) {
      case EN_CREATE:
         pf = aud_event_table.create;
         break;
      case EN_DELETE:
         pf = aud_event_table.delete;
         break;
      case EN_MODDAC:
         pf = aud_event_table.moddac;
         break;
      case EN_MODACCESS:
         pf = aud_event_table.modaccess;
         break;
      case EN_OPEN:
         pf = aud_event_table.open;
         break;
      case EN_CLOSE:
         pf = aud_event_table.close;
         break;
      case EN_PROCESS:
         pf = aud_event_table.process;
         break;
      case EN_REMOVABLE:
         pf = aud_event_table.removable;
         break;
      case EN_LOGIN:
         pf = aud_event_table.login;
         break;
      case EN_ADMIN:
         pf = aud_event_table.admin;
         break;
      case EN_IPCCREAT:
         pf = aud_event_table.ipccreat;
         break;
      case EN_IPCOPEN:
         pf = aud_event_table.ipcopen;
         break;
      case EN_IPCCLOSE:
         pf = aud_event_table.ipcclose;
         break;
      case EN_UEVENT1:
         pf = aud_event_table.uevent1;
         break;
      case EN_UEVENT2:
         pf = aud_event_table.uevent2;
         break;
      case EN_UEVENT3:
         pf = aud_event_table.uevent3;
         break;
      default:
         u.u_error = EINVAL;
         return(-1);
   }
   if ( ((pf & 03) == NONE) ||
        (((pf & 03) == PASS) && (audrec.aud_head.ah_error != 0)) ||
        (((pf & 03) == FAIL) && (audrec.aud_head.ah_error == 0)) ) {
      /* not being audited; it is a no-op */
      return(0);
   }

   p = u.u_procp;
   /* Fill in pid and time for header */
   audrec.aud_head.ah_pid = p->p_pid;
   audrec.aud_head.ah_time = time.tv_sec; /* accurate to the second */

   /* Write the record */
   args[0].data = (caddr_t)&(audrec.aud_head);
   args[0].len = sizeof(audrec.aud_head);
   args[1].data = (caddr_t)&(audrec.aud_body);
   args[1].len = audrec.aud_head.ah_len;
   for (i=2; i < MAX_AUD_ARGS; i++) {
      args[i].data = NULL;
      args[i].len = 0;
   }
   u.u_error = 0;
   if (u.u_error = kern_aud_wr(args))
      return(-1);

   return(0);
} /* audwrite */
#endif AUDIT
