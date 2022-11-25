/* @(#) $Revision: 66.3 $ */     
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_re.h"

#ifdef ED1000
#include "ex_sm.h"
#endif ED1000

#ifndef NONLS8 /* User messages & Country customs */
# define	NL_SETN	8	/* set number */
# include	<msgbuf.h>
# include	<langinfo.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Global, substitute and regular expressions.
 * Very similar to ed, with some re extensions and
 * confirmed substitute.
 */
global(k)
	bool k;
{
	register char *gp;
	register int c;
	register line *a1;
	char globuf[GBSIZE];
	char *Cwas;
	int nlines = lineDOL();
	int oinglobal = inglobal;
	char *oglobp = globp;

	Cwas = Command;
	/*
	 * States of inglobal:
	 *  0: ordinary - not in a global command.
	 *  1: text coming from some buffer, not tty.
	 *  2: like 1, but the source of the buffer is a global command.
	 * Hence you're only in a global command if inglobal==2. This
	 * strange sounding convention is historically derived from
	 * everybody simulating a global command.
	 */
	if (inglobal==2)

#ifndef NONLS8 /* User messages */
		error((nl_msg(1, "Global within global|Global within global not allowed")));
#else NONLS8
		error("Global within global@not allowed");
#endif NONLS8

	markDOT();
	setall();
	nonzero();
	if (skipend())
		error((nl_msg(2, "Global needs re|Missing regular expression for global")));
	c = getchar();
	ignore(compile(c));
	savere(scanre);
	gp = globuf;
	while ((c = getchar()) != '\n') {
		switch (c) {

		case EOF:
			c = '\n';
			goto brkwh;

		case '\\':
			c = getchar();
			switch (c) {

#ifndef ED1000
			case EOF:
				*gp++ = '\\';
				c = '\n';
				goto brkwh;
#endif ED1000

			case '\\':
				ungetchar(c);
				break;

			case '\n':
				break;

			default:
				*gp++ = '\\';
				break;
			}
			break;
		}
		*gp++ = c;
		if (gp >= &globuf[GBSIZE - 2])
			error((nl_msg(3, "Global command too long")));
	}
brkwh:
	ungetchar(c);
out:
	donewline();
	*gp++ = c;
	*gp++ = 0;
	/*
	 * Visual mode needs the default to be an explicit print command.
	 */
	if (inopen && globuf[0] == '\n' && globuf[1] == '\0') {
		globuf[2] = globuf[1];
		globuf[1] = globuf[0];
		globuf[0] = 'p';
	}
	saveall();
	inglobal = 2;
	for (a1 = one; a1 <= dol; a1++) {
		*a1 &= ~01;
		if (a1 >= addr1 && a1 <= addr2 && execute(0, a1) == k)
			*a1 |= 01;
	}
#ifdef notdef
/*
 * This code is commented out for now.  The problem is that we don't
 * fix up the undo area the way we should.  Basically, I think what has
 * to be done is to copy the undo area down (since we shrunk everything)
 * and move the various pointers into it down too.  I will do this later
 * when I have time. (Mark, 10-20-80)
 */
	/*
	 * Special case: g/.../d (avoid n^2 algorithm)
	 */
	if (globuf[0]=='d' && globuf[1]=='\n' && globuf[2]=='\0') {
		gdelete();
		return;
	}
#endif
	if (inopen)
		inopen = -1;
	/*
	 * Now for each marked line, set dot there and do the commands.
	 * Note the n^2 behavior here for lots of lines matching.
	 * This is really needed: in some cases you could delete lines,
	 * causing a marked line to be moved before a1 and missed if
	 * we didn't restart at zero each time.
	 */
	for (a1 = one; a1 <= dol; a1++) {
		if (*a1 & 01) {
			*a1 &= ~01;
			dot = a1;
			globp = globuf;
			commands(1, 1);
			a1 = zero;
		}
	}

#ifdef ED1000
	if (!insm)
		globp = oglobp;
#else
	globp = oglobp;
#endif ED1000

	inglobal = oinglobal;
	endline = 1;
	Command = Cwas;
	netchHAD(nlines);
	setlastchar(EOF);

#ifdef ED1000
	if (insm)
		ungetchar(EOF);
#endif ED1000

	if (inopen) {
		ungetchar(EOF);
		inopen = 1;
	}
}

/*
 * gdelete: delete inside a global command. Handles the
 * special case g/r.e./d. All lines to be deleted have
 * already been marked. Squeeze the remaining lines together.
 * Note that other cases such as g/r.e./p, g/r.e./s/r.e.2/rhs/,
 * and g/r.e./.,/r.e.2/d are not treated specially.  There is no
 * good reason for this except the question: where to you draw the line?
 */
gdelete()
{
	register line *a1, *a2, *a3;

	a3 = dol;
	/* find first marked line. can skip all before it */
	for (a1=zero; (*a1&01)==0; a1++)
		if (a1>=a3)
			return;
	/* copy down unmarked lines, compacting as we go. */
	for (a2=a1+1; a2<=a3;) {
		if (*a2&01) {
			a2++;		/* line is marked, skip it */
			dot = a1;	/* dot left after line deletion */
		} else
			*a1++ = *a2++;	/* unmarked, copy it */
	}
	dol = a1-1;
	if (dot>dol)
		dot = dol;
	change();
}

bool	cflag;
int	scount, slines, stotal;

substitute(c)
	int c;
{
	register line *addr;
	register int n;
	int gsubf, hopcount;

	gsubf = compsub(c);
	if(FIXUNDO)
		save12(), undkind = UNDCHANGE;
	stotal = 0;
	slines = 0;
	for (addr = addr1; addr <= addr2; addr++) {
		scount = hopcount = 0;
		if (dosubcon(0, addr) == 0)
			continue;
		if (gsubf) {
			/*
			 * The loop can happen from s/\</&/g
			 * but we don't want to break other, reasonable cases.
			 */
			hopcount = 0;
			while (*loc2) {

#ifndef	NLS16
				if (++hopcount > sizeof linebuf)
#else
				if (++hopcount > sizeof(linebuf)/sizeof(CHAR))
#endif

					error((nl_msg(4, "substitution loop")));
				if (dosubcon(1, addr) == 0)
					break;
			}
		}
		if (scount) {
			stotal += scount;
			slines++;
			putmark(addr);
			n = append(getsub, addr);
			addr += n;
			addr2 += n;
		}
	}
	if (stotal == 0 && !inglobal && !cflag)
		error((nl_msg(5, "Fail|Substitute pattern match failed")));
	snote(stotal, slines);
	return (stotal);
}

compsub(ch)
{
	register int seof, c, uselastre;
	static int gsubf;

	if (!value(EDCOMPATIBLE))
		gsubf = cflag = 0;
	uselastre = 0;
	switch (ch) {

	case 's':
		ignore(skipwh());
		seof = getchar();
		if (endcmd(seof) || any(seof, "gcr")) {
			ungetchar(seof);
			goto redo;
		}

#ifndef NONLS8 /* Character set features */
		if ((seof >= IS_MACRO_LOW_BOUND) && (isalpha(seof & TRIM) || isdigit(seof & TRIM)))
#else NONLS8
		if ((seof >= IS_MACRO_LOW_BOUND) && (isalpha(seof) || isdigit(seof)))
#endif NONLS8

			error((nl_msg(6, "Substitute needs re|Missing regular expression for substitute")));
		seof = compile(seof);
		uselastre = 1;
		comprhs(seof);
		gsubf = 0;
		cflag = 0;
		break;

	case '~':
		uselastre = 1;
		/* fall into ... */
	case '&':
	redo:
		if (re.Expbuf[0] == 0)
			error((nl_msg(7, "No previous re|No previous regular expression")));
		if (subre.Expbuf[0] == 0)
			error((nl_msg(8, "No previous substitute re|No previous substitute to repeat")));
		break;
	}
	for (;;) {
		c = getchar();
		switch (c) {

		case 'g':
			gsubf = !gsubf;
			continue;

		case 'c':
			cflag = !cflag;
			continue;

		case 'r':
			uselastre = 1;
			continue;

		default:
			ungetchar(c);
			setcount();
			donewline();
			if (uselastre)
				savere(subre);
			else
				resre(subre);
			return (gsubf);
		}
	}
}

comprhs(seof)
	int seof;
{

#ifndef NONLS8 /* 8bit integrity */
	register short *rp, *orp;
	register int c;
	short orhsbuf[RHSSIZE];
#else NONLS8
	register char *rp, *orp;
	register int c;
	char orhsbuf[RHSSIZE];
#endif NONLS8

	rp = rhsbuf;

#ifndef NONLS8 /* 8bit integrity */
#ifndef	NLS16
	orp = orhsbuf;
	while(*orp++ = *rp++);
	rp = rhsbuf;
#else
	STRCPY(orhsbuf, rp);
#endif
#else NONLS8
	CP(orhsbuf, rp);
#endif NONLS8

	for (;;) {
		c = getchar();
		if (c == seof)
			break;
		switch (c) {

		case '\\':
			c = getchar();
			if (c == EOF) {
				ungetchar(c);
				break;
			}
			if (value(MAGIC)) {
				/*
				 * When "magic", \& turns into a plain &,
				 * and all other chars work fine quoted.
				 */
				if (c != '&')
					c |= QUOTE;
				break;
			}
magic:
			if (c == '~') {
				for (orp = orhsbuf; *orp; *rp++ = *orp++)
					if (rp >= &rhsbuf[RHSSIZE - 1])
						goto toobig;
				continue;
			}
			c |= QUOTE;
			break;

		case '\n':
		case EOF:
			if (!(globp && globp[0])) {
				ungetchar(c);
				goto endrhs;
			}

		case '~':
		case '&':
			if (value(MAGIC))
				goto magic;
			break;
		}
		if (rp >= &rhsbuf[RHSSIZE - 1]) {
toobig:
			*rp = 0;

#ifndef NONLS8 /* User messages */
			error((nl_msg(9, "Replacement pattern too long|Replacement pattern too long - limit 256 characters")));
#else NONLS8
			error("Replacement pattern too long@- limit 256 characters");
#endif NONLS8

		}
		*rp++ = c;
	}
endrhs:
	*rp++ = 0;
}

getsub()
{
	register char *p;

	if ((p = linebp) == 0)
		return (EOF);

#ifndef NLS16
	strcLIN(p);
#else
	/* We have to change the contents of linebuf rather than LINEBUF. */
	char_TO_CHAR(p, linebuf);
#endif

	linebp = 0;
	return (0);
}

dosubcon(f, a)
	bool f;
	line *a;
{

	if (execute(f, a) == 0)
		return (0);
	if (confirmed(a)) {
		dosub();
		scount++;
	}
	return (1);
}

confirmed(a)
	line *a;
{
	register int c, ch;

#ifndef NONLS8 /* Country customs */
	char	*nl_langinfo();
#endif NONLS8

	if (cflag == 0)
		return (1);
	pofix();
	pline(lineno(a));
	if (inopen)
		putchar('\n' | QUOTE);
	c = column(loc1 - 1);
	ugo(c - 1 + (inopen ? 1 : 0), ' ');
	ugo(column(loc2 - 1) - c, '^');
	flush();
	ch = c = getkey();
again:
	if (c == '\r')
		c = '\n';
	if (inopen)
		putchar(c), flush();
	if (c != '\n' && c != EOF) {
		c = getkey();
		goto again;
	}
	noteinp();

#ifndef NONLS8 /* Country customs */
	return (ch == (nl_langinfo(YESSTR)[0] & 0377));
#else NONLS8
	return (ch == 'y');
#endif NONLS8

}

/* this function isn't used by anyone
getch()
{
	char c;

	if (read(2, &c, 1) != 1)
		return (EOF);
	return (c & TRIM);
}
*/

ugo(cnt, with)
	int with;
	int cnt;
{

	if (cnt > 0)
		do
			putchar(with);
		while (--cnt > 0);
}

int	casecnt;
bool	destuc;

dosub()
{

#ifndef NONLS8 /* 8bit integrity */
	register CHAR *lp, *sp;
	register short *rp;
#else NONLS8
	register char *lp, *sp, *rp;
#endif NONLS8

	int c;

	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < loc1)
		*sp++ = *lp++;
	casecnt = 0;
	/*
	 * Caution: depending on the hardware, c will be either sign
	 * extended or not if C&QUOTE is set.  Thus, on a VAX, c will
	 * be < 0, but on a 3B, c will be >= 128.
	 */
	while (c = *rp++) {
		/* ^V <return> from vi to split lines */
		if (c == '\r')
			c = '\n';

		if (c & QUOTE)

#ifndef	NLS16
			switch (c & TRIM) {
#else
			switch (c & (TRIM | FIRST | SECOND)) {
#endif

			case '&':
				sp = place(sp, loc1, loc2);
				if (sp == 0)
					goto ovflo;
				continue;

			case 'l':
				casecnt = 1;
				destuc = 0;
				continue;

			case 'L':
				casecnt = LBSIZE;
				destuc = 0;
				continue;

			case 'u':
				casecnt = 1;
				destuc = 1;
				continue;

			case 'U':
				casecnt = LBSIZE;
				destuc = 1;
				continue;

			case 'E':
			case 'e':
				casecnt = 0;
				continue;
			}

#ifndef	NLS16
		if ((c & QUOTE) && (c &= TRIM) >= '1' && c < nbra + '1') {
#else
		if ((c & QUOTE) && (c &= (TRIM | FIRST | SECOND)) >= '1' && c < nbra + '1') {
#endif

			sp = place(sp, braslist[c - '1'], braelist[c - '1']);
			if (sp == 0)
				goto ovflo;
			continue;
		}
		if (casecnt)

#ifndef	NLS16
			*sp++ = fixcase(c & TRIM);
		else
			*sp++ = c & TRIM;
#else
			*sp++ = fixcase(c & (TRIM | FIRST | SECOND));
		else
			*sp++ = c & (TRIM | FIRST | SECOND);
#endif

		if (sp >= &genbuf[LBSIZE])

ovflo:

#ifndef NONLS8 /* User messages */
			error((nl_msg(10, "Line overflow|Line overflow in substitute")));
#else NONLS8
			error("Line overflow@in substitute");
#endif NONLS8

	}
	lp = loc2;

#ifdef ED1000
	loc2 = sp + (linebuf - genbuf);
#else
	loc2 = (sp - genbuf) + linebuf;
#endif ED1000

	while (*sp++ = *lp++)
		if (sp >= &genbuf[LBSIZE])
			goto ovflo;

#ifndef	NLS16
	strcLIN(genbuf);
#else
	STRCLIN(genbuf);
#endif

}

fixcase(c)
	register int c;
{

	if (casecnt == 0)
		return (c);

#ifndef	NLS16
	casecnt--;
#else
	/* decrement casecnt once every character 
	** and no conversion for 16-bit character
	*/
	if (IS_KANJI(c)) {
		if (IS_FIRST(c))
			casecnt--;
		return (c);
	} else
		casecnt--;
#endif

	if (destuc) {

#ifndef NONLS8 /* Character set features */
		if ((c >= IS_MACRO_LOW_BOUND) && islower(c & TRIM))
			c = toupper(c & TRIM);
#else NONLS8
		if ((c >= IS_MACRO_LOW_BOUND) && islower(c))
			c = toupper(c);
#endif NONLS8

	} else

#ifndef NONLS8 /* Character set features */
		if ((c >= IS_MACRO_LOW_BOUND) && isupper(c & TRIM))
			c = tolower(c & TRIM);
#else NONLS8
		if ((c >= IS_MACRO_LOW_BOUND) && isupper(c))
			c = tolower(c);
#endif NONLS8

	return (c);
}

CHAR *
place(sp, l1, l2)
	register CHAR *sp, *l1, *l2;
{

	while (l1 < l2) {
		*sp++ = fixcase(*l1++);
		if (sp >= &genbuf[LBSIZE])
			return (0);
	}
	return (sp);
}

snote(total, nlines)
	register int total, nlines;
{

	if (!notable(total))
		return;
#if defined NLS || defined NLS16
	RL_OKEY
#endif
	printf(mesg((nl_msg(11, "%d subs|%d substitutions"))), total);
	if (nlines != 1 && nlines != total)
		printf((nl_msg(12, " on %d lines")), nlines);
#if defined NLS || defined NLS16
	RL_OSCREEN
#endif
	noonl();
	flush();
}

compile(eof)
	int eof;
{
	register int		c;
	short			re_buf[RE_BUF_SIZE+2];		/* place to assemble incoming reg exp */
	register short		*p_re_buf = re_buf;

	if ((eof >= IS_MACRO_LOW_BOUND) && (isalpha(eof & TRIM) || isdigit(eof & TRIM)))
		error((nl_msg(13, "Regular expressions cannot be delimited by letters or digits")));

	c = getchar();
	if (eof == '\\')
		switch (c) {

		case '/':
		case '?':
			if (scanre.Expbuf[0] == 0)
error((nl_msg(14, "No previous scan re|No previous scanning regular expression")));
			resre(scanre);
			return (c);

		case '&':
			if (subre.Expbuf[0] == 0)
error((nl_msg(15, "No previous substitute re|No previous substitute regular expression")));
			resre(subre);
			return (c);

		default:
			error((nl_msg(16, "Badly formed re|Regular expression \\ must be followed by / or ?")));
		}

	if (c == eof || c == '\n' || c == EOF) {
		if (expbuf[0] == 0)
			error((nl_msg(17, "No previous re|No previous regular expression")));
		if (c != eof)
			ungetchar(c);
		return (eof);
	}

	circf = nbra = 0;

	if (c == '^')						/* anchor at the beginning of line? */
		circf++;
	else
		ungetchar(c);

	/* copy RE input to RE buffer */
	while ( ((c = getchar()) != eof) && (c != '\n') && (c != EOF) ) {
		*p_re_buf++ = c;
		if (c == '\\') {
			c = getchar();
			if (c == '\n')
cerror((nl_msg(29, "No newlines in re's|Can't escape newlines into regular expressions")));
			if (c == EOF)
cerror((nl_msg(30, "Trailing \\|Can't have a trailing \\ in a regular expression")));
			*p_re_buf++ = c;
		}
#ifdef	NLS16	/* 2nd byte of 16-bit char */
		if (IS_FIRST(c))
			*p_re_buf++ = getchar();
#endif
	}
	*p_re_buf = 0;
	
	/* at the end of the RE */

	if(c == '\n' || c == EOF)
		ungetchar(c);

	switch (regcomp(re_buf)) {

		case 0:						/* RE was successfully compiled */
			return (eof);

		case REG_ESPACE:
cerror((nl_msg(19, "Re too complex|Regular expression too complicated")));
		case REG_EPAREN:
cerror((nl_msg(20, "Unmatched \\( or extra \\)|Unequal numbers of \\('s and \\)'s in regular expression")));
		case REG_ENSUB:
cerror((nl_msg(21, "Awash in \\('s!|Too many \\('d subexressions in a regular expression")));
		case REG_ECOLLATE:
cerror((nl_msg(22, "Bad collation element|A non-collating element specified where one is required")));
		case REG_ERANGE:
cerror((nl_msg(25, "Bad range endpoint|First endpoint > second or ctype class used as endpoint")));
		case REG_ECTYPE:
cerror((nl_msg(26, "Bad character class|Invalid ctype character class '[: :]' named")));
		case REG_EBRACK:
cerror((nl_msg(27, "Bad match list|Empty or incomplete match list: '[]' or '[^]'")));
		case REG_EESCAPE:
cerror((nl_msg(30, "Trailing \\|Can't have a trailing \\ in a regular expression")));
		default:
cerror((nl_msg(32, "RE syntax error|Invalid regular expression syntax used")));

/* Not supported by ex/vi at this time:
/*
/*		case REG_ESUBREG:
/* cerror("Bad \\n|\\n in regular expression with n greater than the number of \\('s");
/*		case REG_EBRACE:	/* \{ \} imbalance						*/
/*		case REG_EABRACE:	/* number too large in \{ \} construct				*/
/*		case REG_EBBRACE:	/* invalid number in \{ \} construct				*/
/*		case REG_ECBRACE:	/* more than 2 numbers in \{ \} construct			*/
/*		case REG_EDBRACE:	/* first number exceeds second in \{ \} construct		*/

	}
}


cerror(s)
	char *s;
{

	expbuf[0] = 0;
	error(s);
}



execute(gf, addr)
	line *addr;
{
	register CHAR *p1;
	register CHAR *locs;

	if (gf) {
		if (circf)
			return (0);
		locs = p1 = loc2;
	} else {
		if (addr == zero)
			return (0);
		p1 = linebuf;
		getline(*addr);
	}

	do {
		if (regexec(p1))
			return 0;

	} while (gf && loc2 <= locs && CHARADV(p1));		/* find a match past locs if g-flag set */

	return 1;
}
