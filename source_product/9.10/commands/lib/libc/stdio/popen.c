/* @(#) $Revision: 70.7 $ */   
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#ifdef __lint
#define fileno _fileno
#endif /* __lint */
#define fclose _fclose
#define execl _execl
#define vfork _vfork
#define pipe _pipe
#define close _close
#define fcntl _fcntl
#define fdopen _fdopen
#define waitpid _waitpid
#define popen _popen
#define pclose _pclose
#ifdef _ANSIC_CLEAN
#define malloc _malloc
#define free _free
#endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"
#include <fcntl.h>
#include <errno.h>

#define	tst(a,b) (*mode == 'r' ? (b) : (a))
#define	RDR	0
#define	WTR	1

extern FILE *fdopen();
extern int execl(), vfork(), pipe(), close(), fcntl();

/*
 * A pid table is maintained so that pclose() knows what pid to
 * wait() for.  The table is merely a linked list of elements that
 * are dynamically allocated as needed.  We never free the elements,
 * since they are so small and it is more efficient to have one
 * avalialable for the next call to popen().
 */
typedef struct pid_table
{
    pid_t  pid;			/* pid of child */
    FILE   *filep;		/* FILE *, for lookup */
    struct pid_table *next;	/* next element */
} PID_TABLE;

static PID_TABLE *popen_table;

#ifdef _NAMESPACE_CLEAN
#undef popen
#pragma _HP_SECONDARY_DEF _popen popen
#define popen _popen
#endif /* _NAMESPACE_CLEAN */

FILE *
popen(cmd, mode)
char *cmd;
char *mode;
{
    int p[2];
    register int myside, yourside;
    register pid_t pid;
    register PID_TABLE *poptr;
    PID_TABLE *popen_info;

    if (pipe(p) < 0)
	return (FILE *)NULL;

    /*
     * Search for a free entry in our popen_table.  We add entries
     * to this list as needed, but never release them so that they
     * are around for future use.
     */
    for (poptr = popen_table; poptr != NULL; poptr = poptr->next)
	if (poptr->filep == (FILE *)NULL)
	    break;
    
    if (poptr == NULL)
    {
	/*
	 * Didn't find a free entry, allocate a new one and add it
	 * to our list.
	 */
	if ((poptr = (PID_TABLE *)malloc(sizeof(PID_TABLE))) == NULL)
	{
	    (void)close(p[0]);
	    (void)close(p[1]);
	    errno = ENOMEM;
	    return (FILE *)NULL;
	}
	poptr->pid   = 0;
	poptr->filep = (FILE *)NULL;
	poptr->next  = popen_table;
	popen_table  = poptr;
    }

    myside = tst(p[WTR], p[RDR]);
    yourside = tst(p[RDR], p[WTR]);
    if ((pid = vfork()) == 0)
    {
	/*
	 * myside and yourside reverse roles in child
	 */
	int stdio;
	register PID_TABLE *poptr2;

	/* close all pipes from other popen's */
	for (poptr2 = popen_table; poptr2 != NULL; poptr2=poptr2->next)
	    if (poptr2->filep != (FILE *)NULL)
		(void)close(fileno(poptr2->filep));

	stdio = tst(0, 1);
	(void)close(myside);
	if (yourside != stdio)
	{
	    /* only if stdio not previously closed... */
	    (void)close(stdio);
	    if(fcntl(yourside, F_DUPFD, stdio) == -1) {
		return (FILE *)NULL;
 	    }
	    (void)close(yourside);
	}
	/* "Q" flag tells the shell to ignore the "ENV" variable if present */
	(void)execl("/bin/posix/sh", "sh", "-cQ", cmd, 0);

	/* The shell wasn't executed; POSIX.2 requires that we exit in such a */
	/* manner that a subsequent call to pclose will return status=127 */
	_exit(127);
    }

    if (pid == -1)
    {
	/*
	 * If the fork() failed, we must close the two file descriptors
	 * of the pipe.  Save the errno from the fork(), so that the
	 * close() calls don't mess it up.
	 */
	int errno_save = errno;
	(void)close(yourside);
	(void)close(myside);
	errno = errno_save;

	return (FILE *)NULL;
    }

    /*
     * Close the child's pipe, turn our pipe into a FILE and initialize
     * the fields of poptr to the pid and filep.
     */
    poptr->pid = pid;
    (void)close(yourside);
    return (poptr->filep = fdopen(myside, mode));
}

#ifdef _NAMESPACE_CLEAN
#undef pclose
#pragma _HP_SECONDARY_DEF _pclose pclose
#define pclose _pclose
#endif /* _NAMESPACE_CLEAN */

int
pclose(ptr)
FILE	*ptr;
{
    register PID_TABLE *poptr;
    int status;
    int save_errno;
    pid_t pid;

    /*
     * find the entry in our pid table so we know who to wait
     * on.
     */
    for (poptr = popen_table; poptr != NULL; poptr = poptr->next)
	if (poptr->filep == ptr)
	    break;
    
    if (poptr == NULL)
    {
	errno = ECHILD;
	return -1;
    }

    pid = poptr->pid;

    /* mark this pipe closed */
    poptr->pid = 0;
    poptr->filep = (FILE *)NULL;

    (void)fclose(ptr);

    save_errno = errno;

    while(1)
    {
	errno = 0;
	if (waitpid(pid, &status, 0) == -1)
	{
	     if (errno == EINTR)
		 continue;
	     else
	     {
		 status = -1;
		 break;
	     }
	}
	else
	    break;
    }

    if (errno == 0)
	errno = save_errno;

    return status;
}
