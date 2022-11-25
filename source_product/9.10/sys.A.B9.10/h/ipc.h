/* $Header: ipc.h,v 1.19.83.5 93/10/19 16:11:54 drew Exp $ */

#ifndef _SYS_IPC_INCLUDED
#define _SYS_IPC_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_XOPEN_SOURCE

  /* Needed types and structures */

#  ifndef _KEY_T
#    define _KEY_T
      typedef long key_t;	/* for ftok() function */
#  endif /* _KEY_T */

#  ifndef _UID_T
#    define _UID_T
     typedef long uid_t;	/* Used for user IDs */
#  endif /* _UID_T */

#  ifndef _GID_T
#    define _GID_T
     typedef long gid_t;	/* Used for group IDs */
#  endif /* _GID_T */

#  ifndef _MODE_T
#    define _MODE_T
     typedef unsigned short mode_t;     /* For file types and modes */
#  endif /* _MODE_T */

   /* Common IPC Access Structure */
   struct ipc_perm {
#    ifdef _CLASSIC_ID_TYPES
	unsigned short  filler_uid;
	unsigned short  uid;    /* owner's user id */
	unsigned short  filler_gid;
	unsigned short  gid;    /* owner's group id */
	unsigned short  filler_cuid;
	unsigned short  cuid;   /* creator's user id */
	unsigned short  filler_cgid;
	unsigned short  cgid;   /* creator's group id */
#    else
        uid_t        	uid;    /* owner's user id */
        gid_t	        gid;    /* owner's group id */
        uid_t	        cuid;   /* creator's user id */
        gid_t	        cgid;   /* creator's group id */
#    endif
	mode_t		mode;	/* access modes */
	unsigned short	seq;	/* slot usage sequence number */
	key_t		key;	/* key */
#  ifdef __hp9000s800
#    ifdef _KERNEL
	unsigned short	ndx;	/* ndx of proc who has lock */
	unsigned short	wait;	/* waits, wanted, lock bits; reserved for 
				   specific ipc facilities */
#    else /* not _KERNEL */
	unsigned short	__ndx;	/* ndx of proc who has lock */
	unsigned short	__wait;	/* waits, wanted, lock bits; reserved for 
				   specific ipc facilities */
#    endif /* not _KERNEL */
#  endif /* __hp9000s800 */
	char		pad[20]; /* room for future expansion */
   };

#  define  IPC_CREAT	0001000		/* create entry if key doesn't exist */
#  define  IPC_EXCL	0002000		/* fail if key exists */
#  define  IPC_NOWAIT  	0004000		/* error if request must wait */

   /* Keys. */
#  define  IPC_PRIVATE	(key_t)0	/* private key */

   /* Control Commands. */
#  define	IPC_RMID	0	/* remove identifier */
#  define	IPC_SET		1	/* set options */
#  define	IPC_STAT	2	/* get options */
#endif /* _INCLUDE_XOPEN_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE

/* Function prototype for ftok() */

#ifndef _KERNEL
#  ifdef __cplusplus
      extern "C" {
#  endif /* __cplusplus */

#    ifdef _PROTOTYPES
       extern key_t ftok(const char *, char);
#    else /* not _PROTOTYPES */
       extern key_t ftok();
#    endif /* not _PROTOTYPES */

#  ifdef __cplusplus
      }
#  endif /* __cplusplus */
#endif /* not _KERNEL */

   /* Common IPC Definitions. */
   /* Mode bits. */
#  define	IPC_ALLOC	0100000		/* entry currently allocated */
#    define	IPC_LOCKED	0040000		/* structure is locked */
#    define	IPC_WANTED	0004000		/* process waiting for lock */

   /*
   ** Struct oipc_perm is the obsolete version of struct ipc_perm.
   ** It was used before release A.08.00.
   ** Object code compatibility is supported for old a.out files
   ** that were compiled with this version.  However, old .o files
   ** should be recompiled with the new struct ipc_perm if they are
   ** to be linked with the C library made for release A.08.00 or
   ** later releases.
   **/
   struct oipc_perm {
	unsigned short	uid;	/* owner's user id */
	unsigned short	gid;	/* owner's group id */
	unsigned short	cuid;	/* creator's user id */
	unsigned short	cgid;	/* creator's group id */
	unsigned short	mode;	/* access modes */
	unsigned short	seq;	/* slot usage sequence number */
	key_t		key;	/* key */
#  ifdef __hp9000s800
	unsigned short	ndx;	/* ndx of proc who has lock */
	unsigned short	wait;	/* waits, wanted, lock bits; reserved for 
				   specific ipc facilities */
#  endif /* __hp9000s800 */
   };

     struct ipcmap
     {
	  short	m_size;
	  unsigned short m_addr;
     };

#    define	ipcmapstart(X)		&X[1]
#    define	ipcmapwant(X)		X[0].m_addr
#    define	ipcmapsize(X)		X[0].m_size
#    define	ipcmapdata(X)		{(X)-2, 0} , {0, 0}
#    define	ipcmapinit(X, Y)	X[0].m_size = (Y)-2

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_IPC_INCLUDED */
