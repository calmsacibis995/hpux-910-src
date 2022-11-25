/*
 * @(#)dmmsgtype.h: $Revision: 1.5.83.4 $ $Date: 93/12/09 11:47:56 $
 * $Locker:  $
 */
/* HPUX_ID: %W%		%E% */
#ifndef _SYS_DMMSGTYPE_INCLUDED /* allows multiple inclusion */
#define _SYS_DMMSGTYPE_INCLUDED

/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
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
*/

/*
 * message types
 */

#define DMSIGNAL	 1	/* signal an NSP */
#define DM_CLUSTER	 2      /* cluster */
#define DM_ADD_MEMBER	 3	/* adding new site to cluster*/
#define DM_READCONF	 4	/* read cluster conf file */
#define DM_CLEANUP	 5	/* invoke recovery process */
#define DMNDR_READ	 6      /* character device read */
#define DMNDR_WRITE      7      /* character device write */
#define DMNDR_OPEND      8      /* remote opend */
#define DMNDR_CLOSE      9      /* device close */
#define DMNDR_IOCTL     10      /* device ioctl */
#define DMNDR_SELECT    11      /* device select */
#define DMNDR_STRAT     12      /* block device strategy call */
#define	DMNDR_BIGREAD	13	/* large character device read		*/
#define	DMNDR_BIGWRITE	14	/* large character device write		*/
#define DMNDR_BIGIOFAIL	15	/* failure at using site during big I/O	*/
#define DM_LOOKUP	16	/* lookup a pathname */
#define DMNETSTRAT_READ	17	/* network logical block read */
#define DMNETSTRAT_WRITE 18	/* network logical block write */
#define DMSYNCSTRAT_READ 19	/* synchronous read */
#define DMSYNCSTRAT_WRITE 20	/* synchronous write */
#define DM_CLOSE	21	/* close file */
#define DM_GETATTR	22	/* get file attributes */
#define DM_SETATTR	23	/* set file attributes */
#define DM_IUPDAT	24	/* iupdat request */
#define DM_SYNC 	25	/* switch to synchronous mode */
#define DM_REF_UPDATE 	26	/* update an inode reference count */
#define DM_OPENPW	27	/* wait for pipe open */
#define DM_FIFO_FLUSH	28	/* flush fifo info to server */
#define DM_PIPE		29	/* open a remote pipe */
#define DM_GETMOUNT	30	/* request the mount table */
#define DM_INITIAL_MOUNT_ENTRY	31	/* mount entry during bootup */
#define DM_MOUNT_ENTRY	32	/* update mount table with mount entry */
#define DM_UFS_MOUNT	33	/* mount a UFS file system*/
#define DM_COMMIT_MOUNT	34	/* mount committed */
#define DM_ABORT_MOUNT	35	/* mount aborted */
#define DM_UMOUNT_DEV	36	/* unmount given device number */
#define DM_UMOUNT	37	/* participate in global unmount */
#define DM_SYNCDISC	38	/* synchronize disc */
#define DM_FAILURE	39	/* Declare the site as a failure */
#define DM_SERSETTIME	40	/* set your time to the reference site time */
#define	DM_SERSYNCREQ	41	/* time stamp the time packet		*/
#define	DM_RECSYNCREP	42	/* process the reply time packet	*/
#define DM_GETPIDS	43	/* Get new process IDS			*/
#define DM_RELEASEPIDS	44	/* Release process IDS			*/
#define DM_LSYNC	45	/* Local sync */
#define DM_FSYNC	46	/* Fsync a file */
#define	DM_CHUNKALLOC	47	/*allocate a chunk from the common pool */
#define	DM_CHUNKFREE	48	/*return a chunk to the common pool */
#define DM_TEXT_CHANGE	49	/*change ref cnt on text segment*/
#define DM_XUMOUNT	50	/* cluster-wide xumount */
#define	DM_XRELE	51	/* multisite xrele */
#define DM_USTAT	52	/* ustat */
#define DM_RMTCMD	53	/* Remote Command Execution */
#define	DM_ALIVE	54	/* Alive Packet for Crash Detection */
#define DM_DMMAX	55	/* return the value of dmmax */
#define	DM_SYMLINK	56	/* Symbolic Link */
#define	DM_RENAME	57	/* Rename system call*/
#define DM_FSTATFS	58	/* fstatfs */
#define DM_LOCKF	59	/* A lockf request 			*/
#define	DM_PROCLOCKF	60	/* A deadlock message for proc status	*/
#define DM_LOCKWAIT	61	/* Wait for a lock to clear		*/
#define DM_INOUPDATE	62	/* Update certain fields in an inode	*/
#define DM_UNLOCKF	63	/* An unlockf request 			*/
#define DM_NFS_UMOUNT	64	/* for nfs unmount */
#define DM_COMMIT_NFS_UMOUNT 65
#define DM_ABORT_NFS_UMOUNT  66
#define DM_MARK_FAILED	67	/* Instruct all client to declare a site dead */
#define DM_LOCK_MOUNT	68	/* Obtain clusterwide mount/umount lock	*/
#define DM_UNLOCK_MOUNT 69	/* Release clusterwide mount/umount lock */
#define DM_SETACL	70	/* setacl */
#define DM_GETACL	71	/* getacl */
#define DM_FPATHCONF	72	/* fpathconf */
#define DM_SETEVENT	73	/* setevent */
#define DM_AUDCTL	74	/* audctl */
#define DM_AUDOFF 	75	/* audctl */
#define DM_GETAUDSTUFF	76	/* get auditing stuff */
#define DM_SWAUDFILE	77	/* switch audit file */
#define DM_CL_SWAUDFILE	78	/* broadcast switch audit file */
#define DM_FSCTL	79	/* fsclt */
#define DM_QUOTACTL	80	/* quotactl */
#define DM_QUOTAONOFF	81	/* quotas on or off info for mount table */
#define DM_LOCKED	82	/* check if region of file is locked */
#endif /* _SYS_DMMSGTYPE_INCLUDED */
