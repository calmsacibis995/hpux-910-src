/* HPUX_ID: @(#) $Revision: 72.1 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#include	"jobs.h"
#include	"history.h"


/* These routines are used by this module but defined elsewhere */
extern void	mac_check();
extern void	name_unscope();
extern void	rm_files();
#ifdef VFORK
    extern void	vfork_restore();
#endif	/* VFORK */

/* ========	error handling	======== */

	/* Find out if it is time to go away.
	 * `trapnote' is set to SIGSET when fault is seen and
	 * no trap has been set.
	 */

/*
 *  This routine is called when fatal errors are encountered
 *  A message is printed out and the shell tries to exit
 */

void sh_fail(s1,s2)
register const char *s1,*s2;
{
	mac_check();
	p_setout(ERRIO);
	p_prp(s1);
	if(s2)
	{
		p_str(e_colon,0);
		p_str(s2,NL);
	}
	else
		newline();
#ifdef POSIX2
	/*
	 *  Exit with value 126 for failed exec with ENOEXEC and
	 *  exit with value 127 for failed exec with EACCESS,
	 *  otherwise exit with ERROR
	 */
	if(s2 == e_found)
	    sh_exit(127);
	if(s2 == e_exec)
	    sh_exit(126);

#endif /* POSIX2 */
	sh_exit(ERROR);
	
}

/* Arrive here from `FATAL' errors
 *  a) exit command,
 *  b) default trap,
 *  c) fault with no trap set.
 *
 * Action is to return to command level or exit.
 */

void sh_exit(xno)
int xno;
{
	register unsigned state=(st.states&~(ERRFLG|MONITOR));
	sh.exitval=xno;
	if(xno==SIGFAIL)
		sh.exitval |= sh.lastsig;
	sh.un.com = 0;
	/*
	 *  This if-statement is to make sure that functions
	 *  handle traps and signals correctly.
	 */
	if(sh.trapnote&DELAY_TRAP)
	{
		io_clear(sh.savio);
		longjmp(*sh.freturn,1);
	}

	if(!(state&FORKED) && (state&(BUILTIN|LASTPIPE)))
	{
		io_clear(sh.savio);
		longjmp(*sh.freturn,1);
	}
	state |= is_option(ERRFLG|MONITOR);
	if(!(state&(PROFILE|PROMPT|FUNCTION)) || (state&(ERRFLG|FORKED)))
	{
		st.states = state;
		sh_done(0);
	}
	else
	{
		if(!(state&FUNCTION))
		{
			p_flush();
			name_unscope();
			arg_clear();
			io_clear((struct fileblk*)0);
			io_restore(0);
			if(st.standin)
			{
				*st.standin->last = 0;
				/* flush out input buffer */
				while(finbuff(st.standin)>0)
					io_readc();
			}
		}
#ifdef VFORK
		vfork_restore();
#endif	/* VFORK */
		st.execbrk = st.breakcnt = 0;
		st.exec_flag = st.subflag = 0;
		st.dot_depth = 0;
		hist_flush();
		state &= ~(FUNCTION|FIXFLG|RWAIT|PROMPT|READPR|MONITOR|BUILTIN|
			LASTPIPE|VFORKED|GRACE|NONSTOP);
		state |= is_option(INTFLG|READPR|MONITOR);
		job.pipeflag = 0;
		st.states = state;
		longjmp(*sh.freturn,1);
	}
}

/*
 * This is the exit routine for the shell
 */

void sh_done(sig)
register int sig;
{
	register char *t;
	register int savxit = sh.exitval;
	if(sh.trapnote&SIGBEGIN)
		return;
	/*
	 * DSDe407155
	 * make sure t is not a null pointer and not a null string.
	 */
	t=st.trapcom[0];
	if(t && *t)
	{
		st.trapcom[0]=0; /*should free but not long */
		sh_eval(t);
	}
	else
	{
		/* avoid recursive call for set -e */
		st.states &= ~ERRFLG;
		sh_chktrap();
	}
	io_rmtemp((struct ionod*)0);
#ifdef ACCT
	doacct();
#endif	/* ACCT */
#if VSH || ESH
	if(is_option(EMACS|EDITVI|GMACS) && st.standin && 
		(st.standin->flag&IOEDIT))
		tty_cooked(ERRIO);
#endif
	if(st.states&RM_TMP)
	/* clean up all temp files */
		rm_files(io_tmpname);
	p_flush();
#ifdef JOBS
	if(sig==SIGHUP || (is_option(INTFLG)&&sh.login_sh))
		job_terminate();
#endif	/* JOBS */
	if(sig)
	{
		/* generate fault termination code */
		signal(sig,SIG_DFL);
		sigrelease(sig);
		kill(getpid(),sig);
		pause();
	}
	job_close();
	io_sync();
	_exit(savxit&EXITMASK);
}

