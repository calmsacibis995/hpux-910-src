
/* @(#) $Revision: 72.2 $ */      
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"
#include	"timeout.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include        "dup.h"
#include	<unistd.h>
#ifdef SIGWINCH
#include	<termio.h>
#endif /*SIGWINCH*/

#ifdef NLS || NLS16
#include	<locale.h>
#endif NLS || NLS16

#ifndef NULL
#define NULL 0
#endif NULL

#ifdef RES
#include	<sgtty.h>
#endif

static BOOL	beenhere = FALSE;
char		tmpout[20] = "/tmp/sh-";
struct fileblk	stdfile;
struct fileblk *standin = &stdfile;
int mailchk = 0;
int login_sh = 0;  /* login shell flag */
int suid_flag = 0; /* the we've-been-suid'd flag */

static tchar	*mailp;		
static long	*mod_time = 0;

#if vax
char **execargs = (char**)(0x7ffffffc);
#endif
#if pdp11
char **execargs = (char**)(-2);
#endif

extern int	exfile();
extern tchar 	*simple();
extern int 	_nl_space_alt;
tchar	rshell[] 	= {'-','r','s','h',0};	/* needed for fix to bell bug */
int cheat;

struct fdsave *fdmap;


#ifdef TREEDEBUG
int dbg_fd = -1;	/* File descriptor opened for debug output: /dev/tty */
char dbg_buf[300];
#endif


main(c, v, e)
int	c;
char	*v[];
char	*e[];	
{
	register int	rflag = ttyflg;
	int		rsflag = 1;	/* local restricted flag */
	struct tnamnod	*n;
	sigset_t blocked_sigs;          /* set of sigs blocked from this sh */
#ifndef NLS
#define tv	v
#define te	e
#else NLS
	int cnt, len;
	char mailval[1024];
	tchar **tv;
	tchar **te;
#endif NLS

/* The following section of code is to fix the bug where sh would not unblock*/
/* SIGSEGV, and would therefore go into an infinite loop when more memory    */
/* needed to be allocated.  I'm unblocking SIGSEGV (and SIGCLD and SIGSYS    */
/* too, since these three signals are trapped by the shell into its signal   */
/* handler) so that the shell will get delivery of these signals.            */

	if(sigemptyset(&blocked_sigs)) {
		error((nl_msg(661, "sigemptyset() failed")));
	}
	if(sigprocmask(-1, (sigset_t *)0, &blocked_sigs)) {
		error((nl_msg(662, "sigprocmask() failed")));
	}
	if(sigdelset(&blocked_sigs, SIGSEGV)) {
		error((nl_msg(663, "sigdelset() failed")));
	}
	if(sigdelset(&blocked_sigs, SIGCLD)) {
		error((nl_msg(663, "sigdelset() failed")));
	}
	if(sigdelset(&blocked_sigs, SIGSYS)) {
		error((nl_msg(663, "sigdelset() failed")));
	}
	if(sigprocmask(SIG_SETMASK, &blocked_sigs, (sigset_t *)0)) {
		error((nl_msg(662, "sigprocmask() failed")));
	}

/* End of bug fix for SIGSEGV infinite loop. */
	
	stdsigs();

	/*
	 * initialise storage allocation
	 */

	stakbot = 0;
	addblok((unsigned)0);

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		prs(_errlocale("sh"));
	}
	sprintf(langpath,"%s%s/sh.cat",NLSDIR,getenv("LANG"));

	/* Arguments */
	cnt = blklen(v);
	len = (cnt + 1)*sizeof(tchar *);
	for (cnt--; cnt >= 0; cnt--)
		len += (length(v[cnt])+1)*sizeof(tchar);
	tv = (tchar **)malloc(round(len+BYTESPERWORD, BYTESPERWORD));
	blk_to_tchar(v, tv);

	/* Environment */
	cnt = blklen(e);
	len = (cnt + 1)*sizeof(tchar *);
	for (cnt--; cnt >= 0; cnt--)
		len += (length(e[cnt])+1)*sizeof(tchar);
	te = (tchar **)malloc(round(len+BYTESPERWORD, BYTESPERWORD));
	blk_to_tchar(e, te);
#endif NLS || NLS16

	/*
	 * Determine if this shell is a login shell
	 */

	if (*(simple(tv[0])) == '-')
            login_sh++;

	/*
	 * Determine if we're running in a suid environment.  If we are,
         * then we'll later skip execution of the user's $HOME/.profile
         * file for security reasons.
	 */

         if( (getuid() != geteuid()) || (getgid() != getegid()) )
             suid_flag++;


	/*
	 * Initialize -c arg pointer
	 */

	comdiv = 0;

	/*
	 * set names from userenv
	 */

	setup_env(te);

	/* Set up array dependent on number of file descriptors */

	fdmap=(struct fdsave *)malloc(sizeof(struct fdsave)*sysconf(_SC_OPEN_MAX));
	memset(fdmap,0,sizeof(struct fdsave)*sysconf(_SC_OPEN_MAX));

	/*
	 * 'rsflag' is non-zero if SHELL variable is
	 *  set in environment and contains an'r' in
	 *  the simple file part of the value.
	 */

	if (n = findnam(shell))
	{
	/*	
	 * fix bell bug now explicitly check for rsh rather than 'r' 
	 * all other references to the rshell variable are assoc. with 
	 * this fix.  The preceeding comment is no longer true.  
	 * mn
	 */
	 	if (eqtt(rshell+1, simple(n->namval))) 
			rsflag = 0;
	}

	/*
	 * a shell is also restricted if argv(0) has
	 * "rsh" or "-rsh" as its simple name
	 */

#ifndef RES

	if (c > 0 && (eqtt(rshell+1, simple(*tv)) || eqtt(rshell, simple(*tv))))
		rflag = 0;

#endif

	hcreate();
	set_dotpath();

	/*
	 * look for options
	 * dolc is $#
	 */
	if ((*(tv[0]) == '-') && ((c > 1) && (eqtt(tv[0], tv[1])))) { 
		cheat = 1;
	}
	else {
		cheat = 0;
	}
	dolc = options(c, tv);	

	if (dolc < 2)
	{
		flags |= stdflg;
		{
			register char *flagc = flagadr;

			while (*flagc)
				flagc++;
			*flagc++ = STDFLG;
			*flagc = 0;
		}
	}
	if ((flags & stdflg) == 0)
		dolc--;
	dolv = tv + c - dolc;
	dolc--;
        /*dolv[0] = tv[0];
        dolv++;*/

	/*
	 * return here for shell file execution
	 * but not for parenthesis subshells
	 */
	setjmp(subshell);

	/*
	 * number of positional parameters
	 */
	replace(&cmdadr, dolv[0]);	/* cmdadr is $0 */

	/*
	 * set pidname '$$'
	 */
	assnum(&pidadr, getpid());

	/*
	 * set up temp file names
	 */
	settmp();

	/*
	 * default internal field separators - $IFS
	 */

	cfcat(sptbnl,_nl_space_alt);
	dfault(&ifsnod, sptbnl);

	dfault(&mchknod, MAILCHECK);
	mailchk = mstoi(mchknod.namval);
	if (mailchk < 0)  {
		assign(&mchknod, MAILCHECK);
		mailchk=stoi(MAILCHECK);
	}

	if ((beenhere++) == FALSE)	/* ? profile */
	{
		if (login_sh && !cheat)
		{			/* system profile */

#ifndef RES

			if ((input = pathopen(nullstr, sysprofile)) >= 0)
				exfile(rflag);		/* file exists */

#endif

	/*	
	 * fix bell bug now use $HOME/.profile as manual states.
	 */
			if(!suid_flag) {  /* if we're not running suid */
				if ((input = pathopen(to_tchar(getenv("HOME")), profile)) >= 0)
				{
					exfile(rflag);
					flags &= ~ttyflg;
				}
			}
		}
		if (rsflag == 0 || rflag == 0)
			flags |= rshflg;
		/*
		 * open input file if specified
		 */
		if (comdiv)
		{
			estabf(comdiv);
			input = -1;
		}
		else
		{
			input = ((flags & stdflg) ? 0 : chkopen(cmdadr));

#ifdef ACCT
			if (input != 0)
				preacct(cmdadr);
#endif
			comdiv--;
		}
	}
#if defined pdp11 || vax
	else
	{
		*execargs = (char *)dolv;	/* for `ps' cmd */
	}
#endif
		
	exfile(0);
	done();
}

static int
exfile(prof)
BOOL	prof;
{
	long	mailtime = 0;	/* Must not be a register variable */
	long 	curtime = 0;
	register int	userid;

	/*
	 * move input
	 */
	if (input > 0)
	{
		Ldup(input, INIO);
		input = INIO;
	}

	userid = geteuid();

	/*
	 * decide whether interactive
	 */
	if ((flags & intflg) ||
	    ((flags&oneflg) == 0 && isatty(input)) )
	    
	{
		dfault(&ps1nod, (userid ? stdprompt : supprompt));
		dfault(&ps2nod, readmsg);
		flags |= ttyflg | prompt;
		ignsig(SIGTERM);
		if (mailpnod.namflg != N_DEFAULT)
			setmail(mailpnod.namval);
		else
			setmail(mailnod.namval);
	}
	else
	{
		flags |= prof;
		flags &= ~prompt;
	}

	if (setjmp(errshell) && prof)
	{
		close(input);
		return;
	}
	/*
	 * error return here
	 */

	loopcnt = peekc = peekn = 0;
	fndef = 0;
	nohash = 0;
	iopend = 0;

	if (input >= 0)
		initf(input);
	/*
	 * command loop
	 */
	for (;;)
	{
		tdystak(0);
		stakchk();	/* may reduce sbrk */
		exitset();

		if ((flags & prompt) && standin->fstak == 0 && !eof)
		{

			if (mailp)
			{
				time(&curtime);

				if ((curtime - mailtime) >= mailchk)
				{
					chkmail();
				        mailtime = curtime;
				}
			}
#ifdef SIGWINCH
		{
		 struct winsize wsize;

		 if(ioctl(0, TIOCGWINSZ, &wsize) >= 0)
		     set_l_and_c(wsize.ws_row, wsize.ws_col);
		}
#endif /* SIGWINCH */
			if(flags & noprompt)
			    flags &= ~noprompt;
			else if(flags & sigwtrap)
			    flags &= ~sigwtrap;
			else
			    prst(ps1nod.namval);

#ifdef TIME_OUT
			alarm(TIMEOUT);
#endif

			flags |= waiting;
		}

		trapnote = 0;
		peekc = readc();
		if (eof) {
			return;
		}

#ifdef TIME_OUT
		alarm(0);
#endif

		flags &= ~waiting;

#ifdef TREEDEBUG
		{
		struct trenod *cmd_tree = cmd(NL, MTFLG);
		dbg_fd = open("/dev/tty",1);
		prs_buff("---BEGIN EXECUTION TREE---");
		ptree(cmd_tree,0);
		prc_buff(NL);
		prs_buff("----END EXECUTION TREE----\n");
		flushb();
		execute(cmd_tree, 0, eflag);
		}
#else
		execute(cmd(NL, MTFLG), 0, eflag);
#endif
		eof |= (flags & oneflg);
	}
}

chkpr()
{
	if ((flags & prompt) && standin->fstak == 0)
		prst(ps2nod.namval);
}

settmp()
{
	itos(getpid());
	serial = 0;
	tmpnam = movstr(to_char(numbuf), &tmpout[TMPNAM]);
}

Ldup(fa, fb)
register int	fa, fb;
{
#ifdef RES

	dup(fa | DUPFLG, fb);
	close(fa);
	ioctl(fb, FIOCLEX, 0);

#else

	if (fa >= 0)
	    {
	    if (fa != fb)
		{ close(fb);
		  fcntl(fa,0,fb);		/* normal dup */
		  close(fa);
		  fcntl(fb, 2, 1);		/* autoclose for fb */
		}
	    else	/* fa == fb */
		fcntl(fb, 2, 1);		/* autoclose for fb */
	    }

#endif
}


chkmail()
{
	register tchar 	*s = mailp;
	register tchar	*save;

	long	*ptr = mod_time;
	tchar	*start;
	BOOL	flg; 
	struct stat	statb;

	while (*s)
	{
		start = s;
		save = 0;
		flg = 0;

		while (*s)
		{
			if (*s != COLON)	
			{
				if (*s == '%' && save == 0)
					save = s;
			
				s++;
			}
			else
			{
				flg = 1;
				*s = 0;
			}
		}

		if (save)
			*save = 0;

		if (*start && stat(to_char(start), &statb) >= 0) 
		{
			if(statb.st_size && *ptr
				&& statb.st_mtime != *ptr)
			{
				if (save)
				{
					prst(save+1);
					newline();
				}
				else
					prs(nl_msg(602,mailmsg));
			}
			*ptr = statb.st_mtime;
		}
		else if (*ptr == 0)
			*ptr = 1;

		if (save)
			*save = '%';

		if (flg)
			*s++ = COLON;

		ptr++;
	}
}


setmail(mailpath)
	tchar *mailpath;
{
	register tchar	*s = mailpath;
	register int 	cnt = 1;

	long	*ptr;

	free(mod_time);
	if (mailp = mailpath)
	{
		while (*s)
		{
			if (*s == COLON)
				cnt += 1;

			s++;
		}

		ptr = mod_time = (long *)alloc(sizeof(long) * cnt);

		while (cnt)
		{
			*ptr = 0;
			ptr++;
			cnt--;
		}
	}
}
