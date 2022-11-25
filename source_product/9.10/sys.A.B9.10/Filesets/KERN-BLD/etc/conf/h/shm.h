/* $Header: shm.h,v 1.25.83.5 93/10/19 16:15:25 drew Exp $ */

#ifndef _SYS_SHM_INCLUDED
#define _SYS_SHM_INCLUDED

/*
**	IPC Shared Memory Facility.
*/

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_XOPEN_SOURCE

#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#  include "../h/ipc.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#  include <sys/ipc.h>
#endif /* _KERNEL_BUILD */


   /*
   **	Permission Definitions.
   */
#  define SHMLBA	4096	/* segment low boundary address multiple */
			        /* (SHMLBA must be a power of 2) */
				/* (1 << PGSHIFT) */

   /*
   **	Message Operation Flags.
   */

#  define SHM_RDONLY	010000	/* attach read-only (else read-write) */
#  define SHM_RND	020000	/* round attach address to SHMLBA */

   /*
   **	Types.
   */

#  ifndef _SHMATT_T
#    define _SHMATT_T
     typedef unsigned short int shmatt_t;
#  endif /* _SHMATT_T */

   /*
   **	Structure Definitions.
   */

   /*
   **	There is a shared mem ID data structure for each segment in the system.
   **
   **	Note: Do not change the order of the fields or the size of the
   **	      structure.  Otherwise, object code compatibility will be broken.
   **/

   struct shmid_ds {
       struct ipc_perm	shm_perm;	/* operation permission struct */
       int		shm_segsz;	/* size of segment in bytes */
#ifdef _INCLUDE_HPUX_SOURCE
       struct vas	*shm_vas;       /* virtual address space this entry */
#else
       struct __vas	*shm_vas;       /* virtual address space this entry */
#endif
#ifdef _CLASSIC_ID_TYPES
	unsigned short  shm_filler_lpid;
	unsigned short  shm_lpid;	/* pid of last shmop */
	unsigned short  shm_filler_cpid;
	unsigned short  shm_cpid;	/* pid of creator */
#else
	pid_t		shm_lpid;	/* pid of last shmop */
	pid_t		shm_cpid;	/* pid of creator */
#endif
       shmatt_t		shm_nattch;	/* current # attached (accurate) */
       shmatt_t		shm_cnattch;	/* in memory # attached (inaccurate) */
       time_t		shm_atime;	/* last shmat time */
       time_t		shm_dtime;	/* last shmdt time */
       time_t		shm_ctime;	/* last change time */
       char		shm_pad[24];	/* room for future expansion */
   };


   /* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
     extern void *shmat(int, const void *, int);
     extern int shmctl(int, int, struct shmid_ds *);
     extern int shmdt(const void *);
#   if defined(_XPG2) || defined(_XPG3)
     extern int shmget(key_t, int, int);
#   else /* not (_XPG2 || _XPG3) */
     extern int shmget(key_t, size_t, int);
#   endif /* not (_XPG2 || _XPG3) */
#  else /* not _PROTOTYPES */
     extern char *shmat();
     extern int shmctl();
     extern int shmdt();
     extern int shmget();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */

#endif /* _INCLUDE_XOPEN_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE

/*
**	Permission Definitions.
**	Implementation Constants.
*/

#  define SHM_R	0400		/* read permission */
#  define SHM_W	0200		/* write permission */

/*
**	ipc_perm Mode Definitions.
**	Only allowed to use bits 033000; others used in ipc.h
*/

#  define SHM_CLEAR    01000	/* clear segment on next attach */
#  define SHM_DEST     02000	/* destroy segment when # attached = 0 */
#  define SHM_NOSWAP  010000	/* region for shared memory is memory locked */
			      /* (or should be when the region is allocated) */

/*
**	Structure Definitions.
*/

/*
** Struct oshmid_ds is the obsolete version of struct shmid_ds.
** Object code compatibility is supported for old a.out files
** that were compiled with this version.  However, old .o files
** should be recompiled with the new struct shmid_ds if they are
** to be linked with the new libc.a.
*/
   struct oshmid_ds {
        struct oipc_perm   shm_perm;	/* operation permission struct */
        int		   shm_segsz;	/* size of segment in bytes */
        struct vas	  *shm_vas;       /* virtual address space this entry */
        unsigned short int shm_lpid;	/* pid of last shmop */
        unsigned short int shm_cpid;	/* pid of creator */
        unsigned short int shm_nattch;	/* current # attached (accurate) */
        unsigned short int shm_cnattch;	/* in memory # attached (inaccurate) */
        time_t		   shm_atime;	/* last shmat time */
        time_t		   shm_dtime;	/* last shmdt time */
        time_t		   shm_ctime;	/* last change time */
#ifdef __hp9000s300
        char		   shm_pad[60];	/* for object code compability */
#endif /* __hp9000s300 */
#ifdef __hp9000s800
        char		   shm_pad[148];/* for object code compability */
#endif /* __hp9000s800 */
   };

   struct	shminfo {
	int	shmmax;	  /* max shared memory segment size */
	int	shmmin;	  /* min shared memory segment size */
	int	shmmni;   /* # of shared memory identifiers */
	int	shmseg;   /* max attached shared memory segments per process */
   };

#    define SHM_LOCK	3	/* Lock segment in core */
#    define SHM_UNLOCK	4	/* Unlock segment */

#  ifdef _KERNEL
     extern struct shmid_ds shmem[];        /* shared memory header */
#  endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_SHM_INCLUDED */
