static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/*
**	ipcrm - IPC remove
**	Remove specified message queues, semaphore sets and shared memory ids.
*/

#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/msg.h>
#include	<sys/sem.h>
#include	<sys/shm.h>
#include	<sys/errno.h>
#include	<stdio.h>

char opts[] = "q:m:s:Q:M:S:";	/* allowable options for getopt */
extern char	*optarg;	/* arg pointer for getopt */
extern int	optind;		/* option index for getopt */
extern int	errno;		/* error return */

key_t getkey();

main(argc, argv)
int	argc;		/* arg count */
char	*argv[];	/* arg vector */
{
	register int	o;	/* option flag */
	register int	err;	/* error count */
	register int	ipc_id;	/* id to remove */
	register key_t	ipc_key;/* key to remove */
	char		*p;	/* ptr to detect bad args */
	extern	long	atol();
	/* Go through the options */
	err = 0;
	while ((o = getopt(argc, argv, opts)) != EOF)
		switch(o) {

		case 'q':	/* message queue */
			ipc_id = (int) strtol(optarg,&p,10);
			if (*p != '\0') {
				s_oops("msqid",optarg);
				break;
			}
			if (msgctl(ipc_id, IPC_RMID, 0) == -1)
				oops("msqid", (long)ipc_id);
			break;

		case 'm':	/* shared memory */
			ipc_id = (int) strtol(optarg,&p,10);
			if (*p != '\0') {
				s_oops("shmid",optarg);
				break;
			}
			if (shmctl(ipc_id, IPC_RMID, 0) == -1)
				oops("shmid", (long)ipc_id);
			break;

		case 's':	/* semaphores */
			ipc_id = (int) strtol(optarg,&p,10);
			if (*p != '\0') {
				s_oops("semid",optarg);
				break;
			}
			if (semctl(ipc_id, IPC_RMID, 0) == -1)
				oops("semid", (long)ipc_id);
			break;

		case 'Q':	/* message queue (by key) */
			if((ipc_key = getkey(optarg)) == 0) {
				s_oops("msgkey",optarg);
				break;
			};
			if ((ipc_id=msgget(ipc_key, 0)) == -1
				|| msgctl(ipc_id, IPC_RMID, 0) == -1)
				oops("msgkey", ipc_key);
			break;

		case 'M':	/* shared memory (by key) */
			if((ipc_key = getkey(optarg)) == 0) {
				s_oops("shmkey",optarg);
				break;
			};
			if ((ipc_id=shmget(ipc_key, 0, 0)) == -1
				|| shmctl(ipc_id, IPC_RMID, 0) == -1)
				oops("shmkey", ipc_key);
			break;

		case 'S':	/* semaphores (by key) */
			if((ipc_key = getkey(optarg)) == 0) {
				s_oops("semkey",optarg);
				break;
			};
			if ((ipc_id=semget(ipc_key, 0, 0)) == -1
				|| semctl(ipc_id, IPC_RMID, 0) == -1)
				oops("semkey", ipc_key);
			break;

		default:
		case '?':	/* anything else */
			err++;
			break;
		}
	if (err || (optind < argc)) {
	    fputs("usage: ipcrm [ [-q msqid] [-m shmid] [-s semid]\n",
	       stderr);
	    fputs("\t[-Q msgkey] [-M shmkey] [-S semkey] ... ]\n",
	       stderr);
	    exit(1);
	}
}

oops(s, i)
char *s;
long   i;
{
	extern char *ltoa();
	char *e;

	switch (errno) {

	case	ENOENT:	/* key not found */
	case	EINVAL:	/* id not found */
		e = "not found";
		break;

	case	EPERM:	/* permission denied */
		e = "permission denied";
		break;
	default:
		e = "unknown error";
	}

	fputs("ipcrm: ", stderr);
	fputs(s, stderr);
	fputc('(', stderr);
	fputs(ltoa(i), stderr);
	fputs("): ", stderr);
	fputs(e, stderr);
	fputc('\n', stderr);
}

s_oops(s, m)
char *s;
char *m;
{
	char *e;

	e = "not found";

	fputs("ipcrm: ", stderr);
	fputs(s, stderr);
	fputc('(', stderr);
	fputs(m, stderr);
	fputs("): ", stderr);
	fputs(e, stderr);
	fputc('\n', stderr);
}

key_t
getkey(kp)
register char *kp;
{
	extern unsigned long strtoul();

	return((key_t)strtoul(kp, (char **)NULL,0));
}
