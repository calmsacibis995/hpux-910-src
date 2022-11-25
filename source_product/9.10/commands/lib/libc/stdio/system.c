#ifdef _NAMESPACE_CLEAN
#  define access	_access
#  define execl		_execl
#  define vfork		_vfork
#  define system	_system
#  define sigaction	_sigaction
#  define sigaddset	_sigaddset
#  define sigemptyset	_sigemptyset
#  define sigprocmask	_sigprocmask
#  define waitpid	_waitpid
#endif /* _NAMESPACE_CLEAN */

#include <errno.h>
#include <sys/signal.h>
#include <unistd.h>

#define	SHELL	"/bin/posix/sh"

#ifdef _NAMESPACE_CLEAN
#undef system
#pragma _HP_SECONDARY_DEF _system system
#define system _system
#endif /* _NAMESPACE_CLEAN */
system(s)
char *s;
{
pid_t pid;
int status;
sigset_t oldmask;
struct sigaction act;
struct sigaction o_int_act;
struct sigaction o_quit_act;

    /*
     * ANSI-C and XPG4 require that we return non-zero when the command
     * interpreter is available and zero otherwise.  POSIX.2 requires that we 
     * return non-zero, since in a POSIX.2 conformant system, the interpreter
     * is always available.
     */
    if (s == (char *)0)
    {
	if (access(SHELL, X_OK) == 0)
	    return 1;
	else
	    return 0;
    }

    /*
     * POSIX.2 and XPG4 requires that we ignore SIGINT and SIGQUIT.
     * We must also block SIGCHLD.
     */
    act.sa_handler = SIG_IGN;
    (void) sigemptyset(&act.sa_mask);
    /*
     * The following calls to sigemptyset() are technically required by
     * POSIX, but on HP-UX they are not necessary (given that sa_mask is of
     * type sigset_t which is an array of longs.  I'm leaving this code 
     * commented out for now, but we may want to reconsider if the HP-UX
     * sigset_t definition changes.
     *
     * (void) sigemptyset(&o_int_act.sa_mask);
     * (void) sigemptyset(&o_quit_act.sa_mask);
     */
    act.sa_flags = 0;
    (void) sigaction(SIGINT, &act, &o_int_act);
    (void) sigaction(SIGQUIT, &act, &o_quit_act);

    /* Block SIGCHLD */
    /* Just use the sa_mask from the act struct since it has already been
       initialized */
    (void) sigaddset(&act.sa_mask, SIGCHLD);
    (void) sigprocmask(SIG_BLOCK, &act.sa_mask, &oldmask);
    
    if ((pid = vfork()) == 0)	/* vfork successful, in child process */
    {
        /* Restore original handlers and signal mask in child before exec */
        (void) sigaction(SIGINT, &o_int_act, (struct sigaction *)0);
        (void) sigaction(SIGQUIT, &o_quit_act, (struct sigaction *)0);
        (void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)0);

	/* "Q" flag tells the shell to ignore the "ENV" variable if present */
	(void) execl(SHELL, "sh", "-cQ", s, 0);
	_exit(127);		/* exec failed */
    }

    /* else parent process */
    /* check that the vfork() succeeded */
    if (pid == -1)
    {
	/*
	 * vfork failed: set status (return value) to -1
	 *     errno should have been set by vfork()
	 */
	status = -1;
    }
    else
    {
        /*
         * POSIX.2 and XPG4 also require that system() not return until 
	 * the child process has terminated.  This means that if we get
	 * interrupted for any other reason, we need to loop on the 
	 * waitpid().
         */
	int save_errno = errno;

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
    }

    /* Restore original handlers and signal mask */
    (void) sigaction(SIGINT, &o_int_act, (struct sigaction *)0);
    (void) sigaction(SIGQUIT, &o_quit_act, (struct sigaction *)0);
    (void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)0);

    return status;
}
