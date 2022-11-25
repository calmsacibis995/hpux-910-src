/* @(#) $Revision: 70.1 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifndef NONLS8 /* User messages */
# define	NL_SETN	16	/* set number */
# include	<msgbuf.h>
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Input routines for open/visual.
 * We handle upper case only terminals in visual and reading from the
 * echo area here as well as notification on large changes
 * which appears in the echo area.
 */

/*
 * Return the key.
 */
ungetkey(c)
	int c;		/* mjm: char --> int */
{

	if (Peekkey != ATTN)
		Peekkey = c;
}

/*
 * Return a keystroke, but never a ^@.
 */
getkey()
{
	register int c;		/* mjm: char --> int */

	do {
		c = getbr();
		if (c==0)
			beep();
	} while (c == 0);
	return (c);
}

/*
 * Tell whether next keystroke would be a ^@.
 */
peekbr()
{

	Peekkey = getbr();
	return (Peekkey == 0);
}

short	precbksl;

/*
 * Get a keystroke, including a ^@.
 * If an key was returned with ungetkey, that
 * comes back first.  Next comes unread input (e.g.
 * from repeating commands with .), and finally new
 * keystrokes.
 *
 * The hard work here is in mapping of \ escaped
 * characters on upper case only terminals.
 */

getbr()
{
	CHAR ch;
	register int c, d;
	register char *colp;
	int cnt;
	static char Peek2key;
	extern short slevel, ttyindes;

getATTN:
	if (Peekkey) {
		c = Peekkey;
		Peekkey = 0;
		return (c);
	}
	if (Peek2key) {
		c = Peek2key;
		Peek2key = 0;
		return (c);
	}
#ifdef ED1000
	if (vglobp) {
		if (*vglobp)
			return (lastvgk = *vglobp++);
		lastvgk = 0;
		return (ESCAPE);
	}
#endif ED1000
	if (vmacp) {
		if (*vmacp)

#ifndef NONLS8 /* 8bit integrity */
#ifndef	NLS16
			return(*vmacp++ & TRIM);
#else
			return(*vmacp++ & (QUOTE | TRIM | FIRST | SECOND));
#endif
#else NONLS8
			return(*vmacp++);
#endif NONLS8

		/* End of a macro or set of nested macros */
		vmacp = 0;
		if (inopen == -1)	/* don't screw up undo for esc esc */
			vundkind = VMANY;
		inopen = 1;	/* restore old setting now that macro done */
		vch_mac = VC_NOTINMAC;
	}
	if (vglobp) {
		if (*vglobp)

#ifndef NONLS8 /* 8bit integrity */
#ifndef	NLS16
			return (lastvgk = *vglobp++ & TRIM);
#else
			return (lastvgk = *vglobp++ & (QUOTE | TRIM | FIRST | SECOND));
#endif
#else NONLS8
			return (lastvgk = *vglobp++);
#endif NONLS8

		lastvgk = 0;
		return (ESCAPE);
	}
	flusho();
again:

#ifndef	NLS16
	if ((c=read(slevel == 0 ? 0 : ttyindes, &ch, 1)) != 1) {
#else
	if ((c=kread(slevel == 0 ? 0 : ttyindes, &ch, 1)) != 1) {
#endif

		if (errno == EINTR)
			goto getATTN;
		error((nl_msg(1, "Input read error")));
	}

#ifndef	NLS16
	c = ch & TRIM;
#else
	c = ch & (TRIM | FIRST | SECOND);
#endif

	if (beehive_glitch && slevel==0 && c == ESCAPE) {
		if (read(0, &Peek2key, 1) != 1)
			goto getATTN;
		Peek2key &= TRIM;
		switch (Peek2key) {
		case 'C':	/* SPOW mode sometimes sends \EC for space */
			c = ' ';
			Peek2key = 0;
			break;
		case 'q':	/* f2 -> ^C */
			c = CTRL(c);
			Peek2key = 0;
			break;
		case 'p':	/* f1 -> esc */
			Peek2key = 0;
			break;
		}
	}
	/*
	 * The algorithm here is that of the UNIX kernel.
	 * See the description in the programmers manual.
	 */

#ifndef	NLS16
	if (UPPERCASE) {
#else
	if (IS_ANK(c) && UPPERCASE) {
#endif


#ifndef NONLS8 /* Character set features */
		if ((c >= IS_MACRO_LOW_BOUND) && isupper(c & TRIM))
			c = tolower(c & TRIM);
#else NONLS8
		if ((c >= IS_MACRO_LOW_BOUND) && isupper(c))
			c = tolower(c);
#endif NONLS8

		if (c == '\\') {
			if (precbksl < 2)
				precbksl++;
			if (precbksl == 1)
				goto again;
		} else if (precbksl) {
			d = 0;

#ifndef NONLS8 /* 8bit integrity */
			if ((c >= IS_MACRO_LOW_BOUND) && islower(c & TRIM))
				d = toupper(c & TRIM);
#else NONLS8
			if ((c >= IS_MACRO_LOW_BOUND) && islower(c))
				d = toupper(c);
#endif NONLS8

			else {
				colp = "({)}!|^~'~";
				while (d = *colp++)
					if (d == c) {
						d = *colp++;
						break;
					} else
						colp++;
			}
			if (precbksl == 2) {
				if (!d) {
					Peekkey = c;
					precbksl = 0;
					c = '\\';
				}
			} else if (d)
				c = d;
			else {
				Peekkey = c;
				precbksl = 0;
				c = '\\';
			}
		}
		if (c != '\\')
			precbksl = 0;
	}
#ifdef TRACE
	if (trace) {
		if (!techoin) {
			tfixnl();
			techoin = 1;
			fprintf(trace, "*** Input: ");
		}
		tracec(c);
	}
#endif
	lastvgk = 0;
	return (c);
}

#ifdef	NLS16
kread(fildes, buf, nbyte)		/* read() and put attribute */
int		fildes;			/* file descriptor */
CHAR		*buf;			/* short type buffer pointer */
unsigned	nbyte;			/* the number of bytes to be read */
{
	unsigned	count = 0;	/* to be returned */
	unsigned char	c1, c2;		/* dummy character */
	static CHAR 	secondkey;	/* Peek ahead Kanji 2nd byte character */

	while (nbyte--)	{
		if (secondkey)	{
			*buf++ = secondkey;
			secondkey = 0;
			count++;
			continue;
		}
		if (read(fildes, &c1, 1) <= 0)
			break;
		count++;
		if (c1 != EOF && FIRSTof2(c1 & TRIM))	{
			if (read(fildes, &c2, 1) <= 0)
				break;
			if (c2 != EOF && SECONDof2(c2 & TRIM))	{
				*buf++ = (c1 & TRIM) | FIRST;
				secondkey = (c2 & TRIM) | SECOND;
			} else	{
				*buf++ = c1 & TRIM;
				secondkey = c2 & TRIM;
			}
		} else
			*buf++ = c1 & TRIM;
	}
	return(count);
}
#endif

/*
 * Get a key, but if a delete, quit or attention
 * is typed return 0 so we will abort a partial command.
 */
getesc()
{
	register int c;

#if defined NLS || defined NLS16
	c = RL_KEY(getkey());
#else
	c = getkey();
#endif
	switch (c) {

	case CTRL(v):
	case CTRL(q):
#if defined NLS || defined NLS16
		c = RL_KEY(getkey());
#else
		c = getkey();
#endif
		return (c);

	case ATTN:
	case QUIT:
		ungetkey(c);
		return (0);

	case ESCAPE:
		return (0);
	}
	return (c);
}

/*
 * Peek at the next keystroke.
 */
peekkey()
{

	Peekkey = getkey();
	return (Peekkey);
}

/*
 * Read a line from the echo area, with single character prompt c.
 * A return value of 1 means the user blewit or blewit away.
 */

readecho(c)
	char c;
{
	register CHAR *sc = cursor;
	register int (*OP)();
	bool waste;
	register int OPeek;

	if (WBOT == WECHO)
		vclean();
	else
		vclrech(0);
	splitw++;
	vgoto(WECHO, 0);
	putchar(c);
#if defined NLS || defined NLS16
	if (right_to_left) {
		if (rl_mode == NL_NONLATIN && c == ':')
			tputs(latin_lang,1,putch);
		RL_OKEY
		tputs(unlock_keys, 1, putch);
		flusho();
		opp_back = 0;
		opp_fix = 1;
	}
#endif
	vclreol();
	vgoto(WECHO, 1);
	cursor = linebuf; linebuf[0] = 0; genbuf[0] = c;
	if (peekbr()) {
		if (!INS[0] || (INS[0] & (QUOTE|TRIM)) == OVERBUF)
			goto blewit;
		vglobp = INS;
	}
	OP = Pline; Pline = normline;
	ignore(vgetline(0, genbuf + 1, &waste, c));
#if defined NLS || defined NLS16
	if (right_to_left) {
		flip_echo(genbuf);
		if (rl_mode == NL_NONLATIN && c == ':') {
			if (opp_terminate) {
				base_lang();
			} else {
				tputs(tparm(cursor_address,destline-1,0),0,putch);
				base_mode();
			}
		}
		RL_OSCREEN
		opp_insert = 0;
		opp_back = 0;
		opp_fix = 1;
	}
#endif
	if (Outchar == termchar)
		putchar('\n');
	vscrap();
	Pline = OP;
	if (Peekkey != ATTN && Peekkey != QUIT && Peekkey != CTRL(h)) {
		cursor = sc;
		vclreol();
		return (0);
	}
blewit:
#if defined NLS || defined NLS16
	if (right_to_left) {
		flip_echo(genbuf);
		if (rl_mode == NL_NONLATIN && c == ':') {
			if (opp_terminate) {
				base_lang();
			} else {
				tputs(tparm(cursor_address,destline-1,0),0,putch);
				base_mode();
			}
		}
		RL_OSCREEN
		opp_insert = 0;
		opp_back = 0;
		opp_fix = 1;
	}
#endif
	OPeek = Peekkey==CTRL(h) ? 0 : Peekkey; Peekkey = 0;
	splitw = 0;
	vclean();
	vshow(dot, NOLINE);
	vnline(sc);
	Peekkey = OPeek;
	return (1);
}

/*
 * A complete command has been defined for
 * the purposes of repeat, so copy it from
 * the working to the previous command buffer.
 */
setLAST()
{

	if (vglobp || vmacp)
		return;
	lastreg = vreg;
	lasthad = Xhadcnt;
	lastcnt = Xcnt;
	*lastcp = 0;

#ifndef	NLS16
	CP(lastcmd, workcmd);
#else
	STRCPY(lastcmd, workcmd);
#endif

}

/*
 * Gather up some more text from an insert.
 * If the insertion buffer oveflows, then destroy
 * the repeatability of the insert.
 */
addtext(cp)
	CHAR *cp;
{

	if (vglobp)
		return;
	addto(INS, cp);
	if ((INS[0] & (QUOTE|TRIM)) == OVERBUF)
		lastcmd[0] = 0;
}

setDEL()
{

	setBUF(DEL);
}

/*
 * Put text from cursor upto wcursor in BUF.
 */
setBUF(BUF)
	register CHAR *BUF;
{
	register int c;
	register CHAR *wp = wcursor;

	c = *wp;
	*wp = 0;
	BUF[0] = 0;
	addto(BUF, cursor);
	*wp = c;
}

addto(buf, str)
	register CHAR *buf, *str;
{

	if ((buf[0] & (QUOTE|TRIM)) == OVERBUF)
		return;

#ifndef	NLS16
	if (strlen(buf) + strlen(str) + 1 >= VBSIZE) {
#else
	if (STRLEN(buf) + STRLEN(str) + 1 >= VBSIZE) {
#endif

		buf[0] = OVERBUF;
		return;
	}

#ifndef	NLS16
	ignore(strcat(buf, str));
#else
	ignore(STRCAT(buf, str));
#endif

}

/*
 * Note a change affecting a lot of lines, or non-visible
 * lines.  If the parameter must is set, then we only want
 * to do this for open modes now; return and save for later
 * notification in visual.
 */
noteit(must)
	bool must;
{
#if defined NLS16 && defined EUC
	int col_vtub ;
#endif
	register int sdl = destline, sdc = destcol;

	if (notecnt < 2 || !must && state == VISUAL)
		return (0);
	splitw++;
	if (WBOT == WECHO)
		vmoveitup(1, 1);
	vigoto(WECHO, 0);

#if defined NLS || defined NLS16
	RL_OKEY
#endif
#ifndef NONLS8 /* User messages */
	if (notesgn[0] == 'm')
		printf((nl_msg(2, "%d more line")), notecnt);
	else if (notesgn[0] == 'f')
		printf((nl_msg(3, "%d fewer line")), notecnt);
	else
		printf((nl_msg(4, "%d line")), notecnt);
#else NONLS8
	printf("%d %sline", notecnt, notesgn);
#endif NONLS8

	if (notecnt > 1)
		putchar('s');
	if (*notenam) {
		printf(" %s", notenam);
		if (*(strend(notenam) - 1) != 'e')
			putchar('e');
		putchar('d');
	}
#if defined NLS || defined NLS16
	RL_OSCREEN
#endif

	/*
	 * The following is needed to force the echo line to be cleared
	 * to the end of the line.  The editor thinks it is writing to
	 * an already clear line but on terminals with more lines in
	 * memory than are displayed on the screen, the line may have
	 * garbage from some previous scroll operation.  It would be
	 * nice to either make this conditional on the terminal
	 * capability (of memory lines > screen lines) or do something
	 * intelligent during the scroll operations.
	 */
#if defined NLS16 && defined EUC
	col_vtub = vcolumn( vtube[destline], destcol ) ;
	vtube[destline][col_vtub] = 'x';
#else
	vtube[destline][destcol] = 'x';
#endif

	vclreol();
	notecnt = 0;
	if (state != VISUAL)
		vcnt = vcline = 0;
	splitw = 0;
	if (state == ONEOPEN || state == CRTOPEN)
		vup1();
	destline = sdl; destcol = sdc;
	return (1);
}

/*
 * Rrrrringgggggg.
 * If possible, flash screen.
 */
beep()
{

	if (flash_screen && value(FLASH))
		vputp(flash_screen, 0);
	else if (bell)
		vputp(bell, 0);
}

/*
 * Map the command input character c,
 * for keypads and labelled keys which do cursor
 * motions.  I.e. on an adm3a we might map ^K to ^P.
 * DM1520 for example has a lot of mappable characters.
 */

map(c,maps)
	register int c;
	register struct maps *maps;
{
	register int d;
	register int d_start;
	register char *p;
	register CHAR *q;
	CHAR b[20];	/* Assumption: no keypad sends string longer than 10 */
	/*
	 * was 10. Actually, 11 data units are necessary to keep 10 keypad
	 * strings. This modification is for safeguard.
	 */

	/*
	 * Mapping for special keys on the terminal only.
	 * BUG: if there's a long sequence and it matches
	 * some chars and then misses, we lose some chars.
	 *
	 * For this to work, some conditions must be met.
	 * 1) Keypad sends SHORT (2 or 3 char) strings
	 * 2) All strings sent are same length & similar
	 * 3) The user is unlikely to type the first few chars of
	 *    one of these strings very fast.
	 * Note: some code has been fixed up since the above was laid out,
	 * so conditions 1 & 2 are probably not required anymore.
	 * However, this hasn't been tested with any first char
	 * that means anything else except escape.
	 */
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"map(%c): ",c);
#endif
	/*
	 * If c==0, the char came from getesc typing escape.  Pass it through
	 * unchanged.  0 messes up the following code anyway.
	 */
	if (c==0)
		return(0);

	b[0] = c;
	b[1] = 0;
	d_start = (maps==arrows && !value(KEYBOARDEDIT)) ? arrows_start :
		  (maps==immacs && !value(KEYBOARDEDIT_I)) ? immacs_start :
		  0;
	for (d=d_start; maps[d].mapto; d++) {
#ifdef MDEBUG
		if (trace)
			fprintf(trace,"\ntry '%s', ",maps[d].cap);
#endif
		if (p = maps[d].cap) {
			for (q=b; *p; p++, q++) {
#ifdef MDEBUG
				if (trace)
					fprintf(trace,"q->b[%d], ",q-b);
#endif
				if (*q==0) {
					/*
					 * Is there another char waiting?
					 *
					 * This test is oversimplified, but
					 * should work mostly. It handles the
					 * case where we get an ESCAPE that
					 * wasn't part of a keypad string.
					 */
					if ((c=='#' ? peekkey() : fastpeekkey()) == 0) {
#ifdef MDEBUG
						if (trace)
							fprintf(trace,"fpk=0: will return '%c'",c);
#endif
						/*
						 * Nothing waiting.  Push back
						 * what we peeked at & return
						 * failure (c).
						 *
						 * We want to be able to undo
						 * commands, but it's nonsense
						 * to undo part of an insertion
						 * so if in input mode don't.
						 */
#ifdef MDEBUG
						if (trace)
							fprintf(trace, "Call macpush, b %d %d %d\n", b[0], b[1], b[2]);
#endif

#ifndef	NLS16
						macpush(&b[1],maps == arrows);
#else
						MACPUSH(&b[1],maps == arrows);
#endif

#ifdef MDEBUG
						if (trace)
							fprintf(trace, "return %d\n", c);	
#endif
						return(c);
					}
					*q = getkey();
					q[1] = 0;
				}

#ifndef	NLS16
				if (*p != *q)
#else
				if ((*p & TRIM) != (*q & TRIM))
#endif

					goto contin;
			}
			macpush(maps[d].mapto,maps == arrows);
			c = getkey();
#ifdef MDEBUG
			if (trace)
				fprintf(trace,"Success: push(%s), return %c",maps[d].mapto, c);
#endif
			return(c);	/* first char of map string */
			contin:;
		}
	}
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"Fail: push(%s), return %c", &b[1], c);
#endif

#ifndef	NLS16
	macpush(&b[1],0);
#else
	MACPUSH(&b[1],0);
#endif

	return(c);
}

/*
 * Push st onto the front of vmacp. This is tricky because we have to
 * worry about where vmacp was previously pointing. We also have to
 * check for overflow (which is typically from a recursive macro)
 * Finally we have to set a flag so the whole thing can be undone.
 * canundo is 1 iff we want to be able to undo the macro.  This
 * is false for, for example, pushing back lookahead from fastpeekkey(),
 * since otherwise two fast escapes can clobber our undo.
 */

macpush(st, canundo)
char *st;
int canundo;

#ifdef	NLS16
{
	CHAR temp[BUFSIZ];

	char_TO_CHAR(st, temp);
	MACPUSH(temp, canundo);
}

MACPUSH(st, canundo)
CHAR *st;
int canundo;
#endif

{
	CHAR tmpbuf[BUFSIZ];

	if (st==0 || *st==0)
		return;
#ifdef MDEBUG
	if (trace)
		fprintf(trace, "macpush(%s), canundo=%d\n",st,canundo);
#endif

#ifndef	NLS16
	if ((vmacp ? strlen(vmacp) : 0) + strlen(st) > BUFSIZ)
#else
	if ((vmacp ? STRLEN(vmacp) : 0) + STRLEN(st) > BUFSIZ)
#endif


#ifndef NONLS8 /* User messages */
		error((nl_msg(5, "Macro too long|Macro too long  - maybe recursive?")));
#else NONLS8
		error("Macro too long@ - maybe recursive?");
#endif NONLS8

	if (vmacp) {

#ifndef	NLS16
		strcpy(tmpbuf, vmacp);
#else
		STRCPY(tmpbuf, vmacp);
#endif

		if (!FIXUNDO)
			canundo = 0;	/* can't undo inside a macro anyway */
	}

#ifndef	NLS16
	strcpy(vmacbuf, st);
#else
	STRCPY(vmacbuf, st);
#endif

	if (vmacp)

#ifndef	NLS16
		strcat(vmacbuf, tmpbuf);
#else
		STRCAT(vmacbuf, tmpbuf);
#endif
	vmacp = vmacbuf;
	/* arrange to be able to undo the whole macro */
	if (canundo) {
#ifdef notdef
		otchng = tchng;
		vsave();
		saveall();
		inopen = -1;	/* no need to save since it had to be 1 or -1 before */
		/* UCSqm00044: move back following line inside #ifdef */
		/* since rev. 66.2, to fix 'u' clear screen problem */
		vundkind = VMANY;
#endif
		vch_mac = VC_NOCHANGE;
	}
}

#ifdef UNDOTRACE
visdump(s)
char *s;
{
	register int i;

	if (!trace) return;

	fprintf(trace, "\n%s: basWTOP=%d, basWLINES=%d, WTOP=%d, WBOT=%d, WLINES=%d, WCOLS=%d, WECHO=%d\n",
		s, basWTOP, basWLINES, WTOP, WBOT, WLINES, WCOLS, WECHO);
	fprintf(trace, "   vcnt=%d, vcline=%d, cursor=%d, wcursor=%d, wdot=%d\n",
		vcnt, vcline, cursor-linebuf, wcursor-linebuf, wdot-zero);
	for (i=0; i<TUBELINES; i++)
		if (vtube[i] && *vtube[i])
			fprintf(trace, "%d: '%s'\n", i, vtube[i]);
	tvliny();
}

vudump(s)
char *s;
{
	register line *p;
	char savelb[1024];

	if (!trace) return;

	fprintf(trace, "\n%s: undkind=%d, vundkind=%d, unddel=%d, undap1=%d, undap2=%d,\n",
		s, undkind, vundkind, lineno(unddel), lineno(undap1), lineno(undap2));
	fprintf(trace, "  undadot=%d, dot=%d, dol=%d, unddol=%d, truedol=%d\n",
		lineno(undadot), lineno(dot), lineno(dol), lineno(unddol), lineno(truedol));
	fprintf(trace, "  [\n");
	CP(savelb, linebuf);
	fprintf(trace, "linebuf = '%s'\n", linebuf);
	for (p=zero+1; p<=truedol; p++) {
		fprintf(trace, "%o ", *p);
		getline(*p);
		fprintf(trace, "'%s'\n", linebuf);
	}
	fprintf(trace, "]\n");
	CP(linebuf, savelb);
}
#endif

/*
 * Get a count from the keyed input stream.
 * A zero count is indistinguishable from no count.
 */
vgetcnt()
{
	register int c, cnt;

	cnt = 0;
	for (;;) {
#if defined NLS || defined NLS16
		c = RL_KEY(getkey());
#else
		c = getkey();
#endif


#ifndef NONLS8 /* Character set features */
		if ((c < IS_MACRO_LOW_BOUND) || !isdigit(c & TRIM))
#else NONLS8
		if ((c < IS_MACRO_LOW_BOUND) || !isdigit(c))
#endif NONLS8

			break;
		cnt *= 10, cnt += c - '0';
	}
	ungetkey(c);
	Xhadcnt = 1;
	Xcnt = cnt;
	return(cnt);
}

/*
 * fastpeekkey is just like peekkey but insists the character come in
 * fast (within 1 second). This will succeed if it is the 2nd char of
 * a machine generated sequence (such as a function pad from an escape
 * flavor terminal) but fail for a human hitting escape then waiting.
 */
fastpeekkey()
{
	int trapalarm();
	register int c;
	int (*oldint)();

	/*
	 * If the user has set notimeout, we wait forever for a key.
	 * If we are in a macro we do too, but since it's already
	 * buffered internally it will return immediately.
	 * In other cases we force this to die in 1 second.
	 * This is pretty reliable (VMUNIX rounds it to .5 - 1.5 secs,
	 * but UNIX truncates it to 0 - 1 secs) but due to system delays
	 * there are times when arrow keys or very fast typing get counted
	 * as separate.  notimeout is provided for people who dislike such
	 * nondeterminism.
	 */
	oldint = (int(*)())signal(SIGINT, SIG_IGN);
	if (value(TIMEOUT) && inopen >= 0) {
		signal(SIGALRM, trapalarm);
		alarm(1);
	}
	CATCH
		oldint = (int(*)())signal(SIGINT, SIG_IGN);
		if (value(TIMEOUT) && inopen >= 0) {
			signal(SIGALRM, trapalarm);
			setalarm();
		}
		c = peekkey();
		cancelalarm();
	ONERR
		c = 0;
	ENDCATCH
	signal(SIGINT, oldint);
	/* Should have an alternative method based on select for 4.2BSD */
	return(c);
}

static int ftfd;
struct requestbuf {
	short time;
	short signo;
};

/*
 * Arrange for SIGALRM to come in shortly, so we don't
 * hang very long if the user didn't type anything.  There are
 * various ways to do this on different systems.
 */
setalarm()
{
#ifndef	hpux
#ifdef FTIOCSET
	char ftname[20];
	struct requestbuf rb;

	/*
	 * Use nonstandard "fast timer" to get better than
	 * one second resolution.  We must wait at least
	 * 1/15th of a second because some keypads don't
	 * transmit faster than this.
	 */

	/* Open ft psuedo-device - we need our own copy. */
	if (ftfd == 0) {
		strcpy(ftname, "/dev/ft0");
		while (ftfd <= 0 && ftname[7] <= '~') {
			ftfd = open(ftname, 0);
			if (ftfd <= 0)
				ftname[7] ++;
		}
	}
	if (ftfd <= 0) {	/* Couldn't open a /dev/ft? */
		alarm(1);
	} else {
		rb.time = 6;	/* 6 ticks = 100 ms > 67 ms. */
		rb.signo = SIGALRM;
		ioctl(ftfd, FTIOCSET, &rb);
	}
#else
	/*
	 * No special capabilities, so we use alarm, with 1 sec. resolution.
	 */
	alarm(1);
#endif
#else	hpux
#include <time.h>
#ifndef hpe
	struct itimerval itimer;

	itimer.it_interval.tv_sec=0;
	itimer.it_interval.tv_usec=0;
	itimer.it_value.tv_sec=0;
	itimer.it_value.tv_usec=1000*value(TIMEOUTLEN);	/* convert msec to usec */

	if (setitimer(ITIMER_REAL, &itimer, (struct itimerval *)0))
		alarm(1);
#else
	trapalarm();
#endif hpe
#endif	hpux
}

/*
 * Get rid of any impending incoming SIGALRM.
 */
cancelalarm()
{
	struct requestbuf rb;
#ifdef FTIOCSET
	if (ftfd > 0) {
		rb.time = 0;
		rb.signo = SIGALRM;
		ioctl(ftfd, FTIOCCANCEL, &rb);
	}
#endif
	alarm(0);	/* Have to do this whether or not FTIOCSET */
}

trapalarm() {
	alarm(0);
	if (vcatch)
		longjmp(vreslab,1);
}
