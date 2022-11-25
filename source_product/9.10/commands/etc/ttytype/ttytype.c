static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/*
 * ttytype -- a program to automatically recognize certain types of
 *            terminals and their dimensions.
 *
 * Currently supports Wyse 30/50/60 terminals (perhaps others),
 * Standard ANSI terminals, and Hewlett-Packard terminals.
 *
 * The source code, algorithms and methods contained within this
 * original work of the Hewlett-Packard Company is copyrighted material
 * and may not be distributed, copied, or modified in any way without
 * the explicit written consent of the Hewlett-Packard Company.
 *
 * Copyright (c) 1990 Hewlett-Packard Company, All Rights Reserved.
 * Company Confidential Material.
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#ifndef hpux
#include <strings.h>
#define strchr index
#define strrchr rindex
#else
#include <string.h>
#include <unistd.h>
#endif

#if defined(_POSIX_VERSION) && _POSIX_VERSION >= 198808L
#include <termios.h>
#define TERMIOS
#else
#include <termio.h>
#define tcflush(FD,OPT) ioctl(FD,TCFLSH,0)
#define tcgetattr(FD,TIO) ioctl(FD,TCGETA,TIO)
#define tcsetattr(FD,OPT,TIO) ioctl(FD,TCSETAF,TIO)
#endif

#define FALSE 0
#define TRUE  1

/* Timeout values for select call on tty */
#define TIMEOUT	  900		/* milliseconds until first char */
#define INTERVAL  900		/* milliseconds between chars	 */

typedef enum Termtype {UNKNOWN, ANSI, WYSE, HP, USER_SUPPLIED} Termtype;

extern char *getenv();

int term = -1;			/* file descriptor for terminal */
Termtype type = UNKNOWN;	/* type of terminal, ANSI, HP or WYSE */
int debug = FALSE;		/* internal debug info */
int verbose = FALSE;		/* if true, print more data */
int ask = TRUE;			/* prompt if we can't identify */
int ask_first = FALSE;		/* prompt before identifying */
int shell = FALSE;		/* output TERM, LINES & COLUMNS */
int tryWYSE = TRUE;		/* should we check for a WYSE term? */
int tryANSI = TRUE;		/* should we check for an ANSI term? */
int tryHP = TRUE;		/* should we check for an HP term? */
char erase[] = "^H";		/* backspace */
int lines = -1;
int columns = -1;
int timeout = TIMEOUT;
int interval = INTERVAL;
#ifdef TERMIOS
struct termios tio;		/* temp terminal settings */
struct termios otio;		/* orig terminal settings */
#else
struct termio tio;		/* temp terminal settings */
struct termio otio;		/* orig terminal settings */
#endif
char *defaultID = "hp";

char *progname;

/*
 * A message printed with -v (used multiple times)
 */
char cols_and_lines[] = "%s: COLUMNS=%d; LINES=%d\n";

void
quit(sig)
    int sig;
{
    char *ptr;

    /* flush I/O and restore tty settings */
    (void)tcsetattr(term, TCSANOW, &otio);

    close(term);

    if (sig != 0)
	sig |= 128;

    exit(sig);
}

/*
 * dump a string to stderr, converting control characters
 * to "^C" format
 */
void
dumpstr(fp, str)
    FILE *fp;
    char *str;
{
    char ch;

    while ((ch = *str++ & 0x7F) != '\0')
    {
	if (isprint(ch))
	    putc(ch, fp);
	else
	{
	    putc('^', fp);
	    putc(ch ^ 64, fp);
	}
    }
}

/*
 * get a response into t, return length
 */
getresp(t, lim)
    char *t;
    int lim;
{
    char ch;
    int i = 0;

    --lim;
    while (i < lim && read(term, &ch, 1) == 1 && ch != '\n')
	if (isprint(ch))
	    t[i++] = ch;
	else
	    write(term, "\007", 1);
    t[i] = '\0';
    return i;
}

/* Send str to term, and store response in buf.	 Return byte count. */
interrogate(str, buf, bufsize, endchars, flusherrs)
    char *str, *buf;
    int bufsize;
    char *endchars;
    int flusherrs;
{
    int readfds;
    char *ptr;
    char *tail = buf;
    int left = bufsize;
    int ret;
    struct timeval wait;

    if (debug)
    {
	fprintf(stderr, "\r%s: writing %d characters: \"",
	    progname, strlen(str));
	dumpstr(stderr, str);
	fputs("\"\n", stderr);
    }

    if (flusherrs)
	fflush(stderr);

    tcflush(term, TCIFLUSH);
    write(term, str, strlen(str));
    *buf = '\0';
    readfds = (1 << term);
    /*
     * Here we read the response from our inquisition (noone expects
     * the Spanish inquisition!).  We wait up to TIMEOUT milliseconds
     * for the first character to arrive.  Once we get a character, we
     * allow up to INTERVAL milliseconds after each character for more
     * characters to arrive.  Also, if endchar is nonzero and is
     * read from the input stream, we terminate the read early.
     */
    wait.tv_sec = timeout / 1000;
    wait.tv_usec = (timeout % 1000) * 1000;
    while (left > 1)		/* leave room for trailing null */
    {
	int n;

	/*
	 * Read some characters with a timeout.	 If we have characters
	 * to read, select will return > 0, otherwise, we are done.
	 */
	if (select(term + 1, &readfds, NULL, NULL, &wait) <= 0)
	    break;

	if ((n = read(term, tail, left)) <= 0)
	    break;

	left -= n;
	tail[n] = '\0';
	if (endchars && strpbrk(tail, endchars))
	    break;
	tail += n;
	wait.tv_sec = interval / 1000;
	wait.tv_usec = (interval % 1000) * 1000;
    }

    ret = bufsize - left;

    if (flusherrs)
    {
	ptr = "i\r     \r";
	write(term, ptr, strlen(ptr));	/* clean up mess on screen */
    }

    if (debug)
    {
	fprintf(stderr, "\r%s: read %d characters: \"", progname, ret);
	dumpstr(stderr, buf);
	fputs("\"\n", stderr);
    }

    /* remove all whitespace characters */
    if (ret > 0)
    {
	int i, j;

	for (i = j = 0; i < ret; i++)
	    if (!isspace(buf[i]))
		buf[j++] = buf[i];
	ret = j;
	buf[ret] = '\0';
    }

    if (flusherrs)
	fflush(stderr);

    return ret;
}

parse_pos(buf, firstp, first_sep, secondp, second_sep,
    first_fudge, second_fudge)
    char *buf;
    int *firstp;
    char *first_sep;
    int *secondp;
    char *second_sep;
    int first_fudge;
    int second_fudge;
{
    char *endp;
    int num1;
    int num2;

    /*
     * ccc is columns and rrr is rows (in ascii).
     */
    *firstp = *secondp = 0;
    num1 = strtol(buf, &endp, 10);
    if (strchr(first_sep, *endp) != (char *)0)
    {
	num2 = strtol(endp + 1, &endp, 10);
	if (strchr(second_sep, *endp) != (char *)0)
	    if (num1 > 0 && num2 > 0)
	    {
		*firstp = num1 + first_fudge;
		*secondp = num2 + second_fudge;
		return TRUE;
	    }
    }
    return FALSE;
}

int
isANSI(buf, bufsize)
    char *buf;
    int bufsize;
{
    char *ptr;
    int n = interrogate(".\033[c", buf, bufsize, "c", TRUE);

    if (n < 1)
	return FALSE;

    if (verbose)
    {
	fprintf(stderr, "%s: ANSI terminal response \"", progname);
	dumpstr(stderr, buf);
	fputs("\"", stderr);
	fflush(stderr);
    }

    if (n > 3 && buf[0] == '\033' && (ptr = strchr(buf, '?')) != NULL)
    {
	switch (atoi(ptr + 1))
	{
	case 0:
	    strcpy(buf, "vt180");
	    break;		/* VT180 (KERMIT 2.5) */
	case 1:
	    strcpy(buf, "vt100");
	    break;		/* VT100 */
	case 2:
	    strcpy(buf, "la120");
	    break;		/* LA120 */
	case 3:
	    strcpy(buf, "la38");
	    break;		/* LA34 or LA38 */
	case 4:
	    strcpy(buf, "vt132");
	    break;		/* VT132 */
	case 5:
	    strcpy(buf, "gigi");
	    break;		/* GIGI */
	case 6:
	    strcpy(buf, "vt100");
	    break;		/* was VT102, for ncsa telnet */
	case 7:
	    strcpy(buf, "vt131");
	    break;		/* VT131 */
	case 8:
	    strcpy(buf, "vt278");
	    break;		/* VT278 */
	case 9:
	    strcpy(buf, "lqp8f");
	    break;		/* LQP8F */
	case 10:
	    strcpy(buf, "la100");
	    break;		/* LA100 */
	case 11:
	    strcpy(buf, "la120j");
	    break;		/* LA120J (Katakana) */
	case 12:
	    strcpy(buf, "vt125");
	    break;		/* VT125 */
	case 13:
	    strcpy(buf, "lqp02");
	    break;		/* LQP02 */
	case 15:
	    strcpy(buf, "la12");
	    break;		/* LA12 */
	case 16:
	    strcpy(buf, "vt102j");
	    break;		/* VT102J (Katakana) */
	case 17:
	    strcpy(buf, "la50");
	    break;		/* LA50 Letter printer */
	case 18:
	    strcpy(buf, "vt80");
	    break;		/* VT80 */
	case 19:
	    strcpy(buf, "clatter");
	    break;		/* CLATTER */
	case 20:
	    strcpy(buf, "la80");
	    break;		/* LA80 */
	case 21:
	    strcpy(buf, "pc350");
	    break;		/* PC-350 in native mode */
	case 22:
	    strcpy(buf, "vt102");
	    break;		/* PRO350 in VT102 mode */
	case 23:
	    strcpy(buf, "vt102+");
	    break;		/* PRO350 in 8-bit VT102+ mode */
	case 62:
	    strcpy(buf, "vt200");
	    break;		/* VT200 family terminal */
	default:
	    break;		/* not a DEC */
	}
    }
    else
	return FALSE;

    if (verbose)
    {
	fprintf(stderr, " mapped to \"%s\"\n", buf);
	fflush(stderr);
    }

    if (shell)
    {
	char dimen[12];

	/*
	 * ANSI terminals can save their current position, so tell
	 * the terminal to save its position, go to 999,999, send
	 * back its current cursor position, and go back to where it
	 * was.
	 *
	 * ANSI terminals send back an "^[[rrr;cccR", where
	 * rrr is lines (rows) and ccc is columns
	 */
	if (interrogate("\0337\033[r\033[999;999H\033[6n\0338",
		    dimen, sizeof dimen, "R", TRUE))
	    if (parse_pos(dimen + 2, &lines, ";", &columns, "R", 0, 0))
		if (verbose)
		{
		    fprintf(stderr, cols_and_lines,
			progname, columns, lines);
		    fflush(stderr);
		}
    }
    return TRUE;
}

int
isWYSE(buf, bufsize)
    char *buf;
    int bufsize;
{
    int n = interrogate(".\033 ", buf, bufsize, "\n", TRUE);

    if (n < 1)
	return FALSE;

    if (verbose)
    {
	fprintf(stderr, "%s: WYSE terminal response \"", progname);
	dumpstr(stderr, buf);
	fputs("\"\n", stderr);
	fflush(stderr);
    }

    if (shell)
    {
	char pos[10];
	/*
	 * Save current cursor position, find location of lower-right
	 * corner, and then return cursor
	 */
	if (interrogate("\033b", pos, sizeof pos, "C", TRUE))
	{
	    char dimen[10];
	    char *endp;

	    /*
	     * NOTE:
	     *   The real wyse terminal doesn't let you move the cursor
	     *   to a position that isn't in the current window size
	     *   (like 999,999).  Instead, it ignores the escape
	     *   sequence, printing it to the display once it has
	     *   determined that is out of range.  Consequently, we use
	     *   a different method to get the cursor to the lower left
	     *   corner.
	     *
	     *   1. Home the cursor
	     *   2. Cursor up; no wrap             (moves to last line)
	     *   3. Backspace     (moves to last column, previous line)
	     *
	     *   This sequence means that we must add 1 to the number
	     *   of lines.
	     *
	     *   The HP terminal, emulating a wyse does the 999,999
	     *   move correctly, but this other method works for
	     *   both terminals (and is actually shorter).
	     */
	    interrogate("\033{\013\b\033b", dimen, sizeof dimen, "C",
		    FALSE);
	    write(term, "\033a", 2);
	    write(term, pos, strlen(pos));
	    fflush(stderr);

	    /*
	     * Parse "^rrrRcccC", where
	     * ccc is columns and rrr is rows (in ascii).
	     */
	    parse_pos(dimen, &lines, "R", &columns, "C", 1, 0);
	    if (verbose && columns > 0 && lines > 0)
	    {
		fprintf(stderr, cols_and_lines,
		    progname, columns, lines);
		fflush(stderr);
	    }
	}
    }

    return TRUE;
}

int
isHP(buf, bufsize)
    char *buf;
    int bufsize;
{
    int console, n;
    char pos[13];

    n = interrogate(".\033*s1^\021", buf, bufsize, "\n", TRUE);

    if (n < 1)
	return FALSE;

    if (verbose)
    {
	fprintf(stderr, "%s: HP terminal response \"", progname);
	dumpstr(stderr, buf);
	fputs("\"\n", stderr);
	fflush(stderr);
    }

    console = (strncmp(buf, "9000/3", 6) == 0 ||
	       strncmp(buf, "9000/4", 6) == 0);

    if (shell || console)
    {
	/*
	 * Save current cursor position, find location of lower-right
	 * corner, and then return cursor
	 */
	if (interrogate("\033`\021", pos, sizeof pos, "\n", TRUE))
	{
	    char dimen[13];
	    char *endp;

	    interrogate("\033&a999c999Y\033`\021",
		    dimen, sizeof dimen, "\n", FALSE);
	    write(term, pos, strlen(pos));
	    fflush(stderr);

	    /*
	     * Parse "^[&aCCCxRRRY" or "^[&aCCCcRRRY", where
	     * CCC is columns and RRR is rows (in ascii).
	     */
	    parse_pos(dimen+3, &columns, "xc", &lines, "Y", 1, 1);
	    if (verbose && columns > 0 && lines > 0)
	    {
		fprintf(stderr, cols_and_lines,
		    progname, columns, lines);
		fflush(stderr);
	    }

	    if (console)
	    {
		switch (lines)
		{
		case 20:
		    strcpy(buf, "9845");
		    break;
		case 23:
		    strcpy(buf, "9826");
		    break;
		case 24:
		    strcpy(buf, "300l");
		    break;
		case 28:
		    strcpy(buf, "98541");
		    break;
		case 46:
		    strcpy(buf, "300h");
		    break;
		case 47:
		    strcpy(buf, "9837");
		    break;
		case 49:
		    strcpy(buf, "98548");
		    break;
		default:
		    if (shell)
		    {
			/* doesn't matter, user has LINES and COLUMNS */
			strcpy(buf, "hp");
		    }
		    else
		    {
			/* have to prompt */
			defaultID = "hp";
			return FALSE;
		    }
		    break;
		}
		if (verbose)
		{
		    fprintf(stderr, "%s: id mapped to \"%s\"\n",
			progname, buf);
		    fflush(stderr);
		}
	    }
	}
    }

    return TRUE;
}

void
setup_signals()
{
#ifdef SIG_SETMASK
    struct sigaction action;

    (void)sigemptyset(&action.sa_mask);
    (void)sigaddset(&action.sa_mask, SIGHUP);
    (void)sigaddset(&action.sa_mask, SIGINT);
    (void)sigaddset(&action.sa_mask, SIGTERM);
    action.sa_handler = quit;
    action.sa_flags = 0;

    (void)sigaction(SIGHUP, &action, (struct sigaction *)0);
    (void)sigaction(SIGINT, &action, (struct sigaction *)0);
    (void)sigaction(SIGTERM, &action, (struct sigaction *)0);
#else
    signal(SIGHUP, quit);
    signal(SIGINT, quit);
    signal(SIGTERM, quit);
#endif
}

main(argc, argv)
    int argc;
    char **argv;
{
    char id[20];		/* terminfo ID string */
    char *ttydev = "/dev/tty";	/* default device to identify */
    char c;			/* utility */
    int gave_types = FALSE;	/* any -t options? */
    char iobuffer[BUFSIZ];	/* we want stderr buffered */

    extern char *optarg;	/* for getopt(3) */
    extern int optind, opterr;	/* for getopt(3) */

    if ((progname = strrchr(argv[0], '/')) == (char *)0)
	progname = argv[0];
    else
	progname++;

    id[0] = '\0';
    setvbuf(stderr, iobuffer, _IOFBF, BUFSIZ);

    opterr = 0;
    while ((c = getopt(argc, argv, "aghpsvt:DT:R:C:")) != EOF)
	switch (c)
	{
	case 'D':
	    debug = TRUE;
	case 'v': /* verbose */
	    verbose = TRUE;
	    break;
	case 's':
	    shell = TRUE;
	    break;
	case 'a':
	    ask = FALSE;
	    break;
	case 'p':
	    ask_first = TRUE;
	    break;
	case 't':
	    {
		char *s;

		if (!gave_types)
		{
		    tryWYSE = tryANSI = tryHP = FALSE;
		    gave_types = TRUE;
		}

		for (s = optarg; *s; s++)
		    *s = tolower(*s);

		if (strcmp(optarg, "hp") == 0)
		    tryHP = TRUE;
		else if (strcmp(optarg, "wyse") == 0)
		    tryWYSE = TRUE;
		else if (strcmp(optarg, "ansi") == 0 ||
			strcmp(optarg, "dec") == 0)
		    tryANSI = TRUE;
		else
		{
		    fprintf(stderr,
			"%s: argument to -t must be \"ansi\", \"hp\" or \"wyse\"\n", progname);
		    exit(1);
		}
	    }
	    break;
	case 'T':
	    ttydev = optarg;
	    break;
	case 'R': /* max response time */
	    if ((timeout = atoi(optarg)) < 0)
		timeout = 0;
	    break;
	case 'C': /* max inter-character time */
	    if ((interval = atoi(optarg)) < 0)
		interval = 0;
	    break;
	default:
	    fprintf(stderr, "Usage: %s [-apsv] [-t type]\n", progname);
	    exit(2);
	}

    /* open terminal for read/write with no wait-delay for input */
    if ((term = open(ttydev, O_RDWR | O_NDELAY)) <= 0)
    {
	fprintf(stderr, "%s: couldn't open %s for reading\n",
	    progname, ttydev);
	exit(1);
    }

    if (ask_first)
    {
	int flags;

	/*
	 * Oops, we have to turn off O_NDELAY first.
	 */
	if ((flags = fcntl(term, F_GETFL)) != -1)
	    (void)fcntl(term, F_SETFL, (flags & (~O_NDELAY)));

	write(term, "TERM = ", 7);
	if (getresp(id, sizeof id))
	    type = USER_SUPPLIED;

	/*
	 * Restore O_NDELAY to its original value
	 */
	(void)fcntl(term, F_SETFL, flags);
    }

    if (type == UNKNOWN)
    {
	/* kill echo & set unbuffered input mode & flush the input */
	if (tcgetattr(term, &tio) == -1)
	{
	    fputs(progname, stderr);
	    fflush(stderr);
	    perror(": couldn't get current tty settings");
	    exit(1);
	}
	otio = tio;			/* save original values */
	tio.c_iflag = IGNPAR | ISTRIP | ICRNL;
	tio.c_oflag = OPOST | ONLCR;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 0;

	/*
	 * set handler before we change tty settings, then change
	 * the settings to what we want.
	 * Finally, we trash the type-ahead buffer.
	 */
	setup_signals();
	if (tcsetattr(term, TCSAFLUSH, &tio) == -1)
	{
	    fputs(progname, stderr);
	    fflush(stderr);
	    perror(": couldn't change current tty settings");
	    exit(1);
	}

	if (tryWYSE && isWYSE(id, sizeof id))
	    type = WYSE;
	else if (tryANSI && isANSI(id, sizeof id))
	{
	    type = ANSI;
	    erase[1] = '?'; /* DEL */
	}
	else if (tryHP && isHP(id, sizeof id))
	    type = HP;

	/*
	 * Restore terminal to original tty settings
	 */
	(void)tcsetattr(term, TCSANOW, &otio);

	if (type == UNKNOWN)
	    if (ask)
	    {
		/*
		 * prompt for type, "What we have here is a failure to
		 * communicate ..."
		 * flush input & turn on echo, canon
		 */
		char msg[80];		/* for output messages	 */
		int flags;

		/*
		 * Turn off O_NDELAY.
		 */
		if ((flags = fcntl(term, F_GETFL)) != -1)
		{
		    flags &= ~O_NDELAY;
		    (void)fcntl(term, F_SETFL, flags);
		}

		/*
		 * Now prompt for the terminal type and read the
		 * response.  We prompt directly to /dev/tty.
		 */
		sprintf(msg, "TERM = (%s) ", defaultID);
		write(term, msg, strlen(msg));
		if (!getresp(id, sizeof id))
		    strcpy(id, defaultID);  /* or set to default */

		if (verbose && *id)
		{
		    fprintf(stderr,
			"%s: manual terminal response is \"%s\"\n",
			progname, id);
		    fflush(stderr);
		}
	    }
	    else
		strcpy(id, "unknown");
    }

    if (shell)
    {
	static char TERM[] = "TERM";
	static char LINES[] = "LINES";
	static char COLUMNS[] = "COLUMNS";
	static char ERASE[] = "ERASE";
	char *sformat = "%s='%s'; export %s;\n";
	char *dformat = "%s=%d; export %s;\n";
	char *SHELL = getenv("SHELL");

	if (SHELL == NULL)
	    SHELL = "sh";
	else
	{
	    char *cp = strrchr(SHELL, '/');
	    if (cp != NULL)
		SHELL = cp + 1;
	}

	if (strcmp(SHELL, "csh") == 0 || strcmp(SHELL, "tcsh") == 0)
	{
	    /* we have a brain-damaged csh user... */
	    sformat = "setenv %s %s;\n";
	    dformat = "setenv %s %d;\n";
	}

	printf(sformat, TERM, id, TERM);
	if (lines > 0)
	    printf(dformat, LINES, lines, LINES);
	if (columns > 0)
	    printf(dformat, COLUMNS, columns, COLUMNS);
	printf(sformat, ERASE, erase, ERASE);
    }
    else
	puts(id);

    return 0;
}
