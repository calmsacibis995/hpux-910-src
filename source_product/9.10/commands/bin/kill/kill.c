static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 * kill - send signal to process
 */

#include <stdio.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <ctype.h>

extern char *ltoa();
char *signal_name="";	/* for error message */

main(argc, argv)
int argc;
char *argv[];
{
    register signo, pid, res;
    int errlev = 0, neg = 0, zero = 0;
    extern errno;
    char *msg;

    /*
     *  POSIX.2 requires "kill -l" to list the signals
     */

    if (argv[1][0] == '-' && argv[1][1] == 'l' && argv[1][2] == '\0')
        list_signals(argv[2]);

    if (argc > 1 && argv[1][0] == '-')
    {
	/*
	 * If the first argument is "--", set the signal number to
	 * the default.  This is so you can send the default signal
	 * to a process group without explicitly listing the default
	 * signal. [and it is getopt-like]
	 *
         * POSIX.2 also requires that if the first argument to kill
         * is a negative integer, it shall be interpreted as a signal
         * and not as a negative pid.
	 */
	if (argv[1][1] == '-' && argv[1][2] == '\0')
	    signo = SIGTERM;
	else
	{
	    signo = atosigno(argv[1] + 1);
	    signal_name = argv[1] + 1;
	}
	argc--;
	argv++;
    }
    else
	signo = SIGTERM;

    if (argc <= 1)
	usage();

    argv++;
    while (argc > 1)
    {
	if (**argv == '-')
	    neg++;
	if (**argv == '0')
	    zero++;
	pid = atoi(*argv);
	if ((pid == 0 && !zero) || (pid < 0 && !neg) ||
	    ((pid > MAXPID || pid < -MAXPID) &&
					    (pid != KILL_ALL_OTHERS)))
	    usage();

	if ((res = kill(pid, signo)) < 0)
	{
	    switch (errno)
	    {
	    case EINVAL:
	        if (pid == 1 && (signo == SIGKILL || signo == SIGSTOP))
		    fprintf(stderr, "kill: can't send pid 1 SIGKILL or SIGSTOP\n");
		    else
			fprintf(stderr, "kill: %s: invalid signal\n", signal_name);
		    break;

	    case EPERM:
		fprintf(stderr, "kill: %d: permission denied\n", pid);
		break;

	    case ESRCH:
	    default:
		if (pid < 0)
		{
		    pid = abs(pid);
		    fprintf(stderr, "kill: %d: no such process group\n", pid);
		}
		else
		    fprintf(stderr, "kill: %d: no such process\n",pid);
    	    }
	    errlev = 2;
	}
	argc--;
	argv++;
	neg = zero = 0;
    }
    return errlev;
}

usage()
{
    fputs("usage: kill [ -signo ] pid ...\n", stderr);
    exit(2);
}

struct
{
    char	*signame;
    int	signo;
} sig[] =
{
    /* Required Signals */
    /* POSIX.2 requires NULL be first */
    { "NULL",	0},
    { "HUP",	SIGHUP },
    { "INT",	SIGINT },
    { "QUIT",	SIGQUIT },
    { "ILL",	SIGILL },
    { "TRAP",	SIGTRAP },
    { "ABRT",	SIGABRT },
    { "EMT",	SIGEMT },
    { "FPE",	SIGFPE },
    { "KILL",	SIGKILL },
    { "BUS",	SIGBUS },
    { "SEGV",	SIGSEGV },
    { "SYS",	SIGSYS },
    { "PIPE",	SIGPIPE },
    { "ALRM",	SIGALRM },
    { "TERM",	SIGTERM },
    { "USR1",	SIGUSR1 },
    { "USR2",	SIGUSR2 },
    { "CHLD",	SIGCHLD },
    { "PWR",	SIGPWR },
    { "VTALRM",	SIGVTALRM },
    { "PROF",	SIGPROF },
#ifdef SIGPOLL
    { "POLL",	SIGPOLL },
#else
    { "IO",	SIGIO },
#endif
    { "WINCH",	SIGWINCH },
    { "STOP",	SIGSTOP },
    { "TSTP",	SIGTSTP },
    { "CONT",	SIGCONT },
    { "TTIN",	SIGTTIN },
    { "TTOU",	SIGTTOU },
    { "URG",	SIGURG },
    { "LOST",	SIGLOST },
#ifdef SIGDIL
    { "DIL",	SIGDIL },
#endif
    { (char*)0,	0 }
};

atosigno(s)
char	*s;
{
    int i;
    char *p=s;

    if (i = atoi(s))
	return i;

    if (*p++ == '0' && *p == '\0')
	return 0;

    for (p = s; *p; p++)
	*p = toupper(*p);

    if (strncmp(s, "SIG", 3) == 0)
	s += 3;

    for (i = 0; sig[i].signame != (char *)0; i++)
	if (strcmp(s, sig[i].signame) == 0)
	    return sig[i].signo;
    return -1;
}



list_signals(parm)
char *parm;
{
    /*
     *  POSIX.2 requires the signals be printed in the
     *  following format on stdout:
     *       "%s%c",<signal>,<sep>
     *  where <signal> is in uppercase, without the SIG prefix
     *  and <sep> is either a newline or a space.  The last <sep>
     *  must be a newline.  The first name printed shall be "NULL".
     */
     /*
      * POSIX.2 requires that if kill -l [sxit_status] is issued,
      * kill will write out the signal name corresponding to the status
      */


#define WSIZE 79                /* Assume display is 80 characters wide */
    int i;
    char *p;
    int length=0;               /* length of string so far ("NULL") */
    int new_length=0;           /* length of next signal name */
  char output[WSIZE+1];       /* we build the output here */

    if(parm) {	/* if -l [exit_status] is given */
	errno = 0;
	i = atoi(parm);
	if(errno) {
		fprintf(stderr, "kill: invalid signal\n");
		exit(1);
	}
	/* exit status (for POSIX shell) will be 128+signal number if terminated
	 * by a signal */
	if(i > 128) i -= 128;
	if(i < 0 || i >= NSIG) {
		fprintf(stderr, "kill: invalid signal\n");
		exit(1);
	}
	fputs(sig[i].signame, stdout);
	fputs("\n",stdout);
	exit(0);
    }
    for (i=0, p=output; sig[i].signame != (char *)0; i++)
    {
        if ((length + (new_length = strlen(sig[i].signame))) >= WSIZE)
        {
            /* we have a full line, print it out and reset */
            *p++ = '\n'; *p='\0';
            fputs(output, stdout);
            p = output; length = 0;
        }
        strcpy(p, sig[i].signame);
        p += new_length;
        *p++ = ' ';
        length += (1 + new_length);
    }

    if (p != output)
    {
        /* print out the last line (if any) */
        *p++ = '\n'; *p = '\0';
        fputs(output, stdout);
    }
   /*  return success from a listing */
    exit(0);
}

