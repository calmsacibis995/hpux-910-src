/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_setacl.c,v $
 * $Revision: 1.8.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:22:20 $
 */

/* HPUX_ID: @(#)ufs_setacl.c	55.1	88/12/23 */

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

#ifdef ACLS
#include "../h/param.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/uio.h"
#include "../dux/lookupops.h"
#include "../dux/lkup_dep.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/vfs.h"
#ifdef QUOTA
#include "../h/mount.h"
#include "../ufs/quota.h"
#endif QUOTA
/* This tuple marks an unused entry in an acl */

struct acl_tuple acl_tuple_unused = {
	ACLUNUSED, ACLUNUSED, 0
};

/* V N O _ G E T A C L
 *
 * This routine copies the ACL into the kernel location pointed to by 
 * the parameter. This routine can be used by DUX as well as by
 * the ufs_getacl routine
 */

vno_getacl(vp, ntuples, acl)
struct vnode *vp;
int ntuples;
struct acl_tuple *acl;
{
	register struct inode *ip;
	int tuple_count = 0;
	register struct acl_tuple *iacl;
	register int i;

	/* The user cannot request more than NACLTUPLES tuples */

	if (ntuples > NACLTUPLES)
	{
		u.u_error = EINVAL;
		return(0);
	}

	ip = VTOI(vp);

	if (ip->i_contin)
	{
		if (!ip->i_contip)
			panic("vno_getacl: i_contin set, i_contip not; fsck the disc");

		if ((ip->i_contip->i_mode & IFMT) != IFCONT)
			panic("vno_getacl: continuation inode is wrong file type; fsck the disc");

		iacl = ip->i_contip->i_acl;

		/* count the number of optional tuples */

		for (i=0; i<NOPTTUPLES; i++)
		{
			if (iacl[i].uid == ACLUNUSED)
				break;
		}
		tuple_count = i;
	}

	/*
	 * If the number of tuples requested in the system call is
	 * zero then we will simply return the number of tuples
	 * in the acl (including base tuples). If the number of
	 * tuples requested is non-zero, then we must place the acl
	 * itself in the location requested
	 */

	if (ntuples != 0)
	{
		/*
		 * if the number of tuples requested is less than the
		 * number of tuples in the acl return an error
		 */

		if (ntuples < tuple_count + NBASETUPLES)
		{
			u.u_error = EINVAL;
			return(0);
		}
		init_acl(acl,NACLTUPLES);

		/* copy the optional tuples */

		if (ip->i_contip)
		{
			for (i=0; i<tuple_count; i++)
			{
				acl[i] = iacl[i];
			}
		}

		/* Put the base tuples into the access control list */

		insert_tuple(acl, ip->i_uid, ACL_NSGROUP, 
			getbasetuple(ip->i_mode,ACL_USER), NACLTUPLES);
		insert_tuple(acl, ACL_NSUSER, ip->i_gid,
			getbasetuple(ip->i_mode,ACL_GROUP), NACLTUPLES);
		insert_tuple(acl, ACL_NSUSER, ACL_NSGROUP,
			getbasetuple(ip->i_mode,ACL_OTHER), NACLTUPLES);

	}
	return tuple_count + NBASETUPLES;
}

/* D E L E T E _ A L L _ T U P L E S
 *
 * delete all the optional tuples in an ACL by zeroing out the continuation
 * inode number and freeing the continuation inode.
 */

delete_all_tuples(ip)
register struct inode *ip;
{
	register struct inode * cip;

	cip = ip->i_contip;

	if (cip == NULL)		/* no optional tuples */
		return;

	ip->i_contin = 0;
	ip->i_contip = 0;
	imark(ip,ICHG);

	/* Set nlink to zero to free the inode. We call in_inactive directly
	 * since we are totally bypassing the vnode layer. For this
	 * reason we also zero the vcount.
	 */

        /* wait for inode to be available -make sure not doing update */
	ilock(cip);
	iunlock(cip);
	cip->i_vnode.v_count = 0;
	cip->i_nlink=0;
#ifdef QUOTA
        /* If quotas are defined, decrement the quota inode count since we  */
        /* are deleting a continuation inode w/o deleting the "real" inode. */
        /* The parameters use the "real" inode, since the continuation      */
        /* does not contain all of the needed fields and the results are    */
        /* the same.                                                        */
	(void)chkiq(VFSTOM(ip->i_vnode.v_vfsp), ip, ip->i_uid, 0);
#endif QUOTA
	in_inactive(cip);
}

/* I N S E R T _ T U P L E
 *
 * Insert a tuple in sorted order into an ACL. We sort first on specificity
 * level and second on numeric value.
 */

insert_tuple(acl,uid,gid,mode,maxtuples)
register struct acl_tuple *acl;
register int uid,gid,mode,maxtuples;
{
	register int i;
	int in1;		/* index where this entry goes */

	/*
	 * Find out where this tuple goes. Numeric ordering with special
	 * cases for (u,g) and (u,*) tuples will work. The (u,*) tuples
	 * must skip the (u,g) tuples and then check numeric order and
	 * (u,g) must check and make sure they are not running into the
	 * (u,*) tuples (running into (*,g) tuples is OK)
	 */

	for (i=0; i<maxtuples; i++)
	{
		if (gid == ACL_NSGROUP)	/* a (u,*) tuple */
		{
			if ((acl[i].uid < ACL_NSUSER) && 
			    (acl[i].gid < ACL_NSGROUP))
				continue;
		}
		else if (uid < ACL_NSUSER)	/* a (u,g) tuple */
		{
			if (acl[i].gid == ACL_NSGROUP)
				break;
		}

		if (uid > acl[i].uid)
			continue;
		if (uid == acl[i].uid)
		{
			if (gid > acl[i].gid)
				continue;
			if (gid == acl[i].gid)
			{
				u.u_error = EINVAL;
				return;
			}
		}
		break;
	}

	in1 = i;

	/* 
	 * Find first open slot. If no open slot exists return
	 * errno E2Big
	 */

	for (; i<maxtuples; i++)
	{
		if (acl[i].uid == ACLUNUSED)
			break;
	}

	if (i == maxtuples)
	{
		u.u_error = E2BIG;
		return;
	}

	/*
	 * move all the acl entries into the next highest slot, starting
	 * at the highest entry. Then fill in the emptied out slot
	 */

	for (; i>in1; i--)
		acl[i] = acl[i-1];

	acl[in1].uid = uid;
	acl[in1].gid = gid;
	acl[in1].mode = mode;

}

/* I N I T _ A C L
 *
 * Fill an acl with the specified number of unused entries
 */

init_acl(acl,ntuples)
register struct acl_tuple *acl;
register int ntuples;
{
	register int i;

	for (i=0; i<ntuples; i++)
	{
		*acl++ = acl_tuple_unused;
	}
}

/* C H E C K _ A C L _ X
 *
 * check whether the execute bit is set on any of the optional tuples in
 * this acl. This code is only used by ufs_getattr. It assumes that a 
 * check has already been made for the presence of the continuation inode.
 */

check_acl_X(ip)
struct inode *ip;
{
	register struct acl_tuple *iacl;
	register int i;

	if (!ip->i_contip)
		panic("check_acl_X: i_contin set, i_contip not; fsck the disc");

	if ((ip->i_contip->i_mode & IFMT) != IFCONT)
		panic("check_acl_X: continuation inode is wrong file type; fsck the disc");

	iacl = ip->i_contip->i_acl;
	for (i=0; i<NOPTTUPLES; i++, iacl++)
	{
		if (iacl->uid == ACLUNUSED)
			return 0;
		if (iacl->mode & X_OK)
			return 1;
	}
	return 0;
}

/* C H E C K _ A C L _ T U P L  E
 *
 * if a tuple exists in the acl for this inode which matches this tuple, 
 * replace the owner (or group) and the owner (or group) mode bits with 
 * the old owner (or group) and the old owner (or group) mode bits, having 
 * saved the mode field first. Then, update the ip->i_mode field with the
 * saved mode found in the tuple.
 */

check_acl_tuple(ip,tp)
register struct inode *ip;
register struct acl_tuple *tp;
{
	register struct acl_tuple *iacl;
	register int i, savemode;
	register struct inode *cip;

	if (ip->i_contin == 0)
		return;

	cip = ip->i_contip;

	if (!cip)
		panic("check_acl_t: i_contin set, i_contip not; fsck the disc");

	if ((cip->i_mode & IFMT) != IFCONT)
		panic("check_acl_t: continuation inode is wrong file type; fsck the disc");

	iacl = ip->i_contip->i_acl;

	/* find the match */
	for (i=0; i<NOPTTUPLES; i++, iacl++)
		if (iacl->uid == tp->uid && iacl->gid == tp->gid)
			break;

	if (i == NOPTTUPLES)
		return;			/* no match */

	/* a match was found, so now:
	 * 1. save the acl mode field 
	 * 2. delete the found tuple and insert a new one, with the old 
         *    owner or group, as well the old owner or group mode bits.
	      A replace would be nice, but sorted order is assumed.
         * 3. then update the basemode tuple with the new owner or
         *    group mode bits.
         */ 
	
	savemode = iacl->mode;

	/* move subsequent tuples down one slot */
	for (; i<NOPTTUPLES-1; i++,iacl++)
		iacl[0] = iacl[1];
	*iacl = acl_tuple_unused;

	/* reset the iacl pointer */
	iacl = ip->i_contip->i_acl;

	/* this is the u.* tuple */
	if (tp->gid == ACL_NSGROUP) {
		insert_tuple(iacl, ip->i_uid, ACL_NSGROUP, 
			getbasetuple(ip->i_mode,ACL_USER), NACLTUPLES);
		ip->i_mode = setbasemode(ip->i_mode, savemode, ACL_USER);
	}
	/* this is the *.g tuple */
	else  {
		insert_tuple(iacl, ACL_NSUSER, ip->i_gid,
			getbasetuple(ip->i_mode,ACL_GROUP), NACLTUPLES);
		ip->i_mode = setbasemode(ip->i_mode, savemode, ACL_GROUP);
	}
	
	cip->i_flag |= ICHG;
}
#endif
