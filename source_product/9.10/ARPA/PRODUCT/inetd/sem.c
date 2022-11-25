/*
** Copyright 1990 (C) Hewlett Packard Corporation
**
** SEM.C
**
** Routines to manipulate inetd's semaphore.  The semaphore is used to
** detect the presence of an inetd that is already running and to find
** the pid of a running inetd so that it can be sent signals.
*/

#ifndef lint
static char rcsid[] = "$Header: sem.c,v 1.2.109.1 91/11/21 12:01:20 kcs Exp $";
#endif /* ~lint */

#include "inetd.h"

/*
** Semaphore data structure
*/

union semunion {
    int	val;
    struct  semid_ds *buf;
    ushort  *array;
} semarg;

key_t sem_key = INETD_SEM_KEY;	/* The unique key for inetd */
int num_sems = 1;		/* Only one semaphore is necessary */
int semid = 0;			/* The id of inetd's semaphore. */

/*
** CLOSESEM
**
**    This routine will remove inetd's semaphore.
**
**    Parameters:
**        None, but it assumes that semid has already
**        been set with a call to semgetid().
**
**    Returns Value:
**	  None.
**
**    Side Effects:
**        The inetd semaphore will no longer exist.
*/

void
semclose()
{
    semarg.val = 0;

    if (semctl(semid, 0, IPC_RMID, semarg) == -1) {
	perror("inetd: semctl(IPC_RMID)");
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
	perror("inetd: semctl (SETVAL)");
    }
}

/*
** SEMGETPID
**
**    This routine will retrieve the value associated with the
**    semaphore designated by semid, which is the pid of inetd.
**
**    Parameters:
**        None, but assumes semid has been set in semgetid().
**    Return Value:
**        Returns the pid of the running inetd, which should be
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
**    Find the semaphore id of inetd.
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
**    Initialize inetd's semaphore.  Returns 0 if successful,
**    1 of an inetd is already running, -1 if there is some
**    other problem.
*/

seminit()
{

    if (semgetid(SEM_CREATE) == -1) {
	if (errno == EEXIST) {
	    int inetd_pid;
	    /*
	    ** find the pid of the inetd, and try to send it signal
	    ** zero (test for existence) ... if it's not present,
	    ** then remove the semaphore.
	    */
	    if (semgetid(0) == -1)
		return(-1);
	    inetd_pid = semgetpid();
	    if (inetd_pid > 0 && kill(inetd_pid, 0) < 0  &&
		errno == ESRCH) {
		/*
		** then there is no inetd -- the sem was
		** left by a kill -9 on the inetd ...
		*/
		semclose();
		if (semgetid(SEM_CREATE) == -1)
		    return(-1);
		return(0);
	    }
	    else {
		/*
		** The kill failed, which means an inetd
		** is running.
		*/
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


