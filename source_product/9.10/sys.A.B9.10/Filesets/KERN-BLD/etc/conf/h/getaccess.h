/* $Header: getaccess.h,v 1.5.83.4 93/09/17 18:26:39 kcs Exp $ */

#ifndef _SYS_GETACCESS_INCLUDED
#define _SYS_GETACCESS_INCLUDED

/*
 * Copyright (c) Hewlett-Packard Company, 1988
 *
 * EFFECTIVE FILE ACCESS PERMISSION CHECKING
 *
 * These definitions support the getaccess() system call.
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#include "../h/param.h"		/* just for NGROUPS */
#else /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#include <limits.h>		/* just for NGROUPS_MAX */
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#  ifndef _UID_T
#    define _UID_T
     typedef long uid_t;	/* Used for user IDs */
#  endif /* _UID_T */

#  ifndef _GID_T
#    define _GID_T
     typedef long gid_t;	/* Used for group IDs */
#  endif /* _GID_T */

/* Return values from getaccess() */
/* These must match the values found in <sys/file.h> and <sys/unistd.h> */
#  ifndef R_OK
#    define R_OK	4	/* Test for read permission */
#    define W_OK	2	/* Test for write permission */
#    define X_OK	1	/* Test for execute (search) permission */
#    define F_OK	0	/* Test for existence of file */
#  endif /* R_OK */


#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
 extern int getaccess(const char *, uid_t, int, const gid_t *, void *, void *);
#else /* not _PROTOTYPES */
 extern int getaccess();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */



/*
 * SPECIAL PARAMETER VALUES:
 *
 * If you give one of these values for uid, the appropriate user ID is used
 * instead.  This is a convenience to avoid having to call getuid(), geteuid(),
 * or somehow figure out the saved user ID.  These values are chosen in
 * coordination with the ones in <sys/acl.h>.
 */

#define	UID_EUID	65502
#define	UID_RUID	65503
#define	UID_SUID	65504

/*
 * If you give one of these values for ngroups, gidset[] is ignored and the
 * respective groups list is used instead.  This is a convenience to avoid
 * having to build an appropriate groups list.
 */

#define	NGROUPS_EGID	  (-1)	/* process's effective gid		*/
#define	NGROUPS_RGID	  (-2)	/* process's real gid			*/
#define	NGROUPS_SGID	  (-3)	/* process's saved gid			*/
#define	NGROUPS_SUPP	  (-4)	/* process's supplementary groups only	*/
#define	NGROUPS_EGID_SUPP (-5)	/* process's eff   gid plus supp groups	*/
#define	NGROUPS_RGID_SUPP (-6)	/* process's real  gid plus supp groups	*/
#define	NGROUPS_SGID_SUPP (-7)	/* process's saved gid plus supp groups	*/

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* not _SYS_GETACCESS_INCLUDED */
