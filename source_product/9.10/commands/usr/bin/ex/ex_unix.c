/* @(#) $Revision: 70.5 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifndef NONLS8 /* User messages */
# define	NL_SETN	13	/* set number */
# include	<msgbuf.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Unix escapes, filtering
 */

/*
 * First part of a shell escape,
 * parse the line, expanding # and % and ! and printing if implied.
 */
unix0(warn)
	bool warn;
{
	register char *up, *fp;
	register short c;
	char printub;
	char  puxb[UXBSIZE + sizeof (int)];

	printub = 0;
	CP(puxb, uxb);
	c = getchar();
	if (c == '\n' || c == EOF)

#ifndef NONLS8 /* User messages */
		error((nl_msg(1, "Incomplete shell escape command|Incomplete shell escape command - use 'shell' to get a shell")));
#else NONLS8
		error("Incomplete shell escape command@- use 'shell' to get a shell");
#endif NONLS8

	up = uxb;
	do {
		switch (c) {

		case '\\':
			if (any(peekchar(), "%#!"))
				c = getchar();
		default:
			if (up >= &uxb[UXBSIZE]) {
tunix:
				uxb[0] = 0;
				error((nl_msg(2, "Command too long")));
			}
			*up++ = c;
			break;

		case '!':
			fp = puxb;
			if (*fp == 0) {
				uxb[0] = 0;

#ifndef NONLS8 /* User messages */
				error((nl_msg(3, "No previous command|No previous command to substitute for !")));
#else NONLS8
				error("No previous command@to substitute for !");
#endif NONLS8

			}
			printub++;
			while (*fp) {
				if (up >= &uxb[UXBSIZE])
					goto tunix;
				*up++ = *fp++;
			}
			break;

		case '#':
			fp = altfile;
			if (*fp == 0) {
				uxb[0] = 0;

#ifndef NONLS8 /* User messages */
				error((nl_msg(4, "No alternate filename|No alternate filename to substitute for #")));
#else NONLS8
				error("No alternate filename@to substitute for #");
#endif NONLS8

			}
			goto uexp;

		case '%':
			fp = savedfile;
			if (*fp == 0) {
				uxb[0] = 0;

#ifndef NONLS8 /* User messages */
				error((nl_msg(5, "No filename|No filename to substitute for %%")));
#else NONLS8
				error("No filename@to substitute for %%");
#endif NONLS8

			}
uexp:
			printub++;
			while (*fp) {
				if (up >= &uxb[UXBSIZE])
					goto tunix;
				*up++ = *fp++;
			}
			break;
		}
		c = getchar();
	} while (c == '"' || c == '|' || !endcmd(c));
	if (c == EOF)
		ungetchar(c);
	*up = 0;
	if (!inopen)
		resetflav();
	if (warn)
		ckaw();
	if (warn && hush == 0 && chng && xchng != chng && value(WARN) && dol > zero) {
		xchng = chng;
		vnfl();
#if defined NLS || defined NLS16
		RL_OKEY
#endif
		printf(mesg((nl_msg(6, "[No write]|[No write since last change]"))));
#if defined NLS || defined NLS16
		RL_OSCREEN
#endif
		noonl();
		flush();
	} else
		warn = 0;
	if (printub) {
		if (uxb[0] == 0)

#ifndef NONLS8 /* User messages */
			error((nl_msg(7, "No previous command|No previous command to repeat")));
#else NONLS8
			error("No previous command@to repeat");
#endif NONLS8

		if (inopen) {
			splitw++;
			vclean();
			vgoto(WECHO, 0);
		}
		if (warn)
			vnfl();
		if (hush == 0)
			lprintf("!%s", uxb);
		if (inopen && Outchar != termchar) {
			vclreol();
			vgoto(WECHO, 0);
		} else
			putnl();
		flush();
	}
}

/*
 * Do the real work for execution of a shell escape.
 * Mode is like the number passed to open system calls
 * and indicates filtering.  If input is implied, newstdin
 * must have been setup already.
 */
ttymode
unixex(opt, up, newstdin, mode)
	char *opt, *up;
	int newstdin, mode;
{
	int pvec[2];
	ttymode f;
#ifdef SIGWINCH
	struct winsize win;
#endif SIGWINCH

#ifndef hpe
	signal(SIGINT, SIG_IGN);
#ifdef SIGTSTP
	if (dosusp)
		signal(SIGTSTP, SIG_DFL);
#endif
#ifdef SIGWINCH
	if (invisual) {
	        if (ioctl(0, TIOCGWINSZ, &win) >= 0)
		{
	    	    save_win.ws_row = win.ws_row;
		    save_win.ws_col = win.ws_col;
		}
		/* vi ignore SIGWINCH while escaping to unix commands */
		signal(SIGWINCH, SIG_IGN);
		ign_winch = 1;
	}
#endif SIGWINCH
	if (inopen)
		f = setty(normf);
	if ((mode & 1) && pipe(pvec) < 0) {
		/* Newstdin should be io so it will be closed */
		if (inopen)
			setty(f);
		error((nl_msg(8, "Can't make pipe for filter")));
	}
#ifndef VFORK
	pid = fork();
#else
	pid = vfork();
#endif
	if (pid < 0) {
		if (mode & 1) {
			close(pvec[0]);
			close(pvec[1]);
		}
		setrupt();
		if (inopen)
			setty(f);
		error((nl_msg(9, "No more processes")));
	}
	if (pid == 0) {
		if (mode & 2) {
			close(0);
			dup(newstdin);
			close(newstdin);
		}
		if (mode & 1) {
			close(pvec[0]);
			close(1);
			dup(pvec[1]);
			if (inopen) {
				close(2);
				dup(1);
			}
			close(pvec[1]);
		}
		if (io)
			close(io);
		if (tfile)
			close(tfile);
#ifndef VMUNIX
		close(erfile);
#endif
		signal(SIGHUP, oldhup);
		signal(SIGQUIT, oldquit);
		if (ruptible)
			signal(SIGINT, SIG_DFL);

		/***********
		* Defect fix -- FSDlj02320 -- Set SIGCLD to default.
		***********/
		signal(SIGCLD, SIG_DFL);

		execl(svalue(SHELL), "sh", opt, up, (char *) 0);
		printf((nl_msg(10, "No %s!\n")), svalue(SHELL));
		error(_NOSTR);
	}
	if (mode & 1) {
		io = pvec[0];
		close(pvec[1]);
	}
	if (newstdin)
		close(newstdin);
#else
	system(up);
#endif hpe
	return (f);
}

/*
 * Wait for the command to complete.
 * F is for restoration of tty mode if from open/visual.
 * C flags suppression of printing.
 */
unixwt(c, f)
	bool c;
	ttymode f;
{

	waitfor();
#ifdef SIGTSTP
	if (dosusp)
		signal(SIGTSTP, onsusp);
#endif

/* Window changes (SIGWINCH) are checked in vcontin [ex_cmds2.c]   */
/* to determine if any window resizes occurred during the forked   */
/* process.  Checking for window resizes, and calling the SIGWINCH */
/* signal handler at this point, causing the screen to become      */
/* garbled.							   */

	if (inopen)
		setty(f);
	setrupt();
	if (!inopen && c && hush == 0) {
		printf("!\n");
		flush();
		termreset();
		gettmode();
	}
}

/*
 * Setup a pipeline for the filtration implied by mode
 * which is like a open number.  If input is required to
 * the filter, then a child editor is created to write it.
 * If output is catch it from io which is created by unixex.
 */
filter(mode)
	register int mode;
{
	static int pvec[2];
	ttymode f;	/* mjm: was register */
	register int nlines = lineDOL();

	mode++;
	if (mode & 2) {
		signal(SIGINT, SIG_IGN);
		if (pipe(pvec) < 0)
			error((nl_msg(11, "Can't make pipe")));
		pid = fork();
		io = pvec[0];
		if (pid < 0) {
			setrupt();
			close(pvec[1]);
			error((nl_msg(12, "No more processes")));
		}
		if (pid == 0) {
			setrupt();
			io = pvec[1];
			close(pvec[0]);
			putfile(1);
			exit(0);
		}
		close(pvec[1]);
		io = pvec[0];
		setrupt();
	}
	f = unixex("-c", uxb, (mode & 2) ? pvec[0] : 0, mode);
	if (mode == 3) {
		delete(0);
		addr2 = addr1 - 1;
	}
	if (mode == 1)
		deletenone();
	if (mode & 1) {
		if(FIXUNDO)
			undap1 = undap2 = addr2+1;
		ignore(append(getfile, addr2));
#ifdef UNDOTRACE
		if (trace)
			vudump("after append in filter");
#endif
	}
	close(io);
	io = -1;
	unixwt(!inopen, f);
	netchHAD(nlines);
}

/*
 * Set up to do a recover, getting io to be a pipe from
 * the recover process.
 */
recover()
{
	static int pvec[2];
	int i;

	if (pipe(pvec) < 0)
		error((nl_msg(13, " Can't make pipe for recovery")));
	pid = fork();
	io = pvec[0];
	if (pid < 0) {
		close(pvec[1]);
		error((nl_msg(14, " Can't fork to execute recovery")));
	}
	if (pid == 0) {
		close(2);
		dup(1);
		close(1);
		dup(pvec[1]);
	        close(pvec[1]);

	/***********
	* Reset all signals of the child process to their defaults.
	***********/
	for (i=1; i < _NSIG; i++)
	    signal(i, SIG_DFL);

#ifndef NAME8
		execl(EXRECOVER, "exrecover", svalue(DIRECTORY), file, (char *) 0);
#else NAME8
		execl(EXRECOVER, "exrecover8", svalue(DIRECTORY), file, (char *) 0);
#endif NAME8
		close(1);
		dup(2);
		error((nl_msg(15, " No recovery routine")));
	}
	close(pvec[1]);
}

/*
 * Wait for the process (pid an external) to complete.
 */
waitfor()
{

	do
		rpid = wait(&status);
	while (rpid != pid && rpid != -1);
	if ((status & 0377) == 0) {
		status = (status >> 8) & 0377;
	} else {
		printf((nl_msg(16, "%d: terminated with signal %d")), pid, status & 0177);
		if (status & 0200)
			printf((nl_msg(17, " -- core dumped")));
		putchar('\n');
	}
}

/*
 * The end of a recover operation.  If the process
 * exits non-zero, force not edited; otherwise force
 * a write.
 */
revocer()
{

	waitfor();
	if (pid == rpid && status != 0)
		edited = 0;
	else
		change();
}
