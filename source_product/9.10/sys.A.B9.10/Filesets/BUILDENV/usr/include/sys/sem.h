/* $Header: sem.h,v 1.20.83.5 93/12/09 12:40:21 marshall Exp $ */
#ifndef _SYS_SEM_INCLUDED
#define _SYS_SEM_INCLUDED

/*
**	IPC Semaphore Facility.
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
   **	Semaphore operation flags
   */

#  define SEM_UNDO	010000	/* set up adjust on exit entry */


   /*
   **	Command definitions for semctl()
   */

#  define GETNCNT	3	/* get semncnt */
#  define GETPID	4	/* get sempid */
#  define GETVAL	5	/* get semval */
#  define GETALL	6	/* get all semval's */
#  define GETZCNT	7	/* get semzcnt */
#  define SETVAL	8	/* set semval */
#  define SETALL	9	/* set all semval's */

   /*
   **	Structure Definitions.
   */

   /*
   **	There is one semaphore structure for each semaphore in the system.
   */

   struct __sem {
     unsigned short int semval;		/* semaphore text map address */
     unsigned short int sempid;		/* pid of last operation */
     unsigned short int semncnt;	/* # awaiting semval > cval */
     unsigned short int semzcnt;	/* # awaiting semval = 0 */
   };

   /*
   **	There is one semaphore ID data structure for each set of semaphores
   **	in the system.
   */

   struct semid_ds {
     struct ipc_perm	sem_perm;	/* operation permission struct */
     struct __sem	*sem_base;	/* ptr to first semaphore in set */
     time_t		sem_otime;	/* last semop time */
     time_t		sem_ctime;	/* last change time */
     unsigned short int	sem_nsems;	/* # of semaphores in set */
     char		sem_pad[22];	/* room for future expansion */
   };

   /*
   **	User semaphore template for semop() system calls.
   */

   struct sembuf {
     unsigned short int	sem_num;	/* semaphore # */
     short		sem_op;		/* semaphore operation */
     short		sem_flg;	/* operation flags */
   };

   /* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

    /* The fourth argument to semctl() varies depending on the value of
       its first argument.  If desired, "union semun" can be declared
       by the user, but this is not necessary since the individual
       member can just be passed as the argument. */

#  ifdef _PROTOTYPES
     extern int semctl(int, int, int, ...);
     extern int semget(key_t, int, int);
     extern int semop(int, struct sembuf *, unsigned int);
#  else /* not _PROTOTYPES */
     extern int semctl();
     extern int semget();
     extern int semop();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */
#endif /* _INCLUDE_XOPEN_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE

#  define sem		__sem		/* Keep X/OPEN name-space clean */

/*
**	Implementation Constants.
*/

#  define PSEMN	(PZERO + 3)	/* sleep priority waiting for greater value */
#  define PSEMZ	(PZERO + 2)	/* sleep priority waiting for zero */

/*
**	Permission Definitions.
*/

#  define	SEM_A	0200	/* alter permission */
#  define	SEM_R	0400	/* read permission */

/*
**    Non-tunable system limits
*/
#    define SEMOPM	500	/* maximum operations per semop system call */

#    ifdef __hp9000s300
#      define SEMMSL	50	/* maximum number of semaphores per ID */
#    else 
#      ifdef __hp9000s700
#        define SEMMSL	500	/* maximum number of semaphores per ID */
#      else
#        define SEMMSL	2048	/* maximum number of semaphores per ID */
				/* need to allow for 2000+ Oracle users */
#      endif /* __hp9000s700 */
#    endif /* __hp9000s300 */

/*
**	Structure Definitions.
*/

/*
** Struct osemid_ds is the obsolete version of struct semid_ds.
** It was used before release A.08.00.
** Object code compatibility is supported for old a.out files
** that were compiled with this version.  However, old .o files
** should be recompiled with the new struct semid_ds if they are
** to be linked with the C library made for release A.08.00 or
** later releases.
*/
    struct osemid_ds {
	 struct oipc_perm sem_perm;	/* operation permission struct */
	 struct __sem	*sem_base;	/* ptr to first semaphore in set */
	 unsigned short	sem_nsems;	/* # of semaphores in set */
	 time_t		sem_otime;	/* last semop time */
	 time_t		sem_ctime;	/* last change time */
    };

/*
**	There is one undo structure per process in the system.
*/

   struct sem_undo {
	struct sem_undo	*un_np;	/* ptr to next active undo structure */
	short		un_cnt;	/* # of active entries */
	struct undo {
		short	un_aoe;	/* adjust on exit values */
		short	un_num;	/* semaphore # */
		int	un_id;	/* semid */
	}	un_ent[1];	/* undo entries (one minimum) */
   };

/*
** semaphore information structure
*/
   struct	seminfo	{
	int	semmap;		/* # of entries in semaphore map */
	int	semmni;		/* # of semaphore identifiers */
	int	semmns;		/* # of semaphores in system */
	int	semmnu;		/* # of undo structures in system */
	int	semmsl;		/* max # of semaphores per ID */
	int	semopm;		/* max # of operations per semop call */
	int	semume;		/* max # of undo entries per process */
	int	semusz;		/* size in bytes of undo structure */
	int	semvmx;		/* semaphore maximum value */
	int	semaem;		/* adjust on exit max value */
   };

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_SEM_INCLUDED */
