/* @(#) $Revision: 72.5 $ */
/**********************************************************************
 *       subroutine to do builtin functions
 **********************************************************************/

#ifndef NLS
#   define catgets(i,sn,mn, s) (s)
#else NLS
#   define NL_SETN 8	/* set number */
#   include <locale.h>
#   include <nl_types.h>
    nl_catd nlmsg_fd;
#endif /* NLS */

#ifdef NLS16
#   include <nl_ctype.h>
#endif /* NLS16 */

#include "sh.h"
#include <sys/ioctl.h>

#ifdef RFA
#include <errnet.h>
#endif /* RFA */

#include <fcntl.h>

extern void (*sigset())();
extern void (*sigsys())();

#ifdef NLS
    extern int	  _nl_space_alt;
#   define ALT_SP (_nl_space_alt & TRIM)
#endif

/*  Called by:

	execute ()
*/
/**********************************************************************/
struct biltins *
isbfunc(t)
/**********************************************************************/
	register struct command *t;
{
	register char *cp;
	register char *dp;
	register struct biltins *bp;
	int dolabel(), dofg1(), dobg1();
	static struct biltins label = { "", dolabel, 0, 0 };
	static struct biltins foregnd = { "%job", dofg1, 0, 0 };
	static struct biltins backgnd = { "%job &", dobg1, 0, 0 };
	cp = to_char(t->t_dcom[0]);
	if (lastchr(t->t_dcom[0]) == ':') {
		label.bname = cp;
		return (&label);
	}
	if (*cp == '%') {
		if (t->t_dflg & FAND) {
			t->t_dflg &= ~FAND;
			backgnd.bname = cp;
			return (&backgnd);
		}
		foregnd.bname = cp;
		return (&foregnd);
	}

	for (bp = bfunc; dp = bp->bname; bp++) {
		if (dp[0] == cp[0] && strcmp(dp,cp) == 0)
			return (bp);
		if (dp[0] > cp[0])
			break;
	}
	return (0);
}

/*  Called by:

	execute ()
*/
/**********************************************************************/
func(t, bp)
	register struct command *t;
	register struct biltins *bp;
/**********************************************************************/
{
	int i;

	xechoit(t->t_dcom);
	setname(bp->bname);
	i = blklen(t->t_dcom) - 1;
	if (i < bp->minargs) {
		bferr((catgets(nlmsg_fd,NL_SETN,1, "Too few arguments")));
	}
	if (i > bp->maxargs)
		bferr((catgets(nlmsg_fd,NL_SETN,2, "Too many arguments")));
	(*bp->bfunct)(t->t_dcom, t);
}

/**********************************************************************/
dolabel()
/**********************************************************************/
{

}

#ifndef NONLS
CHAR CH_minus[] = {'-',0};
#else
#define CH_minus	"-"
#endif

/*  Called by func when the command onintr is executed.
*/
/**********************************************************************/
doonintr(v)
	CHAR **v;
/**********************************************************************/
{
	register CHAR *cp;
	register CHAR *vv = v[1];

	if (parintr == SIG_IGN)
		return;
	if (setintr && intty)
		bferr((catgets(nlmsg_fd,NL_SETN,3, "Can't from terminal")));
	cp = gointr, gointr = 0, xfree(cp);

/*  The variable vv is the argument to onintr:
    
    onintr : reset default for SIGINT
    onintr - : set SIGINT to SIG_IGN
    onintr <label> : set SIGINT to pintr, save label in gointr
*/
	if (vv == 0) {
		if (setintr)
			sigset(SIGINT, SIG_IGN);
		else
			sigset(SIGINT, SIG_DFL);
		gointr = 0;
	} else if (eq((vv = strip(vv)), "-")) {
		sigset(SIGINT, SIG_IGN);
		gointr = CH_minus;
	} else {
		gointr = savestr(vv);
		sigset(SIGINT, pintr);
	}
}

/**********************************************************************/
donohup()
/**********************************************************************/
{

	if (intty)
		bferr((catgets(nlmsg_fd,NL_SETN,4, "Can't from terminal")));
	if (setintr == 0) {
		sigset(SIGHUP, SIG_IGN);
#ifdef CC
		submit(getpid());
#endif
	}
}

/**********************************************************************/
dozip()
/**********************************************************************/
{

#ifdef DEBUG_IF_THEN_ELSE
  printf ("dozip (1): %d, ending and if or switch or case or default\n",
	  getpid ());
#endif

	;
}

/**********************************************************************/
prvars()
/**********************************************************************/
{

	plist(&shvhed);
}

/**********************************************************************/
doalias(v)
	register CHAR **v;
/**********************************************************************/
{
	register struct varent *vp;
	register CHAR *p;

	v++;
	p = *v++;
	if (p == 0)
		plist(&aliases);
	else if (*v == 0) {
		vp = adrof1(strip(p), &aliases);
		if (vp)
			blkpr(vp->vec), printf("\n");
	} else {
		if (eq(p, "alias") || eq(p, "unalias")) {
			setname(to_char(p));
			bferr((catgets(nlmsg_fd,NL_SETN,5, "Too dangerous to alias that")));
		}
		set1(strip(p), saveblk(v), &aliases);
	}
}

/**********************************************************************/
unalias(v)
	CHAR **v;
/**********************************************************************/
{

	unset1(v, &aliases);
}

/**********************************************************************/
dologout()
/**********************************************************************/
{

	islogin();
	goodbye();
}

/**********************************************************************/
dologin(v)
	CHAR **v;
/**********************************************************************/
{

	islogin();
	sigset(SIGTERM, parterm);
	sigrelse(SIGUSR2);
#ifdef 	V4FS
	execl("/usr/bin/login", "login", to_char(v[1]), 0);
#else
	execl("/bin/login", "login", to_char(v[1]), 0);
#endif 	/* V4FS */
	untty();

#ifdef DEBUG_EXIT
  printf ("dologin (1): %d, Calling exit(1)\n", getpid ());
#endif

	exit(1);
}
               /* MAXCH increased from 1024 --> NCARGS;  */

#define	MAXCH	NCARGS

/**********************************************************************/
donewgrp(v)
	CHAR **v;
/**********************************************************************/
{
	char tmp[MAXCH*2+1];

	if (*v[1] && *v[2])
		strcpy(tmp, to_char(v[2]));

	if (chkstop == 0 && setintr)
		panystop(0);
	sigset(SIGTERM, parterm);
	sigrelse(SIGUSR2);
	untty();
	if (*v[1] && *v[2]) {
#ifdef V4FS
		execl("/usr/bin/newgrp", "newgrp", to_char(v[1]), tmp, 0);
#else	/* Not V.4FS */
		execl("/bin/newgrp", "newgrp", to_char(v[1]), tmp, 0);
#endif	/* V4FS */
		execl("/usr/bin/newgrp", "newgrp", to_char(v[1]), tmp, 0);
	} else {
#ifdef V4FS
		execl("/usr/bin/newgrp", "newgrp", to_char(v[1]), 0);
#else	/* Not V.4FS */
		execl("/bin/newgrp", "newgrp", to_char(v[1]), 0);
#endif	/* V4FS */
		execl("/usr/bin/newgrp", "newgrp", to_char(v[1]), 0);
	}
	ununtty();
	untty();

#ifdef DEBUG_EXIT
  printf ("donewgrp (1): %d, Calling exit(1)\n", getpid ());
#endif

	exit(1);
}


#ifdef RFA
/*
 * This is added to give csh capability of accessing thru lan
 *
 * The code will prompt for a password that will not be echoed on
 * the user's terminal if the first occurence of a ':' in the
 * login string is at the end of the string.  This is to check
 * for the possibility of someone having a password with a : in it
 * and giving it to the netunam command.
 * i.e.  netunam /net/blah  login:passwd:
 *            Mike Shipley CNO  09/24/84
 */
#define MAXLOGIN	64

int  got_sigsys;

/**********************************************************************/
void
fault_netunam()
/**********************************************************************/
{
	got_sigsys = 1;
}

/**********************************************************************/
donetunam(v)
CHAR **v;
/**********************************************************************/
{
	extern int  errnet;
#ifdef NLS
	char	with_password[MAXLOGIN];
#endif
	char *password, *login_to_use;
	int 	original_length;
	char *getpass();
 	char *strchr();
	char *v2 = to_char(v[2]);
	int ret;
	void (*oldsig)();

	original_length = strlen(v2);
	if (original_length >= MAXLOGIN)
	{
		bferr((catgets(nlmsg_fd,NL_SETN,7, "name and password too long")));
		return;
	}
	if (strchr(v2, ':') != (v2 + (original_length - 1)) ) {
#ifdef NONLS
		login_to_use = v2;
#else
		strcpy(with_password, v2);
		login_to_use = with_password;
#endif
	}
	else
	{
		password = getpass((catgets(nlmsg_fd,NL_SETN,6, "Password:")));
		if (original_length + strlen(password) >= MAXLOGIN)
		{
			bferr((catgets(nlmsg_fd,NL_SETN,7, "name and password too long")));
			return;
		}
		strcpy(with_password, v2);
		strcpy(&with_password[original_length], password);
		login_to_use = with_password;
	}

	got_sigsys = 0;
	oldsig = sigset(SIGSYS, fault_netunam);
	ret = netunam(to_char(v[1]), login_to_use);
	sigset(SIGSYS, oldsig);
	if (got_sigsys)
	{
		printf(catgets(nlmsg_fd,NL_SETN,36, "network not supported\n"));
		return;
	}
	else {
	if (ret)
	{
		if (errno == ENET)
		{
			if (errnet == NE_NOLOGIN) {
				printf((catgets(nlmsg_fd,NL_SETN,8, "invalid remote login.  errnet = %d\n")),
					errnet);
				bferr((catgets(nlmsg_fd,NL_SETN,9, "network error")));

			}
			else if (errnet == NE_CONNLOST) {
				printf((catgets(nlmsg_fd,NL_SETN,10, "remote node not answering.  errnet = %d\n")), errnet);
				bferr((catgets(nlmsg_fd,NL_SETN,11, "network error")));
			}
			else {
				printf((catgets(nlmsg_fd,NL_SETN,12, "errnet = %d\n")), errnet);
				bferr((catgets(nlmsg_fd,NL_SETN,13, "network error")));
			}
		}
		else {
			if (errno == ENOENT) {
				printf((catgets(nlmsg_fd,NL_SETN,14, "invalid network special file.  errno = %d\n")), ENOENT);
			}
			else {
#ifndef NLS
				bferr(sys_errlist[errno]);
#else
				bferr(strerror(errno));
#endif
			}
		}
	}
	}
}
#endif /* RFA */

/**********************************************************************/
islogin()
/**********************************************************************/
{

	if (chkstop == 0 && setintr)
		panystop(0);
	if (loginsh)
		return;
	error((catgets(nlmsg_fd,NL_SETN,15, "Not login shell")));
}

/**********************************************************************/
doif(v, kp)
	CHAR **v;
	struct command *kp;
/**********************************************************************/
{
	register int i;
	register CHAR **vv;

	v++;
	i = exp(&v);

#ifdef DEBUG_IF_THEN_ELSE
  printf ("doif (1): %d, Value of expression: %d\n", getpid (), i);
#endif
	vv = v;
	if (*vv == NOSTR)
		bferr((catgets(nlmsg_fd,NL_SETN,16, "Empty if")));
	if (eq(*vv, "then")) {
		if (*++vv)
			bferr((catgets(nlmsg_fd,NL_SETN,17, "Improper then")));
		setname("then");
		/*
		 * If expression was zero, then scan to else,
		 * otherwise just fall into following code.
		 */
		if (!i)
		  {

#ifdef DEBUG_SEARCH
  printf ("doif (2): %d, Calling search for ZIF.\n", getpid ());
#endif
			search(ZIF, 0);
		  }
		return;
	}
	/*
	 * Simple command attached to this if.
	 * Left shift the node in this tree, munging it
	 * so we can reexecute it.
	 */
	if (i) {

#ifdef DEBUG_IF_THEN_ELSE
  printf ("doif (3): %d, Calling lshift.\n", getpid ());
#endif
		lshift(kp->t_dcom, vv - kp->t_dcom);

#ifdef DEBUG_IF_THEN_ELSE
  printf ("doif (4): %d, Calling reexecute.\n", getpid ());
#endif
		reexecute(kp);
		donefds();
	}
}

/*
 * Reexecute a command, being careful not
 * to redo i/o redirection, which is already set up.
 */
/*  Called by:
	dorepeat ()
	doif ()
*/
/**********************************************************************/
reexecute(kp)
	register struct command *kp;
/**********************************************************************/
{

	kp->t_dflg &= FSAVE;
	kp->t_dflg |= FREDO;
	/*
	 * If tty is still ours to arbitrate, arbitrate it;
	 * otherwise dont even set pgrp's as the jobs would
	 * then have no way to get the tty (we can't give it
	 * to them, and our parent wouldn't know their pgrp, etc.
	 */

#ifdef DEBUG_IF_THEN_ELSE
  printf ("reexecute (1): %d, Calling execute.\n", getpid ());
#endif
	execute(kp, tpgrp > 0 ? tpgrp : -1);
}

/**********************************************************************/
doelse()
/**********************************************************************/
{

#ifdef DEBUG_SEARCH
  printf ("doelse (1): %d, Calling search on ZELSE.\n", getpid ());
#endif
	search(ZELSE, 0);
}

/**********************************************************************/
dogoto(v)
	CHAR **v;
/**********************************************************************/
{
	register struct whyle *wp;
	CHAR *lp;

	/*
	 * While we still can, locate any unknown ends of existing loops.
	 * This obscure code is the WORST result of the fact that we
	 * don't really parse.
	 */
	for (wp = whyles; wp; wp = wp->w_next)
		if (wp->w_end == 0) {

#ifdef DEBUG_SEARCH
  printf ("dogoto (1): %d, Calling search for ZBREAK.\n", getpid ());
#endif
			search(ZBREAK, 0);
			wp->w_end = btell();
		} else
			bseek(wp->w_end);

#ifdef DEBUG_SEARCH
  printf ("dogoto (2): %d, Calling search for ZGOTO.\n", getpid ());
#endif
	search(ZGOTO, 0, lp = globone(v[1]));
	xfree(lp);
	/*
	 * Eliminate loops which were exited.
	 */
	wfree();
}

/**********************************************************************/
doswitch(v)
	register CHAR **v;
/**********************************************************************/
{
	register CHAR *cp, *lp;

	v++;
	if (!*v || *(*v++) != '(')
		goto syntax;
	cp = **v == ')' ? nullstr : *v++;
	if (*(*v++) != ')')
		v--;
	if (*v)
syntax:
		error((catgets(nlmsg_fd,NL_SETN,18, "Syntax error")));

#ifdef DEBUG_SEARCH
  printf ("doswitch (1): %d, Calling search for ZSWITCH.\n", getpid ());
#endif
	search(ZSWITCH, 0, lp = globone(cp));
	xfree(lp);
}

/**********************************************************************/
dobreak()
/**********************************************************************/
{

	if (whyles)
		toend();
	else
		bferr((catgets(nlmsg_fd,NL_SETN,19, "Not in while/foreach")));
}

/**********************************************************************/
doexit(v)
	CHAR **v;
/**********************************************************************/
{

	if (chkstop == 0)
		panystop(0);
	/*
	 * Don't DEMAND parentheses here either.
	 */
	v++;
	if (*v) {
		set(CH_status, putn(exp(&v)));
		if (*v)
			bferr((catgets(nlmsg_fd,NL_SETN,20, "Expression syntax")));
	}
	btoeof();
	if (intty)
		close(SHIN);
}

/**********************************************************************/
doforeach(v)
	register CHAR **v;
/**********************************************************************/
{
	register CHAR *cp;
	register struct whyle *nwp;

#ifdef TRACE_DEBUG
  printf ("doforeach (1): pid: %d, beginning doforeach\n", getpid ());
#endif
	v++;
	cp = strip(*v);
/*	while (*cp && letter(*cp))
		cp++;			DSDe409530: can be alpha-numeric */
	if (letter(*cp))
		while (alnum(*++cp));
	if (*cp || Strlen(*v) >= 20)
		bferr((catgets(nlmsg_fd,NL_SETN,21, "Invalid variable")));
	cp = *v++;
	if (v[0][0] != '(' || v[blklen(v) - 1][0] != ')')
		bferr((catgets(nlmsg_fd,NL_SETN,22, "Words not ()'ed")));
	v++;
	gflag = 0, rscan(v, tglob);
	v = glob(v);
	if (v == 0)
		bferr((catgets(nlmsg_fd,NL_SETN,23, "No match")));

/*  Insert at the beginning of the list.
*/
	nwp = (struct whyle *) calloc(1, sizeof *nwp);
	nwp->w_fe = nwp->w_fe0 = v; gargv = 0;
	nwp->w_start = btell();
	nwp->w_fename = savestr(cp);
	nwp->w_next = whyles;
	whyles = nwp;
	/*
	 * Pre-read the loop so as to be more
	 * comprehensible to a terminal user.
	 */
	if (intty)
		preread();
	doagain();
}

/**********************************************************************/
dowhile(v)
	CHAR **v;
/**********************************************************************/
{
	register int status;
	register bool again = whyles != 0 && whyles->w_start == lineloc &&
	    whyles->w_fename == 0;

	v++;
	/*
	 * Implement prereading here also, taking care not to
	 * evaluate the expression before the loop has been read up
	 * from a terminal.
	 */
	if (intty && !again)
		status = !exp0(&v, 1);
	else
		status = !exp(&v);
	if (*v)
		bferr((catgets(nlmsg_fd,NL_SETN,24, "Expression syntax")));
	if (!again) {
		register struct whyle *nwp = (struct whyle *) calloc(1, sizeof (*nwp));


/*  Insert at the beginning of the list.
*/
		nwp->w_start = lineloc;
		nwp->w_end = 0;
		nwp->w_next = whyles;
		whyles = nwp;
		if (intty) {
			/*
			 * The tty preread
			 */
			preread();
			doagain();
			return;
		}
	}
	if (status)
		/* We ain't gonna loop no more, no more! */
		toend();
}

/**********************************************************************/
preread()
/**********************************************************************/
{

	whyles->w_end = -1;
	if (setintr)
		sigrelse(SIGINT);

#ifdef DEBUG_SEARCH
  printf ("preread (1): %d, Calling search for ZBREAK.\n", getpid ());
#endif
	search(ZBREAK, 0);
	if (setintr)
		sighold(SIGINT);
	whyles->w_end = btell();
}

/**********************************************************************/
doend()
/**********************************************************************/
{

	if (!whyles)
		bferr((catgets(nlmsg_fd,NL_SETN,25, "Not in while/foreach")));
	whyles->w_end = btell();
	doagain();
}

/**********************************************************************/
docontin()
/**********************************************************************/
{

	if (!whyles)
		bferr((catgets(nlmsg_fd,NL_SETN,26, "Not in while/foreach")));
	doagain();
}

/**********************************************************************/
doagain()
/**********************************************************************/
{

	/* Repeating a while is simple */
	if (whyles->w_fename == 0) {
		bseek(whyles->w_start);
		return;
	}
	/*
	 * The foreach variable list actually has a spurious word
	 * ")" at the end of the w_fe list.  Thus we are at the
	 * of the list if one word beyond this is 0.
	 */
	if (!whyles->w_fe[1]) {
		dobreak();
		return;
	}
	set(whyles->w_fename, savestr(*whyles->w_fe++));
	bseek(whyles->w_start);
}

/*  Called by func () if a 'repeat' built-in command is seen.
*/
/**********************************************************************/
dorepeat(v, kp)
	CHAR **v;
	struct command *kp;
/**********************************************************************/
{
	register int i;

	i = getn(v[1]);
	if (setintr)
		sighold(SIGINT);
	lshift(v, 2);
	while (i > 0) {
		if (setintr)
			sigrelse(SIGINT);
		reexecute(kp);
		--i;
	}
	donefds();
	if (setintr)
		sigrelse(SIGINT);
}

/**********************************************************************/
doswbrk()
/**********************************************************************/
{


#ifdef DEBUG_SEARCH
  printf ("doswbrk (1): %d, Calling search for ZBRKSW.\n", getpid ());
#endif
	search(ZBRKSW, 0);
}

/*  Called by:

	search ()
	syn3 ()
*/
/*  Purpose:  Search through an array of structures, looking for a keyword
              that matches the string 'cp' which is passed in.  If a match
	      is found, return the corresponding value, a short.  For 
	      example, the 'value' associated with 'if' is ZIF, and that
	      associated with 'else' is ZELSE.
*/
/**********************************************************************/
srchx(cp)
	register CHAR *cp;
/**********************************************************************/
{
	register struct srch *sp;

	for (sp = srchn; sp->s_name; sp++)
		if (eq(cp, sp->s_name))
			return (sp->s_value);
	return (-1);
}

/**********************************  GLOBALS  ************************/
CHAR	Stype;
CHAR	*Sgoal;
/**********************************************************************/


/*VARARGS2*/
/**********************************************************************/
search(type, level, goal)
	int type;
	register int level;
	CHAR *goal;
/**********************************************************************/
{
	CHAR wordbuf[BUFSIZ];
	register CHAR *aword = wordbuf;
	register CHAR *cp;

	Stype = type; Sgoal = goal;
	if (type == ZGOTO)
		bseek(0l);
	do {
		if (intty && fseekp == feobp) {
			setup_tty(1);/* Extra characters appear after prompt
				      * from typeahead, so set echo off before
				      * outputing the prompt : DSDe410174 */
			printf("? "), flush();
		}
		aword[0] = 0, (void) getword(aword);

#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (1): %d, aword: %s\n", getpid (), to_char (aword));
#endif

/*  This routine looks through a global array of structures for keywords and
    their associated constants.  For example, the keyword 'else' is associated
    in these structures with the constant ZELSE.  If the keyword is found, 
    the value is returned.
*/
		switch (srchx(aword)) {

		case ZELSE:

#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (2): %d, case ZELSE, level: %d, type: %o\n", getpid (), level,
	   type);
#endif
			if (level == 0 && type == ZIF)
				return;
			break;

		case ZIF:
			while (getword(aword))
				continue;
			if ((type == ZIF || type == ZELSE) && eq(aword, "then"))
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (3): %d, ZIF: Incrementing level: %d\n", getpid (), level);
#endif
				level++;
			  }
			break;

		case ZENDIF:
			if (type == ZIF || type == ZELSE)
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (4): %d, ZENDIF: Decrementing level: %d\n", getpid (), level);
#endif
				level--;
			  }
			break;

		case ZFOREACH:
		case ZWHILE:
			if (type == ZBREAK)
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (5): %d, ZFOREACH | ZWHILE: Incrementing level: %d\n", 
	   getpid (), level);
#endif
				level++;
			  }
			break;

		case ZEND:
			if (type == ZBREAK)
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (6): %d, ZEND: Decrementing level: %d\n", getpid (), level);
#endif
				level--;
			  }
			break;

		case ZSWITCH:
			if (type == ZSWITCH || type == ZBRKSW)
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (7): %d, ZSWITCH: Incrementing level: %d\n", getpid (), 
	   level);
#endif
				level++;
			  }
			break;

		case ZENDSW:
			if (type == ZSWITCH || type == ZBRKSW)
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (8): %d, ZENDSW & (ZSWITCH | ZBRKWS): Decr level: %d\n", 
	  getpid (), level);
#endif
				level--;
			  }
			break;

		case ZLABEL:
			if (type == ZGOTO && getword(aword) 
						   && Strcmp(aword, goal) == 0)
                          {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (9): %d, ZLABEL: Setting level to -1\n", getpid ());
#endif
				level = -1;
                          }
			break;

		default:

#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (10): %d, case default, level: %d, type: %o\n", getpid (), 
	   level, type);
  printf ("\tZGOTO: %o, ZSWITCH: %o \n", ZGOTO, ZSWITCH);
#endif
			if (type != ZGOTO && (type != ZSWITCH || level != 0))
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (11): %d, breaking due to ZGOTO, ZSWITCH, or level\n", 
	   getpid ());
#endif
				break;
			  }

			if (lastchr(aword) != ':')
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (12): %d, breaking due to lastchr of aword not ':'\n", 
	  getpid ());
#endif
				break;
			  }

			aword[Strlen(aword) - 1] = 0;

#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (13): %d, changed aword: %s\n", getpid (), to_char (aword));
#endif
			if (type == ZGOTO && Strcmp(aword, goal) == 0 
				     || type == ZSWITCH && eq(aword, "default"))
                          {

#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (14): %d, setting level to -1\n", getpid ());
#endif
				level = -1;
                          }
			break;

		case ZCASE:
			if (type != ZSWITCH || level != 0)
				break;
			(void) getword(aword);
			if (lastchr(aword) == ':')
				aword[Strlen(aword) - 1] = 0;
			cp = strip(Dfix1(aword));
			if (Gmatch(goal, cp))
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (15): %d, ZCASE: Setting level to -1\n", getpid ());
#endif
				level = -1;
			  }
			xfree(cp);
			break;

		case ZDEFAULT:
			if (type == ZSWITCH && level == 0)
			  {
#ifdef DEBUG_IF_THEN_ELSE
  printf ("search (16): %d, ZDEFAULT: Setting level to -1\n", getpid ());
#endif
				level = -1;
			  }
			break;
		}
		(void) getword(NOSTR);
	} while (level >= 0);
}

/*  Called by:

	search ()
*/
/**********************************************************************/
getword(wp)
	register CHAR *wp;
/**********************************************************************/
{
/*  Note:  wp is either a pointer to a buffer where the input will be put,
           or 0.
*/
	register int found = 0;
	register int c, d, tc;
	static CHAR eohere[BUFSIZ] = {0};	/* init explicitly! */
	CHAR hereword[BUFSIZ];

#ifdef DEBUG_SEARCH
  CHAR *debugWp = wp;
#endif

/*  This tells readc that we want to know about EOFs.
*/
	c = readc(1);
	d = 0;

/*  Do this loop while we aren't saving any input characters.  This will
    a single iteration loop in the case where we ARE saving characters.
*/
	do {

/*  Skip white space in all cases.  Skip open parentheses if we are saving
    the input (i.e. wp is not 0).
*/
#ifndef NLS
		while (c == ' ' || c == '\t' || (c == '(' && wp))
#else
		while (c == ' ' || c == '\t' || c == ALT_SP || (c == '(' && wp))
#endif
			c = readc(1);

/*  Skip comment lines; read characters till a newline is found.
*/
		if (c == '#')
			do
				c = readc(1);
			while (c >= 0 && c != '\n');

/*  If we got back a negative number, then we're out of data.  This is an
    error condition.  The label 'post' decides what erro occurs and causes
    the appropriate error message to be printed.
*/
		if (c < 0)
			goto past;

/*  If we get a newline, break this loop if we are saving characters.  If we
    are not saving characters then return to the calling routine.
*/
		if (c == '\n') {
			if (wp)
				break;

#ifdef DEBUG_SEARCH
  printf ("getword (1): %d, Saw newline and not saving, so returning.\n", 
	   getpid());
#endif

			return (0);
		}

		unreadc(c);
		found = 1;

/*  Do this loop while:

	( we are looking for a matching quote character 
	OR
        we don't see any white space 
	AND 
	( we are not saving characters OR we don't see open parentheses ))
	AND
	we don't see a newline
*/
		do {
			c = readc(1);

/*  If we get a '<' character, and are 
 *      - not inside a string, and
 *      - not already inside a here-document
 *  we may have a here-document.
 */
			if (c == '<' && !d && !eohere[0]) {

/*  If the next character is not '<', we do not have a here-document.
 */
				if ((tc = readc(1)) != '<')
					unreadc(tc);

/*  Otherwise, we have a here-document -- skip over it.
 */
				else {

/*  Clear c so we do not return '<' as part of a word.
 */
					c = 0;

/*  Get and save the string that marks the end of the here-document and
 *  ignore everything else on the line.
 */
					if (!getword(eohere))
						bferr((catgets(nlmsg_fd,NL_SETN,6, "Missing name for redirect")));
						
#ifdef DEBUG_SEARCH
  printf ("getword (3): %d, Scanning here-document terminated by %s\n", 
	   getpid(), to_char (eohere));
#endif

/*  Now, skip input until we find the string that marks the end of the
 *  here-document.
 */
					do {
						(void) getword(NOSTR);
					} while (getword(hereword) &&
							!Eq(eohere, hereword));
					eohere[0] = 0;
				}
			}

/*  If we get an escaped newline, turn it into a space.
*/
			if (c == '\\' && (c = readc(1)) == '\n')
				c = ' ';

/*  If we get a quote character, remember it.  However, if we are already
    remembering one, if this is the same then no need to remember anymore
    since we found the match.
*/
			if (any(c, "'\""))
				if (d == 0)
					d = c;
				else if (d == c)
					d = 0;

/*  If we are out of data then this is an error like above.  The 'post' label
    figures out which error and causes an error message to be printed.
*/
			if (c < 0)
				goto past;

/*  FINALLY, save the character if we are saving input.
*/
			if (wp)
				*wp++ = c;
#ifndef NLS
		} while ((d || c != ' ' && c != '\t' && (wp == 0 || c != '(')) && c != '\n');
#else
		} while ((d || c != ' ' && c != '\t' && c != ALT_SP
			 && (wp == 0 || c != '(')) && c != '\n');
#endif
	} while (wp == 0);

	unreadc(c);

/*  If we read input, then terminate the saved input with a NULL.
*/
	if (found)
		*--wp = 0;

#ifdef DEBUG_SEARCH
  printf ("getword (2): %d, word being returned: %s\n", getpid(), 
	  to_char (debugWp));
#endif

	return (found);

/*  This figures out error conditions.  Stype is a global that was set
    by the search() routine.
*/
past:
	switch (Stype) {

	case ZIF:
		bferr((catgets(nlmsg_fd,NL_SETN,27, "then/endif not found")));

	case ZELSE:
		bferr((catgets(nlmsg_fd,NL_SETN,28, "endif not found")));

	case ZBRKSW:
	case ZSWITCH:
		bferr((catgets(nlmsg_fd,NL_SETN,29, "endsw not found")));

	case ZBREAK:
		bferr((catgets(nlmsg_fd,NL_SETN,30, "end not found")));

	case ZGOTO:
		setname(to_char(Sgoal));
		bferr((catgets(nlmsg_fd,NL_SETN,31, "label not found")));
	}
	/*NOTREACHED*/
}

/**********************************************************************/
toend()
/**********************************************************************/
{

	if (whyles->w_end == 0) {

#ifdef DEBUG_SEARCH
  printf ("toend (1): %d, Calling search for ZBREAK.\n", getpid ());
#endif
		search(ZBREAK, 0);
		whyles->w_end = btell() - 1;
	} else
		bseek(whyles->w_end);
	wfree();
}

/*  Called by:
	dogoto ()
	toend ()
	btoeof ()
*/
/**********************************************************************/
wfree()
/**********************************************************************/
{
/*  btell () returns the pointer into the input buffers (fseekp).
*/
	long o = btell();

	while (whyles) {
		register struct whyle *wp = whyles;
		register struct whyle *nwp = wp->w_next;

		if (o >= wp->w_start && (wp->w_end == 0 || o < wp->w_end))
			break;
		if (wp->w_fe0)
			blkfree(wp->w_fe0);
		if (wp->w_fename)
			xfree(wp->w_fename);
		xfree((CHAR *)wp);
		whyles = nwp;
	}
}

/**********************************************************************/
doecho(v)
	CHAR **v;
/**********************************************************************/
{

	echo(' ', v);
}

/**********************************************************************/
doglob(v)
	CHAR **v;
/**********************************************************************/
{

	echo(0, v);
	flush();
}

/**********************************************************************/
echo(sep, v)
	CHAR sep;
	register CHAR **v;
/**********************************************************************/
{
	register CHAR *cp;
	int nonl = 0;

	if (setintr)
		sigrelse(SIGINT);
	v++;
	if (*v == 0)
		return;
	gflag = 0; rscan(v, tglob);
	if (gflag) {
		v = glob(v);
		if (v == 0)
			bferr((catgets(nlmsg_fd,NL_SETN,32, "No match")));
	} else
		scan(v, trim);
	if (sep == ' ' && eq(*v, "-n"))
		nonl++, v++;
	if (sep == ' ' && eq(*v, "-N"))
		sep = '\n', v++;

	while (cp = *v++) {
		register unsigned int c;

#ifdef DEBUG_ECHO
  printf ("echo (1): %d, cp: %s\n", getpid (), to_char (cp));
#endif

		while (c = *cp++) {
			if (sep == ' ' && *cp && (c&TRIM) == '\\') {

#ifdef DEBUG_ECHO
  printf ("echo (2): %d, found a backslash\n", getpid ());
#endif
				c = *cp++;
				if ((c&TRIM) == 'c') {
					flush();
					return;
				} else if ((c&TRIM) == 'n')
				    {
#ifdef DEBUG_ECHO
  printf ("echo (3): %d, found a newline\n", getpid ());
#endif
					c = '\n';
				    }
				else
					putchar('\\');
			}
#ifndef NLS16
			putchar(c | QUOTE);
#else
			/* put out both bytes, 1 at a time */
			if (c & KMASK) {
				putchar(((c >> 8) & 0377) | 0200);
				putchar((c&0377) | QUOTE);
			}
			else putchar(c | QUOTE);
#endif
		}
		if (*v)
			putchar(sep | QUOTE);
	}
	if (sep && nonl == 0)
		putchar('\n');
	else
		flush();
	if (setintr)
		sighold(SIGINT);
	if (gargv)
		blkfree(gargv), gargv = 0;
}

char 	**environ;
int	env_spare = 0;		/* no. of extra pointers in environ */
#ifndef NONLS
CHAR	**Environ = 0;		/* added this copy (CHAR version of environ) */
#endif

/**********************************************************************/
dosetenv(v)
	register CHAR **v;
/**********************************************************************/
{
	CHAR *lp = globone(v[2]);
	CHAR *ep;

#ifdef NLS16
 	/* If name is LANG, set new locale */
 	if (eq(v[1], "LANG"))
 		setlocale(LC_ALL, to_char(lp));

  	/* If name is any of the LC_* vairables, set new locale categary */
 	if (eq(v[1], "LC_COLLATE"))
 		setlocale(LC_COLLATE, to_char(lp));
 	if (eq(v[1], "LC_CTYPE"))
 		setlocale(LC_CTYPE, to_char(lp));
 	if (eq(v[1], "LC_MONETARY"))
 		setlocale(LC_MONETARY, to_char(lp));
 	if (eq(v[1], "LC_NUMERIC"))
 		setlocale(LC_NUMERIC, to_char(lp));
 	if (eq(v[1], "LC_TIME"))
 		setlocale(LC_TIME, to_char(lp));
#endif
	setenv(v[1], lp);

#ifdef NLS
	if (eq(v[1], "LANG")) {		/* If name is LANG, open new catalog */
		if (nlmsg_fd != -1)
			close(nlmsg_fd);	/* close current catalog file */
		nlmsg_fd=catopen("csh");
		if (nlmsg_fd != -1)
			fcntl(nlmsg_fd, F_SETFD, 1);    /* set close-on-exec flag*/
	}
#endif

	if (eq(v[1], "PATH")) {
		importpath(lp);
		dohash();
	}
	xfree(lp);
}

/**********************************************************************/
dounsetenv(v)
	register CHAR **v;
/**********************************************************************/
{

	v++;
	do {

#ifdef NLS
	if (eq(*v, "LANG")) {
		if (nlmsg_fd != -1) {
			close(nlmsg_fd);	/* close current catalog file */
			nlmsg_fd = -1;
		}
	}
#endif
#ifdef NLS16
 	/* If name is LANG, reset locale */
	if (eq(*v, "LANG"))
 		setlocale(LC_ALL, "");

  	/* If name is any of the LC_* vairables, reset locale categary */
 	if (eq(*v, "LC_COLLATE"))
 		setlocale(LC_COLLATE, "");
 	if (eq(*v, "LC_CTYPE"))
 		setlocale(LC_CTYPE, "");
 	if (eq(*v, "LC_MONETARY"))
 		setlocale(LC_MONETARY, "");
 	if (eq(*v, "LC_NUMERIC"))
 		setlocale(LC_NUMERIC, "");
 	if (eq(*v, "LC_TIME"))
 		setlocale(LC_TIME, "");
#endif
		unsetenv(*v++);
	} while (*v);
}

#ifndef NONLS
CHAR CH_equal[] = {'=',0};
#else
#define CH_equal	"="
#endif
static int first=1;

/**********************************************************************/
setenv(name, value)
	CHAR *name, *value;
/**********************************************************************/
{
/*  This routine is fairly complicated with NLS versus without NLS.
    Therefore the comments have been according to which path is taken.
    Lines beginning with '*+' are comments for blocks of code that execute
    whether or not NLS is used, those beginning with '+' execute if no
    NLS is used, and those beginning with '*' execute if NLS is used.
*/

/*  The environment is an array of pointers to strings.
*+  Initially Environ points to nothing.  Set 'ep' to point to environ.  
*/

	register CHAR **ep = (CHAR **)environ;
	register CHAR *cp, *dp;
	CHAR *blk[2];
	CHAR *tempPtr;
	char **oldenviron;
#ifndef NONLS
	char **nep;
#endif

#ifdef DEBUG_SETENV
  printf ("setenv (1): %d, name arg: %s\n", getpid (), to_char (name));
  printf ("\tvalue arg: %s\n", to_char (value));
#endif

	if (first==1)
	{

#ifndef NONLS
/*  THIS HAPPENS WITH NLS:
*
*    (assign value to Environ)
*
*    blk_to_short first callocs space for the size of the array of pointers.
*    It then calls savestr which callocs space for the string, converts the
*    string to shorts and stores them in the new space with Strcpy.  The
*    result here is a whole new copy of the environment, stored as shorts.  
*    It is pointed to by ep and Environ.  Note that environ still points
*    to the original environment.
*/

	    ep = Environ = blk_to_short(environ);

#endif

/*  THIS ALWAYS HAPPENS:
*+    
*+    Step through the short array.  Strspl callocs memory the size of 
*+    both strings, copies both into the new memory and returns a pointer to the
*+    new memory.  A temporary pointer is used so that the original memory
*+    can be freed after the copy.  xfree frees the string pointed to by
*+    tempPtr.
*/

/*
*+    The following doesn't seem necessary whether we're using NLS or not.
	    for (; *ep; ep++)
	      {
		tempPtr = *ep;
		*ep = Strspl(tempPtr,nullstr);
		xfree (tempPtr);
	      }
*/

	    blk[0] = blk[1] = 0;

#ifdef NONLS
/*  THIS DOESN'T HAPPEN WITH NLS:
+    
+    blkspl_spare callocs space for both sets of pointers, then copies all the
+    pointers into the new block and returns a pointer to the new block. 
*/

	    environ = blkspl_spare(environ, blk);
	    ep = environ;

#else
/*  THIS HAPPENS WITH NLS:
*    
*    blkspl_spare callocs space for both sets of pointers, then copies all the 
*    pointers into the new block and returns a pointer to the new block. 
*/

	    Environ = blkspl_spare(Environ, blk);
	    environ = blkspl_spare2(environ, blk);

#endif

	    first=0;

	}
#ifndef NONLS
/*  THIS HAPPENS WITH NLS:
*    
*    Set ep to point to Environ. 
*/

	ep = Environ;

#endif
/*  THIS ALWAYS HAPPENS:
*+    
*+    Step through all of the environment strings in Environ or environ.  The 
*+    outermost for loop goes through the names.  The inner for loop goes 
*+    through each character of the name.  It steps through the name as long 
*+    as there are characters in the name and they match the target name.  When
*+    this is no longer the case, if we're not at the end of the target name 
*+    and the environment name isn't pointing to '=', then skip to the next 
*+    entry in the environment.  Note that if this is a new environment 
*+    variable, there won't be a match and this loop will just go through the 
*+    entire environment list.
*/

	for (ep; *ep; ep++) {

#ifdef DEBUG_SETENV
  printf ("setenv (2): %d, *ep: %s\n", getpid (), to_char (*ep));
#endif

		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;

/*  THIS ALWAYS HAPPENS:
*+    
*+    If there is a match between the target name and the environment name, this
*+    happens.
*+
*+    Strspl callocs memory for both strings and then uses Strcpy and Strcat
*+    to copy '=' and <value> into it.  It returns a pointer to the new memory.
*+
*+    Next the whole environment entry is freed: <name>=<old value>.
*+
*+    Strspl is again used to allocate memory for the environment name and value
*+    and this information is copied into the new memory and the environment
*+    pointer set to point to it: <name>=<new value>
*+
*+    Finally the memory used to temporarily store the '='<new value> is freed.
*+
*+    The scan routine is used to replace each character in the final string
*+    with the character ANDed with 077777.
*/

		cp = Strspl(CH_equal, value);
		xfree(*ep);
		*ep = Strspl(name, cp);
		xfree(cp);
		scan1(*ep, trim);

#ifndef NONLS

#ifdef DEBUG_SETENV
  printf ("setenv (3): %d, IFnDEF NONLS; blk_to_char (Environ)\n", getpid ());
#endif

/*  THIS HAPPENS WITH NLS:
*    
*    Index into the environ array an amount equivalent to what was used
*    to index into Environ.  Free up the memory at that point.
*    Then call savebyte to calloc space for the new string and
*    copy the character version of the string into the new space.
*/

		nep = environ + (ep - Environ);
		xfree(*nep);
		*nep = savebyte(to_char(*ep));

#endif

#ifdef DEBUG_SETENV
  printf ("setenv (4): %d, exiting\n", getpid ());
#endif

/*  At this point the routine returns.
*/
		return;
	}

/*  THIS ALWAYS HAPPENS:
*+    
*+    If there wasn't a match with the environment, the name must not exist
*+    there.  So Strspl is used to calloc space for the new name, '='
*+    and the value.
*+
*+    blk[0] -> <name>=<value>
*/

	cp = Strspl(name, CH_equal);
	blk[0] = Strspl(cp, value); blk[1] = 0;
	xfree(cp);
	scan1(blk[0], trim);

#ifdef NONLS

#ifdef DEBUG_SETENV
  printf ("setenv (5): %d, IFDEF NONLS; blkspl (environ)\n", getpid ());
#endif

/*  THIS DOESN'T HAPPEN WITH NLS:
+    
+    blkspl_spare is used to copy blk to the end of environ.  If there are
+    no spare entries in environ, we calloc a chunk of memory that
+    is big enough to hold both, plus some spares, and then copy
+    both into the newly allocated space.
*/

	environ = blkspl_spare(environ, blk);

#else

#ifdef DEBUG_SETENV
  printf ("setenv (6): %d, IFnDEF NONLS; copying [Ee]nviron\n", getpid ());
#endif

/*  THIS HAPPENS WITH NLS:
*    
* (need to update both copies)
*
*    blkspl_spare is used to copy blk to the end of Environ.  If there are
*    no spare entries in Environ, we calloc a chunk of memory that
*    is big enough to hold both, plus some spares, and then copy
*    both into the newly allocated space.
*
*    If blkspl_spare allocated new memory for Environ, then we call
*    blkspl_spare2 to calloc space for a whole new array of pointers
*    and then copy the old environ to the newly allocated memory,
*    and also append the new string.
*/

	ep = Environ;
	Environ = blkspl_spare(Environ, blk);

	blk[0] = savebyte(to_char(blk[0]));
	if (ep != Environ) /* must have allocated a new array in blkspl_spare */
		environ = blkspl_spare2(environ, blk);
	else 
		blkcat(environ, blk);
		

#endif

#ifdef DEBUG_SETENV
  printf ("setenv (7): %d, exiting setenv\n", getpid());
#endif

}

/**********************************************************************/
unsetenv(name)
	CHAR *name;
/**********************************************************************/
{
/*  This routine is fairly complicated with NLS versus without NLS.
    Therefore the comments have been according to which path is taken.
    Lines beginning with '*+' are comments for blocks of code that execute
    whether or not NLS is used, those beginning with '+' execute if no
    NLS is used, and those beginning with '*' execute if NLS is used.
*/

/*  environ is an array of pointers to strings:
*/

	register CHAR **ep = (CHAR **)environ;
	register CHAR *cp, *dp;
	CHAR **oep = ep;

	if (first) {
#ifndef NONLS
/*  THIS HAPPENS WITH NLS:
*    
*    Copy the entire environ pointers and strings to shorts if we have
*    not already done so.
*/

		Environ = blk_to_short(environ);

#endif
		first = 0;
	}

#ifndef NONLS
	ep = Environ;
#endif

/*  THIS ALWAYS HAPPENS:
*+    
*+    Find the entry in environ or Environ that points to the string of
*+    interest.
*/

	for (; *ep; ep++) {
		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;
		cp = *ep;

/*  THIS ALWAYS HAPPENS:
*+
*+    Move the pointers from ep + 1 to the end of the block,
*+    up one position.  Increment the spare counter.  Free the
*+    string that has been removed.
*/ 
		for (oep = ep; *oep = *(oep + 1); oep++);
		env_spare++;
		xfree(cp);

#ifndef NONLS
/*  THIS HAPPENS WITH NLS:
*    
*   Fix up the environ array to remove the corresponding character
*   version of the string being deleted.
*/

		cp = *(oep = environ + (ep - Environ));
		for ( ;	*oep = *(oep + 1); oep++);
		xfree(cp);

#endif
		return;
	}
}

/**********************************************************************/
doumask(v)
	register CHAR **v;
/**********************************************************************/
{
	register CHAR *cp = v[1];
	register int i;

	if (cp == 0) {
		i = umask(0);
		umask(i);
		printf("%o\n", i);
		return;
	}
	i = 0;
	while (digit(*cp) && *cp != '8' && *cp != '9')
		i = i * 8 + *cp++ - '0';
	if (*cp || i < 0 || i > 0777)
		bferr((catgets(nlmsg_fd,NL_SETN,33, "Improper mask")));
	umask(i);
}

#ifdef VLIMIT
#include <sys/vlimit.h>

struct limits {
	int	limconst;
	char	*limname;
	int	limdiv;
	char	*limscale;
} limits[] = {
	LIM_NORAISE,	"noraise",	1,	"",
	LIM_CPU,	"cputime",	1,	"seconds",
	LIM_FSIZE,	"filesize",	1024,	"kbytes",
	LIM_DATA,	"datasize",	1024,	"kbytes",
	LIM_STACK,	"stacksize",	1024,	"kbytes",
	LIM_CORE,	"coredumpsize",	1024,	"kbytes",
	LIM_MAXRSS,	"memoryuse",	1024,	"kbytes",
	-1,		0,
};

/**********************************************************************/
struct limits *
findlim(cp)
	CHAR *cp;
/**********************************************************************/
{
	register struct limits *lp, *res;

	res = 0;
	for (lp = limits; lp->limconst >= 0; lp++)
		if (prefix(cp, lp->limname)) {
			if (res)
				bferr("Ambiguous");
			res = lp;
		}
	if (res)
		return (res);
	bferr("No such limit");
	return ((struct limits *) 0);
}

/**********************************************************************/
dolimit(v)
	register CHAR **v;
/**********************************************************************/
{
	register struct limits *lp;
	register int limit;

	v++;
	if (*v == 0) {
		for (lp = limits+1; lp->limconst >= 0; lp++)
			plim(lp);
		if (vlimit(LIM_NORAISE, -1) && getuid())
			printf("Limits cannot be raised\n");
		return;
	}
	lp = findlim(v[0]);
	if (v[1] == 0) {
		plim(lp);
		return;
	}
	limit = getval(lp, v+1);
	setlim(lp, limit);
}

/**********************************************************************/
getval(lp, v)
	register struct limits *lp;
	CHAR **v;
/**********************************************************************/
{
	register float f;
	double atof();
	CHAR *cp = *v++;

	f = atof(to_char(cp));
	while (digit(*cp) || *cp == '.' || *cp == 'e' || *cp == 'E')
		cp++;
	if (*cp == 0) {
		if (*v == 0)
			return ((int)(f+0.5) * lp->limdiv);
		cp = *v;
	}
	if (lp->limconst == LIM_NORAISE)
		goto badscal;
	switch (*cp) {

	case ':':
		if (lp->limconst != LIM_CPU)
			goto badscal;
		return ((int)(f * 60.0 + atof(to_char(cp+1))));

	case 'h':
		if (lp->limconst != LIM_CPU)
			goto badscal;
		limtail(cp, "hours");
		f *= 3600.;
		break;

	case 'm':
		if (lp->limconst == LIM_CPU) {
			limtail(cp, "minutes");
			f *= 60.;
			break;
		}
	case 'M':
		if (lp->limconst == LIM_CPU)
			goto badscal;
		*cp = 'm';
		limtail(cp, "megabytes");
		f *= 1024.*1024.;
		break;

	case 's':
		if (lp->limconst != LIM_CPU)
			goto badscal;
		limtail(cp, "seconds");
		break;

	case 'k':
		if (lp->limconst == LIM_CPU)
			goto badscal;
		limtail(cp, "kbytes");
		f *= 1024;
		break;

	case 'u':
		limtail(cp, "unlimited");
		return (INFINITY);

	default:
badscal:
		bferr("Improper or unknown scale factor");
	}
	return ((int)(f+0.5));
}

/**********************************************************************/
limtail(cp, str0)
	CHAR *cp, *str0;
/**********************************************************************/
{
	register CHAR *str = str0;

	while (*cp && *cp == *str)
		cp++, str++;
	if (*cp)
		error("Bad scaling; did you mean ``%s''?", str0);
}

/**********************************************************************/
plim(lp)
	register struct limits *lp;
/**********************************************************************/
{
	register int lim;

	printf("%s \t", lp->limname);
	lim = vlimit(lp->limconst, -1);
	if (lim == INFINITY)
		printf("unlimited");
	else if (lp->limconst == LIM_CPU)
		psecs((long)lim);
	else
		printf("%d %s", lim / lp->limdiv, lp->limscale);
	printf("\n");
}

/**********************************************************************/
dounlimit(v)
	register CHAR **v;
/**********************************************************************/
{
	register struct limits *lp;

	v++;
	if (*v == 0) {
		for (lp = limits+1; lp->limconst >= 0; lp++)
			setlim(lp, INFINITY);
		return;
	}
	while (*v) {
		lp = findlim(*v++);
		setlim(lp, INFINITY);
	}
}

/**********************************************************************/
setlim(lp, limit)
	register struct limits *lp;
/**********************************************************************/
{

	if (vlimit(lp->limconst, limit) < 0)
		Perror(bname);
}

#endif /* VLIMIT */
#ifdef SIGTSTP

/**********************************************************************/
dosuspend()
/**********************************************************************/
{
	int ldisc;
	int ctpgrp;
	void (*old)();

	if (loginsh)
		error((catgets(nlmsg_fd,NL_SETN,34, "Can't suspend a login shell (yet)")));
	untty();
	old = sigsys(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);
	/* the shell stops here */
	sigsys(SIGTSTP, old);
	if (tpgrp != -1) {
retry:
		ioctl(FSHTTY, TIOCGPGRP, &ctpgrp);
		if (ctpgrp != opgrp) {
			old = sigsys(SIGTTIN, SIG_DFL);
			kill(0, SIGTTIN);
			sigsys(SIGTTIN, old);
			goto retry;
		}
		setpgrp(0, shpgrp);
		ioctl(FSHTTY, TIOCSPGRP, &shpgrp);
	}
}
#endif

/**********************************************************************/
doeval(v)
	CHAR **v;
/**********************************************************************/
{
	CHAR **oevalvec = evalvec;
	CHAR *oevalp = evalp;
	jmp_buf osetexit;
	int reenter;
	CHAR **gv = 0;

	v++;
	if (*v == 0)
		return;
	gflag = 0; rscan(v, tglob);
	if (gflag) {
		gv = v = glob(v);
		gargv = 0;
		if (v == 0)
			error((catgets(nlmsg_fd,NL_SETN,35, "No match")));
		v = copyblk(v);
	} else
		scan(v, trim);
	getexit(osetexit);
	reenter = (setexit() != 0) + 1;
	if (reenter == 1) {
		evalvec = v;
		evalp = 0;
		process(0);
	}
	evalvec = oevalvec;
	evalp = oevalp;
	doneinp = 0;
	if (gv)
		blkfree(gv);
	resexit(osetexit);
	if (reenter >= 2)
		error((char *) 0);
}
