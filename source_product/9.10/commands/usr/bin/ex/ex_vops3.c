/* @(#) $Revision: 66.3 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Routines to handle structure.
 * Operations supported are:
 *	( ) { } [ ]
 *
 * These cover:		LISP		TEXT
 *	( )		s-exprs		sentences
 *	{ }		list at same	paragraphs
 *	[ ]		defuns		sections
 *
 * { and } for C used to attempt to do something with matching {}'s, but
 * I couldn't find definitions which worked intuitively very well, so I
 * scrapped this.
 *
 * The code here is very hard to understand.
 */
line	*llimit;
int	(*lf)();

int	lindent();

bool	wasend;

/*
 * Find over structure, repeated count times.
 * Don't go past line limit.  F is the operation to
 * be performed eventually.  If pastatom then the user said {}
 * rather than (), implying past atoms in a list (or a paragraph
 * rather than a sentence.
 */
lfind(pastatom, cnt, f, limit)
	bool pastatom;
	int cnt, (*f)();
	line *limit;
{
	register int c;
	register int rc = 0;
	CHAR save[LBSIZE];

	/*
	 * Initialize, saving the current line buffer state
	 * and computing the limit; a 0 argument means
	 * directional end of file.
	 */
	wasend = 0;
	lf = f;

#ifndef	NLS16
	strcpy(save, linebuf);
#else
	STRCPY(save, linebuf);
#endif

	if (limit == 0)
		limit = dir < 0 ? one : dol;
	llimit = limit;
	wdot = dot;
	wcursor = cursor;

	if (pastatom >= 2) {
		while (cnt > 0 && word(f, cnt))
			cnt--;
		if (pastatom == 3)
			eend(f);
		if (dot == wdot) {
			wdot = 0;
			if (cursor == wcursor)
				rc = -1;
		}
	}
	else if (!value(LISP)) {
		CHAR *icurs;

		line *idot;
		if (linebuf[0] == 0) {
			do
				if (!lnext())
					goto ret;
			while (linebuf[0] == 0);
			if (dir > 0) {
				wdot--;
				linebuf[0] = 0;
				wcursor = linebuf;
				/*
				 * If looking for sentence, next line
				 * starts one.
				 */
				if (!pastatom) {
					icurs = wcursor;
					idot = wdot;
					goto begin;
				}
			}
		}
		icurs = wcursor;
		idot = wdot;

		/*
		 * Advance so as to not find same thing again.
		 */
		if (dir > 0) {
			if (!lnext()) {
				rc = -1;
				goto ret;
			}
		} else
			ignore(lskipa1(""));

		/*
		 * Count times find end of sentence/paragraph.
		 */
begin:
		for (;;) {
			while (!endsent(pastatom))
				if (!lnext())
					goto ret;
			if (!pastatom || wcursor == linebuf && endPS())
				if (--cnt <= 0)
					break;
			if (linebuf[0] == 0) {
				do
					if (!lnext())
						goto ret;
				while (linebuf[0] == 0);
			} else
				if (!lnext())
					goto ret;
		}

		/*
		 * If going backwards, and didn't hit the end of the buffer,
		 * then reverse direction.
		 */

		if (dir < 0 && (wdot != llimit || wcursor != linebuf)) {
			dir = 1;
			llimit = dot;
			/*
			 * Empty line needs special treatement.
			 * If moved to it from other than begining of next line,
			 * then a sentence starts on next line.
			 */
			if (linebuf[0] == 0 && !pastatom && 
			   (wdot != dot - 1 || cursor != linebuf)) {
				lnext();
				goto ret;
			}
		}

		/*
		 * If we are not at a section/paragraph division,
		 * advance to next.
		 */

		if (wcursor == icurs && wdot == idot || wcursor != linebuf || !endPS())
			ignore(lskipa1(""));
	}
	else {
		c = *wcursor;
		/*
		 * Startup by skipping if at a ( going left or a ) going
		 * right to keep from getting stuck immediately.
		 */
		if (dir < 0 && c == '(' || dir > 0 && c == ')') {
			if (!lnext()) {
				rc = -1;
				goto ret;
			}
		}
		/*
		 * Now chew up repitition count.  Each time around
		 * if at the beginning of an s-exp (going forwards)
		 * or the end of an s-exp (going backwards)
		 * skip the s-exp.  If not at beg/end resp, then stop
		 * if we hit a higher level paren, else skip an atom,
		 * counting it unless pastatom.
		 */
		while (cnt > 0) {
			c = *wcursor;
			if (dir < 0 && c == ')' || dir > 0 && c == '(') {
				if (!lskipbal("()"))
					goto ret;
				/*
 				 * Unless this is the last time going
				 * backwards, skip past the matching paren
				 * so we don't think it is a higher level paren.
				 */
				if (dir < 0 && cnt == 1)
					goto ret;
				if (!lnext() || !ltosolid())
					goto ret;
				--cnt;
			} else if (dir < 0 && c == '(' || dir > 0 && c == ')')
				/* Found a higher level paren */
				goto ret;
			else {
				if (!lskipatom())
					goto ret;
				if (!pastatom)
					--cnt;
			}
		}
	}
ret:

#ifndef	NLS16
	strcLIN(save);
#else
	STRCLIN(save);
#endif

	return (rc);
}

/*
 * Is this the end of a sentence?
 */
endsent(pastatom)
	bool pastatom;
{
	register CHAR *cp = wcursor;
	register int c, d;

	/*
	 * If this is the beginning of a line, then
	 * check for the end of a paragraph or section.
	 */

	if (cp == linebuf)
		return (endPS());

	/*
	 * Sentences end with . ! ? not at the beginning
	 * of the line, and must be either at the end of the line,
	 * or followed by 2 spaces.  Any number of intervening ) ] ' "
	 * characters are allowed.
	 */
#if defined NLS || defined NLS16
	if (right_to_left) {
		if (!any(c = *cp & TRIM, rl_endsent))
			goto tryps;
		do
			if ((d = *++cp & TRIM) == 0)
				return (1);
		while (any(d, rl_insent));

		if (*cp == 0)
			return (1);
		if (*cp == ' ' || ((*cp & TRIM) == alt_space))
			if (*++cp == ' ' || ((*cp & TRIM) == alt_space))
				return (1);
	} else {
		if (!any(c = *cp, ".!?"))
			goto tryps;
		do
			if ((d = *++cp) == 0)
				return (1);
		while (any(d, ")]'\""));
		if (*cp == 0 || *cp++ == ' ' && *cp == ' ')
			return (1);
	}
#else
	if (!any(c = *cp, ".!?"))
		goto tryps;
	do
		if ((d = *++cp) == 0)
			return (1);
	while (any(d, ")]'"));
	if (*cp == 0 || *cp++ == ' ' && *cp == ' ')
		return (1);
#endif
tryps:
	if (cp[1] == 0)
		return (endPS());
	return (0);
}

/*
 * End of paragraphs/sections are respective
 * macros as well as blank lines and form feeds.
 */
endPS()
{
	return (linebuf[0] == 0 ||
		isa(svalue(PARAGRAPHS)) || isa(svalue(SECTIONS)));
}

lindent(addr)
	line *addr;
{
	register int i;
	CHAR *swcurs = wcursor;
	line *swdot = wdot;

again:
	if (addr > one) {
		register CHAR *cp;
		register int cnt = 0;

		addr--;
		getline(*addr);
		for (cp = linebuf; *cp; cp++)
			if (*cp == '(')
				cnt++;
			else if (*cp == ')')
				cnt--;
		cp = vpastwh(linebuf);
		if (*cp == 0)
			goto again;
		if (cnt == 0)
			return (whitecnt(linebuf));
		addr++;
	}
	wcursor = linebuf;
	linebuf[0] = 0;
	wdot = addr;
	dir = -1;
	llimit = one;
	lf = lindent;
	if (!lskipbal("()"))
		i = 0;
	else if (wcursor == linebuf)
		i = 2;
	else {
		register CHAR *wp = wcursor;

		dir = 1;
		llimit = wdot;
		if (!lnext() || !ltosolid() || !lskipatom()) {
			wcursor = wp;
			i = 1;
		} else
			i = 0;

#ifndef	NLS16
		i += column(wcursor) - 1;
#else
#ifdef EUC
		i += column(wcursor) ;
		if( IS_SECOND(wcursor[-1]) ){
			i -= C_COLWIDTH( ( (int)(wcursor[-2]) ) & TRIM ) ;
		}else{
			i -= 1 ;
		}
#else EUC
		i += column(wcursor) - (IS_SECOND(wcursor[-1]) ? 2 : 1);
#endif EUC
#endif

		if (!inopen)

#ifndef	NLS16
			i--;
#else
#ifdef EUC
			if( IS_SECOND(linebuf[i-1]) ){
				i -= C_COLWIDTH( ( (int)(linebuf[i-2]) ) & TRIM ) ;
			}else{
				i -= 1 ;
			}
#else EUC
			IS_SECOND(linebuf[i-1]) ? (i -= 2) : i--;
#endif EUC
#endif

	}
	wdot = swdot;
	wcursor = swcurs;
	return (i);
}

lmatchp(addr)
	line *addr;
{
	register int i;
	register char *parens;
	register CHAR *cp;

#if defined NLS || defined NLS16
	if (right_to_left) {
		for (cp = cursor; !any(*cp & TRIM, rl_parens);)
			if (*cp++ == 0)
				return (0);
		lf = 0;
		parens = any(*cp, "()") ? "()" :
			 any(*cp, "[]") ? "[]" : 
			 any(*cp, "{}") ? "{}" : 
		         any(*cp & TRIM, rl_paren_1) ? rl_paren_1 :
			 any(*cp & TRIM, rl_paren_2) ? rl_paren_2 :
			 rl_paren_3;
	} else {
		for (cp = cursor; !any(*cp, "({[)}]");)
			if (*cp++ == 0)
				return (0);
		lf = 0;
		parens = any(*cp, "()") ? "()" : any(*cp, "[]") ? "[]" : "{}";
	}
	if (*cp == (parens[1] & TRIM)) {
#else
	for (cp = cursor; !any(*cp, "({[)}]");)
		if (*cp++ == 0)
			return (0);
	lf = 0;
	parens = any(*cp, "()") ? "()" : any(*cp, "[]") ? "[]" : "{}";
	if (*cp == parens[1]) {
#endif
		dir = -1;
		llimit = one;
	} else {
		dir = 1;
		llimit = dol;
	}
	if (addr)
		llimit = addr;
	if (splitw)
		llimit = dot;
	wcursor = cp;
	wdot = dot;
	i = lskipbal(parens);
	return (i);
}

lsmatch(cp)
	CHAR *cp;
{
	CHAR save[LBSIZE];
	register CHAR *sp = save;
	register CHAR *scurs = cursor;

	wcursor = cp;

#ifndef	NLS16
	strcpy(sp, linebuf);
#else
	STRCPY(sp, linebuf);
#endif

	*wcursor = 0;

#ifndef	NLS16
	strcpy(cursor, genbuf);
	cursor = strend(linebuf) - 1;
#else
	STRCPY(cursor, genbuf);
	cursor = REVERSE(STREND(linebuf));
#endif

	if (lmatchp(dot - vcline)) {
		register int i = insmode;
		register int c = outcol;
		register int l = outline;

		if (!move_insert_mode)
			endim();

		vgoto(splitw ? WECHO : LINE(wdot - llimit), column(wcursor) - 1);

		flush();
		sleep(1);
		vgoto(l, c);
		if (i)
			goim();
	}
	else {

#ifndef	NLS16
		strcLIN(sp);
		strcpy(scurs, genbuf);
#else
		STRCLIN(sp);
		STRCPY(scurs, genbuf);
#endif

		if (!lmatchp((line *) 0))
			beep();
	}

#ifndef	NLS16
	strcLIN(sp);
#else
	STRCLIN(sp);
#endif

	wdot = 0;
	wcursor = 0;
	cursor = scurs;
}

ltosolid()
{

	return (ltosol1("()"));
}

ltosol1(parens)
	register char *parens;
{
	register CHAR *cp;

	if (*parens && !*wcursor && !lnext())
		return (0);

#ifndef NONLS8 /* Character set features */
	while (((*wcursor >= IS_MACRO_LOW_BOUND) && _isspace(*wcursor & TRIM)) || (*wcursor == 0 && *parens))
#else NONLS8
	while (((*wcursor >= IS_MACRO_LOW_BOUND) && isspace(*wcursor)) || (*wcursor == 0 && *parens))
#endif NONLS8

		if (!lnext())
			return (0);
	if (any(*wcursor, parens) || dir > 0)
		return (1);

#ifndef	NLS16
	for (cp = wcursor; cp > linebuf; cp--)
#else
	for (cp = wcursor; cp > linebuf; PST_DEC(cp))
#endif

#ifndef NONLS8 /* Character set features */
		if (((cp[-1] >= IS_MACRO_LOW_BOUND) && _isspace(cp[-1] & TRIM)) || any(cp[-1], parens))
#else NONLS8
		if (((cp[-1] >= IS_MACRO_LOW_BOUND) && isspace(cp[-1])) || any(cp[-1], parens))
#endif NONLS8

			break;
	wcursor = cp;
	return (1);
}

lskipbal(parens)
	register char *parens;
{
	register int level = dir;
	register int c;

	do {
		if (!lnext()) {
			wdot = NOLINE;
			return (0);
		}
#if defined NLS || defined NLS16
		c = *wcursor & TRIM;
		if (c == (parens[1] & TRIM))
			level--;
		else if (c == (parens[0] & TRIM))
			level++;
#else
		c = *wcursor;
		if (c == parens[1])
			level--;
		else if (c == parens[0])
			level++;
#endif
	} while (level);
	return (1);
}

lskipatom()
{

	return (lskipa1("()"));
}

lskipa1(parens)
	register char *parens;
{
	register int c;

	for (;;) {
		if (dir < 0 && wcursor == linebuf) {
			if (!lnext())
				return (0);
			break;
		}
		c = *wcursor;

#ifndef NONLS8 /* Character set features */
		if (c && (((c >= IS_MACRO_LOW_BOUND) && _isspace(c & TRIM)) || any(c, parens)))
#else NONLS8
		if (c && (((c >= IS_MACRO_LOW_BOUND) && isspace(c)) || any(c, parens)))
#endif NONLS8

			break;
		if (!lnext())
			return (0);
		if (dir > 0 && wcursor == linebuf)
			break;
	}
	return (ltosol1(parens));
}

lnext()
{

	if (dir > 0) {
		if (*wcursor)

#ifndef	NLS16
			wcursor++;
#else
			PST_INC(wcursor);
#endif

		if (*wcursor)
			return (1);
		if (wdot >= llimit) {
			if (lf == vmove && wcursor > linebuf)

#ifndef	NLS16
				wcursor--;
#else
				PST_DEC(wcursor);
#endif

			return (0);
		}
		wdot++;
		getline(*wdot);
		wcursor = linebuf;
		return (1);
	} else {

#ifndef	NLS16
		--wcursor;
#else
		PRE_DEC(wcursor);
#endif

		if (wcursor >= linebuf)
			return (1);
		if (lf == lindent && linebuf[0] == '(')
			llimit = wdot;
		if (wdot <= llimit) {
			wcursor = linebuf;
			return (0);
		}
		wdot--;
		getline(*wdot);

#ifndef	NLS16
		wcursor = linebuf[0] == 0 ? linebuf : strend(linebuf) - 1;
#else
		wcursor = linebuf[0] == 0 ? linebuf : REVERSE(STREND(linebuf));
#endif

		return (1);
	}
}

lbrack(c, f)
	register int c;
	int (*f)();
{
	register line *addr;

	addr = dot;
	for (;;) {
		addr += dir;
		if (addr < one || addr > dol) {
			addr -= dir;
			break;
		}
		getline(*addr);
		if (linebuf[0] == '{' ||
		    value(LISP) && linebuf[0] == '(' ||
		    isa(svalue(SECTIONS))) {
			if (c == ']' && f != vmove) {
				addr--;
				getline(*addr);
			}
			break;
		}
		if (c == ']' && f != vmove && linebuf[0] == '}')
			break;
	}
	if (addr == dot)
		return (0);
	if (f != vmove)

#ifndef	NLS16
		wcursor = c == ']' ? strend(linebuf) : linebuf;
#else
		wcursor = c == ']' ? STREND(linebuf) : linebuf;
#endif

	else
		wcursor = 0;
	wdot = addr;
	vmoving = 0;
	return (1);
}

isa(cp)
	register char *cp;
{

	if (linebuf[0] != '.')
		return (0);
	for (; cp[0] && cp[1]; cp += 2)
		if (linebuf[1] == cp[0]) {
			if (linebuf[2] == cp[1])
				return (1);
			if (linebuf[2] == 0 && cp[1] == ' ')
				return (1);
		}
	return (0);
}
