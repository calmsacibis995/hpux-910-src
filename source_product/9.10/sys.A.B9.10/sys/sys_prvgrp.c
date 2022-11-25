/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sys_prvgrp.c,v $
 * $Revision: 1.21.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:12:44 $
 */

/* HPUX_ID: @(#)sys_prvgrp.c	55.1		88/12/23 */

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
 * Privileged group routines.
 */


#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/errno.h"
#ifdef AUDIT
#include "../h/audit.h"
#endif AUDIT

struct privgrp_map priv_global;

/* PRIV_MAXGRPS includes the elements in privgrp_map plus */
/* one element for the global privileges in priv_global.  */
/* moved to privgrp.h in DUX*/
struct privgrp_map privgrp_map[EFFECTIVE_MAXGRPS];

/*
 * Initialize privileged group map
 */
privgrp_init()
{
    register int i;
    register struct proc * p;

    for (i = 0;i < EFFECTIVE_MAXGRPS; i++)
	privgrp_map[i].priv_groupno = PRIV_NONE;

    priv_global.priv_groupno = PRIV_GLOBAL;

    /* Privilege to use chown(2) is given to all users by default. */
    /* All other global bits remain zero.                          */
    priv_global.priv_mask[(PRIV_CHOWN-1) / (NBBY*NBPW)] =
	1L << (( PRIV_CHOWN -1)% (NBBY*NBPW));

     /*
      * Now set the re-calculate bit in the proc structure
      * of every process.
      */
	i = spl6();
	for (p = proc; p < procNPROC; p++)
		p->p_flag |= SPRIV;
	splx(i);
}

/*
 * Common entry point for getprivgrp and setprivgrp
 */
privgrp()
{
	register struct a {
		u_int	id;
	}	*uap = (struct a *)u.u_ap;
	int		getprivgrp(),
			setprivgrp();
	static int	(*calls[])() = {getprivgrp, setprivgrp};

	if(uap->id > 1) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*calls[uap->id])();
}

/*
 * The getprivgrp system call
 */
getprivgrp()
{
    register struct a {
	struct privgrp_map * grplist;
    } *uap = (struct a *)u.u_ap;
    static struct privgrp_map null_privgrp_map = {PRIV_NONE};
    register int i;

    /*
     * output global privilege mask
     */
    u.u_error = copyout(&priv_global, uap->grplist++, sizeof(priv_global));
    
    if (u.u_error)
	return;
    
    if (u.u_uid == 0)
    	u.u_error = copyout(privgrp_map, uap->grplist, sizeof(privgrp_map));
    else {
	for(i=0;i<EFFECTIVE_MAXGRPS;i++) {
	    u.u_error = copyout((char *) (groupmember(privgrp_map[i].priv_groupno) ?
					  &privgrp_map[i] : &null_privgrp_map),
				(char *)(uap->grplist++),
				sizeof(null_privgrp_map));
	    if (u.u_error)
		return;
	}
    }
}

/*
 * The setprivgrp system call
 */
setprivgrp()
{
    register int i, j, k;
    int s;
    register struct proc * p;
    register struct a {
	gid_t grpid;
	int *mask;
    } *uap = (struct a *)u.u_ap;
    int mask[PRIV_MASKSIZ];

    /*
     * Check for permission to perform this system call
     */
    if (!suser(0))
	return;

    /*
     * Get privilege mask from user area
     */
    u.u_error = copyin(uap->mask, mask, sizeof(mask));
    if (u.u_error)
	return;
#ifdef AUDIT
    /* save the integer array for the audit record */
    if (AUDITEVERON())
	save_iarray((char *)mask, sizeof(mask));
#endif AUDIT

    /*
     * Validate mask given
     */
    for (i = 0; i < PRIV_MASKSIZ; i++) {
	if ((mask[i]&PRIV_ALLPRIV(i)) != mask[i]) {
	    u.u_error = EINVAL;
	    return;
	}
    }

    switch (uap->grpid) {
    case PRIV_NONE:
	/*
	 * Revoke privilages given in mask for
	 * all privileged groups
	 */
	for (i = 0; i < PRIV_MASKSIZ; i++)
	    priv_global.priv_mask[i] &= ~mask[i];
	for (i = 0; i < EFFECTIVE_MAXGRPS; i++) {
	    for (k = j = 0; j < PRIV_MASKSIZ; j++)
		k |= (privgrp_map[i].priv_mask[j] &= ~mask[j]);
	    if (k == 0)
		privgrp_map[i].priv_groupno = PRIV_NONE; /* mark as unused */
	}
	break;

    case PRIV_GLOBAL:
	bcopy(mask, priv_global.priv_mask, sizeof(mask));
	break;

    default:
	/*
	 * See if entry already exists for grpid
	 */
	for (  i = 0
	     ;    i < EFFECTIVE_MAXGRPS
	       && privgrp_map[i].priv_groupno != uap->grpid
	     ; i++)
	    ;

	/*
	 * Either the index now points to an entry
	 * associated with this group id, the index
	 * points to a free element in the list, or
	 * we have run off the end.
	 */
	if (i >= EFFECTIVE_MAXGRPS) {
	    /*
	     * See if some element has a group id of PRIV_NONE
	     * (unused)
	     */
	    for (  i = 0
		 ;    i < EFFECTIVE_MAXGRPS
		    && privgrp_map[i].priv_groupno != PRIV_NONE
		 ; i++)
		;
	}
	if (i >= EFFECTIVE_MAXGRPS) {
	    u.u_error = E2BIG;
	    return;
	}
	privgrp_map[i].priv_groupno = uap->grpid;

	for (k = j = 0; j < PRIV_MASKSIZ; j++)
	    k |= privgrp_map[i].priv_mask[j] = mask[j];

	if (k == 0)
	    privgrp_map[i].priv_groupno = PRIV_NONE;	/* mark as unused */
    }

    /*
     * Now set the re-calculate bit in the proc structure
     * of every process.
     */
    s = spl6();
    for (p = proc; p < procNPROC; p++)
	p->p_flag |= SPRIV;
    splx(s);
}

/*
 * Used to check wether the caller is in a group having
 * the privilege asked for.
 */
in_privgrp(priv, cred)
    int priv;
    struct ucred *cred;
{
    int s;
    int *return_priv;
    int *compute_priv();
    sv_lock_t nsp;

    /*
     * Re-compute privilege mask if necessary
     */
    if (u.u_procp->p_flag&SPRIV) {
	return_priv = (int *)compute_priv(u.u_gid,
					  cred ? cred->cr_groups : u.u_groups);
	bcopy(return_priv, u.u_priv, sizeof(u.u_priv));
	s = UP_SPL6();
	SPINLOCKX(sched_lock,&nsp);
	u.u_procp->p_flag &= ~SPRIV;
	SPINUNLOCKX(sched_lock,&nsp);
	UP_SPLX(s);
    }

    /*
     * See if privilege is in users privilege mask.
     */
    return(wisset(u.u_priv,priv-1));
}

/*
 * Routine to compute the privilege mask given
 * effective group id * and list of access groups.
 * Called when privileged groups are changed, the
 * effective group id or access group list is changed.
 *
 * (This would work faster if we knew that both lists
 *  were sorted according to group number).
 */
int *compute_priv(egid, glist)
    gid_t egid;
    gid_t *glist;
{
    register int i, j, k;
    static int mask[PRIV_MASKSIZ];

    /*
     * Mask starts out with global privileges
     */
    for (i = 0; i < PRIV_MASKSIZ; i++)
	mask[i] = priv_global.priv_mask[i];

    /*
     * Or in privileges for effective group
     */
    for (i = 0; i < EFFECTIVE_MAXGRPS; i++)
	if (privgrp_map[i].priv_groupno == egid) {
	    for (j = 0; j < PRIV_MASKSIZ; j++)
		mask[j] |= privgrp_map[i].priv_mask[j];
	    break;
	}

    /*
     * Or in privileges for other groups
     */
    for (j = 0; j < NGROUPS && glist[j] != NOGROUP; j++)
	for (i = 0; i < EFFECTIVE_MAXGRPS; i++)
	    if (privgrp_map[i].priv_groupno == glist[j]) {
		for (k = 0; k < PRIV_MASKSIZ; k++)
		    mask[k] |= privgrp_map[i].priv_mask[k];
		break;
	    }

    return (mask);
}
