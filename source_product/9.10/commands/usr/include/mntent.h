/* @(#)$Revision: 70.1.1.2 $ */
/*
 * File system table, see checklist(4)
 *
 * Used by dump, mount, umount, swapon, fsck, df, ...
 *
 */
#ifndef _MNTENT_INCLUDED /* allow multiple inclusions */
#define _MNTENT_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */
#ifdef LOCAL_DISK
#ifndef _SYS_TYPES_INCLUDED
#include <sys/types.h>
#endif /* _SYS_TYPES_INCLUDED */
#endif /* LOCAL_DISK */

#ifdef _INCLUDE_HPUX_SOURCE


#define	MNT_CHECKLIST	"/etc/checklist"
#define	MNT_MNTTAB	"/etc/mnttab"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	MNTMAXSTR	128

#define	MNTTYPE_HFS	"hfs"		/* HP HFS file system. */
#ifdef CDROM
#define	MNTTYPE_CDFS	"cdfs"		/* CD-ROM file system. */
#endif /* CDROM */
#define	MNTTYPE_NFS	"nfs"		/* network file system */
#define	MNTTYPE_SWAP	"swap"		/* Swap device */
#ifdef SWFS
#define	MNTTYPE_SWAPFS	"swapfs"	/* File system swap */
#endif /* SWFS */
#define	MNTTYPE_IGNORE	"ignore"	/* Ignore this entry */

/*
 * WARNING:  Any options added here will not show up in /etc/mnttab
 * unless they are also added to the update_mnttab(3x) libarary.
 */
#define	MNTOPT_DEFAULTS	"defaults"	/* use all default opts */
#define	MNTOPT_RO	"ro"		/* read only */
#define	MNTOPT_RW	"rw"		/* read/write */
#define	MNTOPT_SUID	"suid"		/* set uid allowed */
#define	MNTOPT_NOSUID	"nosuid"	/* no set uid allowed */
#define MNTOPT_NOAUTO	"noauto"	/* Don't automatically mount with -a. */

/*
 * NFS specific options
 *    WARNING:  Any options added here will not show up in /etc/mnttab
 *    unless they are also added to the update_mnttab(3x) libarary.
 */
#define	MNTOPT_BG	"bg"	  /* Retry mount in background */
#define	MNTOPT_FG	"fg"	  /* Retry mount in foreground */
#define MNTOPT_RETRY	"retry"	  /* Number of retries allowed. */
#define MNTOPT_RSIZE	"rsize"	  /* Read buffer size in bytes. */
#define MNTOPT_WSIZE	"wsize"	  /* Write buffer size in bytes. */
#define MNTOPT_TIMEO	"timeo"	  /* Timeout in 1/10's of seconds. */
#define MNTOPT_RETRANS	"retrans" /* Number of retransmissions. */
#define MNTOPT_PORT	"port"	  /* IP on server for requests. */
#define	MNTOPT_SOFT	"soft"	  /* soft mount */
#define	MNTOPT_HARD	"hard"	  /* hard mount */
#define MNTOPT_INTR	"intr"	  /* Allow interrupts to hard mounts. */
#define MNTOPT_NOINTR	"nointr"  /* Don't allow interrupts to hard mounts. */
#ifdef NFS3_2
#define MNTOPT_DEVS	"devs"	  /* Allow device file access */
#define MNTOPT_NODEVS	"nodevs"  /* Don't allow device file access */
#define MNTOPT_NOAC	"noac"    /* suppress fress attributes on file open */
#define MNTOPT_NOCTO	"nocto"   /* suppress attribute & name(lookup) caching */
#define MNTOPT_ACTIMEO  "actimeo"  /* attr cache timeout (sec) */
#define MNTOPT_ACREGMIN "acregmin" /* min ac timeout for reg files (sec) */
#define MNTOPT_ACREGMAX "acregmax" /* max ac timeout for reg files (sec) */
#define MNTOPT_ACDIRMIN "acdirmin" /* min ac timeout for dirs (sec) */
#define MNTOPT_ACDIRMAX "acdirmax" /* max ac timeout for dirs (sec) */

#endif /* NFS3_2 */

/*
 * Device swap options
 */
#define	MNTOPT_END      "end"     /* swap after end of file system- S300 only */

/*
 * File system swap options
 */
#ifdef SWFS
#define MNTOPT_MIN	"min"	  /* minimum file system swap */
#define MNTOPT_LIM	"lim"	  /* maximum file system swap */
#define MNTOPT_RES	"res"	  /* reserve space for file system */
#define MNTOPT_PRI	"pri"	  /* file system swap priority */
#endif /* SWFS */

#ifdef QUOTA
#define MNTOPT_QUOTA	"quota"		/* Allow disc quotas */
#define MNTOPT_NOQUOTA	"noquota"	/* Don't allow disc quotas */
#endif /* QUOTA */

#define MNTOPT_CDCASE   "cdcase"

#ifdef GETMOUNT
/*
 * Default mode for /etc/mnttab, and default sizes for NFS mount options
 */
#define MNTTAB_MODE	S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH
#define DEFAULT_WSIZE	NFS_MAXDATA	    /* NFS default defines */
#define DEFAULT_RSIZE	NFS_MAXDATA
#define DEFAULT_PORT	NFS_PORT
#define DEFAULT_TIMEO	7
#define DEFAULT_RETRANS	4
#define DEFAULT_RETRY	1
#define DEFAULT_ACREGMIN 3
#define DEFAULT_ACREGMAX 60
#define DEFAULT_ACDIRMIN 30
#define DEFAULT_ACDIRMAX 60
#endif /* GETMOUNT */

struct	mntent{
	char	*mnt_fsname;	/* name of mounted file system */
	char	*mnt_dir;	/* file system path prefix */
	char	*mnt_type;	/* MNTTYPE_* */
	char	*mnt_opts;	/* MNTOPT* */
	int	mnt_freq;	/* dump frequency, in days */
	int	mnt_passno;	/* pass number on parallel fsck */
	long	mnt_time;	/* Time this filesystem was mounted */
#ifdef LOCAL_DISK
	cnode_t mnt_cnode;	/* Cnode to which this disk is attached */
#endif /* LOCAL_DISK */
};

#if defined(__STDC__) || defined(__cplusplus)
   extern int addmntent(FILE *, struct mntent *);
   extern struct mntent *getmntent(FILE *);
   extern char *hasmntopt(struct mntent *, const char *);
   extern FILE *setmntent(const char *, const char *);
   extern int endmntent(FILE *);
#else /* not __STDC__  defined(__STDC__) || defined(__cplusplus)*/
   extern int addmntent();
   extern struct mntent *getmntent();
   extern char *hasmntopt();
   extern FILE *setmntent();
   extern int endmntent();
#endif /* else not __STDC__ || __cplusplus */

/* for compatibility: */
#define MNTTAB		MNT_CHECKLIST
#define MOUNTED		MNT_MNTTAB
#define MNTTYPE_43	MNTTYPE_HFS

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _MNTENT_INCLUDED */
