static char *HPUX_ID = "@(#) $Revision: 70.3 $";
/*
** more.c - General purpose tty output filter and file perusal program
**
**	by Eric Shienbrood, UC Berkeley
**
**	modified by Geoff Peck, UCB to add underlining, single spacing
**	modified by John Foderaro, UCB to add -c and MORE environment variable
**      modified by Solomon, 3/28/82, to put terminal modes back the
**		way they were on exit.
**	modified by Joe Moran (mojo - HP-DSD)
**		- For use with AT&T tty drivers (-DUSG),
**		- Fixed pointer bugs in initterm(),
**		- Added code for new magic numbers (-Dhpux),
**		- Added code for old non-pdp11 magic numbers (-DNOTPDP)
**		  (This is for systems where HP-UX is not defined, and the
**		  the .o files follow the old format, but bytes not swapped)
**		- Added additional flag (-DSTACK_UP) which says that the
**		  stack grows up instead of down (needed by printf),
**		- Changed execute() so that it doesn't depend
**		  on the stack direction for calling exec().
**	modified by Alan Silverstein (FSD) for TWG-approved enhancements:
**		- Added support for standout (overstrikes) in addition to UL.
**		- -u option controls both UL and SO.
**		- Later, increase LINSIZ to handle nroff output better;
**		- Clear enhancements at end of prbuf (before end of line);
**		- Remove "externs" where they don't belong (for Indigo);
**		- Fix bug in tail() (bad boundary condition).
*/

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#ifdef SIGWINCH
#include <bsdtty.h>
#endif SIGWINCH
#include <stdlib.h>
#include <errno.h>
#include <setjmp.h>
#include <varargs.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef NLS
#include <limits.h>
#include <locale.h>
#include <nl_types.h>
#define NL_SETN 1	/* set number */
#else NLS
#define catgets(i, j, k, s) (s)
#endif NLS

#ifdef	USG	/* mojo */
#  include <termio.h>
#  define	MIN_CHAR         1	/* min # of chars for read request */
#  define	TIME	(char) 255	/* timeout value to satify read */
#else
#  include <sgtty.h>
#endif

#ifdef	hpux		/*	mojo	*/
#  include <magic.h>
#endif	hpux

#ifdef NLS16
#  include <nl_ctype.h>
#endif

/* Help file will eventually go in libpath(more.help) on all systems */

#ifdef USG	/*	mojo	*/
#  ifndef HELPFILE
#      define HELPFILE	"/usr/lib/more.help"
#      define NL_HELPFILE  nlspath("more.help")
#  endif
#  define VI		"/usr/bin/vi"
#else
#  ifndef INGRES
#    ifndef HELPFILE
#        define HELPFILE  libpath(more.help)
#        define NL_HELPFILE  nlspath("more.help")
#    endif
#    define VI		binpath(vi)
#  endif
#endif

#define Fopen(s,m)	(Currline = 0,file_pos=0,fopen(s,m))
#define Ftell(f)	file_pos
#define Fseek(f,off)	(file_pos=off,fseek(f,off,0))
#define Getc(f)		(++file_pos, getc(f))
#define Ungetc(c,f)	(--file_pos, ungetc(c,f))

#ifdef	USG	/*	mojo	*/
#  define stty(fd,argp)	ioctl(fd,TCSETAW,argp)
#  define gtty(fd,argp)	ioctl(fd,TCGETA,argp)
#  define	sg_erase	c_cc[VERASE]
#  define	sg_kill		c_cc[VKILL]
#  define	MBIT		1	/*	arbitary	*/
#  define	RAW		2	/*	arbitary	*/
#  define	CBREAK		MBIT
#else
#    define MBIT	CBREAK
#    define stty(fd,argp)	ioctl(fd,TIOCSETN,argp)
#endif

#define TBUFSIZ	1024
#define LINSIZ	1024
#define ctrl(letter)	('letter' & 077)
#define RUBOUT	'\177'
#define ESC	'\033'
#define QUIT	'\034'

#ifdef	USG	/*	mojo	*/
struct termio 	otty,origtty;
#else
struct sgttyb 	otty,origtty; /* solomon, 3/28/82 */
#endif

long		file_pos, file_size;
int		fnum, no_intty, no_tty, slow_tty;
int		dum_opt, dlines, onquit(), end_it(); 
#ifdef SIGWINCH
int		chgwinsz();
#endif SIGWINCH
#ifdef SIGTSTP
int		onsusp();
#endif SIGTSTP
int		nscroll = 11;	/* Number of lines scrolled by 'd' */
int		fold_opt = 1;	/* Fold long lines */
int		stop_opt = 1;	/* Stop after form feeds */
int		ssp_opt = 0;	/* Suppress white space */
int		ul_opt = 1;	/* Underline/Standout as best we can */
int		promptlen;
int		Currline;	/* Line we are currently at */
int		startup = 1;
int		firstf = 1;
int		notell = 1;
int		bad_so;	/* True if overwriting does not turn off standout */
int		inwait, Pause, errors;
int		within;	/* true if we are within a file,
			false if we are between files */
int		hard = 0;
int		dumb, noscroll, hardtabs, clreol;
int		catch_susp;	/* We should catch the SIGTSTP signal */
char		**fnames;	/* The list of file names */
int		nfiles;		/* Number of files left to process */
char		*shell;		/* The name of the shell to use */
int		shellp;		/* A previous shell command exists */
char		ch;
jmp_buf		restore;
char		obuf[BUFSIZ];	/* stdout buffer */
char		Line[LINSIZ];	/* Line buffer */
int		Lpp = 24;	/* lines per page */
char		*Clear;		/* clear screen */
char		*eraseln;	/* erase line */
char		*Senter, *Sexit;/* enter and exit standout mode */
char		*ULenter, *ULexit;	/* enter and exit underline mode */
char		*chUL;		/* underline character */
char		*chBS;		/* backspace character */
char		*Home;		/* go to home */
char		*cursorm;	/* cursor movement */
char		cursorhome[40];	/* contains cursor movement to home */
char		*EodClr;	/* clear rest of screen */
char		*tgetstr();
int		Mcol = 80;	/* number of columns */
int		Wrap = 1;	/* set if automargins */
char		*getenv();
char		*nlspath();
struct {
    long chrctr, line;
} context, screen_start;
char	PC;		/* pad character */
short	ospeed;

#ifdef NLS
nl_catd		catd;
#endif NLS


main(argc, argv)
int argc;
char *argv[];
{
    register FILE	*f;
    register char	*s;
    register char	*p;
    register int	ch;
    register int	left;
    int			prnames = 0; 
    int			initopt = 0;
    int			srchopt = 0;
    int			clearit = 0;
    int			err	= 0;
    int			initline;
    char		initbuf[80];
    FILE		*checkf();

    nfiles = argc;
    fnames = argv;

    		/* Open /dev/tty for command input. */
    if ((freopen("/dev/tty", "r+", stderr)) == NULL)
    {
	printf("Couldn't reopen stderr\n");
	exit(1);
    }

    initterm ();

#ifdef NLS
    if (!setlocale(LC_ALL,"")) {
	fputs(_errlocale("more"), stderr);
	putenv("LANG=");
	catd = (nl_catd)-1;
    } else
	catd = catopen("more", 0);
#endif NLS

    if(s = getenv("MORE")) argscan(s);

    /* dlines initialized here instead of in argscan () to avoid
     * reinitializing dlines value when scanning for args
     */
    dlines = 0;
	
    while (--nfiles > 0) {
	if ((ch = (*++fnames)[0]) == '-') {
	    argscan(*fnames+1);
	}
	else if (ch == '+') {
	    s = *fnames;
	    if (*++s == '/') {
		srchopt++;
		for (++s, p = initbuf; p < initbuf + 79 && *s != '\0';)
		    *p++ = *s++;
		*p = '\0';
	    }
	    else {
		initopt++;
		for (initline = 0; *s != '\0'; s++)
		    if (isdigit (*s))
			initline = initline*10 + *s -'0';
		--initline;
	    }
	}
	else break;
    }
    /* allow clreol only if Home and eraseln and EodClr strings are
     *  defined, and in that case, make sure we are in noscroll mode
     */
    if(clreol)
    {
	if ( ( Home == (char *)0    || *Home == '\0' )      ||
	     ( eraseln == (char *)0 || *eraseln == '\0' )   ||
	     ( EodClr == (char *)0  || *EodClr == '\0' ) )
	    clreol = 0;
	else noscroll = 1;
    }

    if (dlines == 0)
	dlines = Lpp - (noscroll ? 1 : 2);
    left = dlines;
    if (nfiles > 1)
	prnames++;
    if (!no_intty && nfiles == 0) {
	fputs(catgets(catd, NL_SETN, 1, "Usage: "),stderr);
	fputs(argv[0],stderr);
	fputs(catgets(catd, NL_SETN, 2, " [-dfln] [+linenum | +/pattern] name1 name2 ...\n"),stderr);
	exit(1);
    }
    else
	f = stdin;
    if (!no_tty) {
	signal(SIGQUIT, onquit);
	signal(SIGINT, end_it);
#ifdef SIGWINCH
	signal(SIGWINCH, chgwinsz);
#endif SIGWINCH

#ifdef	SIGTSTP
	if (signal (SIGTSTP, SIG_IGN) == SIG_DFL) {
	    signal(SIGTSTP, onsusp);
	    catch_susp++;
	}
#endif	SIGTSTP
	stty (2, &otty);
    }
    if (no_intty && nfiles==0) {
	if (no_tty && (nfiles == 0)) {
	    copy_file (stdin);
	    no_intty = 0;
	    prnames++;
	    firstf = 0;
	} else if (!no_tty) {
	    if ((ch = Getc (f)) == '\f')
		doclear();
	    else {
		Ungetc (ch, f);
		if (noscroll && (ch != EOF)) {
		    if (clreol)
			home ();
		    else
			doclear ();
		}
	    }
	    if (srchopt)
	    {
		search (initbuf, stdin, 1);
		if (noscroll)
		    left--;
	    }
	    else if (initopt)
		skiplns (initline, stdin);
	    screen (stdin, left);
	    no_intty = 0;
	    prnames++;
	    firstf = 0;
	}
    }

    while (fnum < nfiles) {
	if ((f = checkf (fnames[fnum], &clearit)) != NULL) {
	    context.line = context.chrctr = 0;
	    Currline = 0;
	    if (firstf) setjmp (restore);
	    if (firstf) {
		firstf = 0;
		if (srchopt)
		{
		    search (initbuf, f, 1);
		    if (noscroll)
			left--;
		}
		else if (initopt)
		    skiplns (initline, f);
	    }
	    else if (fnum < nfiles && !no_tty) {
		setjmp (restore);
		left = command (fnames[fnum], f);
	    }
	    if (left != 0) {
		if (clearit && (srchopt || initopt) && (Currline != 0))
		    clearit = 0;
		if ((noscroll || clearit) && (file_size != LONG_MAX))
		    if (clreol)
			home ();
		    else
			doclear ();
		if (prnames) {
		    if (bad_so)
			erase (0);
		    if (clreol)
			cleareol ();
		    pr("::::::::::::::");
		    if (promptlen > 14)
			erase (14);
		    printf ("\n");
		    if(clreol) cleareol();
		    printf("%s\n", fnames[fnum]);
		    if(clreol) cleareol();
		    printf("::::::::::::::\n", fnames[fnum]);
		    if (left > Lpp - 4)
			left = Lpp - 4;
		}
		if (no_tty)
		    copy_file (f);
		else {
		    within++;
		    screen(f, left);
		    within = 0;
		}
	    }
	    setjmp (restore);
	    fflush(stdout);
	    fclose(f);
	    screen_start.line = screen_start.chrctr = 0L;
	    context.line = context.chrctr = 0L;
	}
	else {
	   err = 1;
    }
	fnum++;
	firstf = 0;
    }
    reset_tty ();
	if (err)
	   exit(1);
    else
       exit(0);
}

argscan(s)
char *s;
{

	for (; *s != '\0'; s++)
		if (isdigit(*s))
		    dlines = dlines*10 + *s - '0';
		else if (*s == 'd')
		    dum_opt = 1;
		else if (*s == 'l')
		    stop_opt = 0;
		else if (*s == 'f')
		    fold_opt = 0;
		else if (*s == 'p')
		    noscroll++;
		else if (*s == 'c')
		    clreol++;
		else if (*s == 's')
		    ssp_opt = 1;
		else if (*s == 'u')		/* controls UL and SO */
		    ul_opt = 0;
}


/*
** Check whether the file named by fs is an ASCII file which the user may
** access.  If it is, return the opened file. Otherwise return NULL.
*/

FILE *
checkf (fs, clearfirst)
register char *fs;
int *clearfirst;
{
    struct stat stbuf;
    register FILE *f;
    int c;

    if (*fs=='-' && *(fs+1)=='\0') {
	no_intty=1;
	return(stdin);
    } else
	no_intty=0;
    if (stat (fs, &stbuf) == -1) {
	fflush(stdout);
	if (clreol)
	    cleareol ();
	perror(fs);
	return (NULL);
    }
    if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
	printf(catgets(catd, NL_SETN, 3, "\n*** %s: directory ***\n\n"), fs);
	return (NULL);
    }
    if ((stbuf.st_mode & S_IFMT) == S_IFIFO) {
	printf(catgets(catd, NL_SETN, 99, "\n*** %s: fifo file ***\n\n"), fs);
	return (NULL);
    }
    if ((f=Fopen(fs, "r")) == NULL) {
	fflush(stdout);
	perror(fs);
	return (NULL);
    }

    /* Try to see whether it is an ASCII file */

#ifdef	hpux	/* mojo */
    {
	MAGIC	magic;

	(void) fseek(f, MAGIC_OFFSET, 0);
	if(fread((char *) &magic, sizeof(magic), 1, f) == 1) {
	    if ((magic.system_id == HP9000_ID ||
		 magic.system_id == HP98x6_ID ||
		 magic.system_id == HP9000S200_ID ||
		 _PA_RISC_ID(magic.system_id)) &&
		(magic.file_type == EXEC_MAGIC ||
		 magic.file_type == RELOC_MAGIC ||
		 magic.file_type == SHARE_MAGIC ||
#ifdef DEMAND_MAGIC
		 magic.file_type == DEMAND_MAGIC ||
#endif
		 magic.file_type == AR_MAGIC))
	    {
		    printf(catgets(catd, NL_SETN, 4, "\n******** %s: Not a text file ********\n\n"), fs);
		    fclose (f);
		    return (NULL);
	    }
        }
	(void) rewind(f);
    }
    c = Getc(f);

#else /* not hpux */
    c = Getc(f);

#  ifdef NOTPDP
    switch ((c << 8 | *f->_ptr) & 0177777) {
#  endif NOTPDP
#  ifndef NOTPDP
    switch ((c | *f->_ptr << 8) & 0177777) {
#  endif
    case 0405:
    case 0407:
    case 0410:
    case 0411:
    case 0413:
    case 0177545:
	printf(catgets(catd, NL_SETN, 4, "\n******** %s: Not a text file ********\n\n"), fs);
	fclose (f);
	return (NULL);
    default:
	break;
    }
#endif
    if (c == '\f')
	*clearfirst = 1;
    else {
	*clearfirst = 0;
	Ungetc (c, f);
    }
    if ((file_size = stbuf.st_size) == 0)
	file_size = LONG_MAX;
    return (f);
}

/*
** A real function, for the tputs routine in termlib
*/

putch (ch)
char ch;
{
    putchar (ch);
}

/*
** Print out the contents of the file f, one screenful at a time.
*/

#define STOP -10

screen (f, num_lines)
register FILE *f;
register int num_lines;
{
    register int c;
    register int nchars;
    int length;			/* length of current line */
    static int prev_len = 1;	/* length of previous line */

    for (;;) {
	while (num_lines > 0 && !Pause) {
	    if ((nchars = getline (f, &length)) == EOF)
	    {
	    	if (bad_so || (Senter && *Senter == ' ') && promptlen > 0)
		    erase (0);
		if (clreol)
		    clreos();
		return;
	    }
	    if (ssp_opt && length == 0 && prev_len == 0)
		continue;
	    prev_len = length;
	    if (bad_so || (Senter && *Senter == ' ') && promptlen > 0)
		erase (0);
	    /* must clear before drawing line since tabs on some terminals
	     * do not erase what they tab over.
	     */
	    if (clreol)
		cleareol ();
	    prbuf (Line, length);
	    if (nchars < promptlen)
		erase (nchars);	/* erase () sets promptlen to 0 */
	    else promptlen = 0;
	    /* is this needed?
	     * if (clreol)
	     *	cleareol();	/* must clear again in case we wrapped *
	     */
	    if (nchars < Mcol || !fold_opt)
		putchar('\n');
	    if (nchars == STOP)
		break;
	    num_lines--;
	}
	fflush(stdout);
	if ((c = Getc(f)) == EOF)
	{
	    if (clreol)
		clreos ();
	    return;
	}

	if (Pause && clreol)
	    clreos ();
	Ungetc (c, f);
	setjmp (restore);
	Pause = 0; startup = 0;
	if ((num_lines = command (NULL, f)) == 0)
	    return;
	if (hard && promptlen > 0)
		erase (0);
	if (noscroll && num_lines == dlines)
	{ 
	    if (clreol)
		home();
	    else
		doclear ();
	}
	screen_start.line = Currline;
	screen_start.chrctr = Ftell (f);
    }
}

/*
** Come here if a quit signal is received
*/

onquit()
{
    signal(SIGQUIT, SIG_IGN);
    if (!inwait) {
	putchar ('\n');
	if (!startup) {
	    signal(SIGQUIT, onquit);
	    longjmp (restore, 1);
	}
	else
	    Pause++;
    }
    else if (!dum_opt && notell) {
#ifndef	NLS
	write (2, "[Use q or Q to quit]", 20);
	promptlen += 20;
#else
	{
	    char *s = catgets(catd, NL_SETN, 6, "[Use q or Q to quit]");
	    write (2, s, strlen(s));
	    promptlen += strlen(s);
	}
#endif
	notell = 0;
    }
    signal(SIGQUIT, onquit);
}

#ifdef SIGWINCH
/*
 * Handle a change in the window size
 */

chgwinsz()
{
    struct winsize win;
    char   env_lines[15], env_columns[15]; 

    (void) signal(SIGWINCH, SIG_IGN);
    if (ioctl(fileno(stdout), TIOCGWINSZ, &win) != -1) 
    {
        if ((win.ws_row != 0) && (win.ws_row != Lpp)) 
	{
            Lpp = win.ws_row;
            nscroll = Lpp/2 - 1;
            if (nscroll <= 0)
                nscroll = 1;
            dlines = Lpp - (noscroll ? 1 : 2);
	    sprintf(env_lines, "LINES=%d", Lpp);
	    putenv(env_lines);
        }

        if ((win.ws_col != 0) && (win.ws_col != Mcol))
	{
            Mcol = win.ws_col;
	    sprintf(env_columns, "COLUMNS=%d", Mcol);
	    putenv(env_columns);

	}

    }

    (void) signal(SIGWINCH, chgwinsz);
}
#endif SIGWINCH



/*
** Clean up terminal state and exit. Also come here if interrupt signal received
*/

end_it ()
{

    reset_tty ();
    if (clreol) {
	putchar ('\r');
	clreos ();
	fflush (stdout);
    }
    else if (!clreol && (promptlen > 0)) {
	kill_line ();
	fflush (stdout);
    }
    else
	write (2, "\n", 1);
    _exit(0);
}

copy_file(f)
register FILE *f;
{
    register int c;

    while ((c = getc(f)) != EOF)
	putchar(c);
}

/* Simplified printf function */

printf (va_alist)
va_dcl
{
	register char *fmt;
	register char ch;
	register int ccount;
	va_list ap;

	va_start(ap);
	ccount = 0;
	fmt = va_arg(ap, char *);
	while (*fmt) {
		while ((ch = *fmt++) != '%') {
			if (ch == '\0')
				return (ccount);
			ccount++;
			putchar (ch);
		}
		switch (*fmt++) {
		case 'd':
			ccount += printd (va_arg(ap, int));
			break;
		case 's':
			ccount += pr ((char *)va_arg(ap, char *));
			break;
		case '%':
			ccount++;
			putchar ('%');
			break;
		case '0':
			return (ccount);
		default:
			break;
		}
	}
	return (ccount);

}

/*
** Print an integer as a string of decimal digits,
** returning the length of the print representation.
*/

printd (n)
int n;
{
    int a, nchars;

    if (a = n/10)
	nchars = 1 + printd(a);
    else
	nchars = 1;
    putchar (n % 10 + '0');
    return (nchars);
}

/* Put the print representation of an integer into a string */
static char *sptr;

scanstr (n, str)
int n;
char *str;
{
    sptr = str;
    sprintd (n);
    *sptr = '\0';
}

sprintd (n)
{
    int a;

    if (a = n/10)
	sprintd (a);
    *sptr++ = n % 10 + '0';
}

static char bell = ctrl(G);

strlen (s)
char *s;
{
    register char *p;

    p = s;
    while (*p++)
	;
    return (p - s - 1);
}

/* See whether the last component of the path name "path" is equal to the
** string "string"
*/

tailequ (path, string)
char *path;
register char *string;
{
	register char *tail;

	tail = path + strlen(path);
	while (tail > path)
		if (*(--tail) == '/') {
			++tail;
			break;
		}
	while (*tail++ == *string++)
		if (*tail == '\0')
			return(1);
	return(0);
}

prompt (filename)
char *filename;
{
    if (clreol)
	cleareol ();
    else if (promptlen > 0)
	kill_line ();
    if (!hard) {
	promptlen = 8;
	if (clreol)
	    cleareol ();
	if (Senter && Sexit)
	    tputs (Senter, 1, putch);
	pr(catgets(catd, NL_SETN, 7, "--More--"));
	if (filename != NULL) {
	    promptlen += printf (catgets(catd, NL_SETN, 8, "(Next file: %s)"), filename);
	}
	else if (!no_intty) {
	    promptlen += printf (catgets(catd, NL_SETN, 9, "(%d%%)"), (int)((file_pos * 100) / file_size));
	}
	if (dum_opt) {
	    promptlen += pr(catgets(catd, NL_SETN, 10, "[Press space to continue, q to quit]"));
	}
	if (Senter && Sexit)
	    tputs (Sexit, 1, putch);
	if (clreol)
	    clreos ();
	fflush(stdout);
    }
    else
	write (2, &bell, 1);
    inwait++;
}

/*
** Get a logical line
*/

getline(f, length)
register FILE *f;
int *length;
{
    register int	c;
    register char	*p;
    register int	column;
    int			i;
    static int		colflg;
#ifdef NLS16
#define		BSTATUS b_status = BYTE_STATUS((unsigned char)c, b_status)
    int 		b_status = ONEBYTE;	/* "last was '\n'" */
#else
# define	BSTATUS
#endif

    p = Line;
    column = 0;
    c = Getc (f); BSTATUS;
    if (colflg && c == '\n') {
	Currline++;
        c = Getc (f); BSTATUS;
    }
    while (p < &Line[LINSIZ - 1]) {
	if (c == EOF) {
	    if (p > Line) {
		*p = '\0';
		*length = p - Line;
		return (column);
	    }
	    *length = p - Line;
	    return (EOF);
	}
	c &= 0377;	/* avoid sign extension on 8-bit char */
			/* had to wait until after test for EOF */
	if (c == '\n') {
	    Currline++;
	    break;
	}
	*p++ = c;
	if (c == '\t')
	    if (hardtabs && column < promptlen && !hard) {
		if (eraseln && !dumb) {
		    column = 1 + (column | 7);
		    tputs (eraseln, 1, putch);
		    promptlen = 0;
		}
		else {
		    for (--p; column & 7 && p < &Line[LINSIZ - 1]; column++) {
			*p++ = ' ';
		    }
		    if (column >= promptlen) promptlen = 0;
		}
	    }
	    else {
		if (!hardtabs) {
		    for (--p, i = column; i < 1 + (column | 7); i++)
			*p++ = ' ';
		    column = i;
		}
		else
		    column = 1 + (column | 7);
	    }
	else if (c == '\b')
	    column--;
	else if (c == '\r')
	    column = 0;
	else if (c == '\f' && stop_opt) {
		p[-1] = '^';
		*p++ = 'L';
		column += 2;
		Pause++;
	}
	else if (c == EOF) {
	    *length = p - Line;
	    return (column);
	}
	else if ((unsigned char)c >= ' ' && c != RUBOUT) {
	    column++;
#if defined NLS16 && defined EUC
	    if (b_status == FIRSTOF2 && C_COLWIDTH(c) == 1)
		column--;
#endif
	}
	if (column >= Mcol && fold_opt) {
#ifdef NLS16
		/* avoid putting up only half a kanji */
		/* replace by space and put back first byte */
#ifdef EUC
		if (b_status == FIRSTOF2 && ( C_COLWIDTH(c) - 1 )) {
#else
		if (b_status == FIRSTOF2) {
#endif
			*(p-1) = ' ';
			Ungetc(c, f);
		}
#endif
		break;
	}
	c = Getc (f); BSTATUS;
    }
    if (column >= Mcol && Mcol > 0) {
	if (!Wrap) {
	    *p++ = '\n';
	}
    }
    colflg = column == Mcol && fold_opt;
    *length = p - Line;
    *p = 0;
    return (column);
}

/*
** Erase the rest of the prompt, assuming we are starting at column col.
*/

erase (col)
register int col;
{

    if (promptlen == 0)
	return;
    if (hard) {
	putchar ('\n');
    }
    else {
	if (col == 0)
	    putchar ('\r');
	if (!dumb && eraseln)
	    tputs (eraseln, 1, putch);
	else
	    for (col = promptlen - col; col > 0; col--)
		putchar (' ');
    }
    promptlen = 0;
}

/*
** Erase the current line entirely
*/

kill_line ()
{
    erase (0);
    if (!eraseln || dumb) putchar ('\r');
}

/*
 * force clear to end of line
 */
cleareol()
{
    tputs(eraseln, 1, putch);
}

clreos()
{
    tputs(EodClr, 1, putch);
}

/*
**  Print string and return number of characters
*/

pr(s1)
char	*s1;
{
    register char	*s;
    register char	c;

    for (s = s1; c = *s++; )
	putchar(c);
    return (s - s1 - 1);
}


/* Print a buffer of n characters */

prbuf (s, n)
register char *s;
register int n;
{

#define stateNIL 0			/* no enhancements */
#define stateUL  1
#define stateSO  2

    char c;				/* next ouput character */
#ifdef NLS16
    char c2='\0';			/* 2nd byte of a 2-byte output char. */	
#endif
    register int state;			/* next output char's UL state */
    static   int pstate = stateNIL;	/* current terminal UL state (off) */

    while (--n >= 0)
	if (! ul_opt)			/* controls UL and SO */
	    putchar (*s++);
	else {
	    if (n >= 2 && s[0] == '_' && s[1] == '\b') {
		n -= 2;
	        s += 2;
		c = *s++;
		state = 1;
	    } else if (n >= 2 && s[1] == '\b' && s[2] == '_') {
		n -= 2;
		c = *s++;
		s += 2;
		state = stateUL;
	    } else if ((n >= 3) && (s[1] == '\b') && (s[0] == s[2])) { /* ajs */
		c = *s;			/* save overstricken char */

		do {
		    n -= 2;
		    s += 2;
		} while ((n >= 2) && (s[1] == '\b'));	/* eat overstrikes */

		if (n > 0)		/* eat last overstrike char */
		    s++;
		state = stateSO;

#ifdef NLS16
	/* 
	** To recognize the 2-byte character underline and overstrike sequences 
	*/
	    } else if (n >= 5 && s[0] == s[1] && s[0] == '_' 	/* 2-byte UL */
		       && s[2] == s[3] && s[2] == '\b') {
		n -= 5;
		s += 4;
		c = *s++;
		c2 = *s++;
		state = stateUL;
	    } else if (n >= 5 && s[2] == s[3] && s[2] == '\b' 	/* 2-byte UL */
			      && s[4] == s[5] && s[4] == '_') {
		n -= 5;
		c = *s++;
		c2 = *s++;
		s += 4;
		state = stateUL;
	    } else if (n >= 5 && s[2] == s[3] && s[2] == '\b' 	/* 2-byte OS */
			      && s[0] == s[4] && s[1] == s[5]) {
		c = *s;			/* save overstrike 2-byte character */
		c2 = *(s + 1);		/* ( c and c2 ) */

		do {			/* advance the overstrike seqence */
		    n -= 4;
		    s += 4;
		} while (n >= 3 && s[2] == s[3] && s[2] == '\b');

		if (n > 0) {		/* advance last overstrike character */
		    s += 2;
		    n--;
		}
		state = stateSO;
#endif

	    } else {
		c = *s++;
		state = stateNIL;
	    }
	    if (state != pstate)
	    {
		if (pstate == stateUL)		/* turn off old mode */
		    tputs (ULexit,  1, putch);
		if (pstate == stateSO)
		    tputs (Sexit,   1, putch);

		if (state  == stateUL)		/* turn on new mode */
		    tputs (ULenter, 1, putch);
		if (state  == stateSO)
		    tputs (Senter,  1, putch);
	    }
	    pstate = state;
	    putchar(c);
#ifdef NLS16
	    if ( c2 )			/* output 2nd byte if any */
		putchar(c2);
#endif
	    if ((state == stateUL) && *chUL) {
		pr(chBS);
#ifdef NLS16
	        if ( c2 )		/* if 2-byte character, one more chBS */
		    pr(chBS);
#endif
		tputs(chUL, 1, putch);
	    }
#ifdef NLS16
	c2 = '\0';			/* reset c2 */
#endif
	}

	/* end of line */
	if (pstate == stateUL)		/* turn off old mode */
	    tputs (ULexit,  1, putch);
	if (pstate == stateSO)
	    tputs (Sexit,   1, putch);
	pstate = stateNIL;
}

/*
**  Clear the screen
*/

doclear()
{
    if (Clear && !hard) {
	tputs(Clear, 1, putch);

	/* Put out carriage return so that system doesn't
	** get confused by escape sequences when expanding tabs
	*/
	putchar ('\r');
	promptlen = 0;
    }
}

/*
 * Go to home position
 */
home()
{
    tputs(Home,1,putch);
}

static int lastcmd, lastarg, lastp;
static int lastcolon;
char shell_line[132];

/*
** Read a command and do it. A command consists of an optional integer
** argument followed by the command character.  Return the number of lines
** to display in the next screenful.  If there is nothing more to display
** in the current file, zero is returned.
*/

command (filename, f)
char *filename;
register FILE *f;
{
    register int nlines;
    register int retval;
    register int c;
    char colonch;
    FILE *helpf;
    int done;
    char comchar, cmdbuf[80], *p;

#define ret(val) retval=val;done++;break

    done = 0;
    if (!errors)
	prompt (filename);
    else
	errors = 0;
    if (MBIT == RAW && slow_tty) {
#ifdef	USG	/*	mojo	*/
	otty.c_lflag &= ~ICANON;
	otty.c_cc[VMIN] = MIN_CHAR;
	otty.c_cc[VTIME] = TIME;
#else
	otty.sg_flags |= MBIT;
#endif
	stty(2, &otty);
    }
    for (;;) {
	nlines = number (&comchar);
	lastp = colonch = 0;
	if (comchar == '.') {	/* Repeat last command */
		lastp++;
		comchar = lastcmd;
		nlines = lastarg;
		if (lastcmd == ':')
			colonch = lastcolon;
	}
	lastcmd = comchar;
	lastarg = nlines;
	if (comchar == otty.sg_erase) {
	    kill_line ();
	    prompt (filename);
	    continue;
	}
	switch (comchar) {
	case ':':
	    retval = colon (filename, colonch, nlines);
	    if (retval >= 0)
		done++;
	    break;
	case ' ':
	case 'z':
	    if (nlines == 0) nlines = dlines;
	    else if (comchar == 'z') dlines = nlines;
	    ret (nlines);
	case 'd':
	case ctrl(D):
	    if (nlines != 0) nscroll = nlines;
	    ret (nscroll);
	case RUBOUT:
	case 'q':
	case 'Q':
	    end_it ();
	case 's':
	case 'f':
	    if (nlines == 0) nlines++;
	    if (comchar == 'f')
		nlines *= dlines;
	    putchar ('\r');
	    erase (0);
	    printf ("\n");
	    if (clreol)
		cleareol ();
	    if (nlines > 1)
	        printf (catgets(catd, NL_SETN, 11, "...skipping %d lines\n"), nlines);
	    else
	        printf (catgets(catd, NL_SETN, 12, "...skipping %d line\n"), nlines);

	    if (clreol)
		cleareol ();
	    pr ("\n");

	    while (nlines > 0) {
		while ((c = Getc (f)) != '\n')
		    if (c == EOF) {
			retval = 0;
			done++;
			goto endsw;
		    }
		    Currline++;
		    nlines--;
	    }
	    ret (dlines);
	case '\n':
	    if (nlines != 0)
		dlines = nlines;
	    else
		nlines = 1;
	    ret (nlines);
	case '\f':
	    if (!no_intty) {
		doclear ();
		Fseek (f, screen_start.chrctr);
		Currline = screen_start.line;
		ret (dlines);
	    }
	    else {
		write (2, &bell, 1);
		break;
	    }
	case '\'':
	    if (!no_intty) {
		kill_line ();
		pr (catgets(catd, NL_SETN, 13, "\n***Back***\n\n"));
		Fseek (f, context.chrctr);
		Currline = context.line;
		ret (dlines);
	    }
	    else {
		write (2, &bell, 1);
		break;
	    }
	case '=':
	    kill_line ();
	    promptlen = printd (Currline);
	    fflush (stdout);
	    break;
	case 'n':
	    lastp++;
	case '/':
	    if (nlines == 0) nlines++;
	    kill_line ();
	    pr ("/");
	    promptlen = 1;
	    fflush (stdout);
	    if (lastp) {
		write (2,"\r", 1);
		search ("", f, nlines);	/* Use previous r.e. */
	    }
	    else {
		ttyin (cmdbuf, 78, '/');
		write (2, "\r", 1);
		search (cmdbuf, f, nlines);
	    }
	    ret (dlines-1);
	case '!':
	    do_shell (filename);
	    break;
	case 'h':
#ifdef	NLS
	    if ((helpf = fopen (NL_HELPFILE, "r")) == NULL &&
		(helpf = fopen (HELPFILE, "r")) == NULL)
		error (catgets(catd, NL_SETN, 14, "Cannot open help file"));
#else
	    if ((helpf = fopen (HELPFILE, "r")) == NULL)
		error (catgets(catd, NL_SETN, 14, "Cannot open help file"));
#endif
	    if (noscroll) doclear ();
	    copy_file (helpf);
	    close (helpf);
	    prompt (filename);
	    break;
	case 'v':	/* This case should go right before default */
	    if (!no_intty) {
		kill_line ();
		cmdbuf[0] = '+';
		scanstr (Currline, &cmdbuf[1]);
		pr ("vi "); pr (cmdbuf); putchar (' '); pr (fnames[fnum]);
		execute (filename, VI, "vi", cmdbuf, fnames[fnum], 0);
		break;
	    }
	default:
	    write (2, &bell, 1);
	    break;
	}
	if (done) break;
    }
    putchar ('\r');
endsw:
    inwait = 0;
    notell++;
    if (MBIT == RAW && slow_tty) {
#ifdef	USG	/*	mojo	*/
	otty.c_lflag |= ICANON;
	otty.c_cc[VEOF] = origtty.c_cc[VEOF];
	otty.c_cc[VEOL] = origtty.c_cc[VEOL];
#else
	otty.sg_flags &= ~MBIT;
#endif
	stty(2, &otty);
    }
    return (retval);
}


/*
 * Execute a colon-prefixed command.
 * Returns <0 if not a command that should cause
 * more of the file to be printed.
 */

colon (filename, cmd, nlines)
char *filename;
int cmd;
int nlines;
{
	if (cmd == 0)
		ch = readch ();
	else
		ch = cmd;
	lastcolon = ch;
	switch (ch) {
	case 'f':
		kill_line ();
		if (!no_intty)
			promptlen = printf (catgets(catd, NL_SETN, 15, "\"%s\" line %d"), fnames[fnum], Currline);
		else
			promptlen = printf (catgets(catd, NL_SETN, 16, "[Not a file] line %d"), Currline);
		fflush (stdout);
		return (-1);
	case 'n':
		if ( nfiles <= 0 ) {
		    write (2, &bell, 1);
		    return -1;  /* No files on the command line. */
		}
		if (nlines == 0) {
			if (fnum >= nfiles - 1)
				end_it ();
			nlines++;
		}
		putchar ('\r');
		erase (0);
		skipf (nlines);
		return (0);
	case 'p':
		if (no_intty) {
			write (2, &bell, 1);
			return (-1);
		}
		putchar ('\r');
		erase (0);
		if (nlines == 0)
			nlines++;
		skipf (-nlines);
		return (0);
	case '!':
		do_shell (filename);
		return (-1);
	case 'q':
	case 'Q':
		end_it ();
	default:
		write (2, &bell, 1);
		return (-1);
	}
}

/*
** Read a decimal number from the terminal. Set cmd to the non-digit which
** terminates the number.
*/

number(cmd)
char *cmd;
{
	register int i;

	i = 0; ch = otty.sg_kill;
	for (;;) {
		ch = readch ();
		if (ch >= '0' && ch <= '9')
			i = i*10 + ch - '0';
		else if (ch == otty.sg_kill)
			i = 0;
		else {
			*cmd = ch;
			break;
		}
	}
	return (i);
}

do_shell (filename)
char *filename;
{
	char cmdbuf[80];

	kill_line ();
	pr ("!");
	fflush (stdout);
	promptlen = 1;
	if (lastp)
		pr (shell_line);
	else {
		ttyin (cmdbuf, 78, '!');
		if (expand (shell_line, cmdbuf)) {
			kill_line ();
			promptlen = printf ("!%s", shell_line);
		}
	}
	fflush (stdout);
	write (2, "\n", 1);
	promptlen = 0;
	shellp = 1;
	execute (filename, shell, shell, "-c", shell_line, 0);
}

/*
** Search for nth ocurrence of regular expression contained in buf in the file
*/

#define	INIT	register char *sp = instring;  
#define GETC()	(*sp++)
#define PEEKC()	(*sp)
#define UNGETC(c)	(--sp)
#define RETURN(c)	return
#define ERROR(c)	{expbuf[0] = (char) 0; \
			error (catgets(catd, NL_SETN, 17, "Regular expression botch"));}


#define ESIZE 512
static char expbuf[ESIZE] = "\0";

#include <regexp.h>

search (buf, file, n)
char buf[];
FILE *file;
register int n;
{
    long startline = Ftell (file);
    register long line1 = startline;
    register long line2 = startline;
    register long line3 = startline;
    register int lncount;
    int saveln, rv, re_exec();
    char *s, *re_comp();

    context.line = saveln = Currline;
    context.chrctr = startline;
    lncount = 0;
    /*
    if ((s = re_comp (buf)) != 0)
	error (s);
    */
    compile(buf, expbuf, &expbuf[ESIZE], 0); /* see ERROR() & man page */
    while (!feof (file)) {
	line3 = line2;
	line2 = line1;
	line1 = Ftell (file);
	rdline (file);
	lncount++;
	/*
	if ((rv = re_exec (Line)) == 1)
	*/
	if (step(Line, expbuf))
		if (--n == 0) {
		    if (lncount > 3 || (lncount > 1 && no_intty))
		    {
			pr ("\n");
			if (clreol)
			    cleareol ();
			pr(catgets(catd, NL_SETN, 18, "...skipping\n"));
		    }
		    if (!no_intty) {
			Currline -= (lncount >= 3 ? 3 : lncount);
			Fseek (file, line3);
			if (noscroll)
			    if (clreol) {
				home ();
				cleareol ();
			    } 
			    else
				doclear ();
		    }
		    else {
			kill_line ();
			if (noscroll)
			    if (clreol) {
			        home (); 
			        cleareol ();
			    } 
			    else
				doclear ();
			pr (Line);
			putchar ('\n');
		    }
		    break;
		}
	/*
	else if (rv == -1)
	    error ("Regular expression botch");
	*/
    }
    if (feof (file)) {
	if (!no_intty) {
	    Currline = saveln;
	    Fseek (file, startline);
	}
	else {
	    pr (catgets(catd, NL_SETN, 19, "\nPattern not found\n"));
	    end_it ();
	}
	error (catgets(catd, NL_SETN, 20, "Pattern not found"));
    }
}

execute (filename, cmd, a1, a2, a3, a4, a5, a6, a7, a8, a9)
char *filename;
char *cmd, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	int id;
	char *s;

	fflush (stdout);
	reset_tty ();
	while ((id = fork ()) < 0)
	    sleep (5);
	if (id == 0) {
	    /*
	    mojo - changed from the following line because
	    it won't work on a machine where the stack grows up,
            such as the HP 9000!

	    execv (cmd, &args);
	    */
	    close(0);
	    close(1);
	    dup(2);
	    dup(2);
            execl (cmd, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#ifndef	NLS
	    write (2,  "exec failed\n", 12);
#else
	    s = catgets(catd, NL_SETN, 21, "exec failed\n");
	    write (2, s, strlen(s));
#endif
	    exit (1);
	}
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);

#ifdef SIGWINCH
	signal (SIGWINCH, SIG_IGN);
#endif SIGWINCH

#ifdef SIGTSTP
	if (catch_susp)
	    signal(SIGTSTP, SIG_DFL);
#endif SIGTSTP
	while(wait(0) != id);
	signal (SIGINT, end_it);
	signal (SIGQUIT, onquit);

#ifdef SIGWINCH
	signal (SIGWINCH, chgwinsz);
        /* If window size has changed take the appropriate action */
	chgwinsz(); 	
#endif SIGWINCH

#ifdef SIGTSTP
	if (catch_susp)
	    signal(SIGTSTP, onsusp);
#endif SIGTSTP
	set_tty ();
	pr ("------------------------\n");
	prompt (filename);
}
/*
** Skip n lines in the file f
*/

skiplns (n, f)
register int n;
register FILE *f;
{
    register int c;

    while (n > 0) {
	while ((c = Getc (f)) != '\n')
	    if (c == EOF)
		return;
	n--;
	Currline++;
    }
}

/*
** Skip nskip files in the file list (from the command line). Nskip may be
** negative.
*/

skipf (nskip)
register int nskip;
{
    if (nskip == 0) return;
    if (nskip > 0) {
	if (fnum + nskip > nfiles - 1)
	    nskip = nfiles - fnum - 1;
    }
    else if (within)
	++fnum;
    fnum += nskip;
    if (fnum < 0)
	fnum = 0;
    pr (catgets(catd, NL_SETN, 22, "\n...Skipping "));
    pr ("\n");
    if (clreol)
	cleareol ();
    pr (catgets(catd, NL_SETN, 23, "...Skipping "));
    pr (nskip > 0 ? catgets(catd, NL_SETN, 24, "to file ") :
		    catgets(catd, NL_SETN, 25, "back to file "));
    pr (fnames[fnum]);
    pr ("\n");
    if (clreol)
	cleareol ();
    pr ("\n");
    --fnum;
}

/*----------------------------- Terminal I/O -------------------------------*/

initterm ()
{
    char	buf[TBUFSIZ];
    static	char	clearbuf[200];	/*	mojo - made static	*/
					/*	to avoid dangling ptrs	*/
    char        *termname;
    char	*clearptr, *padstr;
    int		ldisc;

    setbuf(stdout, obuf);
    if (!(no_tty = gtty(1, &otty))) {
        termname = getenv("TERM");
	if (termname == (char *) NULL || tgetent(buf, termname) <= 0) {
	    dumb++; ul_opt = 0;
	}
	else {
	    if (((Lpp = tgetnum("li")) < 0) || tgetflag("hc")) {
		hard++;	/* Hard copy terminal */
		Lpp = 24;
	    }
	    if (tailequ (fnames[0], "page") || !hard && tgetflag("ns"))
		noscroll++;
	    if ((Mcol = tgetnum("co")) < 0)
		Mcol = 80;
	    Wrap = tgetflag("am");
	    bad_so = tgetflag ("xs");
	    clearptr = clearbuf;
	    eraseln = tgetstr("ce",&clearptr);
	    Clear = tgetstr("cl", &clearptr);

	    /*
	     * Set up for standout if terminal supports it:
	     */

	    if ((Senter = tgetstr ("so", &clearptr)) == NULL)
		 Senter = "";
	    if ((Sexit  = tgetstr ("se", &clearptr)) == NULL)
		 Sexit  = "";

	    /*
	     *  Set up for underlining:  some terminals don't need it;
	     *  others have start/stop sequences, still others have an
	     *  underline char sequence which is assumed to move the
	     *  cursor forward one character.  If underline sequence
	     *  isn't available, settle for standout sequence.
	     */

	    if (tgetflag("ul") || tgetflag("os")) {
		ul_opt = 0;			/* controls both UL and SO */
	    }
	    if ((chUL = tgetstr("uc", &clearptr)) == NULL ) {
		chUL = "";
	    }
	    if ((ULenter = tgetstr("us", &clearptr)) == NULL &&
		(!*chUL) && (ULenter = tgetstr("so", &clearptr)) == NULL) {
		ULenter = "";
	    }
	    if ((ULexit = tgetstr("ue", &clearptr)) == NULL &&
		(!*chUL) && (ULexit = tgetstr("se", &clearptr)) == NULL) {
		ULexit = "";
	    }
	    
	    if (padstr = tgetstr("pc", &clearptr))
		PC = *padstr;
	    Home = tgetstr("ho",&clearptr);
	    if ((Home == 0) || *Home == '\0')	/* mojo - avoid ptr bug */
	    {
		if ((cursorm = tgetstr("cm", &clearptr)) != NULL) {
		    strcpy(cursorhome, tgoto(cursorm, 0, 0));
		    Home = cursorhome;
	       }
	    }
	    EodClr = tgetstr("cd", &clearptr);
	}
	if ((shell = getenv("SHELL")) == NULL)
	    shell = "/bin/sh";
    }
    no_intty = gtty(0, &otty);
    gtty(2, &otty);
    gtty(2, &origtty);	/* solomon, 3/28/82 */
#ifdef	USG	/*	mojo	*/
    ospeed = otty.c_cflag & CBAUD;
    slow_tty = ospeed < B1200;
    hardtabs =  !(otty.c_oflag & TAB3);
    if (!no_tty) {
	otty.c_lflag &= ~ECHO;
	if (MBIT == CBREAK || !slow_tty) {
	    otty.c_lflag &= ~ICANON;
	    otty.c_cc[VMIN] = MIN_CHAR;
	    otty.c_cc[VTIME] = TIME;
	}
    }
#else
    ospeed = otty.sg_ospeed;
    slow_tty = ospeed < B1200;
    hardtabs =  !(otty.sg_flags & XTABS);
    if (!no_tty) {
	otty.sg_flags &= ~ECHO;
	if (MBIT == CBREAK || !slow_tty)
	    otty.sg_flags |= MBIT;
    }
#endif
}

readch ()
{
	char ch;
	extern int errno;

	if (read (2, &ch, 1) <= 0)
		if (errno != EINTR) {
			reset_tty();
			fputs(catgets(catd, NL_SETN, 26, "Can't read stderr.\n"),stderr);
			exit(0);
		}
		else
			ch = otty.sg_kill;
	return (ch);
}

static char BS = '\b';
static char CARAT = '^';

ttyin (buf, nmax, pchar)
char buf[];
register int nmax;
char pchar;
{
    register char *sptr;
    register char ch;
    register int slash = 0;
    int	maxlen;
    char cbuf;
#ifdef NLS16
	/* For handling 16-bit characters we need to know 2 things */
	/*   1-how far do we backspace ? */
	/*   2-is slash really slash? (could be 2nd half of kanji) */
    int b_status = ONEBYTE;
    int bstatus[80];	/* always nmax=78 when called */
    int *statptr = bstatus;
#	define	ANDISASCII	&& b_status == ONEBYTE
#else
#	define	ANDISASCII
#endif

    sptr = buf;
    maxlen = 0;
    while (sptr - buf < nmax) {
	if (promptlen > maxlen) maxlen = promptlen;
	ch = readch ();
#ifdef NLS16	/* what is this byte, really? */	
	b_status = BYTE_STATUS((unsigned char)ch, b_status);
#endif
	if (ch == '\\' ANDISASCII) {
	    slash++;
	}
	else if ((ch == otty.sg_erase ANDISASCII) && !slash) {
	    if (sptr > buf) {
		--promptlen;
		write (2, &BS, 1);
		--sptr;
#ifdef NLS16
		--statptr;
#endif
		if ((((unsigned char)*sptr) < ' ' && *sptr != '\n') || *sptr == RUBOUT) {
		    --promptlen;
		    write (2, &BS, 1);
		}
#ifdef NLS16
		/* space back 2 over kanji */
		else if (*statptr == SECOF2){
#ifdef EUC
		    --sptr;
		    --statptr;
		    if ( C_COLWIDTH((int)*sptr) - 1 ){ 
		        --promptlen;
		        write (2, &BS, 1);
		    }
#else
		    --promptlen;
		    write (2, &BS, 1);
		    --sptr;
		    --statptr;
#endif EUC
		}
#endif
		continue;
	    }
	    else {
		if (!eraseln) promptlen = maxlen;
		longjmp (restore, 1);
	    }
	}
	else if ((ch == otty.sg_kill ANDISASCII) && !slash) {
	    if (hard) {
		show (ch);
		putchar ('\n');
		putchar (pchar);
	    }
	    else {
		putchar ('\r');
		putchar (pchar);
		if (eraseln)
		    erase (1);
		promptlen = 1;
	    }
	    sptr = buf;
#ifdef NLS16
	    statptr = bstatus;
#endif
	    fflush (stdout);
	    continue;
	}
	if (slash ANDISASCII && (ch == otty.sg_kill || ch == otty.sg_erase)) {
	    write (2, &BS, 1);
	    --sptr;
#ifdef NLS16
	    --statptr;
#endif
	}
	if (ch != '\\')
	    slash = 0;
	*sptr++ = ch;
#ifdef NLS16
	*statptr++ = b_status;
#endif
	if ((((unsigned char)ch) < ' ' && ch != '\n' && ch != ESC) || ch == RUBOUT) {
	    ch += ch == RUBOUT ? -0100 : 0100;
	    write (2, &CARAT, 1);
	    promptlen++;
	}
	cbuf = ch;
	if (ch != '\n' && ch != ESC) {
	    write (2, &cbuf, 1);
	    promptlen++;
	}
	else
	    break;
    }
    *--sptr = '\0';
    if (!eraseln) promptlen = maxlen;
    if (sptr - buf >= nmax - 1)
	error (catgets(catd, NL_SETN, 27, "Line too long"));
}

expand (outbuf, inbuf)
char *outbuf;
char *inbuf;
{
    register char *instr;
    register char *outstr;
    register char ch;
    char temp[200];
    int changed = 0;
#ifdef NLS16
    int b_status = ONEBYTE;
#endif

    instr = inbuf;
    outstr = temp;
    while ((ch = *instr++) != '\0'){
#ifdef NLS16
	b_status = BYTE_STATUS(((unsigned char)ch), b_status);
	if (b_status != ONEBYTE){
	    *outstr++ = ch;
	    continue;
	}
#endif
	switch (ch) {
	case '%':
	    if (!no_intty) {
		strcpy (outstr, fnames[fnum]);
		outstr += strlen (fnames[fnum]);
		changed++;
	    }
	    else
		*outstr++ = ch;
	    break;
	case '!':
	    if (!shellp)
		error (catgets(catd, NL_SETN, 28, "No previous command to substitute for"));
	    strcpy (outstr, shell_line);
	    outstr += strlen (shell_line);
	    changed++;
	    break;
	case '\\':
	    if (*instr == '%' || *instr == '!') {
		*outstr++ = *instr++;
		break;
	    }
	default:
	    *outstr++ = ch;
	}
    }
    *outstr++ = '\0';
    strcpy (outbuf, temp);
    return (changed);
}

show (ch)
register char ch;
{
    char cbuf;

    if (((unsigned char)ch < ' ' && ch != '\n' && ch != ESC) || ch == RUBOUT) {
	ch += ch == RUBOUT ? -0100 : 0100;
	write (2, &CARAT, 1);
	promptlen++;
    }
    cbuf = ch;
    write (2, &cbuf, 1);
    promptlen++;
}

error (mess)
char *mess;
{
    if (clreol)
	cleareol ();
    else
	kill_line ();
    promptlen += strlen (mess);
    if (Senter && Sexit) {
	tputs (Senter, 1, putch);
	pr(mess);
	tputs (Sexit, 1, putch);
    }
    else
	pr (mess);
    fflush(stdout);
    errors++;
    longjmp (restore, 1);
}


set_tty ()
{
#ifdef	USG	/*	mojo	*/
	otty.c_lflag &= ~ICANON;
	otty.c_lflag &= ~ECHO;
	otty.c_cc[VMIN] = MIN_CHAR;
	otty.c_cc[VTIME] = TIME;
#else
	otty.sg_flags |= MBIT;
	otty.sg_flags &= ~ECHO;
#endif
	stty(2, &otty);
}

reset_tty ()
{
    /* Don't reset the tty if output redirected */

    if (no_tty)
	return;

    stty(2, &origtty);
    /* solomon, 3/28/82.  Original version:
    otty.sg_flags |= ECHO;
    otty.sg_flags &= ~MBIT;
    stty(2, &otty);
    */
}

rdline (f)
register FILE *f;
{
    register int c;
    register char *p;

    p = Line;
    while ((c = Getc (f)) != '\n' && c != EOF && p - Line < LINSIZ - 1)
	*p++ = c;
    if (c == '\n')
	Currline++;
    *p = '\0';
}

/* Come here when we get a suspend signal from the terminal */

#ifdef SIGTSTP
onsusp ()
{
    reset_tty ();
    fflush (stdout);
    /* Send the TSTP signal to suspend our process group */
    kill (0, SIGTSTP);
    /* Pause for station break */

    /* We're back */
    signal (SIGTSTP, onsusp);
    set_tty ();

#ifdef SIGWINCH
    /* If window size has changed take the appropriate action */
    chgwinsz(); 	
#endif SIGWINCH

    if (inwait)
	    longjmp (restore);
}
#endif

/* nlspath(file) - return the full path to the nls library file */

#ifdef NLS
#include <msgbuf.h>

char *
nlspath (file)
char *file;			/* the file name */
{
	static char buf[100];	/* build name here */
	char *lang;		/* the pointer returned by getenv */
	extern char *getenv();

	strcpy(buf, NLSDIR);
	if ((lang = getenv("LANG")) == 0)
		return("");
	strcat(buf, lang);
	strcat(buf, "/");
	strcat(buf, file);
	return(buf);
}
#endif

