/* $Header: msyslog.c,v 1.2.109.2 94/10/31 10:29:06 mike Exp $
 * msyslog - either send a message to the terminal or print it on
 *	     the standard output.
 *
 * Converted to use varargs, much better ... jks
 */
# include <stdio.h>
# include <string.h>
# include <errno.h>

/* alternative, as Solaris 2.x defines __STDC__ as 0 in a largely standard
   conforming environment
   #if __STDC__ || (defined(SOLARIS) && defined(__STDC__))
*/
# ifdef __STDC__
# 	include <stdarg.h>
# else	/* ! __STDC__ */
# 	include <varargs.h>
# endif	/* __STDC__ */

# include "ntp_syslog.h"
# include "ntp_stdlib.h"

int     syslogit = 0;
extern int  errno;
extern char    *progname;

# if defined(__STDC__)
void    msyslog (int    level, char    *fmt,...)
# else	/* ! defined (__STDC__) */
/*VARARGS*/
void    msyslog (va_alist)
va_dcl
# endif	/* defined (__STDC__) */
{
# ifndef __STDC__
    int     level;
    char   *fmt;
# endif	/* not __STDC__ */
    va_list     ap;
    char    buf[1025],
            nfmt[256],
            xerr[50],
           *err;
    register int    c,
                    l;
    register char  *n,
                   *f;
    extern int  sys_nerr;
    extern char    *sys_errlist[];
    int     olderrno;

# ifdef __STDC__
    va_start (ap, fmt);
# else	/* ! __STDC__ */
    va_start (ap);
    level = va_arg (ap, int);
    fmt = va_arg (ap, char *);
# endif	/* __STDC__ */

    olderrno = errno;
    n = nfmt;
    f = fmt;
    while ((c = *f++) != '\0' && c != '\n' && n < &nfmt[252])
        {
	if (c != '%')
	    {
	    *n++ = c;
	    continue;
	    }
	if ((c = *f++) != 'm')
	    {
	    *n++ = '%';
	    *n++ = c;
	    continue;
	    }
	if ((unsigned)olderrno > sys_nerr)
	    sprintf ((err = xerr), "error %d", olderrno);
	else
	err = sys_errlist[olderrno];
	if (n + (l = strlen (err)) < &nfmt[254])
	    {
	    strcpy (n, err);
	    n += strlen (err);
	    }
        }
    *n++ = '\n';
    *n = '\0';

    vsprintf (buf, nfmt, ap);

    if (syslogit)
	syslog (level, buf);
    else
	(void)fprintf (level <= LOG_ERR ? stderr : stdout,
	    "%s: %s", progname, buf);

    va_end (ap);
}
