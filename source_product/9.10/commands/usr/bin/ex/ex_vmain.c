/* @(#) $Revision: 70.1 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * This is the main routine for visual.
 * We here decode the count and possible named buffer specification
 * preceding a command and interpret a few of the commands.
 * Commands which involve a target (i.e. an operator) are decoded
 * in the routine operate in ex_voperate.c.
 */

#define	forbid(a)	{ if (a) goto fonfon; }

#ifndef NONLS8 /* User messages */
# define	NL_SETN	17	/* set number */
# include	<msgbuf.h>
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

vmain()
{
	register int c, cnt, i;

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
	short esave[VBUFCOLS];
#else
	short esave[TUBECOLS];
#endif
#else NONLS8
	char esave[TUBECOLS];
#endif NONLS8

	char *oglobp;
	short d;
	line *addr;
	int ind, nlput;
	int shouldpo = 0;

	/* 
	 * volatile tells the compiler to not optimize accesses to this 
	 * variable, as it may be modified in ways that the compiler
	 * cannot see. (FSDlj08162)
	 */
	volatile int onumber, olist, (*OPline)(), (*OPutchar)();

	CHAR	*ovmacp = 0;

	vch_mac = VC_NOTINMAC;

	no_map_ = 0;	/* initialize to false */

#if defined(TEPE) && defined(NLS16)
	tepe_ok = 1;	/* initialize to true */
#endif

#ifdef SIGWINCH
	got_winch = 0;
	got_winch_insert = 0;
#endif SIGWINCH
	/*
	 * If we started as a vi command (on the command line)
	 * then go process initial commands (recover, next or tag).
	 */
	if (initev) {
		oglobp = globp;
		globp = initev;
		hadcnt = cnt = 0;
		i = tchng;
		addr = dot;
		goto doinit;
	}

	vshowmode("");		/* As a precaution */
	/*
	 * NB:
	 *
	 * The current line is always in the line buffer linebuf,
	 * and the cursor at the position cursor.  You should do
	 * a vsave() before moving off the line to make sure the disk
	 * copy is updated if it has changed, and a getDOT() to get
	 * the line back if you mung linebuf.  The motion
	 * routines in ex_vwind.c handle most of this.
	 */
	for (;;) {
#ifdef	TEPE
		/* hp3000 tepe testing prompt */
#ifdef NLS16
		/*
		**  If a tepe prompt hasn't already been issued, then
		**  issue one.  Otherwise reset the flag.  (Companion
		**  code is in ex_voper.c)
		*/
		if (tepe_ok)
			vputc('\021');
		else
			tepe_ok++;
#else NLS16
		vputc('\021');
#endif NLS16
#endif
		/*
		 * Decode a visual command.
		 * First sync the temp file if there has been a reasonable
		 * amount of change.  Clear state for decoding of next
		 * command.
		 */
		TSYNC();
		vglobp = 0;
		vreg = 0;
		hold = 0;
		seenprompt = 1;
		wcursor = 0;
		Xhadcnt = hadcnt = 0;
		Xcnt = cnt = 1;
		splitw = 0;
		if (i = holdupd) {
			if (state == VISUAL)
				ignore(peekkey());
			holdupd = 0;
/*
			if (LINE(0) < ZERO) {
				vclear();
				vcnt = 0;
				i = 3;
			}
*/
			if (state != VISUAL) {
				vcnt = 0;
				vsave();
				vrepaint(cursor);
			} else if (i == 3)
				vredraw(WTOP);
			else
				vsync(WTOP);
			vfixcurs();
		}

		/*
		 * Gobble up counts and named buffer specifications.
		 */
		for (;;) {
looptop:
#ifdef MDEBUG
			if (trace)
				fprintf(trace, "pc=%c",peekkey());
#endif

#if defined NLS || defined NLS16
			if (((RL_KEY(peekkey()&TRIM) >= IS_MACRO_LOW_BOUND) 
			    && isdigit(RL_KEY(peekkey()&TRIM))) && RL_KEY(peekkey()) != '0' ){
#else
			if (((peekkey() >= IS_MACRO_LOW_BOUND) && isdigit(peekkey())) && peekkey() != '0') {
#endif

				hadcnt = 1;
				cnt = vgetcnt();
				forbid (cnt <= 0);
			}
#if defined NLS || defined NLS16
			if (RL_KEY(peekkey()) != '"')
				break;
			ignore(RL_KEY(getkey())), c = RL_KEY(getkey());
#else
			if (peekkey() != '"')
				break;
			ignore(getkey()), c = getkey();
#endif
			/*
			 * Buffer names be letters or digits.
			 * But not '0' as that is the source of
			 * an 'empty' named buffer spec in the routine
			 * kshift (see ex_temp.c).
			 */

#ifndef NONLS8 /* 8bit integrity */
			/*
			 * Must be ASCII letters as buffers have only been
			 * allocated for them, not all the 8 or 16-bit
			 * characters that might be alpha.
			 */
			forbid (c == '0' || (c < IS_MACRO_LOW_BOUND) 
			    || (!isalpha(c & TRIM) && !isdigit(c & TRIM)) || !isascii(c & TRIM));
#else NONLS8
			forbid (c == '0' || (c < IS_MACRO_LOW_BOUND) 
			    || (!isalpha(c) && !isdigit(c)));
#endif NONLS8

			vreg = c;
		}
reread:
		/*
		 * Come to reread from below after some macro expansions.
		 * The call to map allows use of function key pads
		 * by performing a terminal dependent mapping of inputs.
		 */
#ifdef MDEBUG
		if (trace)
			fprintf(trace,"pcb=%c,",peekkey());
#endif
#if defined NLS || defined NLS16
		op = RL_KEY(getkey());
#else
		op = getkey();
#endif
		maphopcnt = 0;
		do {
			/*
			 * Keep mapping the char as long as it changes.
			 * This allows for double mappings, e.g., q to #,
			 * #1 to something else.
			 */
#if defined NLS || defined NLS16
			c = op & TRIM;
#else
			c = op;
#endif
			op = map(c,arrows);
#ifdef MDEBUG
			if (trace)
				fprintf(trace,"pca=%c,",c);
#endif
			/*
			 * Maybe the mapped to char is a count. If so, we have
			 * to go back to the "for" to interpret it. Likewise
			 * for a buffer name.
			 */

#ifndef NONLS8 /* Character set features */
			if (((c >= IS_MACRO_LOW_BOUND) && 
			    isdigit(c & TRIM) && c!='0') || c == '"') {
#else NONLS8
			if (((c >= IS_MACRO_LOW_BOUND) && isdigit(c) &&
			    c!='0') || c == '"') {
#endif NONLS8

				ungetkey(c);
				goto looptop;
			}
			if (!value(REMAP)) {
				c = op;
				break;
			}
			if (++maphopcnt > 256)
				error((nl_msg(1, "Infinite macro loop")));
#if defined NLS || defined NLS16
		} while (c != (op&TRIM));
#else
		} while (c != op);
#endif

		/*
		 * Begin to build an image of this command for possible
		 * later repeat in the buffer workcmd.  It will be copied
		 * to lastcmd by the routine setLAST
		 * if/when completely specified.
		 */
		lastcp = workcmd;
		if (!vglobp)
			*lastcp++ = c;

		/*
		 * First level command decode.
		 */
#ifdef SIGTSTP
		if (c == olttyc.t_suspc) {
			forbid(dosusp == 0);
# ifdef NTTYDISC
			forbid(ldisc != NTTYDISC);
# endif
			vsave();
			oglobp = globp;
			globp = "stop";
			goto gogo;
		}
#endif SIGTSTP
		switch (c) {

		/*
		 * ^L		Clear screen e.g. after transmission error.
		 */

		/*
		 * ^R		Retype screen, getting rid of @ lines.
		 *		If in open, equivalent to ^L.
		 *		On terminals where the right arrow key sends
		 *		^L we make ^R act like ^L, since there is no
		 *		way to get ^L.  These terminals (adm31, tvi)
		 *		are intelligent so ^R is useless.  Soroc
		 *		will probably foul this up, but nobody has
		 *		one of them.
		 */
		case CTRL(l):
		case CTRL(r):
			if (c == CTRL(l) || (key_right && *key_right==CTRL(l))) {
				vclear();
				vdirty(0, vcnt);
			}
			if (state != VISUAL) {
				/*
				 * Get a clean line, throw away the
				 * memory of what is displayed now,
				 * and move back onto the current line.
				 */
				vclean();
				vcnt = 0;
				vmoveto(dot, cursor, 0);
				continue;
			}
			vredraw(WTOP);
			/*
			 * Weird glitch -- when we enter visual
			 * in a very small window we may end up with
			 * no lines on the screen because the line
			 * at the top is too long.  This forces the screen
			 * to be expanded to make room for it (after
			 * we have printed @'s ick showing we goofed).
			 */
			if (vcnt == 0)
				vrepaint(cursor);
			vfixcurs();
			continue;

		/*
		 * $		Escape just cancels the current command
		 *		with a little feedback.
		 */
		case ESCAPE:
			beep();
			continue;

		/*
		 * @   		Macros. Bring in the macro and put it
		 *		in vmacbuf, point vglobp there and punt.
		 */
		 case '@':
			c = getesc();
			if (c == 0)
				continue;
			if (c == '@')
				c = lastmac;
#ifndef NONLS8 /* 8bit integrity */
			if ((c >= IS_MACRO_LOW_BOUND) && isupper(c & TRIM))
				c = tolower(c & TRIM);
			forbid((c < IS_MACRO_LOW_BOUND) || !islower(c & TRIM));
#else NONLS8
			if ((c >= IS_MACRO_LOW_BOUND) && isupper(c))
				c = tolower(c);
			forbid((c < IS_MACRO_LOW_BOUND) || !islower(c));
#endif NONLS8
			lastmac = c;
			vsave();
			CATCH
				char tmpbuf[BUFSIZ];
#ifndef	NLS16
				regbuf(c,tmpbuf,sizeof(vmacbuf));
#else	NLS16
				regbuf(c,tmpbuf,sizeof(vmacbuf)/sizeof(CHAR));
#endif	NLS16
				macpush(tmpbuf, 1);
			ONERR
				lastmac = 0;
				splitw = 0;
				getDOT();
				vrepaint(cursor);
				continue;
			ENDCATCH
#if defined NLS || defined NLS16
			if (right_to_left)
				FLIP(vmacbuf);
#endif
			vmacp = vmacbuf;
			goto reread;

		/*
		 * .		Repeat the last (modifying) open/visual command.
		 */
		case '.':
			/*
			 * Check that there was a last command, and
			 * take its count and named buffer unless they
			 * were given anew.  Special case if last command
			 * referenced a numeric named buffer -- increment
			 * the number and go to a named buffer again.
			 * This allows a sequence like "1pu.u.u...
			 * to successively look for stuff in the kill chain
			 * much as one does in EMACS with C-Y and M-Y.
			 */
			forbid (lastcmd[0] == 0);
			if (hadcnt)
				lastcnt = cnt;
			if (vreg)
				lastreg = vreg;

#ifndef NONLS8 /* Character set features */
			else if ((lastreg >= IS_MACRO_LOW_BOUND) 
			    && isdigit(lastreg & TRIM) && lastreg < '9')
#else NONLS8
			else if ((lastreg >= IS_MACRO_LOW_BOUND) && isdigit(lastreg) && lastreg < '9')
#endif NONLS8

				lastreg++;
			vreg = lastreg;
			cnt = lastcnt;
			hadcnt = lasthad;
			vglobp = lastcmd;
			goto reread;

		/*
		 * ^U		Scroll up.  A count sticks around for
		 *		future scrolls as the scroll amount.
		 *		Attempt to hold the indentation from the
		 *		top of the screen (in logical lines).
		 *
		 * BUG:		A ^U near the bottom of the screen
		 *		on a dumb terminal (which can't roll back)
		 *		causes the screen to be cleared and then
		 *		redrawn almost as it was.  In this case
		 *		one should simply move the cursor.
		 */
		case CTRL(u):
			if (hadcnt)
				vSCROLL = cnt;
			cnt = vSCROLL;
			if (state == VISUAL)
				ind = vcline, cnt += ind;
			else
				ind = 0;
			vmoving = 0;
			vup(cnt, ind, 1);
			vnline(NOCHAR);
			continue;

		/*
		 * ^D		Scroll down.  Like scroll up.
		 */
		case CTRL(d):
#ifdef TRACE
		if (trace)
			fprintf(trace, "before vdown in ^D, dot=%d, wdot=%d, dol=%d\n", lineno(dot), lineno(wdot), lineno(dol));
#endif
			if (hadcnt)
				vSCROLL = cnt;
			cnt = vSCROLL;
			if (state == VISUAL)
				ind = vcnt - vcline - 1, cnt += ind;
			else
				ind = 0;
			vmoving = 0;
			vdown(cnt, ind, 1);
#ifdef TRACE
		if (trace)
			fprintf(trace, "before vnline in ^D, dot=%d, wdot=%d, dol=%d\n", lineno(dot), lineno(wdot), lineno(dol));
#endif

			vnline(NOCHAR);

#ifdef TRACE
		if (trace)
			fprintf(trace, "after vnline in ^D, dot=%d, wdot=%d, dol=%d\n", lineno(dot), lineno(wdot), lineno(dol));
#endif
			continue;

		/*
		 * ^E		Glitch the screen down (one) line.
		 *		Cursor left on same line in file.
		 */
		case CTRL(e):
			if (state != VISUAL)
				continue;
			if (!hadcnt)
				cnt = 1;
			/* Bottom line of file already on screen */
			forbid(lineDOL()-lineDOT() <= vcnt-1-vcline);
			ind = vcnt - vcline - 1 + cnt;
			vdown(ind, ind, 1);
			vnline(cursor);
			continue;

		/*
		 * ^Y		Like ^E but up
		 */
		case CTRL(y):
			if (state != VISUAL)
				continue;
			if (!hadcnt)
				cnt = 1;
			forbid(lineDOT()-1<=vcline); /* line 1 already there */
			ind = vcline + cnt;
			vup(ind, ind, 1);
			vnline(cursor);
			continue;


		/*
		 * m		Mark position in mark register given
		 *		by following letter.  Return is
		 *		accomplished via ' or `; former
		 *		to beginning of line where mark
		 *		was set, latter to column where marked.
		 */
		case 'm':
			/*
			 * Getesc is generally used when a character
			 * is read as a latter part of a command
			 * to allow one to hit rubout/escape to cancel
			 * what you have typed so far.  These characters
			 * are mapped to 0 by the subroutine.
			 */
			c = getesc();
			if (c == 0)
				continue;

			/*
			 * Markreg checks that argument is a letter
			 * and also maps ' and ` to the end of the range
			 * to allow '' or `` to reference the previous
			 * context mark.
			 */
			c = markreg(c);
			forbid (c == 0);
			vsave();
			names[c - 'a'] = (*dot &~ 01);
			ncols[c - 'a'] = cursor;
			anymarks = 1;
			continue;

		/*
		 * ^F		Window forwards, with 2 lines of continuity.
		 *		Count repeats.
		 */
		case CTRL(f):
			vsave();
			if (vcnt > 2) {
				addr = dot + (vcnt - vcline) - 2 + (cnt-1)*basWLINES;
				forbid(addr > dol);
				dot = addr;
				vcnt = vcline = 0;
			}
			vzop(0, 0, '+');
			continue;

		/*
		 * ^B		Window backwards, with 2 lines of continuity.
		 *		Inverse of ^F.
		 */
		case CTRL(b):
			vsave();
			if (one + vcline != dot && vcnt > 2) {
#ifdef ED1000
				addr = dot - vcline - 2 - (cnt-1)*basWLINES;
#else
				addr = dot - vcline + 2 - (cnt-1)*basWLINES;
#endif ED1000
				forbid (addr <= zero);
				dot = addr;
				vcnt = vcline = 0;
			}
			vzop(0, 0, '^');
			continue;

		/*
		 * z		Screen adjustment, taking a following character:
		 *			zcarriage_return		current line to top
		 *			z<NL>		like zcarriage_return
		 *			z-		current line to bottom
		 *		also z+, z^ like ^F and ^B.
		 *		A preceding count is line to use rather
		 *		than current line.  A count between z and
		 *		specifier character changes the screen size
		 *		for the redraw.
		 *
		 */
		case 'z':
			if (state == VISUAL) {
				i = vgetcnt();
				if (i > 0)
					vsetsiz(i);
				c = getesc();
				if (c == 0)
					continue;
			}
			vsave();
			vzop(hadcnt, cnt, c);
			continue;

		/*
		 * Y		Yank lines, abbreviation for y_ or yy.
		 *		Yanked lines can be put later if no
		 *		changes intervene, or can be put in named
		 *		buffers and put anytime in this session.
		 */
		case 'Y':
			ungetkey('_');
			c = 'y';
			no_map_++;	/* don't expand '_' as a macro */
			break;

		/*
		 * J		Join lines, 2 by default.  Count is number
		 *		of lines to join (no join operator sorry.)
		 */
		case 'J':
			forbid (dot == dol);
			if (cnt == 1)
				cnt = 2;
			if (cnt > (i = dol - dot + 1))
				cnt = i;
			vsave();
			vmacchng(1);
			setLAST();
#ifndef	NLS16
			cursor = strend(linebuf);
#else	NLS16
			cursor = STREND(linebuf);
#endif	NLS16
			vremote(cnt, join, 0);
			notenam = "join";
			vmoving = 0;
			killU();
			vreplace(vcline, cnt, 1);
			if (!*cursor && cursor > linebuf)
#ifndef	NLS16
				cursor--;
#else	NLS16
				PST_DEC(cursor);
#endif	NLS16
			if (notecnt == 2)
				notecnt = 0;
			vrepaint(cursor);
			continue;

		/*
		 * S		Substitute text for whole lines, abbrev for c_.
		 *		Count is number of lines to change.
		 */
		case 'S':
			ungetkey('_');
			c = 'c';
			no_map_++;	/* don't expand '_' as a macro */
			break;

		/*
		 * O		Create a new line above current and accept new
		 *		input text, to an escape, there.
		 *		A count specifies, for dumb terminals when
		 *		slowopen is not set, the number of physical
		 *		line space to open on the screen.
		 *
		 * o		Like O, but opens lines below.
		 */
		case 'O':
		case 'o':
			vmacchng(1);
			voOpen(c, cnt);
			continue;

		/*
		 * C		Change text to end of line, short for c$.
		 */
		case 'C':
			if (*cursor) {
				ungetkey('$'), c = 'c';
				break;
			}
			goto appnd;

		/*
		 * ~	Switch case of letter under cursor
		 *      (a count now specifies the number of characters to switch)
		 */
		case '~':
			{
#ifndef	NLS16
				char mbuf[LBSIZE+4];
				char *cp = cursor;
#else	NLS16
				CHAR mbuf[LBSIZE+4];
				CHAR *cp = cursor;
#endif	NLS16
				int mcnt = 0;
				setLAST();

				/*
				 * The following is needed to make the ESC in the
				 * constructed macro taken as a command instead of
				 * more data when this command is re-executed via
				 * the dot (.) command.
				 */
				lastvgk = 0;

				/*
				 * Signal to vgetline not to do maps or abbreviation
				 * processing when executing the R command built below.
				 */
				tilde_cmd++;

				/*
				 * Build a macro such as "R <letters> ^] "
				 */
				mbuf[mcnt++] = 'R';
				while ( *cp && cnt-- > 0 ) {
#ifdef	NLS16
					if (IS_FIRST(*cp)) {
						mbuf[mcnt++] = *cp++;
						mbuf[mcnt++] = *cp++;
					} else
#endif	NLS16
					mbuf[mcnt++] = ((*cp >= IS_MACRO_LOW_BOUND) && isupper(*cp & TRIM))
							? tolower(*cp++ & TRIM)
							: toupper(*cp++ & TRIM);
				}

				/*
				 * Don't do the last space if at the end of a
				 * line, but in any case terminate the 'R'
				 * command and the macro string.
				 */
				mbuf[mcnt++] = ESCAPE;
				if (value(DOUBLEESCAPE))
					mbuf[mcnt++] = ESCAPE;
				if (*cp)
					mbuf[mcnt++] = ' ';
				mbuf[mcnt] = 0;

#ifndef	NLS16
				macpush(mbuf, 1);
#else	NLS16
				MACPUSH(mbuf, 1);
#endif	NLS16
			}
			continue;


		/*
		 * A		Append at end of line, short for $a.
		 */
		case 'A':
			operate('$', 1);
appnd:
			c = 'a';
			/* fall into ... */

		/*
		 * a		Appends text after cursor.  Text can continue
		 *		through arbitrary number of lines.
		 */
		case 'a':
			if (*cursor) {
				if (state == HARDOPEN)
					putchar(*cursor);
#ifndef	NLS16
				cursor++;
#else	NLS16
				PST_INC(cursor);
#endif	NLS16
			}
			goto insrt;

		/*
		 * I		Insert at beginning of whitespace of line,
		 *		short for ^i.
		 */
		case 'I':
			operate('^', 1);
			c = 'i';
			/* fall into ... */

		/*
		 * R		Replace characters, one for one, by input
		 *		(logically), like repeated r commands.
		 *
		 * BUG:		This is like the typeover mode of many other
		 *		editors, and is only rarely useful.  Its
		 *		implementation is a hack in a low level
		 *		routine and it doesn't work very well, e.g.
		 *		you can't move around within a R, etc.
		 */
		case 'R':
			/* fall into... */

		/*
		 * i		Insert text to an escape in the buffer.
		 *		Text is arbitrary.  This command reminds of
		 *		the i command in bare teco.
		 */
		case 'i':
insrt:
			/*
			 * Common code for all the insertion commands.
			 * Save for redo, position cursor, prepare for append
			 * at command and in visual undo.  Note that nothing
			 * is doomed, unless R when all is, and save the
			 * current line in a the undo temporary buffer.
			 */
			vmacchng(1);
			setLAST();
			vcursat(cursor);
			prepapp();
			vnoapp();
			doomed = c == 'R' ? 10000 : 0;
#if defined NLS16 && defined EUC
			b_doomed = doomed ;
#endif
			if(FIXUNDO)
				vundkind = VCHNG;
			vmoving = 0;
#ifndef	NLS16
			CP(vutmp, linebuf);
#else	NLS16
			STRCPY(vutmp, linebuf);
#endif	NLS16
			/*
			 * If this is a repeated command, then suppress
			 * fake insert mode on dumb terminals which looks
			 * ridiculous and wastes lots of time even at 9600B.
			 */
			if (vglobp)
				hold = HOLDQIK;
			vappend(c, cnt, 0);
			/*
			 * Restore the remainder of the macro if this insert
			 * was the result of a put from the inside of a macro.
			 */
			if (ovmacp) {
				vmacp = ovmacp;
				ovmacp = 0;
			}
			continue;

		/*
		 * ^?		An attention, normally a ^?, just beeps.
		 *		If you are a vi command within ex, then
		 *		two ATTN's will drop you back to command mode.
		 */
		case ATTN:
			beep();
			if (initev || peekkey() != ATTN)
				continue;
			/* fall into... */

		/*
		 * ^\		A quit always gets command mode.
		 */
		case QUIT:
			/*
			 * Have to be careful if we were called
			 *	g/xxx/vi
			 * since a return will just start up again.
			 * So we simulate an interrupt.
			 */
			if (inglobal)
				onintr();
			/* fall into... */

#ifdef notdef
		/*
		 * q		Quit back to command mode, unless called as
		 *		vi on command line in which case dont do it
		 */
		case 'q':	/* quit */
			if (initev) {
				vsave();
				CATCH
					error("Q gets ex command mode, :q leaves vi");
				ENDCATCH
				splitw = 0;
				getDOT();
				vrepaint(cursor);
				continue;
			}
#endif
			/* fall into... */

		/*
		 * Q		Is like q, but always gets to command mode
		 *		even if command line invocation was as vi.
		 */
		case 'Q':
			vsave();
			/*
			 * If we are in the middle of a macro, throw away
			 * the rest and fix up undo.
			 * This code copied from getbr().
			 */
			if (vmacp) {
				vmacp = 0;
				if (inopen == -1)	/* don't screw up undo for esc esc */
					vundkind = VMANY;
				inopen = 1;	/* restore old setting now that macro done */
			}
			return;


		/*
		 * ZZ		Like :x
		 */
		 case 'Z':
#if defined NLS || defined NLS16
			forbid(RL_KEY(getkey()) != 'Z');
#else
			forbid(getkey() != 'Z');
#endif
			oglobp = globp;
			globp = "x";
			vclrech(0);
			goto gogo;
			
		/*
		 * P		Put back text before cursor or before current
		 *		line.  If text was whole lines goes back
		 *		as whole lines.  If part of a single line
		 *		or parts of whole lines splits up current
		 *		line to form many new lines.
		 *		May specify a named buffer, or the delete
		 *		saving buffers 1-9.
		 *
		 * p		Like P but after rather than before.
		 */
		case 'P':
		case 'p':
			vmoving = 0;
#ifdef notdef
			forbid (!vreg && value(UNDOMACRO) && inopen < 0);
#endif
			/*
			 * If previous delete was partial line, use an
			 * append or insert to put it back so as to
			 * use insert mode on intelligent terminals.
			 */
			if (!vreg && DEL[0]) {
				forbid ((DEL[0] & (QUOTE|TRIM)) == OVERBUF);
				/*
				 * Hide the rest of the macro if this put is
				 * part of a macro string.  Otherwise, the
				 * insert done below will read the rest of
				 * the macro as part of the string to be put.
				 */
				if (vmacp && *vmacp) {
					ovmacp = vmacp;
					vmacp = 0;
				}
#if defined NLS || defined NLS16
				if (right_to_left)
					FLIP(DEL);
#endif
				vglobp = DEL;
				ungetkey(c == 'p' ? 'a' : 'i');
				goto reread;
			}

			/*
			 * If a register wasn't specified, then make
			 * sure there is something to put back.
			 */
			forbid (!vreg && unddol == dol);
			/*
			 * If we just did a macro the whole buffer is in
			 * the undo save area.  We don't want to put THAT.
			 */
			/* But if we use register, we want to put it.	*/
			forbid (!vreg && vundkind == VMANY && undkind==UNDALL);
			vsave();
			vmacchng(1);
			setLAST();
			i = 0;
			if (vreg && partreg(vreg) || !vreg && pkill[0]) {
				/*
				 * Restoring multiple lines which were partial
				 * lines; will leave cursor in middle
				 * of line after shoving restored text in to
				 * split the current line.
				 */
				i++;
				if (c == 'p' && *cursor)
#ifndef	NLS16
					cursor++;
#else	NLS16
					PST_INC(cursor);
#endif	NLS16
			} else {
				/*
				 * In whole line case, have to back up dot
				 * for P; also want to clear cursor so
				 * cursor will eventually be positioned
				 * at the beginning of the first put line.
				 */
				cursor = 0;
				if (c == 'P') {
					dot--, vcline--;
					c = 'p';
				}
			}
			killU();

			/*
			 * The call to putreg can potentially
			 * bomb since there may be nothing in a named buffer.
			 * We thus put a catch in here.  If we didn't and
			 * there was an error we would end up in command mode.
			 */
			addr = dol;	/* old dol */
			CATCH
				vremote(1, vreg ? putreg : put, vreg);
			ONERR
				if (vreg == -1) {
					splitw = 0;
					if (op == 'P')
						dot++, vcline++;
					goto pfixup;
				}
			ENDCATCH
			splitw = 0;
			nlput = dol - addr + 1;
			if (!i) {
				/*
				 * Increment undap1, undap2 to make up
				 * for their incorrect initialization in the
				 * routine vremote before calling put/putreg.
				 */
				if (FIXUNDO)
					undap1++, undap2++;
				vcline++;
				nlput--;

				/*
				 * After a put want current line first line,
				 * and dot was made the last line put in code
				 * run so far.  This is why we increment vcline
				 * above and decrease dot here.
				 */
				dot -= nlput - 1;
			}
#ifdef TRACE
			if (trace)
				fprintf(trace, "vreplace(%d, %d, %d), undap1=%d, undap2=%d, dot=%d\n", vcline, i, nlput, lineno(undap1), lineno(undap2), lineno(dot));
#endif
			vreplace(vcline, i, nlput);
			if (state != VISUAL) {
				/*
				 * Special case in open mode.
				 * Force action on the screen when a single
				 * line is put even if it is identical to
				 * the current line, e.g. on YP; otherwise
				 * you can't tell anything happened.
				 */
				vjumpto(dot, cursor, '.');
				continue;
			}
pfixup:
			vrepaint(cursor);
			vfixcurs();
			continue;

		/*
		 * ^^		Return to previous file.
		 *		Like a :e #, and thus can be used after a
		 *		"No Write" diagnostic.
		 */
		case CTRL(^):
			forbid (hadcnt);
			vsave();
			ckaw();
			oglobp = globp;
			if (value(AUTOWRITE) && !value(READONLY))
				globp = "e! #";
			else
				globp = "e #";
			goto gogo;

		/*
		 * ^]		Takes word after cursor as tag, and then does
		 *		tag command.  Read ``go right to''.
		 */
		case CTRL(]):
			grabtag();
			oglobp = globp;
			globp = "tag";
			goto gogo;

		/*
		 * &		Like :&
		 */
		 case '&':
			oglobp = globp;
			globp = "&";
			goto gogo;
			
		/*
		 * ^G		Bring up a status line at the bottom of
		 *		the screen, like a :file command.
		 *
		 * BUG:		Was ^S but doesn't work in cbreak mode
		 */
		case CTRL(g):
			oglobp = globp;
			globp = "file";
gogo:
			addr = dot;
			vsave();
			goto doinit;

#ifndef hpux
		/*
		 * We grab the suspend char the user has set
		 * on entry for hpux systems (not hard-coded ^z).
		 */
#ifdef SIGTSTP
		/*
		 * ^Z:	suspend editor session and temporarily return
		 * 	to shell.  Only works with Berkeley/IIASA process
		 *	control in kernel.
		 */
		case CTRL(z):
			forbid(dosusp == 0);
# ifdef NTTYDISC
			forbid(ldisc != NTTYDISC);
# endif
			vsave();
			oglobp = globp;
			globp = "stop";
			goto gogo;
#endif
#endif not hpux

		/*
		 * :		Read a command from the echo area and
		 *		execute it in command mode.
		 */
		case ':':
#if defined NLS || defined NLS16
			if (right_to_left)
				tputs(lock_keys,1,putch), flusho();
#endif
			forbid (hadcnt);
			vsave();
			i = tchng;
			addr = dot;
			if (readecho(c)) {
				esave[0] = 0;

				/*
				 * The following lines protect the screen buffer
				 * from being trashed when a multi-line ':'
				 * command is aborted via a ^U^H (kill line
				 * followed by erase).
				 */
				splitw++;
				vgoto(WECHO, 0);
				fixol();
				splitw = 0;

				goto fixup;
			}
			getDOT();
			/*
			 * Use the visual undo buffer to store the global
			 * string for command mode, since it is idle right now.
			 */
#ifndef	NLS16
			oglobp = globp; strcpy(vutmp, genbuf+1); globp = vutmp;
#else	NLS16
			oglobp = globp; CHAR_TO_char(genbuf+1, vcmd); globp = vcmd;
#endif	NLS16

doinit:
			esave[0] = 0;
			fixech();
			/*
			 * Have to finagle around not to lose last
			 * character after this command (when run from ex
			 * command mode).  This is clumsy.
			 */
			d = peekc;
#ifndef	NLS16
			ungetchar(0);
#else	NLS16
			/*
			 * It is necessary to save and clear not only peekc
			 * but also secondchar.
			 */
			c = secondchar;
			clrpeekc();
#endif	NLS16
			if (shouldpo) {
				/*
				 * So after a "Hit return..." ":", we do
				 * another "Hit return..." the next time
				 */
				pofix();
				shouldpo = 0;
			}
			CATCH
				/*
				 * Save old values of options so we can
				 * notice when they change; switch into
				 * cooked mode so we are interruptible.
				 */
				onumber = value(NUMBER);
				olist = value(LIST);
				OPline = Pline;
				OPutchar = Putchar;
#ifndef CBREAK
				vcook();
#endif
				commands(1, 1);
				if (dot == zero && dol > zero)
					dot = one;
#ifndef CBREAK
				vraw();
#endif
			ONERR
#ifndef CBREAK
				vraw();
#endif

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
				copys(esave, vtube[WECHO], VBUFCOLS);
#else
				copys(esave, vtube[WECHO], TUBECOLS);
#endif
#else NONLS8
				copy(esave, vtube[WECHO], TUBECOLS);
#endif NONLS8

			ENDCATCH
			fixol();
			Pline = OPline;
			Putchar = OPutchar;
			ungetchar(d);
#ifdef	NLS16
			/*
			 * It is necessary to restore secondchar.
			 */
			secondchar = c;
#endif	NLS16
			globp = oglobp;

			/*
			 * If we ended up with no lines in the buffer, make
			 * a line, and don't consider the buffer changed.
			 */
			if (dot == zero) {
				fixzero();

				/*

				Don't know the original reasoning, but we do
				want to consider the buffer changed so that
				:1,$d followed by :x writes out the now
				empty file.

				sync();

				*/
			}
			splitw = 0;

			/*
			 * Special case: did list/number options change?
			 */
			if (onumber != value(NUMBER))
				setnumb(value(NUMBER));
			if (olist != value(LIST))
				setlist(value(LIST));

fixup:
			/*
			 * If a change occurred, other than
			 * a write which clears changes, then
			 * we should allow an undo even if .
			 * didn't move.
			 *
			 * BUG: You can make this wrong by
			 * tricking around with multiple commands
			 * on one line of : escape, and including
			 * a write command there, but its not
			 * worth worrying about.
			 */
			if (FIXUNDO && tchng && tchng != i)
				vundkind = VMANY, cursor = 0;

			/*
			 * If we are about to do another :, hold off
			 * updating of screen.
			 */
			if (vcnt < 0 && Peekkey == ':') {
				getDOT();
				shouldpo = 1;
				continue;
			}
			shouldpo = 0;

			/*
			 * In the case where the file being edited is
			 * new; e.g. if the initial state hasn't been
			 * saved yet, then do so now.
			 */
			if (unddol == truedol) {
				vundkind = VNONE;
				Vlines = lineDOL();
				if (!inglobal)
					savevis();
				addr = zero;
				vcnt = 0;
				if (esave[0] == 0)

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
					copys(esave, vtube[WECHO], VBUFCOLS);
#else
					copys(esave, vtube[WECHO], TUBECOLS);
#endif
#else NONLS8
					copy(esave, vtube[WECHO], TUBECOLS);
#endif NONLS8

			}

			/*
			 * If the current line moved reset the cursor position.
			 */
			if (dot != addr) {
				vmoving = 0;
				cursor = 0;
			}

			/*
			 * If current line is not on screen or if we are
			 * in open mode and . moved, then redraw.
			 */
			i = vcline + (dot - addr);
			if (i < 0 || i >= vcnt && i >= -vcnt || state != VISUAL && dot != addr) {
				if (state == CRTOPEN)
					vup1();
				if (vcnt > 0)
					vcnt = 0;
				vjumpto(dot, (CHAR *) 0, '.');
			} else {
				/*
				 * Current line IS on screen.
				 * If we did a [Hit return...] then
				 * restore vcnt and clear screen if in visual
				 */
				vcline = i;
				if (vcnt < 0) {
					vcnt = -vcnt;
					if (state == VISUAL)
						vclear();
					else if (state == CRTOPEN) {
						vcnt = 0;
					}
				}

				/*
				 * Limit max value of vcnt based on $
				 */
				i = vcline + lineDOL() - lineDOT() + 1;
				if (i < vcnt)
					vcnt = i;
				
				/*
				 * Dirty and repaint.
				 */
				vdirty(0, lines);
				vrepaint(cursor);
			}

			/*
			 * If in visual, put back the echo area
			 * if it was clobberred.
			 */
			if (state == VISUAL) {
				int sdc = destcol, sdl = destline;

				splitw++;
				vigoto(WECHO, 0);
#if defined NLS || defined NLS16
				RL_OKEY
#endif
#if defined NLS16 && defined EUC
				for (i = 0; i < VBUFCOLS - 1; i++) {
#else
				for (i = 0; i < TUBECOLS - 1; i++) {
#endif
					if (esave[i] == 0)
						break;
					vputchar(esave[i]);
				}
#if defined NLS || defined NLS16
				RL_OSCREEN
#endif
				splitw = 0;
				vgoto(sdl, sdc);
			}
			continue;

		/*
		 * u		undo the last changing command.
		 */
		case 'u':
			vundo(1);
			continue;

		/*
		 * U		restore current line to initial state.
		 */
		case 'U':
			vUndo();
			continue;

fonfon:
			beep();
#ifdef	NLS16
			/* skip over the remaining 2nd byte of 16-bit character
			** not to take it as the next command.
			*/
			c = peekkey();
			if (IS_SECOND(c))
				ignore(getkey());
#endif	NLS16
			vmacp = 0;
			inopen = 1;	/* might have been -1 */
			continue;
		}

		/*
		 * Rest of commands are decoded by the operate
		 * routine.
		 */
		operate(c, cnt);
	}
}

/*
 * Grab the word after the cursor so we can look for it as a tag.
 */
grabtag()
{
	register CHAR *cp;
	register char *dp;

#ifdef	NLS16
	register int	wdtype;		/* type of the character */
	register CHAR	lastch;		/* the last byte of the extracted tag */
#endif	NLS16
	cp = vpastwh(cursor);
	if (*cp) {
		dp = lasttag;
#ifdef	NLS16
		/*  a word after cp is extracted as the tag value.  [WORD(1)]
		**  ( but any character but white is allowed as the first
		**  character of tag,  e.g. "*XYZ" is a possible tag. )
		*/
		if (IS_FIRST(*cp))
			*dp++ = *cp++;
		wdtype = wordch(cp + 1);
		wdkind = 1;	/* conservative definition of word : [WORD(1)] */
#endif	NLS16
		do {
			if (dp < &lasttag[sizeof lasttag - 2])
#ifndef	NLS16
				*dp++ = *cp;
#else	NLS16
				*dp++ = lastch = *cp;
#endif	NLS16
			cp++;
#ifndef	NLS16
#ifndef NONLS8 /* Character set features */
		} while (((*cp >= IS_MACRO_LOW_BOUND) && (isalpha(*cp & TRIM) 
		    || isdigit(*cp & TRIM))) || *cp == '_');
#else NONLS8
		} while (((*cp >= IS_MACRO_LOW_BOUND) && (isalpha(*cp) || isdigit(*cp))) || *cp == '_');
#endif NONLS8
#else	NLS16
		} while (wdtype && wordof(wdtype, cp));
			/* word consists of characters of the same type */
		if (IS_FIRST(lastch))
			/* 1st byte of 16-bit character may not be the last of tag */
			dp--;
#endif	NLS16
		*dp++ = 0;
	}
}

/*
 * Before appending lines, set up addr1 and
 * the command mode undo information.
 */
prepapp()
{

	addr1 = dot;
	deletenone();
	addr1++;
	appendnone();
}

/*
 * Execute function f with the address bounds addr1
 * and addr2 surrounding cnt lines starting at dot.
 */
vremote(cnt, f, arg)
	int cnt, (*f)(), arg;
{
	register int oing = inglobal;

	addr1 = dot;
	addr2 = dot + cnt - 1;
	inglobal = 0;
	if (FIXUNDO)
		undap1 = undap2 = dot;
	(*f)(arg);
	inglobal = oing;
	if (FIXUNDO)
		vundkind = VMANY;
	vmcurs = 0;
}

/*
 * Save the current contents of linebuf, if it has changed.
 */
vsave()
{
	CHAR temp[LBSIZE];

#ifndef	NLS16
	CP(temp, linebuf);
#else	NLS16
	STRCPY(temp, linebuf);
#endif	NLS16
	if (FIXUNDO && vundkind == VCHNG || vundkind == VCAPU) {
		/*
		 * If the undo state is saved in the temporary buffer
		 * vutmp, then we sync this into the temp file so that
		 * we will be able to undo even after we have moved off
		 * the line.  It would be possible to associate a line
		 * with vutmp but we assume that vutmp is only associated
		 * with line dot (e.g. in case ':') above, so beware.
		 */
		prepapp();
#ifndef	NLS16
		strcLIN(vutmp);
#else	NLS16
		STRCLIN(vutmp);
#endif	NLS16
		putmark(dot);
		vremote(1, yank, 0);
		vundkind = VMCHNG;
		notecnt = 0;
		undkind = UNDCHANGE;
	}
	/*
	 * Get the line out of the temp file and do nothing if it hasn't
	 * changed.  This may seem like a loss, but the line will
	 * almost always be in a read buffer so this may well avoid disk i/o.
	 */
	getDOT();
#ifndef	NLS16
	if (strcmp(linebuf, temp) == 0)
		return;
	strcLIN(temp);
#else	NLS16
	if (STRCMP(linebuf, temp) == 0)
		return;
	STRCLIN(temp);
#endif	NLS16
	putmark(dot);
}

#undef	forbid
#define	forbid(a)	if (a) { beep(); return; }

/*
 * Do a z operation.
 * Code here is rather long, and very uninteresting.
 */
vzop(hadcnt, cnt, c)
	bool hadcnt;
	int cnt;
	register int c;
{
	register line *addr;

	if (state != VISUAL) {
		/*
		 * Z from open; always like a z=.
		 * This code is a mess and should be cleaned up.
		 */
		vmoveitup(1, 1);
		vgoto(outline, 0);
		ostop(normf);
		setoutt();
		addr2 = dot;
		vclear();
		destline = WECHO;
		zop2(Xhadcnt ? Xcnt : value(WINDOW) - 1, '=');
		if (state == CRTOPEN)
			putnl();
		putNFL();
		termreset();
		Outchar = vputchar;
		ignore(ostart());
		vcnt = 0;
		outline = destline = 0;
		vjumpto(dot, cursor, 0);
		return;
	}
	if (hadcnt) {
		addr = zero + cnt;
		if (addr < one)
			addr = one;
		if (addr > dol)
			addr = dol;
		markit(addr);
	} else
		switch (c) {

		case '+':
			addr = dot + vcnt - vcline;
			break;

		case '^':
			addr = dot - vcline - 1;
			forbid (addr < one);
			c = '-';
			break;

		default:
			addr = dot;
			break;
		}
	switch (c) {

	case '.':
	case '-':
		break;

	case '^':
		forbid (addr <= one);
		break;

	case '+':
		forbid (addr >= dol);
		/* fall into ... */

	case CR:
	case NL:
		c = CR;
		break;

	default:
		beep();
#ifdef	NLS16
		/* skip over the remaining 2nd byte of 16-bit character
		** not to take it as the next command.
		*/
		c = peekkey();
		if (IS_SECOND(c))
			ignore(getkey());
#endif	NLS16
		return;
	}
	vmoving = 0;
	vjumpto(addr, NOCHAR, c);
}
