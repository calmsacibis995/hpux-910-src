/* $Header: msg.h,v 1.13.83.5 93/10/19 16:13:10 drew Exp $ */

#ifndef _SYS_MSG_INCLUDED
#define _SYS_MSG_INCLUDED

/*
**	IPC Message Facility.
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
   **	Message Operation Flags.
   */

#  define MSG_NOERROR	010000	/* no error if big message */

   /*
   **	Types
   */

#  ifndef _MSGQNUM_T
#    define _MSGQNUM_T
     typedef unsigned short int msgqnum_t;
#  endif /* _MSGQNUM_T */

#  ifndef _MSGLEN_T
#    define _MSGLEN_T
     typedef unsigned short int msglen_t;
#  endif /* _MSGLEN_T */

   /*
   **	Structure Definitions.
   */

   /*
   **	There is one msg structure for each message that may be in the system.
   */

   struct __msg {
	struct __msg		*msg_next;	/* ptr to next message on q */
	long			msg_type;	/* message type */
	msglen_t		msg_ts;		/* message text size */
	long			msg_spot;	/* message text map address */
   };


   /*
   **	There is one msg queue ID data structure for each queue in the system.
   */

   struct msqid_ds {
	struct ipc_perm	   msg_perm;	   /* operation permission struct */
	struct __msg	   *msg_first;	   /* ptr to first message on q */
	struct __msg	   *msg_last;	   /* ptr to last message on q */
	msgqnum_t 	   msg_qnum;	   /* # of messages on q */
	msglen_t	   msg_qbytes;	   /* max # of bytes on q */
#   ifdef _CLASSIC_ID_TYPES
        unsigned short int msg_filler_lspid;
        unsigned short int msg_lspid;      /* pid of last msgsnd */
        unsigned short int msg_filler_lrpid;
        unsigned short int msg_lrpid;      /* pid of last msgrcv */
#   else
        pid_t	           msg_lspid;      /* pid of last msgsnd */
        pid_t	           msg_lrpid;      /* pid of last msgrcv */
#   endif
	time_t		   msg_stime;	   /* last msgsnd time */
	time_t		   msg_rtime;	   /* last msgrcv time */
	time_t		   msg_ctime;	   /* last change time */
	msglen_t	   msg_cbytes;	   /* current # bytes on q */
	char		   msg_pad[22];	   /* room for future expansion */
   };


   /* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
     extern int msgctl(int, int, struct msqid_ds *);
     extern int msgget(key_t, int);

  /* NOTE:  The second parameter to msgrcv() and msgsnd() is
     a pointer to a user-defined structure.  The first field must
     be of type long that will specify the type of the message,
     followed by a data portion that will hold the data bytes
     of the message.
  */
     extern int msgrcv(int, void *, size_t, long, int);
     extern int msgsnd(int, const void *, size_t, int);
#  else /* not _PROTOTYPES */
     extern int msgctl();
     extern int msgget();
     extern int msgrcv();
     extern int msgsnd();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
       }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */
#endif /* _INCLUDE_XOPEN_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE
#  define msg		__msg		/* Keep XOPEN name-space clean */

/*
**	Implementation Constants.
*/

#  define	PMSG	(PZERO + 2)	/* message facility sleep priority */

/*
**	Permission Definitions.
*/

#  define	MSG_R	0400	/* read permission */
#  define	MSG_W	0200	/* write permission */

/*
**	ipc_perm Mode Definitions.
*/

#  define	MSG_RWAIT	01000	/* a reader is waiting for a message */
#  define	MSG_WWAIT	02000	/* a writer is waiting to send */

#  ifdef __hp9000s800
/*
**	ipc_perm wait Definitions.
*/
#    define	MSG_QWAIT	00001	/* a writer is waiting on qp */
#    define	MSG_FWAIT	00002	/* a writer is waiting on msgfp */
#    define	MSG_MWAIT	00004	/* a writer is waiting on msgmap */
#  endif /* __hp9000s800 */

/*
**	Structure Definitions.
*/

/*
** Struct omsqid_ds is the obsolete version of msqid_ds.
** It was used before release A.08.00.
** Object code compatibility is supported for old a.out files
** that were compiled with this version.  However, old .o files
** should be recompiled with the new struct msqid_ds if they are
** to be linked with the C library made for release A.08.00 or
** later releases.
*/
   struct omsqid_ds {
	struct oipc_perm msg_perm;	/* operation permission struct */
	struct __msg	*msg_first;	/* ptr to first message on q */
	struct __msg	*msg_last;	/* ptr to last message on q */
	unsigned short	msg_cbytes;	/* current # bytes on q */
	unsigned short	msg_qnum;	/* # of messages on q */
	unsigned short	msg_qbytes;	/* max # of bytes on q */
	unsigned short	msg_lspid;	/* pid of last msgsnd */
	unsigned short	msg_lrpid;	/* pid of last msgrcv */
	time_t		msg_stime;	/* last msgsnd time */
	time_t		msg_rtime;	/* last msgrcv time */
	time_t		msg_ctime;	/* last change time */
   };

/*
**	User message buffer template for msgsnd() and msgrcv() system calls.
*/

   struct msgbuf {
	long	mtype;		/* message type */
	char	mtext[1];	/* message text */
   };

/*
**	Message information structure.
*/

   struct msginfo {
	int	       msgmap;	/* # of entries in msg map */
	int	       msgmax; 	/* max message size */
	int	       msgmnb; 	/* max # bytes on queue */
	int	       msgmni; 	/* # of message queue identifiers */
	int	       msgssz; 	/* msg segment size */
				    /* (must be word size multiple) */
	int	       msgtql; 	/* # of system message headers */
	unsigned short msgseg; 	/* # of msg segments (MUST BE < 32768) */
   };
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_MSG_INCLUDED */
