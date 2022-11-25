/* @(#) $Revision: 70.4 $ */   
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include <locale.h>

#ifdef TRACE
char	tttrace[]	= { '/','d','e','v','/','t','t','y','x','x',0 };
FILE	*trace;
bool	trubble;
bool	techoin;
#endif

/*
 * The code for ex is divided as follows:
 *
 * ex.c			Entry point and routines handling interrupt, hangup
 *			signals; initialization code.
 *
 * ex_addr.c		Address parsing routines for command mode decoding.
 *			Routines to set and check address ranges on commands.
 *
 * ex_cmds.c		Command mode command decoding.
 *
 * ex_cmds2.c		Subroutines for command decoding and processing of
 *			file names in the argument list.  Routines to print
 *			messages and reset state when errors occur.
 *
 * ex_cmdsub.c		Subroutines which implement command mode functions
 *			such as append, delete, join.
 *
 * ex_data.c		Initialization of options.
 *
 * ex_get.c		Command mode input routines.
 *
 * ex_io.c		General input/output processing: file i/o, unix
 *			escapes, filtering, source commands, preserving
 *			and recovering.
 *
 * ex_put.c		Terminal driving and optimizing routines for low-level
 *			output (cursor-positioning); output line formatting
 *			routines.
 *
 * ex_re.c		Global commands, substitute, regular expression
 *			compilation and execution.
 *
 * ex_set.c		The set command.
 *
 * ex_subr.c		Loads of miscellaneous subroutines.
 *
 * ex_temp.c		Editor buffer routines for main buffer and also
 *			for named buffers (Q registers if you will.)
 *
 * ex_tty.c		Terminal dependent initializations from termcap
 *			data base, grabbing of tty modes (at beginning
 *			and after escapes).
 *
 * ex_unix.c		Routines for the ! command and its variations.
 *
 * ex_v*.c		Visual/open mode routines... see ex_v.c for a
 *			guide to the overall organization.
 */

/*
 * Main procedure.  Process arguments and then
 * transfer control to the main command processing loop
 * in the routine commands.  We are entered as either "ex", "edit", "vi"
 * or "view" and the distinction is made here.  Actually, we are "vi" if
 * there is a 'v' in our name, "view" is there is a 'w', and "edit" if
 * there is a 'd' in our name.  For edit we just diddle options;
 * for vi we actually force an early visual command.
 */

#ifndef NONLS8	/* User messages */
# define NL_SETN	1	/* set number */
# include <msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

main(ac, av)
	register int ac;
	register char *av[];
{
#ifndef VMUNIX
	char *erpath = EXSTRINGS;
#endif
	register char *cp;
	register int c;
	bool recov = 0;
	bool ivis;
	bool itag = 0;
	bool fast = 0;
	extern int onemt();
	extern int oncore();
	extern int verbose;
	char pwd[MAXPATHLEN+2];
#if defined NLS || defined NLS16
	char *lang;
#endif NLS || NLS16
#ifdef TRACE
	register char *tracef;
#endif

#ifdef SIGWINCH
	invisual = 0;
#endif SIGWINCH

#ifdef hpe
	{
		char *p;
		int i;

		/* Av[0] points to "filename.group.account".   */
		/* Strip the group and account name so they    */
		/* won't interfere with the way we figure out  */
		/* how this program was invoked (i.e ex or vi) */
		p = strchr(av[0],'.');
		*p = '\0';

		/* Program name will be in upper case - need   */
		/* to downshift name because algorthm below    */
		/* depends on lower case letters.              */
		for (p = av[0]; *p; ++p)
			*p = tolower(*p);

		/* set tabs for proper cursor control */
		_settabs();
	}
#endif

#if defined NLS || defined NLS16	/* initialize to the right language */
	if (!setlocale(LC_ALL,"")) {
		/* couldn't initialize the locale, tell them why */
		columns = 80;		/* minimum initialization needed to output error message */
		putS(_errlocale(""));
		putS((nl_msg(3, "[Hit return to continue]")));
		ignchar();
		/*
		 * unset LANG, etc., so that later exec's & popen's won't give
		 * us multiple warnings from other NLS'ized commands
		 */
		putenv("LANG="); putenv("LC_COLLATE="); putenv("LC_CTYPE=");
		putenv("LC_TIME="); putenv("LC_NUMERIC="); putenv("LC_MONETARY=");
		_nl_fn = (nl_catd)-1;
	} else
		nl_catopen("ex");

	if (_nl_direct == NL_RTL) {
		/* set up right-to-left language variables */
		right_to_left = 1;
		rl_order = _nl_order;
		rl_mode = _nl_mode;
		alt_space = _nl_space_alt;
		alt_uparrow = ascii_to_alt('^');
		alt_tilde = ascii_to_alt('~');
		alt_amp = ascii_to_alt('@');
		alt_quote = ascii_to_alt('"');
		alt_slash = ascii_to_alt('/');
		alt_backslash = ascii_to_alt('\\');
		alt_zero = ascii_num_to_alt('0');
		if (!alt_zero)
			alt_zero = '0';
		if (!strcmp((lang = getenv("LANG")),"hebrew")) {
			rl_lang = HEBREW;
			rl_keys = heb_keys;
		} else if (!strcmp(lang,"arabic")) {
			rl_lang = ARABIC;
			rl_keys = arb_keys;
		} else if (!strcmp(lang,"arabic-w")) {
			rl_lang = ARABIC;
			rl_keys = arb_keys;
		} else {
			rl_lang = OTHER;
		}
		get_rlsent();
	} else {
		alt_space = ' ';
		opp_fix = 1;
	}
#endif NLS || NLS16

	/*
	 * Immediately grab the tty modes so that we wont
	 * get messed up if an interrupt comes in quickly.
	 */
	gTTY(2);
#ifndef USG
	normf = tty.sg_flags;
#else
	normf = tty;
#endif
#ifdef ED1000
	breakstate = tty.c_iflag;
	tty.c_iflag |= IGNBRK;
	sTTY(2);
	/*
	 * check if we have more than one arg other than an optional
	 * "-r". If yes, then produce an err saying multiple file edits
	 * are not supported and user must use "fi newfile" command.
	 */
	if (argv[1][0] == '-' && argv[1][1] != 'r') {
		error(nl_msg(101, "usage: ed1000 [-r] [file]\n"));
		exit();
	}
#endif ED1000

	ppid = getpid();
	/* Note - this will core dump if you didn't -DSINGLE in CFLAGS */
	lines = 24;
	columns = 80;	/* until defined right by setupterm */
	/*
	 * Defend against d's, v's, w's, and a's in directories of
	 * path leading to our true name.
	 */
	av[0] = tailpath(av[0]);

	/*
	 * Figure out how we were invoked: ex, edit, vi, view.
	 */
	ivis = any('v', av[0]);	/* "vi" */
	if (any('w', av[0]))	/* "view" */
		value(READONLY) = 1;
	if (any('d', av[0])) {	/* "edit" or "vedit" */
		value(NOVICE) = 1;
		value(REPORT) = 1;
		value(MAGIC) = 0;
		value(SHOWMODE) = 1;
#ifdef ED1000
		value(OPTIMIZE) = 0;
		value(WRITEANY) = 0;
		/* force !writeany for the new ed1000 for now. This
		 * option somehow gets set in the 16-bit ed1000, and a
		 * test passes, allowing a write to an existing file
		 * --svn
		 */
#endif ED1000
	}

#ifndef VMUNIX
	/*
	 * For debugging take files out of . if name is a.out.
	 */
	if (av[0][0] == 'a')
		erpath = tailpath(erpath);
#endif
	/*
	 * Open the error message file.
	 */
	draino();
#ifndef VMUNIX
	erfile = open(erpath+4, 0);
	if (erfile < 0) {
		erfile = open(erpath, 0);
	}
#endif
	pstop();

	/*
	 * Initialize interrupt handling.
	 */
	oldhup = signal(SIGHUP, SIG_IGN);
	if (oldhup == SIG_DFL)
		signal(SIGHUP, onhup);
	oldquit = signal(SIGQUIT, SIG_IGN);
#ifdef USG
	oldchild = signal(SIGCLD, SIG_IGN);
#endif
	ruptible = signal(SIGINT, SIG_IGN) == SIG_DFL;
	if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
		signal(SIGTERM, onhup);
	if (signal(SIGEMT, SIG_IGN) == SIG_DFL)
		signal(SIGEMT, onemt);
	signal(SIGILL, oncore);
	signal(SIGTRAP, oncore);
	signal(SIGIOT, oncore);
	signal(SIGFPE, oncore);
	signal(SIGBUS, oncore);
	signal(SIGSEGV, oncore);
	signal(SIGPIPE, oncore);
#ifdef SIGWINCH     /* ignore SIGWINCH until we are in "vi" mode */
	signal(SIGWINCH, SIG_IGN);
	ign_winch = 1;
#endif SIGWINCH

	/*
	 * Initialize end of core pointers.
	 * Normally we avoid breaking back to fendcore after each
	 * file since this can be expensive (much core-core copying).
	 * If your system can scatter load processes you could do
	 * this as ed does, saving a little core, but it will probably
	 * not often make much difference.
	 */
	fendcore = (line *) sbrk(0);
	endcore = fendcore - 2;

	/*
	 * Process flag arguments.
	 */
	ac--, av++;
	while (ac && av[0][0] == '-') {
		c = av[0][1];
#ifdef ED1000
		switch (c) {
		case 'r':
			recov++;
			break;

#else ED1000
		if (c == 0) {
			hush = 1;
			value(AUTOPRINT) = 0;
			fast++;
		} else switch (c) {

		case 'R':
			value(READONLY) = 1;
			break;

		case 'T':
#ifdef TRACE
			if (av[0][2] == 0)
				tracef = "trace";
			else {
				tracef = tttrace;
				tracef[8] = av[0][2];
				if (tracef[8])
					tracef[9] = av[0][3];
				else
					tracef[9] = 0;
			}
			trace = fopen(tracef, "w");
#define tracbuf NULL
			if (trace == NULL)
				printf("Trace create error\n");
			else
				setbuf(trace, tracbuf);
#endif
			break;

		case 'l':
			value(LISP) = 1;
			value(SHOWMATCH) = 1;
			break;

		case 'r':
			recov++;
			break;

		case 'V':
			verbose = 1;
			break;

		case 't':
			if (av[0][2]) {
				itag = 1;
				strncpy(lasttag, &av[0][2],TAGSIZE-1);
			}
			else if (ac > 1 && av[1][0] != '-') {
				ac--, av++;
				itag = 1;
				strncpy(lasttag, av[0],TAGSIZE-1);
			}
			break;

		case 'v':
			ivis = 1;
			break;

		case 'w':
			defwind = 0;
			if (av[0][2] == 0) defwind = 3;

#ifndef NONLS8	/* Character set features */
			else for (cp = &av[0][2]; isdigit(*cp & TRIM); cp++)
#else NONLS8
			else for (cp = &av[0][2]; isdigit(*cp); cp++)
#endif NONLS8

				defwind = 10*defwind + *cp - '0';
			break;

		case 'x':
			/* -x: encrypted mode */

/* ========================================================================= */
/*
** CRYPT block 1
*/
#ifdef CRYPT
			xflag = xeflag = 1;
#else CRYPT
			smerror((nl_msg(1, "The -x option is not available.\n")), '\0');
#endif CRYPT
/* ========================================================================= */

			break;
#endif ED1000
		default:
			smerror((nl_msg(2, "Unknown option %s\n")), av[0]);
			break;
		}
		ac--, av++;
	}

	if (ac && av[0][0] == '+') {
		firstpat = &av[0][1];
		ac--, av++;
	}

/* ========================================================================= */
/*
** CRYPT block 2
*/
#ifdef CRYPT
	if(xflag){
		key = getpass(KEYPROMPT);
		kflag = crinit(key, perm);
	}
#endif CRYPT
/* ========================================================================= */

	/*
	 * If we are doing a recover and no filename
	 * was given, then execute an exrecover command with
	 * the -r option to type out the list of saved file names.
	 * Otherwise set the remembered file name to the first argument
	 * file name so the "recover" initial command will find it.
	 */
	if (recov) {
		if (ac == 0) {
			ppid = 0;
			setrupt();
#ifndef NAME8
			execl(EXRECOVER, "exrecover", "-r", 0);
#else NAME8
			execl(EXRECOVER, "exrecover8", "-r", 0);
#endif NAME8
			filioerr(EXRECOVER);
			exit(1);
		}
		CP(savedfile, *av++), ac--;
	}

	/*
	 * Initialize the argument list.
	 */
	argv0 = av;
	argc0 = ac;
	args0 = av[0];
	erewind();

	/*
	 * Initialize a temporary file (buffer) and
	 * set up terminal environment.  Read user startup commands.
	 */
	if (setexit() == 0) {
		setrupt();
		intty = isatty(0);
		value(PROMPT) = intty;
		if (cp = getenv("SHELL"))
			CP(shell, cp);
		if (fast)
			setterm("dumb");
		else {
			gettmode();
			cp = getenv("TERM");
			if (cp == NULL || *cp == '\0')
				cp = "unknown";
			setterm(cp);
#if defined NLS || defined NLS16
			/*
			** for now use TERM to set opp lang booleans.
			** Eventually this should be in terminfo.
			*/
			if (right_to_left) {
				if (!strncmp(cp,"hp150",5)) {
					opp_jump_insert = 0;
					opp_terminate = 0;
				} else if (!strncmp(cp,"hp2392",6)) {
					opp_jump_insert = 1;
					opp_terminate = 1;
				} else {
					right_to_left = 0;
				}
				if (right_to_left) {
					get_rlterm();
					set_rlterm();
				}
			}
#endif
		}
	}
	init();	/* moved up here in case initializations contain open command */

#ifndef ED1000
#ifdef SIGTSTP
	if (!hush && signal(SIGTSTP, SIG_IGN) == SIG_DFL)
		signal(SIGTSTP, onsusp), dosusp++;
#endif

	if (setexit() == 0 && !fast) {
		dir_chg = 1;
		if ((globp = getenv("EXINIT")) && *globp)
			commands(1,1);
		else {
			globp = 0;
#ifndef hpe
			if ((cp = getenv("HOME")) != 0 && *cp)

#ifndef	NLS16
				source(strcat(strcpy(genbuf, cp), "/.exrc"), 1);
#else
				source(strcat(strcpy(GENBUF, cp), "/.exrc"), 1);
#endif
#else
			if ((cp = getenv("HOME")) != 0 && *cp)
				source(strcat(strcpy(genbuf, "exrc."), cp), 1);
			else if ((cp = getenv("HPHGROUP")) != 0 && *cp) { 
				strcpy(genbuf, "exrc.");
				strcat(genbuf, cp);
				strcat(genbuf,".");
				if ((cp = getenv("HPACCOUNT")) != 0 && *cp) 
					strcat(genbuf, cp);
				source(genbuf, 1);
			}

#endif hpe
		}
		/*
		 * Allow local .exrc too.  This loses if . is $HOME,
		 * but nobody should notice unless they do stupid things
		 * like putting a version command in .exrc.  Besides,
		 * they should be using EXINIT, not .exrc, right?
		 */
#ifndef hpe
		/* somebody noticed--don't do $HOME/.exrc twice */
		/*
		 * Also, only process a local .exrc if EXINIT or $HOME/.exrc
		 * set the exrc option (a security feature).
		 * Also for security: disallow local .exrc if uid root.
		 * Ref. SR#5003081711 
		 */
		if (getuid() && value(EXRC) && ((cp = getenv("HOME")) == 0 || strcmp(cp, getcwd(pwd, MAXPATHLEN+2))))
			source(".exrc", 1);
#else
		source("exrc", 1);
#endif hpe
	}
#endif ED1000
	dir_chg = 0;

	/*
	 * Initial processing.  Handle tag, recover, and file argument
	 * implied next commands.  If going in as 'vi', then don't do
	 * anything, just set initev so we will do it later (from within
	 * visual).
	 */
	if (setexit() == 0) {
		if (recov)
			globp = "recover";
		else if (itag)
			globp = ivis ? "tag" : "tag|p";
#ifdef ED1000
		else if (argc)
			globp = "version|next|1\n";
		else
			globp = "version";
#else ED1000
		else if (argc)
			globp = "next";
#endif ED1000
		if (ivis)
			initev = globp;
		else if (globp) {
			inglobal = 1;
			commands(1, 1);
			inglobal = 0;
		}
	}

	/*
	 * Vi command... go into visual.
	 * Strange... everything in vi usually happens
	 * before we ever "start".
	 */
	if (ivis) {
		/*
		 * Don't have to be upward compatible with stupidity
		 * of starting editing at line $.
		 */
		if (dol > zero)
			dot = one;
		globp = "visual";
		if (setexit() == 0)
			commands(1, 1);
	}

	/*
	 * Clear out trash in state accumulated by startup,
	 * and then do the main command loop for a normal edit.
	 * If you quit out of a 'vi' command by doing Q or ^\,
	 * you also fall through to here.
	 */
	seenprompt = 1;

#ifndef	NLS16
	ungetchar(0);
#else
	/*
	 * It is necessary to clear not only peekc but also secondchar.
	 */
	clrpeekc();
#endif

	globp = 0;
	initev = 0;
	setlastchar('\n');
	setexit();
	commands(0, 0);
	cleanup(1);
	exit(0);
}

/*
 * Initialization, before editing a new file.
 * Main thing here is to get a new buffer (in fileinit),
 * rest is peripheral state resetting.
 */
init()
{
	register int i;

	/* allowing no interrupts here seems to make things go smoother */
	signal(SIGINT, SIG_IGN);

	fileinit();
	dot = zero = truedol = unddol = dol = fendcore;
	one = zero+1;
	undkind = UNDNONE;
	chng = 0;
	real_empty = oreal_empty = 0;
	edited = 0;
	for (i = 0; i <= 'z'-'a'+1; i++)
		names[i] = 1;
	anymarks = 0;
/* ========================================================================= */
/*
** CRYPT block 3
*/
#ifdef CRYPT
        if(xflag) {
                xtflag = 1;
                makekey(key, tperm);
        }
#endif CRYPT
/* ========================================================================= */
	setrupt();	/* re-enable interrupts again */
}

/*
 * Return last component of unix path name p.
 */
char *
tailpath(p)
register char *p;
{
	register char *r;

	for (r=p; *p; p++)

#ifndef	NLS16
		if (*p == '/')
#else
		/* prevent mistaking 2nd byte of 16-bit character for a ANK. */
		if (FIRSTof2((*p)&TRIM) && SECONDof2((*(p+1))&TRIM))
			p++;
		else if (*p == '/')
#endif

			r = p+1;
	return(r);
}
