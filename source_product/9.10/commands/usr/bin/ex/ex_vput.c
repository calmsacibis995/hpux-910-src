/* @(#) $Revision: 66.4 $ */   
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifndef NONLS8 /* User messages */
# define	NL_SETN	19	/* set number */
# include	<msgbuf.h>
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Deal with the screen, clearing, cursor positioning, putting characters
 * into the screen image, and deleting characters.
 * Really hard stuff here is utilizing insert character operations
 * on intelligent terminals which differs widely from terminal to terminal.
 */
vclear()
{

#ifdef TRACE
	if (trace)
		tfixnl(), fprintf(trace, "------\nvclear, clear_screen '%s'\n", clear_screen);
#endif
	tputs(clear_screen, lines, putch);
	destcol = 0;
	outcol = 0;
	destline = 0;
	outline = 0;
	if (inopen)
#if defined NLS16 && defined EUC
		vclrbyte(vtube0, BTOCRATIO * WCOLS * (WECHO - ZERO + 1));
#else
		vclrbyte(vtube0, WCOLS * (WECHO - ZERO + 1));
#endif
}

/*
 * Clear memory.
 */
vclrbyte(cp, i)

#ifndef NONLS8 /* 8bit integrity */
	register short *cp;
#else NONLS8
	register char *cp;
#endif NONLS8

	register int i;
{

	if (i > 0)
		do
			*cp++ = 0;
		while (--i != 0);
}

/*
 * Clear a physical display line, high level.
 */
vclrlin(l, tp)
	int l;
	line *tp;
{

	vigoto(l, 0);
#if defined NLS || defined NLS16
	if ((hold & HOLDAT) == 0) {
		register int sof = opp_fix;
		opp_fix = 1;
		if (right_to_left && (rl_mode == NL_NONLATIN)) {
			putchar(tp > dol ? ((UPPERCASE || tilde_glitch) ? alt_uparrow : alt_tilde) : alt_amp);
		} else {
			putchar(tp > dol ? ((UPPERCASE || tilde_glitch) ? '^' : '~') : '@');
		}
		opp_fix = sof;
	}
#else
	if ((hold & HOLDAT) == 0)
		putchar(tp > dol ? ((UPPERCASE || tilde_glitch) ? '^' : '~') : '@');
#endif
	if (state == HARDOPEN)
		sethard();
	vclreol();
}

/*
 * Clear to the end of the current physical line
 */
vclreol()
{
	register int i, j;
#if defined NLS16 && defined EUC
	int col_vtub ;
	int i_vtub ;
#endif

#ifndef NONLS8 /* 8bit integrity */
	register short *tp;
#else NONLS8
	register char *tp;
#endif NONLS8

#ifdef ADEBUG
	if (trace)
		fprintf(trace, "vclreol(), destcol %d, ateopr() %d\n", destcol, ateopr());
#endif
	if (destcol == WCOLS)
		return;
	destline += destcol / WCOLS;
	destcol %= WCOLS;
	if (destline < 0 || destline > WECHO)
		error((nl_msg(1, "Internal error: vclreol")));
	i = WCOLS - destcol;
#if defined NLS16 && defined EUC
	col_vtub = vcolumn( vtube[destline], destcol ) ;
	i_vtub = BTOCRATIO * WCOLS - col_vtub ;
	tp = vtube[destline] + col_vtub ;
#else
	tp = vtube[destline] + destcol;
#endif
	if (clr_eol) {
		if (insert_null_glitch && *tp || !ateopr()) {
			vcsync();
			vputp(clr_eol, 1);
		}
#if defined NLS16 && defined EUC
		vclrbyte(tp, i_vtub) ;
#else
		vclrbyte(tp, i);
#endif
		return;
	}
	if (*tp == 0)
		return;
#if defined NLS16 && defined EUC
	while (i_vtub > 0 && (j = *tp & (QUOTE|TRIM))) {
#else
	while (i > 0 && (j = *tp & (QUOTE|TRIM))) {
#endif
		if (j != ' ' && (j & QUOTE) == 0) {
			destcol = WCOLS - i;
			vputchar(' ');
		}
#if defined NLS16 && defined EUC
		--i_vtub ;
		if ( IS_FIRST( *tp ) ) {
			i += ( 2 - C_COLWIDTH( ( (int)(*tp) ) & TRIM  ) ) ;
		}
#endif
		--i, *tp++ = 0;
	}
}

/*
 * Clear the echo line.
 * If didphys then its been cleared physically (as
 * a side effect of a clear to end of display, e.g.)
 * so just do it logically.
 * If work here is being held off, just remember, in
 * heldech, if work needs to be done, don't do anything.
 */
vclrech(didphys)
	bool didphys;
{

#ifdef ADEBUG
	if (trace)
		fprintf(trace, "vclrech(%d), Peekkey %d, hold %o\n", didphys, Peekkey, hold);
#endif
	if (Peekkey == ATTN)
		return;
	if (hold & HOLDECH) {
		heldech = !didphys;
		return;
	}
	if (!didphys && (clr_eos || clr_eol)) {
		splitw++;
		/*
		 * If display is retained below, then MUST use clr_eos or
		 * clr_eol since we don't really know whats out there.
		 * Vigoto might decide (incorrectly) to do nothing.
		 */
		if (memory_below) {
			vgoto(WECHO, 0);
			/*
			 * This is tricky.  If clr_eos is as cheap we
			 * should use it, so we don't have extra junk
			 * floating around in memory below.  But if
			 * clr_eol costs less we should use it.  The real
			 * reason here is that clr_eos is incredibly
			 * expensive on the HP 2626 (1/2 second or more)
			 * which makes ^D scroll really slow.  But the
			 * 2621 has a bug that shows up if we use clr_eol
			 * instead of clr_eos, so we make sure the costs
			 * are equal so it will prefer clr_eol.
			 */
			 /* But we must prevent extra junk floating	*/
			 /* around in memory below, so we use clr_eos.	*/
			if (clr_eos)
				vputp(clr_eos, 1);
			else
				vputp(clr_eol, 1);
		} else {
			if (teleray_glitch) {
				/* This code basically handles the t1061
				 * where positioning at (0, 0) won't work
				 * because the terminal won't let you put
				 * the cursor on it's magic cookie.
				 *
				 * Should probably be ceol_standout_glitch
				 * above, or even a
				 * new glitch, but right now t1061 is the
				 * only terminal with teleray_glitch.
				 */
				vgoto(WECHO, 0);
				vputp(delete_line, 1);
			} else {
				vigoto(WECHO, 0);
				vclreol();
			}
		}
		splitw = 0;
		didphys = 1;
	}
	if (didphys)
#if defined NLS16 && defined EUC
		vclrbyte(vtube[WECHO], BTOCRATIO * WCOLS);
#else
		vclrbyte(vtube[WECHO], WCOLS);
#endif
	heldech = 0;
}

/*
 * Fix the echo area for use, setting
 * the state variable splitw so we wont rollup
 * when we move the cursor there.
 */
fixech()
{

	splitw++;
	if (state != VISUAL && state != CRTOPEN) {
		vclean();
		vcnt = 0;
	}
	vgoto(WECHO, 0); flusho();
}

/*
 * Put the cursor ``before'' cp.
 */
vcursbef(cp)
	register CHAR *cp;
{

	if (cp <= linebuf)
		vgotoCL(value(NUMBER) << 3);
	else

#ifndef	NLS16
		vgotoCL(column(cp - 1) - 1);
#else
		if (IS_SECOND(*(cp - 1)))
#ifdef EUC
			vgotoCL(column(cp - 1) - C_COLWIDTH( ( (int)(*( cp -2 )) ) & TRIM ) );
#else EUC
			vgotoCL(column(cp - 1) - 2);
#endif EUC
		else
			vgotoCL(column(cp - 1) - 1);
#endif

}

/*
 * Put the cursor ``at'' cp.
 */
vcursat(cp)
	register CHAR *cp;
{

	if (cp <= linebuf && linebuf[0] == 0)
		vgotoCL(value(NUMBER) << 3);
	else
		vgotoCL(column(cp - 1));
}

/*
 * Put the cursor ``after'' cp.
 */
vcursaft(cp)
	register CHAR *cp;
{

#ifndef	NLS16
	vgotoCL(column(cp));
#else
	if (IS_FIRST(*cp))
#ifdef EUC
		vgotoCL(column(cp) + C_COLWIDTH( ( (int)(*cp) ) & TRIM ) - 1 ) ;
#else EUC
		vgotoCL(column(cp) + 1) ;
#endif EUC
	else
		vgotoCL(column(cp));
#endif

}

/*
 * Fix the cursor to be positioned in the correct place
 * to accept a command.
 */
vfixcurs()
{

	vsetcurs(cursor);
}

/*
 * Compute the column position implied by the cursor at ``nc'',
 * and move the cursor there.
 */
vsetcurs(nc)
	register CHAR *nc;
{
	register int col;

	col = column(nc);
	if (linebuf[0])
		col--;
	vgotoCL(col);
	cursor = nc;
}

/*
 * Move the cursor invisibly, i.e. only remember to do it.
 */
vigoto(y, x)
	int y, x;
{

	destline = y;
	destcol = x;
}

/*
 * Move the cursor to the position implied by any previous
 * vigoto (or low level hacking with destcol/destline as in readecho).
 */
vcsync()
{

	vgoto(destline, destcol);
}

/*
 * Goto column x of the current line.
 */
vgotoCL(x)
	register int x;
{

	if (splitw)
		vgoto(WECHO, x);
	else
		vgoto(LINE(vcline), x);
}

/*
 * Invisible goto column x of current line.
 */
vigotoCL(x)
	register int x;
{

	if (splitw)
		vigoto(WECHO, x);
	else
		vigoto(LINE(vcline), x);
}

/*
 * Show the current mode in the right hand part of the echo line,
 * then return the cursor to where it is now.
 */
vshowmode(msg)
char *msg;
{
	char locmsg[20];
	int savecol, saveline, savesplit;
	char *p;

	if (!value(SHOWMODE))
		return;
	/* Don't flash it for ".", macros, etc. */
	/*
	** ... unless we are at the end of the macro and thus we know the
	** macro left the user in some mode that they should know about.
	** This is primarily intended for users that want to use macros to
	** map their insert key to the "i" command.  We should also catch
	** the case where the mapping is to, for example, "istart>" (i.e.,
	** the macro puts the user in insert mode and supplied the 1st six
	** characters of input, letting the user supply the rest).  But
	** it would be much more difficult to detect such cases and they
	** should be relatively rare.
	*/
	if ((vmacp && *vmacp) || vglobp)
		if (*msg)
			return;
	savecol = outcol; saveline = outline; savesplit = splitw;
	splitw = 1;	/* To avoid scrolling */
	vigoto(WECHO, WCOLS-20);

	if (*msg) {
		strcpy(locmsg, msg);
		if (value(TERSE))
			locmsg[1] = 0;	/* only show first char */
		vcsync();
		for (p=locmsg; *p; p++)
			vputchar(*p);
	} else {
		/*
		 * Going back to command mode - clear the message.
		 */
		vclreol();
	}

	FLAGS(WECHO) |= VDIRT;
	vgoto(saveline, savecol);
	splitw = savesplit;
}

/*
 * Move cursor to line y, column x, handling wraparound and scrolling.
 */
vgoto(y, x)
	register int y, x;
{


#ifndef NONLS8 /* 8bit integrity */
	register short *tp;
#else NONLS8
	register char *tp;
#endif NONLS8

	register int c;

	/*
	 * Fold the possibly too large value of x.
	 */
	if (x >= WCOLS) {
		y += x / WCOLS;
		x %= WCOLS;
	}
	if (y < 0) {
		error((nl_msg(2, "Internal error: vgoto")));
	}
	if (outcol >= WCOLS) {
		if (auto_right_margin) {
			outline += outcol / WCOLS;
			outcol %= WCOLS;
		} else
			outcol = WCOLS - 1;
	}

	/*
	 * In a hardcopy or glass crt open, print the stuff
	 * implied by a motion, or backspace.
	 */
	if (state == HARDOPEN || state == ONEOPEN) {
		if (y != outline)
			error((nl_msg(3, "Line too long for open")));
		if (x + 1 < outcol - x || (outcol > x && !cursor_left))
			destcol = 0, fgoto();
#if defined NLS16 && defined EUC
		tp = vtube[WBOT] + vcolumn( vtube[WBOT], outcol ) ;
#else
		tp = vtube[WBOT] + outcol;
#endif
		while (outcol != x)
			if (outcol < x) {
				if (*tp == 0)
					*tp = ' ';

#ifndef	NLS16
				c = *tp++ & TRIM;
#else
				c = *tp++ & (TRIM | FIRST | SECOND);
#endif

				vputc(c && (!over_strike || erase_overstrike) ? c : ' '), outcol++;
#if defined NLS16 && defined EUC
				if ( IS_FIRST( *(tp-1) ) ) {
					outcol -= ( 2 - C_COLWIDTH( ( (int)(*(tp-1)) ) & TRIM ) ) ;
				}
#endif
			} else {
				vputp(cursor_left, 0);
				outcol--;
			}
		destcol = outcol = x;
		destline = outline;
		return;
	}

	/*
	 * If the destination position implies a scroll, do it.
	 */
	destline = y;
	if (destline > WBOT && (!splitw || destline > WECHO)) {
		endim();
		vrollup(destline);
	}

	/*
	 * If there really is a motion involved, do it.
	 * The check here is an optimization based on profiling.
	 */
	destcol = x;
	if ((destline - outline) * WCOLS != destcol - outcol) {
		if (!move_insert_mode)
			endim();
		fgoto();
	}
}

/*
 * This is the hardest code in the editor, and deals with insert modes
 * on different kinds of intelligent terminals.  The complexity is due
 * to the cross product of three factors:
 *
 *	1. Lines may display as more than one segment on the screen.
 *	2. There are 2 kinds of intelligent terminal insert modes.
 *	3. Tabs squash when you insert characters in front of them,
 *	   in a way in which current intelligent terminals don't handle.
 *
 * The two kinds of terminals are typified by the DM2500 or HP2645 for
 * one and the CONCEPT-100 or the FOX for the other.
 *
 * The first (HP2645) kind has an insert mode where the characters
 * fall off the end of the line and the screen is shifted rigidly
 * no matter how the display came about.
 *
 * The second (CONCEPT-100) kind comes from terminals which are designed
 * for forms editing and which distinguish between blanks and ``spaces''
 * on the screen, spaces being like blank, but never having had
 * and data typed into that screen position (since, e.g. a clear operation
 * like clear screen).  On these terminals, when you insert a character,
 * the characters from where you are to the end of the screen shift
 * over till a ``space'' is found, and the null character there gets
 * eaten up.
 *
 *
 * The code here considers the line as consisting of several parts
 * the first part is the ``doomed'' part, i.e. a part of the line
 * which is being typed over.  Next comes some text up to the first
 * following tab.  The tab is the next segment of the line, and finally
 * text after the tab.
 *
 * We have to consider each of these segments and the effect of the
 * insertion of a character on them.  On terminals like HP2645's we
 * must simulate a multi-line insert mode using the primitive one
 * line insert mode.  If we are inserting in front of a tab, we have
 * to either delete characters from the tab or insert white space
 * (when the tab reaches a new spot where it gets larger) before we
 * insert the new character.
 *
 * On a terminal like a CONCEPT our strategy is to make all
 * blanks be displayed, while trying to keep the screen having ``spaces''
 * for portions of tabs.  In this way the terminal hardward does some
 * of the hacking for compression of tabs, although this tends to
 * disappear as you work on the line and spaces change into blanks.
 *
 * There are a number of boundary conditions (like typing just before
 * the first following tab) where we can avoid a lot of work.  Most
 * of them have to be dealt with explicitly because performance is
 * much, much worse if we don't.
 *
 * A final thing which is hacked here is two flavors of insert mode.
 * Datamedia's do this by an insert mode which you enter and leave
 * and by having normal motion character operate differently in this
 * mode, notably by having a newline insert a line on the screen in
 * this mode.  This generally means it is unsafe to move around
 * the screen ignoring the fact that we are in this mode.
 * This is possible on some terminals, and wins big (e.g. HP), so
 * we encode this as a ``can move in insert capability'' mi,
 * and terminals which have it can do insert mode with much less
 * work when tabs are present following the cursor on the current line.
 */

/*
 * Routine to expand a tab, calling the normal Outchar routine
 * to put out each implied character.  Note that we call outchar
 * with a QUOTE.  We use QUOTE internally to represent a position
 * which is part of the expansion of a tab.
 */
vgotab()
{
	register int i = tabcol(destcol, value(TABSTOP)) - destcol;

	do
		(*Outchar)(QUOTE);
	while (--i);
}

/*
 * Variables for insert mode.
 */
int	linend;			/* The column position of end of line */
int	tabstart;		/* Column of start of first following tab */
int	tabend;			/* Column of end of following tabs */
int	tabsize;		/* Size of the following tabs */
int	tabslack;		/* Number of ``spaces'' in following tabs */
int	inssiz;			/* Number of characters to be inserted */
#if defined NLS16 && defined EUC
int	b_inssiz ;		/* Number of bytes to be inserted	*/
#endif
int	inscol;			/* Column where insertion is taking place */
int	shft;			/* Amount tab expansion shifted rest of line */
int	slakused;		/* This much of tabslack will be used up */

#ifdef	NLS16
int	beftabwrap;		/*
				 * Number of wraparounding bytes until
				 * tabstart. This is not always 1 because
				 * DUMMY_BLANK causes beftabwrap to be increased
				 * with the historical wraparound in
				 * multi-line text.
				 */
int	needspace;		/*
				 * Number of columns to be required for future
				 * tab shifting. This is determined by both
				 * beftabwrap and current tab condition.
				 */

int	afttabwrap;		/*
				 * Number of wraparounding columns until
				 * tabend. This is determined by needspace.
				 * This is power of value(TABSTOP).
				 */
int	linendwrap;		/*
				 * Number of final wraparounding columns.
				 */

#ifdef EUC
int 	extr_byte ;		/*
				 * Number of extra byte for the character
				 the byte size of which is greater than 
				 the column size
				 */


	short vlinebuf[TUBESIZE] ; /* the buffer for insertions about the	*/
				 /* screen image buffer			*/


	short *vbuf0 ;		/* the pointer indicates the beginning	*/
				/* of vlinebuf[]			*/

/* to see the screen image buffer	*/
#ifdef TRACE
	char cvlinebuf[TUBESIZE] ;
	char *cvbuf ;
#endif TRACE


#endif EUC
#endif
/*
 * This routine MUST be called before insert mode is run,
 * and brings all segments of the current line to the top
 * of the screen image buffer so it is easier for us to
 * maniuplate them.
 */
vprepins()
{
	register int i;

#ifndef NONLS8 /* 8bit integrity */
	register short *cp = vtube0;
#else NONLS8
	register char *cp = vtube0;
#endif NONLS8

#if defined NLS16 && defined EUC
	short *vcp  ;
	int bytesize ;
	vbuf0 = vlinebuf ;
	vcp = vbuf0 ;
	vclrbyte( vcp, TUBESIZE ) ;
#endif

	for (i = 0; i < DEPTH(vcline); i++) {
		vmaktop(LINE(vcline) + i, cp);
#if defined NLS16 && defined EUC
		bytesize = CTOBSIZE( cp, WCOLS ) ;
		copys( vcp, cp, bytesize ) ;
		vcp += bytesize ;
		cp += BTOCRATIO * WCOLS ;
#else
		cp += WCOLS;
#endif
	}
}

vmaktop(p, cp)
	register int p;

#ifndef NONLS8 /* 8bit integrity */
	short *cp;
#else NONLS8
	char *cp;
#endif NONLS8

{
	register int i;

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
	short temp[VBUFCOLS];
#else
	short temp[TUBECOLS];
#endif
#else NONLS8
	char temp[TUBECOLS];
#endif NONLS8


	if (p < 0 || vtube[p] == cp)
		return;
	for (i = ZERO; i <= WECHO; i++)
		if (vtube[i] == cp) {

#ifndef NONLS8 /* Character set features */
#if  defined NLS16 && defined EUC
			copys(temp, vtube[i], BTOCRATIO * WCOLS);
			copys(vtube[i], vtube[p], BTOCRATIO * WCOLS);
			copys(vtube[p], temp, BTOCRATIO * WCOLS);
#else
			copys(temp, vtube[i], WCOLS);
			copys(vtube[i], vtube[p], WCOLS);
			copys(vtube[p], temp, WCOLS);
#endif
#else NONLS8
			copy(temp, vtube[i], WCOLS);
			copy(vtube[i], vtube[p], WCOLS);
			copy(vtube[p], temp, WCOLS);
#endif NONLS8

			vtube[i] = vtube[p];
			vtube[p] = cp;
			return;
		}
	error((nl_msg(4, "Line too long")));
}

/*
 * Insert character c at current cursor position.
 * Multi-character inserts occur only as a result
 * of expansion of tabs (i.e. inssize == 1 except
 * for tabs) and code assumes this in several place
 * to make life simpler.
 */
vinschar(c)
	int c;		/* mjm: char --> int */
{
	register int i;

#ifndef NONLS8 /* 8bit integrity */
	register short *tp;
#else NONLS8
	register char *tp;
#endif NONLS8

#ifdef	NLS16
	register int j;
	static int firstbyte = 0;	/* First byte of Kanji character */
#ifdef EUC
	int cw_ch ;	/* column width of the character	*/
	int rem_col ;	/* the number of column remaining in the line	*/
	int k ;
#endif EUC
	/*
	 * If first byte of Kanji character is handled to this function
	 * vinschar(), the byte is saved in firstbyte and the subsequent
	 * byte is waited. This function is intended to handle CHARACTER
	 * (not BYTE). Otherwise, any escape sequence to control screen
	 * is possibly inserted between two bytes of Kanji character.
	 *
	 * If the first byte of Kanji character will be placed on the
	 * edge of the terminal right margine, the character is placed
	 * on the beginning of the next line.
	 */
	if (IS_FIRST(c)) {
#ifdef EUC
		cw_ch = C_COLWIDTH( c & TRIM ) ;
		if ( WCOLS ) {
			rem_col = WCOLS - ( destcol % WCOLS ) ;
			if ( cw_ch > rem_col ) {
				while ( rem_col > 0 ) {
					vinschar(' '|DUMMY_BLANK);
					rem_col-- ;
				}
			}
		}
#else EUC
		if (((destcol + 1) % WCOLS) == 0)
			vinschar(' '|DUMMY_BLANK);
#endif EUC
		firstbyte = c;
		return;
	} else if (!IS_SECOND(c))
		firstbyte = 0;
#endif

	if ((!enter_insert_mode || !exit_insert_mode) && ((hold & HOLDQIK) || !value(REDRAW) || value(SLOWOPEN))) {
		/*
		 * Don't want to try to use terminal
		 * insert mode, or to try to fake it.
		 * Just put the character out; the screen
		 * will probably be wrong but we will fix it later.
		 */
		if (c == '\t') {
			vgotab();
			return;
		}

#ifdef	NLS16
		if (firstbyte)	{
			vputchar(firstbyte);
			firstbyte = 0;
		}
#endif

		vputchar(c);
		if (DEPTH(vcline) * WCOLS + !value(REDRAW) >
		    (destline - LINE(vcline)) * WCOLS + destcol)
			return;
		/*
		 * The next line is about to be clobbered
		 * make space for another segment of this line
		 * (on an intelligent terminal) or just remember
		 * that next line was clobbered (on a dumb one
		 * if we don't care to redraw the tail.
		 */
		if (insert_line) {
			vnpins(0);
		} else {
			c = LINE(vcline) + DEPTH(vcline);
			if (c < LINE(vcline + 1) || c > WBOT)
				return;
			i = destcol;
			vinslin(c, 1, vcline);
			DEPTH(vcline)++;
			vigoto(c, i);
			vprepins();
		}
		return;
	}
	/*
	 * Compute the number of positions in the line image of the
	 * current line.  This is done from the physical image
	 * since that is faster.  Note that we have no memory
	 * from insertion to insertion so that routines which use
	 * us don't have to worry about moving the cursor around.
	 */
#if defined NLS16 && defined EUC
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Vinschar() \n  Vbuf = %s \n", cvbuf ) ;
		for ( i= 0 ; i < 5 ; i++ ) {
			NTCHAR_TO_char(vtube[i], cvbuf, BTOCRATIO*WCOLS -1 ) ;
			fprintf( trace, " \n Content of vtube[ %d ] = %s \n", i, cvbuf ) ;
		}
	}
#endif IDEBUG
	if ( *vbuf0 == 0 )
#else
	if (*vtube0 == 0)
#endif
		linend = 0;
	else {
		/*
		 * Search backwards for a non-null character
		 * from the end of the displayed line.
		 */
#if defined NLS16 && defined EUC
		i = BTOCRATIO * WCOLS * DEPTH(vcline);
		if ( i >= TUBESIZE ) {
			i = TUBESIZE - 1 ;
		}
		if (i == 0)
			i = BTOCRATIO * WCOLS;
#else
		i = WCOLS * DEPTH(vcline);
		if (i == 0)
			i = WCOLS;
#endif
#if defined NLS16 && defined EUC
		tp = vbuf0 + i ;
#else
		tp = vtube0 + i;
#endif
		while (*--tp == 0)
			if (--i == 0)
				break;
#if defined NLS16 && defined EUC
		linend = 0 ;
		tp = vbuf0 ;
		for( k = 0 ; k < i ; tp++, k++ ) {
			if( IS_FIRST( *tp ) )
				linend -= ( 2 - C_COLWIDTH( ( (int)(*tp) ) & TRIM ) ) ;
			linend++ ;
		}
#else
		linend = i;
#endif

	}

	/*
	 * We insert at a position based on the physical location
	 * of the output cursor.
	 */
	inscol = destcol + (destline - LINE(vcline)) * WCOLS;
	if (c == '\t') {
		/*
		 * Characters inserted from a tab must be
		 * remembered as being part of a tab, but we can't
		 * use QUOTE here since we really need to print blanks.
		 * QUOTE|' ' is the representation of this.
		 */
		inssiz = tabcol(inscol, value(TABSTOP)) - inscol;
#if defined NLS16 && defined EUC
		b_inssiz = tabcol(inscol, value(TABSTOP)) - inscol;
#endif
#if defined NLS || defined NLS16
		if (right_to_left) {
			c = rl_mode==NL_LATIN ? ' ' | QUOTE : alt_space | QUOTE;
		} else {
			c = ' ' | QUOTE;
		}
#else
		c = ' ' | QUOTE;
#endif

#ifdef NLS16
#ifdef EUC
	} else if (firstbyte) {
		inssiz = C_COLWIDTH( firstbyte & TRIM ) ;
		b_inssiz = 2 ;
	} else {
		inssiz = 1;
		b_inssiz = 1 ;
	}
	extr_byte = b_inssiz - inssiz ;
#else EUC
	} else if (firstbyte) {
		inssiz = 2;
#endif EUC
#endif NLS16

#ifndef EUC
	} else
		inssiz = 1;
#endif EUC

	/*
	 * If the text to be inserted is less than the number
	 * of doomed positions, then we don't need insert mode,
	 * rather we can just typeover.
	 */
	if (inssiz <= doomed) {
		endim();
		if (inscol != linend) {
			doomed -= inssiz;
#if defined NLS16 && defined EUC
			b_doomed -= inssiz;
#endif
		}

#if defined NLS || defined NLS16
		if (opp_back) {
			tputs(tparm(cursor_address,opp_line,opp_col+opp_len),0,putch);
			godm();
			vputp(delete_character, DEPTH(vcline));
			enddm();
			tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
			goim();
			opp_back--;
		}
#endif
#ifdef	NLS16
		if (firstbyte)	{
			vputchar(firstbyte);
#ifdef EUC
			inssiz -= ( C_COLWIDTH( firstbyte & TRIM ) -1 ) ;
			b_inssiz -= 1 ;
			firstbyte = 0;
#else EUC
			firstbyte = 0;
			inssiz--;
#endif EUC
		}
#endif

		do
			vputchar(c);
		while (--inssiz);
#if defined NLS16 && defined EUC
		vprepins() ; 	/* update vbuf[] */
#endif
		return;
	}

	/*
	 * Have to really do some insertion, thus
	 * stake out the bounds of the first following
	 * group of tabs, computing starting position,
	 * ending position, and the number of ``spaces'' therein
	 * so we can tell how much it will squish.
	 */

#if defined NLS16 && defined EUC
	tp = vbuf0 + vcolumn( vbuf0, inscol ) ;
#else
	tp = vtube0 + inscol;
#endif
	for (i = inscol; i < linend; i++){
#if defined NLS16 && defined EUC
		if (IS_FIRST(*tp)) {
			i -= ( 2 - C_COLWIDTH( ( (int)( *tp ) ) & TRIM ) ) ;
		}
#endif
		if (*tp++ & QUOTE) {
			--tp;
			break;
		}
	}
	tabstart = tabend = i;
	tabslack = 0;
	while (tabend < linend) {
#if defined NLS16 && defined EUC
		if ( IS_FIRST( *tp ) ){
			cw_ch =  C_COLWIDTH( ( (int)( *tp ) ) & TRIM ) ;
			tabsize -= ( 2 - cw_ch ) ;
			tabend -= ( 2 - cw_ch ) ;
		}
#endif
		i = *tp++;
		if ((i & QUOTE) == 0)
			break;
		if ((i & TRIM) == 0)
			tabslack++;
		tabsize++;
		tabend++;
	}
	tabsize = tabend - tabstart;

	/*
	 * For HP's and DM's, e.g. tabslack has no meaning.
	 */
	if (!insert_null_glitch)
		tabslack = 0;

#ifdef	NLS16
	/*
	 * It is necessary to pre-compute beftabwrap, afttabwrap, linendwrap.
	 * and needspace.
	 */
	beftabwrap = inssiz ;	/* initial value of beftabwrap */
	
	/* Start from the first line and end at the last line */

	for (i = (inscol + beftabwrap - 1)/WCOLS + 1;
	     i <= (tabstart + beftabwrap - doomed - 1)/WCOLS; i++)	{
		/* Get the character at the position which across the right */
		/* boundary of the display when the character is inserted    */

#ifdef EUC
		tp = vbuf0 + vcolumn( vbuf0, ( i * WCOLS - beftabwrap + doomed ) ) ;
#else EUC
		tp = vtube0 + i * WCOLS - beftabwrap + doomed;
#endif EUC

		/* If there is a DUMMY_BLANK, we can squash it and can save  */
		/* the space needed to move the next line.		     */
		/* This case is occur only when ( beftabwrap - doomed ) = 1 */
		/* so the calculation is finished.			     */

		if (*tp & DUMMY_BLANK)	{
			beftabwrap--;
			break;
		}

		/* If there is a kanji character, there need to be one more  */
		/* space to move the next line.			             */

		if (IS_SECOND(*tp))	{
#ifdef EUC
			cw_ch = C_COLWIDTH( ( (int)(*--tp) ) & TRIM  ) ;
			beftabwrap += ( cw_ch - 1 ) ;
#else EUC
			beftabwrap++;
			tp--;
#endif EUC
		}

		/* If there is a DUMMY_BLANK, we can squash it and can save  */
		/* the space needed to move the next line.		     */

		j = beftabwrap;
#ifdef EUC
		while (j-- && tp < vbuf0 + vcolumn(vbuf0, tabstart ) ){

			if ( IS_FIRST( *tp ) ) {
				j += ( 2 - C_COLWIDTH( ( (int)( *tp ) ) & TRIM ) );
			}
			if (*tp++ & DUMMY_BLANK)
				beftabwrap--;
		}
#else EUC
		while (j-- && tp < vtube0 + tabstart)
			if (*tp++ & DUMMY_BLANK)
				beftabwrap--;
#endif EUC
	}

		/*
		 * If line has no tab character, needspace, afttabwrap, and
		 * linendwrap are not required.
		 */
	
		if (tabsize == 0)
			goto next;
	
		/* From now on, the number of column needed for the movement */
		/* is calculated.					     */
	
		/* The space before the tabstart needed for the insertion    */
	
		needspace = beftabwrap - (tabcol(tabstart, value(TABSTOP)) - tabstart) - doomed;
	
		/* The space after the tabstart needed for the insertion    */
		/* if the shift of tab position is needed.		    */
	
		linendwrap = afttabwrap = (needspace / value(TABSTOP) + 1) * value(TABSTOP);
	
		/* if linendwrap is less than the number of the space before */
		/* the start of the printing character.			     */
	
		if (linendwrap < (j = (tabend / WCOLS + 1) * WCOLS - tabend))
			j = linendwrap;
	
		/* calculate the shift value after tabend.		     */
	
		for (i = (tabend + j - 1) / WCOLS + 1;
		     i <= (linend + linendwrap - 1) / WCOLS; i++)	{
	
			/* Get the character at the position which across the right */
			/* boundary of the display when the text after tabend is     */
			/* shifted.						     */
	
#ifdef EUC
			tp = vbuf0 + vcolumn( vbuf0, ( i * WCOLS - linendwrap ) ) ;
#else EUC
			tp = vtube0 + i * WCOLS - linendwrap;
#endif EUC
	
			/* If there is a DUMMY_BLANK, we can squash it and can save  */
			/* the space needed to move the next line.		     */
			/* This case is occur only when linendwrap = 1     	     */
			/* so the calculation is finished.			     */
	
			if (*tp & DUMMY_BLANK)	{
				linendwrap--;
				break;
			}
	
			/* If there is a kanji character, there need to be one more  */
			/* space to move the next line.			             */
	
			if (IS_SECOND(*tp))	{
#ifdef EUC
				linendwrap += ( C_COLWIDTH( ( (int)(*--tp) ) & TRIM ) - 1 ) ;
#else EUC
				linendwrap++;
				tp--;
#endif EUC
			}
	
			/* If there is a DUMMY_BLANK, we can squash it and can save  */
			/* the space needed to move the next line.		     */
	
			j = linendwrap;
#ifdef EUC
			while (j-- && *tp) {
				if ( IS_FIRST( *tp ) ) {
					j += ( 2 - C_COLWIDTH( ( (int)( *tp ) ) & TRIM ) );
				}
				if (*tp++ & DUMMY_BLANK)
					linendwrap--;
			}
#else EUC
			while (j-- && *tp)
				if (*tp++ & DUMMY_BLANK)
					linendwrap--;
#endif EUC
		}
	

next:
#ifdef EUC
#ifdef IDEBUG
	if (trace) {
		fprintf(trace, "\n Vinschar() : doomed %d, b_doomed %d, b_inssiz %d, ",
			doomed, b_doomed, b_inssiz);
	}
#endif IDEBUG
#endif EUC
#endif

#ifdef IDEBUG
	if (trace) {
		fprintf(trace, "inscol %d, inssiz %d, tabstart %d, ",
			inscol, inssiz, tabstart);
		fprintf(trace, "tabend %d, tabslack %d, linend %d\n",
			tabend, tabslack, linend);
	}
#endif

	/*
	 * The real work begins.
	 */
	slakused = 0;
	shft = 0;

#ifndef	NLS16
	if (tabsize) {
		/*
		 * There are tabs on this line.
		 * If they need to expand, then the rest of the line
		 * will have to be shifted over.  In this case,
		 * we will need to make sure there are no ``spaces''
		 * in the rest of the line (on e.g. CONCEPT-100)
		 * and then grab another segment on the screen if this
		 * line is now deeper.  We then do the shift
		 * implied by the insertion.
		 */
		if (inssiz >= doomed + tabcol(tabstart, value(TABSTOP)) - tabstart) {
			if (insert_null_glitch)
				vrigid();
			vneedpos(value(TABSTOP));
			vishft();
		}
	} else if (inssiz > doomed)
		/*
		 * No tabs, but line may still get deeper.
		 */
		vneedpos(inssiz - doomed);
#else
	if (tabsize) {
		if (needspace >= 0)	{
			if (insert_null_glitch)
				vrigid();
			vneedpos(linendwrap);
			vishft();
		}
	} else if (beftabwrap > doomed)
		vneedpos(beftabwrap - doomed);
#endif

	/*
	 * Now put in the inserted characters.
	 */
#if defined NLS16 && defined EUC
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Vinschar() : Content of vbuf before viin() = %s \n", cvbuf ) ;
	}
#endif IDEBUG
#endif
#ifndef	NLS16
	viin(c);
#else
	viin(firstbyte, c);
#endif
	/*
	 * Now put the cursor in its final resting place.
	 */
	destline = LINE(vcline);
	destcol	= inscol + inssiz;
	vcsync();
#if defined NLS || defined NLS16
	if (opp_insert) {
		if ((opp_inscol / WCOLS) != (linend / WCOLS)) {
			tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
		}
	}
#endif

#ifdef	NLS16
	firstbyte = 0;
#endif
 
}

/*
 * Rigidify the rest of the line after the first
 * group of following tabs, typing blanks over ``spaces''.
 */
vrigid()
{
	register int col;

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
	register short *tp ;
	tp = vbuf0 + vcolumn( vbuf0, tabend ) ;
#else
	register short *tp = vtube0 + tabend;
#endif
#else NONLS8
	register char *tp = vtube0 + tabend;
#endif NONLS8


	for (col = tabend; col < linend; col++){
#if defined NLS16 && defined EUC
		if ( IS_FIRST( *tp ) ) {
			col -= ( 2 - C_COLWIDTH( ( (int)( *tp ) ) & TRIM ) );
		}
#endif
		if ((*tp++ & TRIM) == 0) {
			endim();
			vgotoCL(col);
			vputchar(' ' | QUOTE);
		}
	}
}

/*
 * We need cnt more positions on this line.
 * Open up new space on the screen (this may in fact be a
 * screen rollup).
 *
 * On a dumb terminal we may infact redisplay the rest of the
 * screen here brute force to keep it pretty.
 */
vneedpos(cnt)
	int cnt;
{
	register int d = DEPTH(vcline);
	register int rmdr = d * WCOLS - linend;

	if (cnt <= rmdr - insert_null_glitch)
		return;
	endim();
	vnpins(1);
}

vnpins(dosync)
	int dosync;
{
	register int d = DEPTH(vcline);
	register int e;

	e = LINE(vcline) + DEPTH(vcline);
	if (e < LINE(vcline + 1)) {
		vigoto(e, 0);
		vclreol();
		return;
	}
	DEPTH(vcline)++;
	if (e < WECHO) {
		e = vglitchup(vcline, d);
		vigoto(e, 0); vclreol();
		if (dosync) {
			int (*Ooutchar)() = Outchar;
			Outchar = vputchar;
			vsync(e + 1);
			Outchar = Ooutchar;
		}
	} else {
		vup1();
		vigoto(WBOT, 0);
		vclreol();
	}
#if defined NLS16 && defined EUC
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Content of vbuf after vnpins before vprepins = %s \n", cvbuf ) ;
	}
#endif IDEBUG
#endif
	vprepins();
}

/*
 * Do the shift of the next tabstop implied by
 * insertion so it expands.
 */
vishft()
{
	int tshft = 0;
	int j;
	register int i;

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
	register short *tp = vbuf0;
	register short *up;
	int cw_ch ;	/* column width of the character	*/
	int rem_col ;	/* the number of column remaining in the line	*/
	int k ;
/* tp_buf and up_buf represent the position in the vbuf0 not	*/
/* on the screen						*/
	short *tp_buf = vbuf0 ;
	short *up_buf ;
	int b_linend ;	/* linend for vbuf0	*/
	int b_tabend ;	/* tabend for vbuf0	*/
#else
	register short *tp = vtube0;
	register short *up;
#endif
#else NONLS8
	register char *tp = vtube0;
	register char *up;
#endif NONLS8

	short oldhold = hold;
	shft = value(TABSTOP);
	hold |= HOLDPUPD;
	if (!enter_insert_mode && !exit_insert_mode) {
		/*
		 * Dumb terminals are easy, we just have
		 * to retype the text.
		 */

#ifndef	NLS16
		vigotoCL(tabend + shft);
#else
		vigotoCL(tabend + afttabwrap);
#endif

#ifndef	NLS16
		up = tp + tabend;
		for (i = tabend; i < linend; i++)
			vputchar(*up++);
#else
		up = tp + tabend;
#ifdef EUC
		up_buf = tp_buf + vcolumn( tp_buf, tabend ) ;
#endif EUC
		for (i = tabend; i < linend; i++)	{
#ifdef EUC
			if (*up_buf & DUMMY_BLANK)	{
				up_buf++;
#else EUC
			if (*up & DUMMY_BLANK)	{
#endif EUC
				up++ ;
				continue;
			}
#ifdef EUC
			if ( IS_FIRST(*up_buf) ) {
				cw_ch = C_COLWIDTH( ( (int)(*up_buf) ) & TRIM ) ;
				if ( WCOLS ) {
				rem_col = WCOLS - ( ( up - vbuf0 ) % WCOLS ) ;
					if ( cw_ch > rem_col ) {
						while ( rem_col > 0 ) {
							vputchar(' '|DUMMY_BLANK);
							rem_col-- ;
						}
					}
				}
				i -= ( 2 - cw_ch );
				up -= ( 2 - cw_ch );
			}
			vputchar(*up_buf++);
			up++ ;
#else EUC
			if ((up - vtube0 + 1) % WCOLS == 0 && IS_FIRST(*up))
				vputchar(' '|DUMMY_BLANK);
			vputchar(*up++);
#endif EUC
		}
#endif

	} else if (insert_null_glitch) {
		/*
		 * CONCEPT-like terminals do most of the work for us,
		 * we don't have to muck with simulation of multi-line
		 * insert mode.  Some of the shifting may come for free
		 * also if the tabs don't have enough slack to take up
		 * all the inserted characters.
		 */
		i = shft;
		slakused = inssiz - doomed;
		if (slakused > tabslack) {
			i -= slakused - tabslack;
			slakused -= tabslack;
		}
		if (i > 0 && tabend != linend) {
			tshft = i;
			vgotoCL(tabend);
			goim();
			do
				vputchar(' ' | QUOTE);
			while (--i);
		}
	} else {
		/*
		 * HP and Datamedia type terminals have to have multi-line
		 * insert faked.  Hack each segment after where we are
		 * (going backwards to where we are.)  We then can
		 * hack the segment where the end of the first following
		 * tab group is.
		 */

#ifndef	NLS16
		for (j = DEPTH(vcline) - 1; j > (tabend + shft - 1) / WCOLS; j--) {
			vgotoCL(j * WCOLS);
			goim();
			up = tp + j * WCOLS - shft;
			i = shft;
#ifdef NLS
			if (right_to_left && rl_curorder != NL_SCREEN) {
				if (opp_insert) {
					if (opp_terminate) {
						base_lang();
					} else {
						base_mode();
						tputs(tparm(cursor_address,destline,destcol),0,putch);
					}
				}
				tputs(screen_order, 1, putch);
				flusho();
			}
#endif
			do {
				if (*up)
					vputchar(*up++);
				else
					break;
			} while (--i);
#ifdef NLS
			if (right_to_left && rl_curorder != NL_SCREEN) {
				if (opp_insert) {
					opp_lang();
				}
				tputs(key_order, 1, putch);
				flusho();
			}
#endif
		}
		if ((tabstart / WCOLS) == (tabend / WCOLS)) {
#ifdef NLS
			vigotoCL(opp_spctab = tabstart);
#else
			vigotoCL(tabstart);
#endif
			i = shft - (inssiz - doomed);
		}
		else {
#ifdef NLS
			vigotoCL(opp_spctab = tabend);
#else
			vigotoCL(tabend);
#endif
			i = shft;
		}
		if (i > 0) {
			tabslack = inssiz - doomed;
			vcsync();
			goim();
#ifdef NLS
			/* guarantee we're really there in opp lang insertion */
			if (opp_insert) {
				register int row = LINE(vcline) + opp_spctab/WCOLS;
				register int col = opp_spctab % WCOLS;
				tputs(tparm(cursor_address,row,col),0,putch);
				opp_numtab = ++i;
			}
			do
				vputchar(right_to_left ? rl_mode == NL_NONLATIN ? alt_space : ' ' : ' ');
			while (--i);
#else
			do
				vputchar(' ');
			while (--i);
#endif
		}
#else
		/*
		 * Wraparound characters are displayed on the next line.
		 * But DUMMY_BLANK must NOT be appeared to the next line.
		 */
		int	wrap = afttabwrap;	/* historical bytes for wraparound */

		/*
		 * If afttabwrap exceeds the number of coulmns from tabend
		 * to right margin, wrap is changed with the rest of columns.
		 */
		if (afttabwrap > (tabend / WCOLS + 1) * WCOLS - tabend)
			wrap = (tabend / WCOLS + 1) * WCOLS - tabend;
		for (j = (tabend + wrap - 1) / WCOLS + 1;
		     j <= (linend + wrap - 1) / WCOLS; j++)	{
			up = tp + j * WCOLS - wrap;
#ifdef EUC
			up_buf = tp_buf + vcolumn( tp_buf, ( j * WCOLS - wrap ) ) ;
			if (*up_buf & DUMMY_BLANK)
#else EUC
			if (*up & DUMMY_BLANK)
#endif EUC
				break;
			vgotoCL(j * WCOLS);
#ifdef EUC
			if (IS_SECOND(*up_buf))	{
				wrap +=  ( ( C_COLWIDTH( ( (int)(*--up_buf) ) & TRIM ) ) -1 )  ;
				up--;
			}
#else EUC
			if (IS_SECOND(*up))	{
				wrap++;
				up--;
			}
#endif EUC
			goim();
			i = wrap;
			if (right_to_left && rl_curorder != NL_SCREEN) {
				if (opp_insert) {
					if (opp_terminate) {
						base_lang();
					} else {
						base_mode();
						tputs(tparm(cursor_address,destline,destcol),0,putch);
					}
				}
				tputs(screen_order, 1, putch);
				flusho();
			}
#ifdef EUC
			while (i-- && *up_buf) {
				if ( IS_FIRST( *up_buf ) ) {
					cw_ch =  C_COLWIDTH( ( (int)( *up_buf ) ) & TRIM ) ;
					i += ( 2 - cw_ch );
					up -= ( 2 - cw_ch );
				}
				if (*up_buf & DUMMY_BLANK)	{
					wrap--;
					up++;
					up_buf++ ;
				} else {
					vputchar(*up_buf++);
					up++ ;
				}
			}
#else EUC
			while (i-- && *up)
				if (*up & DUMMY_BLANK)	{
					wrap--;
					up++;
				} else
					vputchar(*up++);
#endif EUC
			if (right_to_left && rl_curorder != NL_SCREEN) {
				if (opp_insert) {
					opp_lang();
				}
				tputs(key_order, 1, putch);
				flusho();
			}
		}
		/*
		 * If actual shift value exceeds shft due to wraparound
		 * from the previous line on the screen, spaces should
		 * be placed on the position which is caluculated based on
		 * the actual shifted value.
		 */
		if (beftabwrap % shft < doomed + tabcol(tabstart, value(TABSTOP)) - tabstart) {
			opp_spctab = destcol;
			i = 0;
		} else if ((tabstart / WCOLS) != ((tabstart + afttabwrap) / WCOLS)) {
			vigotoCL(opp_spctab = ((tabstart + afttabwrap) / WCOLS) * WCOLS);
			i = shft - (needspace % shft - doomed);
		} else if ((tabstart / WCOLS) == (tabend / WCOLS)) {
			vigotoCL(opp_spctab = tabstart);
			i = shft - (((beftabwrap - 1) % shft + 1) - doomed);
		} else {
			vigotoCL(opp_spctab = tabend);
			i = shft;
		}
		if (i > 0) {
			tabslack = beftabwrap % shft - doomed;
			vcsync();
			goim();
			/* guarantee we're really there in opp lang insertion */
			if (opp_insert) {
				register int row = LINE(vcline) + opp_spctab/WCOLS;
				register int col = opp_spctab % WCOLS;
				tputs(tparm(cursor_address,row,col),0,putch);
				opp_numtab = ++i;
			}
			do
				vputchar(right_to_left ? rl_mode == NL_NONLATIN ? alt_space : ' ' : ' ');
			while (--i);
		}
#endif

	}
	/*
	 * Now do the data moving in the internal screen
	 * image which is common to all three cases.
	 */
#if defined NLS16 && defined EUC
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Vishft vbuf before update = %s \n", cvbuf ) ;
	}
#endif IDEBUG
#endif

#ifndef	NLS16
	tp += linend;
	up = tp + shft;
	i = linend - tabend;
	if (i > 0)
		do
			*--up = *--tp;
		while (--i);
#else
	tp += linend;
#ifdef EUC
	b_tabend = vcolumn( tp_buf, tabend ) ;
	b_linend = vcolumn( tp_buf, linend ) ;
	tp_buf += b_linend ;
	up_buf = tp_buf + linendwrap ;
#endif EUC
	up = tp + linendwrap;
	/*
	 * At first, existing characters are filling to right edge.
	 */
#ifdef EUC
	for (i = b_linend; i > b_tabend; i--){
		--tp ;
		if ( IS_SECOND( *--tp_buf ) ) {
			if ( IS_FIRST( *( tp_buf - 1 ) ) ) {
				cw_ch = C_COLWIDTH( ( (int)( *( tp_buf -1 ) ) ) & TRIM ) ;
				up += ( 2 - cw_ch ) ;
				tp += ( 2 - cw_ch ) ;
			}
		}
		if (!(*tp_buf & DUMMY_BLANK)){
			*--up_buf = *tp_buf ;
			--up ;
		}
	}
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Vishft vbuf after filling to right edge = %s \n", cvbuf ) ;
		fprintf( trace, " \n Vishft tp  = %d \n", tp - vbuf0 ) ;
	}
#endif IDEBUG
#else EUC
	for (i = linend; i > tabend; i--)
		if (!(*--tp & DUMMY_BLANK))
			*--up = *tp;
#endif EUC
	/*
	 * At final, if there is gap between right filling and linend,
	 * right filled character is adjusted. 'tp' points the
	 * beginning of the gap and 'up' points the next to
	 * the end of the gap.
	 */
	tp += afttabwrap;
#ifdef EUC
	tp_buf += afttabwrap ;
	while (up > tp && *up_buf)	{
		if ( IS_FIRST( *up_buf ) ) {
			cw_ch = C_COLWIDTH( ( (int)(*up_buf) ) & TRIM ) ;
			if ( WCOLS ) {
				rem_col = WCOLS - ( ( tp - vbuf0 ) % WCOLS ) ;
				if ( cw_ch > rem_col ) {
					while ( rem_col > 0 ) {
						*tp_buf++ = ' '|DUMMY_BLANK;
						tp++ ;
						rem_col-- ;
					}
				}
			}
			tp -= ( 2 - cw_ch ) ;
			up -= ( 2 - cw_ch ) ;
		}
		*tp_buf++ = *up_buf++;
		tp++ ;
		up++ ;
	}
#else EUC
	while (up > tp && *up)	{
		if (IS_FIRST(*up) && ((tp - vtube0 + 1) % WCOLS) == 0)
			*tp++ = ' '|DUMMY_BLANK;
		*tp++ = *up++;
	}
#endif EUC
#endif

#if defined NLS16 && defined EUC
	if (insert_null_glitch && tshft) {
		i = tshft;
		do{
			*--up_buf = ' ' | QUOTE;
			--up ;
		}while (--i);
	}
#ifdef IDEBUG
	if ( trace ) {
		fprintf( trace, "\n Vishft() : afttabwrap=%d, linendwrap=%d  \n", afttabwrap, linendwrap ) ;
	}
#endif IDEBUG

	/* update vtube						*/
	/* i represents the number of column			*/
	/* j represents the number of line			*/
	tp = vtube0 ;
	tp_buf = vbuf0 ;
	k = (linend -1 + linendwrap)/WCOLS + 1 ;
	/* clear vtube0	after the line where tabend is	*/
	vclrbyte( tp, ( k*BTOCRATIO*WCOLS ) ) ;
	for( i = 0, j = 0 ; j < k ; ) {
		if( IS_FIRST( *tp_buf ) ) {
			i -= ( 2 - C_COLWIDTH( ( (int)(*tp_buf) ) & TRIM ) ) ;
			i++ ;
			*tp++ = *tp_buf++ ;
		}
		*tp++ = *tp_buf++ ;
		i++ ;
		if( i % WCOLS == 0 ) {
			j++ ;
			tp = vtube0 + j * BTOCRATIO * WCOLS ;
		}
	}
	/* end of update		*/
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Vishft vbuf after update = %s \n", cvbuf ) ;
		for ( i= 0 ; i < 5 ; i++ ) {
			NTCHAR_TO_char(vtube[i], cvbuf, BTOCRATIO*WCOLS -1 ) ;
			fprintf( trace, " \n Vishft vtube[ %d ] = %s \n", i, cvbuf ) ;
		}
	}
#endif IDEBUG
#else

	if (insert_null_glitch && tshft) {
		i = tshft;
		do
			*--up = ' ' | QUOTE;
		while (--i);
	}
#endif
	hold = oldhold;
}

/*
 * Now do the insert of the characters (finally).
 */

#ifndef	NLS16
viin(c)
	int c;		/* mjm: char --> int */
#else
/*
 * Viin() is changed to handle CHARACTER (not BYTE), because
 * Kanji character should be sepaeated with each byte.
 * First is valid if Kanji character should be inserted.
 * If not, first is set with 0.
 */
viin(first, c)
	int first, c;
#ifdef EUC
/* tp_buf and up_buf represent the position in the vbuf0 not	*/
/* on the screen						*/
#endif EUC
#endif

{

#ifndef NONLS8 /* 8bit integrity */
	register short *tp, *up;
#if defined NLS16 && defined EUC
	short *tp_buf ;
	short *up_buf ;
	register int k ;
	int cw_ch ;	/* column width of the character	*/
	int rem_col ;	/* the number of column remaining in the line	*/
	int b_remdoom ;
	int b_inscol ;		/* inscol for vbuf0	*/
	int b_tabstart ;	/* tabstart for vbuf0	*/
	int b_linend ;		/* linend for vbuf0	*/
#endif
#else NONLS8
	register char *tp, *up;
#endif NONLS8
#if defined NLS || defined NLS16
	register bool sim;
#endif	

	register int i, j;
	register bool noim = 0;
	int remdoom;
	short oldhold = hold;

	hold |= HOLDPUPD;

#ifndef	NLS16
	if (tabsize && (enter_insert_mode && exit_insert_mode) && inssiz - doomed > tabslack)
#else
	if (tabsize && (enter_insert_mode && exit_insert_mode) && beftabwrap - doomed > tabslack)
#endif

		/*
		 * There is a tab out there which will be affected
		 * by the insertion since there aren't enough doomed
		 * characters to take up all the insertion and we do
		 * have insert mode capability.
		 */
		if (inscol + doomed == tabstart) {
			/*
			 * The end of the doomed characters sits right at the
			 * start of the tabs, then we don't need to use insert
			 * mode; unless the tab has already been expanded
			 * in which case we MUST use insert mode.
			 */
			slakused = 0;
			noim = !shft;
		} else {
			/*
			 * The last really special case to handle is case
			 * where the tab is just sitting there and doesn't
			 * have enough slack to let the insertion take
			 * place without shifting the rest of the line
			 * over.  In this case we have to go out and
			 * delete some characters of the tab before we start
			 * or the answer will be wrong, as the rest of the
			 * line will have been shifted.  This code means
			 * that terminals with only insert chracter (no
			 * delete character) won't work correctly.
			 */

#ifndef	NLS16
			i = inssiz - doomed - tabslack - slakused;
#else
			i = beftabwrap - doomed - tabslack - slakused;
#endif

			i %= value(TABSTOP);
			if (i > 0) {
				vgotoCL(tabstart);
#if defined NLS || defined NLS16
				/* guarantee we're really there in opp lang insertion */
				if (opp_insert) {
					register int row = LINE(vcline) + tabstart/WCOLS;
					register int col = tabstart % WCOLS;
					tputs(tparm(cursor_address,row,col),0,putch);
					opp_eattab = 1;
				}
#endif
				godm();
#ifndef	NLS16
				for (i = inssiz - doomed - tabslack; i > 0; i--)
#else
				for (i = beftabwrap - doomed - tabslack; i > 0; i--)
#endif

					vputp(delete_character, DEPTH(vcline));
				enddm();
			}
		}

	/* 
	 * Now put out the characters of the actual insertion.
	 */
	vigotoCL(inscol);
	remdoom = doomed;
#if defined NLS16 && defined EUC
	b_remdoom = b_doomed ;
#endif

#ifdef	NLS16
	/*
	 * This is a special case. If doomed is 1 and Kanji character is
	 * inserted, original mechanism produces enter_insert_mode
	 * between two bytes of Kanji character. To avoid it,
	 * a doomed character is deleted before Kanji character is written.
	 */
	if (first && remdoom == 1 && !noim)	{
		vcsync();
		godm();
		vputp(delete_character, DEPTH(vcline));
		enddm();
		remdoom--;
#ifdef EUC
		b_remdoom-- ;
#endif EUC
	}
#endif
		
	for (i = inssiz; i > 0; i--) {
		if (remdoom > 0) {
			remdoom--;
#if defined NLS16 && defined EUC
			b_remdoom-- ;
#endif
			endim();
		} else if (noim)
			endim();
		else if (enter_insert_mode && exit_insert_mode) {
			vcsync();
#if defined NLS || defined NLS16
			sim = insmode;
#endif
			goim();
		}
#if defined NLS || defined NLS16
		if (opp_insert) {
			if (opp_numtab) {
				if (opp_eattab) {
					tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
					opp_eattab = 0;
				} else {
					register int row = LINE(vcline) + opp_spctab/WCOLS;
					register int col = opp_spctab % WCOLS;
					tputs(tparm(cursor_address,row,col),0,putch);
					godm();
					vputp(delete_character, DEPTH(vcline));
					enddm();
					tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
					goim();
				}
				opp_numtab--;
				opp_spctab++;
			} else if (opp_eattab) {
				tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
				opp_eattab = 0;
			} else if ((!sim && opp_jump_insert) ||
			          ((opp_inscol / WCOLS) != (linend / WCOLS))) {
				tputs(tparm(cursor_address,opp_line,opp_col),0,putch);
				goim();
			}
		}
#endif

#ifndef	NLS16
		vputchar(c);
#else
		if (first)	{
#ifdef EUC
			i += ( 2 - C_COLWIDTH( first & TRIM ) ) ;
#endif EUC
			vputchar(first);
			vputchar(c);
			i--;
		} else
			vputchar(c);
#endif

	}

	if (!enter_insert_mode || !exit_insert_mode) {
		/*
		 * We are a dumb terminal; brute force update
		 * the rest of the line; this is very much an n^^2 process,
		 * and totally unreasonable at low speed.
		 *
		 * You asked for it, you get it.
		 */
#if defined NLS16 && defined EUC
		tp_buf = vbuf0 + vcolumn( vbuf0, ( inscol + doomed ) ) ;
#endif
		tp = vtube0 + inscol + doomed;

#ifndef	NLS16
		for (i = inscol + doomed; i < tabstart; i++)
			vputchar(*tp++);
#else
#ifdef EUC
		for (i = inscol + doomed; i < tabstart; i++)	{
			if (*tp_buf & DUMMY_BLANK)	{
				tp_buf++ ;
				tp++;
				continue;
			}
			if ( IS_FIRST( *tp_buf ) ) {
				cw_ch = C_COLWIDTH( ( (int)(*tp_buf) ) & TRIM ) ;
				if ( WCOLS ) {
					rem_col = WCOLS - ( ( tp - vbuf0 ) % WCOLS ) ;
					if ( cw_ch > rem_col ) {
						while ( rem_col > 0 ) {
							vputchar(' '|DUMMY_BLANK);
							rem_col-- ;
						}
					}
				}
				i -= ( 2 - cw_ch ) ;
				tp -= ( 2 - cw_ch ) ;
			}
			vputchar(*tp_buf++);
			tp++ ;
		}
#else EUC
		for (i = inscol + doomed; i < tabstart; i++)	{
			if (*tp & DUMMY_BLANK)	{
				tp++;
				continue;
			}
			if ((tp - vtube0 + 1) % WCOLS == 0 && IS_FIRST(*tp))
				vputchar(' '|DUMMY_BLANK);
			vputchar(*tp++);
		}
#endif EUC
#endif

		hold = oldhold;

#ifndef	NLS16
		vigotoCL(tabstart + inssiz - doomed);
		for (i = tabsize - (inssiz - doomed) + shft; i > 0; i--)
#else
		vigotoCL(tabstart + beftabwrap - doomed);
		j = (beftabwrap - 1) % value(TABSTOP) + 1;
		for (i = tabsize - (j - doomed) + shft; i > 0; i--)
#endif

			vputchar(' ' | QUOTE);
	} else {
		if (!insert_null_glitch) {
			/*
			 * On terminals without multi-line
			 * insert in the hardware, we must go fix the segments
			 * between the inserted text and the following
			 * tabs, if they are on different lines.
			 *
			 * Aaargh.
			 */

#ifndef	NLS16
			tp = vtube0;
			for (j = (inscol + inssiz - 1) / WCOLS + 1;
			    j <= (tabstart + inssiz - doomed - 1) / WCOLS; j++) {
				vgotoCL(j * WCOLS);
#ifdef	NLS
				if (opp_insert && !((inscol+1) % WCOLS)) {
					tputs(carriage_return,0,putch);
					tputs(cursor_down,0,putch);
				}
#endif
				i = inssiz - doomed;
				up = tp + j * WCOLS - i;
				goim();
				do
					vputchar(*up++);
				while (--i && *up);
			}
#else NLS16
			int	wrap = inssiz;	/* historical bytes for wraparound */

#ifdef EUC
			tp = vbuf0;
			tp_buf = vbuf0 ;
			for (j = (inscol + wrap - 1) / WCOLS + 1;
			    j <= (tabstart + wrap - doomed - 1) / WCOLS; j++) {
				up = tp + j * WCOLS - wrap + doomed;
				up_buf = tp_buf + vcolumn( tp_buf, ( j * WCOLS - wrap + doomed ) ) ;
				if (*up_buf & DUMMY_BLANK)
					break;
				vgotoCL(j * WCOLS);
				if (opp_insert && !((inscol+1) % WCOLS)) {
					tputs(carriage_return,0,putch);
					tputs(cursor_down,0,putch);
				}
				/*
				 * 'Up' points the character to be wraparounded.
				 * If 'up' points the second byte
				 * of Kanji character, 'up' must point its first
				 * byte and 'wrap' must be increased by one.
				 */
				if (IS_SECOND(*up_buf))	{
					wrap ++ ;
					up_buf-- ;
					up--;
				}
				goim();
				/*
				 * A dummy blank which is placed on the
				 * terminal right margin, needs not to put
				 * to the next line.
				 */
				i = wrap;
				while (i-- && up < tp + tabstart) {
					if ( IS_FIRST( *up_buf ) ) {
						cw_ch = C_COLWIDTH( ( (int)(*up_buf) ) & TRIM ) ;
						i += ( 2 - cw_ch ) ;
						up -= ( 2 - cw_ch ) ;
					}
					if (*up_buf & DUMMY_BLANK)	{
						wrap--;
						up_buf++ ;
						up++;
					} else{
						vputchar(*up_buf++);
						up++ ;
					}
				}
			}
#else EUC
			tp = vtube0;
			for (j = (inscol + wrap - 1) / WCOLS + 1;
			    j <= (tabstart + wrap - doomed - 1) / WCOLS; j++) {
				up = tp + j * WCOLS - wrap + doomed;
				if (*up & DUMMY_BLANK)
					break;
				vgotoCL(j * WCOLS);
				if (opp_insert && !((inscol+1) % WCOLS)) {
					tputs(carriage_return,0,putch);
					tputs(cursor_down,0,putch);
				}
				/*
				 * 'Up' points the character to be wraparounded.
				 * If 'up' points the second byte
				 * of Kanji character, 'up' must point its first
				 * byte and 'wrap' must be increased by one.
				 */
				if (IS_SECOND(*up))	{
					wrap++;
					up--;
				}
				goim();
				/*
				 * A dummy blank which is placed on the
				 * terminal right margin, needs not to put
				 * to the next line.
				 */
				i = wrap;
				while (i-- && up < tp + tabstart)
					if (*up & DUMMY_BLANK)	{
						wrap--;
						up++;
					} else
						vputchar(*up++);
			}
#endif EUC
#endif NLS16

		} else {
			/*
			 * On terminals with multi line inserts,
			 * life is simpler, just reflect eating of
			 * the slack.
			 */
#if defined NLS16 && defined EUC
			tp = vbuf0 + tabend;
			tp_buf = vbuf0 + vcolumn( vbuf0, tabend ) ;
			for (i = tabsize - (inssiz - doomed); i >= 0; i--) {
				--tp ;
				if ((*--tp_buf & (QUOTE|TRIM)) == QUOTE) {
					--tabslack;
					if (tabslack >= slakused)
						continue;
				}
				*tp_buf = ' ' | QUOTE;
			}
#else
			tp = vtube0 + tabend;
			for (i = tabsize - (inssiz - doomed); i >= 0; i--) {
				if ((*--tp & (QUOTE|TRIM)) == QUOTE) {
					--tabslack;
					if (tabslack >= slakused)
						continue;
				}
				*tp = ' ' | QUOTE;
			}
#endif
		}
		/*
		 * Blank out the shifted positions to be tab positions.
		 */
		if (shft) {

#ifndef	NLS16
			tp = vtube0 + tabend + shft;
			for (i = tabsize - (inssiz - doomed) + shft; i > 0; i--)
				if ((*--tp & QUOTE) == 0)
					*tp = ' ' | QUOTE;
#else
#ifdef EUC
			tp_buf = vbuf0 + vcolumn( vbuf0, tabend ) + afttabwrap +extr_byte ;
			j = (beftabwrap - 1) % shft + 1;
			for (i = tabsize - (j - doomed) + shft; i > 0; i--)

				if ((*--tp_buf & QUOTE) == 0)
					*tp_buf  = ' ' | QUOTE;
#else EUC
			tp = vtube0 + tabend + afttabwrap;
			j = (beftabwrap - 1) % shft + 1;
			for (i = tabsize - (j - doomed) + shft; i > 0; i--)
				if ((*--tp & QUOTE) == 0)
					*tp = ' ' | QUOTE;
#endif EUC
#endif
		}
	}

	/*
	 * Finally, complete the screen image update
	 * to reflect the insertion.
	 */
	hold = oldhold;

#if defined NLS16 && defined EUC
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Viin() : Content of before move before insert vbuf = %s \n", cvbuf ) ;
	}
#endif IDEBUG
#endif
#ifndef	NLS16
	tp = vtube0 + tabstart; up = tp + inssiz - doomed;
	for (i = tabstart; i > inscol + doomed; i--)
		*--up = *--tp;
	for (i = inssiz; i > 0; i--)
		*--up = c;
#else
#ifdef EUC
#ifdef IDEBUG
	if (trace) {
		fprintf(trace, "\nViin() :  inscol=%d, inssiz=%d, tabstart=%d, ",
			inscol, inssiz, tabstart);
		fprintf(trace, " doomed=%d, b_doomed=%d, b_inssiz=%d, beftabwrap=%d, extr_byte=%d \n",
			doomed, b_doomed, b_inssiz, beftabwrap, extr_byte );
	}
#endif
	b_inscol = vcolumn( vbuf0, inscol ) ;
	b_tabstart = vcolumn( vbuf0, tabstart ) ;
	/* if there are tabsize and extr_byte > b_doomed, we shift the 	*/
	/* portions after tabstart so that the extr_byte may not	*/
	/* overwrite the tabs						*/
	if ( tabsize && ( extr_byte > b_doomed ) ) {
		b_linend = vcolumn( vbuf0, linend ) ;
		tp_buf = vbuf0 + b_linend ;
		up_buf = tp_buf - b_doomed + extr_byte ;
		for ( i = b_linend ; i > b_tabstart ; i-- ) {
			*--up_buf = *--tp_buf ;
		}
	}
	

	tp = vbuf0 + tabstart; up = tp + beftabwrap - doomed;
	tp_buf = vbuf0 + b_tabstart ;
	up_buf = tp_buf + beftabwrap - b_doomed + extr_byte ;
	/*
	 * At first, existing characters are filled to right edge.
	 */
	for (i = b_tabstart; i > b_inscol + b_doomed; i--){
		--tp ;
		if ( IS_SECOND( *--tp_buf )  ) {
			if ( IS_FIRST( *( tp_buf - 1 ) ) ) {
				cw_ch = C_COLWIDTH( ( (int)(*( tp_buf -1 )) ) & TRIM ) ;
				tp += ( 2 - cw_ch ) ;
				up += ( 2 - cw_ch ) ;
			}
		}
		if (!(*tp_buf & DUMMY_BLANK)){
			--up ;
			*--up_buf = *tp_buf ;
		}
	}

	tp -= doomed;

	/*  Have we gone past the front end of the buffer?  If we did, this  */
        /*  will overwrite memory, and will eventually cause a core dump on  */
        /*  the 300's.  I think we should also change the value of doomed    */
	/*  to reflect the difference between where tp used to be and where  */
        /*  the front end of the buffer is.				     */

	if ( tp < vbuf0 ) 
	{		
        /*  (tp + doomed) is where tp had originally been pointing.          */
	    doomed = vbuf0 - (tp + doomed);
	    tp = vbuf0;		
	}
	
	tp_buf -= b_doomed;
	if ( tp_buf < vbuf0 )  	
	{
	/*  (tp_buf + b_doomed) is where tp_buf had originally been pointing.*/
	    b_doomed = vbuf0 - (tp_buf + b_doomed);    
	    tp_buf = vbuf0;	
	}

#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Content of vbuf after filled to right edge = %s \n", cvbuf ) ;
	}
#endif IDEBUG
	/*
	 * At second, characters to be inserted are filled from left edge.
	 */
	for (i = b_inssiz; i > 0; i--)	{
		if (first)	{
			cw_ch = C_COLWIDTH( first & TRIM ) ;
			if ( WCOLS ) {
				rem_col = WCOLS - ( ( tp - vbuf0 ) % WCOLS ) ;
				if ( cw_ch > rem_col ) {
					while ( rem_col > 0 ) {
						*tp_buf++ = ' '|DUMMY_BLANK;
						tp++ ;
						rem_col-- ;
					}
				}
			}
			tp++ ;
			*tp_buf++ = first;
			i--;
			tp -= ( 2 - cw_ch ) ;
		}
		tp++ ;
		*tp_buf++ = c ;
	}
	/*
	 * If there is gap between both filling, right filled characters
	 * are adjusted. 'tp' points the beginning of gap, and 'up'
	 * points the next to the end of the gap.
	 */
	while (up > tp && *up_buf )	{
		if ( IS_FIRST( *up_buf ) ) {
			cw_ch = C_COLWIDTH( ( (int)(*up_buf) ) & TRIM ) ;
			if ( WCOLS ) {
				rem_col = WCOLS - ( ( tp - vbuf0 ) % WCOLS ) ;
				if ( cw_ch > rem_col ) {
					while ( rem_col > 0 ) {
						*tp_buf++ = ' '|DUMMY_BLANK;
						tp++ ;
						rem_col-- ;
					}
				}
			}
			tp -= ( 2 - cw_ch ) ;
			up -= ( 2 - cw_ch ) ;
		}
		tp++ ;
		up++ ;
		*tp_buf++ = *up_buf++;
	}
	/* update vtube						*/
	/* i represents the number of column			*/
	/* j represents the number of line			*/
	tp = vtube0 ;
	tp_buf = vbuf0 ;
	k = linend - 1 ;
	if( needspace >= 0 ) {
		k = k + linendwrap ;
	}
	k = k / WCOLS + 1 ;
	/* clear vtube0	*/
	vclrbyte( tp, k*BTOCRATIO*WCOLS ) ;
	for( i=0, j=0 ; j < k ; ) {
		if( IS_FIRST( *tp_buf ) ) {
			i -= ( 2 - C_COLWIDTH( ( (int)(*tp_buf) ) & TRIM ) ) ;
			i++ ;
			*tp++ = *tp_buf++ ;
		}
		*tp++ = *tp_buf++ ;
		i ++ ;
		if( i % WCOLS == 0 ) {
			j++ ;
			tp = vtube0 + j * BTOCRATIO * WCOLS ;
		}
	}
	b_doomed = 0 ;
#ifdef IDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		TCHAR_TO_char(vbuf0, cvbuf) ;
		fprintf( trace, " \n Viin() \n Content of vbuf after update = %s \n", cvbuf ) ;
		for ( i= 0 ; i < 5 ; i++ ) {
			NTCHAR_TO_char(vtube[i], cvbuf,BTOCRATIO*WCOLS -1 ) ;
			fprintf( trace, " \n Content of vtube[ %d ] = %s \n", i, cvbuf ) ;
		}
	}
#endif IDEBUG
#else EUC
	tp = vtube0 + tabstart; up = tp + beftabwrap - doomed;
	/*
	 * At first, existing characters are filled to right edge.
	 */
	for (i = tabstart; i > inscol + doomed; i--)
		if (!(*--tp & DUMMY_BLANK))
			*--up = *tp;
	tp -= doomed;
	/*
	 * At second, characters to be inserted are filled from left edge.
	 */
	for (i = inssiz; i > 0; i--)	{
		if (first)	{
			if (((tp - vtube0 + 1) % WCOLS) == 0)
				*tp++ = ' '|DUMMY_BLANK;
			*tp++ = first;
			i--;
		}
		*tp++ = c;
	}
	/*
	 * If there is gap between both filling, right filled characters
	 * are adjusted. 'tp' points the beginning of gap, and 'up'
	 * points the next to the end of the gap.
	 */
	while (up > tp && *up)	{
		if (IS_FIRST(*up) && ((tp - vtube0 + 1) % WCOLS) == 0)
			*tp++ = ' '|DUMMY_BLANK;
		*tp++ = *up++;
	}
#endif EUC
#endif
#if defined NLS16 && defined EUC
	doomed -= inssiz ;
	if ( doomed <= 0 ){
		doomed = 0 ;
	}
#else
	doomed = 0;
#endif
}

/*
 * Go into ``delete mode''.  If the
 * sequence which goes into delete mode
 * is the same as that which goes into insert
 * mode, then we are in delete mode already.
 */
godm()
{

	if (insmode) {
		if (eq(enter_delete_mode, enter_insert_mode))
			return;
		endim();
	}
	vputp(enter_delete_mode, 0);
}

/*
 * If we are coming out of delete mode, but
 * delete and insert mode end with the same sequence,
 * it wins to pretend we are now in insert mode,
 * since we will likely want to be there again soon
 * if we just moved over to delete space from part of
 * a tab (above).
 */
enddm()
{

	if (eq(enter_delete_mode, enter_insert_mode)) {
		insmode = 1;
		return;
	}
	vputp(exit_delete_mode, 0);
}

/*
 * In and out of insert mode.
 * Note that the code here demands that there be
 * a string for insert mode (the null string) even
 * if the terminal does all insertions a single character
 * at a time, since it branches based on whether enter_insert_mode is null.
 */
goim()
{

	if (!insmode)
		vputp(enter_insert_mode, 0);
	insmode = 1;
}

endim()
{

	if (insmode) {
		vputp(exit_insert_mode, 0);
		insmode = 0;
	}
}

/*
 * Put the character c on the screen at the current cursor position.
 * This routine handles wraparound and scrolling and understands not
 * to roll when splitw is set, i.e. we are working in the echo area.
 * There is a bunch of hacking here dealing with the difference between
 * QUOTE, QUOTE|' ', and ' ' for CONCEPT-100 like terminals, and also
 * code to deal with terminals which overstrike, including CRT's where
 * you can erase overstrikes with some work.  CRT's which do underlining
 * implicitly which has to be erased (like CONCEPTS) are also handled.
 */
vputchar(c)
	register int c;
{

#ifndef NONLS8 /* 8bit integrity */
	register short *tp;
#if defined NLS16 && defined EUC
	int col_vtub ;
	int cw_ch ;	/* column width of the character	*/
	int rem_col ;	/* the number of column remaining in the line	*/

	int ins_col = 0 ; /* the number of columns of the character to be overwritten   */
	int ins_byte = 0 ; /* the number of bytes of the character to be overwritten   */

	int src_col = 0 ; /* the number of columns of the character to be overwritten on */
	int src_byte = 0 ; /* the number of bytes of the character to be overwritten on */

	static short prevchar ; /* this variable contains the previous character overwritten on		*/
				/* and is used to access the second byte of CS2 char			*/

	static short prevccol ; /* this variable contains the column width of the previous character overwritten */
	static short diffcol ; /* this variable is used when CS2 character is overwritten on CS1 character */
	static int end_output ;	/* indicares whether output ended ( =1 ) or not ( =0 ) 	*/
	static int hold_sp_out ; /* determines whether a space must be put out or not	*/
				 /* to erace ramining part of the multi-column		*/
				 /* character						*/
	/* the following variables are used to move the screen image buffer vtube[]	*/
	int offset ;
	short *destptr ;
	short *srcptr ;
	short *endptr ;

#endif
#else NONLS8
	register char *tp;
#endif NONLS8

	register int d;

#ifndef	NLS16
	c &= (QUOTE|TRIM);
#else
	/*
	 * First byte of Kanji character that is suspended to write.
	 */
	static int susfirst= 0;

	c &= (QUOTE | TRIM | FIRST | SECOND | DUMMY_BLANK);
#endif

#ifdef TRACE
	if (trace) {
		fprintf( trace, "\n         vputchar( " );
		tracec(c);
		fprintf( trace, " ) " );
	}
#endif TRACE
	/* Fix problem of >79 chars on echo line. */
	if (destcol >= WCOLS-1 && splitw && destline == WECHO)
		pofix();

#ifdef	NLS16
	/*
	 * If the first byte of Kanji character will be placed on the
	 * edge of the terminal right margine, the character is placed
	 * on the beginning of the next line.
	 */
#ifdef EUC
	if ( IS_FIRST(c) ) {
		cw_ch = C_COLWIDTH( c & TRIM ) ;
		if ( WCOLS ) {
			rem_col = WCOLS - ( destcol % WCOLS ) ;
			if ( cw_ch > rem_col ) {
				while ( rem_col > 0 ) {
					vputchar(' '|DUMMY_BLANK);
					rem_col-- ;
				}
			}
		}
	}
#else EUC
	if (((destcol + 1) % WCOLS) == 0 && IS_FIRST(c))
		vputchar(' '|DUMMY_BLANK);
#endif EUC
	
#endif

#if defined NLS16 && defined EUC
	/* prevent losing the second byte of CS2 	*/
	if ( ( destcol >= WCOLS ) && ( ! ( IS_SECOND( c ) ) ) ) {
#else
	if (destcol >= WCOLS) {
#endif
		destline += destcol / WCOLS;
		destcol %= WCOLS;
	}
	if (destline > WBOT && (!splitw || destline > WECHO))
		vrollup(destline);
#if defined NLS16 && defined EUC
#ifdef VPDEBUG
	if (trace) {
		fprintf( trace, " \n destcol, diffcol, and prevchar  before vputchar = %d, %d, %d \n", destcol, diffcol, prevchar ) ;
	}
#endif VPDEBUG
	tp = vtube[destline] + vcolumn( vtube[destline], destcol ) - diffcol ;
	if ( IS_FIRST( prevchar ) ) {
		tp -= ( 2 - C_COLWIDTH( ( (int)prevchar ) & TRIM ) ) ;
	}
	/* we need to access the second byte of CS2 character only when	*/
	/* another CS2 characer is put					*/
	if ( ( IS_FIRST( *tp ) ) && ( IS_FIRST( c ) ) ){
		prevchar = *tp  ;
	}else {
		prevchar = 0 ;
	}
#else
	tp = vtube[destline] + destcol;
#endif
#ifdef VPDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		NTCHAR_TO_char(vtube[destline], cvbuf, BTOCRATIO*WCOLS -1 ) ;
		fprintf( trace, " \n Content of vtube[destline] before vputchar = %s \n", cvbuf ) ;
	}
#endif VPDEBUG
	switch (c) {

	case '\t':
		vgotab();
		return;

	case ' ':
		/*
		 * We can get away without printing a space in a number
		 * of cases, but not always.  We get away with doing nothing
		 * if we are not in insert mode, and not on a CONCEPT-100
		 * like terminal, and either not in hardcopy open or in hardcopy
		 * open on a terminal with no overstriking, provided,
		 * in all cases, that nothing has ever been displayed
		 * at this position.  Ugh.
		 */

#ifndef	NLS16
#ifdef	NLS
		if (!insmode && !insert_null_glitch && (state != HARDOPEN || over_strike) && (*tp&TRIM) == 0 && opp_fix) {
#else
		if (!insmode && !insert_null_glitch && (state != HARDOPEN || over_strike) && (*tp&TRIM) == 0) {
#endif
#else
		if (!insmode && !insert_null_glitch && (state != HARDOPEN || over_strike) && (*tp & (TRIM | FIRST | SECOND)) == 0 && opp_fix) {
#endif

			*tp = ' ';
			destcol++;
			return;
		}
		goto def;

	case QUOTE:
		if (insmode) {
			/*
			 * When in insert mode, tabs have to expand
			 * to real, printed blanks.
			 */
			c = ' ' | QUOTE;
			goto def;
		}
		if (*tp == 0) {
			/*
			 * A ``space''.
			 */
			if ((hold & HOLDPUPD) == 0)
				*tp = QUOTE;
			destcol++;
			return;
		}
		/*
		 * A ``space'' ontop of a part of a tab.
		 */
		if (*tp & QUOTE) {
			destcol++;
			return;
		}
		c = ' ' | QUOTE;
		/* fall into ... */

def:
	default:

#ifndef	NLS16
		d = *tp & TRIM;
#else
		/*
		 * If character to be put to the terminal is not
		 * DUMMY_BLANK and character displayed is DUMMY_BLANK,
		 * then tp is incremented by one.
		 * Otherwise, wasteful redraw operation will be occurred.
		 */

#ifdef EUC
		if ( IS_FIRST(c) && (*tp & DUMMY_BLANK) ){
           /* multi-column character can't be overwritten on	*/
	   /* the DUMMY_BLANK so skip the DUMMY_BLANK 		*/
			tp += ( C_COLWIDTH( c & TRIM ) - 1  );
		}
#else EUC
		if (IS_FIRST(c) && (*tp & DUMMY_BLANK))
			tp++;
#endif EUC
		d = *tp & (TRIM | FIRST | SECOND);
#endif

		/*
		 * Now get away with doing nothing if the characters
		 * are the same, provided we are not in insert mode
		 * and if we are in hardopen, that the terminal has overstrike.
		 */

#ifndef	NLS16
#ifdef	NLS
		if (d == (c & TRIM) && !insmode && (state != HARDOPEN || over_strike) && opp_fix) {
#else
		if (d == (c & TRIM) && !insmode && (state != HARDOPEN || over_strike)) {
#endif
#else
		if (d == (c & (TRIM | FIRST | SECOND)) && !insmode && (state != HARDOPEN || over_strike) && opp_fix) {
#ifdef EUC
			if ( IS_FIRST(c) ) {
				susfirst = c;
				end_output = 0 ;
			} else if (IS_SECOND(c)) {
				if (!susfirst){
				/*
				 * When 1st byte was printed and 2nd byte
				 * is same as current printed character on the
				 * screen, this 2nd byte is also printed.
				 */
					goto next;
				} else {
					destcol -= ( 2 - C_COLWIDTH( susfirst & TRIM ) ) ;
					susfirst = 0 ;
				}
				end_output = 1 ;
			}else{
				susfirst = 0;
				end_output = 1 ;
			}

#else EUC
			if (IS_FIRST(c))
				susfirst = c;
			else if (!susfirst && IS_SECOND(c))
				/*
				 * When 1st byte was printed and 2nd byte
				 * is same as current printed character on the
				 * screen, this 2nd byte is also printed.
				 */
					goto next;
			else
				susfirst = 0;
#endif EUC
#endif
			if ((hold & HOLDPUPD) == 0) {
				*tp = c ;
			}
			destcol++;
			return;
		}

#ifdef	NLS16
next:
		if (susfirst)
			destcol--;
#endif

		/*
		 * Backwards looking optimization.
		 * The low level cursor motion routines will use
		 * a cursor motion right sequence to step 1 character
		 * right.  On, e.g., a DM3025A this is 2 characters
		 * and printing is noticeably slower at 300 baud.
		 * Since the low level routines are not allowed to use
		 * spaces for positioning, we discover the common
		 * case of a single space here and force a space
		 * to be printed.
		 */
		if (destcol == outcol + 1 && tp[-1] == ' ' && outline == destline) {
			vputc(' ');
			outcol++;
		}

		/*
		 * This is an inline expansion a call to vcsync() dictated
		 * by high frequency in a profile.
		 */
		if (outcol != destcol || outline != destline)
			vgoto(destline, destcol);

		/*
		 * Deal with terminals which have overstrike.
		 * We handle erasing general overstrikes, erasing
		 * underlines on terminals (such as CONCEPTS) which
		 * do underlining correctly automatically (e.g. on nroff
		 * output), and remembering, in hardcopy mode,
		 * that we have overstruct something.
		 */

#ifndef	NLS16
#ifdef	NLS
		if (!insmode && d && d != ' ' && d != (c & TRIM) && opp_fix) {
#else
		if (!insmode && d && d != ' ' && d != (c & TRIM)) {
#endif
#else
		if (!insmode && d && d != ' ' && d != (c & (TRIM | FIRST | SECOND)) && opp_fix) {
#endif

			if (erase_overstrike && (over_strike || transparent_underline && (c == '_' || d == '_'))) {
				vputc(' ');
				outcol++, destcol++;
				back1();
			} else
				rubble = 1;
		}


		/*
		 * In insert mode, put out the insert_character sequence, padded
		 * based on the depth of the current line.
		 * A terminal which had no real insert mode, rather
		 * opening a character position at a time could do this.
		 * Actually should use depth to end of current line
		 * but this rarely matters.
		 */
		if (insmode)
			vputp(insert_character, DEPTH(vcline));

#ifndef	NLS16
		vputc(c & TRIM);
#else
		/*
		 * If c is second byte of Kanji character and its
		 * first byte is suspended to write, then
		 * both first and second byte need to write.
		 * Otherwise, only second byte will be written and
		 * character will be disguised.
		 */
		if (susfirst)	{
#ifdef EUC
			prevccol = C_COLWIDTH( susfirst & TRIM ) ;
#endif EUC
			vputc(susfirst & (TRIM | FIRST | SECOND));
			destcol++, outcol++;
			susfirst = 0;
		}
		vputc(c & (TRIM | FIRST | SECOND));
#endif


		/*
		 * In insert mode, insert_padding is a post insert pad.
		 */
		if (insmode)
			vputp(insert_padding, DEPTH(vcline));
		destcol++, outcol++;

#ifdef	NLS16
#ifdef EUC
		
		if ( IS_SECOND( c ) ) {
			outcol -= ( 2 - prevccol ) ;
			destcol -= ( 2 - prevccol ) ;
			end_output = 1 ;
		}else if ( IS_FIRST( c ) ) {
			prevccol = C_COLWIDTH( c & TRIM ) ;
			end_output = 0 ;
		}else {
			end_output = 1 ;
		}
		if (!insmode) {

			/* if c is not a multi-byte character then column size = 1*/

			if (!IS_FIRST(c)) {
				ins_col = 1 ;
				ins_byte = 1 ;
			}else{
			/* if c is a multi-byte character then get the column size*/

				ins_col = C_COLWIDTH( c & TRIM );
				ins_byte = 2 ;
			}
			if ( !IS_SECOND( *(tp+1) ) ) {
			/* if prevchar is not a multi-byte character then column size = 1*/
				src_col  = 1 ;
				src_byte = 1 ;
			}else{
			/* if prevchar is a multi-byte character then get the column size*/
				src_col =  C_COLWIDTH( ( (int)(*tp) ) & TRIM ) ; 
				src_byte = 2 ;
			}
#ifdef VPDEBUG
			if (trace)
				fprintf( trace, " \n Overwrite \n destcol=%d, ins_col=%d, ins_byte=%d, src_col=%d, src_byte=%d \n", destcol, ins_col, ins_byte, src_col, src_byte ) ;
#endif VPDEBUG
			if ( ( ins_col != src_col ) || ( ins_byte != src_byte ) ){
				/* if c is UJIS CS2 and the character to be overwitten on is 	*/
				/* not CS2 then expand vtube					*/

				if( ins_col < ins_byte ) {
				/* calculate the position for the screen image buffer, vtube[]	*/
					col_vtub = vcolumn( vtube[destline], destcol ) ;
					offset = ins_byte - ins_col ;
					destptr = vtube[destline] + BTOCRATIO * WCOLS -1 ;
					srcptr = destptr - offset  ;
					endptr = vtube[destline] + col_vtub ;
					while( srcptr >= endptr ) {
						*destptr-- = *srcptr-- ;
					}
				}

				/* if c is not UJIS CS2 and the character to be overwritten	*/
				/* on is CS2 then erase the second byte and shift vtube		*/
				if ( src_col < src_byte ) {

				/* calculate the position for the screen image buffer, vtube[]	*/
					col_vtub = vcolumn( vtube[destline], destcol ) ;
					offset = src_byte - src_col ;
					destptr = vtube[destline] + col_vtub -1 ;
					srcptr  = destptr + offset ;
					endptr = vtube[destline] + BTOCRATIO*WCOLS - 1 ;
					while ( srcptr <= endptr ) {
						*destptr++ = *srcptr++ ;
					}
					if( b_doomed ) {
						b_doomed -= offset ;
					}
					prevchar = 0 ;
				}
			} 

			/* if CS2 is overwritten on CS1, after the first byte	was 	*/
			/* overwritten, the character is misunderstood as CS2	*/
			/* by vcolumn() and the address calculation will be wrong	*/
			/* so we back the pointer by diffcol.				*/
			if( ( ins_byte == src_byte ) && ( ins_col < src_col ) ) { 
				diffcol = src_col - ins_col ;
			}else{
				diffcol = 0 ;
			}
			/*
			 * If ANK or Kanji 2nd byte will be overwritten on the Kanji
			 * 1st byte character, its destroyed 2nd byte is possibly
			 * remained as it is on the display.
			 * To avoid it, this destroyed 2nd byte
			 * should be replaced with a space character.
			 * This operation might duplicate with the terminal
			 * functionality.
			 */
			/* if c is UJIS CS2 and the character to be	 */
			/* if the column size of the character on the screen is larger */
			/* than that of the character to be overwritten, the remaining */
			/* columns are replaced with space characters		       */
			if ( ( ( src_col > ins_col ) && ( IS_SECOND( *( tp+1 ) ) ) ) || hold_sp_out  ) {
				if( end_output ) {
					hold_sp_out = 0 ;
					vputc(' ') ;
					outcol++ ;
					vgoto(destline, destcol) ;
				}else {
					hold_sp_out = 1 ;
				}
			}
		}
#else EUC
		/*
		 * If ANK or Kanji 2nd byte will be overwritten on the Kanji
		 * 1st byte character, its destroyed 2nd byte is possibly
		 * remained as it is on the display.
		 * To avoid it, this destroyed 2nd byte
		 * should be replaced with a space character.
		 * This operation might duplicate with the terminal
		 * functionality.
		 */
		if (!insmode && IS_SECOND(vtube[destline][destcol])
			&& !IS_FIRST(c))	{
			vputc(' ');
			outcol++;
			vgoto(destline, destcol);
		}
#endif EUC
#endif
		/*
		 * Unless we are just bashing characters around for
		 * inner working of insert mode, update the display.
		 */
		if ((hold & HOLDPUPD) == 0) {
			*tp = c;
		}

		/*
		 * CONCEPT braindamage in early models:  after a wraparound
		 * the next newline is eaten.  It's hungry so we just
		 * feed it now rather than worrying about it.
		 * Fixed to use	return linefeed to work right
		 * on vt100/tab132 as well as concept.
		 */
#if defined NLS16 && defined EUC
		/*	Prevent the second byte of CS2 being lost	*/
		if (eat_newline_glitch && outcol % WCOLS == 0 && (!( IS_FIRST(c) ) ) ) {
#else
		if (eat_newline_glitch && outcol % WCOLS == 0) {
#endif
			vputc('\r');
			vputc('\n');
		}
	}
#if defined NLS16 && defined EUC
#ifdef VPDEBUG
	if (trace) {
		cvbuf = cvlinebuf ;
		NTCHAR_TO_char(vtube[destline], cvbuf, BTOCRATIO*WCOLS -1) ;
		fprintf( trace, " \n Content of vtube[destline] after vputchar = %s \n", cvbuf ) ;
	}
#endif VPDEBUG
#endif
}

/*
 * Delete display positions stcol through endcol.
 * Amount of use of special terminal features here is limited.
 */
physdc(stcol, endcol)
	int stcol, endcol;
{

#ifndef NONLS8 /* 8bit integrity */
	register short *tp, *up;
	short *tpe;
#else NONLS8
	register char *tp, *up;
	char *tpe;
#endif NONLS8

	register int i;
	register int nc = endcol - stcol;
#if defined NLS16 && defined EUC
	int j ;
	int nb = 0 ;	/* the number of bytes to be deleted	*/
#endif

#ifdef IDEBUG
	if (trace)
		tfixnl(), fprintf(trace, "physdc(%d, %d)\n", stcol, endcol);
#endif
	if (!delete_character || nc <= 0)
		return;
	if (insert_null_glitch) {
		/*
		 * CONCEPT-100 like terminal.
		 * If there are any ``spaces'' in the material to be
		 * deleted, then this is too hard, just retype.
		 */
		vprepins();
#if defined NLS16 && defined EUC
		up = vbuf0 + vcolumn( vbuf0, stcol ) ;
#else
		up = vtube0 + stcol;
#endif
		i = nc;
		do {
#if defined NLS16 && defined EUC
			if ( IS_FIRST( *up ) ) {
				i += ( 2 - C_COLWIDTH( ( (int)(*up) ) & TRIM ) ) ;
			}
#endif
			if ((*up++ & (QUOTE|TRIM)) == QUOTE)
				return;
		} while (--i);

		i = 2 * nc ;

		do {
#if defined NLS16 && defined EUC
			if ( IS_FIRST( *up ) ) {
				i += ( 2 - C_COLWIDTH( ( (int)(*up) ) & TRIM ) ) ;
			}
#endif
			if (*up == 0 || (*up++ & QUOTE) == QUOTE)
				return;
		} while (--i);
		vgotoCL(stcol);
	} else {
		/*
		 * HP like delete mode.
		 * Compute how much text we are moving over by deleting.
		 * If it appears to be faster to just retype
		 * the line, do nothing and that will be done later.
		 * We are assuming 2 output characters per deleted
		 * characters and that clear to end of line is available.
		 */

		i = stcol / WCOLS;
		/* stcol is not on the same line as endcol is on	*/
		if (i != endcol / WCOLS)
			return;
		i += LINE(vcline);
		stcol %= WCOLS;
		endcol %= WCOLS;
#if defined NLS16 && defined EUC
		up = vtube[i] ;
		nb = vcolumn( up, endcol ) - vcolumn( up, stcol ) ;
		tp = up + vcolumn( up, endcol ) ;
		tpe = up + ( BTOCRATIO * WCOLS ) ;
		while (tp < tpe && *tp ){
			tp++ ;
		}
#else
		up = vtube[i]; tp = up + endcol; tpe = up + WCOLS;
		while (tp < tpe && *tp)
			tp++;
#endif
		if (tp - (up + stcol) < 2 * nc)
			return;
		vgoto(i, stcol);
	}

	/*
	 * Go into delete mode and do the actual delete.
	 * Padding is on delete_character itself.
	 */
	godm();
	for (i = nc; i > 0; i--)
		vputp(delete_character, DEPTH(vcline));
	vputp(exit_delete_mode, 0);

	/*
	 * Straighten up.
	 * With CONCEPT like terminals, characters are pulled left
	 * from first following null.  HP like terminals shift rest of
	 * this (single physical) line rigidly.
	 */
	if (insert_null_glitch) {
#if defined NLS16 && defined EUC
		up = vbuf0 + vcolumn( vbuf0, stcol ) ;
		tp = vbuf0 + vcolumn( vbuf0, endcol ) ;
		nb = vcolumn( vbuf0, endcol ) - vcolumn( vbuf0, stcol ) ;
#else
		up = vtube0 + stcol;
		tp = vtube0 + endcol;
#endif
		while (i = *tp++) {
			if ((i & (QUOTE|TRIM)) == QUOTE)
				break;
			*up++ = i;
		}
		do
			*up++ = i;
#if defined NLS16 && defined EUC
		while (--nb);
	/* update vtube						*/
	/* i represents the number of column			*/
	/* j represents the number of line			*/
	up = vbuf0 ;
	tp = vtube0 ;
	for( i =0, j = 0 ; *up ; i++ ) {
		if( i % WCOLS == 0 ) {
			tp = vtube0 + j * BTOCRATIO * WCOLS ;
			vclrbyte( tp, BTOCRATIO*WCOLS);
			j++ ;
		}
		if( IS_FIRST( *up ) ) {
			i -= ( 2 - C_COLWIDTH( ( (int)(*up) ) & TRIM ) ) ;
			i++ ;
			*tp++ = *up++ ;
		}
		*tp++ = *up++ ;
	}
	/* end of update		*/
#else
		while (--nc);
#endif
	} else {

#ifndef NONLS8 /* 8bit integrity */
#ifndef	NLS16
		copys(up + stcol, up + endcol, WCOLS - endcol);
#else
		/*
		 * The DUMMY_BLANK on the end of this line must be
		 * avoided to copy in the middle of this line.
		 * DUMMY_BLANK is changed with null character.
		 * This operation depends on vclrbyte().
		 */
#ifdef EUC
		tp = up + vcolumn( up, endcol ) ;
		up += vcolumn( up, stcol ) ;
		for (i = BTOCRATIO * WCOLS - vcolumn( vbuf0, endcol ) ; i > 0; i--)	{
#else EUC
		tp = up + endcol;
		up += stcol;
		for (i = WCOLS - endcol; i > 0; i--)	{
#endif EUC
			if (*tp & DUMMY_BLANK)
				*up++ = 0 ;
			else
				*up++ = *tp ;
			tp++;
		}
#endif
#else NONLS8
		copy(up + stcol, up + endcol, WCOLS - endcol);
#endif NONLS8

#if defined NLS16 && defined EUC
		vclrbyte(tpe - nb, nb);
#else
		vclrbyte(tpe - nc, nc);
#endif
	}
}

#if defined NLS16 && defined EUC
/*
 * Delete data in the screen image buffer stcol through endbyte.
 */
bphysdc(stcol, endbyte)
	int stcol, endbyte ;
{

	register short *tp, *up;
	short *tpe;
	register int i;
	register int nb = endbyte - stcol ;
	int j ;

#ifdef IDEBUG
	if (trace)
		tfixnl(), fprintf(trace, "bphysdc(%d, %d)\n", stcol, endbyte);
#endif
	if ( nb <= 0)
		return;
	/*
	 * Straighten up.
	 * With CONCEPT like terminals, characters are pulled left
	 * from first following null.  HP like terminals shift rest of
	 * this (single physical) line rigidly.
	 */

	if (insert_null_glitch) {
		vprepins();
		up = vbuf0 + vcolumn( vbuf0, stcol ) ;
		tp = vbuf0 + endbyte;
		nb = tp - up ;
		while (i = *tp++) {
			if ((i & (QUOTE|TRIM)) == QUOTE)
				break;
			*up++ = i;
		}
		do
			*up++ = i;
		while (--nb);
		/* update vtube						*/
		/* i represents the number of column			*/
		/* j represents the number of line			*/
		up = vbuf0 ;
		tp = vtube0 ;
		for( i =0, j = 0 ; *up ; i++ ) {
			if( i % WCOLS == 0 ) {
				vclrbyte( tp, BTOCRATIO*WCOLS ) ;
				tp = vtube0 + j * BTOCRATIO * WCOLS ;
				j++ ;
			}
			if( IS_FIRST( *up ) ) {
				i -= ( 2 - C_COLWIDTH( ( (int)(*up) ) & TRIM ) ) ;
				i++ ;
				*tp++ = *up++ ;
			}
			*tp++ = *up++ ;
		}
		/* end of update		*/
	} else {
		/*
		 * HP like delete mode.
		 */
		i = stcol / WCOLS;
		/* stcol is not on the same line as endcol is on	*/
		if (i != endbyte / WCOLS)
			return;
		i += LINE(vcline);
		stcol %= WCOLS;
		endbyte %= WCOLS;
		nb = endbyte - stcol ;
		/*
		 * The DUMMY_BLANK on the end of this line must be
		 * avoided to copy in the middle of this line.
		 * DUMMY_BLANK is changed with null character.
		 * This operation depends on vclrbyte().
		 */
		up = vtube[i] + vcolumn( vtube[i], stcol ) ;
		tp = vtube[i] + endbyte ;
		tpe = vtube[i] + BTOCRATIO * WCOLS ;
		
		for (i = BTOCRATIO * WCOLS - endbyte ; i > 0; i--)	{
			if (*tp & DUMMY_BLANK)
				*up++ = 0 ;
			else
				*up++ = *tp ;
			tp++;
		}

		vclrbyte(tpe - nb, nb);
	}
}

/*
 * Delete display positions stcol through endcol only on the screen.
 * Amount of use of special terminal features here is limited.
 */
vphysdc(stcol, endcol)
	int stcol, endcol;
{

	register short *tp, *up;
	short *tpe;

	register int i;
	register int nc = endcol - stcol;
	int j ;
	int nb = 0 ;	/* the number of bytes to be deleted	*/

#ifdef IDEBUG
	if (trace)
		tfixnl(), fprintf(trace, " \n vphysdc(%d, %d)\n", stcol, endcol);
#endif
	if (!delete_character || nc <= 0)
		return;
	if (insert_null_glitch) {
		/*
		 * CONCEPT-100 like terminal.
		 * If there are any ``spaces'' in the material to be
		 * deleted, then this is too hard, just retype.
		 */
		vprepins();
		up = vbuf0 + vcolumn( vbuf0, stcol ) ;
		i = nc;
		do {
			if ( IS_FIRST( *up ) ) {
				i += ( 2 - C_COLWIDTH( ( (int)(*up) ) & TRIM ) ) ;
			}
			if ((*up++ & (QUOTE|TRIM)) == QUOTE)
				return;
		} while (--i);

		i = 2 * nc ;

		do {
			if ( IS_FIRST( *up ) ) {
				i += ( 2 - C_COLWIDTH( ( (int)(*up) ) & TRIM ) ) ;
			}
			if (*up == 0 || (*up++ & QUOTE) == QUOTE)
				return;
		} while (--i);
		vgotoCL(stcol);
	} else {
		/*
		 * HP like delete mode.
		 */

		i = stcol / WCOLS;
		/* stcol is not on the same line as endcol is on	*/
		if (i != endcol / WCOLS)
			return;
		i += LINE(vcline);
		stcol %= WCOLS;
		endcol %= WCOLS;
		vgoto(i, stcol);
	}

	/*
	 * Go into delete mode and do the actual delete.
	 * Padding is on delete_character itself.
	 */
	godm();
	for (i = nc; i > 0; i--)
		vputp(delete_character, DEPTH(vcline));
	vputp(exit_delete_mode, 0);


}
/* This function was introduced to calculate the position in the screen */
/* image buffer from the position on the screen indicated by column	*/
int
vcolumn( ptr , column )
short *ptr ; /* starting point of the buffer 	*/
int column ; /* the position measured by column */
	     /* on the screen                   */
{
	int vcol  ;    /* column in the buffer 	*/
	int countcol ; /* column on the screen	*/
	int charcol;   /* the number of columns occupied	*/
		       /* by the character			*/
	int limit ;	/* value of limit		*/
	int neglimit ;	/* negative value of limit	*/
	limit = BTOCRATIO * column ;
	if( ptr == ( CHAR* )0 ) {
		return( column ) ;
	}
	if( column >= 0 ) {
		for ( countcol=0, vcol=0 ; vcol <= limit && countcol < column ; ptr++, vcol++ ){
			if ( IS_FIRST(*ptr) ) {
				if ( IS_SECOND( *( ptr + 1 ) ) ) {
					countcol += ( C_COLWIDTH( ( (int)(*ptr) ) & TRIM ) -1 ) ;
				}else{
					countcol ++ ;
				}
				
			}else{
				countcol++ ;
			}
		}
		if ( vcol > limit ) {
			return( column ) ;
		}
		return(vcol);
	}else{
		neglimit = -1 * limit ;
		for ( countcol=0, vcol=0 ; neglimit <= vcol &&  column < countcol ; ptr--, vcol-- ){
			if ( IS_SECOND(*ptr) ) {
				if ( IS_FIRST( *( ptr - 1 ) ) ) {
					countcol -= ( C_COLWIDTH( ( (int)( *( ptr - 1 ) ) ) & TRIM ) -1 ) ;
				}else{
					countcol -- ;
				}
				
			}else{
				countcol-- ;
			}
		}
		if ( vcol < neglimit ) {
			return( column ) ;
		}
	}
}
#endif

#ifdef TRACE
#if defined NLS16 && defined EUC
TCHAR_TO_char( a, b )
register CHAR *a ;
register char *b ;
{ 
	register int i ;
	for( i = 0 ; i < TUBESIZE -1  ; i++ ){
		if ( ( *a & TRIM ) == 0 ) {
			*b++ = ' ' ;
			a++ ;
			i++ ;
		}else {
			*b++ = *a++ ;
		}
	}
	*b = '\0';
}

NTCHAR_TO_char( a, b, n )
register CHAR *a ;
register char *b ;
register int n ;
{ 
	register int i ;
	for( i = 0 ; i < n -1  ; i++ ){
		if ( ( *a & TRIM ) == 0 ) {
			*b++ = ' ' ;
			a++ ;
			i++ ;
		}else {
			*b++ = *a++ ;
		}
	}
	*b = '\0';
}

#endif

tfixnl()
{

	if (trubble || techoin)
		fprintf(trace, "\n");
	trubble = 0, techoin = 0;
}

tvliny()
{
	register int i;

	if (!trace)
		return;
	tfixnl();
	fprintf(trace, "vcnt = %d, vcline = %d, vliny = ", vcnt, vcline);
	for (i = 0; i <= vcnt; i++) {
		fprintf(trace, "%d", LINE(i));
		if (FLAGS(i) & VDIRT)
			fprintf(trace, "*");
		if (DEPTH(i) != 1)
			fprintf(trace, "<%d>", DEPTH(i));
		if (i < vcnt)
			fprintf(trace, " ");
	}
	fprintf(trace, "\n");
}

tracec(c)
	int c;		/* mjm: char --> int */
{

	if (!techoin)
		trubble = 1;
	if (c == ESCAPE)
		fprintf(trace, "$");
	else if (c & QUOTE)	/* mjm: for 3B (no sign extension) */
		fprintf(trace, "~%c", ctlof(c&TRIM));
	else if (c < ' ' || c == DELETE)
		fprintf(trace, "^%c", ctlof(c));
	else
		fprintf(trace, "%c", c);
}

/*
 * Put a character with possible tracing.
 */
vputch(c)
	int c;
{

	if (trace)
		tracec(c);
	vputc(c);
}
#endif
