/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/crdebug.h,v $
 * $Revision: 1.3.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:24:17 $
 */
#define NFS_OPEN	0x1
#define NFS_CLOSE	0x2
#define NFS_RDWR	0x4
#define RWVP		0x8
#define NFSWRITE	0x10
#define NFSREAD		0x20
#define NFS_GETATTR	0x40
#define NFS_SETATTR	0x80
#define NFS_ACCESS	0x100
#define NFS_READLINK	0x200
#define NFS_FSYNC	0x400
#define NFS_INACTIVE	0x800
#define NFS_LOOKUP	0x1000
#define NFS_CREATE	0x2000
#define NFS_REMOVE	0x4000
#define NFS_LINK	0x8000
#define NFS_RENAME	0x10000
#define NFS_MKDIR	0x20000
#define NFS_RMDIR	0x40000
#define NFS_SYMLINK	0x80000
#define NFS_READDIR	0x100000
#define NFS_STRATEGY	0x200000
#define DO_BIO		0x400000
#define NFS_LOCKF	0x800000
#define MAKENFSNODE	0x1000000
#define DNLC_REMOVE	0x2000000
#define DNLC_PURGE	0x4000000
#define DNLC_PURGE_VP	0x8000000
#define DNLC_PURGE1	0x10000000
#define RFSCALL		0x20000000
/* #define CLGET		0x40000000*/
#define RFS_DISPATCH	0x80000000
/* NFS_LOCKCTL */
/* clget */

/*
 *  Grag the quota fields in the u-area for this purpose since
 *  they are not used.
 */
#define CR_LOGVP(vp)	u.u_quota = vp;
#define CR_UNLOGVP	u.u_quota = 0;
#define CR_ENTER(p)	u.u_qflags |= p;
#define CR_LEAVE(p)	u.u_qflags &= ~p;


