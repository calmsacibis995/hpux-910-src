/* HPUX_ID: @(#) $Revision: 66.11 $  */

/*
 * "init" is the general process spawning program.  It reads
 *  /etc/inittab for a script.
 *
 * "init" opens systty during the life of the sysinit process (spawned
 * by init at the initialization time for the "sysinit" entry).
 * c_cflag of the termio structure for systty is then recorded.  This
 * info is used when an I/O operation to systty is required.  c_cflag
 * contains baud, par, etc.
 *    Example:
 *         si::sysinit:stty 9600 </dev/systty
 *
 * This is a special case for init since it assumes that the
 * 'sysinit' entry is some form of an stty-like command
 */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<signal.h>
#include	<stdio.h>
#include	"utmp.h"
#include	<errno.h>
#include	<memory.h>
#include	<pwd.h>
#include	<termio.h>
#include	<ctype.h>
#include	<ndir.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sys/sysmacros.h> /* defines 'major(x)' in userinit */
#include        <unistd.h>
#include        <sys/wait.h>

#ifdef DISKLESS
#   include	<sys/unsp.h>    /* definition of UNSP_SIG */
#endif DISKLESS

#ifdef AUDIT
#   include	<sys/audit.h>
#   define SECUREPASS	"/.secure/etc/passwd"
#endif AUDIT

#ifdef TRUX
#   include	<sys/security.h>
#endif

#include        "init.h"
#include        "proctable.h"

#ifndef m_selcode
#   define m_selcode(x)	(int)((unsigned)(x)>>16&0xff)
#endif

#if defined(__hp9000s300) || defined(__hp9000s800)
#   define PTYMAJOR	17		/* slave number	*/
#   define PTYSC	0
#   define PTY_SUPP	1		/* ptys are supported */
#endif

#define	fioctl(p,sptr,cmd)	ioctl(fileno(p),sptr,cmd)

#ifdef UDEBUG
   pid_t SPECIALPID; /* Any pid can be made special for debugging */
#else
/* Normally the special pid is process 1 */
#   define SPECIALPID	1
#endif

/*
 * Useful file and device names.
 */
#ifdef UDEBUG
    char SYSTTY[]	= "/dev/systtyx";
    char SYSCON[]	= "/dev/sysconx";
    char CONSOLE[]	= "/dev/consolex"; /* Console */
    char CORE_RECORD[]	= "core_record";
    char IOCTLSYSCON[]	= "ioctl.syscon"; /* Last syscon modes */
#   undef  UTMP_FILE
#   define UTMP_FILE      "utmp"
#   undef  WTMP_FILE
#   define WTMP_FILE      "wtmp"
#else
    char SYSTTY[]	= "/dev/systty";  /* System Console */
    char SYSCON[]	= "/dev/syscon";  /* Virtual System console */
    char CONSOLE[]	= "/dev/console"; /* Console */
    char IOCTLSYSCON[]	= "/etc/ioctl.syscon";	/* Last syscon modes */
#endif

#ifdef TRUX
/*
 * SU must be a character pointer for SecureWare stuff to work
 */
char *SU       = "/bin/su";      /* Super-user prog, single user mode */
#else
char SU[]       = "/bin/su";     /* Super-user prog, single user mode */
#endif /* TRUX */
char SUNAM[]    = "root";        /* name of Super-user */
char SH[]       = "/bin/sh";     /* Standard Shell */

/*
 * A list of shells to try if there is no root shell in /etc/passwd
 */
struct shells_to_exec
{
    char *path;
    char *arg0;
} shells[] = 
{
#ifdef TRUX
    { "/bin/su",	"su"    },
#else
    { SU,		SU   	},
#endif
    { SH,		"-sh"	},
    { "/bin/ksh",	"-ksh"	},
    { "/bin/csh",	"-csh"	},
    { NULL,		NULL	}
};

struct passwd *rootpwd;		/* passwd entry for SUNAM */

int n_prev[NSIG];	    /* Number of times previously in state */
int cur_state = -1;	    /* Current state of "init" */
int prior_state;
int prev_state;	            /* State "init" was in last time it woke */
int new_state;	            /* State user wants "init" to go to. */
int op_modes = BOOT_MODES;  /* Current state of "init" */
int prev_modes;	            /* init's mode prior to starting spawn() */
int new_inittab;            /* indicates that /etc/inittab changed */

/*
 * When we should next wake up to respawn a process that was respawning
 * too rapidly
 */
int respawn_time;

char INIT_STATE[] = "INIT_STATE=S";

/*
 * The following structures contain a set of modes for /dev/syscon
 */
#define control(x)	('x'&037)

struct	termio	dflt_termio =
{
    BRKINT|IGNPAR|ISTRIP|IXON|IXANY|ICRNL,
    OPOST|ONLCR|TAB3,
    /*
     * CLOCAL should not be set here, it will be
     * determined by the default systty
     * configuration at initialization time
     */
    CS8|CREAD|B9600,
    ISIG|ICANON|ECHO|ECHOK,
    0,0177,control(\\),'#','@',control(D),0,0,0
};

struct	termio	termio;
int	systtycf = CS8|CREAD|B9600;

union WAKEUP
{
    struct WAKEFLAGS
    {
	unsigned w_usersignal : 1;  /* User sent signal to "init" */
	unsigned w_childdeath : 1;  /* An "init" child died */
	unsigned w_powerhit : 1;    /* The OS experienced powerfail */
    } w_flags;
    int w_mask;
} wakeup;

int rsflag;	  /* Set if a respawn has taken place */

/*
 * Set by SIGPWR handler; used by idle() to determine
 * if powerfail has occurred while idling
 */
int pwr_sig = 0;

/*
 * This is the value of our own pid.  If the value is SPECIALPID,
 * then we have to fork to interact with outside world.
 */
pid_t	own_pid;

/*
 * A zero table entry used when calling "account" for non-process type
 * accounting.
 */
proctbl_t dummy;

#ifdef DEBUG
char comment[120];
#endif

sigset_t nosignals;      /* Signal mask with no signals in it */
sigset_t allsignals;     /* mask with all signals (except SIGPWR) */
sigset_t childsignal;    /* mask with just SIGCLD */
sigset_t alarmsignal;    /* mask with just SIGALRM */


/********************/
/****    main    ****/
/********************/
main(argc,argv)
int argc;
char **argv;
{
    int defaultlevel;
    FILE *fp;
    int chg_lvl_flag;
    struct stat statbuf;
    int i;
    int first_time = TRUE;

    sigemptyset(&nosignals);
    sigfillset(&allsignals);
    sigdelset(&allsignals, SIGPWR);
#ifdef UDEBUG
    sigdelset(&allsignals, LVL2);
    sigdelset(&allsignals, LVL3);
    sigdelset(&allsignals, LVL4);
#endif
    sigemptyset(&childsignal); sigaddset(&childsignal, SIGCLD);
    sigemptyset(&alarmsignal); sigaddset(&alarmsignal, SIGALRM);

#ifdef SecureWare
    if (ISSECURE)
#ifdef UDEBUG
	init_file_sources((char **) 0, &SU);
#else
	init_file_sources(&INITTAB, &SU);
#endif /* UDEBUG */
#endif /* SecureWare */
#ifdef TRUX
	shells[0].path = shells[0].arg0 = SU;
#endif

#ifdef UDEBUG
    if (argc == 1)
	SPECIALPID = getpid();
#endif

    /*
     * Ensure that we are writing utmp entries to /etc/utmp
     */
    utmpname(UTMP_FILE);

    /*
     * Determine if we are process 1, the main init, or a user invoked
     * init, whose job it is to inform init to change levels or perform
     * some other action.
     */
    if ((own_pid = getpid()) != SPECIALPID)
	userinit(argc, argv);

#ifdef DEBUG1
    debug("Init is about to start, pid = %d\n", own_pid);
#endif

    /*
     * Set up all signals to be caught or ignored as is appropriate.
     * We block ALL signals, the only time we process a signal is
     * when we do a sigsuspend() call [and then we let *any* signal
     * come in].
     */
    sigprocmask(SIG_BLOCK, &allsignals, NULL);

    init_signals();

#ifdef DISKLESS
    /*
     * Initialize the kernel global variables: my_site (id) and
     * my_sitename which are needed for accessing site dependent
     * objects and intra-cluster communication purposes.  These
     * information are kept in /etc/clusterconf file whose content will
     * vary depending on the role that each site plays.  On root
     * server, the cluster configuration file  contains the complete
     * list of id and names for the entire cluster.  When workstations
     * are initialized as a member of cluster, they will  get site id
     * information from the root server.  Workstations may  have a
     * separate local root file system for use when running as a
     * standalone machine.  The local root  should contain a
     * /etc/clusterconf file containing its host machine's  site id
     * and site names.  This function must be done by process 1 so that
     * process 1's u.u_ area can contain proper site information and
     * pass it to all user processes created heretoafter.
     */
    set_site_id();
#endif /* DISKLESS */

    /* If we've been given a second argument upon boot... */
    if ((argc >= 2) && (getpid() == SPECIALPID))
    {
	if ((defaultlevel = ltos(*argv[1])) == BOGUS_STATE)
	    defaultlevel = initialize(BOGUS_STATE);
	else
	{
	    i = initialize(defaultlevel);
	    if (i != defaultlevel)
		defaultlevel = i;
	    else
		console("Overriding default level with level '%s'\n",
		    argv[1]);
	}
    }
    else
	defaultlevel = initialize(0);
    chg_lvl_flag = FALSE;

#ifdef DEBUG
    console("Debug version of init starting-pid = %d\n", SPECIALPID);
#endif

    /*
     * If there is no default level supplied, ask the user to supply
     * one.
     */
    if (defaultlevel == 0)
	new_state = getlvl();
    else
	new_state = defaultlevel;

    INIT_STATE[11] = stol(new_state);

#if defined(SecureWare)
    if(ISSECURE)
    	init_authenticate(new_state == SINGLE_USER);
#endif

    if (new_state == SINGLE_USER)
    {
	single(defaultlevel);
	chg_lvl_flag = TRUE;
    }
    else
    {
	prev_state = cur_state;
	if (cur_state >= 0)
	{
	    n_prev[cur_state]++;
	    prior_state = cur_state;
	}
	cur_state = new_state;
    }

    /* stat to be sure /etc/utmp is truncate'able by postacct() */
    if (stat("/etc/utmp", &statbuf) == -1)
    {				/* can't stat /etc/utmp */
	if (stat("/etc", &statbuf) == -1)
	{			/* can't even stat /etc */
	    cur_state = SINGLE_USER;
	    console("/etc does not exist\n");
	}
    }

    /* defer accounting of BOOT_TIME and RUN_LEVEL entries */
    dummy.p_flags = DEFERACCT;
    account(BOOT_TIME, &dummy, NULL);	/* Put Boot Entry in "utmp" */
    account(RUN_LVL, &dummy, NULL); /* Make the run level entry */
    dummy.p_flags = 0;

#ifdef DEBUG1
    debug("About to enter main loop\n");
#endif
    /*
     * Here is the beginning of the main process loop.
     */
    for (;;)
    {
	time_t timeout_value;

#ifdef DEBUG1
	debug("In main loop\n");
#endif

#ifdef MEMORY_CHECKS
	check_proc_alloc();
	check_cmd_alloc();
#endif
	/*
	 * Make sure that our concept of the /etc/inittab file is up
	 * to date.  read_inittab() returns TRUE if it changed.
	 * If this is the first time through the main loop, force
	 * spawn() to look through inittab even though it probably
	 * hasn't changed.
	 */
	new_inittab = read_inittab();
	if (first_time)
	{
	    new_inittab = TRUE;
	    first_time = FALSE;
	}

	/*
	 * If in "normal" mode, check all living processes and initiate
	 * kill sequence on those that should not be there anymore.
	 */
	if (op_modes == NORMAL_MODES && cur_state != LVLa
		&& cur_state != LVLb && cur_state != LVLc)
	    remove();

	/*
	 * If a change in run levels is the reason we awoke, now do the
	 * accounting to report the change in the utmp file.  Also
	 * report the change on the system console.
	 */
	if (chg_lvl_flag)
	{
#ifdef DEBUG1
            debug("Change levels to %c\n", level(cur_state));
#endif
	    chg_lvl_flag = FALSE;
	    account(RUN_LVL, &dummy, NULL);
	    console("New run level: %c\n", level(cur_state));
	}

	/*
	 * Save the original mode (unless it is PF_MODES).  It may
	 * change in spawn() if we get a SIGPWR.  If so, we'll want to
	 * run spawn() again with the original mode.
	 */
	if (op_modes != PF_MODES)
	    prev_modes = op_modes;

	/*
	 * Do any cleanup on processes that have terminated
	 *
	 * proc_cleanup() also:
	 *   o) Returns the shortest time that we should sleep before
	 *      sending a SIGKILL to any process that have been warned
	 *      to exit. [0 if no such processes exist].
	 */
	timeout_value = proc_cleanup();

	/*
	 * Scan the inittab file and spawn and respawn processes that
	 * should be alive in the current state.
	 *
	 * Set respawn_time to 0, respawn() [called by spawn()] will
	 * set this to the earliest time that we must wake up to
	 * respawn a process that was respawning too rapidly, or 0 if
	 * there is no such process.
	 */
	respawn_time = 0;
	spawn();

	if (rsflag)
	    rsflag = 0;

	if (cur_state == SINGLE_USER)
	{
	    single(0);
	    if (cur_state != prev_state &&
		cur_state != LVLa &&
		cur_state != LVLb &&
		cur_state != LVLc)
	    {
		chg_lvl_flag = TRUE;
		continue;
	    }
	}

	/*
	 * If a powerfail signal was received during the last sequence,
	 * set mode to powerfail.  When "spawn" is entered the first
	 * thing it does is to check "powerhit".  If it is in PF_MODES
	 * then it clears "powerhit" and does a powerfail sequence.  If
	 * it is not in PF_MODES, then it puts itself in PF_MODES and
	 * then clears "powerhit".  Should "powerhit" get set again
	 * while "spawn" is working on a powerfail sequence (multiple
	 * powerfail), the following code will see that "spawn" tries
	 * to execute the powerfail sequence again.  This guarantees
	 * that the powerfail sequence will be successfully completed
	 * before further processing takes place.
	 */
	if (wakeup.w_flags.w_powerhit)
	{
	    /*
	     *  Make sure that cur_state != prev_state so that
	     *  ONCE and WAIT types work.
	     */
	    if (op_modes == NORMAL_MODES && prev_modes == NORMAL_MODES
		    || cur_state == prev_state)
		prev_state = 0;
	    op_modes = PF_MODES;
	}

	/*
	 * If "spawn" was not just called while in "normal" mode, then
	 * set the mode to "normal" and call it again to check normal
	 * states.
	 */
	else if (op_modes != NORMAL_MODES)
	{
	    /*
	     *  If the mode got changed in spawn(), reset op_modes
	     *  to its original value so init runs again checking
	     *  the original modes.
	     */
	    if (prev_modes != op_modes)
		op_modes = prev_modes;
	    else
		op_modes = NORMAL_MODES;

	    /*
	     *  If we have just finished a powerfail sequence
	     *  (which had the prev_state == 0), set the
	     *  prev_state = cur_state before the next pass
	     *  through.
	     */
	    if (prev_state == 0)
		prev_state = cur_state;
	}

	/*
	 * "spawn was last called with "normal" modes.
	 * If it was a change of levels that awakened us and the new
	 * level is one of the demand levels, LVL[a-c], then reset
	 * the cur_state to the previous state and do another scan to
	 * take care of the usual "respawn" actions.
	 */
	else if (cur_state == LVLa ||
		 cur_state == LVLb ||
		 cur_state == LVLc)
	{
	    if (cur_state >= 0)
		n_prev[cur_state]++;
	    cur_state = prior_state;
	    prior_state = prev_state;
	    prev_state = cur_state;
	    account(RUN_LVL, &dummy, NULL);
	}
	else
	/*
	 * At this point "init" is finished with all actions for the
	 * current wakeup.  Pause until something new takes place.
	 */
	{
	    prev_state = cur_state;

	    /*
	     * Set the alarm to the minimum time that we should either
	     * wake up to send a SIGKILL to a warned process or to
	     * respawn a process that was respawning to rapidly.  If
	     * we don't need to do this, then timeout_value is 0, which
	     * clears the alarm clock anyway.
	     */
	    time_up = FALSE;
#ifdef DEBUG1
	    debug("respawn_time is %d, timeout_value is %d\n",
		respawn_time, timeout_value);
#endif
	    if (respawn_time != 0)
	    {
		respawn_time -= time(0);
		if (respawn_time > 0 &&
		    (timeout_value == 0 ||
		     respawn_time < timeout_value))
		    timeout_value = respawn_time;
	    }
#ifdef DEBUG1
	    if (timeout_value != 0)
		debug("Setting alarm for %d seconds\n", timeout_value);
	    else
		debug("Clearing alarm\n");
#endif
	    alarm(timeout_value);

	    while (wakeup.w_mask == 0 && !time_up)
	    {
		/*
		 * Now pause until there is a signal of some sort.
		 * Signals are disallowed until the sigsuspend call is
		 * actually performed but then all signals are
		 * treated until we return from sigsuspend.
		 */
		sigsuspend(&nosignals);
	    }

	    /*
	     * Now install the new level, if a change in level happened
	     * and then allow signals again while we do our normal
	     * processing.
	     */
	    if (wakeup.w_flags.w_usersignal)
	    {
#ifdef DEBUG
		debug("\nmain\tSignal-new: %c cur: %c prev: %c\n",
			level(new_state), level(cur_state),
			level(prev_state));
#endif
		/*
		 * If we woke up because of a user signal, call
		 * endutent() so that the next utmp access goes to the
		 * current utmp file.
		 * This is so that if /etc/utmp gets trashed, you can
		 * sort-of recover it by fixing up one manually and then
		 * doing a "telinit q"
		 */
		endutent();

		/*
		 * Set flag so that we know to change run level in utmp
		 * file all the old processes have been removed.  Do
		 * not set the flag if a "telinit {Q | a | b | c}" was
		 * done or a telinit to the same level at which init is
		 * already running (which is the same thing as a
		 * "telinit Q").
		 */
		if (new_state != cur_state)
		    if (new_state == LVLa ||
			new_state == LVLb ||
			new_state == LVLc)
		    {
			prev_state = prior_state;
			prior_state = cur_state;
			cur_state = new_state;
			account(RUN_LVL, &dummy, NULL);
		    }
		    else
		    {
			prev_state = cur_state;
			if (cur_state >= 0)
			{
			    n_prev[cur_state]++;
			    prior_state = cur_state;
			}
			cur_state = new_state;
			chg_lvl_flag = TRUE;
		    }

		/*
		 * If the new level is SINGLE_USER, get the state of
		 * the terminal which is "syscon".  These will be
		 * restored before the "su" is started up on the line.
                 */
		if (new_state == SINGLE_USER)
		    get_ioctl_syscon();
		new_state = 0;
	    }

	    /*
	     * If we awoke because of a powerfail, change the operating
	     * mode to powerfail mode.
	     */
	    if (wakeup.w_flags.w_powerhit)
		op_modes = PF_MODES;

	    wakeup.w_mask = 0; /* Clear all wakeup reasons. */
	}
    }
}

/**********************/
/****    single    ****/
/**********************/
single(defaultlevel)
int defaultlevel;
{
    register proctbl_t *su_process;
    int state;
#ifdef DEBUG
    long exit_status;
#endif
    char shell_env[MAXPATHLEN+6];
    char home_env[MAXPATHLEN+5];

    static char *envp[] =
    {
	0,	/* for SHELL= */
	0,	/* for HOME= */
	"PATH=:/bin:/usr/bin:/etc",
	"MAIL=/usr/mail/root",
	"LOGNAME=root",
	INIT_STATE,
	0
    };

    strcpy(shell_env, "SHELL=");
    strcpy(home_env, "HOME=");
    INIT_STATE[11] = stol(SINGLE_USER);
    envp[0] = shell_env;
    envp[1] = home_env;

    for (;;)
    {
	struct stat dummy;

	console("SINGLE USER MODE\n");

	/*
	 * Check /etc/passwd for root entry as safety measure.
	 * Even though we did this in initialize(), we do it in this
	 * loop, in case the user changes the passwd file while we
	 * run the shell.
	 *
	 * We run /bin/su if the root password entry is okay, otherwise
	 * we run /bin/sh.
	 */
#ifdef AUDIT
	if ((rootpwd = getpwnam(SUNAM)) == NULL ||
	    (stat(SECUREPASS, &dummy) != -1 &&
	     getspwnam(SUNAM) == NULL))
	     rootpwd = NULL;
#else
	rootpwd = getpwnam(SUNAM);
#endif
	su_process = efork(NULLPROC, 0);

	/*
	 * Child process...
	 */
	if (su_process == NULLPROC)
	{
	    int i = 0;

            /*
	     * Check Super-user passwd entry (set earlier) to make
	     * sure SU will work.  If not, try using the other shells.
	     */
	    if (rootpwd == NULL ||
		rootpwd->pw_uid != 0 ||
		!valid_exec(rootpwd->pw_shell, FALSE))
	    {
		if (rootpwd == NULL)
		{
		    console("\007\007\
WARNING: No passwd entry for %s.\n\
FIX %s ENTRY IN /etc/passwd AND REBOOT !!!\007\007\n\n\n",
			SUNAM, SUNAM);
		}
		else if (rootpwd->pw_uid != 0)
		{
		    console("\007\007\
WARNING: Non-zero UID for %s.\n\
FIX %s ENTRY IN /etc/passwd AND REBOOT !!!\007\007\n\n\n",
			SUNAM, SUNAM);
		}
		else
		{
		    console("\007\007\
WARNING: Bad shell \"%s\" for %s.\n\
FIX %s ENTRY IN /etc/passwd AND REBOOT !!!\007\007\n\n\n",
		    rootpwd->pw_shell, SUNAM, SUNAM);
		}
		i = 1; /* skip "/bin/su" entry */
	    }
			
	    if (rootpwd && i == 0)
		strcpy(shell_env+6, rootpwd->pw_shell);

	    /*
	     * Check that his home directory exists.  If not, set
	     * his HOME environment variable to "/".
	     */
	    {
		struct stat statbuf;

		if (rootpwd && stat(rootpwd->pw_dir, &statbuf) == 0 &&
		    S_ISDIR(statbuf.st_mode))
		    strcpy(home_env+5, rootpwd->pw_dir);
		else
		    strcpy(home_env+5, "/");
	    }

	    /*
	     * Exec "/bin/su" if it looks as if it will work
	     */
	    if (i == 0)
	    {
		opensyscon(0); /* we want to have a controlling tty */
		execle(shells[0].path, shells[0].arg0, "-", 0,
		    &envp[0]);
		console("execle of %s failed; errno = %d\n",
			 shells[0].path, errno);
		sleep(2);
	    }

	    /*
	     * Loop through all of our other shells (skipping the
	     * first one which is "/bin/su".
	     */
	    for (i = 1; shells[i].path != NULL; i++)
	    {
		opensyscon(0); /* we want to have a controlling tty */
		strcpy(shell_env+6, shells[i].path);
		execle(shells[i].path, shells[i].arg0, 0, &envp[0]);
		console("execle of %s failed, errno = %d\n",
			 shells[i].path, errno);
		sleep(2);
	    }
	    console("\007\007\
ERROR: Couldn't exec any programs for single user mode\n");
	    sleep(5);
	    exit(1);
	}

        /*
	 * If we are the parent, wait around for the child to die or
	 * for "init" to be signaled to change levels.
	 */
#ifdef DEBUG
	while ((exit_status = waitproc(su_process)) == -1)
#else
	while (waitproc(su_process) == -1)
#endif
	{
	    /*
	     * Did we waken because a change of levels?  If so, kill
	     * the child and then exit.
	     */
	    if (wakeup.w_flags.w_usersignal)
	    {
		if (new_state >= LVL0 && new_state <= LVL6)
		{
		    kill(su_process->p_pid, SIGKILL);
		    prev_state = cur_state;
		    if (cur_state >= 0)
		    {
			n_prev[cur_state]++;
			prior_state = cur_state;
		    }
		    cur_state = new_state;
		    new_state = 0;
		    wakeup.w_mask = 0;
		    return;
		}
	    }

	    /*
	     * All other reasons for waking are ignored when in
	     * SINGLE_USER mode.  The only child we are interested in
	     * is being waited for explicitely by "waitproc".
	     */
	    wakeup.w_mask = 0;
	}

	/*
	 * Since the su user process died and the level hasn't been
	 * changed by a signal, either request a new level from the
	 * user if default one wasn't supplied, or use the supplied
	 * default level.
	 */
#ifdef DEBUG
	console("Child's status was 0x%04x\n", exit_status);
#endif
	state = (defaultlevel != 0) ? defaultlevel : getlvl();
	if (state != SINGLE_USER)
	{
	    /*
	     * If the new level is not SINGLE_USER, then exit,
	     * otherwise go back and make up a new "su" process.
	     */
	    prev_state = cur_state;
	    if (cur_state >= 0)
	    {
		n_prev[cur_state]++;
		prior_state = cur_state;
	    }
	    cur_state = state;
	    return;
	}
    }
}

/**********************/
/****    remove    ****/
/**********************/

/*
 * "remove" scans through "proc_table" and performs cleanup.  If there
 * is a process in the table, which shouldn't be here at the current
 * runlevel, then "remove" kills the processes.
 */
remove()
{
    register proctbl_t *process;
    register proctbl_t *prev;
    register proctbl_t *next;
    cmdline_t *cmd;
    time_t min_time = 0; /* time to wake up and KILL a WARNED process */

#ifdef DEBUG1
    debug("In remove()\n");
#endif

    /*
     * If we didn't change levels and /etc/inittab hasn't changed,
     * just return, there is nothing to do.
     */
    if (!new_inittab && cur_state == prev_state)
	return;

    /*
     * Clear the TOUCHED flag on all entries so that when we have
     * finished scanning /etc/inittab, we will be able to tell if
     * we have any processes for which there is no entry in
     * /etc/inittab.
     */
    for (process = proc_table; process; process = process->next)
	process->p_flags &= ~TOUCHED;

    /*
     * Scan all /etc/inittab entries.
     */
    for (cmd = cmd_table; cmd; cmd = cmd->next)
    {
        /*
	 * Scan for process which goes with this entry in /etc/inittab.
	 */
	for (process = proc_table; process; process = process->next)
	{
            /*
	     * Does this slot contain the process we are looking for?
	     */
	    if (!id_eq(process->p_id, cmd->c_id))
		continue;
#ifdef DEBUG
	    debug("remove- id:%s pid: %d time: %lo %d %o %o\n",
		    C(&process->p_id[0]), process->p_pid,
		    process->p_time, process->p_count,
		    process->p_flags, process->p_exit);
#endif

	    /*
	     * Determine if this process can either:
	     *   o) Be alive at this level
	     *   o) Be left alone because it is a demand request
	     *
	     * Dead processes are only touched if they are
	     * marked NOCLEANUP.
	     */
	    if ((process->p_flags & (LIVING|NOCLEANUP|DEMANDREQUEST)) &&
		cur_state != SINGLE_USER && cmd->c_action != M_OFF &&
		((cmd->c_levels & mask(cur_state)) ||
		 (process->p_flags & DEMANDREQUEST)))
		process->p_flags |= TOUCHED;
	}
    }

    /*
     * Scan the proc_table for those marked as LIVING, NAMED, and which
     * haven't been touched TOUCHED by the above scanning.
     * Any entries that haven't been touched cannot exist at the
     * current level so they must be WARNED
     * (unless they are DEAD or have already been WARNED or KILLED).
     */
    for (process = proc_table; process; process = process->next)
    {
	if ((process->p_flags & (LIVING|NAMED|TOUCHED|WARNED|KILLED)) ==
				(LIVING|NAMED))
	{
	    process->p_flags |= WARNED;
	    process->p_time = time((time_t *)0) + TWARN;
#ifdef DEBUG1
	    debug("Warning process %s, pid %d\n",
		process->p_id, process->p_pid);

	    if (kill(process->p_pid, SIGTERM) == -1)
		debug("Warning failed, errno is %d\n", errno);
#else
	    kill(process->p_pid, SIGTERM);
#endif
	}

	/*
	 * Determine the minimum time that we should sleep before
	 * sending a SIGKILL to a WARNED process.
	 */
	if ((process->p_flags & (LIVING|WARNED|KILLED)) ==
						        (LIVING|WARNED))
	    if (min_time == 0 || process->p_time < min_time)
		min_time = process->p_time;
    }

    /*
     * Wait around for any WARNED processes to die, or their grace
     * period to expire.
     * If the grace period expires, we send them a SIGKILL and keep
     * going.
     */
    while (min_time != 0)
    {
	time_t now = time(0);
	long sleep_val;

	sleep_val = min_time - now;
#ifdef DEBUG1
	debug("remove(), waiting for warned to die\n\
    now is %d, min_time is %d, sleep_val is %d\n",
	    now, min_time, sleep_val);
#endif
	if (sleep_val > 0)
	{
	    alarm(sleep_val + 1);
	    sigsuspend(&nosignals);
	}
	else
	    alarm(0);

	min_time = 0;
	now = time(0);

#ifdef DEBUG1
	debug("remove(), after setimer()/sigsuspend(), now is %d\n",
	    now);
#endif

	/*
	 * Scan for processes which should be dying.  We hope they will
	 * die without having to be sent a SIGKILL signal.
	 */
	for (process = proc_table; process; process = next)
	{
	    next = process->next;

	    /*
	     * If this process has been warned to die and his grace
	     * period has expired, kill him with SIGKILL.
	     */
	    if ((process->p_flags & (WARNED|LIVING|KILLED)) ==
						        (WARNED|LIVING))
		if (process->p_time < now)
		{
		    /*
		     * If the kill fails due to pid not found, we should
		     * clean up the process table.
		     * This shouldn't happen with reliable signals,
		     * but we could be in a race condition where the
		     * process has exited, but we haven't processed
		     * its SIGCLD.
		     */
		    process->p_flags |= KILLED;
#ifdef DEBUG1
		    debug("Killing warned process %s, pid %d\n",
			process->p_id, process->p_pid);
#endif
		    if (kill(process->p_pid, SIGKILL) == -1 &&
			    errno == ESRCH)
		    {
#ifdef DEBUG1
			debug("kill: process %s, pid %d not found\n",
			    process->p_id, process->p_pid);
#endif
			/*
			 * If this is a named process, take care of
			 * any accounting.
			 */
			if ((process->p_flags & (NAMED|ACCOUNTED)) ==
						       NAMED)
			{
			    account(DEAD_PROCESS, process, NULL);
			    process->p_flags |= ACCOUNTED;
			}

			/*
			 * If this process isn't in /etc/inittab, we
			 * delete it from the process table.  Otherwise
			 * we just record that is dead and leave it
			 * in the table.
			 */
			if (!(process->p_flags & TOUCHED))
			{
			    if (process == proc_table)
				proc_table = next;
			    else
				prev->next = next;
			    proc_free(process);
			    continue;
			}
			else
			{
			    process->p_flags &= ~(LIVING|WARNED|KILLED);
			    process->p_time = now;
			}
		    }
		}
		else
		{
		    /*
		     * His grace period hasn't expired, record the
		     * minimum time to wait until we try to KILL
		     * anyone.
		     */
		    if (min_time == 0 || process->p_time < min_time)
			min_time = process->p_time;
#ifdef DEBUG1
		    debug("remove(), %s has time left (%d sec)\n",
			process->p_id, (process->p_time - now) + 1);
#endif
		}
	    prev = process;
	}
    }

    /*
     * At this point, all warned processes have been killed.  Look
     * for any NAMED processes that have died and need to be cleaned
     * up.
     */
    for (process = proc_table; process; process = next)
    {
	next = process->next;

	if ((process->p_flags & (LIVING|NAMED)) == NAMED)
	{
	    if (!(process->p_flags & ACCOUNTED))
	    {
		account(DEAD_PROCESS, process, NULL);
		process->p_flags |= ACCOUNTED;
	    }

	    /*
	     * If this named process hasn't been TOUCHED, then free the
	     * space.  It has either died of it's own accord, but isn't
	     * respawnable or was killed because it shouldn't exist at
	     * this level.
	     */
	    if (!(process->p_flags & TOUCHED))
	    {
		if (process == proc_table)
		    proc_table = next;
		else
		    prev->next = next;
		proc_free(process);
		continue;
	    }
	}
	else
	    /*
	     * If this is a named process that no longer exists in
	     * inittab, but it hasn't died yet, clear the NOCLEANUP
	     * flag so that we can clean him up if/when he does finally
	     * die.
	     */
	    if ((process->p_flags & (NAMED|TOUCHED)) == NAMED)
		process->p_flags &= ~NOCLEANUP;
	prev = process;
    }
}

/*
 * do_spawn() --
 *    Given a (possibly NULL) process table entry and an inittab
 *    entry, decide if the command needs to be (re)started.
 *
 *    If it should be run, do_spawn does the necessary stuff to run
 *    the command.
 *
 *    This routine should never be called on a LIVING process.
 */
int
do_spawn(proc, cmd)
proctbl_t *proc;
cmdline_t *cmd;
{
    short lvl_mask = mask(cur_state);

#ifdef DEBUG1
    debug("In do_spawn()\n");
#endif
#ifdef DEBUG1
    if (proc && proc->p_flags != NEWENTRY)
	debug("do_spawn - process:\t%s\t%05d\n%s\t%d\t%o\t%o\n",
		C(&proc->p_id[0]), proc->p_pid,
		ctime(&proc->p_time), proc->p_count,
		proc->p_flags, proc->p_exit);
    debug("do_spawn - cmd:\t%s\t%o\t%o\n\"%s\"\n", C(cmd->c_id),
	    cmd->c_levels, cmd->c_action, cmd->c_command);
#endif

    if (proc && (proc->p_flags & LIVING))
	return;

    /*
     * If there is an existing entry, and it is marked as DEMANDREQUEST,
     * one of the levels a,b, or c is in its levels mask, and the
     * action field is ONDEMAND and ONDEMAND is a permissable mode,
     * and the process is dead, then respawn it.
     */
    if (proc && (proc->p_flags & DEMANDREQUEST) &&
	(cmd->c_levels & (MASKa|MASKb|MASKc)) &&
	(cmd->c_action & op_modes) == M_ONDEMAND)
    {
	respawn(proc, cmd);
	return;
    }

    /*
     * If the action is not an action we are interested in, skip
     * the entry.
     */
    if ((cmd->c_action & op_modes) == 0 ||
	(cmd->c_levels & lvl_mask) == 0)
	return;

    /*
     * If the modes are the normal modes (ONCE, WAIT, RESPAWN, OFF,
     * ONDEMAND) and the action field is either OFF or the action
     * field is ONCE or WAIT and the current level is the same as
     * the last level, then skip this entry.  ONCE and WAIT only
     * get run when the level changes.
     */
    if ((op_modes == NORMAL_MODES) &&
	(cmd->c_action == M_OFF ||
	 (cmd->c_action & (M_ONCE | M_WAIT)) &&
	  cur_state == prev_state))
	return;

    /*
     * At this point we are interested in performing the action for
     * this entry.  Actions fall into two catagories, spinning off
     * a process and not waiting, and spinning off a process and
     * waiting for it to die.
     * If the action is ONCE, RESPAWN, ONDEMAND, POWERFAIL, or BOOT
     * then spin off a process, but don't wait.
     */
    if (cmd->c_action & (M_ONCE|M_RESPAWN|M_PF|M_BOOT))
    {
	respawn(proc, cmd);
	return;
    }

    /*
     * The action must be WAIT, BOOTWAIT, or POWERWAIT,
     * therefore spin off the process, but wait for it to die
     * before continuing.
     */
    if (cmd->c_action & M_BOOTWAIT)
	proc->p_flags |= DEFERACCT;

    respawn(proc, cmd);

    while (waitproc(proc) == -1)
	continue;

    account(DEAD_PROCESS, proc, NULL);
    proc->p_flags |= ACCOUNTED;

    /*
     * Removed this entry from the process table
     */
    proc_delete(proc);
}

/*
 * spawn() --
 *     scan for /etc/inittab entries which should be run at this
 *     mode.  If a process which should be running is found not to be
 *     running, then it is started.
 */
void
spawn()
{
    register proctbl_t *proc;
    cmdline_t *cmd;

#ifdef DEBUG1
    debug("In spawn()\n");
#endif
    /*
     * First check the "powerhit" flag.  If it is set, make sure
     * the modes are PF_MODES and clear the "powerhit" flag.
     */
    if (wakeup.w_flags.w_powerhit)
    {
	wakeup.w_flags.w_powerhit = 0;
	op_modes = PF_MODES;
    }

#ifdef DEBUG1
    debug("spawn\tSignal-new: %c cur: %c prev: %c\n",
	level(new_state), level(cur_state), level(prev_state));
    debug("spawn- lvl_mask: %o op_modes: %o prev_modes: %o\n",
	mask(cur_state), op_modes, prev_modes);
#endif

    /*
     * If either the inittab file changed or we changed levels or we
     * changed the op_modes, we must scan through all of the entries in
     * /etc/inittab, looking them up in the process table.
     *
     * If the inittab file is the same as last time, all we need to
     * do is look for NAMED, non-LIVING processes.  We then look these
     * up in our inittab data structure.
     * By doing the search this way, processing should be sped up
     * substantially over the first method.
     */
    if (new_inittab ||
	cur_state != prev_state || prev_modes != op_modes)
	for (cmd = cmd_table; cmd; cmd = cmd->next)
	{
	    proc = findslot(cmd);
	    do_spawn(proc, cmd);
	    if (proc->p_flags == NEWENTRY)
		proc_free(proc);
	}
    else
	for (proc = proc_table; proc; proc = proc->next)
	    if ((proc->p_flags & (NAMED|LIVING)) == NAMED &&
		(cmd = find_init_entry(proc->p_id)))
		do_spawn(proc, cmd);
#ifdef DEBUG1
    debug("Leave spawn()\n");
#endif
}

/***********************/
/****    respawn    ****/
/***********************/

/*
 * respawn() --
 *     spawn a shell, insert the information about the process
 *     into the proc_table, and do the startup accounting.
 */
void
respawn(process, cmd)
register proctbl_t *process;
register struct CMD_LINE *cmd;
{
    register int i;
    FILE *fp;
    int modes;
    long now;
    time_t next_time;
    static char *envp[] =
    {
	"PATH=/bin:/etc:/usr/bin", INIT_STATE, 0
    };

#ifdef DEBUG1
    debug("**  respawn  **  id:%s\n", C(&process->p_id[0]));
#endif

    /*
     * If we weren't given a process table entry to use, allocate one
     * and put it into the process table.
     */
    if (process == (proctbl_t *)NULL)
    {
	process = proc_alloc();
	process->next = proc_table;
	proc_table = process;
	process->p_flags = 0;
    }

    /*
     * The modes to be sent to "efork" are 0 unless we are spawning
     * a LVLa, LVLb, or LVLc entry or we will be waiting for the death
     * of the child before continuing.
     */
    modes = NAMED;
    if ((process->p_flags & DEMANDREQUEST) ||
	cur_state == LVLa || cur_state == LVLb || cur_state == LVLc)
	modes |= DEMANDREQUEST;

    if ((cmd->c_action & (M_SYSINIT|M_WAIT|M_BOOTWAIT|M_PWAIT)) != 0)
	modes |= NOCLEANUP;

    if (cmd->c_action & M_BOOTWAIT)
	modes |= DEFERACCT;

    /*
     * If this is a respawnable process, check the threshold
     * information to avoid excessive respawns.
     */
    if (cmd->c_action & M_RESPAWN)
    {
	/*
	 * Add the NOCLEANUP to all respawnable commands so that the
	 * information about the frequency of respawns isn't lost.
	 */
	modes |= NOCLEANUP;
	time(&now);

	/*
	 * If no time is assigned, then this is the first time this
	 * command is being processed in this series.  Assign the
	 * current time.
	 */
	if (process->p_time == 0L)
	    process->p_time = now;

	/* Have we just reached the respawn limit? */
	else if (process->p_count++ == SPAWN_LIMIT)
	{
	    /*
	     * If so, have we been respawning it too rapidly?
	     */
	    if ((now - process->p_time) < SPAWN_INTERVAL)
	    {
		/*
		 * If so, generate an error message and refuse to
		 * respawn the process for now.
		 */
		console("Command is respawning too rapidly.\nWill try again in %d minutes.\nCheck for possible errors.\nid:%4s \"%s\"\n",
			INHIBIT/60,
			&cmd->c_id[0], &cmd->c_command[EXEC]);

		/*
		 * Determine when we can respawn this process.  If
		 * this is before when any other process can be
		 * respawned, set "respawn_time" to this time.
		 * The main loop will set an alarm to make sure that
		 * it wakes up in time to restart this guy.
		 */
		next_time = process->p_time + INHIBIT;
		if (next_time < respawn_time || respawn_time == 0)
		    respawn_time = next_time;

		return;
	    }
	    /*
	     * If process hasn't been respawning too often, reset the
	     * count and time to 0 and allow respawn.
	     */
	    else
	    {
		process->p_time = now;
		process->p_count = 0;
	    }
	}
	/*
	 * If this process has been respawning too rapidly and the
	 * inhibit time limit hasn't expired yet, refuse to respawn.
	 */
	else if (process->p_count > SPAWN_LIMIT)
	{
	    /*
	     * Keep p_count from overflowing
	     */
	    process->p_count = SPAWN_LIMIT + 1;
	    if ((now - process->p_time) < INHIBIT)
	    {
		/*
		 * Can't respawn him yet.
		 * Determine when we can respawn this process.  If
		 * this is before when any other process can be
		 * respawned, set "respawn_time" to this time.
		 * The main loop will set an alarm to make sure that
		 * it wakes up in time to restart this guy.
		 */
		next_time = process->p_time + INHIBIT;
		if (next_time < respawn_time || respawn_time == 0)
		    respawn_time = next_time;
		return;
	    }

	    /*
	     * If it is time to try respawning this command, clear the
	     * count and reset the time to now.
	     */
	    else
	    {
		process->p_time = now;
		process->p_count = 0;
	    }
	}
	rsflag = TRUE;
    } /* **** if (cmd->c_action & M_RESPAWN) **** */

    /*
     * Spawn a child process to execute this command.
     */
    process = efork(process, modes);

    /*
     * If we are the child, close up all the open files and set up the
     * default standard input and standard outputs.
     */
    if (process == NULLPROC)
    {
	/*
	 * We wan't all files closed.  Init never has any files open
	 * except possibly the utmp file, so we call endutent() to make
	 * sure that is closed.
	 */
	endutent();

	/*
	 * Now "exec" a shell with the -c option and the command from
	 * /etc/inittab.
	 */
	execle(SH, "INITSH", "-c", cmd->c_command, 0, &envp[0]);
	console("Command\n\"%s\"\n failed to execute.  errno = %d\n",
	    cmd->c_command, errno);

	/*
	 * Don't come back so quickly that "init" hasn't had a chance
	 * to complete putting this child in "proc_table".
	 */
	sleep(20);
	exit(1);
    }

    /*
     * We are the parent, therefore insert the necessary information
     * in the proc_table.
     */
    else
    {
	process->p_id[0] = cmd->c_id[0];
	process->p_id[1] = cmd->c_id[1];
	process->p_id[2] = cmd->c_id[2];
	process->p_id[3] = cmd->c_id[3];
	/*
	 * Perform the accounting for the beginning of a process.
	 * Note that all processes are initially "INIT_PROCESS"es.
	 * Getty will change the type to "LOGIN_PROCESS" and login will
	 * change it to "USER_PROCESS" when they run.
	 */
	account(INIT_PROCESS, process,
		prog_name(&cmd->c_command[EXEC]));
    }
#ifdef DEBUG1
    debug("Leave respawn()\n");
#endif
}
/**************************/
/****    initialize    ****/
/**************************/

/*
 * Perform the initial state setup and look for an initdefault entry in
 * the "inittab" file.
 */
int
initialize(initstate)
int initstate;
{
    cmdline_t *cmd;
    register int mask, i;
    static int states[] =
    {
	LVL0, LVL1, LVL2, LVL3, LVL4, LVL5, LVL6, SINGLE_USER
    };
    FILE *fp_systty, *fp;
    char device[sizeof ("/dev/")+MAXNAMLEN];
    struct direct *dirent;
    DIR *fd_dev;
    struct stat statbuf, statcon;
    register proctbl_t *process, *oprocess;
    int stfp;
    struct termio stterm;
    struct passwd *getpwnam();
#ifdef AUDIT
    struct s_passwd *getspwnam();
    struct stat s_pfile;
#endif

#ifdef DEBUG1
    debug("In initialize()\n");
#endif

    /* Initialize state to "SINGLE_USER" "BOOT_MODES" */
    if (cur_state >= 0)
    {
	n_prev[cur_state]++;
	prior_state = cur_state;
    }
    cur_state = SINGLE_USER;
    op_modes = BOOT_MODES;

#ifdef UDEBUG
    save_ioctl();
#endif

    /*
     * Since init is not a process group leader, we can temporarily
     * open systty and get and termio info without make systty to be
     * control terminal.  We need to get the default CLOCAL bit in
     * c_cflag and copy it to dflt_termio.  So the default tty
     * configuration is correct.			 -kao
     */
    if ((stfp = open(SYSTTY, O_RDWR)) == EOF)
    {
	link(CONSOLE, SYSTTY);
	stfp = open(SYSTTY, O_RDWR);
    }

    if (stfp != EOF)
    {
	ioctl(stfp, TCGETA, &stterm);
	dflt_termio.c_cflag |= (stterm.c_cflag & CLOCAL);
	systtycf = dflt_termio.c_cflag;
	close(stfp);
    }

    read_inittab();

    /*
     * Look for an "initdefault" entry in "/etc/inittab", which
     * specifies the initial level to which "init" is to go at
     * startup time.
     */
    for (cmd = cmd_table; cmd; cmd = cmd->next)
    {
	if (initstate == 0 && cmd->c_action == M_INITDEFAULT)
	{
	    /*
	     * Look through the "c_levels" word, starting at the highest
	     * level.  The assumption is that there will only be one
	     * level specified, but if there is more than one, the
	     * system come up at the highest possible level.
	     */
	    for (mask = MASKSU, i = (sizeof (states)/sizeof (int)) - 1;
					      mask > 0; mask >>= 1, i--)
	    {
		if (mask & cmd->c_levels)
		    initstate = states[i];
	    }
	    continue;
	}

	/*
	 * If the entry is for a system initialization command,
	 * execute it at once, and wait for it to complete.
	 */
	if (cmd->c_action == M_SYSINIT)
	{
#ifdef DEBUG1
            debug("*** sysinit *** cmd = %s\n", cmd->c_command);
#endif
	    /*
	     * Start a process.  Notice that we don't say this is a
	     * named process, since we don't need it in the process
	     * table after system initialization and we don't want to
	     * do accounting on SYSINIT processes.
	     */
	    process = efork(NULLPROC, 0);

	    /*
	     * during the life of the process in sysinit, systty is
	     * opened
	     */
	    if ((stfp = open(SYSTTY, O_RDWR)) == EOF)
	    {
		link(CONSOLE, SYSTTY);
		stfp = open(CONSOLE, O_RDWR);
	    }

	    if (process == NULLPROC)
	    {
		/*
		 * Notice no bookkeeping is performed on these
		 * entries.  This is to avoid doing anything that
		 * would cause writes to the file system to take
		 * place.  No writing should be done until the
		 * operator has had the chance to decide whether
		 * the file system needs checking or not.
		 */

		/*
		 * We wan't all files closed.  Init never has any files
		 * open except possibly the utmp file, so we call
		 * endutent() to make sure that is closed.
		 */
		endutent();

		execl(SH, "INITSH", "-c", cmd->c_command, 0);
		exit(1);
	    }
	    else
		while (waitproc(process) == -1)
		    continue;

	    /*
	     * hopefully, the command in sysinit will setup the
	     * right baud rate
	     */
	    if (stfp != -1)
	    {
		ioctl(stfp, TCGETA, &stterm);
		systtycf = stterm.c_cflag;
		if ((systtycf|CBAUD) == B0)
		    systtycf = dflt_termio.c_cflag;
		close(stfp);
	    }
#ifdef ACCTDEBUG
	    debug("SYSINIT- id: %.4s term: %o exit: %o\n",
		    cmd->c_id, (process->p_exit & 0xff),
		    (process->p_exit & 0xff00) >> 8);
#endif
	}
    }

    /*
     * Get the ioctl settings for /dev/syscon so that it can be
     * brought up in the state it was in when the system went down.
     */
    get_ioctl_syscon();

    /*
     * Check to see if there were no /etc/inittab entries (i.e. no
     * or empty /etc/inittab).
     * Warn the user and come up SINGLE_USER
     */
    if (cmd_table == (cmdline_t *)NULL)
    {
	console("WARNING: %s is CORRUPT.", INITTAB);
	if (initstate && initstate != SINGLE_USER)
	{
	    console("Cannot enter run-level %c.\
\007System being brought up SINGLE-USER !!!\n", level(initstate));
	}
	else
	    console("\007System being brought up SINGLE-USER !!!\n");
	initstate = SINGLE_USER;
    }

    /*
     * Check /etc/passwd for root entry as safety measure.
     * Come up single user if not.  Single() has code to handle this
     * situation
     */
#ifdef AUDIT
    if ((rootpwd = getpwnam(SUNAM)) == NULL ||
	(stat(SECUREPASS, &s_pfile) != -1 && getspwnam(SUNAM) == NULL))
#else
    if ((rootpwd = getpwnam(SUNAM)) == NULL)
#endif
	initstate = SINGLE_USER;
    else
	if (rootpwd->pw_uid != 0 ||
	    !valid_exec(rootpwd->pw_shell, FALSE))
	    initstate = SINGLE_USER;

    if (initstate > 0)
	return initstate;

    /*
     * If the system console is remote, put a message on the system tty
     * warning anyone there that syscon is elsewhere.
     */
    if ((process = efork(NULLPROC, 0)) == NULLPROC)
    {
	/*
	 * Child process
	 */
	if ((fp_systty = fopen(SYSTTY, "r+")) != (FILE *)NULL)
	{
	    if (fstat(fileno(fp_systty), &statbuf) != -1
		    && stat(SYSCON, &statcon) != -1
		    && statbuf.st_rdev != statcon.st_rdev)
	    {
		/*
		 * Since the devices of syscon and systty don't match,
		 * find the device that syscon is linked to and send
		 * the warning to the physical system tty.
		 */
		if ((fd_dev = opendir("/dev")) != (DIR *)0)
		{
		    memset(&device[0], 0, sizeof (device));
		    strcpy(&device[0], "/dev/");
		    while ((dirent = readdir(fd_dev)) != (struct direct *)0)
		    {
			/* If this isn't the syscon entry itself.... */
			strncpy(&device[sizeof ("/dev/")-1],
				&dirent->d_name[0], MAXNAMLEN);
			if (strcmp(SYSCON, &device[0]))
			{
			    if (stat(&device[0], &statbuf) != -1 &&
			        statbuf.st_rdev == statcon.st_rdev)
				break;
			}
		    }
		}
		closedir(fd_dev);
		if (fioctl(fp_systty, TCGETA, &stterm) != EOF)
		{
		    stterm.c_cflag = systtycf;
		    fioctl(fp_systty, TCSETA, &stterm);
		}

		if (statbuf.st_rdev != statcon.st_rdev)
		    fprintf(fp_systty,
			"\nInit: system console is remote.  Type <DEL> to regain control.\n");
		else
		    fprintf(fp_systty,
			"\nInit: system console is remote:  %s; Type <DEL> to regain control.\n", &device[0]);
		fflush(fp_systty);
		fclose(fp_systty);
	    }
	}
	exit(0);
    }

    /*
     * Parent.  Wait for the child to die.
     */
    while (waitproc(process) == -1)
	continue;

    /*
     * Since no "initdefault" entry was found, return 0.  This will
     * have "init" ask the user at /dev/syscon to supply a level.
     */
    return 0;
}

/****************************/
/****    init_signals    ****/
/****************************/

/*
 * Initialize all signals to either be caught or ignored.
 */
void
init_signals()
{
    struct sigaction action;

#ifdef DEBUG1
    debug("In initialize()\n");
#endif
    /*
     * Block all signals during execution of a signal handler, with
     * the exception of SIGPWR.
     */
    sigfillset(&action.sa_mask);
    sigdelset(&action.sa_mask, SIGPWR);
    action.sa_flags = 0;

    action.sa_handler = siglvl;
    sigaction(LVLQ, &action, NULL);    		/* siglvl() */
    sigaction(LVL0, &action, NULL);    		/* siglvl() */
    sigaction(LVL1, &action, NULL);    		/* siglvl() */

#ifdef UDEBUG
    action.sa_handler = SIG_DFL;
#endif
    sigaction(LVL2, &action, NULL);    		/* siglvl() */
    sigaction(LVL3, &action, NULL);    		/* siglvl() */
    sigaction(LVL4, &action, NULL);    		/* siglvl() */
#ifdef UDEBUG
    action.sa_handler = siglvl;
#endif
    sigaction(LVL5, &action, NULL);    		/* siglvl() */
    sigaction(LVL6, &action, NULL);    		/* siglvl() */
    sigaction(SINGLE_USER, &action, NULL);	/* siglvl() */
    sigaction(LVLa, &action, NULL);    		/* siglvl() */
    sigaction(LVLb, &action, NULL);    		/* siglvl() */
    sigaction(LVLc, &action, NULL);    		/* siglvl() */

    action.sa_handler = alarmclk;
    sigaction(SIGALRM, &action, NULL);    	/* alarmclk() */

#ifdef UDEBUG
    action.sa_handler = SIG_DFL;
#else
    action.sa_handler = SIG_IGN;
#endif
    sigaction(SIGTERM, &action, NULL);    	/* SIG_IGN */
    sigaction(SIGUSR2, &action, NULL);    	/* SIG_IGN */

    action.sa_handler = idle;	/* used by reboot(1m) */
    sigaction(SIGUSR1, &action, NULL);    	/* idle() */

    action.sa_handler = childeath;
    sigaction(SIGCLD, &action, NULL);    	/* childeath() */

    action.sa_handler = powerfail;
    sigaction(SIGPWR, &action, NULL);    	/* powerfail() */

    /*
     * Add code for job control
     */
    action.sa_handler = SIG_IGN;
#ifdef SIGTSTP
    sigaction(SIGTSTP, &action, NULL);    	/* SIG_IGN */
#endif
#ifdef SIGTTIN
    sigaction(SIGTTIN, &action, NULL);    	/* SIG_IGN */
#endif
#ifdef SIGTTOU
    sigaction(SIGTTOU, &action, NULL);    	/* SIG_IGN */
#endif

#ifdef DISKLESS
    /*
     * Setup User NSP request signal handler
     */
    action.sa_handler = need_unsp;
    sigaction(UNSP_SIG, &action, NULL);    	/* need_unsp() */
#endif

    alarm(0);
}

/*
 * siglvl() --
 *    Signal routine that catches general user signals that are used
 *    to tell init to change states.
 */
void
siglvl(sig)
int sig;
{
    register proctbl_t *process;

#ifdef DEBUG1
    debug("In siglvl(), signal is %d\n", sig);
#endif
    /*
     * If the signal received is a "LVLQ" signal, do not really
     * change levels, just restate the current level.
     * Otherwise, set new_stat to the new level, the main loop will
     * then do whatever is necessary.
     */
    if (sig == LVLQ)
	new_state = cur_state;
    else
	new_state = sig;

    /*
     * Clear all times and repeat counts in the process table since
     * either the level is changing or the user has editted the
     * "/etc/inittab" file and wants us to look at it again.  If the
     * user has fixed a typo, we don't want residual timing data
     * preventing the fixed command line from executing.
     * Don't do this to WARNED or KILLED processes, since we use
     * the p_time field for these to determine when to send a SIGKILL
     * to them.
     */
    for (process = proc_table; process; process = process->next)
	if ((process->p_flags & (WARNED|KILLED)) == 0)
	{
	    process->p_time = 0L;
	    process->p_count = 0;
	}

    /* Set the flag saying that a "user signal" was received. */
    wakeup.w_flags.w_usersignal = 1;
}

/*
 * alarmclk() --
 *    Signal handler called when a SIGALRM signal is caught.
 *    Merely sets time_up to TRUE to indicate that the timer has
 *    expired.
 */
void
alarmclk()
{
    time_up = TRUE;
}

/*
 * childeath() --
 *    Signal handler invoked when a child process dies.  We merely
 *    record the exit status, etc in the process table.  The main
 *    loop cleans up the processes later.
 */
void
childeath()
{
    register proctbl_t *process;
    register pid_t pid;
    int status;

#ifdef DEBUG1
    debug("In childeath()\n");
#endif
    for (;;)
    {
	/*
	 * Perform wait to get the process id of the child who died and
	 * then scan the process table to see if we are interested in
	 * this process.
	 */
	if ((pid = waitpid(-1, &status, WNOHANG)) == -1 || pid == 0)
	    break;
#ifdef UDEBUG
	debug("childeath: pid=%d status=%x\n", pid, status);
#endif

	for (process = proc_table; process; process = process->next)
	    if (process->p_pid == pid)
	    {
		/*
		 * Mark this process as having died and store the exit
		 * status.  Also set the wakeup flag for a dead child
		 * and break out of the loop.
		 */
		process->p_flags &= ~LIVING;
		process->p_exit = status;
		wakeup.w_flags.w_childdeath = 1;
		break;
	    }

#ifdef UDEBUG
	if (process == NULL)
	    debug("Didn't find process %d.\n", pid);
#endif
    }
#ifdef DEBUG1
    debug("Leaving childeath()\n");
#endif
}

/*
 * powerfail() --
 *    Signal handler for a power failure signal.  Just set some
 *    variables to indicate that we recieved this signal.
 */
void
powerfail()
{
    wakeup.w_flags.w_powerhit = pwr_sig = 1;
}

/**********************/
/****    getlvl    ****/
/**********************/

/*
 * Get the new run level from /dev/syscon.  If someone at /dev/systty
 * types a <del> while we are waiting for the user to start typing,
 * relink /dev/syscon to /dev/systty.
 */
int fd_systty;

/*
 * Due to problems with <stdio>, getlvl doesn't use it.  This
 * define makes for simple printing of strings.
 */
#define		WRTSTR(x,y)	write(x,y,sizeof(y)-1)

int
getlvl()
{
    char c;
    int status;
    struct termio tminfo;
    struct stat stat1, stat2;
    int fd_tmp;
    struct termio stterm;
    register proctbl_t *process;
    int did_switch = FALSE;
    static char levels[] =
    {
	LVL0, LVL1, LVL2, LVL3, LVL4, LVL5, LVL6, SINGLE_USER
    };

    /*
     * Fork a child who will request the new run level from
     * /dev/syscon.
     */
    if ((process = efork(NULLPROC, 0)) == NULLPROC)
    {
	/*
	 * Child process.
	 *
	 * If /dev/systty and /dev/syscon link to the same device don't
	 * expecting interrupt from /dev/systty
	 */
	if (stat(SYSCON, &stat1) != -1 && stat(SYSTTY, &stat2) != -1 &&
	    stat1.st_rdev != stat2.st_rdev)
	{
	    /*
	     * Open /dev/systty so that if someone types a <del>, we
	     * can be informed of the fact.
	     */
	    if ((fd_tmp = open(SYSTTY, O_RDWR)) != -1)
	    {
		/* Make sure the system tty is not RAW. */
		stterm = dflt_termio;
		stterm.c_cflag = systtycf;
		ioctl(fd_tmp, TCSETA, &stterm);
		/*
		 * Make sure the file descriptor is greater than 2 so
		 * that it won't interfere with the standard
		 * descriptors.
		 */
		if (fd_tmp < 3)
                {
		    fd_systty = fcntl(fd_tmp, 0, 3);
		    close(fd_tmp);
		}
		else
		    fd_systty = fd_tmp;

		/*
		 * Prepare to catch the interupt signal if <del> typed
		 * at /dev/systty.
		 */
		{
		    struct sigaction action;

		    /*
		     * Block all signals during execution of a signal
		     * handler, with the exception of SIGPWR.
		     */
		    sigfillset(&action.sa_mask);
		    sigdelset(&action.sa_mask, SIGPWR);
		    action.sa_flags = 0;
		    action.sa_handler = switchcon;

		    sigaction(SIGINT, &action, NULL);
		    sigaction(SIGQUIT, &action, NULL);
		}
	    }
	}
#ifdef UDEBUG
	{
	    struct sigaction action;

	    /*
	     * Block all signals during execution of a signal
	     * handler, with the exception of SIGPWR.
	     */
	    sigfillset(&action.sa_mask);
	    sigdelset(&action.sa_mask, SIGPWR);
	    action.sa_flags = 0;
	    action.sa_handler = abort;

	    sigaction(SIGUSR1, &action, NULL);
	    sigaction(SIGUSR2, &action, NULL);
	}
#endif
	for (;;)
	{
	    /*
	     * Close the current descriptors and open ones to
	     * /dev/syscon.
	     */
	    opensyscon(O_NOCTTY);

	    /*
	     * Print something unimportant and pause, since reboot may
	     * be taking place over a line coming in over the
	     * dataswitch.  The dataswitch sometimes gets the carrier
	     * up before the connection is complete and the first write
	     * gets lost.
	     */
	    WRTSTR(1, "\n");
	    sleep(2);

	    /* Now read in the user response. */
	    for (c = '\0'; c != EOF;)
	    {
		/*
		 * If the user typed a <DEL> to switch the consoles
		 * and we have done that, let him know.
		 */
		if (!did_switch && fd_systty == -1)
		{
		    WRTSTR(1, "****	SYSCON CHANGED TO /dev/systty	****\n");
		    did_switch = TRUE;
		}

		WRTSTR(1, "ENTER RUN LEVEL (0-6,s, or S): ");

		/*
		 * Get a character from the user which isn't a space or
		 * tab.
		 */
		do
		{
		    if (read(0, &c, 1) == -1)
			c = EOF;
		    else
			c &= 0x7f;
		} while (c == ' ' || c == '\t');

		if (ioctl(0, TCGETA, &tminfo) != EOF)
		    ioctl(0, TCSETAF, &tminfo);

		/*
		 * If the character is a digit between 0 and 6 or the
		 * letter S, fine, exit with the level equal to the new
		 * desired state.
		 */
		if (c == 'S' || c == 's')
		    c = '7';
		if (c >= '0' && c <= '7')
		    exit(levels[c - '0']);
		if (c != EOF)
		    WRTSTR(1, "\nUsage: 0123456sS\n");
	    }
	}
    }

    /*
     * Parent.  Wait for the child to die and return it's status.
     */
    while ((status = waitproc(process)) == -1)
	continue;

    /* Ignore any signals such as powerfail when in "getlvl". */
    wakeup.w_mask = 0;

    /* Return the new run level to the caller. */
#ifdef DEBUG
    debug("getlvl: status: %o exit: %o termination: %o\n", status, (status & 0xff00) >> 8, (status & 0xff));
#endif

    return (status & 0xff00) >> 8;
}

/*************************/
/****    switchcon    ****/
/*************************/

void
switchcon(sig)
int sig;
{
    /*
     * If this is the first time a <del> has been typed on the
     * /dev/systty, then unlink /dev/syscon and relink it to
     * /dev/systty.  Also reestablish file pointers.
     */
    if (fd_systty != -1)
    {
	reset_syscon();
	opensyscon(O_NOCTTY);

	/*
	 * Set fd_systty to -1 so that we ignore any deletes from it in
	 * the future as far as relinking /dev/syscon to /dev/systty.
	 */
	fd_systty = -1;
    }
}

/*********************/
/****    efork    ****/
/*********************/

/*
 * efork() --
 *     fork a child.  The parent inserts the process in its table of
 *     processes that are directly a result of forks that it has
 *     performed.
 *     The child just changes the "global" with the process id for this
 *     process to it's new value.
 *
 *     If "efork" is called with a pointer into the proc_table it uses
 *     that slot, otherwise it searches for a free slot.
 *
 *     Whichever way it is called, it returns the pointer to the
 *     proc_table entry.
 */
proctbl_t *
efork(process, modes)
register proctbl_t *process;
int modes;
{
    pid_t childpid;
    int failcount = 0;	/* count the number of times fork() fails */

#ifdef DEBUG1
    debug("In efork()\n");
#endif

#ifdef TRUX
    /*
     * The following test for mode=-1 protects against the recursion:
     *
     *    remove->account->init_update_tty->sub_process->efork
     *        ->proc_cleanup->account->etc.
     *
     * sub_process calls efork with mode=-1 to avoid the proc_cleanup call.
     * In other words, just do the fork; don't clean up the proc table.
     * Note that "proclnup" flag in proc_cleanup is used to avoid a similar
     * recursion, but it does not fix the problem being fixed here. In fact,
     * I believe the mode=-1 fix used here is a superset of the proclnup fix,
     * which probably is not needed anymore.
     */
    if (ISSECURE) {
        if (modes != (int)-1)
            proc_cleanup();
        else
            modes=0;
    }
    else
        proc_cleanup();
#else
    proc_cleanup();
#endif TRUX

#ifdef DEBUG1
    debug("In efork(), after proc_cleanup()\n");
#endif
    while ((childpid = fork()) == -1)
    {
	if (++failcount >= 5) {
		perror("init: fork");
	}
#ifdef DEBUG1
        debug("In efork(), couldn't fork()\n");
#endif
	/*
	 * Shorten the alarm timer in case someone else's child dies
	 * and free up a slot in the process table.
	 * Then wait for some children to die.
	 */
	alarm(5);
	sigsuspend(&nosignals);
    }

    /*
     * This is the parent process
     */
    if (childpid != 0)
    {
#ifdef DEBUG1
        debug("In efork(), forked a child process\n");
#endif
	/*
	 * If a pointer into the process table was not specified, then
	 * get a new one.
	 */
	if (process == NULLPROC)
	    process = proc_alloc();

	/*
	 * Add this new entry to the table of processes (if not already
	 * there).
	 */
	if (process->p_flags & NEWENTRY)
	{
	    process->next = proc_table;
	    proc_table = process;
	}
	process->p_pid = childpid;
	process->p_flags = (LIVING|modes);

	return process;
    }

    /*
     * This is the child process
     */
    {
	struct sigaction action;
        int i;

	own_pid = getpid();     /* Reset child's concept of its pid. */

         /*
          * This is so /dev/tty works right
          */
	setsid();

	/*
	 * Reset the alarm and all signals to the system
         * defaults for child process.
	 */
	alarm(0);
	action.sa_handler = SIG_DFL;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	for (i = SIGHUP; i <= SIGPWR; i++)
	    sigaction(i, &action, NULL);

	/*
	 * Unblock all signals
	 */
	sigprocmask(SIG_SETMASK, &nosignals, NULL);

	return NULL;
    }
}
/*
 * waitproc() --
 *    "waitproc" waits for a specified process to die.  For this routine
 *    to work, the specified process must already in the proc_table.
 *    "waitproc" returns the exit status of the specified process when
 *    it dies.
 */
long
waitproc(process)
register proctbl_t *process;
{
    int answer;

#ifdef DEBUG1
    debug("In waitproc(), process is %s\n",
	  process->p_flags & LIVING ? "alive" : "DEAD");
#endif
    /* Wait around until the process dies. */
    if (process->p_flags & LIVING)
	sigsuspend(&nosignals);

#ifdef DEBUG1
    debug("In waitproc(), after sigsuspend() process is %s\n",
	  process->p_flags & LIVING ? "ALIVE" : "dead");
#endif
    if (process->p_flags & LIVING)
	return -1;

    /*
     * Make sure to only return 16 bits so that answer will always
     * be positive whenever the process of interest really died.
     */
    answer = (process->p_exit & 0xffff);

    /*
     * Free the slot in the proc_table.
     */
    proc_delete(process);

    return answer;
}

/**************************/
/****    opensyscon    ****/
/**************************/

/*
 * "opensyscon" opens stdin, stdout, and stderr, making sure
 * that their file descriptors are 0, 1, and 2, respectively.
 */
void
opensyscon(open_flags)
unsigned int open_flags;
{
    register FILE *fp;
    register int state;
    int fd;
#ifdef PTY_SUPP
    unsigned long oldalarm;
    struct sigaction action;
    struct sigaction old_action;
#endif

    /*
     * Close all of our open files so that we can open the
     * console as stdin, stdout and stderr.
     */
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
    close(0);
    close(1);
    close(2);
    endutent();
    if (getpid()!=1)	/* Don't want to be session leader if we're	*/
	setsid();	/* only printing a diagnostic message.		*/

#ifdef PTY_SUPP
    /*
     * Setup for timeout for the open (for ptys).
     * Block all signals during execution of a signal
     * handler, with the exception of SIGPWR.
     */
    sigfillset(&action.sa_mask);
    sigdelset(&action.sa_mask, SIGPWR);
    action.sa_flags = 0;
    action.sa_handler = reset_syscon;

    sigaction(SIGALRM, &action, &old_action);
    oldalarm = alarm(5);
#endif
    if ((fd = open(SYSCON, O_RDWR|open_flags)) == -1)
    {
	/*
	 * If the open fails, switch back to /dev/systty.
	 */
	if ((fd = open(SYSTTY, O_RDWR|open_flags)) == -1)
	    fd = open(CONSOLE, O_RDWR|open_flags);
    }
    if (fd == -1 || (fp = fdopen(fd, "r+")) == NULL)
	return;

#ifdef PTY_SUPP
    alarm(oldalarm);		/* reset alarm to previous conditions */
    sigaction(SIGALRM, &old_action, NULL);
#endif

    fdup(fp);
    fdup(fp);
    setbuf(fp, NULL);
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    /*
     * Make sure the hangup on last close is off.  Then restore
     * the modes that were on syscon when the signal was sent.
     */
    get_ioctl_syscon();		/* make sure get the right termio */
    termio.c_cflag &= ~HUPCL;
    fioctl(fp, TCSETA, &termio);
}

/*
 * get_ioctl_syscon() --
 *     retrieves the /dev/syscon settings from the file
 *     "/etc/ioctl.syscon".
 */
void
get_ioctl_syscon()
{
    register FILE *fp;
    int iflags, oflags, cflags, lflags, ldisc, cc[8], i;

    /*
     * Read in the previous modes for /dev/syscon from ioctl.syscon.
     */
    if ((fp = fopen(IOCTLSYSCON, "r")) == NULL)
	reset_syscon();
    else
    {
	i = fscanf(fp, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
		&iflags, &oflags, &cflags, &lflags, &ldisc,
		&cc[0], &cc[1], &cc[2], &cc[3],
		&cc[4], &cc[5], &cc[6], &cc[7]);

	/*
	 * If the file is formatted properly, use the values to
	 * initialize the console terminal condition.
	 */
	if (i == 13)
	{
	    termio.c_iflag = (ushort)iflags;
	    termio.c_oflag = (ushort)oflags;
	    termio.c_cflag = (ushort)cflags;
	    termio.c_lflag = (ushort)lflags;
	    termio.c_line = (char)ldisc;
	    for (i = 0; i < 8; i++)
		termio.c_cc[i] = (char)cc[i];
	}
	/*
	 * If the file is badly formatted, use the default
	 * settings.
	 */
	else
	    memcpy(&termio, &dflt_termio, sizeof dflt_termio);
	fclose(fp);
    }
}

/*
 * reset_syscon() --
 *    relinks /dev/syscon to /dev/systty and puts the default ioctl
 *    setting back into /etc/ioctl.syscon and the incore arrays.
 */
void
reset_syscon()
{
    register FILE *fp;
    unsigned short c_cflg;
    struct stat statcon, statstty;
    int consfd;

    unlink(SYSCON);
    link(SYSTTY, SYSCON);
#ifdef SecureWare
    if (ISSECURE)
    	init_data_file_mask();
    else
    	umask(~0644);
#else
    umask(~0644);
#endif
    fp = fopen(IOCTLSYSCON, "w");

    stat(CONSOLE, &statcon);
    stat(SYSTTY, &statstty);
    /*
     * The assumption is that the console has the baud rate setup by
     * the kernel correctly.  (i.e. match the physical setup of the
     * console).  If systty is linked to console, we get the baud rate
     * of console and use it as the default baud rate.  If systty is
     * not linked to console, we use the baud rate which is set up by
     * users through the sysinit entry in inittab
     */
    if (statcon.st_rdev == statstty.st_rdev)
    {
	consfd = open(CONSOLE, O_RDONLY|O_NOCTTY);
	if (ioctl(consfd, TCGETA, &termio) != EOF)
	    c_cflg = termio.c_cflag;
	close(consfd);
    }
    else
    {
	c_cflg = systtycf;
    }
    memcpy(&termio, &dflt_termio, sizeof (struct termio));
    termio.c_cflag = c_cflg;
    fprintf(fp, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
	    termio.c_iflag, termio.c_oflag, termio.c_cflag,
	    termio.c_lflag, termio.c_line,
	    termio.c_cc[0], termio.c_cc[1], termio.c_cc[2],
	    termio.c_cc[3], termio.c_cc[4], termio.c_cc[5],
	    termio.c_cc[6],
	    termio.c_cc[7]);
    fflush(fp);
    fclose(fp);
#ifdef SecureWare
    if (ISSECURE)
    	init_secure_mask();
    else
    	umask(0);
#else
    umask(0);
#endif
}


/*
 * save_ioctl() --
 *     Get the ioctl state of SYSCON and write it into IOCTLSYSCON.
 */
void
save_ioctl()
{
    register FILE *fp;
    register proctbl_t *process;
    int is_user_init = (own_pid != SPECIALPID);

    /*
     * If we are the child, open /dev/syscon, do an ioctl to get the
     * modes, and write them out to "/etc/ioctl.syscon".
     *
     * Don't fork a child if we are a user level init process.
     */
    if (is_user_init || (process = efork(NULLPROC, 0)) == NULLPROC)
    {
	if ((fp = fopen(SYSCON, "w")) == NULL)
	    console("Unable to open %s\n", SYSCON);
	else
	{
	    /*
	     * Turn off the HUPCL bit, so that carrier stays up after
	     * the shell is killed.
	     */
	    if (fioctl(fp, TCGETA, &termio) != -1)
	    {
		termio.c_cflag &= ~HUPCL;
		fioctl(fp, TCSETA, &termio);
	    }
	}
	fclose(fp);

	/*
	 * Write the state of "/dev/syscon" into "/etc/ioctl.syscon"
	 * so that it will be remembered across reboots.
	 */
#ifdef SecureWare
	if (ISSECURE)
	    init_data_file_mask();
	else
	    umask(~0644);
#else
	umask(~0644);
#endif
	if ((fp = fopen(IOCTLSYSCON, "w")) == NULL)
	    console("Can't open %s. errno: %d\n", IOCTLSYSCON, errno);
	else
	{
	    fprintf(fp, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
		    termio.c_iflag, termio.c_oflag, termio.c_cflag,
		    termio.c_lflag, termio.c_line, termio.c_cc[0],
		    termio.c_cc[1], termio.c_cc[2], termio.c_cc[3],
		    termio.c_cc[4], termio.c_cc[5], termio.c_cc[6],
		    termio.c_cc[7]);
	    fclose(fp);
	}
	if (is_user_init)
	    return;
	exit(0);
    }

    /*
     * If we are the parent, wait for the child to die.
     */
    while (waitproc(process) == -1)
	continue;
}


/*
 * msg() --
 *    Print a message to the system console without becoming the
 *    controlling process of the tty.
 *
 *    If is_error is true, the message is preceeded by "\nINIT: "
 *
 *    Note that the number of arguments passed to "console" is
 *    determined by the print format.
 */

msg(is_error, format, arg1, arg2, arg3, arg4)
int is_error;
char *format;
int arg1, arg2, arg3, arg4;
{
    register proctbl_t *process;
    char outbuf[BUFSIZ];

#ifdef DEBUG1
    debug("In console(), message:\n");
    debug(format, arg1, arg2, arg3, arg4);
#endif
    /* 
     * Since we have O_NOCTTY, we can print directly to the
     * console, even if we are process id 1.
     */
    opensyscon(O_NOCTTY);
    setbuf(stdout, outbuf);
    if (is_error)
	fprintf(stdout, "\nINIT: ");
    fprintf(stdout, format, arg1, arg2, arg3, arg4);
    fflush(stdout);
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
    close(0);
    close(1);
    close(2);
}

/*
 * console() --
 *   prints an error message on the console using msg()
 */
console(format, arg1, arg2, arg3, arg4)
char *format;
int arg1, arg2, arg3, arg4;
{
    msg(TRUE, format, arg1, arg2, arg3, arg4);
}

/*
 * timer() --
 *     a substitute for sleep() which uses alarm() and sigsuspend()
 */
timer(waitime)
register int waitime;
{
    setimer(waitime);
    while (!time_up)
	sigsuspend(&nosignals);
}

/*
 * setimer() --
 *     Flag set to TRUE by alarm interupt routine each time an alarm
 *     interupt takes place.
 */
int time_up;

setimer(timelimit)
int timelimit;
{
    time_up = FALSE;
    alarm(timelimit);
}

/************************/
/****    userinit    ****/
/************************/

/*
 * Routine to handle requests from users to main init running as
 * process 1.
 */
void
userinit(argc, argv)
int argc;
char *argv[];
{
    FILE *fp;
    char *ln;
    int init_signal;
    int saverr;
#ifdef PTY_SUPP
    struct stat statbuf;
#endif

    /*
     * We are a user invoked init.  Is there an argument and is it
     * a single character?  If not, print usage message and quit.
     */
    if (argc != 2 || *(*++argv + 1) != '\0')
    {
	fprintf(stderr, "Usage: init [0123456SsQqabc]\n");
	exit(0);
    }
    else
	switch (**argv)
	{
	case 'Q':
	case 'q':
	    init_signal = LVLQ;
	    break;
	case '0':
	    init_signal = LVL0;
	    break;
	case '1':
	    init_signal = LVL1;
	    break;
	case '2':
	    init_signal = LVL2;
	    break;
	case '3':
	    init_signal = LVL3;
	    break;
	case '4':
	    init_signal = LVL4;
	    break;
	case '5':
	    init_signal = LVL5;
	    break;
	case '6':
	    init_signal = LVL6;
	    break;

	/*
	 * If the request is to switch to single user mode, make sure
	 * that this process is talking to a legal teletype line and
	 * that /dev/syscon is linked to this line.
	 */
	case 'S':
	case 's':
	    ln = ttyname(0);	/* Get the name of tty */
	    if (*ln == '\0')
	    {
		fprintf(stderr, "Standard input not a tty line\n");
		exit(1);
	    }
#ifdef PTY_SUPP
	    fstat(0, &statbuf);
	    if (major(statbuf.st_rdev) == PTYMAJOR &&
		m_selcode(statbuf.st_rdev) == PTYSC)
	    {
		fprintf(stderr, "CAN'T CHANGE SYSCON TO A PTY ...\n");
		fprintf(stderr, "SYSCON CHANGED TO %s\n", SYSTTY);
		reset_syscon();
		/* Print message to console about relink */
		if ((fp = fopen(SYSTTY, "r+")) != NULL)
		{
		    fprintf(fp, "\n****	SYSCON CHANGED TO %s	****\n",
			SYSTTY);
		    fclose(fp);
		}
		init_signal = SINGLE_USER;
		break;
	    }
	    else
#endif
	    if (strcmp(ln, SYSCON) != 0)
	    {
		/*
		 * Leave a message if possible on system console saying
		 * where /dev/syscon is currently connected.
		 */
		if ((fp = fopen(SYSTTY, "r+")) != NULL)
		{
		    fprintf(fp, "\n****	SYSCON CHANGED TO %s	****\n",
			ln);
		    fclose(fp);
		}

		/*
		 * Unlink /dev/syscon and then relink it to the current
		 * line.
		 */
		if (unlink(SYSCON) == -1)
		{
		    perror("Can't unlink /dev/syscon\n");
		    exit(1);
		}
		if (link(ln, SYSCON) == -1)
		{
		    saverr = errno;
		    fprintf(stderr, "Can't link /dev/syscon to %s", ln);
		    errno = saverr;
		    perror(": ");
		    link(SYSTTY, SYSCON);   /* Try to leave a syscon */
		    exit(1);
		}
	    }
	    /*
	     * If the new level is SINGLE_USER, it is necessary to save
	     * the state of the terminal which is "syscon".  These will
	     * be restored before the "su" is started up on the line.
	     */
	    save_ioctl();
	    init_signal = SINGLE_USER;
	    break;
	case 'a':
	    init_signal = LVLa;
	    break;
	case 'b':
	    init_signal = LVLb;
	    break;
	case 'c':
	    init_signal = LVLc;
	    break;
	/*
	 * If the argument was invalid, print the usage message.
	 */
	default:
	    fputs("Usage: init [01234567SQabc]\n", stderr);
	    exit(1);
	}

    /*
     * Now send signal to main init and then exit.
     */
    if (kill(SPECIALPID, init_signal) == -1)
    {
	fputs("Could not send signal to \"init\".\n", stderr);
	exit(1);
    }
    else
	exit(0);
}

/*
 * idle() --
 *
 *     "idle" allows the system to coast to a stop after receiving
 *     SIGUSR1 from the reboot(1m) command, performing accounting
 *     on processes as they die as a result of reboot killing them.
 *     added 9/85 by rer.
 */

/* time allowed in idle before returning to normal op */
#define	REBOOT_GRACE	(unsigned long)60

void
idle(sig)
int sig;
{
    register proctbl_t *process;
    register pid_t pid;
    int status;
    sigset_t signals;

    /*
     * Close the utmp file, if it is open.  This is so that we don't
     * have any files open when the system halts (just to be safe).
     */
    endutent();

    pwr_sig = 0;			  /* reset powerfail flag */
    alarm(REBOOT_GRACE);

    /*
     * allow a SIGALRM signal
     */
    sigemptyset(&signals);
    sigaddset(&signals, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &signals, NULL);

    /*
     * Idle loop:
     * 	Keep idling until:
     *	   a)  we run out of children to wait on
     *	   b)  we receive a signal (other than SIGPWR)
     *	   c)  we get some other error from wait
     *
     * If we get a powerfail during idle, catch the signal but do
     * not process inittab's POWER_MODES entries until the idle
     * has completed.
     */
    for (;;)
    {
	pid = wait(&status);
	if (pid == -1)
	{
	    if (errno == EINTR && pwr_sig && wakeup.w_flags.w_powerhit)
		pwr_sig = 0;
	    else
		return;
	}

	if (sig == SIGUSR1)
	{
	    proctbl_t *prev = NULL;

	    for (process = proc_table; process;
				prev = process, process = process->next)
		if ((process->p_flags & LIVING) &&
		    process->p_pid == pid)
		{
		    process->p_flags &= ~LIVING;
		    process->p_exit &= status;
		    account(DEAD_PROCESS, process, NULL);
		    if (process == proc_table)
			proc_table = process->next;
		    else
			prev->next = process->next;
		    proc_free(process);
		    break;
		}
	}
    }
}
