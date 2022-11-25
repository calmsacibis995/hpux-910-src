/* @(#) $Revision: 1.14.83.3 $ */
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ino.h,v $
 * $Revision: 1.14.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:20:03 $
 */
#ifndef _SYS_INO_INCLUDED /* allows multiple inclusion */
#define _SYS_INO_INCLUDED

struct dinode {
	union {
		struct	icommon di_icom;
		char	di_size[128];
	} di_un;
};

#ifdef ACLS
struct cinode {
	union {
		struct	icont	ci_icont;
		char	ci_size[128];
	} ci_un;
};
#endif /* ACLS */

#define di_ic		di_un.di_icom
#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid
#define	di_size		di_ic.ic_size.val[1]
#define	di_db		di_ic.ic_un2.ic_reg.ic_db
#define	di_ib		di_ic.ic_un2.ic_reg.ic_un.ic_ib
#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime
#define di_symlink	di_ic.ic_un2.ic_symlink
#define di_flags	di_ic.ic_flags
#define	di_rdev		di_ic.ic_un2.ic_reg.ic_db[0]
#define	di_pseudo	di_ic.ic_un2.ic_reg.ic_db[1]
#define di_rsite        di_ic.ic_un2.ic_reg.ic_db[2]
#define	di_blocks	di_ic.ic_blocks
#define di_gen		di_ic.ic_gen
#define di_fversion	di_ic.ic_fversion
#define di_frptr	di_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_frptr
#define di_fwptr	di_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fwptr
#define di_frcnt	di_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_frcnt
#define di_fwcnt	di_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fwcnt
#define di_fflag	di_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fflag
#define di_fifosize	di_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fifosize
#ifdef ACLS
#define	di_contin	di_ic.ic_contin

#define ci_ic		ci_un.ci_icont
#define	ci_mode		ci_ic.icc_mode
#define	ci_nlink	ci_ic.icc_nlink
#define ci_acl		ci_un.ci_icont.icc_acl
#endif /* ACLS */

#endif /* _SYS_INO_INCLUDED */
