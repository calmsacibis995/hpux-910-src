/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/quota.c,v $
 * $Revision: 1.5.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/11 13:20:03 $
 */

/*
(c) Copyright 1983, 1984, 1985, 1986 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
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

#ifdef QUOTA
/*
 * Code pertaining to management of the in-core data structures.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../h/mount.h"
#include "../ufs/fs.h"
#include "../h/malloc.h"

#include "../dux/cct.h"
extern site_t my_site;

#ifdef GETMOUNT
extern time_t mount_table_change_time;
#endif

#define DQ_CACHE_GROW_SIZE 2

/*
 * Dquot cache - hash function.
 */
#define	DQHASH(uid, mp) \
	(((unsigned)(mp) + (unsigned)(uid)) & (NDQHASH-1))

/*
 * Dquot in core hash chain headers
 */
struct	dqhead	dqhead[NDQHASH];

/*
 * Dquot free list.
 */
struct dquot dqfreelist;

/*
 * Dquot reserved list.
 */
extern struct dquot dqresvlist;


#define dqinsheadfree(DQP) { \
	(DQP)->dq_freef = dqfreelist.dq_freef; \
	(DQP)->dq_freeb = &dqfreelist; \
	dqfreelist.dq_freef->dq_freeb = (DQP); \
	dqfreelist.dq_freef = (DQP); \
}

#define dqinstailfree(DQP) { \
	(DQP)->dq_freeb = dqfreelist.dq_freeb; \
	(DQP)->dq_freef = &dqfreelist; \
	dqfreelist.dq_freeb->dq_freef = (DQP); \
	dqfreelist.dq_freeb = (DQP); \
}

#define dqremfree(DQP) { \
	(DQP)->dq_freeb->dq_freef = (DQP)->dq_freef; \
	(DQP)->dq_freef->dq_freeb = (DQP)->dq_freeb; \
}

typedef	struct dquot *DQptr;

/* 
 * Add entries to the quota cache as needed.
 */
void
dquot_grow(num)
int num;
{
   register struct dquot *dqp;

   /*
    *	Changed MALLOC call to kmalloc to save space. When
    *	MALLOC is called with a variable size, the text is
    *	large. When size is a constant, text is smaller due to
    *	optimization by the compiler. (RPC, 11/11/93)
    */
   dqp = (struct dquot *) kmalloc(num*sizeof(struct dquot), M_DQUOTA, M_WAITOK);
   for (;num > 0; dqp++, num--)
   {
	dqp->dq_forw = dqp->dq_back = dqp;
        dqp->dq_flags = 0;
        dqp->dq_cnt = 0;
        dqp->dq_mp = NULL;
	dqinsheadfree(dqp);
   }
}


/*
 * Initialize quota caches.
 */
void
qtinit()
{
	register struct dqhead *dhp;

	/*
	 * Initialize the cache between the in-core structures
	 * and the per-file system quota files on disk.
	 */
	for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++) {
		dhp->dqh_forw = dhp->dqh_back = (DQptr)dhp;
	}
	dqfreelist.dq_freef = dqfreelist.dq_freeb = (DQptr)&dqfreelist;
	dqresvlist.dq_freef = dqresvlist.dq_freeb = (DQptr)&dqresvlist;
        dqresvlist.dq_flags = 0;
}


/*
 * Move the entries on the reserved list to the free list
 */
dq_move_to_freelist()
{
   if (dqresvlist.dq_freef != &dqresvlist)  /* list is not empty */
   {
      dqresvlist.dq_freef->dq_freeb = dqfreelist.dq_freeb;
      dqresvlist.dq_freeb->dq_freef = &dqfreelist;
      dqfreelist.dq_freeb->dq_freef = dqresvlist.dq_freef;
      dqfreelist.dq_freeb           = dqresvlist.dq_freeb;
      dqresvlist.dq_freef = dqresvlist.dq_freeb = &dqresvlist;
   }
   dqresvlist.dq_flags &= ~(DQ_RESERVED | DQ_FREE);
}

/*
 * See if mount entry pointer passed in points to a valid mount entry.
 */
mount_entry_exists(mp)
	struct mount *mp;
{
	register struct mount *existing_mp;
	register struct mounthead *mhp;
	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
	{
		for (existing_mp = mhp->m_hforw; 
		     existing_mp != (struct mount *)mhp;
		     existing_mp = existing_mp->m_hforw)
		{
			if (existing_mp == mp) {
				return(1);
			}
		}
	}
	
	/*
	 * mp does not match any existing mount entries.
	 */
	return(0);
}


/*
 * Obtain the user's on-disk quota limit for file system specified.
 */
int
getdiskquota(uid, mp, force, dqpp)
	register int uid;
	register struct mount *mp;
	int force;			/* don't do enable checks */
	struct dquot **dqpp;		/* resulting dquot ptr */
{
	register struct dquot *dqp;
	register struct dqhead *dhp;
	register struct inode *qip;
	int error;
#ifdef GETMOUNT
        int old_mount_change_time;
#endif


	dhp = &dqhead[DQHASH(uid, mp)];
loop:
#ifdef GETMOUNT
	old_mount_change_time = mount_table_change_time;
#endif
	/*
	 * Check for quotas enabled.
	 */
	if ((mp->m_qflags & MQ_ENABLED) == 0 && !force)
		return (ESRCH);
        if (((my_site_status & CCT_CLUSTERED) == 0) ||
            mp->m_site == my_site) /* local filesystem */
        {
	   qip = mp->m_qinod;
           if (qip == NULL)
		panic("getdiskquota");
	}

	/*
	 * Check the cache first.
	 */
	for (dqp = dhp->dqh_forw; dqp != (DQptr)dhp; dqp = dqp->dq_forw) {
		if (dqp->dq_uid != uid || dqp->dq_mp != mp)
			continue;
		if (dqp->dq_flags & DQ_LOCKED) {
			dqp->dq_flags |= DQ_WANT;
			(void) sleep((caddr_t)dqp, PINOD+1);
			/*
			 * Make sure that the mount entry pointer passed 
			 * in still points to a valid mount entry.
			 * If not, return error.
			 */
#ifdef GETMOUNT
			if (old_mount_change_time != mount_table_change_time) {
#endif
				if (!mount_entry_exists(mp)) {
					return(ENODEV);
				}
#ifdef GETMOUNT
                        }
#endif
			goto loop;
		}
		dqp->dq_flags |= DQ_LOCKED;
		/*
		 * Cache hit with no references.
		 * Take the structure off the free list.
		 */
		if (dqp->dq_cnt == 0)
			dqremfree(dqp);
		dqp->dq_cnt++;
		*dqpp = dqp;
		return (0);
	}
	/*
	 * Not in cache.
	 * Get dquot at head of free list.
	 */
	if ((dqp = dqfreelist.dq_freef) == &dqfreelist) { /* empty freelist */
                /* first, see if the reserved list can be free'd */
                if (dqresvlist.dq_flags & DQ_FREE)
                {
                   dq_move_to_freelist();
                   if ((dqp = dqfreelist.dq_freef) == &dqfreelist) 
                   {  /* freelist is still empty, get new entries */
                      dquot_grow(DQ_CACHE_GROW_SIZE);
                      dqp = dqfreelist.dq_freef;
	   	   }
		}
                else  /* list is still reserved, get new entries */
		{
                   dquot_grow(DQ_CACHE_GROW_SIZE);
                   dqp = dqfreelist.dq_freef;
		}
	}
	if (dqp->dq_cnt != 0 || (dqp->dq_flags & (DQ_LOCKED | DQ_WANT)) != 0)
		panic("diskquota");
        if ((dqp->dq_flags & DQ_MOD) != 0)
	{
           dqp->dq_flags |= DQ_LOCKED;
           dqupdate(dqp);
        }
	/*
	 * Make sure that the mount entry pointer passed in still points to
	 * a valid mount entry.  If not, return error.
	 */
#ifdef GETMOUNT
         if (old_mount_change_time != mount_table_change_time) {
#endif
	         if (!mount_entry_exists(mp)) {
                 	return(ENODEV);
                 }
#ifdef GETMOUNT
         }
#endif
	/*
	 * Take it off the free list, and off the hash chain it was on.
	 * Then put it on the new hash chain.
	 */
	dqremfree(dqp);
	remque(dqp);
	dqp->dq_flags = DQ_LOCKED;
	dqp->dq_cnt = 1;
	dqp->dq_uid = uid;
	dqp->dq_mp = mp;
	insque(dqp, dhp);
        if (((my_site_status & CCT_CLUSTERED) == 0) ||
            mp->m_site == my_site)
	{  /* Only get limits if at server */
   	   if (dqoff(uid) < qip->i_size) {
		/*
		 * Read quota info off disk.
		 */
		error = rdwri(UIO_READ, qip, (caddr_t)&dqp->dq_dqb,
		    sizeof (struct dqblk), dqoff(uid), UIOSEG_KERNEL, (int *)0);
		if (error) {
			/*
			 * I/O error in reading quota file.
			 * Put dquot on a private, unfindable hash list,
			 * put dquot at the head of the free list and
			 * reflect the problem to caller.
			 */
			remque(dqp);
			dqp->dq_cnt = 0;
			dqp->dq_mp = NULL;
			dqp->dq_forw = dqp;
			dqp->dq_back = dqp;
			dqinsheadfree(dqp);
			DQUNLOCK(dqp);
			return (EIO);
		}
	   } else {
		bzero((caddr_t)&dqp->dq_dqb, sizeof (struct dqblk));
	   }
	}
	*dqpp = dqp;
	return (0);
}

/*
 * Release dquot.
 */
void
dqput(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_cnt == 0 || (dqp->dq_flags & DQ_LOCKED) == 0)
		panic("dqput");
	if (--dqp->dq_cnt == 0)
		dqinstailfree(dqp);
	DQUNLOCK(dqp);
}

/*
 * Update on disk quota info.
 */
void
dqupdate(dqp)
	register struct dquot *dqp;
{
	register struct inode *qip;
        int error;
	qip = dqp->dq_mp->m_qinod;
	if (qip == NULL ||
	   (dqp->dq_flags & (DQ_LOCKED|DQ_MOD)) != (DQ_LOCKED|DQ_MOD))
		panic("dqupdate");
        /* make sure that any clients with this quotas file open will get *
         * these changes being made here. (Should not be a performance    *
         * problem since setsync returns if sync flag is already set, and *
         * clients will rarely have the quotas files open.                *
         */
        if (my_site_status & CCT_CLUSTERED) 
           setsync(qip);

        error =	rdwri(UIO_WRITE, qip, (caddr_t)&dqp->dq_dqb,
	    sizeof (struct dqblk), dqoff(dqp->dq_uid), UIOSEG_KERNEL, (int *)0);
	if (error == 0) 
        {
           /* increment the version number, so that if a client has an old *
            * copy of the quotas file in its cache, it will invalidate     *
            * it's buffers.                                                *
            */
           qip->i_fversion++;
           dqp->dq_flags &= ~DQ_MOD;
	}
#ifdef OSDEBUG
	else {
		printf("dqupdate: error %d writing inode 0x%x\n", error, inode);
	}
#endif
}

/*
 * Invalidate a dquot.
 * Take the dquot off its hash list and put it on a private,
 * unfindable hash list. Also, put it at the head of the free list.
 */
dqinval(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_cnt || (dqp->dq_flags & (DQ_MOD|DQ_WANT)) )
		panic("dqinval");
	dqp->dq_flags = 0;
	remque(dqp);
	dqremfree(dqp);
	dqp->dq_mp = NULL;
	dqp->dq_forw = dqp;
	dqp->dq_back = dqp;
	dqinsheadfree(dqp);
}
#endif QUOTA
