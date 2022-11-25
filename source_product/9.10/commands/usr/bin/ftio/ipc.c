/* HPUX_ID: @(#) $Revision: 66.2 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : ipc.c 
 *	Purpose ............... : Inter Process Communications stuff. 
 *	Author ................ : David Williams. 
 *
 *	Description:
 *
 *	Contents:
 *
 *-----------------------------------------------------------------------------
 */

#include "ftio.h"
#include "define.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

static	int	shm_id = -1;
static	int	sem_id = -1;

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : wait_packet()
 *	Purpose ............... : Waits on a new packet to be available. 
 *
 *	Description:
 *
 *	Wait_packet() uses semop(2) to wait on indication from the other
 *	process that a new packet is available.
 *
 *	Returns:
 *
 *	none.
 */

wait_packet(dirn)
int	dirn;
{
	struct	sembuf	operation;
	
#ifdef DEBUG
	if (Diagnostic > 2)
		(void)printf("XXX: semop: PID:%d op:DOWN num:%d\n", Pid, dirn);
#endif 

	operation.sem_flg = 0;		/* always */
	operation.sem_op = DOWN;	/* direction to count */
	operation.sem_num = dirn;	/* semaphore number */

	if (semop(sem_id, &operation, 1) == -1)
	{
		(void)ftio_mesg(FM_NSEMOP);
		(void)myexit(1);
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : release_packet()
 *	Purpose ............... : Release a packet to the other process. 
 *
 *	Description:
 *
 *	Release_packet() uses semop(2) to indicate to the other process
 *	that a packet is available.
 *
 *	Returns:
 *
 *	none. 
 *
 */

release_packet(dirn)
int	dirn;
{
	struct	sembuf	operation;
	
#ifdef DEBUG
	if (Diagnostic > 2)
		(void)printf("XXX: semop: PID:%d op:UP num:%d\n", Pid, dirn);
#endif 

	operation.sem_flg = 0;		/* always */
	operation.sem_op = UP;	/* direction to count */
	operation.sem_num = dirn;	/* semaphore number */

	if (semop(sem_id, &operation, 1) == -1)
	{
		(void)ftio_mesg(FM_NSEMOP);
		(void)myexit(1);
	}
}


static	key_t	key;

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : shmalloc()
 *	Purpose ............... : Allocate shared memory. 
 *
 *	Description:
 *
 *	Shmalloc() uses the pathname given as *s to generate a IPC key
 *	it uses this in a shared memory request of size bytes. This
 *	memory is then attached to the user's data space, and a pointer
 *	to it is retured after long alignment is acheived.
 *
 *	Returns:
 *
 *	pointer to shared memory chunk.
 *
 *	NULL if fail.
 */

char    *
shmalloc(s, size)
char	*s;
int	size;
{
	char	*p;

	key_t	ftok();
	char	*shmat();

	/* 
	 *      We can open the output device, so use its name as an
	 * 	accessable path for stdipc() which follows.
	 * 	stdipc() returns a key based on path and id, which can
	 * 	be used in a subsequent shmget() request
	 */
	if (!host && (key = ftok(s, 't')) == -1)	/* for local device */
		return NULL;
	else
		key = IPC_PRIVATE;	/* for remote device */

	/*
	 *	Get extra to allow for long alignment
	 */
	size += sizeof(long);

	/* 
	 * 	Use the key we just got as part of shared memory
	 * 	request.
	 *
	 * 	Request a shared memory segment of size buffersize
	 * 	with read and write access to the user only - see shm_perm_flg
	 */
	if ((shm_id  = shmget(key, size, SHMACCESS | IPC_CREAT)) == -1)
		return NULL;
	
	/* 
	 * 	Use shmop(2) to attach the requested shared data segment
	 * 	to our data segment.
	 */
	if ((int)(p = shmat(shm_id, (char *)0, SHMACCESS | IPC_CREAT)) == -1)
		return NULL;

	/*
	 * 	Make sure the buffers are short & long address aligned, this
	 * 	is necessary for writing in the short type binary headers
	 */
	if ((int)p & (sizeof(long) - 1))
	{
		p += sizeof(long);
		p = (int)p & ~(sizeof(long) - 1);
	}

	return p;
}

init_sems()
{
	/*
	 * 	Request the creation 3 system V semaphores
	 * with read and write access to the user only - see shm_perm_flg
	 * again the key made above again
	 *
	 * sema(INPUT) is used by the filereading process to
	 * inform that a buffer has been filled.
	 *
	 * sema(OUTPUT) is used by the tapewriting process to
	 * inform that a buffer has been successfully written to tape.
	 *
	 */
	if ( (sem_id  = semget(key, 2, SHMACCESS | IPC_CREAT)) == -1)
		return -1;

	/*
	 * initialise semaphores
	 */
	(void)reset_sems();

	return 0;
}

union semun {
	int	val;
	struct	semid_ds *buf;
	ushort	*array;
};

reset_sems()
{
	union	semun	sems;
	int	i;

	/*
	 * 	Initialise semaphores
	 */
	sems.val = 1;
	if (semctl(sem_id, INPUT , (int)SETVAL, sems) == -1)
		return -1;

	sems.val = 0;
	if (semctl(sem_id, OUTPUT, (int)SETVAL, sems) == -1)
		return -1;

	for (i = 0; i < Nobuffers; i++)
	{
		Packets[i].status = PKT_OK;
	}

	return 0;
}


release_ipc()
{
	int	retval = 0;

	if (shm_id != -1 && shmctl(shm_id, IPC_RMID, NULL) != 0)
	{
		(void)ftio_mesg(FM_NRSHM, shm_id);
		retval = -1;
	}

	if (sem_id != -1 && semctl(sem_id, IPC_RMID, NULL) != 0)
	{
		(void)ftio_mesg(FM_NRSEM, sem_id);
		retval = -1;
	}

	return retval;
}


#ifdef PKT_DEBUG
get_val(dirn)
    int dirn;
{
    return semctl(sem_id,dirn,GETVAL,NULL);
}
#endif PKT_DEBUG
