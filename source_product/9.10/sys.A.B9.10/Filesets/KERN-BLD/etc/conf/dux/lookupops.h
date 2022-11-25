/*
 * @(#)lookupops.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 16:44:24 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */
/*
 *Lookup opcodes for DUX
 */

#define	LKUP_LOOKUP	 0	/* Lookup only */
#define LKUP_OPEN	 1
#define LKUP_CREATE	 2
#define LKUP_STAT	 3
#define LKUP_ACCESS	 4
#define LKUP_NAMESETATTR 5	/* chown, chdir, truncate, ... */
#define LKUP_REMOVE	 6	/* unlink and rmdir */
#define LKUP_MKNOD	 7	/* mknod and mkdir */
#define LKUP_LINK	 8	/* the second lookup in the link */
#define LKUP_GETMDEV	 9	/* get a device for mounting */
#define LKUP_UMOUNT	10	/* unmount a device */
#define LKUP_NORECORD	11	/* like LKUP_LOOKUP bot without saving state */
#define LKUP_EXEC	12	/* lookup for execution */
#define LKUP_STATFS	13	/* perform statfs */
#define LKUP_SETACL	14	/* setacl */
#define LKUP_GETACL	15	/* getacl */
#define LKUP_GETACCESS	16	/* getaccess */
#define LKUP_PATHCONF	17	/* pathconf */
