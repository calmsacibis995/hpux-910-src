/* @(#) $Revision: 64.1 $ */      
/*LINTLIBRARY*/

#include <errno.h>
#include <signal.h>

extern int execl(), waitpid();
extern int vfork();
static struct sigvec ignvec = { SIG_IGN, 0, 0 };

int
system(s)
char	*s;
{
    pid_t pid;
    int status;
    int save_errno;
    struct sigvec istat, qstat;

    if (s == (char *)0)
	return 1;

    if ((pid = vfork()) == 0)
    {			/* fork successful, in child process */
	(void)execl("/bin/sh", "sh", "-c", s, 0);
	_exit(127);
    }

    /* else parent process */
    /* check that the vfork succeeded */
    if (pid == -1)
    {
	/*
	 * fork failed: return -1
	 * errno should have been set
	 * by vfork()
	 */
	return -1;
    }
    (void)sigvector(SIGINT, &ignvec, &istat);
    (void)sigvector(SIGQUIT, &ignvec, &qstat);

    save_errno = errno;
    do
    {
	errno = 0;
    } while (waitpid(pid, &status, 0) == -1 && errno == EINTR);

    if (errno == 0)
	errno = save_errno;

    (void)sigvector(SIGINT, &istat, (struct sigvec *)0);
    (void)sigvector(SIGQUIT, &qstat, (struct sigvec *)0);
    return status;
}
