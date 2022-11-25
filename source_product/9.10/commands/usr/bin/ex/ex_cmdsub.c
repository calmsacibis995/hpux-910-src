/* @(#) $Revision: 70.3.1.1 $ */     
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifdef ED1000
#include "ex_sm.h"
#endif ED1000

/*
 * Command mode subroutines implementing
 *	append, args, copy, delete, join, move, put,
 *	shift, tag, yank, z and undo
 */

bool	endline = 1;
line	*tad1;
static	jnoop();

#ifndef NONLS8 /* User messages */
# define	NL_SETN	5	/* set number */
# include	<msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Append after line a lines returned by function f.
 * Be careful about intermediate states to avoid scramble
 * if an interrupt comes in.
 */
append(f, a)
	int (*f)();
	line *a;
{
	register line *a1, *a2, *rdot;
	int nline;

	nline = 0;
	dot = a;
	if(FIXUNDO && !inopen && f!=getsub) {
		undap1 = undap2 = dot + 1;
		undkind = UNDCHANGE;
	}
	while ((*f)() == 0) {
		if (truedol >= endcore) {
			if (morelines() < 0) {
				if (FIXUNDO && f == getsub) {
					undap1 = addr1;
					undap2 = addr2 + 1;
				}

#ifndef NONLS8 /* User messages */
				error((nl_msg(1, "Out of memory|Out of memory - too many lines in file")));
#else NONLS8
				error("Out of memory@- too many lines in file");
#endif NONLS8

			}
		}
		nline++;
		a1 = truedol + 1;
		a2 = a1 + 1;
		dot++;
		undap2++;
		dol++;
		unddol++;
		truedol++;
		for (rdot = dot; a1 > rdot;)
			*--a2 = *--a1;
		*rdot = 0;
		putmark(rdot);
#ifdef ED1000
		if (f == gettty || f == sm_get)
#else
		if (f == gettty)
#endif ED1000
		{
			dirtcnt++;
			TSYNC();
		}
	}
	return (nline);
}

appendnone()
{

	if(FIXUNDO) {
		undkind = UNDCHANGE;
		undap1 = undap2 = addr1;
	}
}

/*
 * Print out the argument list, with []'s around the current name.
 */
pargs()
{
	register char **av = argv0, *as = args0;
	register int ac;

	for (ac = 0; ac < argc0; ac++) {
		if (ac != 0)
			putchar(' ');
		if (ac + argc == argc0 - 1)
			printf("[");
		lprintf("%s", as);
		if (ac + argc == argc0 - 1)
			printf("]");
		as = av ? *++av : strend(as) + 1;
	}
	noonl();
}

/*
 * Delete lines; two cases are if we are really deleting,
 * more commonly we are just moving lines to the undo save area.
 */
delete(hush)
	bool hush;
{
	register line *a1, *a2;

	nonzero();
	if(FIXUNDO) {
		register void (*dsavint)();

#ifdef UNDOTRACE
		if (trace)
			vudump("before delete");
#endif
		change();
		dsavint = signal(SIGINT, SIG_IGN);
		undkind = UNDCHANGE;
		a1 = addr1;
		squish();
		a2 = addr2;
		if (a2++ != dol) {
			reverse(a1, a2);
			reverse(a2, dol + 1);
			reverse(a1, dol + 1);
		}
		dol -= a2 - a1;
		unddel = a1 - 1;
		if (a1 > dol)
			a1 = dol;
		dot = a1;
		pkill[0] = pkill[1] = 0;
		signal(SIGINT, dsavint);
#ifdef UNDOTRACE
		if (trace)
			vudump("after delete");
#endif
	} else {
		register line *a3;
		register int i;

		change();
		a1 = addr1;
		a2 = addr2 + 1;
		a3 = truedol;
		i = a2 - a1;
		unddol -= i;
		undap2 -= i;
		dol -= i;
		truedol -= i;
		do
			*a1++ = *a2++;
		while (a2 <= a3);
		a1 = addr1;
		if (a1 > dol)
			a1 = dol;
		dot = a1;
	}
	if (!hush)
		killed();
}

deletenone()
{

	if(FIXUNDO) {
		undkind = UNDCHANGE;
		squish();
		unddel = addr1;
	}
}

/*
 * Crush out the undo save area, moving the open/visual
 * save area down in its place.
 */
squish()
{
	register line *a1 = dol + 1, *a2 = unddol + 1, *a3 = truedol + 1;

	if(FIXUNDO) {
		if (inopen == -1)
			return;
		if (a1 < a2 && a2 < a3)
			do
				*a1++ = *a2++;
			while (a2 < a3);
		truedol -= unddol - dol;
		unddol = dol;
	}
}

/*
 * Join lines.  Special hacks put in spaces, two spaces if
 * preceding line ends with '.', or no spaces if next line starts with ).
 */
static	int jcount, jnoop();

join(c)
	int c;
{
	register line *a1;
	register CHAR *cp, *cp1;

	cp = genbuf;
	*cp = 0;
	for (a1 = addr1; a1 <= addr2; a1++) {
		getline(*a1);
		cp1 = linebuf;
		if (a1 != addr1 && c == 0) {
			while (*cp1 == ' ' || *cp1 == '\t')
				cp1++;

			if (*cp1 && cp > genbuf && cp[-1] != ' ' && cp[-1] != '\t') {

#ifndef	NLS16
				if (*cp1 != ')') {
#else
			/*  no space is padded if both of the last of former line
			**  and the first of latter line are 16-bit characters.
			*/
				if (*cp1 != ')' && !(IS_FIRST(*cp1) && IS_SECOND(cp[-1]))) {
#endif

					*cp++ = ' ';
					if (cp[-2] == '.')
						*cp++ = ' ';
				}
			}
		}
		while (*cp++ = *cp1++)
			if (cp > &genbuf[LBSIZE-2])
				error((nl_msg(2, "Line overflow|Result line of join would be too long")));
		cp--;
	}

#ifndef	NLS16
	strcLIN(genbuf);
#else
	STRCLIN(genbuf);
#endif

	delete(0);
	jcount = 1;
	if (FIXUNDO)
		undap1 = undap2 = addr1;
	ignore(append(jnoop, --addr1));
	if (FIXUNDO)
		vundkind = VMANY;
}

static
jnoop()
{

	return(--jcount);
}

/*
 * Move and copy lines.  Hard work is done by move1 which
 * is also called by undo.
 */
int	getcopy();

move()
{
	register line *adt;
	bool iscopy = 0;

	if (Command[0] == 'm') {
		setdot1();
		markpr(addr2 == dot ? addr1 - 1 : addr2 + 1);
	} else {
		iscopy++;
		setdot();
	}
	nonzero();
	adt = address((CHAR *)0);
	if (adt == 0)
		serror((nl_msg(3, "%s where?|%s requires a trailing address")), Command);
	donewline();
	move1(iscopy, adt);
	killed();
}

move1(cflag, addrt)
	int cflag;
	line *addrt;
{
	register line *adt, *ad1, *ad2;
	int nlines;

	adt = addrt;
	nlines = (addr2 - addr1) + 1;
	if (cflag) {
		tad1 = addr1;
		ad1 = dol;
		ignore(append(getcopy, ad1++));
		ad2 = dol;
	} else {
		ad2 = addr2;
		for (ad1 = addr1; ad1 <= ad2;)
			*ad1++ &= ~01;
		ad1 = addr1;
	}
	ad2++;
	if (adt < ad1) {
		if (adt + 1 == ad1 && !cflag && !inglobal)
			error((nl_msg(4, "That move would do nothing!")));
		dot = adt + (ad2 - ad1);
		if (++adt != ad1) {
			reverse(adt, ad1);
			reverse(ad1, ad2);
			reverse(adt, ad2);
		}
	} else if (adt >= ad2) {
		dot = adt++;
		reverse(ad1, ad2);
		reverse(ad2, adt);
		reverse(ad1, adt);
	} else
		error((nl_msg(5, "Move to a moved line")));
	change();
	if (!inglobal)
		if(FIXUNDO) {
			if (cflag) {
				undap1 = addrt + 1;
				undap2 = undap1 + nlines;
				deletenone();
			} else {
				undkind = UNDMOVE;
				undap1 = addr1;
				undap2 = addr2;
				unddel = addrt;
				squish();
			}
		}
}

getcopy()
{

	if (tad1 > addr2)
		return (EOF);
	getline(*tad1++);
	return (0);
}

/*
 * Put lines in the buffer from the undo save area.
 */
getput()
{

	if (tad1 > unddol)
		return (EOF);
	getline(*tad1++);
	tad1++;
	return (0);
}

put()
{
	register int cnt;

	if (!FIXUNDO)
		error((nl_msg(6, "Cannot put inside global/macro")));
	cnt = unddol - dol;
	if (cnt && inopen && pkill[0] && pkill[1]) {
		pragged(1);
		return;
	}
	tad1 = dol + 1;
	ignore(append(getput, addr2));
	undkind = UNDPUT;
	notecnt = cnt;
	netchange(cnt);
}

/*
 * A tricky put, of a group of lines in the middle
 * of an existing line.  Only from open/visual.
 * Argument says pkills have meaning, e.g. called from
 * put; it is 0 on calls from putreg.
 */
pragged(kill)
	bool kill;
{
	extern CHAR *cursor;
	register CHAR *gp = &genbuf[cursor - linebuf];

	/*
	 * This kind of stuff is TECO's forte.
	 * We just grunge along, since it cuts
	 * across our line-oriented model of the world
	 * almost scrambling our addled brain.
	 */
	if (!kill)
		getDOT();

#ifndef	NLS16
	strcpy(genbuf, linebuf);
#else
	STRCPY(genbuf, linebuf);
#endif

	getline(*unddol);
	if (kill)
		*pkill[1] = 0;

#ifndef	NLS16
	strcat(linebuf, gp);
#else
	STRCAT(linebuf, gp);
#endif

	putmark(unddol);
	getline(dol[1]);
	if (kill)

#ifndef	NLS16
		strcLIN(pkill[0]);
	strcpy(gp, linebuf);
	strcLIN(genbuf);
#else
		STRCLIN(pkill[0]);
	STRCPY(gp, linebuf);
	STRCLIN(genbuf);
#endif

	putmark(dol+1);
	undkind = UNDCHANGE;
	undap1 = dot;
	undap2 = dot + 1;
	unddel = dot - 1;
	undo(1);
}

/*
 * Shift lines, based on c.
 * If c is neither < nor >, then this is a lisp aligning =.
 */
shift(c, cnt)
	int c;
	int cnt;
{
	register line *addr;
	register CHAR *cp;
	CHAR *dp;
	register int i;

	if(FIXUNDO)
		save12(), undkind = UNDCHANGE;
	cnt *= value(SHIFTWIDTH);
	for (addr = addr1; addr <= addr2; addr++) {
		dot = addr;
		if (c == '=' && addr == addr1 && addr != addr2)
			continue;
		getDOT();
		i = whitecnt(linebuf);
		switch (c) {

		case '>':
			if (linebuf[0] == 0)
				continue;
			cp = genindent(i + cnt);
			break;

		case '<':
			if (i == 0)
				continue;
			i -= cnt;
			cp = i > 0 ? genindent(i) : genbuf;
			break;

		default:
			i = lindent(addr);
			getDOT();
			cp = genindent(i);
			break;
		}

#ifndef	NLS16
		if (cp + strlen(dp = vpastwh(linebuf)) >= &genbuf[LBSIZE - 2])
#else
		if (cp + STRLEN(dp = vpastwh(linebuf)) >= &genbuf[LBSIZE - 2])
#endif

			error((nl_msg(7, "Line too long|Result line after shift would be too long")));

#ifndef	NLS16
		CP(cp, dp);
		strcLIN(genbuf);
#else
		STRCPY(cp, dp);
		STRCLIN(genbuf);
#endif

		putmark(addr);
	}
	killed();
}

/*
 * Find a tag in the tags file.
 * Most work here is in parsing the tags file itself.
 */
tagfind(quick)
	bool quick;
{
	char cmdbuf[BUFSIZ];
	char filebuf[FNSIZE];
	char tagfbuf[128];
	register int c, d;
	bool samef = 1;
	int tfcount = 0;
	int omagic, tl;
	char *fn, *fne;
	bool more_tag_files;
#ifdef STDIO		/* mjm: was VMUNIX */
	/*
	 * We have lots of room so we bring in stdio and do
	 * a binary search on the tags file.
	 */
#ifndef TRACE	/* already did this if TRACE set */
# undef EOF
# include <stdio.h>
# undef getchar
# undef putchar
#endif TRACE
	FILE *iof;
	char iofbuf[BUFSIZ];
	long mid;	/* assumed byte offset */
	long top, bot;	/* length of tag file */
	struct stat sbuf;
#endif

	omagic = value(MAGIC);
	tl = value(TAGLENGTH);
	if (!skipend()) {
		register char *lp = lasttag;

		while (!iswhite(peekchar()) && !endcmd(peekchar()))
			if (lp < &lasttag[sizeof lasttag - 2])

#ifndef	NLS16
				*lp++ = getchar();
#else
				*lp++ = c = getchar();
#endif

			else
				ignchar();

#ifdef	NLS16
		if (IS_FIRST(c))
			lp--;
#endif

		*lp++ = 0;
		if (!endcmd(peekchar()))
badtag:
			error((nl_msg(8, "Bad tag|Give one tag per line")));
	} else if (lasttag[0] == 0)
		error((nl_msg(9, "No previous tag")));
	c = getchar();
	if (!endcmd(c))
		goto badtag;
	if (c == EOF)
		ungetchar(c);
	clrstats();

	/*
	 * Loop once for each file in tags "path".
	 */
	CP(tagfbuf, svalue(TAGS));
	for (more_tag_files = 1, fn = fne = tagfbuf; more_tag_files; fn = ++fne) {
		while (*fne && *fne != ' ')
			fne++;
		if (*fne == 0)
			more_tag_files = 0;	/* done, quit after this time */
		else
			*fne = 0;		/* null terminate filename */
#ifdef STDIO		/* mjm: was VMUNIX */
		iof = fopen(fn, "r");
		if (iof == NULL)
			continue;
		tfcount++;
		setbuf(iof, iofbuf);
		fstat(fileno(iof), &sbuf);
		top = sbuf.st_size;
		if (top == 0L || iof == NULL)
			top = -1L;
		bot = 0L;
		while (top >= bot) {
#else
		/*
		 * Avoid stdio and scan tag file linearly.
		 */
		io = open(fn, 0);
		if (io<0)
			continue;
		tfcount++;
		while (getfile() == 0) {
#endif
			/* loop for each tags file entry */

#ifndef	NLS16
			register char *cp = linebuf;
#else
			register char *cp = LINEBUF;
#endif

			register char *lp = lasttag;
			char *oglobp;

#ifdef STDIO		/* mjm: was VMUNIX */
			mid = (top + bot) / 2;
			fseek(iof, mid, 0);
			if (mid > 0)	/* to get first tag in file to work */
				/* scan to next \n */

#ifndef	NLS16
				if(fgets(linebuf, sizeof linebuf, iof)==NULL)
#else
				if(fgets(LINEBUF, sizeof LINEBUF, iof)==NULL)
#endif

					goto goleft;
			/* get the line itself */

#ifndef	NLS16
			if(fgets(linebuf, sizeof linebuf, iof)==NULL)
#else
			if(fgets(LINEBUF, sizeof LINEBUF, iof)==NULL)
#endif

				goto goleft;

#ifndef	NLS16
			linebuf[strlen(linebuf)-1] = 0;	/* was '\n' */
#else
			LINEBUF[strlen(LINEBUF)-1] = 0;	/* was '\n' */
#endif

#endif
			while (*cp && *lp == *cp)
				cp++, lp++;
			/*
			 * This if decides whether there is a tag match.
			 *  A positive taglength means that a
			 *  match is found if the tag given matches at least
			 *  taglength chars of the tag found.
			 *  A taglength of greater than 511 means that a
			 *  match is found even if the tag given is a proper
			 *  prefix of the tag found.  i.e. "ab" matches "abcd"
			 */
#ifdef STDIO		/* mjm: was VMUNIX */
			if ( *lp == 0 && (iswhite(*cp) || tl > 511 || tl > 0 && lp-lasttag >= tl) ) {
				/*
				 * Found a match.  Force selection to be
				 *  the first possible.
				 */
				if ( mid == bot  &&  mid == top ) {
					; /* found first possible match */
				}
				else {
					/* postpone final decision. */
					top = mid;
					continue;
				}
			}
			else {

#ifndef NONLS8 /* 8bit integrity */
#ifndef	NLS16
				if ((unsigned char)*lp > (unsigned char)*cp)
#else
				if ((*lp & TRIM) > (*cp & TRIM))
#endif
#else NONLS8
				if (*lp > *cp)
#endif NONLS8

					bot = mid + 1;
				else
goleft:
					top = mid - 1;
				continue;
			}
#else
			if ( *lp == 0 && (iswhite(*cp) || tl > 511 || tl > 0 && lp-lasttag >= tl) ) {
				; /* Found it. */
			}
			else {
				/* Not this tag.  Try the next */
				continue;
			}
#endif
			/*
			 * We found the tag.  Decode the line in the file.
			 */
#ifdef STDIO		/* mjm: was VMUNIX */
			fclose(iof);
#else
			close(io);
#endif
			/* Rest of tag if abbreviated */
			while (!iswhite(*cp))
				cp++;

			/* name of file */
			while (*cp && iswhite(*cp))
				cp++;
			if (!*cp)
badtags:
				serror((nl_msg(10, "%s: Bad tags file entry")), lasttag);
			lp = filebuf;
			while (*cp && *cp != ' ' && *cp != '\t') {
				if (lp < &filebuf[sizeof filebuf - 2])
					*lp++ = *cp;
				cp++;
			}
			*lp++ = 0;

			if (*cp == 0)
				goto badtags;
			if (dol != zero) {
				/*
				 * Save current position in 't for ^^ in visual.
				 */
				names['t'-'a'] = *dot &~ 01;
				if (inopen) {
					extern CHAR *ncols['z'-'a'+2];
					extern CHAR *cursor;

					ncols['t'-'a'] = cursor;
				}
			}
			strcpy(cmdbuf, cp);
			if (strcmp(filebuf, savedfile) || !edited) {
				char cmdbuf2[sizeof filebuf + 10];

				/* Different file.  Do autowrite & get it. */
				if (!quick) {
					ckaw();
					if (chng && dol > zero)

#ifndef NONLS8 /* User messages */
						error((nl_msg(11, "No write|No write since last change (:tag! overrides)")));
#else NONLS8
						error("No write@since last change (:tag! overrides)");
#endif NONLS8

				}
				oglobp = globp;
				strcpy(cmdbuf2, "e! ");
				strcat(cmdbuf2, filebuf);
				globp = cmdbuf2;
				d = peekc;

#ifndef	NLS16
				ungetchar(0);
#else
				/*
				 * It is necessary to save abd clear
				 * not only peekc but also secondchar.
				 */
				c = secondchar; clrpeekc();
#endif

				commands(1, 1);
				peekc = d;

#ifdef	NLS16
				/*
				 * It is necessary to restore secondchar.
				 */
				secondchar = c;
#endif

				globp = oglobp;
				value(MAGIC) = omagic;
				samef = 0;
			}

			/*
			 * Look for pattern in the current file.
			 */
			oglobp = globp;
			globp = cmdbuf;
			d = peekc;

#ifndef	NLS16
			ungetchar(0);
#else
			/*
			 * It is necessary to save and clear not only
			 * peekc but also secondchar.
			 */
			c = secondchar; clrpeekc();
#endif

			if (samef)
				markpr(dot);
			/*
			 * BUG: if it isn't found (user edited header
			 * line) we get left in nomagic mode.
			 */
			value(MAGIC) = 0;
			commands(1, 1);
			peekc = d;

#ifdef	NLS16
			/*
			 * It is necessary to restore secondchar.
			 */
			secondchar = c;
#endif

			globp = oglobp;
			value(MAGIC) = omagic;
			return;
		}	/* end of "for each tag in file" */

		/*
		 * No such tag in this file.  Close it and try the next.
		 */
#ifdef STDIO		/* mjm: was VMUNIX */
		fclose(iof);
#else
		close(io);
#endif
	}	/* end of "for each file in path" */
	if (tfcount <= 0)
		error((nl_msg(12, "No tags file")));
	else

#ifndef NONLS8 /* User messages */
		serror((nl_msg(13, "%s: No such tag|%s: No such tag in tags file")), lasttag);
#else NONLS8
		serror("%s: No such tag@in tags file", lasttag);
#endif NONLS8

}

/*
 * Save lines from addr1 thru addr2 as though
 * they had been deleted.
 */
yank()
{

	if (!FIXUNDO)
		error((nl_msg(14, "Can't yank inside global/macro")));
	save12();
	undkind = UNDNONE;
	killcnt(addr2 - addr1 + 1);
}

/*
 * z command; print windows of text in the file.
 *
 * If this seems unreasonably arcane, the reasons
 * are historical.  This is one of the first commands
 * added to the first ex (then called en) and the
 * number of facilities here were the major advantage
 * of en over ed since they allowed more use to be
 * made of fast terminals w/o typing .,.22p all the time.
 */
bool	zhadpr;
bool	znoclear;
short	zweight;

zop(hadpr)
	int hadpr;
{
	register int c, nlines, op;
	bool excl;

	zhadpr = hadpr;
	notempty();
	znoclear = 0;
	zweight = 0;
	excl = exclam();
	switch (c = op = getchar()) {

	case '^':
		zweight = 1;
	case '-':
	case '+':
		while (peekchar() == op) {
			ignchar();
			zweight++;
		}
	case '=':
	case '.':
		c = getchar();
		break;

	case EOF:
		znoclear++;
		break;

	default:
		op = 0;
		break;
	}

#ifndef NONLS8 /* Character set features */
	if ((c >= IS_MACRO_LOW_BOUND) && isdigit(c & TRIM)) {
#else NONLS8
	if ((c >= IS_MACRO_LOW_BOUND) && isdigit(c)) {
#endif NONLS8

		nlines = c - '0';
		for(;;) {
			c = getchar();

#ifndef NONLS8 /* Character set features */
			if ( (c < IS_MACRO_LOW_BOUND) || !isdigit(c & TRIM))
#else NONLS8
			if ( (c < IS_MACRO_LOW_BOUND) || !isdigit(c))
#endif NONLS8

				break;
			nlines *= 10;
			nlines += c - '0';
		}
		if (nlines < lines)
			znoclear++;
		value(WINDOW) = nlines;
		if (op == '=')
			nlines += 2;
	} else
		nlines = op == EOF ? value(SCROLL) : excl ? lines - 1 : 2*value(SCROLL);
	if (inopen || c != EOF) {
		ungetchar(c);
		donewline();
	}
	addr1 = addr2;
	if (addr2 == 0 && dot < dol && op == 0)
		addr1 = addr2 = dot+1;
	setdot();
	zop2(nlines, op);
}

zop2(nlines, op)
	register int nlines;
	register int op;
{
	register line *split;

	split = NULL;
	switch (op) {

	case EOF:
		if (addr2 == dol)
			error((nl_msg(15, "\nAt EOF")));
	case '+':
		if (addr2 == dol)
			error((nl_msg(16, "At EOF")));
		addr2 += nlines * zweight;
		if (addr2 > dol)
			error((nl_msg(17, "Hit BOTTOM")));
		addr2++;
	default:
		addr1 = addr2;
		addr2 += nlines-1;
		dot = addr2;
		break;

	case '=':
	case '.':
		znoclear = 0;
		nlines--;
		nlines >>= 1;
		if (op == '=')
			nlines--;
		addr1 = addr2 - nlines;
		if (op == '=')
			dot = split = addr2;
		addr2 += nlines;
		if (op == '.') {
			markDOT();
			dot = addr2;
		}
		break;

	case '^':
	case '-':
		addr2 -= nlines * zweight;
		if (addr2 < one)
			error((nl_msg(18, "Hit TOP")));
		nlines--;
		addr1 = addr2 - nlines;
		dot = addr2;
		break;
	}
	if (addr1 <= zero)
		addr1 = one;
	if (addr2 > dol)
		addr2 = dol;
	if (dot > dol)
		dot = dol;
	if (addr1 > addr2)
		return;
	if (op == EOF && zhadpr) {
		getline(*addr1);
		putchar('\r' | QUOTE);
		shudclob = 1;
	} else if (znoclear == 0 && clear_screen != _NOSTR && !inopen) {
		flush1();
		vclear();
	}
	if (addr2 - addr1 > 1)
		pstart();
	if (split) {
		plines(addr1, split - 1, 0);
		splitit();
		plines(split, split, 0);
		splitit();
		addr1 = split + 1;
	}
	plines(addr1, addr2, 0);
}

static
splitit()
{
	register int l;

	for (l = columns > 80 ? 40 : columns / 2; l > 0; l--)
		putchar('-');
	putnl();
}

plines(adr1, adr2, movedot)
	line *adr1;
	register line *adr2;
	bool movedot;
{
	register line *addr;

#ifdef ED1000
	line	*markp;
	char	temp[151];
	CHAR	*ptr;
	CHAR	*saveptr;
	int	ch;
	char	tempch;
	int	minus,i,ptoi;
	int	j;
	int	no_mark;
	char	buf[LBSIZE];
	CHAR	BUF[LBSIZE];

	/*  display fn on = esc Y; off = esc Z, 2 * cursor left, 2 * delete char */

	static char dfon[] = "\033Y";
	static char dfoff[] = "\033Z\033D\033P\033D\033P";
#endif ED1000

	pofix();
	for (addr = adr1; addr <= adr2; addr++) {
		getline(*addr);
#ifdef ED1000
		ptr = linebuf + STRLEN(linebuf) - 1;
		j = column(ptr);
		if ( column(ptr) > 79 && insm){
			/* svn: in scrn mode and line is > 79 chars */
			ptr=vfindcol(79);

			if (column(ptr) > 79 ) {
				saveptr = ptr;
				/* (orig):
				 * strinsert2(linebuf,"..",ptr-linebuf);
				 */
				/* this is one option, with the STRING
				 * functions, you have fewer function
				 * calls:
				 *
				 * CHAR_TO_char(linebuf, buf);
				 * strinsert2(buf,"..",ptr-linebuf);
				 * char_TO_CHAR(buf, linebuf);
				 */
				char_TO_CHAR("..", BUF);
				STRINSERT2(linebuf, BUF, ptr-linebuf);

				while (column(ptr) <= 80){
					/* (orig):
					 * strinsert2(linebuf," ",ptr-linebuf);
					 */
					/* possible option, fewer calls
					 * with STRING functions.
					 * 
					 * CHAR_TO_char(linebuf, buf);
					 * strinsert2(buf," ",ptr-linebuf);
					 * char_TO_CHAR(buf, linebuf);
					 */
					char_TO_CHAR(" ", BUF);
					STRINSERT2(linebuf, BUF, ptr-linebuf);
					ptr++;
				}
			} else if (column(ptr) == 79){
				ptr++;
				/* (orig):
				 * strinsert2(linebuf,"..",ptr-linebuf);
				 */
				/* possible option; fewer function calls
				 * with STRING functions.
				 *
				 * CHAR_TO_char(linebuf, buf);
				 * strinsert2(buf,"..",ptr-linebuf);
				 * char_TO_CHAR(buf, linebuf);
				 */
				char_TO_CHAR("..", BUF);
				STRINSERT2(linebuf, BUF, ptr-linebuf);
			}

		}
		no_mark = 0;
		if (insm){
			for (markp =( anymarks ? names : &names['z' -'a'+1])
			    ;
			markp <= &names['z'-'a'];markp++)
			if (*markp == *addr ){
				no_mark = 1;
				ch = markp - names + 'a';
				i = STRLEN(linebuf);
				temp[0] = ':';
				temp[1] = ch;
				temp[2] = '\0';

				/* (orig):
				 * strinsert2(linebuf,temp,i);
				 */
				/* possible option; fewer calls with
				 * the STRING functions.
				 *
				 * CHAR_TO_char(linebuf, buf);
				 * strinsert2(buf,temp,i);
				 * char_TO_CHAR(buf, linebuf);
				 */
				char_TO_CHAR(temp, BUF);
				STRINSERT2(linebuf, BUF, i);

				if (column(&linebuf[i]) > 80 )
					minus = 80;
				else	minus = 0;

				while ((column(&linebuf[i])-minus)< 80){
					/* (orig):
					 * strinsert2(linebuf," ",i);
					 */
					/*
					 * CHAR_TO_char(linebuf, buf);
					 * strinsert2(buf," ",i);
					 * char_TO_CHAR(buf, linebuf);
					 */
					char_TO_CHAR(" ", BUF);
					STRINSERT2(linebuf, BUF, i);
					i++;
				} /* while */

			} /* if (*markp == *addr) */

			if (no_mark == 0) {
				no_mark = STRLEN(linebuf);
				/* (orig):
				 * strinsert2(linebuf," ",no_mark);
				 * strinsert2(linebuf," ",no_mark);
				 */
				/*
				 * CHAR_TO_char(linebuf, buf);
				 * strinsert2(buf," ",no_mark);
				 * strinsert2(buf," ",no_mark);
				 * char_TO_CHAR(buf, linebuf);
				 */
				char_TO_CHAR(" ", BUF);
				STRINSERT2(linebuf, BUF, no_mark);
				STRINSERT2(linebuf, BUF, no_mark);
			}
			/*  if any control characters, turn display functions on & off */

			if (value(DISPLAY_FNTS) == 1){
				for(i=STRLEN(linebuf)-1;i>=0;i--) {
					if (linebuf[i] < 32 && sm_ctrltab[linebuf[i]]!=' ') {
						/* (orig):
						 * strinsert2(linebuf,dfon,0);
						 * strinsert2(linebuf,dfoff,strlen(linebuf));
						 */
						/*
						 * CHAR_TO_char(linebuf, buf);
						 * strinsert2(buf,dfon,0);
						 * strinsert2(buf,dfoff,strlen(linebuf));
						 * char_TO_CHAR(buf, linebuf);
						 */
						char_TO_CHAR(dfon, BUF);
						STRINSERT2(linebuf, BUF, 0);
						char_TO_CHAR(dfoff, BUF);
						STRINSERT2(linebuf, BUF, STRLEN(linebuf));

						break;
					}
				}
			}
		}
#endif ED1000
		pline(lineno(addr));
		if (inopen)
			putchar('\n' | QUOTE);
		if (movedot)
			dot = addr;
	}
}

pofix()
{

	if (inopen && Outchar != termchar) {
		vnfl();
		setoutt();
	}
}

/*
 * Dudley doright to the rescue.
 * Undo saves the day again.
 * A tip of the hatlo hat to Warren Teitleman
 * who made undo as useful as do.
 *
 * Command level undo works easily because
 * the editor has a unique temporary file
 * index for every line which ever existed.
 * We don't have to save large blocks of text,
 * only the indices which are small.  We do this
 * by moving them to after the last line in the
 * line buffer array, and marking down info
 * about whence they came.
 *
 * Undo is its own inverse.
 */
undo(c)
	bool c;
{
	register int i;
	register line *jp, *kp;
	line *dolp1, *newdol, *newadot;

#ifdef UNDOTRACE
	if (trace)
		vudump("before undo");
#endif
	if (inglobal && inopen <= 0)

#ifndef NONLS8 /* User messages */
		error((nl_msg(19, "Can't undo in global|Can't undo in global commands")));
#else NONLS8
		error("Can't undo in global@commands");
#endif NONLS8

	if (!c)
		somechange();
	pkill[0] = pkill[1] = 0;
	change();
	if (undkind == UNDMOVE) {
 		/*
		 * Command to be undone is a move command.
		 * This is handled as a special case by noting that
		 * a move "a,b m c" can be inverted by another move.
		 */
		if ((i = (jp = unddel) - undap2) > 0) {
			/*
			 * when c > b inverse is a+(c-b),c m a-1
			 */
			addr2 = jp;
			addr1 = (jp = undap1) + i;
			unddel = jp-1;
		} else {
			/*
			 * when b > c inverse is  c+1,c+1+(b-a) m b
			 */
			addr1 = ++jp;
			addr2 = jp + ((unddel = undap2) - undap1);
		}
		kp = undap1;
		move1(0, unddel);
		dot = kp;
		Command = "move";
		killed();
	} else {
		int cnt;

		newadot = dot;
		cnt = lineDOL();
		newdol = dol;
		dolp1 = dol + 1;
		/*
		 * Command to be undone is a non-move.
		 * All such commands are treated as a combination of
		 * a delete command and a append command.
		 * We first move the lines appended by the last command
		 * from undap1 to undap2-1 so that they are just before the
		 * saved deleted lines.
		 */
		if ((i = (kp = undap2) - (jp = undap1)) > 0) {
			if (kp != dolp1) {
				reverse(jp, kp);
				reverse(kp, dolp1);
				reverse(jp, dolp1);
			}
			/*
			 * Account for possible backward motion of target
			 * for restoration of saved deleted lines.
			 */
			if (unddel >= jp)
				unddel -= i;
			newdol -= i;
			/*
			 * For the case where no lines are restored, dot
			 * is the line before the first line deleted.
			 */
			dot = jp-1;
		}
		/*
		 * Now put the deleted lines, if any, back where they were.
		 * Basic operation is: dol+1,unddol m unddel
		 */
		if (undkind == UNDPUT) {
			unddel = undap1 - 1;
			squish();
		}
		jp = unddel + 1;
		if ((i = (kp = unddol) - dol) > 0) {
			if (jp != dolp1) {
				reverse(jp, dolp1);
				reverse(dolp1, ++kp);
				reverse(jp, kp);
			}
			/*
			 * Account for possible forward motion of the target
			 * for restoration of the deleted lines.
			 */
			if (undap1 >= jp)
				undap1 += i;
			/*
			 * Dot is the first resurrected line.
			 */
			dot = jp;
			newdol += i;
		}
		/*
		 * Clean up so we are invertible
		 */
		unddel = undap1 - 1;
		undap1 = jp;
		undap2 = jp + i;
		dol = newdol;
		netchHAD(cnt);
		if (undkind == UNDALL) {
			dot = undadot;
			undadot = newadot;
		} else
			undkind = UNDCHANGE;
	}
	/*
	 * Defensive programming - after a munged undadot.
	 * Also handle empty buffer case.
	 */
	if ((dot <= zero || dot > dol) && dot != dol)
		dot = one;
#ifdef UNDOTRACE
	if (trace)
		vudump("after undo");
#endif
}

/*
 * Be (almost completely) sure there really
 * was a change, before claiming to undo.
 */
somechange()
{
	register line *ip, *jp;

	switch (undkind) {

	case UNDMOVE:
		return;

	case UNDCHANGE:
		if (undap1 == undap2 && dol == unddol)
			break;
		return;

	case UNDPUT:
		if (undap1 != undap2)
			return;
		break;

	case UNDALL:
		if (unddol - dol != lineDOL())
			return;
		for (ip = one, jp = dol + 1; ip <= dol; ip++, jp++)
			if ((*ip &~ 01) != (*jp &~ 01))
				return;
		break;

	case UNDNONE:
		error((nl_msg(20, "Nothing to undo")));
	}
	error((nl_msg(21, "Nothing changed|Last undoable command didn't change anything")));
}

/*
 * Map or abbreviate command:
 * map|ab lhs rhs
 */
#define AB_LMAX  40	/* max char for lhs in the ab command */
#define MAP_LMAX 40	/* max char for lhs in the map command */
#define RMAX     240    /* max char for rhs in both ab/map command */
/*
 * Rev 70.3 notes:
 * The max # of char for the Right Hand Side is set to 240. Idealy,
 * this should be set the LINE_MAX (2048), but apparently, if in ab, rhs
 * goes longer than (around) 250 chars, the mapping will screw up the screen.
 */
mapcmd(un, ab)
	int un;	/* true if this is unmap command */
	int ab;	/* true if this is abbr command */
{
	char lhs[RMAX+1]; 	/* same size as rhs, requires for unmap */
	char rhs[RMAX+1];	/* FSDlj09328 */
	register char *p;
	register int c;		/* mjm: char --> int */
	char *dname;
	struct maps *mp;	/* the map structure we are working on */
#if defined NLS || defined NLS16
	char *ws = " \t";
	if (right_to_left) ws = rl_white;
#endif

	mp = ab ? abbrevs : exclam() ? immacs : arrows;
	if (skipend()) {
		int i;
		int i_start;

		/* print current mapping values */
		if (peekchar() != EOF)
			ignchar();
		if (un)
			error((nl_msg(22, "Missing lhs")));
		if (inopen)
			pofix();
		i_start = (mp==arrows && !value(KEYBOARDEDIT)) ? arrows_start :
			  (mp==immacs && !value(KEYBOARDEDIT_I)) ? immacs_start :
			  0;
		for (i=i_start; mp[i].mapto; i++)
			if (mp[i].cap) {
				lprintf("%s", mp[i].descr);
				putchar('\t');
				lprintf("%s", mp[i].cap);
				putchar('\t');
				lprintf("%s", mp[i].mapto);
				putNFL();
			}
		return;
	}

	ignore(skipwh());
	for (p=lhs; ; ) {
		c = getchar();
		if (c == CTRL(v)) {
			c = getcd();	/* getcd() so .exrc can map ^D */
#if defined NLS || defined NLS16
		} else if (!un && any(c, ws)) {
#else
		} else if (!un && any(c, " \t")) {
#endif
			/* End of lhs */
			break;
		} else if (endcmd(c) && c!='"') {
			ungetchar(c);
			if (un) {
				donewline();
				*p = 0;
				addmac(lhs, _NOSTR, _NOSTR, mp);
				return;
			} else
				error((nl_msg(23, "Missing rhs")));
		}
		*p++ = c;
	}
	*p = 0;
	/* check LHS for being too long */
	if (p - lhs > (ab ? AB_LMAX : MAP_LMAX)) 
		error((nl_msg(31, "Too much macro text")));

	if (skipend())
		error((nl_msg(24, "Missing rhs")));
	for (p=rhs; ; ) {
		c = getchar();
		if (c == CTRL(v)) {
			c = getcd();	/* getcd() so .exrc can map ^D */
		} else if (endcmd(c) && c!='"') {
			ungetchar(c);
			break;
		}
		*p++ = c;
	}
	*p = 0;
	/* FSDlj09328: prevent marco string going too long */
	/*
	 * Rev 70.3 notes: 
	 * Reduce the upper bound from MAXCHARMACS (2048) to RMAX (240)
 	 * to avoid wired behavior on loooong mac's. 
	 */
	if (p - rhs > RMAX) 
		error((nl_msg(31, "Too much macro text")));

	donewline();
	/*
	 * For "map" command only:
	 * Special hack for function keys: #1 means key f1, etc.
	 * If the terminal doesn't have function keys, we just use #1.
	 */
	if (!ab && lhs[0] == '#') { 
		char *fnkey;
		char *fkey();

#ifndef	NLS16
		char funkey[3];
#else
		char funkey[4];
#endif


		fnkey = fkey(lhs[1] - '0');

#ifndef	NLS16
		funkey[0] = 'f'; funkey[1] = lhs[1]; funkey[2] = 0;
#else
		funkey[0] = 'f'; funkey[1] = lhs[1]; funkey[3] = 0;
		if (FIRSTof2(lhs[1]&TRIM) && SECONDof2(lhs[2]&TRIM))
			funkey[2] = lhs[2]; 
		else
			funkey[2] = 0; 
#endif

		if (fnkey)
			strcpy(lhs, fnkey);
		dname = funkey;
	} else {
		dname = lhs;
	}
	addmac(lhs,rhs,dname,mp);
}

/*
 * Add a macro definition to those that already exist. The sequence of
 * chars "src" is mapped into "dest". If src is already mapped into something
 * this overrides the mapping. There is no recursion. Unmap is done by
 * using _NOSTR for dest.  Dname is what to show in listings.  mp is
 * the structure to affect (arrows, etc).
 */
addmac(src,dest,dname,mp)
	register char *src, *dest, *dname;
	register struct maps *mp;
{
	register int slot, zer;
	bool deleted_map;

#ifdef UNDOTRACE
	if (trace)
		fprintf(trace, "addmac(src='%s', dest='%s', dname='%s', mp=%x\n", src, dest, dname, mp);
#endif
	if (dest && mp==arrows) {	/* :map */
		/* Make sure user doesn't screw himself */
		/*
		 * Prevent tail recursion. We really should be
		 * checking to see if src is a suffix of dest
		 * but this makes mapping involving escapes that
		 * is reasonable mess up.
		 */

#ifndef	NLS16
		if (src[1] == 0 && src[0] == dest[strlen(dest)-1])
#else
		if ((src[1] == 0 || (FIRSTof2(src[0]&TRIM) && SECONDof2(src[1]&TRIM) && src[2] == 0))
		  && eq(src, dest+strlen(dest)-strlen(src)))
#endif

			error((nl_msg(25, "No tail recursion")));
		/*
		 * We don't let the user rob himself of ":", and making
		 * multi char words is a bad idea so we don't allow it.
		 * Note that if user sets mapinput and maps all of return,
		 * linefeed, and escape, he can screw himself. This is
		 * so weird I don't bother to check for it.
		 */

#ifndef NONLS8 /* Character set features */
		if ( ((src[0] >= IS_MACRO_LOW_BOUND) && isalpha(src[0] & TRIM) && src[1]) || any(src[0],":") )
#else NONLS8
		if ( ((src[0] >= IS_MACRO_LOW_BOUND) && isalpha(src[0]) && src[1]) || any(src[0],":") )
#endif NONLS8

			error((nl_msg(26, "Too dangerous to map that")));
	}
	else if (dest && mp==immacs) {		/* :map! */
		/* 
		 * for map! command, it is recursive if src is a tail 
  		 * substring of dest, or a non-alphanumeric follows
 		 * the substring.
		 */
                int lsrc  = strlen(src);
                int ldest = strlen(dest);
                int i;

                for (i=0; i<=ldest-lsrc; i++)  
			if (!isalnum((int)dest[i+lsrc]) && 
			    !strncmp(src, dest+i, lsrc))
				error((nl_msg(27, "No recursive mapping")));
	}
	else if (dest) {			/* :ab */
		/* FSDlj08906:
		 * Unlike map!, it becomes recursive only if src is a 
		 * substring of dest, and the substring is enclosed 
		 * within non-alphanumeric characters.
		 * Eg. if src = abc, dest is recursive if it is:
		 *     abc or .abc or abc. or .abc.
		 * but are OK in the following cases:
		 *     xabc or abcx or xabcx
		 */
		int lsrc  = strlen(src);
		int ldest = strlen(dest);
		int i;

		for (i=0; i<=ldest-lsrc; i++) {
			if (i == 0) {
				if (!isalnum((int)dest[lsrc]) && 
				    !strncmp(src, dest, lsrc)) 
					error((nl_msg(27, "No recursive mapping")));
			}
			else if (!isalnum((int)dest[i-1]) && 
				 !isalnum((int)dest[i+lsrc]) && 
				 !strncmp(src, dest+i, lsrc))
				error((nl_msg(27, "No recursive mapping")));
		}	
	}
	/*
	 * If the src were null it would cause the dest to
	 * be mapped always forever. This is not good.
	 */
	if (src == _NOSTR || src[0] == 0)
		error((nl_msg(28, "Missing lhs")));

	/* see if we already have a def for src */
	deleted_map = 0;
	for (slot=0; mp[slot].mapto; slot++) {
		if (eq(src, mp[slot].cap) || (mp == abbrevs && dest == _NOSTR && eq(src, mp[slot].mapto))) {
			zer = slot;
			do {
				/* if so, squeeze out slot */
				mp[zer] = mp[zer+1];
			} while (mp[++zer].cap);
			deleted_map++;
			/* may have fewer terminfo maps to keep track of */
			if (mp==arrows && arrows_start && slot<arrows_start)
				arrows_start--;
			else if (mp==immacs && immacs_start && slot<immacs_start)
				immacs_start--;
			/* have to redo this one cause they all moved down */
			slot--;
		}
	}

	if (dest == _NOSTR) {
		/* unmap */
		if (!deleted_map)
			error((nl_msg(29, "Not mapped|That macro wasn't mapped")));
		return;
	}

	/* append new map to end of existing maps */
	if (slot >= MAXNOMACS)
		error((nl_msg(30, "Too many macros")));
	if (msnext == 0)	/* first time */
		msnext = mapspace;
	/* Check is a bit conservative, we charge for dname even if reusing src */
	if (msnext - mapspace + strlen(dest) + strlen(src) + strlen(dname) + 3 > MAXCHARMACS)
		error((nl_msg(31, "Too much macro text")));
	CP(msnext, src);
	mp[slot].cap = msnext;
	msnext += strlen(src) + 1;	/* plus 1 for null on the end */
	CP(msnext, dest);
	mp[slot].mapto = msnext;
	msnext += strlen(dest) + 1;
	if (dname) {
		CP(msnext, dname);
		mp[slot].descr = msnext;
		msnext += strlen(dname) + 1;
	} else {
		/* default descr to string user enters */
		mp[slot].descr = src;
	}
}

/*
 * Implements macros from command mode. c is the buffer to
 * get the macro from.
 */
cmdmac(c)
char c;
{
	char macbuf[BUFSIZ];
	line *ad, *a1, *a2;
	char *oglobp;
	short pk;
	bool oinglobal;

	lastmac = c;
	oglobp = globp;
	oinglobal = inglobal;
	pk = peekc; peekc = 0;
	if (inglobal < 2)
		inglobal = 1;
	regbuf(c, macbuf, sizeof(macbuf));
	a1 = addr1; a2 = addr2;
	for (ad=a1; ad<=a2; ad++) {
		globp = macbuf;
		dot = ad;
		commands(1,1);
	}
	globp = oglobp;
	inglobal = oinglobal;
	peekc = pk;
}

char *
vgetpass(prompt)
char *prompt;
{
	register char *p;
	register c;
	static char pbuf[9];
	char *getpass();

	/* In ex mode, let the system hassle with setting no echo */
	if (!inopen)
		return getpass(prompt);
	printf("%s", prompt); flush();
	for (p=pbuf; (c = getkey())!='\n' && c!=EOF && c!='\r';) {
		if (p < &pbuf[8])
			*p++ = c;
	}
	*p = '\0';
	return(pbuf);
}
