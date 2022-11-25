# ifndef lint
static char rcsid[] = "$Header: sem.c,v 1.6.109.5 95/02/21 16:08:37 mike Exp $";
# endif	/* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: sem.o $Revision: 1.6.109.5 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <sys/ipc.h>
# include <sys/sem.h>
# include "sendmail.h"

# define SEMKEY	((key_t) 88844888)   /* Daemon semaphore key */

static union semunion {
    int val;
    struct semid_ds *buf;
    ushort *array;
} semarg;

struct semid_ds sem_stat_buf;

int semid;

/*
**  SEMINIT -- initialize mutual exclusion semaphore
**
**	Parameters:
**		euid -- effective uid of sendmail when it started; set the
**			uid of the semaphore to this value so that only the
**			real owner can remove it
**		egid -- effective gid of sendmail when it started; set the
**			gid of the semaphore to this value so that only
**			members of the mail group can remove it
**		recursive -- true if called by seminit
**
**	Returns:
**		if the semaphore already exists, its integer semid; 
**		if not, the integer semid of a new semaphore;
**		if unsuccessful, -1.
**
**	Side Effects:
**		if a valid semaphore already exists, none;
**		if there is an invalid semaphore, it is removed;
**		if there was no valid semaphore, a new semaphore is allocated;
**		  it is initialized to 1;
**		  its sem_perm.uid is set to euid;
**		  its sem_perm.gid is set to egid.
**		
*/

int
seminit(euid, egid, recursive)
    uid_t euid;
    gid_t egid;
    bool recursive;
{
    int semflg, r;

    /* determine whether semaphore already exists */
    semflg = IPC_CREAT | IPC_EXCL | 0644;	/* fail if already exists */
    semid = semget(SEMKEY, 1, semflg);		/* create a semaphore */
    if (semid >= 0)	/* created new semaphore */
    {
	/* get perm.uid */
	semarg.buf = &sem_stat_buf;
	if (semctl(semid, 0, IPC_STAT, semarg) < 0)
	    return(-1);

	/*
	**	set sem_perm.uid to euid,
	**	so semaphore can only be removed by that user
	*/
	semarg.buf->sem_perm.uid = euid;
	r = semctl(semid, 0, IPC_SET, semarg);
	if (r < 0)
	    return(r);

	/* initialize semaphore value */
	semarg.val = 1;
	if (semctl(semid, 0, SETVAL, semarg) < 0)
	    return(-1);
    }
    else if (errno == EEXIST && !recursive)   /* semaphore already existed */
    {
	semflg = 0;
	/* get the id of the existing semaphore */
	if ((semid = semget(SEMKEY, 1, semflg)) >= 0)
	{
	    semarg.buf = &sem_stat_buf;
	    if (semctl(semid, 0, IPC_STAT, semarg) < 0)
		return(-1);
	    if ((semarg.buf->sem_perm.uid == euid) &&
		(semarg.buf->sem_nsems == 1))
	    {
		/* semaphore is valid */
		return(semid);
	    }
	}
	else if (errno != EINVAL)
	{
	    /* unable to get the semid */
	    return(semid);
	}

	/* existing semaphore was invalid -- remove it */
	semid = semctl(semid, 0, IPC_RMID, semarg);
	if (semid < 0)
	    return(semid);
	/* try it again */
	semid = seminit(euid, egid, TRUE);
    }

    return(semid);			/* return semid or -1 on failure */
}
/*
**  DAEMONPID -- return pid of currently running sendmail daemon, if any
**
**	Parameters:
**		None.
**
**	Returns:
**		the pid of the last sendmail daemon to acquire the semaphore.
**		if unsuccessful, -1.
**
**	Side Effects:
**		if successful, none;
**		if unsuccessful, errno will be set.
**		
*/

int
daemonpid()
{
     return(semctl(semid, 0, GETPID, semarg));
}
/*
**  ACQUIRE -- get the appropriate level of mutual exclusion
**
**	Parameters:
**		None.
**
**	Returns:
**		if successful, 0;
**		if unsuccessful, -1.
**
**	Description:
**		To run as the sendmail daemon, a process must acquire
**		the daemon semaphore.  If the attempt fails,
**		acquire returns immediately and the caller should report
**		the failure.
*/

acquire()
{
    struct sembuf sop;
    int serr, sr;

    errno = 0;
    sop.sem_num = 0;	/* poll the semaphore */
    sop.sem_op = -1;	/* decrement semaphore value */

    /*
    ** increment semaphore value on exit; return immediately if not
    ** available
    */
    sop.sem_flg = SEM_UNDO | IPC_NOWAIT ;
    return(semop(semid, &sop, 1));
}
