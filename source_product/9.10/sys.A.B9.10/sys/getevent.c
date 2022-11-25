/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/getevent.c,v $
 * $Revision: 1.7.83.4 $        $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/15 12:18:59 $
 */
/* HPUX_ID: @(#)getevent.c	55.1		88/12/23 */

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

  	getevent - get events and system calls which are currently
		   audited

	SYNOPSIS

	    #include <sys/audit.h>

	    int getevent (a_syscall, a_event)
	    struct aud_syscall  *a_syscall;
	    struct aud_event_tbl  *a_event;

	DESCRIPTION

	    Getevent gets the events and system calls which are currently
	    being audited.  The events are returned in a table pointed
	    to by a_event.  The system calls are returned in a table
	    pointed to by a_syscall.  This call is restricted to users
	    with appropriate privilege.  For C2 this is the superuser.

	RETURN VALUE

            Upon successful completion, a value of 0 is returned.
            Otherwise, a -1 is returned.
  
***********************************************************************/

#ifdef AUDIT
#include "../h/audit.h"
#include "../h/param.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../dux/cct.h"
#include "../dux/dux_hooks.h"
#include "../dux/dm.h"
extern site_t root_site, my_site;
int	event_table_want, event_table_lock;

struct aud_type aud_syscall[MAX_SYSCALL];

struct aud_event_tbl aud_event_table;

int
getevent()
{
	register struct a {
		struct aud_type		*a_syscall;
		struct aud_event_tbl	*a_event;
	} *uap = (struct a *)u.u_ap;

	if (!suser()) {		/* check for user with appropriate permission */
			u.u_error = EPERM;
			return(-1);
	}
	u.u_error = copyout((char *)aud_syscall, (char *)uap->a_syscall, 
			(sizeof(struct aud_type) * MAX_SYSCALL));
	if (u.u_error) {
		return(-1);
	}
	u.u_error = copyout(&aud_event_table, (char *)uap->a_event, 
			sizeof(struct aud_event_tbl));
	if (u.u_error) {
		return(-1);
	}
	return(0);
}

/***********************************************************************

  	setevent - set current events and system calls which are to be
		   audited

	SYNOPSIS

	    #include <sys/audit.h>

	    int setevent (a_syscall, a_event)
	    struct aud_syscall  *a_syscall;
	    struct aud_event_tbl  *a_event;

	DESCRIPTION

	    Setevent sets the events and system calls which currently
	    are to be audited.  The event and system call settings in
	    the tables pointed to by a_syscall and a_event will become
	    the current settings.  This call is restricted to users
	    with appropriate privilege.  For C2 this is the superuser.

	RETURN VALUE

            Upon successful completion, a value of 0 is returned.
            Otherwise, a -1 is returned.
  
***********************************************************************/

int
setevent()
{
	register struct a {
		struct aud_type		*a_syscall;
		struct aud_event_tbl	*a_event;
	} *uap = (struct a *)u.u_ap;

	struct aud_type 	local_aud_syscall[MAX_SYSCALL];
	struct aud_event_tbl	local_aud_event_table;

	if (!suser()) {		/* check for user with appropriate permission */
			u.u_error = EPERM;
			return(-1);
	}

	/* We need to copy at least the first table into local memory first
	 * If we don't we could fail on the second copyin and leave the 
	 * tables in a inconsistent state
	 */

	u.u_error = copyin((char *)uap->a_syscall, (char *)local_aud_syscall,
			(sizeof(struct aud_type) * MAX_SYSCALL));
	if (u.u_error) {
		return(-1);
	}
	u.u_error = copyin((char *)uap->a_event, &local_aud_event_table,
			sizeof(aud_event_table));
	if (u.u_error) {
		return(-1);
	}

	if (my_site != root_site)
		DUXCALL(SETEVENT)(local_aud_syscall,&local_aud_event_table,
				root_site);
	else
		fill_event_table(local_aud_syscall,&local_aud_event_table);
	return(0);
}

fill_event_table(ptr_aud_syscall,ptr_aud_event_table)
struct aud_type 	*ptr_aud_syscall;	/* ptr to the aud_syscall table */
struct aud_event_tbl	*ptr_aud_event_table;	/* ptr to the event table */
{

	/* lock the server's event table, then copy the event table and
	 * syscall table, and broadcast the tables to the cluster. When
	 * the broadcast has completed, unlock the event table. This will
	 * assure a consistent state throughout the cluster for the tables
	 * in event of "simultaneous" setevent calls.
	 */

	if ((my_site_status & CCT_CLUSTERED) &&
	    (my_site == root_site))
	{
		while (event_table_lock) {
			event_table_want = 1;
			sleep((caddr_t)&aud_event_table, PSWP+1);
		}
		event_table_lock = 1;
	}
	bcopy(ptr_aud_syscall,aud_syscall,sizeof (aud_syscall));
	bcopy(ptr_aud_event_table,&aud_event_table,sizeof(aud_event_table));

	if ((my_site_status & CCT_CLUSTERED) &&
	    (my_site == root_site))
	{
		/* broadcast the table */;

		DUXCALL(SETEVENT)(ptr_aud_syscall,ptr_aud_event_table,
				DM_CLUSTERCAST);

		/* unlock the table */
		event_table_lock = 0;
		if (event_table_want)
		{
			event_table_want = 0;
			wakeup(&aud_event_table);
		}
	}

	return;
}

/* aud_trans_syscall - routine to translate the syscall table to the host
 * format; from 800 to 300 or the reverse depending upon the architecture
 * of the machine we are running on.
 *
 * The table is translate in-place (the original is destroyed).
 */

void
aud_trans_syscall(scall_table)

  char scall_table[];

{

#ifdef __hp9000s800
          /* converting from 300 syscall table to 800 syscall table */
	  scall_table[237]=scall_table[186];	/* setacl  */
	  scall_table[238]=scall_table[187];	/* fsetacl */
	  scall_table[241]=scall_table[192];	/* setaudid */
	  scall_table[243]=scall_table[194];	/* setaudproc */
	  scall_table[245]=scall_table[196];	/* setevent */
	  scall_table[247]=scall_table[198];	/* audswitch */
	  scall_table[248]=scall_table[199];	/* audctl */
	  scall_table[198]=scall_table[234];	/* vfs_mount */
	  scall_table[193]=scall_table[236];	/* setdomainname */

          /* clear old 300 entries */
	  scall_table[186]=0;
	  scall_table[187]=0;
	  scall_table[192]=0;
	  scall_table[194]=0;
	  scall_table[196]=0;
	  scall_table[199]=0;
	  scall_table[234]=0;
	  scall_table[236]=0;
#endif /* hp9000s800 */


#ifdef __hp9000s300
          /* converting from 800 syscall table to 300 syscall table */
	  scall_table[234]=scall_table[198];	/* vfs_mount */
	  scall_table[186]=scall_table[237];	/* setacl */
	  scall_table[187]=scall_table[238];	/* fsetacl */
	  scall_table[192]=scall_table[241];	/* setaudid */
	  scall_table[194]=scall_table[243];	/* setaudproc */
	  scall_table[196]=scall_table[245];	/* setevent */
	  scall_table[198]=scall_table[247];    /* audswitch */
	  scall_table[199]=scall_table[248];    /* audctl */
	  scall_table[236]=scall_table[193];    /* setdomainname */

          /* clear old 800 entries */
	  scall_table[237]=0;
	  scall_table[238]=0;
	  scall_table[241]=0;
	  scall_table[243]=0;
	  scall_table[245]=0;
	  scall_table[247]=0;
	  scall_table[248]=0;
	  scall_table[193]=0;
#endif /* hp9000s200 */

}

#endif AUDIT
