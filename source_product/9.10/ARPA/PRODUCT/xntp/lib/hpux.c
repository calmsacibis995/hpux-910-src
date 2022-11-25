# ifdef HPUX
/* $Header: hpux.c,v 1.2.109.2 94/10/28 17:31:46 mike Exp $
 * hpux.c -- compatibility routines for HPUX.
 * XXX many of these are not needed anymore.
 */
# 	include <string.h>
# 	include <memory.h>
# 	ifdef	HAVE_UNISTD_H
# 		include <unistd.h>
# 	endif	/* HAVE_UNISTD_H */
# 	include <stdio.h>
# 	include <sys/stat.h>
# 	include <sys/types.h>
# 	include <sys/stat.h>
# 	include <sys/utsname.h>


# 	if (HPUX < 8)
char
*index (s, c)
register char  *s;
register int    c;
{
    return strchr (s, c);
}


char
*rindex (s, c)
register char  *s;
register int    c;
{
    return strrchr (s, c);
}


int
bcmp (a, b, count)
register char  *a,
               *b;
register int    count;
{
    return memcmp (a, b, count);
}


void
bcopy (from, to, count)
register char  *from;
register char  *to;
register int    count;
{
    if ((to == from) || (count <= 0))
	return;

    if ((to > from) && (to <= (from + count)))
        {
	to += count;
	from += count;

	do
	    {
	    *--to = *--from;
	    } while (--count);
        }
    else
        {
	do
	    {
	    *to++ = *from++;
	    } while (--count);
        }
}


void
bzero (area, count)
register char  *area;
register int    count;
{
    memset (area, 0, count);
}
# 	endif	/* (HPUX < 8) */


getdtablesize ()
{
    return (sysconf (_SC_OPEN_MAX));
}


int
setlinebuf (a_stream)
FILE   *a_stream;
{
    return setvbuf (a_stream, (char    *)NULL, _IOLBF, 0);
}


char   *
FindConfig (base)
char   *base;
{
    static char     result[BUFSIZ];
    char    hostname[BUFSIZ],
           *cp;
    struct stat     sbuf;
    struct utsname  unamebuf;

    /* All keyed by initial target being a directory */
    (void)strcpy (result, base);
    if (stat (result, &sbuf) == 0)
        {
	if (S_ISDIR (sbuf.st_mode))
	    {

	    /* First choice is my hostname */
	    if (gethostname (hostname, BUFSIZ) >= 0)
	        {
		(void)sprintf (result, "%s/%s", base, hostname);
		if (stat (result, &sbuf) == 0)
		    {
		    goto outahere;
		    }
		else
		    {

		    /* Second choice is of form default.835 
		    */
		    (void)uname (&unamebuf);
		    if (strncmp (unamebuf.machine, "9000/", 5) == 0)
			cp = unamebuf.machine + 5;
		    else
			cp = unamebuf.machine;
		    (void)sprintf (result, "%s/default.%s", base, cp);
		    if (stat (result, &sbuf) == 0)
		        {
			goto outahere;
		        }
		    else
		        {

			/* Last choice is just default */
			(void)sprintf (result, "%s/default", base);
			if (stat (result, &sbuf) == 0)
			    {
			    goto outahere;
			    }
			else
			    {
			    (void)strcpy (result, "/not/found");
			    }
		        }
		    }
	        }
	    }
        }
outahere:
    return (result);
}
# endif	/* HPUX */
