/* @(#) $Revision: 72.7 $ */   
/*********************************************************************
 			          MAIN
 
  Revision 1.7  83/10/14  15:26:30  shel
  restore tty state when user hits an interrupt while in input CBREAK mode
  
  Revision 1.6  83/10/12  19:05:07  shel
  autologout features along with saving histories across logouts
  
  C Shell
 
   Bill Joy, UC Berkeley, California, USA
   October 1978, May 1980
  
   Jim Kulp, IIASA, Laxenburg, Austria
   April 1980
   
 ********************************************************************/

#include "sh.h"
#include <sys/ioctl.h>
#include <fcntl.h>

#ifdef SIGWINCH
#include <termio.h>
#endif /* SIGWINCH */

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#include <locale.h>
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#define NL_SETN 3	/* set number */
#endif NLS

#ifdef NLS16
#include <nl_ctype.h>
#endif

#ifndef NONLS	/* we want these arrays to be initialized as CHAR arrays */
CHAR	CH_pl1[] = {'.',0};
CHAR	CH_pl2[] = {'/','l','b','i','n',0};
CHAR	CH_pl3[] = {'/','b','i','n',0};
CHAR	CH_pl4[] = {'/','u','s','r','/','l','b','i','n',0};
CHAR	CH_pl5[] = {'/','u','s','r','/','b','i','n',0};

#ifdef V4FS
	CHAR	*pathlist[] =	{ CH_pl1, CH_pl2, CH_pl4, CH_pl5, 0 };
#else
	CHAR	*pathlist[] =	{ CH_pl1, CH_pl3, CH_pl5, 0 };
#endif 	/* V4FS */

CHAR	CH_dh2[] = {'-','h',0};
CHAR	*dumphist[] =	{ CH_history, CH_dh2, 0, 0 };
CHAR	CH_lh1[] = {'s','o','u','r','c','e',0};
CHAR	CH_lh2[] = {'-','h',0};
CHAR	CH_lh3[] = {'~','/','.','h','i','s','t','o','r','y',0};
CHAR	*loadhist[] =	{ CH_lh1, CH_lh2, CH_lh3, 0 };
#else
#  ifdef V4FS
	char 	*pathlist[] = { ".", "/lbin", "/usr/lbin", "/usr/bin", 0 };
#  else
	char 	*pathlist[] = { ".", "/bin", "/usr/bin", 0 };
#  endif	/* V4FS */

char	*dumphist[] = { "history", "-h", 0, 0 };
char	*loadhist[] = { "source", "-h", "~/.history", 0 };
#endif

CHAR	HIST = '!';
CHAR 	HISTSUB = '^';
bool	nofile;
bool	reenter;
bool	nverbose;
bool	nexececho;
bool	quitit;
bool	fast;
bool	prompt = 1;
bool	enterhist = 0;

#ifndef NONLS
CHAR	default_autologout[] = {'6','0',0};	/* 1 Hour Alarm default */
#define DEFAULT_AUTOLOGOUT	default_autologout 
#else
#define DEFAULT_AUTOLOGOUT	"60"
#endif

/*  This routine isn't called by any other.
*/
/**********************************************************************/
void
auto_logout ()
/**********************************************************************/
{
    setup_tty (0); /* Restore terminal before auto_logout */
    printf ((catgets(nlmsg_fd,NL_SETN,1, "auto-logout\n")));
    close (SHIN);
    child++;
    goodbye ();
}


#ifdef SIGWINCH
/*
 *  This is the signal handler for SIGWINCH.  It will check to see
 *  if the size of the window has changed, and if so, it will update
 *  the LINES and COLUMNS variables.
 */
struct winsize window_size;

winch_handler()
{
    struct winsize new_size;

    if(ioctl(FSHDIAG, TIOCGWINSZ, &new_size) < 0)
	return(-1);

    window_change(new_size.ws_col, new_size.ws_row);

}

window_change(col, row)
int col, row;
{
    if(col != window_size.ws_col)
    {
	/*
	 * The number of columns changed.  Save the new
	 * size and update COLUMNS.
	 */

	CHAR *tmp;

	window_size.ws_col = col;
	/* putn returns calloc'ed space, so free it before we return */
	tmp = putn(window_size.ws_col);
	setenv(CH_columns, tmp);
	xfree(tmp);
    }
    if(row != window_size.ws_row)
    {
	/*
	 * The number of columns changed.  Save the new
	 * size and update LINES.
	 */

	CHAR *tmp;
	
	window_size.ws_row = row;
	/* putn returns calloc'ed space, so free it before we return */
	tmp = putn(window_size.ws_row);
	setenv(CH_lines, tmp);
	xfree(tmp);
    }
}
#endif /* SIGWINCH */

#ifndef NONLS	/* more CHAR string constants for 8 bit processing */
CHAR	CH_tmpsh[] = {'/','t','m','p','/','s','h',0};
CHAR	CH_pound_sign[] = {'#',' ',0};
CHAR	CH_percent_sign[] = {'%',' ',0};
#else
#ifdef V4FS
#define CH_tmpsh 	"/var/tmp/sh"
#else	/* Not V4FS */
#define CH_tmpsh 	"/tmp/sh"
#endif	/* V4FS */
#define CH_pound_sign	"# "
#define	CH_percent_sign	"% "
#endif

/**********************************************************************/
main(c, av)
	int c;
	char **av;
/**********************************************************************/
{
	register CHAR **v, *cp;
	register CHAR *p;
	register CHAR *noHome;
	register char *q;
	char **v0, *cp0;

#ifdef SIGTSTP
	register int f;
#endif
	extern void (*sigset())();
	extern void (*sigsys())();

#ifdef SPIN_DEBUG
/*  Debug:  This loop can be used to cause the shell to hang so that the
	    process can be adopted by a debugger such as xdb.  The infinite
	    loop is broken when spinDebug takes on a value other than 0.
*/
  int spinDebug = 0;

  while (spinDebug == 0)
    ;
#endif

/*  All the NLS defines are used to open the message catalog, the setlocale
    initializes to the language.
*/
#ifdef NLS
	/* open message catalog file */
	nlmsg_fd=catopen("csh",0);
	if (nlmsg_fd != -1)
		fcntl(nlmsg_fd, F_SETFD, 1);	/* set close-on-exec flag */
#endif

#if defined NLS || defined NLS16	/* initialize to the current language */
	if (!setlocale(LC_ALL, ""))
		write(2, _errlocale("csh"), strlen(_errlocale("csh")));
#endif NLS || NLS16

#ifdef SIGWINCH				/* set LINES and COLUMNS */
	{
	struct winsize ws;

	if(ioctl(0, TIOCGWINSZ, &ws) < 0)
	     ;
	else window_change(ws.ws_col, ws.ws_row);

	}
#endif SIGWINCH
	settimes();			/* Immed. estab. timing base */

/*  The variable paraml is a global, defined in sh.h of type wordent.  Here it 
    is intialized to itself.  Its value is is a global NULL string (nullstr).
*/
	paraml.next = paraml.prev = &paraml;
	paraml.word = nullstr;    

/*  The variable av corresponds to the argv.
*/
	v0 = av;
	if (!strcmp(v0[0], "a.out"))	/* A.out's are quittable */
		quitit = 1;

/*  The variable uid is a global.
*/
	uid = getuid();
	setintr=0;

/*  The variable loginsh is a global flag and is set to '1' if v0[0] is a 
    '-', meaining that this is a login shell.
*/
	loginsh = **v0 == '-';

/*  The variable tenexflag is a global defined in sh.h and is used in the
    tenex routine.  The variable pjobs is a global defined in sh.h and is
    used in the pintr1 routine.
*/
	tenexflag = 1;
	pjobs = 0;

/*  The variable chktim is a global.  I think it is used for checking mail.
*/
        /* only checktime if csh is a login shell */
	if (loginsh)
		time(&chktim);

/*  This routine copies file descriptors 0, 1, 2, and 16 and closes all others.
*/
	/*
	 * Move the descriptors to safe places.
	 * The variable didfds is 0 while we have only FSH* to work with.
	 * When didfds is true, we have 0,1,2 and prefer to use these.
	 */
	initdesc();

	/*
	 * Initialize the shell variables.
	 * ARGV and PROMPT are initialized later.
	 * STATUS is also munged in several places.
	 * CHILD is munged when forking/waiting
	 */

	set(CH_autologout, DEFAULT_AUTOLOGOUT);
	sigset (SIGALRM, auto_logout);
	set(CH_status, CH_zero);

/*  No bug report on this one, but...

    The shell tries to get HOME from the environment.  If this fails, getenv
    returns 0, and so does to_short.  Initially this value was then being
    used in savestr and then being assigned to cp.  However, even if the value
    passed to it was 0, savestr would just save the NULL string, so cp would
    always get a value.  Then cp was being checked for NOSTR and this could 
    never occur.  
    
    So an intermediate value was used (noHome) to get the outcome of the getenv
    and to_short.  This is the value used to check against NOSTR and flag the 
    fact that scripts can't be read.  This affects .login and .cshrc scripts.

    Original lines:

	dinit(cp = savestr(to_short(getenv("HOME"))));	
	if (cp == NOSTR)
*/

/* Fix for dts DSDe417680:
   setexit() is called before set() 'cos if globbing chars are
   present and if globbing fails, a longjmp() is done when set() or
   set1() is called.
*/
	if ( !setexit()) {
		noHome = to_short (getenv ("HOME"));
		dinit(cp = savestr (noHome));	
		if (noHome == NOSTR)
			fast++;		/* No home -> can't read scripts */

		set(CH_home, cp);
	}
	/*
	 * Grab other useful things from the environment.
	 * Should we grab everything??
	 */

	if ( !setexit()) {
		if ((cp = to_short(getenv("USER"))) != NOSTR)
			set(CH_user, savestr(cp));
	}

	if ( !setexit()) {
		if ((cp = to_short(getenv("TERM"))) != NOSTR)
			set(CH_term, savestr(cp));
	}
/*  It looks like shvhed is the head of a linked list of shell variables, but
    it is never initialized.
*/
	/*
	 * Re-initialize path if set in environment
	 */

/* Fix for dts DSDe417680:
   setexit() is called before set() 'cos if globbing chars are
   present and if globbing fails, a longjmp() is done when set() or
   set1() is called.
*/
	if ( !setexit()) {
		if ((cp = to_short(getenv("PATH"))) == NOSTR)
			set1(CH_path, saveblk(pathlist), &shvhed);
		else {
			register unsigned i = 0;
			register CHAR *dp;
			register CHAR **pv;

			for (dp = cp; *dp; dp++)
				if (*dp == ':')
					i++;
			pv = (CHAR **)calloc(i+2, sizeof (CHAR **));
			for (dp = cp, i = 0; ;)
				if (*dp == ':') {
					*dp = 0;
					pv[i++] = savestr(*cp ? cp : CH_dot);
					*dp++ = ':';
					cp = dp;
				} else if (*dp++ == 0) {
					pv[i++] = savestr(*cp ? cp : CH_dot);
					break;
				}
			pv[i] = 0;
			set1(CH_path, pv, &shvhed);
		}
		set(CH_shell, savestr(to_short(SHELLPATH)));
	}
	doldol = putn(getpid());		/* For $$ */
	shtemp = Strspl(CH_tmpsh, doldol);	/* For << */

	/*
	 * Record the interrupt states from the parent process.
	 * If the parent is non-interruptible our hand must be forced
	 * or we (and our children) won't be either.
	 * Our children inherit termination from our parent.
	 * We catch it only if we are the login shell.
	 */

/*  The routine sigset returns the previous action.  So this code gets that
    action and sets the action to SIG_IGN.  Then it resets the action to
    whatever it originally was.  Since all that the shell was looking for 
    was the current action, why not use the sigaction routine to find out what 
    that action is?  
*/
	parintr = sigset(SIGINT, SIG_IGN);      /* parents interruptibility */
	sigset(SIGINT, parintr);
	parterm = sigset(SIGTERM, SIG_IGN);     /* parents terminability */
	sigset(SIGTERM, parterm);
	/*
	 * Process the arguments.
	 *
	 * Note that processing of -v/-x is actually delayed till after
	 * script processing.
	 *
	 * We set the first character of our name to be '-' if we are
	 * a shell running interruptible commands.  Many programs which
	 * examine ps'es use this to filter such shells out.
	 */
	c--, v0++;
	while (c > 0 && (cp0 = v0[0])[0] == '-') {
		do switch (*cp0++) {

/*  This case must correspond to no options?!?  As is when I type in: csh
    Then the shell is interruptible and there is no shell script.
*/
		case 0:			/* -	Interruptible, no prompt */
			prompt = 0;
			setintr++;
			nofile++;
			break;

		case 'c':		/* -c	Command input from arg */
			if (c == 1)
			  {

#ifdef DEBUG_EXIT
  printf ("main (1): %d, -c1: calling exit (0)\n", getpid ());
#endif
				exit(0);
			  }
			c--, v0++;
			/*
			 * We cannot use to_short() here to convert the
			 * argument, because it might be logner than the
			 * limit of to_short().
			 */
			arginp = calloc((unsigned)strlen(v0[0])+1, sizeof(CHAR));
			p = arginp;
			q = *v0;
#ifndef NLS16
			while (*p++ = *q++ & 0377);
#else

#ifdef EUC
			while (*p++ = _CHARADV(q) & TRIM);
#else  EUC
			while (*p++ = CHARADV(q) & TRIM);
#endif EUC

#endif
			prompt = 0;
			nofile++;
			break;

		case 'e':		/* -e	Exit on any error */
			exiterr++;
			break;

		case 'f':		/* -f	Fast start */
			fast++;
			break;

		case 'i':		/* -i	Interactive, even if !intty */
			intact++;
			nofile++;
			break;

		case 'n':		/* -n	Don't execute */
			noexec++;
			break;

		case 'q':		/* -q	(Undoc'd) ... die on quit */
			quitit = 1;
			break;

		case 's':		/* -s	Read from std input */
			nofile++;
			break;

		case 't':		/* -t	Read one line from input */
			onelflg = 2;
			prompt = 0;
			nofile++;
			break;

		case 'v':		/* -v	Echo hist expanded input */
			nverbose = 1;			/* ... later */
			break;

		case 'x':		/* -x	Echo just before execution */
			nexececho = 1;			/* ... later */
			break;

		case 'T':               /* -T   Disable Tenex features */
			tenexflag = 0;
			break;

		case 'V':		/* -V	Echo hist expanded input */
			setNS(CH_verbose);		/* NOW! */
			break;

		case 'X':		/* -X	Echo just before execution */
			setNS(CH_echo);			/* NOW! */
			break;

		} while (*cp0);
		v0++, c--;
	}

	if (quitit)		/* With all due haste, for debugging */
		sigset(SIGQUIT, SIG_DFL);

	/*
	 * Unless prevented by -, -c, -i, -s, or -t, if there
	 * are remaining arguments the first of them is the name
	 * of a shell file from which to read commands.
	 */
	if (nofile == 0 && c > 0) {
		nofile = open(v0[0], 0);
		if (nofile < 0) {
			child++;		/* So this ... */
			Perror(v0[0]);		/* ... doesn't return */
		}
		file = savestr(to_short(v0[0]));
		SHIN = dmove(nofile, FSHIN);	/* Replace FSHIN */
		prompt = 0;
		c--, v0++;

#ifdef TRACE_DEBUG
  printf ("main (1): pid: %d, There is a script.\n", getpid ());
#endif
	}
	/*
	 * Consider input a tty if it really is or we are interactive.
	 */
	intty = intact || isatty(SHIN);
	/*
	 * Decide whether we should play with signals or not.
	 * If we are explicitly told (via -i, or -) or we are a login
	 * shell (arg0 starts with -) or the input and output are both
	 * the ttys("csh", or "csh</dev/ttyx>/dev/ttyx")
	 * Note that in only the login shell is it likely that parent
	 * may have set signals to be ignored
	 */
/*  Also note that setintr may have already been set from the argument
    parsing.  (No arguments.)
*/
	if (loginsh || intact || intty && isatty(SHOUT))
	{
		setintr = 1;

#ifdef TRACE_DEBUG
  printf ("main (2): pid: %d, This is an interruptable shell.n", getpid ());
#endif

	}
#ifdef TELL
	settell();
#endif

	v0[c] = 0;
	v = blk_to_short(v0);

	/*
	 * Save the remaining arguments in argv.
	 */
	setq(CH_argv, v, &shvhed);

	/*
	 * Set up the prompt.
	 */
	if (prompt)
		set(CH_prompt, uid == 0 ? CH_pound_sign : CH_percent_sign);

	/*
	 * If we are an interactive shell, then start fiddling
	 * with the signals; this is a tricky game.
	 */
#ifdef SIGTSTP        	/* only used when SIGTSTP is implemented.--hn */
/*  This returns the process group of the calling process.
*/
	shpgrp = getpgrp(0);
#endif

/*  This is the terminal process group initialization.  
*/
	opgrp = tpgrp = -1;
	if (setintr) {

#ifdef TRACE_DEBUG
  printf ("main (3): pid: %d, Changing signals.\n", getpid ());
#endif

		**av = '-';
		if (!quitit)		/* Wary! */
		  {
			sigset(SIGQUIT, SIG_IGN);
#ifdef TRACE_DEBUG
  printf ("main (4): pid: %d, Changed SIGQUIT to SIG_IGN\n", getpid ());
#endif

		  }

#ifdef DEBUG_SIGNAL
  printf ("main (5): pid: %d, Setting SIGINT to pintr: %ol\n", getpid (),
	  pintr);
#endif

		sigset(SIGINT, pintr);

		sighold(SIGINT);
		sigset(SIGTERM, SIG_IGN);

#ifdef TRACE_DEBUG
  printf ("main (5): pid: %d, Changed SIGINT to pintr: %X\n", getpid (), pintr);
  printf ("\t\tSIGTERM to SIG_IGN\n");
#endif

#ifdef SIGTSTP
		if (quitit == 0 && arginp == 0) {
			sigset(SIGTSTP, SIG_IGN);
			sigset(SIGTTIN, SIG_IGN);
			sigset(SIGTTOU, SIG_IGN);
#ifdef TRACE_DEBUG
  printf ("main (6): pid: %d, Changed SIGTSTP, SIGTTIN, SIGTTOU to SIG_IGN\n");
#endif
			/*
			 * Wait till in foreground, in case someone
			 * stupidly runs
			 *	csh &
			 * dont want to try to grab away the tty.
			 */
			if (isatty(FSHDIAG))
				f = FSHDIAG;
			else if (isatty(FSHOUT))
				f = FSHOUT;
			else if (isatty(OLDSTD))
				f = OLDSTD;
			else
				f = -1;

/*  This returns the foreground process group associated with the terminal
    into tpgrp.  This is checked against the process group of the shell.  The
    two should be the same, or tpgrp should be -1, in which case it gets set
    to the process group of the shell.
*/
retry:
			if (ioctl(f, TIOCGPGRP, &tpgrp) == 0 && tpgrp != -1) {
				if (tpgrp != shpgrp) {
				  void (*old)() = sigsys(SIGTTIN, SIG_DFL);
				  kill(0, SIGTTIN);
				  sigsys(SIGTTIN, old);
				  goto retry;
				}
				opgrp = shpgrp;
				shpgrp = getpid();
				tpgrp = shpgrp;
				setpgrp(0, shpgrp);
				ioctl(f, TIOCSPGRP, &shpgrp);
#ifdef TRACE_DEBUG
  printf ("main (7): pid: %d, Set tpgrp: %d\n", getpid (), tpgrp);
#endif

				(void) dcopy(f, FSHTTY);
				fcntl(FSHTTY, F_SETFD, 1);
			} else {
notty:
  printf((catgets(nlmsg_fd,NL_SETN,2, "Warning: no access to tty; thus no job control in this shell...\n")));
				tpgrp = -1;
			}
		}
#endif
	}

	sigset(SIGCLD, pchild);		/* while signals not ready */

	/*
	 * The following signal handler is needed so that the
	 * child can signal to the parent executing a  csh script 
	 * to set doneinp.
	 */
	sighold(SIGUSR2);
	sigset(SIGUSR2, psigusr2);

#ifdef SIGWINCH
	/*
	 *  Setup to catch SIGWINCH so that the shell can update
	 *  the LINES and COLUMNS variables.
	 */
	sigset(SIGWINCH, winch_handler);
#endif /* SIGWINCH */

	/*
	 * Set an exit here in case of an interrupt or error reading
	 * the shell start-up scripts.
	 */
/*  A longjmp can return to this location if another setexit () is not done
    before the longjmp occurs.  This routine is a macro for 'setjmp (reslab)'
    which is defined in sh.h.
*/
	setexit();
	haderr = 0;		/* In case second time through */

/*  This code sources the startup files.  If the shell is a login shell,
    /etc/csh.login gets done first, then $HOME/.cshrc, then $HOME/.login.
    If the shell is not a login shell, only $HOME/.cshrc is done.  The $HOME
    must be set if we get into the code since the variable 'fast' is being
    checked.  This was set near the beginning of main if $HOME could not be
    obtained from the environment.
*/
	if (!fast && reenter == 0) {
		reenter++;
		if (loginsh) {
			srccat("/etc","/csh.login");
		}
		/* Will have value(home) here because set fast if don't */
		srccat(to_char(value(CH_home)), "/.cshrc");
		if (!fast && !arginp && !onelflg)
			dohash();
		if (loginsh) {
			srccat(to_char(value(CH_home)), "/.login");
		}
#ifdef TRACE_DEBUG
  printf ("main (8): pid: %d, Loadhist: %s\n", getpid (), *loadhist);
#endif
		dosource(loadhist);
	}

	/*
	 * Now are ready for the -v and -x flags
	 */
	if (nverbose)
		setNS(CH_verbose);
	if (nexececho)
		setNS(CH_echo);

	/*
	 * All the rest of the world is inside this call.
	 * The argument to process indicates whether it should
	 * catch "error unwinds".  Thus if we are a interactive shell
	 * our call here will never return by being blown past on an error.
	 */

	process(setintr);

	/*
	 * Mop-up.
	 */
	if (loginsh) {
		printf((catgets(nlmsg_fd,NL_SETN,3, "logout\n")));
		close(SHIN);
		child++;
		goodbye();
	}
	rechist();
	exitstat();
}

/*  Called by:

	exit ()
	texec ()
	dologin ()
	donewgrp ()
	dosuspend()
*/
/**********************************************************************/
untty()
/**********************************************************************/
{
#ifdef SIGTSTP
	if (tpgrp > 0) {
		setpgrp(0, opgrp);
		ioctl(FSHTTY, TIOCSPGRP, &opgrp);
	}
#endif
}

/*  Called by: 
	
	dosetenv ()
*/
/**********************************************************************/
importpath(cp)
CHAR *cp;
/**********************************************************************/
{
	register int i = 0;
	register CHAR *dp;
	register CHAR **pv;
	int c;
	static CHAR dot[2] = {'.', 0};

	for (dp = cp; *dp; dp++)
		if (*dp == ':')
			i++;
	/*
	 * i+2 where i is the number of colons in the path.
	 * There are i+1 directories in the path plus we need
	 * room for a zero terminator.
	 */
	pv = (CHAR **) calloc( (unsigned)(i+2), sizeof (CHAR **));
	dp = cp;
	i = 0;
	if (*dp)
	for (;;) {
		if ((c = *dp) == ':' || c == 0) {
			*dp = 0;
			pv[i++] = savestr(*cp ? cp : dot);
			if (c) {
				cp = dp + 1;
				*dp = ':';
			} else
				break;
		}
		dp++;
	}
	pv[i] = 0;
	set1(CH_path, pv, &shvhed);
}

/*  Called by:
	
	main ()
	goodbye ()
*/
/*
 * Source to the file which is the catenation of the argument names.
 */
/**********************************************************************/
srccat(cp, dp)
	char *cp, *dp;
/**********************************************************************/
{
	register char *ep = strspl(cp, dp);
	register int unit = dmove(open(ep, 0), -1);

	/* ioctl(unit, FIOCLEX, NULL); */
	xfree((CHAR *)ep);
#ifdef INGRES
	srcunit(unit, 0, 0);
#else
	srcunit(unit, 0, 0);
#endif
}

/*  Called by:
	
	srccat ()
	dosource ()
*/
/*
 * Source to a unit.  If onlyown it must be our file or our group or
 * we don't chance it.	This occurs on ".cshrc"s and the like.
 */
/**********************************************************************/
srcunit(unit, onlyown, hflg)
	register int unit;
	bool onlyown;
	bool hflg;
/**********************************************************************/
{
	/* We have to push down a lot of state here 
	 * All this could go into a structure 
	 */

	int oSHIN = -1, oldintty = intty;
	struct whyle *oldwhyl = whyles;
	CHAR *ogointr = gointr, *oarginp = arginp;
	CHAR *oevalp = evalp, **oevalvec = evalvec;
	int oonelflg = onelflg;
	bool oenterhist = enterhist;
	CHAR OHIST = HIST;

#ifdef TELL
	bool otell = cantell;
#endif
	struct Bin saveB;
	/* The (few) real local variables */
	jmp_buf oldexit;
	int reenter;

	if (unit < 0)
		return;
	if (didfds)
		donefds();
	if (onlyown) {
		struct stat stb;

		if (fstat(unit, &stb) < 0 || (stb.st_uid != uid && stb.st_gid != getgid())) {
			close(unit);
			return;
		}
	}

	/*
	 * There is a critical section here while we are pushing down the
	 * input stream since we have stuff in different structures.
	 * If we weren't careful an interrupt could corrupt SHIN's Bin
	 * structure and kill the shell.
	 *
	 * We could avoid the critical region by grouping all the stuff
	 * in a single structure and pointing at it to move it all at
	 * once.  This is less efficient globally on many variable references
	 * however.
	 */
/*  Save the old environment for a longjmp.
*/
	getexit(oldexit);
	reenter = 0;
	if (setintr)
		sighold(SIGINT);

/*  This section saves the Bin structure B.  That structure is defined in sh.h
    and contains the seek pointer, buffer pointers, and buffers for input.
    It then reinitializes them.  The variables fseekp, feobp, fblocks, and fbuf
    are all defined to point to items in the B structure.
*/
	/* Setup the new values of the state stuff saved above */
#ifndef NONLS
	/* since copy now works on CHAR, declare new routine to */
	/* copy buffers of bytes */
	b_copy((char *)&saveB, (char *)&B, sizeof saveB);
#else
	copy((char *)&saveB, (char *)&B, sizeof saveB);
#endif
	fbuf = (char **) 0;
	fseekp = feobp = fblocks = 0;
	oSHIN = SHIN, SHIN = unit, arginp = 0, onelflg = 0;
	intty = isatty(SHIN), whyles = 0, gointr = 0;
	evalvec = 0; evalp = 0;
	enterhist = hflg;
	if (enterhist)
		HIST = '\0';

/*  If this works, then a longjmp can return to this point provided another
    setjmp has not occured before the longjmp.
*/
	reenter = (setexit() != 0) + 1;
	if (reenter == 1) {
		/*
		 * Now if we are allowing commands to be interrupted,
		 * we let ourselves be interrupted.
		 */
		if (setintr)
			sigrelse(SIGINT);
#ifdef TELL
		settell();
#endif
		process(0);		/* 0 -> blow away on errors */
	}
	if (setintr)
		sigrelse(SIGINT);
	if (oSHIN >= 0) {
		register int i;

/*  Free up the new B structure and restore the old one.
*/
		/* We made it to the new state... free up its storage */
		/* This code could get run twice but xfree doesn't care */
		for (i = 0; i < fblocks; i++)
			xfree((CHAR *)fbuf[i]);
		xfree((CHAR *)fbuf);

		/* Reset input arena */
#ifndef NONLS
		/* since copy now works on CHAR, declare new routine to */
		/* copy buffers of bytes */
		b_copy((char *)&B, (char *)&saveB, sizeof B);
#else
		copy((char *)&B, (char *)&saveB, sizeof B);
#endif
		close(SHIN), SHIN = oSHIN;
		arginp = oarginp, onelflg = oonelflg;
		evalp = oevalp, evalvec = oevalvec;
		intty = oldintty, whyles = oldwhyl, gointr = ogointr;
		if (enterhist)
			HIST = OHIST;
		enterhist = oenterhist;
#ifdef TELL
		cantell = otell;
#endif
	}

/*  Reset the previous setjmp envrionment.
*/
	resexit(oldexit);
	/*
	 * If process reset() (effectively an unwind) then
	 * we must also unwind.
	 */
	if (reenter >= 2)
		error((char*) 0);
}

#ifndef NONLS
CHAR CH_slash_hist[] = {'/','.','h','i','s','t','o','r','y',0};
#else
#define CH_slash_hist	"/.history"
#endif

/*  Called by:
	
	main ()
	goodbye ()
*/
/**********************************************************************/
rechist()
/**********************************************************************/
{
	CHAR buf[BUFSIZ];
	int fp, ftmp, oldidfds;

	if (!fast) {
		if (value(CH_savehist)[0] == '\0')
			return;
		Strcpy(buf, value(CH_home));
		Strcat(buf, CH_slash_hist);
		fp = creat(to_char(buf), 0777);
		if (fp == -1)
			return;
		oldidfds = didfds;
		didfds = 0;
		ftmp = SHOUT;
		SHOUT = fp;
		Strcpy(buf, value(CH_savehist));
		dumphist[2] = (buf);
		dohist(dumphist);
		close(fp);
		SHOUT = ftmp;
		didfds = oldidfds;
	}
}

/*  Called by:

	auto_logout ()
	main ()
	dologout ()
*/
/**********************************************************************/
goodbye()
/**********************************************************************/
{

	if (loginsh) {
		sigset(SIGQUIT, SIG_IGN);
		sigset(SIGINT, SIG_IGN);
		sigset(SIGTERM, SIG_IGN);
		setintr = 0;		/* No interrupts after "logout" */
		if (adrof(CH_home))
			srccat(to_char(value(CH_home)), "/.logout");
	}
	rechist();
	exitstat();
}

/*  Called by:

	main ()
	goodbye ()
	exp6 ()
	backeval ()
	readc ()
	pjwait ()
	execute ()
*/
/**********************************************************************/
exitstat()
/**********************************************************************/
{
	/*
	 * Note that if STATUS is corrupted (i.e. getn bombs)
	 * then error will exit directly because we poke child here.
	 * Otherwise we might continue unwarrantedly (sic).
	 */
	child++;

#ifdef DEBUG_EXIT
  printf ("exitstat (1): %d, Calling exit with status\n", getpid ());
#endif

	exit(getn(value(CH_status)));
}

#ifndef NONLS
CHAR	CH_ja1[] = {'j','o','b','s',0};
CHAR	*jobargv[2] = { CH_ja1, 0 };
#else
char 	*jobargv[2] = { "jobs", 0 };
#endif

/*  This routine is not called directly by any other routine.  It is used
    as a signal handler for SIG_INT.
*/
/*
 * Catch an interrupt, e.g. during lexical input.
 * If we are an interactive shell, we reset the interrupt catch
 * immediately.  In any case we drain the shell output,
 * and finally go through the normal error mechanism, which
 * gets a chance to make the shell go away.
 */
/**********************************************************************/
void
pintr()
/**********************************************************************/
{
#ifdef DEBUG_SIGNAL
  printf ("pintr (1): %d, In pintr\n", getpid ());
#endif

	pintr1(1);
}

/*  Called by:

	pintr ()
	pjwait ()
*/
/**********************************************************************/
pintr1(wantnl)
	bool wantnl;
/**********************************************************************/
{
	register CHAR **v;
	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (1): %d, In pintr1\n", getpid ());
#endif

	if (setintr) {
	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (2): %d, Releasing SIGINT\n", getpid ());
#endif

		sigrelse(SIGINT);
		if (pjobs) {
			pjobs = 0;
			printf("\n");
			dojobs(jobargv);
	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (3): %d, Calling bferr\n", getpid ());
#endif

			bferr((catgets(nlmsg_fd,NL_SETN,4, "Interrupted")));
		}
	}
	if (setintr)
	  {
	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (4): %d, holding SIGINT\n", getpid ());
#endif

		sighold(SIGINT);
	  }

	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (5): %d, releasing SIGCLD\n", getpid ());
#endif

	sigrelse(SIGCLD);
	if (intty) /* if we are connected to tty reset ioctl state */
	  {
	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (6): %d, calling setup_tty\n", getpid ());
#endif

	    setup_tty(0);
	  }

	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (7): %d, calling draino\n", getpid ());
#endif

	draino();

	/*
	 * If we have an active "onintr" then we search for the label.
	 * Note that if one does "onintr -" then we shan't be interruptible
	 * so we needn't worry about that here.
	 */
	if (gointr) {

	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (8): %d, have an active onintr\n", getpid ());
#endif

/*  It looks like search seeks to the beginning of the script and then reads
    till it finds the label stored in gointr, then a longjmp occurs and 
    processing restarts at the label.  It is the case that if an onintr -
    was seen, the routine doonintr set SIG_INT to SIG_IGN.
*/

#ifdef DEBUG_SEARCH
  printf ("pintr1 (9): %d, calling search with ZGOTO\n", getpid ());
#endif
		search(ZGOTO, 0, gointr);
		timflg = 0;
		if (v = pargv)
			pargv = 0, blkfree(v);
		if (v = gargv)
			gargv = 0, blkfree(v);

/*  This is defined as longjmp (reslab).  So it causes the program to continue
    at the most recent setjmp (setexit()).  
*/
		reset();
	} else if (intty && wantnl)
		printf("\n");		/* Some like this, others don't */

/*  Ends up resetting the input buffer via btoeof (), freeing memory for
    the 'whyle' linked list and input buffers, then does a longjmp to restore
    to a previous stack.  It looks like the longjmp goes back to process ().
*/
	
#ifdef DEBUG_SIGNAL
  printf ("pintr1 (10): %d, calling error (0)\n", getpid ());
#endif

	error((char *) 0);
}

/*  Called by:

	main ()
	srcunit ()
	doeval ()
*/
/*
 * Process is the main driving routine for the shell.
 * It runs all command processing, except for those within { ... }
 * in expressions (which is run by a routine evalav in sh.exp.c which
 * is a stripped down process), and `...` evaluation which is run
 * also by a subset of this code in sh.glob.c in the routine backeval.
 *
 * The code here is a little strange because part of it is interruptible
 * and hence freeing of structures appears to occur when none is necessary
 * if this is ignored.
 *
 * Note that if catch is not set then we will unwind on any error.
 * If an end-of-file occurs, we return.
 */
/**********************************************************************/
process(catch)
	bool catch;
/**********************************************************************/
{
	register CHAR *cp;
	jmp_buf osetexit;

/*  This is used as a parse tree.
*/
	struct command *t;

/*  Save the current setjmp environment.
*/
	getexit(osetexit);
	for (;;) {
		pendjob();

/*  It look like if the next pointer is not pointing to the same wordent
    structure, then the string is freed and the pointers are set to point
    to the structure.
*/
		if (paraml.next != &paraml) {
			freelex(&paraml);
			paraml.next = paraml.prev = &paraml;
			paraml.word = nullstr;         
		}
		t = 0;

/*  A longjmp can come back to this point provided that an intervening setjmp
    has not occurred.
*/
		setexit();
		justpr = enterhist;	/* execute if not entering history */

		/*
		 * Interruptible during interactive reads
		 */
		if (setintr)
			sigrelse(SIGINT);

/*  Free the string in paraml.
*/
		/*
		 * For the sake of reset()
		 */
		freelex(&paraml), freesyn(t), t = 0;

/*  Tried this out on a 7.0 version to see how it worked with vfork.  It
    went through this check with:

       % jobs -z      haderr = 1 and catch = 1

    This set haderr = 0 and then did a closem() and continue.
    It also went through this check with a shell script:

      % 1            where 1 has in it: jobs -z
		     haderr = 1 and catch = 0
    
    In this case it does the unwind; does a longjmp that restores a previous
    stack but all the globals retain their values.
*/
		if (haderr) {
			if (!catch) {
				/* unwind */
				doneinp = 0;
				resexit(osetexit);
				reset();
			}
			haderr = 0;
			/*
			 * Every error is eventually caught here or
			 * the shell dies.  It is at this
			 * point that we clean up any left-over open
			 * files, by closing all but a fixed number
			 * of pre-defined files.  Thus routines don't
			 * have to worry about leaving files open due
			 * to deeper errors... they will get closed here.
			 */
			closem();
			continue;
		}
		if (doneinp) {
			doneinp = 0;
			break;
		}
#ifdef SIGWINCH
		/*
		 *  We need to check if a SIGWINCH occured
		 *  during the previous foreground job (if
		 *  any), because we were in the wrong pro-
		 *  cess group to get the SIGWINCH.
		 */
		winch_handler();
#endif /* SIGWINCH */
		if (chkstop)
			chkstop--;
		if (neednote)
			pnote();
		if (intty && evalvec == 0) {
			mailchk();
			/*
			 * If we are at the end of the input buffer
			 * then we are going to read fresh stuff.
			 * Otherwise, we are rereading input and don't
			 * need or want to prompt.
			 */
			if (fseekp == feobp)
			    printprompt();
			flush();
			if (cp = value(CH_autologout))
			    alarm (Atoi (cp) * 60);	/* Autologout ON */
		}
		err = 0;

		/*
		 * Echo not only on VERBOSE, but also with history expansion.
		 * If there is a lexical error then we forego history echo.
		 */
		if (lex(&paraml) && !err && intty || adrof(CH_verbose)) {
			haderr = 1;
			prlex(&paraml);
			haderr = 0;
		}
		alarm (0);				/* Autologout OFF */

		/*
		 * The parser may lose space if interrupted.
		 */
		if (setintr)
			sighold(SIGINT);

		/*
		 * Save input text on the history list if 
		 * reading in old history, or it
		 * is from the terminal at the top level and not
		 * in a loop.
		 */
		if (enterhist || catch && intty && !whyles)
			savehist(&paraml);

		/*
		 * Print lexical error messages, except when sourcing
		 * history lists.
		 */
		if (!enterhist && err)
			error(err);

		/*
		 * If had a history command :p modifier then
		 * this is as far as we should go
		 */
		if (justpr)
			reset();

		alias(&paraml);

/*  Create a parse tree from the word list.
*/
		/*
		 * Parse the words of the input into a parse tree.
		 */
		t = syntax(paraml.next, &paraml, 0);
		if (err)
			error(err);

		/*
		 * Execute the parse tree
		 */
#ifdef TRACE_DEBUG
  printf ("process (1):  pid: %d, Calling execute with tpgrp: %d\n", getpid (),
	  tpgrp);
#endif

		execute(t, tpgrp);

		/*
		 * Made it!
		 */
		freelex(&paraml), freesyn(t);
	}
/*  Restore the previous setjmp environment.
*/
	resexit(osetexit);
}

/*  Called by:

	main ()
*/
/**********************************************************************/
dosource(t)
	register CHAR **t;
/**********************************************************************/
{
	register CHAR *f;
	register int u;
	bool hflg = 0;
	CHAR buf[BUFSIZ];

	t++;
	if (*t && eq(*t, "-h")) {
		t++;
		hflg++;
		if (!*t)
		    bferr((catgets(nlmsg_fd,NL_SETN,10, "Too few arguments")));
	}
	if (*(t+1))
		bferr((catgets(nlmsg_fd,NL_SETN,11, "Too many arguments")));
	Strcpy(buf, *t);
	f = globone(buf);
	if((u = open(to_char(f), 0)) >= 0) {
		struct stat stb;
		if(fstat(u,&stb)<0) {
			close(u);
			u = -1;
		} else if((stb.st_mode&S_IFMT)!=S_IFREG) {
			close(u);
			error((catgets(nlmsg_fd,NL_SETN,9, "%s: Not a regular file")), to_char(f));
		}
	}
	u = dmove(u, -1);
	xfree(f);
	if (u < 0 && !hflg)
		Perror(to_char(f));
	srcunit(u, 0 ,hflg);
}

/*  Called by:

	process ()
*/
/*
 * Check for mail.
 * If we are a login shell, then we don't want to tell
 * about any mail file unless its been modified
 * after the time we started.
 * This prevents us from telling the user things he already
 * knows, since the login program insists on saying
 * "You have mail."
 */
/**********************************************************************/
mailchk()
/**********************************************************************/
{
	register struct varent *v;
	register CHAR **vp;
	time_t t;
	int intvl, cnt;
	struct stat stb;
	bool new;

	v = adrof(CH_mail);
	if (v == 0)
		return;
	time(&t);
	vp = v->vec;
	cnt = blklen(vp);
	intvl = (cnt && number(*vp)) ? (--cnt, getn(*vp++)) : MAILINTVL;
	if (intvl < 1)
		intvl = 1;
	if (chktim + intvl > t)
		return;
	for (; *vp; vp++) {
		if (stat(to_char(*vp), &stb) < 0)
			continue;
		new = stb.st_mtime > time0;
		if (stb.st_size == 0 || stb.st_atime > stb.st_mtime
		 || (stb.st_atime < chktim && stb.st_mtime < chktim)
		 || loginsh && !new )
			continue;
		if (cnt == 1) {
			if (new)
				printf((catgets(nlmsg_fd,NL_SETN,5, "You have new mail.\n")));
			else
				printf((catgets(nlmsg_fd,NL_SETN,6, "You have mail.\n")));
		} else {
			if (new)
				printf((catgets(nlmsg_fd,NL_SETN,7, "New mail in %s.\n")), to_char(*vp));
			else
				printf((catgets(nlmsg_fd,NL_SETN,8, "Mail in %s.\n")), to_char(*vp));
		}
	}
	chktim = t;
}

#include <pwd.h>
/*  Called by:

	expand ()
*/
/*
 * Extract a home directory from the password file
 * The argument points to a buffer where the name of the
 * user whose home directory is sought is currently.
 * We write the home directory of the user back there.
 */
/**********************************************************************/
gethdir(home)
	CHAR *home;
/**********************************************************************/
{
	register struct passwd *pp = getpwnam(to_char(home));

	if (pp == 0)
		return (1);

/*  Turn the directory name into a 16 bit string and copy it into 'home'.
*/
	Strcpy(home, to_short(pp->pw_dir));
	return (0);
}

/*  Called by:

	main ()
	backeval ()
*/
/*
 * Move the initial descriptors to their eventual
 * resting places, closin all other units.
 */
/**********************************************************************/
initdesc()
/**********************************************************************/
{
	struct stat *s = (struct stat *) malloc(sizeof(struct stat));

/*  Globals defined in sh.h, closed child unused.
*/
	didcch = 0;			/* Havent closed for child */
	didfds = 0;			/* 0, 1, 2 aren't set up */

/*  Set up I/O for the child.
    
    fstat returns a 0 for OK.

    0, 1, 2, 16, 17, 18, and 19 remain open.  0 is duplicated into 16, 1 into 
    17, 2 into 18, and 16 into 19.

    FSHIN = SHIN = 16
    FSHOUT = SHOUT = 17
    FSHDIAG = SHDIAG = 18
    FOLDSTD = OLDSTD = 19
*/
	if (fstat(0, s) >= 0)
		SHIN = dcopy(0, FSHIN);
	SHOUT = dcopy(1, FSHOUT);
	SHDIAG = dcopy(2, FSHDIAG);
	if (fstat(FSHIN, s) >= 0)
		OLDSTD = dcopy(SHIN, FOLDSTD);
	xfree((CHAR *)s);

/*  Close all other open file descriptors.
*/
	closem();
}

/*  Called by:

	prexit ()
	main ()
	exitstat ()
	error ()
	dologin ()
	doenwgrp ()
*/
/**********************************************************************/
exit(i)
	int i;
/**********************************************************************/
{

#if defined (DEBUG_EXIT) || defined (DEBUG_DONEINP)
  printf ("exit (1): pid: %d, exit value: %d\n", getpid (), i);
#endif

	if (prompt && tenexflag) /* csh -T turns off echo: DSDe412942. Use
							  * tenexflag. */
		setup_tty(0); /* DSDe410174: we are setting echo off before prompt, 
					   * reset terminal status here */

	untty();
#ifdef PROF
	IEH3exit(i);
#else
	_exit(i);
#endif
}

/*  Called by:

	process ()
	tenex ()
	backspace ()
*/
/**********************************************************************/
printprompt()
/**********************************************************************/
{
    register CHAR *cp;

    if (!whyles)
    {	
	    if (prompt)
	    {	
	    	if (tenexflag) /* csh -T turns off echo: DSDe412942 */
	       	  setup_tty(1);	/* Extra characters appear after prompt from
				 * typeahead, so set echo off before outputing
				 * the prompt : DSDe410174 */
	    	for (cp = value(CH_prompt); *cp; cp++) {
#ifdef NLS16
			    /* if *cp is a kanji, we want to put out both */
			    /* bytes and also change the 16th bit back to 1 */
		 	    if (*cp & KMASK) {
				    putchar(((*cp >> 8) & 0377) | 0200);
				    putchar(*cp & 0377);
				    continue;
			    }
#endif NLS16
			    if (*cp == HIST)
				    printf("%d", eventno + 1);
			    else {
				    if (*cp == '\\' && cp[1] == HIST)
					    cp++;
				    putchar(*cp | QUOTE);
			    }
		}
	    }
    }
    else {
	    /* guaranteed to be intty */

		if (tenexflag) /* csh -T turns off echo: DSDe412942 */
	    	setup_tty(1);/* Extra characters appear after prompt from
			  * typeahead, so set echo off before outputing
			  * the prompt : DSDe410174 */
	    /*
	     * Prompt for forward reading loop
	     * body content.
	     */
	    printf("? ");
    }
    flush ();
}
