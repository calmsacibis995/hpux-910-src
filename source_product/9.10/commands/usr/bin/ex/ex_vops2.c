/* @(#) $Revision: 70.4 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Low level routines for operations sequences,
 * and mostly, insert mode (and a subroutine
 * to read an input line, including in the echo area.)
 */

extern CHAR	*vUA1, *vUA2;		/* mjm: extern; also in ex_vops.c */
extern CHAR	*vUD1, *vUD2;		/* mjm: extern; also in ex_vops.c */

#ifndef NONLS8 /* User messages */
# define	NL_SETN	18	/* set number */
# include	<msgbuf.h>
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

#ifdef NLS16
CHAR	dummy_caret[2] = {'^', 0};		/* "^" for 16-bit */
CHAR	dummy_zero[2] = {'0', 0};		/* "0" for 16-bit */
CHAR	dummy_ctrld[2] = {CTRL(d)|QUOTE, 0};	/* "\204" for 16-bit */
CHAR	dummy_ctrlt[2] = {CTRL(t)|QUOTE, 0};	/* "\224" for 16-bit */
CHAR	dummy_space[2] = {' ', 0};		/* " " for 16-bit */
CHAR	dummy_newln[2] = {'\n', 0};		/* "\n" for 16-bit */
#endif

/*
 * Obleeperate characters in hardcopy
 * open with \'s.
 */
bleep(i, cp)
	register int i;
	CHAR *cp;
{

	i -= column(cp);
	do
		putchar('\\' | QUOTE);
	while (--i >= 0);
	rubble = 1;
}

/*
 * Common code for middle part of delete
 * and change operating on parts of lines.
 */
vdcMID()
{
	register CHAR *cp;

	squish();
	setLAST();
	if (FIXUNDO)

#ifndef	NLS16
		vundkind = VCHNG, CP(vutmp, linebuf);
#else
		vundkind = VCHNG, STRCPY(vutmp, linebuf);
#endif

	if (wcursor < cursor)
		cp = wcursor, wcursor = cursor, cursor = cp;
	vUD1 = vUA1 = vUA2 = cursor; vUD2 = wcursor;
	return (column(wcursor - 1));
}

/*
 * Take text from linebuf and stick it
 * in the VBSIZE buffer BUF.  Used to save
 * deleted text of part of line.
 */
takeout(BUF)
	CHAR *BUF;
{
	register CHAR *cp;

	if (wcursor < linebuf)
		wcursor = linebuf;
	if (cursor == wcursor) {
		beep();
		return;
	}
	if (wcursor < cursor) {
		cp = wcursor;
		wcursor = cursor;
		cursor = cp;
	}
	setBUF(BUF);
	if ((BUF[0] & (QUOTE|TRIM)) == OVERBUF)
		beep();
}

/*
 * Are we at the end of the printed representation of the
 * line?  Used internally in hardcopy open.
 */
ateopr()
{
	register int i, c;

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
	register short *cp ;
	int col_vtub ;
	cp = vtube[destline] + vcolumn( vtube[destline], destcol ) ;
#else
	register short *cp = vtube[destline] + destcol;
#endif
#else NONLS8
	register char *cp = vtube[destline] + destcol;
#endif NONLS8

#if defined NLS16 && defined EUC
	col_vtub = vcolumn( vtube[destline], WCOLS - 20 ) ;
#endif

	for (i = WCOLS - destcol; i > 0; i--) {
#if defined NLS16 && defined EUC
		if ( IS_FIRST(*cp) ){
			i += ( 2 - C_COLWIDTH( ( (int)(*cp) ) & TRIM ) ) ;
		}
#endif
		c = *cp++;
		if (c == 0) {
			/*
			 * Optimization to consider returning early, saving
			 * CPU time.  We have to make a special check that
			 * we aren't missing a mode indicator.
			 */
#if defined NLS16 && defined EUC
			if (destline == WECHO && destcol < WCOLS-11 && vtube[WECHO][col_vtub])
#else
			if (destline == WECHO && destcol < WCOLS-11 && vtube[WECHO][WCOLS-20])
#endif
				return 0;
			return (1);
		}
		if (c != ' ' && (c & QUOTE) == 0)
			return (0);
	}
	return (1);
}

/*
 * Append.
 *
 * This routine handles the top level append, doing work
 * as each new line comes in, and arranging repeatability.
 * It also handles append with repeat counts, and calculation
 * of autoindents for new lines.
 */
bool	vaifirst;
bool	repbreak = 0;	/* entred a <CR> inside a R command */
bool	endbreak = 0;	/* entred <esc> after <CR> inside a R command */
bool	gobbled;
CHAR	*ogcursor;
#ifdef SIGWINCH
jmp_buf vappend_env, vgetline_env;
int	ins_winch();
#endif SIGWINCH

vappend(ch, cnt, indent)
	int ch;		/* mjm: char --> int */
	int cnt, indent;
{
	register int pindent = 0;	/* the previous indentation value */
	register int i;
	register CHAR *gcursor;
	bool escape;
	int repcnt, savedoomed;
	int colnum;
#if defined NLS16 && defined EUC
	int b_savedoomed ;
#endif
	short oldhold = hold;
#if defined NLS || defined NLS16
	register int sdc;
	register CHAR *sc;
#endif
#ifdef SIGWINCH
	int resize;
	got_winch_insert = 0;
#endif SIGWINCH

	/*
	 * Before a move in hardopen when the line is dirty
	 * or we are in the middle of the printed representation,
	 * we retype the line to the left of the cursor so the
	 * insert looks clean.
	 */
	if (ch != 'o' && state == HARDOPEN && (rubble || !ateopr())) {
		rubble = 1;
		gcursor = cursor;
		i = *gcursor;
		*gcursor = ' ';
		wcursor = gcursor;
		vmove();
		*gcursor = i;
	}
	vaifirst = indent == 0;

	/*
	 * Handle replace character by (eventually)
	 * limiting the number of input characters allowed
	 * in the vgetline routine.
	 */
	if (ch == 'r')
		repcnt = 2;
	else
		repcnt = 0;

	/*
	 * If an autoindent is specified, then
	 * generate a mixture of blanks to tabs to implement
	 * it and place the cursor after the indent.
	 * Text read by the vgetline routine will be placed in genbuf,
	 * so the indent is generated there.
	 */
	if (value(AUTOINDENT) && indent != 0) {
		gcursor = genindent(indent);
		*gcursor = 0;
		vgotoCL(qcolumn(cursor - 1, genbuf));
	} else {
		gcursor = genbuf;
		*gcursor = 0;
		if (ch == 'o')
			vfixcurs();
	}

	/*
	 * Prepare for undo.  Pointers delimit inserted portion of line.
	 */
	vUA1 = vUA2 = cursor;

	/*
	 * If we are not in a repeated command and a ^@ comes in
	 * then this means the previous inserted text.
	 * If there is none or it was too long to be saved,
	 * then beep() and also arrange to undo any damage done
	 * so far (e.g. if we are a change.)
	 */
	switch (ch) {
	case 'r':
		break;
	case 'R':
		vshowmode("REPLACE MODE");
		break;
	default:
		vshowmode("INPUT MODE");
	}
#ifdef TEPE
	/* hp3000 tepe testing prompt */
	vputc('\021');
	flusho();
#endif
#if defined NLS || defined NLS16
	/* keyorder here just in case append starts with opp lang */
	if (ch != 'r') {
		RL_OKEY
	}
#endif
	if ((vglobp && *vglobp == 0) || peekbr()) {
		if ((INS[0] & (QUOTE|TRIM)) == OVERBUF) {
			beep();
			if (!splitw)
				ungetkey('u');
			doomed = 0;
#if defined NLS16 && defined EUC
			b_doomed = 0 ;
#endif
			hold = oldhold;
			return;
		}
		/*
		 * Unread input from INS.
		 * An escape will be generated at end of string.
		 * Hold off n^^2 type update on dumb terminals.
		 */
		vglobp = INS;
		hold |= HOLDQIK;
	} else if (vglobp == 0)
		/*
		 * Not a repeated command, get
		 * a new inserted text for repeat.
		 */
		INS[0] = 0;

	/*
	 * For wrapmargin to hack away second space after a '.'
	 * when the first space caused a line break we keep
	 * track that this happened in gobblebl, which says
	 * to gobble up a blank silently.
	 */
	gobblebl = 0;

	/*
	 * Text gathering loop.
	 * New text goes into genbuf starting at gcursor.
	 * cursor preserves place in linebuf where text will eventually go.
	 */
	if (*cursor == 0 || state == CRTOPEN)
		hold |= HOLDROL;
	for (;;) {
#if defined NLS || defined NLS16
		/* gather text in keyorder */
		if (ch != 'r') {
			RL_OKEY
		}
#endif
		if (ch == 'r' && repcnt == 0)
			escape = 0;
		else {
#if defined NLS || defined NLS16
			sdc = destcol; opp_fix = 1;
#endif

#ifdef SIGWINCH 
			ign_winch = 0;
			(void) signal (SIGWINCH, ins_winch);

			if (resize = setjmp(vappend_env))
			{
				if (((colnum = column(cursor)) != 1) && (colnum != 0 )) 
				{
#ifdef NLS16
				    PST_INC(cursor);
#else
				    cursor++;
#endif NLS16
				}
				vcursat(cursor);
				cnt = 0;
				switch (ch) 
				{
				    case 'r':
					break;
				    case 'R':
					vshowmode("REPLACE MODE");
					doomed = 10000;
#if defined NLS16 || defined EUC
					b_doomed = doomed;
#endif
					break;
				    default:
					vshowmode("INPUT MODE");
				}
				gcursor = genbuf;
				ign_winch = 0;
				(void) signal (SIGWINCH, ins_winch);
			}
#endif SIGWINCH

			gcursor = vgetline(repcnt, gcursor, &escape, ch);
			/*
			 * After an append, stick information
			 * about the ^D's and ^^D's and 0^D's in
			 * the repeated text buffer so repeated
			 * inserts of stuff indented with ^D as backtab's
			 * can work.
			 */
			if (HADUP)

#ifndef	NLS16
				addtext("^");
#else
				addtext(dummy_caret);
#endif

			else if (HADZERO)

#ifndef	NLS16
				addtext("0");
#else
				addtext(dummy_zero);
#endif

			while (CDCNT < 0)

#ifndef	NLS16
				addtext("\224"), CDCNT++;
#else
				addtext(dummy_ctrlt), CDCNT++;
#endif

			while (CDCNT > 0)

#ifndef	NLS16
				addtext("\204"), CDCNT--;
#else
				addtext(dummy_ctrld), CDCNT--;
#endif

			if (gobbled)

#ifndef	NLS16
				addtext(" ");
#else
				addtext(dummy_space);
#endif

			addtext(ogcursor);
		}
		repcnt = 0;
#if defined NLS || defined NLS16
		/*
		** The strategy here is to create genbuf in keyboard
		** order and convert it to multi-line screen order
		** before the copy to linebuf.  A similar strategy
		** is used for vtube which is entered in keyboard
		** order and converted here in vappend before the
		** call to vreopen.  The flag opp_fix signals that
		** vtube is converted or fixed for opposite language
		** insertion.  The keyboard is locked and unlocked
		** in an attempt to prevent typing base mode characters
		** during an opposite language insertion.
		*/
		if (right_to_left && ch != 'r') {	
			if (opp_insert && escape == '\n')
				tputs(lock_keys, 1, putch), flusho();
			if (*genbuf)
				FLIP_LINE(genbuf, sdc);
			tputs(screen_order, 1, putch);
			rl_curorder = NL_SCREEN;
			if (opp_terminate) {
				base_lang();
			} else {
				base_mode();
			}
			tputs(tparm(cursor_address,destline,destcol),0,putch);
			opp_fix = 1;
		}
#endif
		/*
		 * Smash the generated and preexisting indents together
		 * and generate one cleanly made out of tabs and spaces
		 * if we are using autoindent.
		 */
		pindent = indent;
		if (!vaifirst && value(AUTOINDENT)) {
			i = fixindent(indent);
			if (!HADUP)
				indent = i;

#ifndef	NLS16
			gcursor = strend(genbuf);
#else
			gcursor = STREND(genbuf);
#endif

		}

		/*
		 * Limit the repetition count based on maximum
		 * possible line length; do output implied
		 * by further count (> 1) and cons up the new line
		 * in linebuf.
		 */
		cnt = vmaxrep(ch, cnt, pindent);

#ifndef	NLS16
		CP(gcursor + 1, cursor);
#else
		STRCPY(ADVANCE(gcursor), cursor);
#endif
#if defined NLS || defined NLS16
		sc = cursor;
#endif

		do {

#ifndef	NLS16
			CP(cursor, genbuf);
#else
			STRCPY(cursor, genbuf);
#endif
#if defined NLS || defined NLS16
			if (right_to_left && ch != 'r' && *genbuf)
				upd_vtube(genbuf, sdc);
#endif

			if (cnt > 1) {
				int oldhold = hold;

				Outchar = vinschar;
				hold |= HOLDQIK;

#ifndef	NLS16
				printf("%s", genbuf);
#else
				PRINTF(genbuf);
#endif

				hold = oldhold;
				Outchar = vputchar;
			}
			cursor += gcursor - genbuf;
		} while (--cnt > 0);
		endim();

		/* check if line is too big to display (too many <tabs>) */
		if (LINE(vcline) < 0) {
			error("line too long");
			break;
		}

		vUA2 = cursor;
		if (escape != '\n') {
#ifndef	NLS16
			CP(cursor, gcursor + 1);
#else
			STRCPY(cursor, ADVANCE(gcursor));
#endif
#if defined NLS || defined NLS16
			if (right_to_left && ch != 'r' && *genbuf)
				upd_vtube(gcursor + 1, sdc + (cursor - sc));
#endif
		}

		/*
		 * If doomed characters remain, clobber them,
		 * and reopen the line to get the display exact.
		 */
		if (state != HARDOPEN) {
			DEPTH(vcline) = 0;
			savedoomed = doomed;
#if defined NLS16 && defined EUC
			b_savedoomed = b_doomed ;
			if ( doomed != b_doomed ) {
				if ( b_doomed > 0 ) {
					register int cind = cindent();
					bphysdc(cind, cind + b_doomed) ;
					b_doomed = 0 ;
				}
				if (doomed > 0) {
					register int cind = cindent();
					vphysdc( cind, cind + doomed ) ;
					doomed = 0;
				}
			} else {
#endif
				if (doomed > 0) {
					register int cind = cindent();
#if defined NLS || defined NLS16
					if (right_to_left && ch != 'r')
						chk_vtube_end(cind);
#endif
					physdc(cind, cind + doomed);
					doomed = 0;
#if defined NLS16 && defined EUC
					b_doomed = 0 ;
#endif
				}
#if defined NLS16 && defined EUC
			}
#endif
			i = vreopen(LINE(vcline), lineDOT(), vcline);
#ifdef TRACE
			if (trace)
				fprintf(trace, "restoring doomed from %d to %d\n", doomed, savedoomed);
#endif
#if defined NLS16 && defined EUC
			if (ch == 'R') {
				doomed = savedoomed;
				b_doomed = b_savedoomed;
			}
#else
			if (ch == 'R')
				doomed = savedoomed;
#endif
		}

		/*
		 * All done unless we are continuing on to another line.
		 */
		if (escape != '\n') {
			vshowmode("");
			break;
		}

		/*
		 * Set up for the new line.
		 * First save the current line, then construct a new
		 * first image for the continuation line consisting
		 * of any new autoindent plus the pushed ahead text.
		 */
		killU();

#ifndef	NLS16
		addtext(gobblebl ? " " : "\n");
#else
		addtext(gobblebl ? dummy_space : dummy_newln);
#endif
		vsave();
		cnt = 1;
		if (value(AUTOINDENT)) {
			if (value(LISP))
				indent = lindent(dot + 1);
			else
			     if (!HADUP && vaifirst)
				indent = whitecnt(linebuf);
			vaifirst = 0;

#ifndef	NLS16
			strcLIN(vpastwh(gcursor + 1));
#else
			STRCLIN(vpastwh(ADVANCE(gcursor)));
#endif

			gcursor = genindent(indent);
			*gcursor = 0;

#ifndef	NLS16
			if (gcursor + strlen(linebuf) > &genbuf[LBSIZE - 2])
#else
			if (gcursor + STRLEN(linebuf) > &genbuf[LBSIZE - 2])
#endif

				gcursor = genbuf;

#ifndef	NLS16
			CP(gcursor, linebuf);
#else
			STRCPY(gcursor, linebuf);
#endif

		} else {

#ifndef	NLS16
			CP(genbuf, gcursor + 1);
#else
			STRCPY(genbuf, ADVANCE(gcursor));
#endif

			gcursor = genbuf;
		}

		/*
		 * If we started out as a single line operation and are now
		 * turning into a multi-line change, then we had better yank
		 * out dot before it changes so that undo will work
		 * correctly later.
		 */
		if (FIXUNDO && vundkind == VCHNG) {
			vremote(1, yank, 0);
			undap1--;
		}

		/*
		 * Now do the append of the new line in the buffer,
		 * and update the display.  If slowopen
		 * we don't do very much.
		 */
		vdoappend(genbuf);
		vundkind = VMANYINS;
		vcline++;
		if (state != VISUAL)
			vshow(dot, NOLINE);
		else {
			i += LINE(vcline - 1);
			vopen(dot, i);
			if (value(SLOWOPEN))
				vscrap();
			else
				vsync1(LINE(vcline));
		}
		vshowmode("INPUT MODE");

#ifndef	NLS16
		strcLIN(gcursor);
#else
		STRCLIN(gcursor);
#endif

		*gcursor = 0;
		cursor = linebuf;
		vgotoCL(qcolumn(cursor - 1, genbuf));
#ifdef TEPE
		/* hp3000 tepe testing prompt */
		vputc('\021');
#endif
#if defined NLS || defined NLS16
		if (right_to_left && ch != 'r' && opp_insert && escape == '\n') {
			tputs(unlock_keys, 1, putch);
			opp_lang();
		}
#endif
	}

	/*
	 * All done with insertion, position the cursor
	 * and sync the screen.
	 */
#if defined NLS || defined NLS16
	opp_insert = 0;
#endif
	hold = oldhold;
	if (cursor > linebuf)

#ifndef	NLS16
		cursor--;
#else
		PST_DEC(cursor);
#endif
	if (state != HARDOPEN)
		vsyncCL();
	else if (cursor > linebuf)
		back1();
	doomed = 0;
#if defined NLS16 && defined EUC
	b_doomed = 0 ;
#endif
	wcursor = cursor;
	vmove();
#ifdef SIGWINCH
	if (got_winch_insert)
		winch();
	ign_winch = 0;
	(void) signal (SIGWINCH, winch);
#endif SIGWINCH
}

/*
 * Subroutine for vgetline to back up a single character position,
 * backwards around end of lines (vgoto can't hack columns which are
 * less than 0 in general).
 */
back1()
{

	vgoto(destline - 1, WCOLS + destcol - 1);
}

/*
 * Get a line into genbuf after gcursor.
 * Cnt limits the number of input characters
 * accepted and is used for handling the replace
 * single character command.  Aescaped is the location
 * where we stick a termination indicator (whether we
 * ended with an ESCAPE or a newline/return.
 *
 * We do erase-kill type processing here and also
 * are careful about the way we do this so that it is
 * repeatable.  (I.e. so that your kill doesn't happen,
 * when you repeat an insert if it was escaped with \ the
 * first time you did it.  commch is the command character
 * involved, including the prompt for readline.
 */

CHAR *
vgetline(cnt, gcursor, aescaped, commch)
	int cnt;
	register CHAR *gcursor;
	bool *aescaped;
	char commch;
{
	register int c, ch;
	register CHAR *cp;
	int x, y, iwhite, backsl=0;
	CHAR *iglobp;
	CHAR cstr[2];
	int (*OO)() = Outchar;
#ifdef SIGWINCH
	int resize;
#endif SIGWINCH

#ifdef	NLS16
	char temp[LBSIZE];
#ifdef EUC
	int ex_colw ;
	int morebyte ;
#endif EUC
#endif
#if defined NLS || defined NLS16
	register bool new_state;
	register bool cur_state = 0;
	register bool opp_abbr = 0;
	register bool sim;
#endif

	/*
	 * Clear the output state and counters
	 * for autoindent backwards motion (counts of ^D, etc.)
	 * Remember how much white space at beginning of line so
	 * as not to allow backspace over autoindent.
	 */
	*aescaped = 0;
	ogcursor = gcursor;
	flusho();
	CDCNT = 0;
	HADUP = 0;
	HADZERO = 0;
	gobbled = 0;
	iwhite = whitecnt(genbuf);
	iglobp = vglobp;

	/*
	 * Carefully avoid using vinschar in the echo area.
	 */
	if (splitw)
		Outchar = vputchar;
	else {
		Outchar = vinschar;
		vprepins();
	}
	for (;;) {
		backsl = 0;
		if (gobblebl)
			gobblebl--;
		if (cnt != 0) {
			cnt--;
			if (cnt == 0)
				goto vadone;
		}

#ifdef SIGWINCH
		if (resize = setjmp(vgetline_env))
			goto vadone;
#endif SIGWINCH
		c = getkey();
		if (c != ATTN)

#ifndef	NLS16
			c &= (QUOTE|TRIM);
#else
			c &= (QUOTE | TRIM | FIRST | SECOND);
		/* if c is 1st byte, get one more byte for 2nd */
		if (IS_FIRST(c) && commch == 'r')
				cnt++;
#endif

		ch = c;
		maphopcnt = 0;
		if (vglobp == 0 && Peekkey == 0 && commch != 'r' && !tilde_cmd)
			while ((ch = map(c, immacs)) != c) {
				c = ch;
				if (!value(REMAP))
					break;
				if (++maphopcnt > 256)
					error((nl_msg(1, "Infinite macro loop")));
			}
		if (!iglobp) {

			/*
			 * Erase-kill type processing.
			 * Only happens if we were not reading
			 * from untyped input when we started.
			 * Map users erase to ^H, kill to -1 for switch.
			 */
#ifndef USG
			if (c == tty.sg_erase)
				c = CTRL(h);
			else if (c == tty.sg_kill)
				c = -1;
#else
			if (c == tty.c_cc[VERASE])
				c = CTRL(h);
			else if (c == tty.c_cc[VKILL])
				c = -1;
#endif
			switch (c) {

			/*
			 * ^?		Interrupt drops you back to visual
			 *		command mode with an unread interrupt
			 *		still in the input buffer.
			 *
			 * ^\		Quit does the same as interrupt.
			 *		If you are a ex command rather than
			 *		a vi command this will drop you
			 *		back to command mode for sure.
			 */
			case ATTN:
			case QUIT:
				ungetkey(c);
				goto vadone;

			/*
			 * ^H		Backs up a character in the input.
			 *
			 * BUG:		Can't back around line boundaries.
			 *		This is hard because stuff has
			 *		already been saved for repeat.
			 */
			case CTRL(h):
bakchar:

#ifndef	NLS16
				cp = gcursor - 1;
#else
				cp = REVERSE(gcursor);
#ifdef EUC
				morebyte = 0 ;
				if ( IS_FIRST( *cp ) ) {
					morebyte = 2 - C_COLWIDTH( ( (int)(*cp) ) & TRIM  ) ;
				}
#endif EUC
#endif

				if (cp < ogcursor) {
					if (splitw) {
						/*
						 * Backspacing over readecho
						 * prompt. Pretend delete but
						 * don't beep.
						 */
						ungetkey(c);
						goto vadone;
					}
					beep();
					continue;
				}
				goto vbackup;

			/*
			 * ^W		Back up a white/non-white word.
			 */
			case CTRL(w):
				wdkind = 1;

#ifndef	NLS16
#ifndef NONLS8 /* Character set features */
				for (cp = gcursor; cp > ogcursor && (cp[-1] >= IS_MACRO_LOW_BOUND) 
				    && _isspace(cp[-1] & TRIM); cp--)
#else NONLS8
				for (cp = gcursor; cp > ogcursor && (cp[-1] >= IS_MACRO_LOW_BOUND) 
				    && isspace(cp[-1]); cp--)
#endif NONLS8
#else
				for (cp = gcursor; cp > ogcursor && (*REVERSE(cp) >= IS_MACRO_LOW_BOUND) 
				    && _isspace(*REVERSE(cp) & TRIM); PST_DEC(cp))
#endif

					continue;

#ifndef	NLS16
				for (c = wordch(cp - 1);
				    cp > ogcursor && wordof(c, cp - 1); cp--)
					continue;
#else				/* back to the first of the previous word. */
#ifdef EUC
				morebyte = 0 ;
				for (c = wordch(REVERSE(cp));
				    cp > ogcursor && wordof(c, REVERSE(cp)); ) {
					if ( IS_FIRST( *REVERSE(cp) ) ) {
						morebyte += 2 - C_COLWIDTH( ( (int)( *(cp-2) ) )  & TRIM ) ;
					}
					PST_DEC(cp) ;
				}
#else EUC
				for (c = wordch(REVERSE(cp));
				    cp > ogcursor && wordof(c, REVERSE(cp)); PST_DEC(cp))
					continue;
#endif EUC
#endif

				goto vbackup;

			/*
			 * users kill	Kill input on this line, back to
			 *		the autoindent.
			 */
			case -1:
#if defined NLS16 && defined EUC
				morebyte = 0 ;
				for (cp = gcursor; cp > ogcursor ; ) {
					if ( IS_FIRST( *REVERSE(cp) ) ) {
						morebyte += 2 - C_COLWIDTH( ( (int)( *(cp-2) ) )  & TRIM ) ;
					}
					PST_DEC(cp) ;
				}
#else
				cp = ogcursor;
#endif
vbackup:
				if (cp == gcursor) {
					beep();
					continue;
				}
#ifdef	NLS
				sim = insmode;
#endif
				endim();
				*cp = 0;
				c = cindent();
				vgotoCL(qcolumn(cursor - 1, genbuf));
				if (doomed >= 0)
					doomed += c - cindent();
#if defined NLS16 && defined EUC
				if ( b_doomed >= 0 )
					b_doomed += c - cindent() + morebyte ;
#ifdef IDEBUG
				if( trace )
					fprintf ( trace, " \n vgetline, doomed=%d, b_doomed=%d \n", doomed, b_doomed ) ;
#endif IDEBUG
#endif
				gcursor = cp;
#if defined NLS || defined NLS16
				if (opp_insert) {
					if (opp_len) {
						opp_len--;
						if (opp_jump_insert && (ch == '\b' || opp_abbr) && (sim || opp_back)) {
							tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
							godm();
							vputp(delete_character, DEPTH(vcline));
							enddm();
							opp_back++;
							opp_abbr = 0;
							if (doomed) {
								tputs(tparm(cursor_address,opp_line,opp_col+opp_len),0,putch);
								goim();
								vputc(' ');
								endim();
								tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
								
							}
						}
					} else {
						opp_insert = 0;
						cur_state = 0;
					}
				}
#endif
				continue;

			/*
			 * \		Followed by erase or kill
			 *		maps to just the erase or kill.
			 */
			case '\\':
				x = destcol, y = destline;
				putchar('\\');
				vcsync();
				c = getkey();
#ifndef USG
				if (c == tty.sg_erase
				    || c == tty.sg_kill)
#else
				if (c == tty.c_cc[VERASE]
				    || c == tty.c_cc[VKILL])
#endif
				{
					vgoto(y, x);
					if (doomed >= 0) {
						doomed++;
#if defined NLS16 && defined EUC
						b_doomed++;
#endif
					}
					goto def;
				}
				ungetkey(c), c = '\\';
				backsl = 1;
				break;

			/*
			 * ^Q		Super quote following character
			 *		Only ^@ is verboten (trapped at
			 *		a lower level) and \n forces a line
			 *		split so doesn't really go in.
			 *
			 * ^V		Synonym for ^Q
			 */
			case CTRL(q):
			case CTRL(v):
				x = destcol, y = destline;
				putchar('^');
				vgoto(y, x);
				c = getkey();
#ifdef TIOCSETC
				if (c == ATTN)
					c = nttyc.t_intrc;
#endif
				if (c != NL) {
					if (doomed >= 0){
						doomed++;
#if defined NLS16 && defined EUC
						b_doomed++ ;
#endif
					}
					goto def;
				}
				break;
			}
		}

		/*
		 * If we get a blank not in the echo area
		 * consider splitting the window in the wrapmargin.
		 */
		if (c != NL && !splitw) {
			if (c == ' ' && gobblebl) {
				gobbled = 1;
				continue;
			}

#ifndef	NLS16
			if (value(WRAPMARGIN) &&
				(outcol >= OCOLUMNS - value(WRAPMARGIN) ||
				 backsl && outcol==0) &&
				commch != 'r') {
#else
			/*
			 * When the 1st byte is located just on the wrapmargin
			 * column, the below process should be invoked.
			 * Actually, this case does not across wrapmargin column
			 * yet. But to beep at the 1st byte, it it necessary
			 * to check ahead that the 2nd byte is located beyond
			 * wrapmargin column.
			 */
#ifdef EUC
			if ( IS_FIRST(c) ) {
				ex_colw = C_COLWIDTH(  c & TRIM  )-1 ;
			}else{
				ex_colw = 0 ;
			}
			if (value(WRAPMARGIN) &&
				(outcol >= OCOLUMNS - value(WRAPMARGIN) - ex_colw ||
				 backsl && outcol==0) &&
				commch != 'r') {
#else EUC
			if (value(WRAPMARGIN) &&
				(outcol >= OCOLUMNS - value(WRAPMARGIN) - IS_FIRST(c) ||
				 backsl && outcol==0) &&
				commch != 'r') {
#endif EUC
#endif

				/*
				 * At end of word and hit wrapmargin.
				 * Move the word to next line and keep going.
				 */
				wdkind = 1;
				*gcursor++ = c;
				if (backsl)
					*gcursor++ = getkey();
				*gcursor = 0;
				/*
				 * Find end of previous word if we are past it.
				 */

#ifndef NONLS8 /* Character set features */
#if defined NLS16 && defined EUC
				morebyte = 0 ;
				for (cp=gcursor; cp>ogcursor && (cp[-1] >= IS_MACRO_LOW_BOUND) 
				    && _isspace(cp[-1] & TRIM); cp--) {
					if ( IS_FIRST( *cp ) ) {
						morebyte += 2 - C_COLWIDTH( ( (int)(*cp) ) & TRIM ) ;
					}
				}
#else
				for (cp=gcursor; cp>ogcursor && (cp[-1] >= IS_MACRO_LOW_BOUND) 
				    && _isspace(cp[-1] & TRIM); cp--)
					;
#endif
#else NONLS8
				for (cp=gcursor; cp>ogcursor && (cp[-1] >= IS_MACRO_LOW_BOUND) 
				    && isspace(cp[-1]); cp--)
					;
#endif NONLS8


#ifndef	NLS16
				if (outcol+(backsl?OCOLUMNS:0) - (gcursor-cp) >= OCOLUMNS - value(WRAPMARGIN)) {
#else
#ifdef EUC
			if ( IS_FIRST(c) ) {
				ex_colw = C_COLWIDTH( c & TRIM )-1 ;
			}else{
				ex_colw = 0 ;
			}
				if (outcol+(backsl?OCOLUMNS:0) - (gcursor-cp) >= OCOLUMNS - value(WRAPMARGIN) - ex_colw ) {
#else EUC
				if (outcol+(backsl?OCOLUMNS:0) - (gcursor-cp) >= OCOLUMNS - value(WRAPMARGIN) - IS_FIRST(c)) {
#endif EUC
#endif

					/*
					 * Find beginning of previous word.
					 */

#ifndef NONLS8 /* Character set features */
#if defined NLS16 && defined EUC
					for (; cp>ogcursor && ((cp[-1] < IS_MACRO_LOW_BOUND) 
					    || !_isspace(cp[-1] & TRIM)); cp--) {
						if ( IS_FIRST( *cp ) ) {
							morebyte += 2 - C_COLWIDTH( ( (int)(*cp) ) & TRIM ) ;
						}
					}
#else
					for (; cp>ogcursor && ((cp[-1] < IS_MACRO_LOW_BOUND) 
					    || !_isspace(cp[-1] & TRIM)); cp--)
						;
#endif
#else NONLS8
					for (; cp>ogcursor && ((cp[-1] < IS_MACRO_LOW_BOUND) 	
					    || !isspace(cp[-1])); cp--)
						;
#endif NONLS8

					if (cp <= ogcursor) {
						/*
						 * There is a single word that
						 * is too long to fit.  Just
						 * let it pass, but beep for
						 * each new letter to warn
						 * the luser.
						 */
						c = *--gcursor;
						*gcursor = 0;

#ifndef	NLS16
						beep();
#else
						/*
						 * Beep once for a character.
						 * Beep should not be outputted
						 * before outputting the 2nd
						 * byte.
						 */
						if (!IS_SECOND(c))
							beep();
#endif

						goto dontbreak;
					}
					/*
					 * Save it for next line.
					 */

#ifndef	NLS16
					macpush(cp, 0);
#else
					MACPUSH(cp, 0);
#endif

					cp--;
				}
				macpush("\n", 0);
				/*
				 * Erase white space before the word.
				 */

#ifndef NONLS8 /* Character set features */
				while (cp > ogcursor && (cp[-1] >= IS_MACRO_LOW_BOUND) && _isspace(cp[-1] & TRIM))
#else NONLS8
				while (cp > ogcursor && (cp[-1] >= IS_MACRO_LOW_BOUND) && isspace(cp[-1]))
#endif NONLS8

					cp--;	/* skip blank */
#if defined NLS || defined NLS16
				/* be sure to keep Arabic tails */
				if (right_to_left && (rl_lang == ARABIC) && ((*cp&TRIM) == alt_space)) {
					int c = *(cp-1);
					if (TAMDEED(c) || BAA_Q(c) || SEEN_Q(c)) {
						cp++;
					}
				}
#endif
				gobblebl = 3;
				goto vbackup;
			}
		dontbreak:;
		}
#if defined NLS16 && defined EUC
		morebyte = 0 ;
#endif

		/*
		 * Word abbreviation mode.
		 */
		cstr[0] = c;

#ifndef	NLS16
		if (anyabbrs && !tilde_cmd && gcursor > ogcursor && !wordch(cstr) && wordch(gcursor-1)) {
#else
		if (anyabbrs && !tilde_cmd && gcursor > ogcursor && wordch(REVERSE(gcursor)) && wordch(cstr) != wordch(REVERSE(gcursor))) {
		/*  check the previous word whether it is defined as an abbreviation
		**  when input character is of the different type from the previous.
		*/
#endif

				int wdtype, abno;

				cstr[1] = 0;
				wdkind = 1;

#ifndef	NLS16
				cp = gcursor - 1;
				for (wdtype = wordch(cp - 1);
				    cp > ogcursor && wordof(wdtype, cp - 1); cp--)
					;
#else				/*  a sequence of characters of the same type is considerd as a
				**  word.  ( but any character is allowed as the last of word,
				**   e.g. "ABC%" is a possible word here. )
				*/
				cp = REVERSE(gcursor);
				for (wdtype = wordch(REVERSE(cp));
				    cp > ogcursor && wordof(wdtype, REVERSE(cp)); PST_DEC(cp))
#ifdef EUC
				{
					if ( IS_FIRST( *REVERSE(cp) ) ) {
						morebyte += 2 - C_COLWIDTH( ( (int)( *(cp-2) ) )  & TRIM ) ;
					}
				}
#else EUC
					;
#endif EUC
#endif

				*gcursor = 0;

#ifdef	NLS16
				CHAR_TO_char(cp, temp);
#endif

				for (abno=0; abbrevs[abno].mapto; abno++) {

#ifndef	NLS16
					if (eq(cp, abbrevs[abno].cap)) {
#else
					if (eq(temp, abbrevs[abno].cap)) {
#endif


#ifndef	NLS16
						macpush(cstr, 0);
#else
						MACPUSH(cstr, 0);
#endif

						macpush(abbrevs[abno].mapto);
#if defined NLS || defined NLS16
						if (right_to_left && opp_jump_insert && OPP_LANG(c))
							opp_abbr = 1;
#endif
						goto vbackup;
					}
				}
		}

		switch (c) {

		/*
		 * ^M		Except in repeat maps to \n.
		 */
		case CR:
			if (vglobp)
				goto def;
			c = '\n';
			if (commch == 'R' && value(AUTOINDENT)) repbreak=1;
			/* presto chango ... */

		/*
		 * \n		Start new line.
		 */
		case NL:
			*aescaped = c;
			goto vadone;

		/*
		 * escape	End insert unless repeat and more to repeat.
		 */
		case ESCAPE:
			if (repbreak) endbreak=1;
			/* check if two escapes are needed to end input mode (except in echo area) */
			if (value(DOUBLEESCAPE) && !splitw) {
				if (peekkey() == ESCAPE)
					c = getkey();	/* yes, eat the 2nd escape */
				else {
					beep();		/* yes, but only got a single escape */
					goto def;	/* so beep and then insert it        */
				}
			}
			if (lastvgk)
				goto def;
			goto vadone;

		/*
		 * ^D		Backtab.
		 * ^T		Software forward tab.
		 *
		 *		Unless in repeat where this means these
		 *		were superquoted in.
		 */
		case CTRL(d):
		case CTRL(t):
			if (vglobp)
				goto def;
			/* fall into ... */

		/*
		 * ^D|QUOTE	Is a backtab (in a repeated command).
		 */
#ifndef NLS16
		case '\204'&0377:
#else
		case CTRL(d) | QUOTE:
#endif

		/*
		 * ^T|QUOTE	Is a forward tab (in a repeated command).
		 */
#ifndef NLS16
		case '\224'&0377:
#else
		case CTRL(t) | QUOTE:
#endif
			*gcursor = 0;
			cp = vpastwh(genbuf);
			c = whitecnt(genbuf);
			if ((ch & TRIM) == CTRL(t)) {
				/*
				 * ^t just generates new indent replacing
				 * current white space rounded up to soft
				 * tab stop increment.
				 */
				/*
				 * HP extension: ^T anywhere other than right
				 *               after initial white space now
				 *		 works properly.
				 */
				if (cp != gcursor || cursor != linebuf) {
					/* where we are */
					x = destcol + WCOLS * (destline - LINE(vcline));
					/* where we want to go */
					c = backtab(x + value(SHIFTWIDTH) + 1);
					/* back up over any spaces at the end of the current input */
					while ((*(gcursor-1) & TRIM) == ' '
#if defined NLS || defined NLS16
						|| (*(gcursor-1) & TRIM) == alt_space
#endif
					) {
						gcursor--;
						x--;
						doomed++;
#if defined NLS16 && defined EUC
						b_doomed++;
#endif
					}
#if defined NLS || defined NLS16
					if (opp_insert) {
						cur_state = opp_insert = opp_len = 0;
					}
#endif
					vgotoCL(x);
					/* go to destination first with tabs */
					while ((y = (x + value(TABSTOP) - (x % value(TABSTOP)))) <= c) {
						putchar( *gcursor++ = '\t' );
						x = y;
					}
					/* and then with spaces */
					for (; x < c; x++)
#if defined NLS || defined NLS16
						putchar( *gcursor++ = right_to_left ? alt_space : ' ' );
#else
						putchar( *gcursor++ = ' ' );
#endif
					flush();
					continue;
				}
				/*
				 * ^T at the beginning of a line or after initial white space
				 */
				cp = genindent(iwhite = backtab(c + value(SHIFTWIDTH) + 1));
				/* ^T is opposite of ^D for repeats */
				CDCNT--;
				/* update screen with each indent character */
#if defined NLS || defined NLS16
				if (opp_insert) {
					opp_line = destline;
					opp_col = destcol;
				}
#endif
				vigotoCL(value(NUMBER) ? 8 : 0);
				doomed += c;
#if defined NLS16 && defined EUC
				b_doomed += c ;
#endif
				for (ogcursor = genbuf; ogcursor < cp; ogcursor++)
					putchar(*ogcursor);
				flush();
				gcursor = cp;
				continue;
			}
			/*
			 * ^D works only if we are at the (end of) the
			 * generated autoindent.  We count the ^D for repeat
			 * purposes.
			 */
			if (c == iwhite && c != 0)
				if (cp == gcursor) {
					iwhite = backtab(c);
					CDCNT++;
					ogcursor = cp = genindent(iwhite);
					/*
					 * The following is because the code at vbackup decides
					 * whether the ^D was successful based upon cp being not
					 * the same as gcursor.  But with certain combinations of
					 * shiftwidth and tabstop, the length of the new indent
					 * string generated by genindent can be the same length
					 * as the previous indent string.  E.g., with sw=4 & ts=5
					 * an indent of "\t   " (string len=4, column pos=8)
					 * becomes after a ^D: "    " (string len=4, column pos=4).
					 */
					if (c != iwhite)
						gcursor = 0;
#if defined NLS16 && defined EUC
					morebyte = 0 ;
#endif
					goto vbackup;
				} else if (&cp[1] == gcursor &&
#if defined NLS || defined NLS16
				    (*cp == '^' || *cp == '0' ||
				    (right_to_left && (*cp == alt_uparrow || *cp == alt_zero)))) {
#else
				    (*cp == '^' || *cp == '0')) {
#endif
					/*
					 * ^^D moves to margin, then back
					 * to current indent on next line.
					 *
					 * 0^D moves to margin and then
					 * stays there.
					 */
#if defined NLS || defined NLS16
					HADZERO = *cp == '0' || (right_to_left && *cp == alt_zero);
					if (HADZERO && right_to_left)
						cur_state = opp_insert = opp_len = 0;
#else
					HADZERO = *cp == '0';
#endif
					ogcursor = cp = genbuf;
					HADUP = 1 - HADZERO;
					CDCNT = 1;
					endim();
					back1();
					vputchar(' ');
#if defined NLS16 && defined EUC
					morebyte = 0 ;
#endif
					goto vbackup;
				}
			if (vglobp && vglobp - iglobp >= 2 &&
			    (vglobp[-2] == '^' || vglobp[-2] == '0')
			    && gcursor == ogcursor + 1)
				goto bakchar;
			continue;

		default:
			/*
			 * Possibly discard control inputs.
			 */
			if (!vglobp && junk(c)) {
				beep();
				continue;
			}
def:
			if (!backsl) {
				int cnt;
#if defined NLS || defined NLS16
				if (right_to_left) {
					if (new_state = OPP_LANG(c)) {
						opp_insert = 1;
						if ((new_state != cur_state) || (opp_col+opp_len >= WCOLS)) {
							opp_len = 1;
							opp_fix = 0;
							opp_back = 0;
							opp_numtab = 0;
							opp_eattab = 0;
							opp_line = destline;
							opp_col = destcol;
							opp_inscol = destcol + (destline - LINE(vcline)) * WCOLS;
							if (cur_state)
								tputs(tparm(cursor_address,destline,destcol),0,putch);
						} else {
							opp_len++;
						}
					} else {
						if (cur_state)
							tputs(tparm(cursor_address,opp_line,opp_col+opp_len),0,putch);
						if (opp_back || opp_numtab)
							endim();
						opp_insert = 0;
						opp_len = 0;
						opp_back = 0;
						opp_numtab = 0;
						opp_eattab = 0;
					}
					cur_state = new_state;
				}
#endif
				putchar(c);
				flush();
			}
			if (gcursor > &genbuf[LBSIZE - 2])
				error((nl_msg(2, "Line too long")));

#ifndef	NLS16
			*gcursor++ = c & TRIM;
#else
			*gcursor++ = c & (TRIM | FIRST | SECOND);
#endif
			vcsync();
			if (value(SHOWMATCH) && !iglobp)
				if (c == ')' || c == '}')
					lsmatch(gcursor);
			continue;
		}
	}
vadone:
	*gcursor = 0;
	if (Outchar != termchar)
		Outchar = OO;
	endim();
	tilde_cmd = 0;
	return (gcursor);
}

int	vgetsplit();
CHAR	*vsplitpt;

/*
 * Append the line in buffer at lp
 * to the buffer after dot.
 */
vdoappend(lp)
	CHAR *lp;
{
	register int oing = inglobal;

	vsplitpt = lp;
	inglobal = 1;
	ignore(append(vgetsplit, dot));
	inglobal = oing;
}

/*
 * Subroutine for vdoappend to pass to append.
 */
vgetsplit()
{

	if (vsplitpt == 0)
		return (EOF);

#ifndef	NLS16
	strcLIN(vsplitpt);
#else
	STRCLIN(vsplitpt);
#endif

	vsplitpt = 0;
	return (0);
}

/*
 * Vmaxrep determines the maximum repetitition factor
 * allowed that will yield total line length less than
 * LBSIZE characters and also does hacks for the R command.
 */
vmaxrep(ch, cnt, indent)
	char ch;
	register int cnt, indent;
{
	register int len, replen;
	register int ntab, nspace;

#ifdef	NLS16
	/*
	 * Klen and kreplen indicate number of CHARACTERS insted of
	 * len and replen, which indicate number of BYTES, respectively.
	 */
	register int klen, kreplen;
#endif
	
	if (cnt > LBSIZE - 2)
		cnt = LBSIZE - 2;

#ifndef	NLS16
	replen = strlen(genbuf);
#else
	replen = STRLEN(genbuf);
#endif

	if (ch == 'R') {

#ifndef	NLS16
		len = strlen(cursor);
		if (replen < len)
			len = replen;
		CP(cursor, cursor + len);
#else
		len = STRLEN(cursor);
		kreplen = kstrlen(genbuf);

		/* DTS: FSDlj09398:
		 * readjust the number of character in linebuf to be
		 * erased. This is needed to correct the losing character
		 * problem when a <CR> is entered inside a R command.
		 */
		if (repbreak && indent && value(AUTOINDENT)) {
			ntab     = (int) indent / value(TABSTOP);
			nspace   = indent % value(TABSTOP);
			kreplen -= (ntab + nspace);
			if (endbreak) endbreak = repbreak = 0;
		}
		klen = kstrlen(cursor);
		if (kreplen < klen)	{
			klen = kreplen;
			len = replen;
		}
		/*
		 * The number of bytes to be deleted depends on the number
		 * of CHARACTERS to be replaced.
		 */
		STRCPY(cursor, NADVANCE(cursor, klen));
#endif

		vUD2 += len;
	}

#ifndef	NLS16
	len = strlen(linebuf);
#else
	len = STRLEN(linebuf);
#endif

	if (len + cnt * replen <= LBSIZE - 2)
		return (cnt);

	/************
	* FSDlj06757
	* Possibility of dividing by zero which causes a core dump.  This
	* happens when you try an add a carriage return inside a line 
	* of 1023 or 1024 characters, replen = 0, and the above test fails.
	* After an input file is read into vi, vi doesn't allow you to
	* create lines greater than 1022 chars.  However, on input, vi
	* used to allow lines up to 1024 chars.  This has also been fixed
	* to only allow lines up to 1022 chars.  Now, we should never get
	* this division by zero, but just in case.....
	*************/
	if (replen == 0)
		return (cnt);
	else
		cnt = (LBSIZE - 2 - len) / replen;
	if (cnt == 0) {
		vsave();
		error((nl_msg(3, "Line too long")));
	}
	return (cnt);
}

#ifdef SIGWINCH
ins_winch()
{
	got_winch_insert = 1;
	longjmp(vgetline_env, 1);
}
#endif SIGWINCH
