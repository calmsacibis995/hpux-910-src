/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_db.c,v $
 * $Revision: 1.6.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/18 13:44:16 $
 */

/* DB_SENDRECV */

/*
 * Database IPC Facility
 */

/*
 * 			Notes on the Implementation
 *
 * 0. Name change
 *
 * All externally visible names have been changed from db_* to proc_*.
 * 
 * 
 * 1. Coding Conventions
 * 
 * All "db" names externally visible names to user programs have an
 * underscore after the "proc", for example, proc_open(), proc_send(),
 * proc_recv(), proc_sendrecv() and proc_close().  All internal "db" names have
 * NO underscore after the "db", for example, the dbipc structure,
 * dbsetuid(), dbfork() and dbsignal().  All flags in the dbipc structure
 * are prefixed with "DBIPC_".  In any data base operation there are
 * potentially two processes: the source process is identified by
 * u.u_procp->p_pid and the destination process is identified by pid or
 * handle->p_pid.  All local variables associated with the source process
 * begin with "source_".  All local variables associated with the
 * destination process begin with "dest_".  Another way of viewing this
 * is that the current process is the source process and the target
 * process is the destination process.  All pointer variables and
 * structure elements end in "p", for example source_userp, a pointer to
 * the source process's user area and dest_dbipcp, a pointer to the
 * destination process's dbipc structure.
 * 
 * 2. Allocation and Deallocation of Dbipc Structures
 * 
 * The first db_open() for a conection allocates a dbipc structure for
 * the source and the destination processes.  It allocates one for the
 * source because the source may want to immediately do a db_recv() on
 * the connection.  It allocates one for the destination because the
 * source may want to immediatly do a db_send() or db_sendrecv() on the
 * connection and it needs some place to put the message.  Note that it
 * marks DBIPC_OPEN_PENDING in the destination process's dbipc structure.
 * 
 * Although the first db_open() allocated two dbipc structures, a close
 * (i.e. db_close() or dbforceclose()) only deallocates one dbipc
 * structure.  This is because the destination process may still contain
 * an unreceived message.  And even if it did not, the complexity of the
 * close algorithm is greatly reduced if each process is responsible for
 * deallocating its own dbipc structure when it calls close.  Therefore,
 * the first close (i.e. db_close() or dbforceclose()) deallocates the
 * source dbipc structure and marks DBIPC_CLOSE_PENDING in the
 * destination dbipc structure.
 * 
 * 3. The dbipc Structure Flags
 * 
 * 3.1 DBIPC_OPEN_PENDING
 * 
 * The DBIPC_OPEN_PENDING flag means that a destination process has done
 * a db_open() to the source process and the source process must
 * recipricate before it can send and/or receive messages.  The only
 * legal thing that the source process can do with a dbipc structure
 * marked DBIPC_OPEN_PENDING is db_open() it, because the connection
 * really is NOT open for the source process.  Therefore db_send(),
 * db_recv(), db_sendrecv() and db_close() will all fail.  The only
 * exception to this is dbforceclose() which can close the source
 * processes connection even if it is marked DBIPC_OPEN_PENDING.
 * 
 * 3.2 DBIPC_CLOSE_PENDING
 * 
 * The DBIPC_CLOSE_PENDING flag means that the destination process has closed
 * its side of the connection for some reason.  Because each process must
 * deallocate its own dbipc structure, the destination set the
 * DB_CLOSE_PENDING flag in the source process to tell the source process that
 * it needs to close its connection soon.  We say soon because the source
 * process may still have an event posted in its dbipc structure even though
 * its dbipc structure is marked DBIPC_CLOSE_PENDING.  The source process can
 * still db_recv() the message.  When DBIPC_CLOSE_PENDING is set the source
 * process can db_open() the connection because it must do so in order to get
 * a handle so that it can close the connection.  When DBIPC_CLOSE_PENDING is
 * set, the source process CAN db_recv() a message, if an event has been
 * posted and it CAN db_close() the connection.  However it CANNOT db_send()
 * or db_sendrecv() a message because the destination process is no longer at
 * the other end of the connection.
 * 
 * 3.3 DBIPC_GONE
 * 
 * The DBIPC_GONE flag means that while the source process (i.e. the one
 * with DBIPC_GONE set in its dbipc structure) was blocked, waiting on a
 * message, the destination process closed the connection for some
 * reason.  The possible reasons are: the destination process called
 * db_close(), the destination process called exit() or the destination
 * process called exec().  In the process of closing its side of the
 * connection, the destination process saw that the source process was
 * blocked waiting to receive a message.  The destination process marked
 * DBIPC_GONE (and DB_CLOSE_PENDING) in the source process's dbipc
 * structure and it made the source process runable.  The DBIPC_GONE flag
 * means that when the source process runs again it should return an
 * error, because the destination process, on whom it was blocked waiting
 * to send a message, has closed its side of the connection.  Effectively
 * the destination process has gone away.
 * 
 * 4. Mutual Exclusion
 * 
 *  The following types of references to p_flag and ipc_flags must be
 *  guarded against powerfail and other external interrupts.  In MP
 *  all of the scheduling and signal stuff is guarded by sched_lock.
 *  The uid fields for access checking are guarded by proclock.  One
 *  important point to note here is that for this to work we must
 *  block any code which could cause a signal to be sent (or any
 *  interrupt which could do so).  The sched_lock protects against
 *  other code sending a signal.
 * 
 * 	Reads of p_flag or ipc_flags where the bit(s) you are reading,
 * 	possibly in middle of some atomic action, may be written on
 * 	the interrupt path.
 * 
 * 	Writes of p_flag or ipc_flags where the bit(s) you are
 * 	writing, possibly in the middle of some atomic action, may be
 * 	read or written on the interrupt path.
 * 
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/db.h"
#include "../h/vm.h"
#ifdef  FSD_KI
#include "../h/ki_calls.h"
#endif  /* FSD_KI */


/*
 * Debug Section
 */

/* 2nd debug flag for db_sleep/wakeup */
#ifdef DB_DBG2

/*
 *  Note:  You cannot turn this on with MP and multiple processors
 *  because of the spl calls in printf() which deadlock with ths
 *  spinlock calls in this file
 */
int db_print2 = 0;	/* db_print controls debug printfs */

#define	NDBPRINT2(a1, a2) \
	if (db_print2) ndbprint2(a1, a2)

ndbprint2(a1, a2) {printf(a1, a2);}

#else

#define	NDBPRINT2(a1, a2)

#endif


#define DB_DBG	1

#ifdef DB_DBG

/*
 * If you turn on the db_print flag, you must boot under rdb with
 * rdb printf's enabled. Otherwise we may violate lock ordering.
 */
int db_print = 0;	/* db_print controls debug printfs */

#define	DBPRINT1(a1) \
	if (db_print) dbprint1(a1)
#define	DBPRINT2(a1, a2) \
	if (db_print) dbprint2(a1, a2)
#define	DBPRINT3(a1, a2, a3) \
	if (db_print) dbprint3(a1, a2, a3)
#define	DBPRINT4(a1, a2, a3, a4) \
	if (db_print) dbprint4(a1, a2, a3, a4)
#define	DBPRINT5(a1, a2, a3, a4, a5) \
	if (db_print) dbprint5(a1, a2, a3, a4, a5)
#define	DBPRINT6(a1, a2, a3, a4, a5, a6) \
	if (db_print) dbprint6(a1, a2, a3, a4, a5, a6)

dbprint1(a1) {printf(a1);}
dbprint2(a1, a2) {printf(a1, a2);}
dbprint3(a1, a2, a3) {printf(a1, a2, a3);}
dbprint4(a1, a2, a3, a4) {printf(a1, a2, a3, a4);}
dbprint5(a1, a2, a3, a4, a5) {printf(a1, a2, a3, a4, a5);}
dbprint6(a1, a2, a3, a4, a5, a6) {printf(a1, a2, a3, a4, a5, a6);}

#else

#define	DBPRINT1(a1)
#define	DBPRINT2(a1, a2)
#define	DBPRINT3(a1, a2, a3)
#define	DBPRINT4(a1, a2, a3, a4)
#define	DBPRINT5(a1, a2, a3, a4, a5)
#define	DBPRINT6(a1, a2, a3, a4, a5, a6)

#endif


/* Our phantom wait channel */
int dbchan;
#define DBCHAN (&dbchan)
#define PDBMSG (PZERO+2)


proc_syscall()
{
	register struct a {
		int op,
		    arg1,
		    arg2,
		    arg3;
	} *uap = (struct a *)u.u_ap;

	switch (uap->op) {

	default:
		u.u_error = EINVAL;
		break;

	}
}

/* proc_sendrecv(), sends a message and waits for a reply. This is the
 * fast case that is executed the most often. If our partner is not 
 * waiting for us then we just switch to someone else.
 *
 * Assumes that we are entered without the spllock
 */
proc_sendrecv()
{
	register struct a {
		int arg1,
		    arg2,
		    arg3;
	} *uap = (struct a *)u.u_ap;

	register int options, handle, message;
	register struct dbipc *source_dbipcp, *dest_dbipcp;
	register struct proc *source_procp, *dest_procp;
	register struct user *source_userp;
	int x;

	handle  = uap->arg1;
	options = uap->arg2;
	message = uap->arg3;

	DBPRINT6("proc_sendrecv: entry, source_pid=%d, source_procp=0x%x, handle=0x%x, message=%d(0x%x)\n",
		u.u_procp->p_pid, u.u_procp, handle, message, message);

	/*
 	 * Cache pointer to u-area and proc table for performance.
	 */
	source_userp = &u;		
	source_procp = source_userp->u_procp;

	/*
	 * Return EINVAL if any options requested
	 */
	if (options) {
		DBPRINT1("proc_sendrecv: exit, EINVAL\n");
		source_userp->u_error = EINVAL;
		return;
	}

#ifdef MP

	/* We make the assumption that kernel_sema is NOT owned when we 
   	 * are entered 
	 */
	SPINLOCK(sched_lock);
#endif
	source_dbipcp = source_procp->p_dbipcp;

	/*
	 * Return EBADF if the handle does not refer to a valid, open
	 * connection.
	 */
	if ((!(source_dbipcp))
	    || (source_dbipcp->ipc_flags & DBIPC_OPEN_PENDING)
	    || (source_dbipcp->ipc_flags & DBIPC_CLOSE_PENDING)
	    || (handle != (int)(source_dbipcp->ipc_procp))) {
		DBPRINT1("proc_sendrecv: exit, EBADF\n");
		source_userp->u_error = EBADF;
#ifdef MP
		SPINUNLOCK(sched_lock);

#endif
		return;
	}

	dest_procp = source_dbipcp->ipc_procp;
	dest_dbipcp = dest_procp->p_dbipcp;

	DBPRINT3("proc_sendrecv: dest_pid=%d, dest_procp=0x%x\n",
		dest_procp->p_pid, dest_procp);

	if (dest_dbipcp->ipc_flags & DBIPC_EVENT_POSTED) {
		DBPRINT1("proc_sendrecv: exit, ENOSPC\n");
		source_userp->u_error = ENOSPC;
#ifdef MP
		SPINUNLOCK(sched_lock);

#endif
		return;
	}

	x = UP_SPL7();

	dest_dbipcp->ipc_flags |= DBIPC_EVENT_POSTED;
	dest_dbipcp->ipc_message = message;

	/*
	 * If I have a message, place the other process on the run queue.
	 * If I do not have a message, resume() the other process if it is
	 * blocked waiting on a message or swtch() and do nothing,
	 * effectively blocking me.
	 */
	if (source_dbipcp->ipc_flags & DBIPC_EVENT_POSTED) {
		if (dest_dbipcp->ipc_flags & DBIPC_BLOCKED) {
			DBPRINT2("proc_sendrecv: dbsetrq(pid=%d)\n",
				dest_procp->p_pid);
			dbsetrq(dest_procp, dest_dbipcp);
		}
		DBPRINT3("proc_sendrecv: exit, received message (first), message=%d(0x%x)\n",
			source_dbipcp->ipc_message, source_dbipcp->ipc_message);
		source_dbipcp->ipc_flags &= ~DBIPC_EVENT_POSTED;
		source_userp->u_rval1 = source_dbipcp->ipc_message;
	} else {

		/* Sleeping */
		source_procp->p_stat = SSLEEP;
		source_procp->p_flag |= SSIGABL;
		source_procp->p_wchan = (caddr_t)DBCHAN;
		if(source_procp->p_flag & SRTPROC)
	    		source_procp->p_pri = (PDBMSG < source_procp->p_rtpri) ?
				PDBMSG : source_procp->p_rtpri; 
		else
	    		source_procp->p_pri = PDBMSG;

		source_dbipcp->ipc_flags |= DBIPC_BLOCKED;
		if (dest_dbipcp->ipc_flags & DBIPC_BLOCKED){

			/* We know this process is really asleep and not
			 * blocked on a semaphore because:
			 *   dbipcp->ipc_flags & DBIPC_BLOCKED
			 * and this flag is only manipulated whie the
			 * sched_lock is held.  If the flag is set, then
			 * we know that the 'dest' process has made it
			 * completely through the code that sets him as
			 * SSLEEP
			 */
			/* If not stopped and SLOAD */
			if ((dest_procp->p_stat == SSLEEP)&&(dest_procp->p_flag & SLOAD)){
				DBPRINT2("proc_sendrecv: blocking, resume(pid=%d)\n",
				dest_procp->p_pid);

				dest_dbipcp->ipc_flags &= ~(DBIPC_BLOCKED);

				/* Wake him up */
				dest_procp->p_stat = SRUN;
				dest_procp->p_flag &= ~SSIGABL;
				dest_procp->p_wchan = 0;

#ifdef hp9000s800
#ifdef MP
#ifdef  FSD_KI
				ki_accum_push_TOS(KT_CSW_CLOCK);
				KI_swtch(proc_sendrecv);
#endif  /* FSD_KI */
				if (save(&(source_userp->u_pcb))) {
					source_procp->p_mpflag &= ~SRUNPROC;
					dest_procp->p_mpflag |= SRUNPROC;
					resume(dest_procp->p_upreg->p_space,
						 dest_procp->p_upreg->p_vaddr);
					/* NOTREACHED */
				}
				/*
				 * We come back without the sched_lock/
				 * it is freed in resume()
				 */
				SPINLOCK(sched_lock);
#ifdef	FSD_KI
				/* End the CSW clock */
				ki_accum_pop_TOS_csw();
#endif	/* FSD_KI */
					
#else
				resume(dest_procp->p_upreg->p_space,
					 dest_procp->p_upreg->p_vaddr);

#endif

#else __hp9000s300
				dbresume(dest_procp);
#endif
			} else {
				DBPRINT1("proc_sendrecv: STOPPED or SWAPPED, swtch()\n");
				dbsetrq(dest_procp,dest_dbipcp);

#ifdef FSD_KI
				_swtch(proc_sendrecv);
#else
				swtch();
#endif /* FSD_KI */
#ifdef MP
				/* We do not come back owning the sched_lock 
				 * from swtch so reacquire it
				 */
				SPINLOCK(sched_lock);
#endif
			}

		} else {

			/* Target not waiting, so switch to someone else */
			DBPRINT1("proc_sendrecv: blocking, swtch()\n");
#ifdef FSD_KI
			_swtch(proc_sendrecv);
#else
			swtch();
#endif /* FSD_KI */
#ifdef MP
			SPINLOCK(sched_lock);
#endif

		}
		/* Was it a normal or abnormal termination */
		if (source_dbipcp->ipc_flags & DBIPC_EVENT_POSTED) {

			DBPRINT3("proc_sendrecv: exit, received message (second), message=%d(0x%x)\n",
				source_dbipcp->ipc_message, source_dbipcp->ipc_message);
			source_userp->u_rval1 = source_dbipcp->ipc_message;
			source_dbipcp->ipc_flags &= ~DBIPC_EVENT_POSTED;

		} else {

			if (source_dbipcp->ipc_flags & DBIPC_GONE) {
				DBPRINT1("proc_sendrecv: exit, ECHILD\n");
				source_dbipcp->ipc_flags &= ~DBIPC_GONE;
				source_userp->u_error = ECHILD; 
			} else {
				DBPRINT1("proc_sendrecv: exit, EINTR\n");
				source_userp->u_error = EINTR;
			}
		}
	}
	UP_SPLX(x);
	SPINUNLOCK(sched_lock);
} 

/* proc_send(), sends a message to the receiver. It never blocks
 *
 * Assumes that we are entered without the spllock.
 *
 */
proc_send()
{
	register struct a {
		int arg1,
		    arg2,
		    arg3;
	} *uap = (struct a *)u.u_ap;

	register int options, handle, message;
	register struct dbipc *dest_dbipcp, *source_dbipcp;
	register struct proc *dest_procp, *source_procp;
	register struct user *source_userp;
	int x;

	handle  = uap->arg1;
	options = uap->arg2;
	message = uap->arg3;


	DBPRINT6("proc_send: entry, source_pid=%d, source_procp=0x%x, handle=0x%x, message=%d(0x%x)\n",
		u.u_procp->p_pid, u.u_procp, handle, message, message);

	/*
 	 * Cache pointer to u-area and proc table for performance.
	 */
	source_userp = &u;
	source_procp = source_userp->u_procp;

	/*
	 * Return EINVAL if any options requested
	 */
	if (options) {
		DBPRINT1("proc_send: exit, EINVAL\n");
		source_userp->u_error = EINVAL;
		return;
	}

#ifdef MP
	/* We make the assumption that kernel_sema is NOT owned when we 
   	 * are entered 
	 */
	SPINLOCK(sched_lock);
#endif
	source_dbipcp = source_procp->p_dbipcp;
	/*
	 * Return EBADF if the handle does not refer to a valid, open
	 * connection.
	 */
	if ((!(source_dbipcp))
	    || (source_dbipcp->ipc_flags & DBIPC_OPEN_PENDING)
	    || (source_dbipcp->ipc_flags & DBIPC_CLOSE_PENDING)
	    || (handle != (int)(source_dbipcp->ipc_procp))) {
		DBPRINT1("proc_send: exit, EBADF\n");
		source_userp->u_error = EBADF;
#ifdef MP
		SPINUNLOCK(sched_lock);
#endif
		return;
	}

	dest_procp = source_dbipcp->ipc_procp;
	dest_dbipcp = dest_procp->p_dbipcp;

	DBPRINT3("proc_send: dest_pid=%d, dest_procp=0x%x\n",
		dest_procp->p_pid, dest_procp);

	if (dest_dbipcp->ipc_flags & DBIPC_EVENT_POSTED) {
		DBPRINT1("proc_send: exit, ENOSPC\n");
		source_userp->u_error = ENOSPC;
#ifdef MP
		SPINUNLOCK(sched_lock);
#endif
		return;
	}

	x = UP_SPL7();
	dest_dbipcp->ipc_flags |= DBIPC_EVENT_POSTED;
	dest_dbipcp->ipc_message = message;
	if (dest_dbipcp->ipc_flags & DBIPC_BLOCKED) {
		DBPRINT2("proc_send: dbsetrq(pid=%d)\n",
			dest_procp->p_pid);
		dbsetrq(dest_procp, dest_dbipcp);
	}
	UP_SPLX(x);
	SPINUNLOCK(sched_lock);
	DBPRINT1("proc_send: exit\n");
}


/* proc_recv(), blocks until a message arrives
 *
 * Assumes that we are entered without the spllock.
 *
 */
proc_recv()
{
	register struct a {
		int arg1,
		    arg2;
	} *uap = (struct a *)u.u_ap;

	register int options, handle;
	register struct dbipc *dest_dbipcp, *source_dbipcp;
	register struct proc *dest_procp, *source_procp;
	register struct user *source_userp;
	int x;

	handle  = uap->arg1;
	options = uap->arg2;

	DBPRINT4("proc_recv: entry, source_pid=%d, source_procp=0x%x, handle=0x%x\n",
		u.u_procp->p_pid, u.u_procp, handle);

	/*
 	 * Cache pointer to u-area and proc table for performance.
	 */
	source_userp = &u;		
	source_procp = source_userp->u_procp;

	/*
	 * Return EINVAL if any options requested
	 */
	if (options) {
		DBPRINT1("proc_recv: exit, EINVAL\n");
		source_userp->u_error = EINVAL;
		return;
	}
#ifdef MP

	/* We make the assumption that kernel_sema is NOT owned when we 
   	 * are entered 
	 */
	SPINLOCK(sched_lock);
#endif
	source_dbipcp = source_procp->p_dbipcp;

	/*
	 * Return EBADF if the handle does not refer to a valid, open
	 * connection.
	 */
	if ((!(source_dbipcp))
	   || (source_dbipcp->ipc_flags & DBIPC_OPEN_PENDING)
	   || ((source_dbipcp->ipc_flags & DBIPC_CLOSE_PENDING)
	      && (~(source_dbipcp->ipc_flags & DBIPC_EVENT_POSTED)))
	   || (handle != (int)(source_dbipcp->ipc_procp))) {
		DBPRINT1("proc_recv: exit, EBADF\n");
		source_userp->u_error = EBADF;
#ifdef MP
		SPINUNLOCK(sched_lock);

#endif
		return;
	}

	dest_procp = source_dbipcp->ipc_procp;
	dest_dbipcp = dest_procp->p_dbipcp;

	DBPRINT3("proc_recv: dest_pid=%d, dest_procp=0x%x\n",
		dest_procp->p_pid, dest_procp);

	x = UP_SPL7();
	/* Is our message already here */
	if (source_dbipcp->ipc_flags & DBIPC_EVENT_POSTED) {

		DBPRINT3("proc_recv: exit, received message (first), message=%d(0x%x)\n",
			source_dbipcp->ipc_message, source_dbipcp->ipc_message);
		source_dbipcp->ipc_flags &= ~DBIPC_EVENT_POSTED;
		source_userp->u_rval1 = source_dbipcp->ipc_message;

	} else {

		/* Sleeping */
		source_procp->p_stat = SSLEEP;
		source_procp->p_flag |= SSIGABL;
		source_procp->p_wchan = (caddr_t)DBCHAN;
		if(source_procp->p_flag & SRTPROC)
	    		source_procp->p_pri = (PDBMSG < source_procp->p_rtpri) ?
				PDBMSG : source_procp->p_rtpri; 
		else
	    		source_procp->p_pri = PDBMSG;

		source_dbipcp->ipc_flags |= DBIPC_BLOCKED;
		DBPRINT1("proc_recv: blocking, swtch()\n");
#ifdef FSD_KI
		_swtch(proc_recv);
#else
		swtch();
#endif /* FSD_KI */
#ifdef MP
		SPINLOCK(sched_lock);
#endif

		/* Was it a normal or abnormal termination */
		if (source_dbipcp->ipc_flags & DBIPC_EVENT_POSTED) {

			DBPRINT3("proc_recv: exit, received message (second), message=%d(0x%x)\n",
				source_dbipcp->ipc_message, source_dbipcp->ipc_message);
			source_dbipcp->ipc_flags &= ~DBIPC_EVENT_POSTED;
			source_userp->u_rval1 = source_dbipcp->ipc_message;

		} else {

			if (source_dbipcp->ipc_flags & DBIPC_GONE) {
				DBPRINT1("proc_recv: exit, ECHILD\n");
				source_dbipcp->ipc_flags &= ~DBIPC_GONE;
				source_userp->u_error = ECHILD; 
			} else {
				DBPRINT1("proc_recv: exit, EINTR\n");
				source_userp->u_error = EINTR;
			}
		}
	}
	UP_SPLX(x);
	SPINUNLOCK(sched_lock);
}


/* proc_open() opens a database send/receive connection
 *
 * Assumes that we are entered without the spllock.
 *
 */
proc_open()
{
	register struct a {
		int arg1,
		    arg2;
	} *uap = (struct a *)u.u_ap;

	register int options, pid;
	register struct dbipc *source_dbipcp, *dest_dbipcp, *new_dbipcp;
	register struct proc *source_procp, *dest_procp;
	register struct user *source_userp;
	int x;

	options = uap->arg1;
	pid =     uap->arg2;

	DBPRINT4("proc_open: entry, source_pid=%d, source_procp=0x%x, dest_pid=%d\n",
		u.u_procp->p_pid, u.u_procp, pid);

	/*
 	 * Cache pointers to u-area and proc table for performance.
	 */
	source_userp = &u;		
	source_procp = source_userp->u_procp;

	/*
	 * Return EINVAL if any options requested
	 */
	if (options) {
		DBPRINT1("proc_open: exit, EINVAL\n");
		source_userp->u_error = EINVAL;
		return;
	}

#ifdef MP
	/* acquire now before we get spinlock */
	new_dbipcp = (struct dbipc *)kmem_alloc(sizeof(struct dbipc));
	dest_dbipcp = (struct dbipc *)kmem_alloc(sizeof(struct dbipc));

	/* We make the assumption that kernel_sema is NOT owned when we 
   	 * are entered  may want to lock sched_lock, processlock, and call
	 * pm_find() and then unlock
	 */
	SPINLOCK(sched_lock);
#endif
	source_dbipcp = source_procp->p_dbipcp;
	/*
 	 * Search for destination process.
	 */
	dest_procp = pfind(pid);

	/*
 	 * Return ESRCH if no such process or process
	 * is a zombie.
	 */
	if ((dest_procp == 0) || (dest_procp->p_stat == SZOMB)) {
		DBPRINT1("proc_open: exit, ESRCH\n");
		source_userp->u_error = ESRCH;
#ifdef MP
		/* Release sched_lock and memory */
		dbopencleanup(new_dbipcp, dest_dbipcp);
#endif
		return;
	}

	/*
 	 * Return EINVAL if trying to communicate with self.
	 */
	if (dest_procp == source_procp) {
		DBPRINT1("proc_open: exit, EINVAL\n");
		source_userp->u_error = EINVAL;
#ifdef MP
		/* Release sched_lock and memory */
		dbopencleanup(new_dbipcp, dest_dbipcp);
#endif
		return;
	}

	/*
	 * Return EPERM if kill(2) semantics would not allow
	 * sending a signal.
	 */
#ifdef MP
	/* This assumes that this is either a global lock as it is
  	 * today, or that no one else is changing me besides myself
	 * which is probably not quite true  (see other cases in the
	 *  file of PROCESS_LOCK) */
	SPINLOCK(PROCESS_LOCK(dest_procp));
#endif
	if (source_userp->u_uid
	    && source_userp->u_uid != dest_procp->p_uid
	    && source_userp->u_ruid != dest_procp->p_uid
	    && source_userp->u_uid != dest_procp->p_suid
	    && source_userp->u_ruid != dest_procp->p_suid) {
		DBPRINT1("proc_open: exit, EPERM\n");
		source_userp->u_error = EPERM;
#ifdef MP
		SPINUNLOCK(PROCESS_LOCK(dest_procp));
		/* Release sched_lock and memory */
		dbopencleanup(new_dbipcp, dest_dbipcp);
#endif
		return;
	}
#ifdef MP
	/* If anyone changes the permission now they will have to come 
	 * into this file which will serialize on sched_lock. The only
	 * sticky part is that setuidxxx must release the PROCESS_LOCK
	 * prior to calling the dbsetuid routine  */
	SPINUNLOCK(PROCESS_LOCK(dest_procp));
#endif

	DBPRINT2("proc_open: dest_procp=0x%x\n", dest_procp);

	if (source_dbipcp) {
		if (source_dbipcp->ipc_flags & DBIPC_OPEN_PENDING) {
			/*
			 * Some has already done a proc_open() to me.  Now see
			 * if I am trying to do a proc_open() to them.
			 */
			if (source_dbipcp->ipc_procp == dest_procp) {
				/*
				 * Someone has already done a proc_open() to me and
				 * I now want to do a proc_open() to them.  Complete
				 * the connection and return successfully.
				 */
				DBPRINT2("proc_open: exit, rendezvous open, handle=0x%x\n",
					dest_procp);
#ifdef MP
				x = UP_SPL7();
				source_dbipcp->ipc_flags &= ~DBIPC_OPEN_PENDING;
				UP_SPLX(x);
				source_userp->u_rval1 = dest_procp;
				/* Release spinlock and memory */
				dbopencleanup(new_dbipcp, dest_dbipcp);
#else
				x = UP_SPL7();
				source_dbipcp->ipc_flags &= ~DBIPC_OPEN_PENDING;
				UP_SPLX(x);
				source_userp->u_rval1 = (int)dest_procp;
#endif
				return;
			} else {
				/*
				 * Someone has already done a proc_open() to me and
				 * I now want to do a proc_open() to someone else.  This,
				 * potential second connection for me is not allowed.
				 * Return EMFILE for trying to open a second connection.
				 */
				DBPRINT1("proc_open: exit, first EMFILE\n");
				source_userp->u_error = EMFILE;
#ifdef MP
				/* Release sched_lock and memory */
				dbopencleanup(new_dbipcp, dest_dbipcp);
#endif
				return;
			}
		} else {
			/*
			 * I already have a valid, open connection. Either
			 * I'm trying to redundantly open a connection to
			 * the same process, or I'm trying to open a second connection.
			 * Neither is allowed.
			 */
			if (source_dbipcp->ipc_procp == dest_procp) {
				DBPRINT1("proc_open: exit, EEXIST\n");
				source_userp->u_error = EEXIST;
#ifdef MP
				/* Release sched_lock and memory */
				dbopencleanup(new_dbipcp, dest_dbipcp);
#endif
				return;
			} else {
				DBPRINT1("proc_open: exit, second EMFILE\n");
				source_userp->u_error = EMFILE;
#ifdef MP
				/* Release sched_lock and memory */
				dbopencleanup(new_dbipcp, dest_dbipcp);
#endif
				return;
			}
		}
	}
	/*
	 * First connection so initialize IPC structure for source process.
	 */
#ifdef MP
	/* Mp aquires kmem_alloc up front */
	source_procp->p_dbipcp = source_dbipcp = new_dbipcp;
#else
	source_procp->p_dbipcp = source_dbipcp
		= (struct dbipc *)kmem_alloc(sizeof(struct dbipc));
#endif
	source_dbipcp->ipc_flags = 0;
	source_dbipcp->ipc_procp = dest_procp;

	/*
	 * Initialize IPC structure for destination process.
	 */
#ifdef MP
	dest_procp->p_dbipcp = dest_dbipcp;
#else
	dest_procp->p_dbipcp = dest_dbipcp
		= (struct dbipc *)kmem_alloc(sizeof(struct dbipc));
#endif
	dest_dbipcp->ipc_flags = DBIPC_OPEN_PENDING;
	dest_dbipcp->ipc_procp = source_procp;

	source_userp->u_rval1 = (int)dest_procp;
	DBPRINT2("proc_open: exit, initial open, handle=0x%x\n",
		dest_procp);
#ifdef MP
	SPINUNLOCK(sched_lock);
#endif
}


#ifdef MP
/* Internal routine to release our sched_lock and our eager allocation
 * of memory.
 */
dbopencleanup(source_dbipcp, dest_dbipcp)
register struct dbipc *source_dbipcp, *dest_dbipcp;
{
	/* Release locks and space */
	SPINUNLOCK(sched_lock);
	kmem_free((caddr_t)source_dbipcp, sizeof(struct dbipc));
	kmem_free((caddr_t)dest_dbipcp, sizeof(struct dbipc));
	
}
#endif /* MP */



/* proc_close(), closes down a db connection established by a previous open
 *
 * Assumes that we are entered without the spllock.
 *
 */
proc_close()
{
	register struct a {
		int arg1,
		    arg2;
	} *uap = (struct a *)u.u_ap;

	register int options, handle;
	register struct proc  *source_procp, *dest_procp;
	register struct dbipc *source_dbipcp, *dest_dbipcp;
	register struct user  *source_userp;
	int x;

	handle  = uap->arg1;
	options = uap->arg2;

	DBPRINT4("proc_close: entry, source_pid=%d, source_procp=0x%x, handle=0x%x\n",
		u.u_procp->p_pid, u.u_procp, handle);

	/*
 	 * Cache pointer to u-area and proc table for performance.
	 */
	source_userp = &u;		
	source_procp = source_userp->u_procp;

	/*
	 * Return EINVAL if any options requested
	 */
	if (options) {
		DBPRINT1("proc_open: exit, EINVAL\n");
		source_userp->u_error = EINVAL;
		return;
	}

#ifdef MP
	/* We make the assumption that kernel_sema is NOT owned when we 
   	 * are entered 
	 */
	SPINLOCK(sched_lock);
#endif
	source_dbipcp = source_procp->p_dbipcp;
	/*
	 * Return EBADF if the handle does not refer to a valid, open
	 * connection.
	 */
	if ((!(source_dbipcp))
	    || (source_dbipcp->ipc_flags & DBIPC_OPEN_PENDING)
	    || (handle != (int)(source_dbipcp->ipc_procp))) {
		DBPRINT1("proc_close: exit, EBADF\n");
		source_userp->u_error = EBADF;
#ifdef MP
		SPINUNLOCK(sched_lock);
#endif
		return;
	}

	dest_procp = source_dbipcp->ipc_procp;

	DBPRINT3("proc_close: dest_pid=%d, dest_procp=0x%x\n",
		dest_procp->p_pid, dest_procp);

	if (!(source_dbipcp->ipc_flags & DBIPC_CLOSE_PENDING)) {
		dest_dbipcp = dest_procp->p_dbipcp;
		/* assert(dest_dbipcp != 0) */

		x = UP_SPL7();

		dest_dbipcp->ipc_flags |= DBIPC_CLOSE_PENDING;
		if (dest_dbipcp->ipc_flags & DBIPC_BLOCKED) {
			dest_dbipcp->ipc_flags |= DBIPC_GONE;
			DBPRINT2("proc_close: dbsetrq(pid=%d)\n",
				dest_procp->p_pid);
			dbsetrq(dest_procp, dest_dbipcp);
		}

		UP_SPLX(x);
	}

	DBPRINT1("proc_close: exit\n");
	source_procp->p_dbipcp = (struct dbipc *)0;
#ifdef MP
	SPINUNLOCK(sched_lock);
#endif
	kmem_free((caddr_t)source_dbipcp, sizeof(struct dbipc));
	source_userp->u_rval1 = 0;
}


/*
 * Internal Force Close
 *
 * Difference from proc_close is that it has no errno or system call value return.
 * Doesn't handle bad handles or OPEN_PENDING
 *
 * Assumes we are entered holding the sched_lock, and that we release it 
 * Called from dbexit, dbsetuid
 */
dbforceclose()
{
	register struct dbipc *source_dbipcp, *dest_dbipcp;
	register struct proc *dest_procp, *source_procp;
	register struct user *source_userp;
	int x;

	/*
 	 * Cache pointer to u-area and proc table for performance.
	 */

	source_userp = &u;		
	source_procp = source_userp->u_procp;
	source_dbipcp = source_procp->p_dbipcp;

	if (!source_dbipcp){
#ifdef MP
		SPINUNLOCK(sched_lock);
#endif
		return;
	}

	DBPRINT3("dbforceclose: entry, source_pid=%d, source_procp=0x%x\n",
		u.u_procp->p_pid, u.u_procp);

	dest_procp = source_dbipcp->ipc_procp;

	DBPRINT3("dbforceclose: dest_pid=%d, dest_procp=0x%x\n",
		dest_procp->p_pid, dest_procp);

	if (!(source_dbipcp->ipc_flags & DBIPC_CLOSE_PENDING)) {
		dest_dbipcp = dest_procp->p_dbipcp;
		/* assert(dest_dbipcp != 0) */

		x = UP_SPL7();

		dest_dbipcp->ipc_flags |= DBIPC_CLOSE_PENDING;
		if (dest_dbipcp->ipc_flags & DBIPC_BLOCKED) {
			dest_dbipcp->ipc_flags |= DBIPC_GONE;
			DBPRINT2("dbforceclose: dbsetrq(pid=%d)\n",
				dest_procp->p_pid);
			dbsetrq(dest_procp, dest_dbipcp);
		}

		UP_SPLX(x);
	}

	DBPRINT1("dbforceclose: exit\n");
	source_procp->p_dbipcp = (struct dbipc *)0;
#ifdef MP
	SPINUNLOCK(sched_lock);
#endif
	kmem_free((caddr_t)source_dbipcp, sizeof(struct dbipc));
}


/* dbexit() shuts down the connection and alerts any buddy hanging on 
 * a receive that his partner has terminated
 *
 * We are entered at spl5 from exit().
 * Schedlock is held on entry.
 */
dbexit()
{
	DBPRINT1("dbexit: called\n");
#ifdef MP
	/* We make the assumption that kernel_sema is NOT owned when we 
   	 * are entered , note that dbforce close releases the sched_lock for
	 *  us. This is not necessary , except I do not know the rules 
	 * for kmem_free().
	 */
	SPINLOCK(sched_lock);
#endif
	dbforceclose();
}


/* Dbsetuid, checks that the two processes still have the capabliity to
 * talk to each other. If not it forces a close. This may not really be 
 * required.
 *
 */
dbsetuid()
{
	register struct dbipc *source_dbipcp;
	register struct proc *dest_procp, *source_procp;
	register struct user *source_userp;

	DBPRINT1("dbsetuid: called\n");

	/*
 	 * Cache pointer to u-area and proc table for performance.
	 */
	source_userp = &u;		
	source_procp = source_userp->u_procp;
#ifdef MP
	/* We make the assumption that kernel_sema is NOT owned when we 
   	 * are entered 
	 */
	SPINLOCK(sched_lock);
#endif
	source_dbipcp = source_procp->p_dbipcp;

	if (!source_dbipcp){
#ifdef MP
		SPINUNLOCK(sched_lock);
#endif
		return;
	}

	dest_procp = source_dbipcp->ipc_procp;

#ifdef MP
	SPINLOCK(PROCESS_LOCK(dest_proc));
#endif

	if (source_userp->u_uid != dest_procp->p_uid
	    && source_userp->u_ruid != dest_procp->p_uid
	    && source_userp->u_uid != dest_procp->p_suid
	    && source_userp->u_ruid != dest_procp->p_suid) {
#ifdef MP
		SPINUNLOCK(PROCESS_LOCK(dest_proc));
#endif

		/* Tear down if kill(2) semantics would not allow */
		dbforceclose();
	}
}


/* dbfork() clears the p_dbipc field of the child. Db_open's are not inherited
 * across a fork. They are inherited across an exec (mostly because that is 
 * what the database folks required).
 *
 * We assume we are at spl1 when entered. No need to lock around here as no 
 * one should have a valid handle on the child anyway.
 */
dbfork(procp)
	register struct proc *procp;
{
	DBPRINT3("dbfork: pid=%d, procp=0x%x\n", procp->p_pid, procp);
	procp->p_dbipcp = (struct dbipc *)0;
}


/*
 * dbsetrq()
 *
 * Must be called with interrupts disabled (powerfail and external)
 * If we are sleeping then make runnable. If stopped then clear
 * wchan so "stop" code will make us runnable later
 *
 * Assumes that you already own the sched_lock.
 *
 */
dbsetrq(procp, dbipcp)
	register struct proc *procp;
	register struct dbipc *dbipcp;
{
	DBPRINT3("dbsetrq: pid=%d, procp=0x%x\n", procp->p_pid, procp);

	/* Clear blocked flag, we only come here if it is set */
	dbipcp->ipc_flags &= ~(DBIPC_BLOCKED);

	/* If he is sleeping we can make runnable */
	if (procp->p_stat == SSLEEP)  {
		procp->p_stat = SRUN;
		procp->p_wchan = 0;
		/* Make sure he is in-core
		 * before adding to runq */
		if (procp->p_flag & SLOAD)
			setrq(procp);

	} else {
		/* If he is stopped, we only
		 * clear the wait event */
		if (procp->p_stat == SSTOP) 
			procp->p_wchan = 0;

	}
#define DBSENDRT 1
#ifdef DBSENDRT
	/* We might want to turn this on */
	if ((procp->p_flag&SLOAD) == 0) {
#ifdef	MP
		cvsync(&runout);
#else	/* ! MP */
		if (runout != 0) {
			runout = 0;
#ifdef MP
			/* You can hold spinlock across wakeup,even a
			 *  semaphore?
			 */
#endif
			wakeup((caddr_t)&runout);
		}
#endif	/* ! MP */
		wantin++;
		/* 
	 	 * Raise priority of swapper to 
	 	 * priority of rt process
	 	 * we have to swap in.
	 	 */
		if((procp->p_flag & SRTPROC) &&
		   (proc[S_SWAPPER].p_pri > procp->p_rtpri)){
			(void)changepri(&proc[S_SWAPPER], procp->p_rtpri);
		}
	}
#endif

}


/* 
 * dbunsleep() - called from unsleep()
 *
 * signals - If in stopped state (stat == SSTOP , and SIGCONT arrives, wchan 
 * 	     != 0 will keep us from being run, until ready. In the case where
 *	     we are stopped and a different signal comes in,  then this code
 *	     will remove the sleep state (wchan cleared).
 *
 *         - When we are sleeping (stat == SSLEEP), and a signal
 *           wants to make us runnable then setrun is called which calls
 *           "unsleep". Unsleep calls us to process the wchan if we are
 *           blocked on a db_ipc event. Setrun then makes the process
 *	     runnable.
 *	
 *	Must hold spllock. Do I hold the sched_lock?
 */

dbunsleep(procp)
register struct proc *procp;
{
register struct dbipc *dbipcp;
	
#ifdef MP
	MP_ASSERT(owns_spinlock(sched_lock), " schedlock not owned" );
#endif
	if(!(dbipcp = procp->p_dbipcp)){
		return;
	}

	/* If we are blocked on one of our events then unblock */
	if (dbipcp->ipc_flags & DBIPC_BLOCKED){
		DBPRINT2("dbunsleep: entry procp=0x%x\n", procp);
#ifdef MP
		/* This code is different for MP because MP changed 
		 * the behavior of kern_sig.c which use to call setrun */
		dbsetrq(procp, dbipcp);
#else
		procp->p_wchan = 0;
		dbipcp->ipc_flags &= ~(DBIPC_BLOCKED);
#endif
	}


}
