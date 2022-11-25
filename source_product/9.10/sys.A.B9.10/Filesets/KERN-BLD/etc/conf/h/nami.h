/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/h/RCS/nami.h,v $
 * $Revision: 1.4.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 11:48:55 $
 */
/* HPUX_ID: @(#)nami.h	52.2     88/04/28  */
#ifndef _SYS_NAMI_INCLUDED 	/* Allows multiple inclusion */
#define _SYS_NAMI_INCLUDED

struct namidata {
	int	ni_offset;
	int	ni_count;
	struct	inode *ni_pdir;
	struct	direct ni_dent;
};

enum nami_op { NAMI_LOOKUP, NAMI_CREATE, NAMI_DELETE };

/* this is temporary until the namei interface changes */
#define	LOOKUP		0	/* perform name lookup only */
#define	CREATE		1	/* setup for file creation */
#define	DELETE		2	/* setup for file deletion */
#define	LOCKPARENT	0x10	/* see the top of namei */
#define SKIPINDIRECT	0x20	/* for dstat syscall */
#endif /* _SYS_NAMI_INCLUDED */
