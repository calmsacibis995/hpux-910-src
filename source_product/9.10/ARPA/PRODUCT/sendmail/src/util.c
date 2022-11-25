/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# ifndef lint
static char rcsid[] = "$Header: util.c,v 1.20.109.11 95/03/22 17:44:03 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)util.c	5.18 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: util.o $Revision: 1.20.109.11 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# include <stdio.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sysexits.h>
# include <errno.h>
# include <ctype.h>
# include "sendmail.h"
# include <sys/socket.h>
# include <pwd.h>

/*
**  STRIPQUOTES -- Strip quotes & quote bits from a string.
**
**	Runs through a string and strips off unquoted quote
**	characters and quote bits.  This is done in place.
**
**	Parameters:
**		s -- the string to strip.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called By:
**		deliver
*/

void stripquotes(s)
	char *s;
{
	register char *p;
	register char *q;
	register char c;

	if (s == NULL)
		return;

	p = q = s;
	do
	{
		c = *p++;
		if (c == '\\')
			c = *p++;
		else if (c == '"')
			continue;
		*q++ = c;
	} while (c != '\0');
}
/*
**  CAPITALIZE -- return a copy of a string, properly capitalized.
**
**	Parameters:
**		s -- the string to capitalize.
**
**	Returns:
**		a pointer to a properly capitalized string.
**
**	Side Effects:
**		none.
*/

char *
capitalize(s)
	register char *s;
{
	static char buf[50];
	register char *p;

	p = buf;

	for (;;)
	{
		while (!isalpha(*s) && *s != '\0')
			*p++ = *s++;
		if (*s == '\0')
			break;
		*p++ = toupper(*s);
		s++;
		while (isalpha(*s))
			*p++ = *s++;
	}

	*p = '\0';
	return (buf);
}
/*
**  XALLOC -- Allocate memory and bitch wildly on failure.
**
**	THIS IS A CLUDGE.  This should be made to give a proper
**	error -- but after all, what can we do?
**
**	Parameters:
**		sz -- size of area to allocate.
**
**	Returns:
**		pointer to data region.
**
**	Side Effects:
**		Memory is allocated.
*/

char *
xalloc(sz)
	register int sz;
{
	register char *p;
	extern char *malloc();

	p = malloc((unsigned) sz);
	if (p == NULL)
	{
		syserr("Out of memory!!");
		abort();
		/* exit(EX_UNAVAILABLE); */
	}
	return (p);
}
/*
**  COPYPLIST -- copy list of pointers.
**
**	This routine is the equivalent of newstr for lists of
**	pointers.
**
**	Parameters:
**		list -- list of pointers to copy.
**			Must be NULL terminated.
**		copycont -- if TRUE, copy the contents of the vector
**			(which must be a string) also.
**
**	Returns:
**		a copy of 'list'.
**
**	Side Effects:
**		none.
*/

char **
copyplist(list, copycont)
	char **list;
	bool copycont;
{
	register char **vp;
	register char **newvp;
	int listsize;

	for (vp = list; *vp != NULL; vp++)
		continue;

	vp++;

	listsize = max(MAXATOM + 1, vp - list) * sizeof *vp;
	newvp = (char **) xalloc(listsize);
	bzero((char *) newvp, listsize);
	bcopy((char *) list, (char *) newvp, (vp - list) * sizeof *vp);

	if (copycont)
	{
		for (vp = newvp; *vp != NULL; vp++)
			*vp = newstr(*vp);
	}

	return (newvp);
}
/*
**  PRINTAV -- print argument vector.
**
**	Parameters:
**		av -- argument vector.
**
**	Returns:
**		none.
**
**	Side Effects:
**		prints av.
*/

void printav(av)
	register char **av;
{
	while (*av != NULL)
	{
# ifdef DEBUG
		if (tTd(0, 44))
			printf("\n\t%08x=", *av);
		else
# endif /* DEBUG */
			(void) putchar(' ');
		xputs(*av++);
	}
	(void) putchar('\n');
}
/*
**  LOWER -- turn letter into lower case.
**
**	Parameters:
**		c -- character to turn into lower case.
**
**	Returns:
**		c, in lower case.
**
**	Side Effects:
**		none.
*/

char
lower(c)
	register char c;
{
	return((isascii(c) && isupper(c)) ? tolower(c) : c);
}
/*
**  XPUTS -- put string doing control escapes.
**
**	Parameters:
**		s -- string to put.
**
**	Returns:
**		none.
**
**	Side Effects:
**		output to stdout
*/

void xputs(s)
	register char *s;
{
	register char c;

	if (s == NULL)
	{
		printf("<null>");
		return;
	}
	(void) putchar('"');
	while ((c = *s++) != '\0')
	{
		if (!isascii(c))
		{
			(void) putchar('\\');
			c &= 0177;
		}
		if (c < 040 || c >= 0177)
		{
			(void) putchar('^');
			c ^= 0100;
		}
		(void) putchar(c);
	}
	(void) putchar('"');
	(void) fflush(stdout);
}
/*
**  MAKELOWER -- Translate a line into lower case
**
**	Parameters:
**		p -- the string to translate.  If NULL, return is
**			immediate.
**
**	Returns:
**		none.
**
**	Side Effects:
**		String pointed to by p is translated to lower case.
**
**	Called By:
**		parse
*/

void makelower(p)
	register char *p;
{
	register char c;

	if (p == NULL)
		return;
	for (; (c = *p) != '\0'; p++)
		if (isascii(c) && isupper(c))
			*p = tolower(c);
}
/*
**  BUILDFNAME -- build full name from gecos style entry.
**
**	This routine interprets the strange entry that would appear
**	in the GECOS field of the password file.
**
**	Parameters:
**		p -- name to build.
**		login -- the login name of this user (for &).
**		buf -- place to put the result.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

void buildfname(p, login, buf)
	register char *p;
	char *login;
	char *buf;
{
	register char *bp = buf;

	if (*p == '*')
		p++;
	while (*p != '\0' && *p != ',' && *p != ';' && *p != '%')
	{
		if (*p == '&')
		{
			(void) strcpy(bp, login);
			*bp = toupper(*bp);
			while (*bp != '\0')
				bp++;
			p++;
		}
		else
			*bp++ = *p++;
	}
	*bp = '\0';
}
/*
**  SAFEFILE -- return true if a file exists and is safe for a user.
**
**	Parameters:
**		fn -- filename to check.
**		uid -- user id to compare against.
**		mode -- mode bits that must match.
**
**	Returns:
**		TRUE if fn exists, is owned by uid, and matches mode.
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

bool
safefile(fn, uid, mode)
	char *fn;
	int uid;
	int mode;
{
	struct stat stbuf;

	if (stat(fn, &stbuf) >= 0 && stbuf.st_uid == uid &&
	    (stbuf.st_mode & mode) == mode)
		return (TRUE);
	errno = 0;
	return (FALSE);
}
/*
**  FIXCRLF -- fix <CR><LF> in line.
**
**	Looks for the <CR><LF> combination and turns it into the
**	UNIX canonical <NL> character.  It only takes one line,
**	i.e., it is assumed that the first <NL> found is the end
**	of the line.
**	It is also assumed that there is room to add a NULL to line.
**
**	Parameters:
**		line -- the line to fix.
**		stripnl -- if true, strip the newline also.
**
**	Returns:
**		none.
**
**	Side Effects:
**		line is changed in place.
*/

void fixcrlf(line, stripnl)
	char *line;
	bool stripnl;
{
	register char *p;

	p = strchr(line, '\n');
	if (p == NULL)
		return;
	if (p > line && p[-1] == '\r')
		p--;
	if (!stripnl)
		*p++ = '\n';
	*p = '\0';
}
/*
**  DFOPEN -- determined file open
**
**	This routine has the semantics of fopen, except that it will
**	keep trying a few times to make this happen.  The idea is that
**	on very loaded systems, we may run out of resources (inodes,
**	whatever), so this tries to get around it.
*/

FILE *
dfopen(filename, mode)
	char *filename;
	char *mode;
{
	register int tries;
	register FILE *fp;

	for (tries = 0; tries < 10; tries++)
	{
		sleep((unsigned) (10 * tries));
		errno = 0;
		fp = fopen(filename, mode);
		if (fp != NULL)
			break;
		if (errno != ENFILE && errno != EINTR)
			break;
	}
	errno = 0;
	return (fp);
}
/*
**  PUTLINE -- put a line like fputs obeying SMTP conventions
**
**	This routine always guarantees outputing a newline (or CRLF,
**	as appropriate) at the end of the string.
**
**	Parameters:
**		l -- line to put.
**		fp -- file to put it onto.
**		m -- the mailer used to control output.
**
**	Returns:
**		none
**
**	Side Effects:
**		output of l to fp.
*/

# define SMTPLINELIM	990	/* maximum line length */

putline(l, fp, m)
	register char *l;
	FILE *fp;
	MAILER *m;
{
	register char *p;
	char svchar;

	/* strip out 0200 bits -- these can look like TELNET protocol */
	if (bitnset(M_LIMITS, m->m_flags))
	{
		p = l;
		while ((*p++ &= ~0200) != 0)
			continue;
	}

	do
	{
		/* find the end of the line */
		p = strchr(l, '\n');
		if (p == NULL)
			p = &l[strlen(l)];

		/* check for line overflow */
		while ((p - l) > SMTPLINELIM && bitnset(M_LIMITS, m->m_flags))
		{
			register char *q = &l[SMTPLINELIM - 1];

			svchar = *q;
			*q = '\0';
			if (l[0] == '.' && bitnset(M_XDOT, m->m_flags))
				(void) putc('.', fp);
			fputs(l, fp);
			(void) putc('!', fp);
			fputs(m->m_eol, fp);
			*q = svchar;
			l = q;
		}

		/* output last part */
		svchar = *p;
		*p = '\0';
		if (l[0] == '.' && bitnset(M_XDOT, m->m_flags)) 
			(void) putc('.', fp);
		fputs(l, fp);
		fputs(m->m_eol, fp);
		*p = svchar;
		l = p;
		if (*l == '\n')
			++l;
	} while (l[0] != '\0');
}
/*
**  XUNLINK -- unlink a file, doing logging as appropriate.
**
**	Parameters:
**		f -- name of file to unlink.
**
**	Returns:
**		none.
**
**	Side Effects:
**		f is unlinked.
*/

void xunlink(f)
	char *f;
{
	register int i;

# ifdef LOG
	if (LogLevel > 20)
		syslog(LOG_DEBUG, "%s: unlink %s", CurEnv->e_id, f);
# endif /* LOG */

	i = unlink(f);
# ifdef LOG
	if (i < 0 && LogLevel > 21)
		syslog(LOG_DEBUG, "%s: unlink-fail %d", f, errno);
# endif /* LOG */
}
/*
**  SFGETS -- "safe" fgets -- times out and ignores random interrupts.
**
**	Parameters:
**		buf -- place to put the input line.
**		siz -- size of buf.
**		fp -- file to read from.
**
**	Returns:
**		NULL on error (including timeout).  This will also leave
**			buf containing a null string.
**		buf otherwise.
**
**	Side Effects:
**		none.
*/

static jmp_buf	CtxReadTimeout;

char *
sfgets(buf, siz, fp)
	char *buf;
	int siz;
	FILE *fp;
{
	register EVENT *ev = NULL;
	register char *p;
	extern readtimeout();

	/* set the timeout */
	if (ReadTimeout != 0)
	{
		if (setjmp(CtxReadTimeout) != 0)
		{
# ifdef LOG
			syslog(LOG_NOTICE,
			    "timeout waiting for input from %s\n",
			    RealHostName? RealHostName : "local host");
# endif	/* LOG */
			errno = 0;
			usrerr("451 timeout waiting for input");
			buf[0] = '\0';
			return (NULL);
		}
		ev = setevent((time_t) ReadTimeout, readtimeout, 0);
	}

	/* try to read */
	p = NULL;
	while (!feof(fp) && !ferror(fp))
	{
		errno = 0;
		p = fgets(buf, siz, fp);
		if (p != NULL || errno != EINTR)
			break;
		clearerr(fp);
	}

	/* clear the event if it has not sprung */
	clrevent(ev);

	/* clean up the books and exit */
	LineNumber++;
	if (p == NULL)
	{
		buf[0] = '\0';
		return (NULL);
	}
	for (p = buf; *p != '\0'; p++)
# ifdef hpux
		if (*p & 0200)
			Saw8Bits = TRUE;
# else	/* ! hpux */
		*p &= ~0200;
# endif /* hpux */
	return (buf);
}

static
readtimeout()
{
	longjmp(CtxReadTimeout, 1);
}
/*
**  FGETFOLDED -- like fgets, but know about folded lines.
**
**	Parameters:
**		buf -- place to put result.
**		n -- bytes available.
**		f -- file to read from.
**
**	Returns:
**		buf on success, NULL on error or EOF.
**
**	Side Effects:
**		buf gets lines from f, with continuation lines (lines
**		with leading white space) appended.  CRLF's are mapped
**		into single newlines.  Any trailing NL is stripped.
*/

char *
fgetfolded(buf, n, f)
	char *buf;
	register int n;
	FILE *f;
{
	register char *p = buf;
	register int i;

	n--;
	while ((i = getc(f)) != EOF)
	{
		if (i == '\r')
		{
			i = getc(f);
			if (i != '\n')
			{
				if (i != EOF)
					(void) ungetc(i, f);
				i = '\r';
			}
		}
		if (--n > 0)
			*p++ = i;
		if (i == '\n')
		{
			LineNumber++;
			i = getc(f);
			if (i != EOF)
				(void) ungetc(i, f);
			if (i != ' ' && i != '\t')
			{
				*--p = '\0';
				return (buf);
			}
		}
	}
	return (NULL);
}
/*
**  CURTIME -- return current time.
**
**	Parameters:
**		none.
**
**	Returns:
**		the current time.
**
**	Side Effects:
**		none.
*/

time_t
curtime()
{
	auto time_t t;

	(void) time(&t);
	return (t);
}
/*
**  ATOBOOL -- convert a string representation to boolean.
**
**	Defaults to "TRUE"
**
**	Parameters:
**		s -- string to convert.  Takes "tTyY" as true,
**			others as false.
**
**	Returns:
**		A boolean representation of the string.
**
**	Side Effects:
**		none.
*/

bool
atobool(s)
	register char *s;
{
	if (*s == '\0' || strchr("tTyY", *s) != NULL)
		return (TRUE);
	return (FALSE);
}
/*
**  ATOOCT -- convert a string representation to octal.
**
**	Parameters:
**		s -- string to convert.
**
**	Returns:
**		An integer representing the string interpreted as an
**		octal number.
**
**	Side Effects:
**		none.
*/

atooct(s)
	register char *s;
{
	register int i = 0;

	while (*s >= '0' && *s <= '7')
		i = (i << 3) | (*s++ - '0');
	return (i);
}
/*
**  WAITFOR -- wait for a particular process id.
**
**	Parameters:
**		pid -- process id to wait for.
**
**	Returns:
**		status of pid.
**		-1 if pid never shows up.
**
**	Side Effects:
**		none.
*/

waitfor(pid)
	int pid;
{
	auto int st;
	int i;

	do
	{
		errno = 0;
		i = wait(&st);
	} while ((i >= 0 || errno == EINTR) && i != pid);
	if (i < 0)
		st = -1;
	return (st);
}
/*
**  BITINTERSECT -- tell if two bitmaps intersect
**
**	Parameters:
**		a, b -- the bitmaps in question
**
**	Returns:
**		TRUE if they have a non-null intersection
**		FALSE otherwise
**
**	Side Effects:
**		none.
*/

bool
bitintersect(a, b)
	BITMAP a;
	BITMAP b;
{
	int i;

	for (i = BITMAPBYTES / sizeof (int); --i >= 0; )
		if ((a[i] & b[i]) != 0)
			return (TRUE);
	return (FALSE);
}
/*
**  BITZEROP -- tell if a bitmap is all zero
**
**	Parameters:
**		map -- the bit map to check
**
**	Returns:
**		TRUE if map is all zero.
**		FALSE if there are any bits set in map.
**
**	Side Effects:
**		none.
*/

bool
bitzerop(map)
	BITMAP map;
{
	int i;

	for (i = BITMAPBYTES / sizeof (int); --i >= 0; )
		if (map[i] != 0)
			return (FALSE);
	return (TRUE);
}
/*
**  ISASOCKET -- return true if file descriptor parameter refers to a socket
**
**	Parameters:
**		fildes -- file descriptor to examine
**
**	Returns:
**		if fildes refers to a socket, 1; otherwise 0.
**
**	Side Effects:
**		none.
**		
*/

isasocket(fildes)
int fildes;
{
    struct sockaddr addr;
    int addrlen = sizeof(struct sockaddr);
    int saveerrno = errno;

    if (getsockname(fildes, &addr, &addrlen) < 0) {
	switch (errno) {
	    case ENOBUFS:
	    case EINVAL:
		errno = saveerrno;
		return(1);
	    default:
		errno = saveerrno;
		return(0);
	}
    } else {
	errno = saveerrno;
	return(1);
    }
}
/*
**  DOUBLEPERCENT -- double '%' characters in a string to prevent
**  them being interpreted by later calls to the printf() routines.
**
**	Parameters:
**		s -- string to copy
**
**	Returns:
**		new string with all % doubled
**
**	Side Effects:
**		none.
**		
*/

char *
doublepercent(s)
char *s;
{
    char buf[MAXLINE + MAXATOM];
    char *p = buf;

    if (s == NULL)
	return(s);
    while ((p - buf + 1) < sizeof buf) {
	if (*s == '%')
	    *p++ = *s;
	if ((*p++ = *s++) == '\0')
		break;
    }
    return(newstr(buf));
}
/*
**	UUCPNAME -- programmatic version of "uuname -l"
**
**	copies utsn.nodename, or if on a diskless cluster,
**	the server's name, to name.
**	
**	Parameters:
**		name -- the buffer to put name in.  This should be 
**			at least SYSNSIZE + 1 chars long.
**
**	Returns:
**		none.
**
**	Side Effects:
**		copies something to name.
**
**	Called By:
**		main, to set the macro k.
*/

# include <sys/utsname.h>

# if defined(DUX) || defined(DISKLESS)
# 	include <cluster.h>
# endif	/* defined (DUX) || defined(DISKLESS) */

# define SYSNSIZE        7

int
uucpname(name)
register char *name;
{
	register char *s, *d;
	char servername[24];
	struct utsname utsn;

	if (uname(&utsn) < 0)
		return(-1);
	s = utsn.nodename;

# if defined(DUX) || defined(DISKLESS)
	if (getservername(servername) == 0)
		s = servername;
# endif	/* defined (DUX) || defined(DISKLESS) */

	d = name;
	while ((*d = *s++) && d < name + SYSNSIZE)
		d++;
	*(name + SYSNSIZE) = '\0';
	return(0);
}

# if defined(DUX) || defined(DISKLESS)
int
getservername(servername)
char *servername;
{
	struct cct_entry *cctptr;

	while ((cctptr = getccent()) != (struct cct_entry *)0)
	{
		if (cctptr->cnode_type == 'r')
		{
			strcpy(servername, cctptr->cnode_name);
			endccent();
			return(0);
		}
	}
	return(1);
}
# endif	/* defined (DUX) || defined(DISKLESS) */
/*
**  SHORTENSTRING -- return short version of a string
**
**	If the string is already short, just return it.  If it is too
**	long, return the head and tail of the string.
**
**	Parameters:
**		s -- the string to shorten.
**		m -- the max length of the string.
**
**	Returns:
**		Either s or a short version of s.
*/

# ifndef MAXSHORTSTR
# 	define MAXSHORTSTR	203
# endif	/* not MAXSHORTSTR */

char *
shortenstring(s, m)
	register char *s;
	int m;
{
	int l;
	static char buf[MAXSHORTSTR + 1];

	l = strlen(s);
	if (l < m)
		return s;
	if (m > MAXSHORTSTR)
		m = MAXSHORTSTR;
	else if (m < 10)
	{
		if (m < 5)
		{
			strncpy(buf, s, m);
			buf[m] = '\0';
			return buf;
		}
		strncpy(buf, s, m - 3);
		strcpy(buf + m - 3, "...");
		return buf;
	}
	m = (m - 3) / 2;
	strncpy(buf, s, m);
	strcpy(buf + m, "...");
	strcpy(buf + m + 3, s + l - m);
	return buf;
}

/*
**  CLEANSTRCPY -- copy string keeping out bogus characters
**
**	Parameters:
**		t -- "to" string.
**		f -- "from" string.
**		l -- length of space available in "to" string.
**
**	Returns:
**		none.
*/

void cleanstrcpy (t, f, l)
register char  *t;
register char  *f;
int     l;
{
# ifdef LOG
    /* check for newlines and log if necessary */
    (void)denlstring (f, TRUE, TRUE);
# endif	/* LOG */

    l--;
    while (l > 0 && *f != '\0')
        {
	if (isascii (*f) &&
		(isalnum (*f) || strchr ("!#$%&'*+-./^_`{|}~", *f) != NULL))
	    {
	    l--;
	    *t++ = *f;
	    }
	f++;
        }
    *t = '\0';
}

/*
**  FAKE_UNDERBAR -- compute the value of the mythical $_ macro.
**
**	Parameters:
**		none
**
**	Returns:
**		A pointer to a char buffer containing the originating
**		machine name. This value should be copied since it's
**		in a static buffer.
*/

static char * fake_underbar()
{
	static char hbuf[MAXNAME];
	char RealUserName[256];
	struct passwd *pw;
	char *p;

	uid_t RealUid;
	
	RealUid = getuid();

	pw = getpwuid(RealUid);
	if (pw != NULL)
		(void) strcpy(RealUserName, pw->pw_name);
	else
		(void) sprintf(RealUserName, "Unknown UID %d", RealUid);

	if (RealHostName == NULL)
	{
		sprintf(hbuf, " %s@localhost", RealUserName);
		return hbuf;
	}
	(void) strcpy(hbuf, RealHostName);

finish:
	if (RealHostName != NULL && RealHostName[0] != '[')
	{
		p = &hbuf[strlen(hbuf)];
		(void) sprintf(p, " [%s]", inet_ntoa(RealHostAddr.sin_addr));
	}
	if (tTd(9, 1))
		printf("fake_underbar: %s\n", hbuf);
	return hbuf;
}

/*
**  DENLSTRING -- convert newlines in a string to spaces
**
**	Parameters:
**		s -- the input string
**		strict -- if set, don't permit continuation lines
**		logattacks -- if set, log attempted attacks.
**
**	Returns:
**		A pointer to a version of the string with newlines
**		mapped to spaces.  This should be copied.
*/

char   * denlstring (s, strict, logattacks)
char   *s;
int 	strict;
int	logattacks;
{
    register char  *p;
    int     l;
    static char    *bp = NULL;
    static int  bl = 0;

    p = s;
    while ((p = strchr(p, '\n')) != NULL)
	if (strict || (*++p != ' ' && *p != '\t'))
		break;

    if (p == NULL)
	return s;

    l = strlen (s) + 1;
    if (bl < l)
        {
	/* allocate more space */
	if (bp != NULL)
	    free (bp);
	bp = xalloc (l);
	bl = l;
        }
    strcpy (bp, s);
    for (p = bp; (p = strchr (p, '\n')) != NULL;)
	*p++ = ' ';

# ifdef LOG
    if (logattacks)
	{
	p = fake_underbar();
    	syslog (LOG_ALERT, "POSSIBLE ATTACK from %s: newline in string \"%s\"",
	    p == NULL ? "[UNKNOWN]" : p, bp);
	}
# endif	/* LOG */

    return bp;
}
