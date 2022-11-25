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
static char rcsid[] = "$Header: macro.c,v 1.9.109.4 95/02/21 16:08:01 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)macro.c	5.7 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */

# include "sendmail.h"

/*
**  EXPAND -- macro expand a string using $x escapes.
**
**	Parameters:
**		s -- the string to expand.
**		buf -- the place to put the expansion.
**		buflim -- the buffer limit, i.e., the address
**			of the last usable position in buf.
**		e -- envelope in which to work.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

void expand(s, buf, buflim, e)
	register char *s;
	register char *buf;
	char *buflim;
	register ENVELOPE *e;
{
	register char *xp;
	register char *q;
	bool skipping;		/* set if conditionally skipping output */
	bool recurse = FALSE;	/* set if recursion required */
	int i;
	char xbuf[BUFSIZ];
	bool gotone = FALSE;		/* set if any expansion done */
	static int depth = 0;		/* current depth of recursion */
	int rlimit = sizeof xbuf;	/* limit on depth of recursion */

	depth++;

# ifdef DEBUG
	if (tTd(35, 24))
	{
		printf("expand(");
		xputs(s);
		printf("), depth = %d\n", depth);
	}
#  endif /* DEBUG */

	skipping = FALSE;
	if (s == NULL)
		s = "";
	for (xp = xbuf; *s != '\0'; s++)
	{
		char c;

		/*
		**  Check for non-ordinary (special?) character.
		**	'q' will be the interpolated quantity.
		*/

		q = NULL;
		c = *s;
		switch (c)
		{
		  case CONDIF:		/* see if var set */
			c = *++s;
			skipping = macvalue(c, e) == NULL;
			continue;

		  case CONDELSE:	/* change state of skipping */
			skipping = !skipping;
			continue;

		  case CONDFI:		/* stop skipping */
			skipping = FALSE;
			continue;

		  case '\001':		/* macro interpolation */
			c = *++s;
			q = macvalue(c & 0177, e);
			if (q == NULL)
				continue;
			gotone = TRUE;
			break;
		}

		/*
		**  Interpolate q or output one character
		*/

		if (skipping || xp >= &xbuf[sizeof xbuf])
			continue;
		if (q == NULL)
			*xp++ = c;
		else
		{
			/* copy to end of q or max space remaining in buf */
			while ((c = *q++) != '\0' && xp < &xbuf[sizeof xbuf - 1])
			{
				if (iscntrl(c) && !isspace(c))
					recurse = TRUE;
				*xp++ = c;
			}
		}
	}
	*xp = '\0';

# ifdef DEBUG
	if (tTd(35, 24))
	{
		printf("expand ==> ");
		xputs(xbuf);
		printf("\n");
	}
#  endif /* DEBUG */

	/* recurse as appropriate */
	if (gotone)
	{
		if (depth + 1 > rlimit)
		{
			errno = 0;
			syserr("Recursive macro definition");
			bzero(buf, buflim - buf);
			depth--;
			return;
		}
		else if (recurse)
		{
			expand(xbuf, buf, buflim, e);
			depth--;
			return;
		}
	}

	/* copy results out */
	i = buflim - buf - 1;
	if (i > xp - xbuf)
		i = xp - xbuf;
	bcopy(xbuf, buf, i);
	buf[i] = '\0';
	depth--;
}
/*
**  DEFINE -- define a macro.
**
**	this would be better done using a #define macro.
**
**	Parameters:
**		n -- the macro name.
**		v -- the macro value.
**		e -- the envelope to store the definition in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		e->e_macro[n] is defined.
**
**	Notes:
**		There is one macro for each ASCII character,
**		although they are not all used.  The currently
**		defined macros are:
**
**		$a   date in ARPANET format (preferring the Date: line
**		     of the message)
**		$b   the current date (as opposed to the date as found
**		     the message) in ARPANET format
**		$c   hop count
**		$d   (current) date in UNIX (ctime) format
**		$e   the SMTP entry message+
**		$f   raw from address
**		$g   translated from address
**		$h   to host
**		$i   queue id
**		$j   official SMTP hostname, used in messages+
**		$l   UNIX-style from line+
**		$n   name of sendmail ("MAILER-DAEMON" on local
**		     net typically)+
**		$o   delimiters ("operators") for address tokens+
**		$p   my process id in decimal
**		$q   the string that becomes an address -- this is
**		     normally used to combine $g & $x.
**		$r   protocol used to talk to sender
**		$s   sender's host name
**		$t   the current time in seconds since 1/1/1970
**		$u   to user
**		$v   version number of sendmail
**		$w   our host name (if it can be determined)
**		$x   signature (full name) of from person
**		$y   the tty id of our terminal
**		$z   home directory of to person
**
**		Macros marked with + must be defined in the
**		configuration file and are used internally, but
**		are not set.
**
**		There are also some macros that can be used
**		arbitrarily to make the configuration file
**		cleaner.  In general all upper-case letters
**		are available.
*/

void define(n, v, e)
	char n;
	char *v;
	register ENVELOPE *e;
{
# ifdef DEBUG
	if (tTd(35, 9))
	{
		printf("define(%c as ", n);
		xputs(v);
		printf(")\n");
	}
#  endif /* DEBUG */
	e->e_macro[n & 0177] = v;
}
/*
**  MACVALUE -- return uninterpreted value of a macro.
**
**	Parameters:
**		n -- the name of the macro.
**
**	Returns:
**		The value of n.
**
**	Side Effects:
**		none.
*/

char *
macvalue(n, e)
	char n;
	register ENVELOPE *e;
{
	n &= 0177;
	while (e != NULL)
	{
		register char *p = e->e_macro[n];

		if (p != NULL)
			return (p);
		e = e->e_parent;
	}
	return (NULL);
}
