/* $Header: quota.h,v 1.17.83.3 93/09/17 20:20:49 kcs Exp $ */

#ifndef _SYS_QUOTA_INCLUDED
#define _SYS_QUOTA_INCLUDED

/*
 * This file contains various macros, constants, structures, and function
 * interfaces that support disk quotas.
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

/* Types needed for this file */

#ifndef _UID_T
#  define _UID_T
   typedef long uid_t;	/* Used for user IDs */
#endif /* _UID_T */


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

    /* When the first argument to quotactl() is Q_QUOTAON, the last
       argument points to a pathname.  Otherwise, it is either ignored
       or points to a dqblk structure (see below).
    */

#ifdef _PROTOTYPES
     extern int quotactl(int, const char *, uid_t, void *);
#else /* not _PROTOTYPES */
     extern int quotactl();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/* Definitions for the cmd argument of the quotactl() system call.  */

#define Q_QUOTAON	1	/* turn quotas on */
#define Q_QUOTAOFF	2	/* turn quotas off */
#define Q_SETQUOTA	3	/* set disk limits & usage */
#define Q_GETQUOTA	4	/* get disk limits & usage */
#define Q_SETQLIM	5	/* set disk limits only */
#define Q_SYNC		6	/* update disk copy of quota usages */


/*
 * The following constants define the default amount of time given a user
 * before the soft limits are treated as hard limits (usually resulting
 * in an allocation failure). These may be  modified by the quotactl
 * system call with the Q_SETQLIM or Q_SETQUOTA commands.
 */

#define	DQ_FTIMELIMIT	(7 * 24*60*60)		/* 1 week */
#define	DQ_BTIMELIMIT	(7 * 24*60*60)		/* 1 week */

/*
 * The dqblk structure defines the format of the disk quota file
 * (as it appears on disk) - the file is an array of these structures
 * indexed by user number.  The setquota sys call establishes the inode
 * for each quota file (a pointer is retained in the mount structure).
 */

struct	dqblk {
	unsigned long	dqb_bhardlimit;	/* absolute limit on disk blks alloc */
	unsigned long	dqb_bsoftlimit;	/* preferred limit on disk blks */
	unsigned long	dqb_curblocks;	/* current block count */
	unsigned long	dqb_fhardlimit;	/* maximum # allocated files + 1 */
	unsigned long	dqb_fsoftlimit;	/* preferred file limit */
	unsigned long	dqb_curfiles;	/* current # allocated files */
	unsigned long	dqb_btimelimit;	/* time limit for excessive disk use */
	unsigned long	dqb_ftimelimit;	/* time limit for excessive files */
};

#define dqoff(UID)	((off_t)((UID) * sizeof(struct dqblk)))

/*
 * The dquot structure records disk usage for a user on a filesystem.
 * There is one allocated for each quota that exists on any filesystem
 * for the current user. A cache is kept of recently used entries.
 * Active inodes have a pointer to the dquot associated with them.
 */
struct	dquot {
	struct	dquot *dq_forw, *dq_back;/* hash list, MUST be first entry */
	struct	dquot *dq_freef, *dq_freeb; /* free list */
	short	dq_flags;
#define DQ_LOCKED	0x01		/* locked for I/O */
#define DQ_WANT		0x02		/* wanted */
#define	DQ_MOD		0x04		/* this quota modified since read */
#define	DQ_BLKS		0x10		/* has been warned about blk limit */
#define	DQ_FILES	0x20		/* has been warned about file limit */
#define	DQ_SOFT_FILES	0x40		/* user is over soft file limit */
#define	DQ_HARD_FILES 	0x80		/* user is over hard file limit */
#define	DQ_TIME_FILES	0x100		/* over soft limit and time expired */
#define	DQ_SOFT_BLKS	0x200    	/* user is over soft block limit */
#define	DQ_HARD_BLKS	0x400		/* user is over hard block limit */
#define	DQ_TIME_BLKS	0x800		/* over soft limit and time expired */
#define	DQ_RESERVED	0x1000		/* timout() called for resv list */
#define	DQ_FREE 	0x2000		/* reserved entries can be free'd */
	short	dq_cnt;			/* count of active references */
	uid_t	dq_uid;			/* user this applies to */
	struct mount *dq_mp;		/* filesystem this relates to */
	struct dqblk dq_dqb;		/* actual usage & quotas */
};

#define	dq_bhardlimit	dq_dqb.dqb_bhardlimit
#define	dq_bsoftlimit	dq_dqb.dqb_bsoftlimit
#define	dq_curblocks	dq_dqb.dqb_curblocks
#define	dq_fhardlimit	dq_dqb.dqb_fhardlimit
#define	dq_fsoftlimit	dq_dqb.dqb_fsoftlimit
#define	dq_curfiles	dq_dqb.dqb_curfiles
#define	dq_btimelimit	dq_dqb.dqb_btimelimit
#define	dq_ftimelimit	dq_dqb.dqb_ftimelimit

/*
 * flags for m_qflags in mount struct
 */
#define MQ_ENABLED	0x01		/* quotas are enabled */


#if defined(_KERNEL) && defined(QUOTA)

/* Minimum number of seconds between messages sent to user when that user */
/* has gone over quotas and is at the filesystem client */
#define DQ_MSG_TIME  60  

/* Number of seconds for timeout routine to wait to call routine to take */
/* quota cache entries off the reserved list and add to free list. */
#define DQ_RESVLIST_TIME 120 

/*
 * Dquot cache - hash chain definitions.
 */
#define	NDQHASH		64		/* smallish power of two */
struct	dqhead	{
	struct	dquot	*dqh_forw;	/* MUST be first */
	struct	dquot	*dqh_back;	/* MUST be second */
};

/*
 * Definitions for origin of quotactl system call 
 * (and also used to determine if parameters are from user or kernel space)
 */
#define DQ_LOCAL   0    /* System call originated from file-server site */
#define DQ_REMOTE  1    /* System call not from file-server site */


extern void qtinit();			/* initialize quota system */
extern struct dquot *getinoquota();	/* establish quota for an inode */
extern int chkdq();			/* check disk block usage */
extern int chkiq();			/* check inode usage */
extern void dqrele();			/* release dquot */
extern int closedq();			/* close quotas */

extern int getdiskquota();		/* get dquot for uid on filesystem */
extern void dqput();			/* release locked dquot */
extern void dqupdate();			/* update dquot on disk */

#define DQLOCK(dqp) { \
	while ((dqp)->dq_flags & DQ_LOCKED) { \
		(dqp)->dq_flags |= DQ_WANT; \
		(void) sleep((caddr_t)(dqp), PINOD+1); \
	} \
	(dqp)->dq_flags |= DQ_LOCKED; \
}

#define DQUNLOCK(dqp) { \
	(dqp)->dq_flags &= ~DQ_LOCKED; \
	if ((dqp)->dq_flags & DQ_WANT) { \
		(dqp)->dq_flags &= ~DQ_WANT; \
		wakeup((caddr_t)(dqp)); \
	} \
}

#define QFILENAME	"/quotas"

#endif /* _KERNEL && QUOTA */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_QUOTA_INCLUDED */
