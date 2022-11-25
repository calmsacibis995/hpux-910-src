/* $Header: types.h,v 1.36.83.5 93/12/08 18:24:32 marshall Exp $ */

#ifndef _SYS_TYPES_INCLUDED
#define _SYS_TYPES_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifndef _DEV_T
#    define _DEV_T
     typedef long dev_t;		/* For device numbers */
#  endif /* _DEV_T */

#  ifndef _INO_T
#    define _INO_T
     typedef unsigned long ino_t;	/* For file serial numbers */
#  endif /* _INO_T */

#  ifndef _MODE_T
#    define _MODE_T
     typedef unsigned short mode_t; 	/* For file types and modes */
#  endif /* _MODE_T */

#  ifndef _NLINK_T
#    define _NLINK_T
     typedef short nlink_t;		/* For link counts */
#  endif /* _NLINK_T */

#  ifndef _OFF_T
#    define _OFF_T
     typedef long off_t;		/* For file offsets and sizes */
#  endif /* _OFF_T */

#  ifndef _PID_T
#    define _PID_T
     typedef long pid_t;		/* For process and session IDs */
#  endif /* _PID_T */

#  ifndef _GID_T
#    define _GID_T
     typedef long gid_t;		/* For group IDs */
#  endif /* _GID_T */

#  ifndef _UID_T
#    define _UID_T
     typedef long uid_t;		/* For user IDs */
#  endif /* _UID_T */

#  ifndef _TIME_T
#    define _TIME_T
     typedef long time_t;		/* For times in seconds */
#  endif /* _TIME_T */

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;	/* Type returned by sizeof() */
#  endif /* _SIZE_T */

#  ifndef _SSIZE_T
#     define _SSIZE_T
      typedef int ssize_t;		/* Signed version of size_t */
#  endif /* _SSIZE_T */

#  ifndef _SITE_T
#    define _SITE_T
     typedef unsigned short __site_t;	/* see stat.h */
#  endif /* _SITE_T */

#  ifndef _CNODE_T
#    define _CNODE_T
     typedef unsigned short __cnode_t;	/* see stat.h */
#  endif /* _CNODE_T */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_XOPEN_SOURCE
#  ifndef _CLOCK_T
#    define _CLOCK_T
     typedef unsigned long clock_t;	/* For clock ticks */
#  endif /* _CLOCK_T */

#  ifndef _KEY_T
#    define _KEY_T
      typedef long key_t;		/* For interprocess communication ID */
#  endif /* _KEY_T */

   typedef unsigned short __ushort;	/* Try to avoid using this */

   typedef long	__daddr_t;		/* For disk block addresses */
   typedef char *__caddr_t;		/* For character addresses */
   typedef long __swblk_t;

#endif /* _INCLUDE_XOPEN_SOURCE */


#ifdef _INCLUDE_AES_SOURCE
#  ifndef _CADDR_T
#    define _CADDR_T
     typedef __caddr_t		caddr_t;   /* also in ptrace.h */
#  endif /* _CADDR_T */
#endif /* _INCLUDE_AES_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
   typedef unsigned char	u_char;	   /* Try to avoid using these */
   typedef unsigned short	u_short;   /* Try to avoid using these */
   typedef unsigned int		u_int;     /* Try to avoid using these */
   typedef unsigned long	u_long;    /* Try to avoid using these */
   typedef unsigned int		uint;	   /* Try to avoid using these */
   typedef unsigned short	ushort;	   /* Try to avoid using these */
   typedef unsigned char  ubit8;
   typedef unsigned short ubit16;
   typedef unsigned long  ubit32;
   typedef char           sbit8;
   typedef short          sbit16;
   typedef long           sbit32;

   typedef __swblk_t		swblk_t;
   typedef __daddr_t		daddr_t;
   typedef __site_t		site_t;
   typedef __cnode_t		cnode_t;

   typedef long			paddr_t;
   typedef short		cnt_t;
   typedef unsigned int		space_t;
   typedef unsigned int    	prot_t;
   typedef unsigned long        cdno_t;
   typedef unsigned short	use_t;

   typedef struct _physadr { int r[1]; } *physadr;
   typedef struct _quad { long val[2]; } quad;

   typedef char spu_t;

/*
 * Only include sysmacros.h when using kernel data structures.  If this
 * file is included for normal user applications, namespace pollution 
 * will occur.
 */
#ifdef _KERNEL
#  ifdef __hp9000s300
#    ifdef _KERNEL_BUILD
#    include "../h/sysmacros.h"
#    else /* ! _KERNEL_BUILD */
#    include <sys/sysmacros.h>
#    endif /* _KERNEL_BUILD */
#  endif /* __hp9000s300 */
#endif /* _KERNEL */

#ifdef __hp9000s300
#    define NREGS_S 15 /* number of regs saved in save (sfc,dfc,a/d2-7,pc) */
#endif /* __hp9000s300 */

#  define MAXSUSE	65535	/* maximum share count on swap device */

#  ifdef __hp9000s300
     typedef struct label_t {
	   int	val[NREGS_S]; 
     } label_t;
#  endif /* __hp9000s300 */

#  ifdef __hp9000s800
     typedef short cpu_t;
     typedef struct label_t {
	int	lbl_rp;
       	int	lbl_sp;
       	int	lbl_s[17];
       	int	lbl_ss[1];
	double	lbl_sf[4];
     } label_t;
#  endif /* __hp9000s800 */

#  ifndef _KERNEL
   typedef char *dm_message;
#  endif /* not _KERNEL */

#  ifndef _AID_T
#     define _AID_T
      typedef long	aid_t;
#  endif /* _AID_T */


/* These probably should be moved to some other header */

#  define UID_NO_CHANGE  ((uid_t) -1)  /* for chown(2) and setresuid(2) */
#  define GID_NO_CHANGE  ((gid_t) -1)  /* for chown(2) and setresgid(2) */

   typedef pid_t		sid_t;	   /* For session IDs */

#  define PGID_NOT_SET   ((pid_t) -1)  /* for no pgrp */
#  define SID_NOT_SET	 ((sid_t) -1)  /* for no session */


/* Types, macros, etc. for select() */

#  ifndef MAXFUPLIM
/*
 * MAXFUPLIM is the absolute limit of open files per process.  No process, 
 * even super-user processes may increase u.u_maxof beyond MAXFUPLIM.
 * MAXFUPLIM means maximum files upper limit.
 * Important Note:  This definition should actually go into h/param.h, but 
 * since it is needed by the select() macros which follow, it had to go here.
 * I did not put it in both files since h/param.h includes this file and that
 * would be error prone anyway.  
 */
#    define MAXFUPLIM       2048

/*
 * These macros are used for select().  select() uses bit masks of file
 * descriptors in longs.  These macros manipulate such bit fields (the
 * file sysrem macros uses chars).  FD_SETSIZE may be defined by the user,
 * but must be >= u.u_highestfd + 1.  Since we know the absolute limit on 
 * number of per process open files is 2048, we need to define FD_SETSIZE 
 * to be large enough to accomodate this many file descriptors.  Unless the 
 * user has this many files opened, he should redefine FD_SETSIZE to a 
 * smaller number.
 */
#    ifndef _KERNEL
#      ifndef FD_SETSIZE
#        define FD_SETSIZE MAXFUPLIM
#      endif
#    else /* not _KERNEL */
#      define FD_SETSIZE MAXFUPLIM
#    endif /* _KERNEL */

     typedef long fd_mask;

#    define NFDBITS (sizeof(fd_mask) * 8)     /* 8 bits per byte */

#    ifndef howmany
#      define howmany(x,y)  (((x)+((y)-1))/(y))
#    endif

     typedef struct fd_set {
       fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];
     } fd_set;

#    define FD_SET(n,p)  ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#    define FD_CLR(n,p) ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#    define FD_ISSET(n,p) ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))

#    ifndef _KERNEL
#        define FD_ZERO(p)     memset((char *)(p), (char) 0, sizeof(*(p)))
#    endif /* not _KERNEL */
#   endif /* not MAXFUPLIM */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef _UNSUPPORTED

	/* 
	 * NOTE: The following header file contains information specific
	 * to the internals of the HP-UX implementation. The contents of 
	 * this header file are subject to change without notice. Such
	 * changes may affect source code, object code, or binary
	 * compatibility between releases of HP-UX. Code which uses 
	 * the symbols contained within this header file is inherently
	 * non-portable (even between HP-UX implementations).
	*/
#ifdef _KERNEL_BUILD
#	include "../h/_types.h"
#else  /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_types.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_TYPES_INCLUDED */
