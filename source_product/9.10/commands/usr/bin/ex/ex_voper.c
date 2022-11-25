/* @(#) $Revision: 70.2 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifndef NONLS8 /* Character set features */
# define	blank()		_isspace(wcursor[0] & TRIM)
#else NONLS8
# define	blank()		isspace(wcursor[0])
#endif NONLS8

#define	forbid(a)	if (a) goto errlab;

CHAR	vscandir[2] =	{ '/', 0 };

#ifdef NLS16
CHAR	dummy_quest[2] = {'?', 0};		/* "?" for 16-bit */
CHAR	dummy_slash[2] = {'/', 0};		/* "/" for 16-bit */
#endif

/*
 * Decode an operator/operand type command.
 * Eventually we switch to an operator subroutine in ex_vops.c.
 * The work here is setting up a function variable to point
 * to the routine we want, and manipulation of the variables
 * wcursor and wdot, which mark the other end of the affected
 * area.  If wdot is zero, then the current line is the other end,
 * and if wcursor is zero, then the first non-blank location of the
 * other line is implied.
 */
operate(c, cnt)
	register int c, cnt;
{
	register int i;
	int (*moveop)(), (*deleteop)();
	register int (*opf)();
	bool subop = 0;
	char *oglobp;
	CHAR *ocurs;
	register line *addr;
	line *odot;
	static CHAR lastFKND;

#ifndef	NLS16
	static CHAR lastFCHR;
#else
	/*
	 * lastFCHR has 16-bit value of 16-bit character.
	 */
	static int lastFCHR;
#endif

	short d;

	moveop = vmove, deleteop = vdelete;
	wcursor = cursor;
	wdot = NOLINE;
	notecnt = 0;
	dir = 1;
	switch (c) {

	/*
	 * d		delete operator.
	 */
	case 'd':
		moveop = vdelete;
		deleteop = beep;
		break;

	/*
	 * s		substitute characters, like c\040, i.e. change space.
	 */
	case 's':
		ungetkey(' ');
		subop++;
		/* fall into ... */

	/*
	 * c		Change operator.
	 */
	case 'c':
		if (c == 'c' && workcmd[0] == 'C' || workcmd[0] == 'S')
			subop++;
		moveop = vchange;
		deleteop = beep;
		break;

	/*
	 * !		Filter through a UNIX command.
	 */
	case '!':
		moveop = vfilter;
		deleteop = beep;
		break;

	/*
	 * y		Yank operator.  Place specified text so that it
	 *		can be put back with p/P.  Also yanks to named buffers.
	 */
	case 'y':
		moveop = vyankit;
		deleteop = beep;
		break;

	/*
	 * =		Reformat operator (for LISP).
	 */
	case '=':
		forbid(!value(LISP));
		/* fall into ... */

	/*
	 * >		Right shift operator.
	 * <		Left shift operator.
	 */
	case '<':
	case '>':
		moveop = vshftop;
		deleteop = beep;
		break;

	/*
	 * r		Replace character under cursor with single following
	 *		character.
	 */
	case 'r':
		vmacchng(1);
		vrep(cnt);
		return;

	default:
		goto nocount;
	}
	vmacchng(1);

#ifndef ED1000
	wcursor = cursor;
#endif ED1000

	/*
	 * Had an operator, so accept another count.
	 * Multiply counts together.
	 */

#ifndef NONLS8 /* Character set features */
	if ((RL_KEY(peekkey() & TRIM) >= IS_MACRO_LOW_BOUND) && isdigit(RL_KEY(peekkey() & TRIM)) && peekkey() != '0') {
#else NONLS8
	if ((peekkey() >= IS_MACRO_LOW_BOUND) && isdigit(peekkey()) && peekkey() != '0') {
#endif NONLS8

		cnt *= vgetcnt();
		Xcnt = cnt;
		forbid (cnt <= 0);
	}

	/*
	 * Get next character, mapping it and saving as
	 * part of command for repeat.
	 */
	/*
	** The user may have mapped '_' to something so don't use mapping if
	** we are processing the Y (y_) or S (c_) commands.  The documentation
	** tells the user that Y and S are equivalent to yy and cc respectively
	** so the user expects mapping '_' will have no effect on the Y or S
	** command.
	*/
	if (no_map_) {
		no_map_ = 0;
		c = getesc();
	} else
		c = map(getesc(),arrows);
	if (c == 0)
		return;
	if (!subop)
		*lastcp++ = c;
nocount:
	opf = moveop;
	switch (c) {

	/*
	 * b		Back up a word.
	 * B		Back up a word, liberal definition.
	 */
	case 'b':
	case 'B':
		dir = -1;
		/* fall into ... */

	/*
	 * w		Forward a word.
	 * W		Forward a word, liberal definition.
	 */
	case 'W':
	case 'w':
		wdkind = c & ' ';
		forbid(lfind(2, cnt, opf, 0) < 0);
		vmoving = 0;
		break;

	/*
	 * E		to end of following blank/nonblank word
	 */
	case 'E':
		wdkind = 0;
		goto ein;

	/*
	 * e		To end of following word.
	 */
	case 'e':
		wdkind = 1;
ein:
		forbid(lfind(3, cnt - 1, opf, 0) < 0);
		vmoving = 0;
		break;

	/*
	 * (		Back an s-expression.
	 */
	case '(':
		dir = -1;
		/* fall into... */

	/*
	 * )		Forward an s-expression.
	 */
	case ')':
		forbid(lfind(0, cnt, opf, (line *) 0) < 0);
		markDOT();
		break;

	/*
	 * {		Back an s-expression, but don't stop on atoms.
	 *		In text mode, a paragraph.  For C, a balanced set
	 *		of {}'s.
	 */
	case '{':
		dir = -1;
		/* fall into... */

	/*
	 * }		Forward an s-expression, but don't stop on atoms.
	 *		In text mode, back paragraph.  For C, back a balanced
	 *		set of {}'s.
	 */
	case '}':
		forbid(lfind(1, cnt, opf, (line *) 0) < 0);
		markDOT();
		break;

	/*
	 * %		To matching () or {}.  If not at ( or { scan for
	 *		first such after cursor on this line.
	 */
	case '%':
		vsave();
		i = lmatchp((line *) 0);
#ifdef TRACE
		if (trace)
			fprintf(trace, "after lmatchp in %, dot=%d, wdot=%d, dol=%d\n", lineno(dot), lineno(wdot), lineno(dol));
#endif
		getDOT();
		forbid(!i);
		if (opf != vmove)
			if (dir > 0)

#ifndef	NLS16
				wcursor++;
#else
				PST_INC(wcursor);
#endif

			else

#ifndef	NLS16
				cursor++;
#else
				PST_INC(cursor);
#endif

		else
			markDOT();
		vmoving = 0;
		break;

	/*
	 * [		Back to beginning of defun, i.e. an ( in column 1.
	 *		For text, back to a section macro.
	 *		For C, back to a { in column 1 (~~ beg of function.)
	 */
	case '[':
		dir = -1;
		/* fall into ... */

	/*
	 * ]		Forward to next defun, i.e. a ( in column 1.
	 *		For text, forward section.
	 *		For C, forward to a } in column 1 (if delete or such)
	 *		or if a move to a { in column 1.
	 */
	case ']':
		if (!vglobp)
#if defined NLS || defined NLS16
			forbid(RL_KEY(getkey()) != c);
#else
			forbid(getkey() != c);
#endif
		forbid (Xhadcnt);
		vsave();
		i = lbrack(c, opf);
		getDOT();
		forbid(!i);
		markDOT();
		if (ospeed > B300)
			hold |= HOLDWIG;
		break;

	/*
	 * ,		Invert last find with f F t or T, like inverse
	 *		of ;.
	 */
	case ',':
		forbid (lastFKND == 0);

#ifndef NONLS8 /* Character set features */
		c = ((lastFKND >= IS_MACRO_LOW_BOUND) && isupper(lastFKND & TRIM)) 
		    ? tolower(lastFKND & TRIM) : toupper(lastFKND & TRIM);
#else NONLS8
		c = ((lastFKND >= IS_MACRO_LOW_BOUND) && isupper(lastFKND)) 
		    ? tolower(lastFKND) : toupper(lastFKND);
#endif NONLS8

		i = lastFCHR;
		if (vglobp == 0)

#ifndef	NLS16
			vglobp = "";
#else
			vglobp = (CHAR *)"";
#endif

		subop++;
		goto nocount;

	/*
	 * 0		To beginning of real line.
	 */
	case '0':
		wcursor = linebuf;
		vmoving = 0;
		break;

	/*
	 * ;		Repeat last find with f F t or T.
	 */
	case ';':
		forbid (lastFKND == 0);
		c = lastFKND;
		i = lastFCHR;
		subop++;
		goto nocount;

	/*
	 * F		Find single character before cursor in current line.
	 * T		Like F, but stops before character.
	 */
	case 'F':	/* inverted find */
	case 'T':
		dir = -1;
		/* fall into ... */

	/*
	 * f		Find single character following cursor in current line.
	 * t		Like f, but stop before character.
	 */
	case 'f':	/* find */
	case 't':
		if (!subop) {
#if defined NLS || defined NLS16
			if (right_to_left) {
				tputs(key_order, 1, putch), flusho();
				right_to_left = 0;
				i = getesc();
				right_to_left = 1;
				tputs(screen_order, 1, putch), flusho();
			} else {
				i = getesc();
			}
#else
			i = getesc();
#endif
			if (i == 0)
				return;
			*lastcp++ = i;

#ifdef	NLS16		/* get the 2nd byte of 16-bit character */
			if (IS_FIRST(i)) {
				*lastcp++ = getesc();
				i = _CHARAT(lastcp - 2);	/* 16-bit code */
			}
#endif

		}
		if (vglobp == 0)
			lastFKND = c, lastFCHR = i;
		for (; cnt > 0; cnt--)
			forbid (find(i) == 0);
		vmoving = 0;
		switch (c) {

		case 'T':

#ifndef	NLS16
			wcursor++;
#else
			PST_INC(wcursor);
#endif

			break;

		case 't':

#ifndef	NLS16
			wcursor--;
#else
			PST_DEC(wcursor);
#endif

		case 'f':
fixup:
			if (moveop != vmove)

#ifndef	NLS16
				wcursor++;
#else
				PST_INC(wcursor);
#endif

			break;
		}
		break;

	/*
	 * |		Find specified print column in current line.
	 */
	case '|':
		if (Pline == numbline)
			cnt += 8;
		vmovcol = cnt;
		vmoving = 1;
		wcursor = vfindcol(cnt);
		break;

	/*
	 * ^		To beginning of non-white space on line.
	 */
	case '^':
		wcursor = vskipwh(linebuf);
		vmoving = 0;
		break;

	/*
	 * $		To end of line.
	 */
	case '$':
		if (opf == vmove) {
			vmoving = 1;
			vmovcol = 20000;
		} else
			vmoving = 0;
		if (cnt > 1) {
			if (opf == vmove) {
				wcursor = 0;
				cnt--;
			} else
				wcursor = linebuf;
			/* This is wrong at EOF */
			wdot = dot + cnt;
			break;
		}
		if (linebuf[0]) {

#ifndef	NLS16
			wcursor = strend(linebuf) - 1;
#else
			wcursor = REVERSE(STREND(linebuf));
#endif

			goto fixup;
		}
		wcursor = linebuf;
		break;

	/*
	 * h		Back a character.
	 * ^H		Back a character.
	 */
	case 'h':
	case CTRL(h):
		dir = -1;
		/* fall into ... */

	/*
	 * space	Forward a character.
	 */
	case 'l':
	case ' ':
		forbid (margin() || opf == vmove && edge());
		while (cnt > 0 && !margin())

#ifndef	NLS16
			wcursor += dir, cnt--;
#else
			wcursor = NADVANCE(wcursor, dir), cnt--;
#endif

		if (margin() && opf == vmove || wcursor < linebuf)

#ifndef	NLS16
			wcursor -= dir;
#else
			wcursor = NREVERSE(wcursor, dir);
#endif

		vmoving = 0;
		break;

	/*
	 * D		Delete to end of line, short for d$.
	 */
	case 'D':
		cnt = INF;
		goto deleteit;

	/*
	 * X		Delete character before cursor.
	 */
	case 'X':
		dir = -1;
		/* fall into ... */
deleteit:
	/*
	 * x		Delete character at cursor, leaving cursor where it is.
	 */
	case 'x':
		if (margin())
			goto errlab;
		vmacchng(1);
		while (cnt > 0 && !margin())

#ifndef	NLS16
			wcursor += dir, cnt--;
#else
			wcursor = NADVANCE(wcursor, dir), cnt--;
#endif

		opf = deleteop;
		vmoving = 0;
		break;

	default:
		/*
		 * Stuttered operators are equivalent to the operator on
		 * a line, thus turn dd into d_.
		 */
		if (opf == vmove || c != workcmd[0]) {
errlab:
			beep();

#ifdef	NLS16
			/* skip over the remaining 2nd byte of 16-bit character
			** not to take it as the next command.
			*/
#ifdef TEPE
			/*
			** The following peekkey will cause a terminal read
			** so we have to issue the tepe prompt now.  However,
			** since the command read won't actually be used
			** until later when another tepe prompt would be
			** issued, set a flag to disable the 2nd prompt.
			*/
			vputc('\021');
			tepe_ok = 0;
#endif TEPE
			c = peekkey();
			if (IS_SECOND(c))
				ignore(getkey());
#endif

			vmacp = 0;
			return;
		}
		/* fall into ... */

	/*
	 * _		Target for a line or group of lines.
	 *		Stuttering is more convenient; this is mostly
	 *		for aesthetics.
	 */
	case '_':
		wdot = dot + cnt - 1;
		vmoving = 0;
		wcursor = 0;
		break;

	/*
	 * H		To first, home line on screen.
	 *		Count is for count'th line rather than first.
	 */
	case 'H':
		wdot = (dot - vcline) + cnt - 1;
		if (opf == vmove)
			markit(wdot);
		vmoving = 0;
		wcursor = 0;
		break;

	/*
	 * -		Backwards lines, to first non-white character.
	 */
	case '-':
		wdot = dot - cnt;
		vmoving = 0;
		wcursor = 0;
		break;

	/*
	 * ^P		To previous line same column.  Ridiculous on the
	 *		console of the VAX since it puts console in LSI mode.
	 */
	case 'k':
	case CTRL(p):
		wdot = dot - cnt;
		if (vmoving == 0)
			vmoving = 1, vmovcol = column(cursor);
		wcursor = 0;
		break;

	/*
	 * L		To last line on screen, or count'th line from the
	 *		bottom.
	 */
	case 'L':
		wdot = dot + vcnt - vcline - cnt;
		if (opf == vmove)
			markit(wdot);
		vmoving = 0;
		wcursor = 0;
		break;

	/*
	 * M		To the middle of the screen.
	 */
	case 'M':
		wdot = dot + ((vcnt + 1) / 2) - vcline - 1;
		if (opf == vmove)
			markit(wdot);
		vmoving = 0;
		wcursor = 0;
		break;

	/*
	 * +		Forward line, to first non-white.
	 *
	 * CR		Convenient synonym for +.
	 */
	case '+':
	case CR:
		wdot = dot + cnt;
		vmoving = 0;
		wcursor = 0;
		break;

	/*
	 * ^N		To next line, same column if possible.
	 *
	 * LF		Linefeed is a convenient synonym for ^N.
	 */
	case CTRL(n):
	case 'j':
	case NL:
		wdot = dot + cnt;
		if (vmoving == 0)
			vmoving = 1, vmovcol = column(cursor);
		wcursor = 0;
		break;

	/*
	 * n		Search to next match of current pattern.
	 */
	case 'n':
		vglobp = vscandir;
		c = *vglobp++;
		goto nocount;

	/*
	 * N		Like n but in reverse direction.
	 */
	case 'N':

#ifndef	NLS16
		vglobp = vscandir[0] == '/' ? "?" : "/";
#else
		vglobp = vscandir[0] == '/' ? dummy_quest : dummy_slash;
#endif

		c = *vglobp++;
		goto nocount;

	/*
	 * '		Return to line specified by following mark,
	 *		first white position on line.
	 *
	 * `		Return to marked line at remembered column.
	 */
	case '\'':
	case '`':
		d = c;
		c = getesc();
		if (c == 0)
			return;
		c = markreg(c);
		forbid (c == 0);
		wdot = getmark(c);
		forbid (wdot == NOLINE);
		forbid (Xhadcnt);
		vmoving = 0;
		wcursor = d == '`' ? ncols[c - 'a'] : 0;
		if (opf == vmove && (wdot != dot || (d == '`' && wcursor != cursor)))
			markDOT();
		if (wcursor) {
			vsave();
			getline(*wdot);

#ifndef	NLS16
			if (wcursor > strend(linebuf))
#else
			if (wcursor > STREND(linebuf))
#endif

				wcursor = 0;
			getDOT();
		}
		if (ospeed > B300)
			hold |= HOLDWIG;
		break;

	/*
	 * G		Goto count'th line, or last line if no count
	 *		given.
	 */
	case 'G':
		if (!Xhadcnt)
			cnt = lineDOL();
		wdot = zero + cnt;
		forbid (wdot < one || wdot > dol);
		if (opf == vmove)
			markit(wdot);
		vmoving = 0;
		wcursor = 0;
		break;

	/*
	 * /		Scan forward for following re.
	 * ?		Scan backward for following re.
	 */
	case '/':
	case '?':
		forbid (Xhadcnt);
		vsave();
		ocurs = cursor;
		odot = dot;
		wcursor = 0;
		if (readecho(c))
			return;
		if (!vglobp)
			vscandir[0] = genbuf[0];

#ifndef	NLS16
		oglobp = globp; CP(vutmp, genbuf); globp = vutmp;
#else
		oglobp = globp; CHAR_TO_char(genbuf, vcmd); globp = vcmd;
#endif

#ifdef ED1000
		/*
		 *	Save away the rest of the command to make . work.
		 *	Before, only an n was saved so any +- line counts
		 *	after the pattern match was lost.  This way we will
		 *	save the entire pattern match again as well as any
		 *	additional matches following a ; and any offsets
		 *	following a + or -.
		 */
		if (!subop) {
			while (*lastcp++ = *++globp);
			lastcp--;
			globp = vutmp;
		}
#endif ED1000
		d = peekc;

#ifndef	NLS16
fromsemi:
		ungetchar(0);
#else
		/*
		 * It is necessary to save and clesr not only peekc
		 * but also secondchar.
		 */
		c = secondchar;
fromsemi:
		clrpeekc();
#endif

		fixech();
		CATCH
#ifndef CBREAK
			/*
			 * Lose typeahead (ick).
			 */
			vcook();
#endif
			addr = address(cursor);
#ifndef CBREAK
			vraw();
#endif
		ONERR
#ifndef CBREAK
			vraw();
#endif
slerr:
			globp = oglobp;
			dot = odot;
			cursor = ocurs;
			ungetchar(d);

#ifdef	NLS16
			/*
			 * It is necessary to restore secondchar.
			 */
			secondchar = c;
#endif

			splitw = 0;
			vclean();
			vjumpto(dot, ocurs, 0);
			return;
		ENDCATCH
		if (globp == 0)
			globp = "";
		else if (peekc)
			--globp;
		if (*globp == ';') {
			/* /foo/;/bar/ */
			globp++;
			dot = addr;
			cursor = loc1;
			goto fromsemi;
		}
		dot = odot;
		ungetchar(d);

#ifdef	NLS16
		/*
		 * It is necessary to restore secondchar.
		 */
		secondchar = c;
#endif

		c = 0;
		if (*globp == 'z')
			globp++, c = '\n';
		if (any(*globp, "^+-."))
			c = *globp++;
		i = 0;

#ifndef NONLS8 /* Character set features */
		while ((*globp >= IS_MACRO_LOW_BOUND) && isdigit(*globp & TRIM))
#else NONLS8
		while ((*globp >= IS_MACRO_LOW_BOUND) && isdigit(*globp))
#endif NONLS8

			i = i * 10 + *globp++ - '0';
		if (any(*globp, "^+-."))
			c = *globp++;
		if (*globp) {
			/* random junk after the pattern */
			beep();
			goto slerr;
		}
		globp = oglobp;
		splitw = 0;
		vmoving = 0;
		wcursor = loc1;
		if (i != 0)
			vsetsiz(i);
		if (opf == vmove) {
			if (state == ONEOPEN || state == HARDOPEN)
				outline = destline = WBOT;
			if (addr != dot || loc1 != cursor)
				markDOT();
			if (loc1 > linebuf && *loc1 == 0)

#ifndef	NLS16
				loc1--;
#else
				PST_DEC(loc1);
#endif

			if (c)
				vjumpto(addr, loc1, c);
			else {
				vmoving = 0;
				if (loc1) {
					vmoving++;
					vmovcol = column(loc1);
				}
				getDOT();
				if (state == CRTOPEN && addr != dot)
					vup1();
				vupdown(addr - dot, NOCHAR);
			}
			return;
		}
		lastcp[-1] = 'n';
		getDOT();
		wdot = addr;
		break;
	}
	/*
	 * Apply.
	 */
	if (vreg && wdot == 0)
		wdot = dot;
	(*opf)(c);
	wdot = NOLINE;
}

/*
 * Find single character c, in direction dir from cursor.
 */

find(c)

#ifndef	NLS16
	char c;
#else
	int c;
#endif

{

	for(;;) {
		if (edge())
			return (0);

#ifndef	NLS16
		wcursor += dir;
		if (*wcursor == c)
#else
		wcursor = NADVANCE(wcursor, dir);
		if (_CHARAT(wcursor) == c)	/* character comparison */
#endif

			return (1);
	}
}

/*
 * Do a word motion with operator op, and cnt more words
 * to go after this.
 */
word(op, cnt)
	register int (*op)();
	int cnt;
{
	register int which;
	register CHAR *iwc;
	register line *iwdot = wdot;
	register int not_last_char = 1;

	if (dir == 1) {
		iwc = wcursor;
		which = wordch(wcursor);
		while (wordof(which, wcursor)) {

#ifndef	NLS16
			if (cnt == 1 && op != vmove && wcursor[1] == 0) {
				wcursor++;
#else
			if (cnt == 1 && op != vmove && *ADVANCE(wcursor) == 0) {
				PST_INC(wcursor);
#endif

				break;
			}
			if (!lnext())
				return (0);
			if (wcursor == linebuf)
				break;
		}
		/* Unless last segment of a change skip blanks */
		if (op != vchange || cnt > 1)
			while (not_last_char && !margin() && blank()) {
				/*
				** End-of-line acts as a break for the last
				** segment except when doing a move.
				** If can't move (as returned by lnext into
				** not_last_char), current blank character must be
				** the last one in the buffer so we must terminate
				** this loop (see while statement above).
				*/
				if (op == vmove || cnt > 1)
					not_last_char = lnext();
				else
#ifndef	NLS16
					wcursor++;
#else
					PST_INC(wcursor);
#endif
			}
		else
			if (wcursor == iwc && iwdot == wdot && *iwc)

#ifndef	NLS16
				wcursor++;
#else
				PST_INC(wcursor);
#endif

		if (op == vmove && margin())

#ifndef	NLS16
			wcursor--;
#else
			PST_DEC(wcursor);
#endif

	} else {
		if (!lnext())
			return (0);
		while (blank())
			if (!lnext())
				return (0);
		if (!margin()) {
			which = wordch(wcursor);
			while (!margin() && wordof(which, wcursor))

#ifndef	NLS16
				wcursor--;
#else
				PST_DEC(wcursor);
#endif

			/*
			** this was misplaced before (i.e., 'which' was
			** not set which caused problems backing up past
			** blank lines)
			*/
			if (wcursor < linebuf || !wordof(which, wcursor))

#ifndef	NLS16
				wcursor++;
#else
				PST_INC(wcursor);
#endif
		}

	}
	return (1);
}

/*
 * To end of word, with operator op and cnt more motions
 * remaining after this.
 */
eend(op)
	register int (*op)();
{
	register int which;

	if (!lnext())
		return;
	while (blank())
		if (!lnext())
			return;
	which = wordch(wcursor);
	while (wordof(which, wcursor)) {

#ifndef	NLS16
		if (wcursor[1] == 0) {
			wcursor++;
#else
		if (*ADVANCE(wcursor) == 0) {
			PST_INC(wcursor);
#endif

			break;
		}
		if (!lnext())
			return;
	}
	if (op != vchange && op != vdelete && wcursor > linebuf)

#ifndef	NLS16
		wcursor--;
#else
		PST_DEC(wcursor);
#endif

}

/*
 * Wordof tells whether the character at *wc is in a word of
 * kind which (blank/nonblank words are 0, conservative words 1).
 */
wordof(which, wc)
	char which;
	register CHAR *wc;
{


#ifndef NONLS8 /* Character set features */
	if (_isspace(*wc & TRIM))
#else NONLS8
	if (isspace(*wc))
#endif NONLS8

		return (0);
	return (!wdkind || wordch(wc) == which);
}

/*
 * Wordch tells whether character at *wc is a word character
 * i.e. an alfa, digit, or underscore.
 */
wordch(wc)
	CHAR *wc;
{
	register int c;

	c = wc[0];

#ifdef	NLS16
	/*	wordch() tells the type of characters.
	**		<type>
	**		  1	:	alphabet, digit or underscore
	**		  2	:	16-bit character ( kanji )
	**		  0	:	others ( non-word character )
	*/	
	if (IS_KANJI(c))
		return(2);
#endif

#ifndef NONLS8 /* Character set features */
	/* Since the behavioral correction of isalpha() in 9.0, 
	 * (ie. it returns false for all 8-bit katakana and 16-kanji)
	 * The following code is modified to avoid the use of isalpha().
	 * 5/8/92
	 *
	 * return (((c >= IS_MACRO_LOW_BOUND) && (isalpha(c & TRIM) 
	 *  || isdigit(c & TRIM)) )|| c == '_');
	 */
	/* DTS: DSDe407280
	 * the following change is not bug proof. For example, it returns
	 * true when c == 0!
	 *
	 *  return (((c >= IS_MACRO_LOW_BOUND) && !isspace(c & TRIM) 
	 *   && !ispunct(c & TRIM)) || c == '_');
	 */
	return (c == '_' || ((c >= IS_MACRO_LOW_BOUND) &&
	        isgraph(c & TRIM) && !ispunct(c & TRIM)));
#else NONLS8
	/*
	return (((c >= IS_MACRO_LOW_BOUND) && (isalpha(c) || isdigit(c))) || c == '_');
	*/
	return (c == '_' || ((c >= IS_MACRO_LOW_BOUND) &&
	        isgraph(c) && !ispunct(c)));
#endif NONLS8

}

/*
 * Edge tells when we hit the last character in the current line.
 */
edge()
{

	if (linebuf[0] == 0)
		return (1);
	if (dir == 1)

#ifndef	NLS16
		return (wcursor[1] == 0);
#else
		return (*ADVANCE(wcursor) == 0);
#endif

	else
		return (wcursor == linebuf);
}

/*
 * Margin tells us when we have fallen off the end of the line.
 */
margin()
{

	return (wcursor < linebuf || wcursor[0] == 0);
}
