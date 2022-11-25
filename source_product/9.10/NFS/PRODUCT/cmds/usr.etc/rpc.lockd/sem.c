/*
** Copyright 1990 (C) Hewlett Packard Corporation
**
** SEM.C
**
** Routines to manipulate lockd's semaphore.  The semaphore is used to
** detect the presence of a lockd  that is already running.  This code
** was leveraged from inetd.
*/

#ifndef lint
static char rcsid[] = "$Header: sem.c,v 1.5.109.1 91/11/19 14:18:11 kcs Exp $";
#endif /* ~lint */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS


#include <errno.h>
#include <sys/sem.h>
#include "prot_sec.h"

#define	LOCKD_SEM_KEY 4484973
#define SEM_CREATE   (IPC_CREAT | IPC_EXCL | 0644)

/*
** Semaphore data structure
*/

union semunion {
    int	val;
    struct  semid_ds *buf;
    ushort  *array;
} semarg;

key_t sem_key = LOCKD_SEM_KEY;	/* The unique key for lockd */
int num_sems = 1;		/* Only one semaphore is necessary */
int semid = 0;			/* The id of lockd's semaphore. */
extern char *progname;

/*
** CLOSESEM
**
**    This routine will remove lockd's semaphore.
**
**    Parameters:
**        None, but it assumes that semid has already
**        been set with a call to semgetid().
**
**    Returns Value:
**	  None.
**
**    Side Effects:
**        The lockd semaphore will no longer exist.
*/

void
semclose()
{
    semarg.val = 0;

    if (semctl(semid, 0, IPC_RMID, semarg) == -1) {
	perror("lockd: semctl(IPC_RMID)");
    }
}

/*
** SEMSETPID
**
**    This routine stores the current pid in the
**    semaphore.
**
**    Parameters:
**        None.
**    Return value:
**        None.
**    Side Effects:
**        The value of the semaphore will contain the current pid.
*/

semsetpid()
{
    semarg.val = 0;

    if (semctl(semid, 0, SETVAL, semarg) == -1) {
	perror("lockd: semctl (SETVAL)");
    }
}

/*
** SEMGETPID
**
**    This routine will retrieve the value associated with the
**    semaphore designated by semid, which is the pid of lockd.
**
**    Parameters:
**        None, but assumes semid has been set in semgetid().
**    Return Value:
**        Returns the pid of the running lockd, which should be
**        stored in the semaphore.  If the call to semctl fails,
**        it returns -1.
**
*/

int
semgetpid()
{
    return(semctl(semid, 0, GETPID, semarg));
}

/*
** SEMGETID
**
**    Find the semaphore id of lockd.
**
**    Parameters:
**        semflg - the flags to use in the call to semget.
**    Return Value:
**        Returns the id of the semaphore.
**    Side Effects:
**        Sets the value of semid to the semaphore id.
*/

int
semgetid(semflg)
   int semflg;
{

    semarg.val = 0;
    
    return (semid = semget(sem_key, num_sems, semflg));
}

/*
** SEMINIT
**
**    Initialize lockd's semaphore.  Returns 0 if successful,
**    1 of an lockd is already running, -1 if there is some
**    other problem.
*/

seminit()
{

    if (semgetid(SEM_CREATE) == -1) {
	if (errno == EEXIST) {
	    int lockd_pid;
	    /*
	    ** find the pid of the lockd, and try to send it signal
	    ** zero (test for existence) ... if it's not present,
	    ** then remove the semaphore.
	    */
	    if (semgetid(0) == -1)
		return(-1);
	    lockd_pid = semgetpid();
	    ENABLEPRIV(SEC_KILL);
	    if (lockd_pid > 0 && kill(lockd_pid, 0) < 0  &&
		errno == ESRCH ) {
		/*
		** then there is no lockd -- the sem was
		** left by a kill -9 on the lockd ...
		*/
	        DISABLEPRIV(SEC_KILL);
		semclose();
		if (semgetid(SEM_CREATE) == -1)
		    return(-1);
		return(0);
	    }
	    else {
		/*
		** The kill failed, which means a lockd
		** is running.
		*/
	        DISABLEPRIV(SEC_KILL);
		return(1);
	    }
	}
	else {
	    /*
	    **	some other errno other than EEXIST on semget;
	    */
	    return(-1);
	}
    }
    return(0);
}


