/* @(#) $Revision: 32.3 $ */    
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"

/*
 * Input routines for command mode.
 * Since we translate the end of reads into the implied ^D's
 * we have different flavors of routines which do/don't return such.
 */
static	bool junkbs;
short	lastc = '\n';

#ifndef NONLS8 /* User messages */
# define	NL_SETN	6	/* set number */
# include	<msgbuf.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

ignchar()
{
	ignore(getchar());
}

getchar()
{
	register int c;

	do
		c = getcd();
	while (!globp && c == CTRL(d));
	return (c);
}

getcd()
{
	register int c;
	extern short slevel;

again:
	c = getach();
	if (c == EOF)
		return (c);

#ifndef	NLS16
	c &= TRIM;
#else
	c &= (TRIM | FIRST | SECOND);
#endif

	if (!inopen && slevel==0)
		if (!globp && c == CTRL(d))
			setlastchar('\n');
		else if (junk(c)) {
			checkjunk(c);
			goto again;
		}
	return (c);
}

peekchar()
{

	if (peekc == 0)
		peekc = getchar();
	return (peekc);
}

peekcd()
{
	if (peekc == 0)
		peekc = getcd();
	return (peekc);
}

int verbose;

#ifndef	NLS16
getach()
#else
getach()
{
	register int c;

	if (c = peekc)	{
		peekc = 0;
		return (c);
	}
	if (c = secondchar)	{
		secondchar = 0;
		return (c);
	}
	c = GETACH();
	if (c != EOF && FIRSTof2(c & TRIM))	{
		secondchar = GETACH();
		if (secondchar != EOF && SECONDof2(secondchar & TRIM))	{
			secondchar |= SECOND;
			return(c | FIRST);
		}
	} else	
		return(c);
}

GETACH()
#endif

{
	register int c;
	static char inline[128];
	struct stat statb;

#ifndef	NLS16
	c = peekc;
	if (c != 0) {

		peekc = 0;
		return (c);
	}
#endif

	if (globp) {
		if (*globp)

#ifndef NONLS8 /* 8bit integrity */
			return (*globp++ & TRIM);
#else NONLS8
			return (*globp++);
#endif NONLS8

		globp = 0;
		return (lastc = EOF);
	}
top:
	if (input) {
		if (c = *input++) {
			if (c &= TRIM)
				return (lastc = c);
			goto top;
		}
		input = 0;
	}
	flush();
	if (intty) {
		c = read(0, inline, sizeof inline - 4);
		if (c < 0)
			return (lastc = EOF);
		if (c == 0 || inline[c-1] != '\n')
			inline[c++] = CTRL(d);
		if (inline[c-1] == '\n')
			noteinp();
		inline[c] = 0;
		for (c--; c >= 0; c--)
			if (inline[c] == 0)
				inline[c] = QUOTE;
		input = inline;
		goto top;
	}

/*
**	#if defined (hp9000s200) || defined (hp9000s500)
**		if (read(0, &(((struct short_char *)&lastc)->low_byte), 1) != 1)
**	#else
*/
	if (read(0, inline, 1) != 1)
/*
**	#endif
*/

		lastc = EOF;
	else {

#ifndef	NLS16
		/* Avoid to be a negative value in lastc */
		lastc = inline[0];
#else
		lastc = inline[0] & TRIM;
#endif

		if (verbose)
			write(2, inline, 1);
	}
	return (lastc);
}

/*
 * Input routine for insert/append/change in command mode.
 * Most work here is in handling autoindent.
 */
static	short	lastin;

gettty()
{
	register int c = 0;
	register CHAR *cp = genbuf;
	char hadup = 0;
	int numbline();
	extern int (*Pline)();
	int offset = Pline == numbline ? 8 : 0;
	int ch;

#ifdef TEPE
	/* hp3000 tepe testing prompt */
	outchar('\021');
#endif
	if (intty && !inglobal) {
		if (offset) {
			holdcm = 1;
			printf("  %4d  ", lineDOT() + 1);
			flush();
			holdcm = 0;
		}
		if (value(AUTOINDENT) ^ aiflag) {
			holdcm = 1;
			if (value(LISP))
				lastin = lindent(dot + 1);
			gotab(lastin + offset);
			while ((c = getcd()) == CTRL(d)) {
				if (lastin == 0 && isatty(0) == -1) {
					holdcm = 0;
					return (EOF);
				}
				lastin = backtab(lastin);
				gotab(lastin + offset);
			}
			switch (c) {

			case '^':
			case '0':
				ch = getcd();
				if (ch == CTRL(d)) {
					if (c == '0')
						lastin = 0;
					if (!over_strike) {
						putchar('\b' | QUOTE);
						putchar(' ' | QUOTE);
						putchar('\b' | QUOTE);
					}
					gotab(offset);
					hadup = 1;
					c = getchar();
				} else
					ungetchar(ch);
				break;

			case '.':
				if (peekchar() == '\n') {
					ignchar();
					noteinp();
					holdcm = 0;
					return (EOF);
				}
				break;

			case '\n':
				hadup = 1;
				break;
			}
		}
		flush();
		holdcm = 0;
	}
	if (c == 0)
		c = getchar();
	while (c != EOF && c != '\n') {
		if (cp > &genbuf[LBSIZE - 2])
			error((nl_msg(1, "Input line too long")));
		*cp++ = c;
		c = getchar();
	}
	if (c == EOF) {
		if (inglobal)
			ungetchar(EOF);
		return (EOF);
	}
	*cp = 0;
	cp = linebuf;
	if ((value(AUTOINDENT) ^ aiflag) && hadup == 0 && intty && !inglobal) {
		lastin = c = smunch(lastin, genbuf);
		for (c = lastin; c >= value(TABSTOP); c -= value(TABSTOP))
			*cp++ = '\t';
		for (; c > 0; c--)
			*cp++ = ' ';
	}

#ifndef	NLS16
	CP(cp, genbuf);
#else
	STRCPY(cp, genbuf);
#endif

	if (linebuf[0] == '.' && linebuf[1] == 0)
		return (EOF);
	return (0);
}

/*
 * Crunch the indent.
 * Hard thing here is that in command mode some of the indent
 * is only implicit, so we must seed the column counter.
 * This should really be done differently so as to use the whitecnt routine
 * and also to hack indenting for LISP.
 */
smunch(col, ocp)
	register int col;
	CHAR *ocp;
{
	register CHAR *cp;

	cp = ocp;
	for (;;)
		switch (*cp++) {

		case ' ':
			col++;
			continue;

		case '\t':
			col += value(TABSTOP) - (col % value(TABSTOP));
			continue;

		default:
			cp--;

#ifndef	NLS16
			CP(ocp, cp);
#else
			STRCPY(ocp, cp);
#endif

			return (col);
		}
}

char	*cntrlhm =	"^H discarded\n";

checkjunk(c)
	char c;
{

	if (junkbs == 0 && c == '\b') {

#ifndef NONLS8 /* User messages */
		write(2, nl_msg(2, cntrlhm), strlen(nl_msg(2, cntrlhm)));
#else NONLS8
		write(2, cntrlhm, 13);
#endif NONLS8

		junkbs = 1;
	}
}

line *
setin(addr)
	line *addr;
{

	if (addr == zero)
		lastin = 0;
	else
		getline(*addr), lastin = smunch(0, linebuf);
}
