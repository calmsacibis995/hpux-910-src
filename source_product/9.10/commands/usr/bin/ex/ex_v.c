/* @(#) $Revision: 70.5 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_re.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Entry points to open and visual from command mode processor.
 * The open/visual code breaks down roughly as follows:
 *
 * ex_v.c	entry points, checking of terminal characteristics
 *
 * ex_vadj.c	logical screen control, use of intelligent operations
 *		insert/delete line and coordination with screen image;
 *		updating of screen after changes.
 *
 * ex_vget.c	input of single keys and reading of input lines
 *		from the echo area, handling of \ escapes on input for
 *		uppercase only terminals, handling of memory for repeated
 *		commands and small saved texts from inserts and partline
 *		deletes, notification of multi line changes in the echo
 *		area.
 *
 * ex_vmain.c	main command decoding, some command processing.
 *
 * ex_voperate.c   decoding of operator/operand sequences and
 *		contextual scans, implementation of word motions.
 *
 * ex_vops.c	major operator interfaces, undos, motions, deletes,
 *		changes, opening new lines, shifts, replacements and yanks
 *		coordinating logical and physical changes.
 *
 * ex_vops2.c	subroutines for operator interfaces in ex_vops.c,
 *		insert mode, read input line processing at lowest level.
 *
 * ex_vops3.c	structured motion definitions of ( ) { } and [ ] operators,
 *		indent for lisp routines, () and {} balancing. 
 *
 * ex_vput.c	output routines, clearing, physical mapping of logical cursor
 *		positioning, cursor motions, handling of insert character
 *		and delete character functions of intelligent and unintelligent
 *		terminals, visual mode tracing routines (for debugging),
 *		control of screen image and its updating.
 *
 * ex_vwind.c	window level control of display, forward and backward rolls,
 *		absolute motions, contextual displays, line depth determination
 */

#ifndef NONLS8 /* 8bit integrity */
#if defined NLS16 && defined EUC
	short	atube[VBUFSIZE + LBSIZE];
#else
	short	atube[TUBESIZE + LBSIZE];
#endif
#else NONLS8
	char	atube[TUBESIZE + LBSIZE];
#endif NONLS8

#ifndef NONLS8 /* User messages */
# define	NL_SETN	14	/* set number */
# include	<msgbuf.h>
# undef	getchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

#ifdef NLS16
CHAR	dummy_null = 0;			/* "" for 16-bit */
#endif

/*
 * Enter open mode
 */
oop()
{
	register CHAR *ic;

	ttymode f;	/* mjm: was register */

	ovbeg();
	if (peekchar() == '/') {
		ignore(compile(getchar()));
		savere(scanre);
		if (execute(0, dot) == 0)
			error((nl_msg(1, "Fail|Pattern not found on addressed line")));
		ic = loc1;
		if (ic > linebuf && *ic == 0)
			ic--;
	} else {
		getDOT();
		ic = vskipwh(linebuf);
	}
	donewline();

	/*
	 * If overstrike then have to HARDOPEN
	 * else if can move cursor up off current line can use CRTOPEN (~~vi1)
	 * otherwise (ugh) have to use ONEOPEN (like adm3)
	 */
	if (over_strike && !erase_overstrike)
		bastate = HARDOPEN;
	else if (cursor_address || cursor_up)
		bastate = CRTOPEN;
	else
		bastate = ONEOPEN;
	setwind();

	/*
	 * To avoid bombing on glass-crt's when the line is too long
	 * pretend that such terminals are 160 columns wide.
	 * If a line is too wide for display, we will dynamically
	 * switch to hardcopy open mode.
	 */
	if (state != CRTOPEN)
		WCOLS = TUBECOLS;
	if (!inglobal)
		savevis();
	vok(atube);
	if (state != CRTOPEN)
		columns = WCOLS;
	Outchar = vputchar;
	f = ostart();
	if (state == CRTOPEN) {
		if (outcol == UKCOL)
			outcol = 0;
		vmoveitup(1, 1);
	} else
		outline = destline = WBOT;
	vshow(dot, NOLINE);
	vnline(ic);
#ifdef SIGWINCH
	invisual = 1;
	ign_winch = 0;
	(void) signal (SIGWINCH, winch);
#endif SIGWINCH

	vmain();

#ifdef SIGWINCH
	invisual = 0;
	(void) signal (SIGWINCH, SIG_IGN);
	ign_winch = 1;
#endif SIGWINCH

	if (state != CRTOPEN)
		vclean();
	Command = "open";
	ovend(f);
}

ovbeg()
{

	if (inopen)
		error((nl_msg(2, "Recursive open/visual not allowed")));
	Vlines = lineDOL();
	fixzero();
	setdot();
	pastwh();
	dot = addr2;
}

ovend(f)
	ttymode f;
{

	splitw++;
	vgoto(WECHO, 0);
	vclreol();
	vgoto(WECHO, 0);
	holdcm = 0;
	splitw = 0;
	ostop(f);
	setoutt();
	undvis();
	columns = OCOLUMNS;
	inopen = 0;
	flusho();
	netchHAD(Vlines);
}

/*
 * Enter visual mode
 */
vop()
{
	register int c;
	ttymode f;	/* mjm: was register */
	extern char termtype[];

	if (!cursor_address && !cursor_up) {
		if (initev) {
toopen:
			if (generic_type)
				merror((nl_msg(3, "I don't know what kind of terminal you are on - all I have is '%s'.")), termtype);
			putNFL();
			merror((nl_msg(4, "[Using open mode]")));
			putNFL();
			oop();
			return;
		}
		error((nl_msg(5, "Visual needs addressible cursor or upline capability")));
	}
	if (over_strike && !erase_overstrike) {
		if (initev)
			goto toopen;
		error((nl_msg(6, "Can't use visual on a terminal which overstrikes")));
	}
	if (!clear_screen) {
		if (initev)
			goto toopen;
		error((nl_msg(7, "Visual requires clear screen capability")));
	}
	if (!scroll_forward) {
		if (initev)
			goto toopen;
		error((nl_msg(8, "Visual requires scrolling")));
	}
	ovbeg();
	bastate = VISUAL;
	c = 0;
	if (any(peekchar(), "+-^."))
		c = getchar();
	pastwh();

#ifndef NONLS8 /* Character set features */
	vsetsiz(((peekchar() >= IS_MACRO_LOW_BOUND) && isdigit(peekchar() & TRIM)) ? getnum() : value(WINDOW));
#else NONLS8
	vsetsiz(((peekchar() >= IS_MACRO_LOW_BOUND) && isdigit(peekchar())) ? getnum() : value(WINDOW));
#endif NONLS8

	setwind();
	donewline();
	vok(atube);
	if (!inglobal)
		savevis();
	Outchar = vputchar;
	vmoving = 0;
	f = ostart();
	if (initev == 0) {
		vcontext(dot, c);
		vnline(NOCHAR);
	}
#ifdef SIGWINCH
	invisual = 1;
	ign_winch = 0;
	(void) signal (SIGWINCH, winch);
#endif SIGWINCH	

	vmain();

#ifdef SIGWINCH
	invisual = 0;
	(void) signal (SIGWINCH, SIG_IGN);
	ign_winch = 1;
#endif SIGWINCH

	Command = "visual";
	ovend(f);
}

#ifdef ED1000
var	diff;
#endif ED1000

/*
 * Hack to allow entry to visual with
 * empty buffer since routines internally
 * demand at least one line.
 */
fixzero()
{

	if (dol == zero) {
		register bool ochng = chng;


#ifndef	NLS16
		vdoappend("");
#else
		vdoappend(&dummy_null);
#endif

		/* flag that the line just added is not real */
		real_empty++;

		if (!ochng)
			sync();
		addr1 = addr2 = one;
	} else if (addr2 == zero)
		addr2 = one;
}

/*
 * Save lines before visual between unddol and truedol.
 * Accomplish this by throwing away current [unddol,truedol]
 * and then saving all the lines in the buffer and moving
 * unddol back to dol.  Don't do this if in a global.
 *
 * If you do
 *	g/xxx/vi.
 * and then do a
 *	:e xxxx
 * at some point, and then quit from the visual and undo
 * you get the old file back.  Somewhat weird.
 */
savevis()
{

	if (inglobal)
		return;
	truedol = unddol;
	saveall();
	unddol = dol;
	undkind = UNDNONE;
}

/*
 * Restore a sensible state after a visual/open, moving the saved
 * stuff back to [unddol,dol], and killing the partial line kill indicators.
 */
undvis()
{
	register int i;

	if (ruptible)
		signal(SIGINT, onintr);
	squish();
	pkill[0] = pkill[1] = 0;
	unddol = truedol;
	unddel = zero;
	undap1 = one;
	undap2 = dol + 1;
	undkind = UNDALL;
	if (undadot <= zero || undadot > dol)
		undadot = zero+1;

#ifndef NONLS8 /* 8bit integrity */
# ifndef	NLS16
	vutmp = (char *) 0;
	vtube0 = (short *)vutmp;
# else
	vtube0 = vutmp = (CHAR *) 0;
# endif
#else NONLS8
	vtube0 = vutmp = (char *) 0;
#endif NONLS8

	for (i=0; i < TUBELINES; i++)

#ifndef NONLS8 /* 8bit integrity */
		vtube[i] = (short *) 0;
#else NONLS8
		vtube[i] = (char *) 0;
#endif NONLS8

	WCOLS = 0;
}

/*
 * Set the window parameters based on the base state bastate
 * and the available buffer space.
 */
setwind()
{

	WCOLS = columns;
	switch (bastate) {

	case ONEOPEN:
		if (auto_right_margin)
			WCOLS--;
		/* fall into ... */

	case HARDOPEN:
		basWTOP = WTOP = WBOT = WECHO = 0;
		ZERO = 0;
		holdcm++;
		break;

	case CRTOPEN:
		basWTOP = lines - 2;
		/* fall into */

	case VISUAL:
		ZERO = lines - TUBESIZE / WCOLS;
		if (ZERO < 0)
			ZERO = 0;
		if (ZERO > basWTOP)
			error((nl_msg(9, "Screen too large for internal buffer")));
		WTOP = basWTOP; WBOT = lines - 2; WECHO = lines - 1;
		break;
	}
	state = bastate;
	basWLINES = WLINES = WBOT - WTOP + 1;
}

/*
 * Can we hack an open/visual on this terminal?
 * If so, then divide the screen buffer up into lines,
 * and initialize a bunch of state variables before we start.
 */
vok(atube)

#ifndef NONLS8 /* 8bit integrity */
	register short *atube;
#else NONLS8
	register char *atube;
#endif NONLS8

{
	register int i;

	if (WCOLS == 1000)
		serror((nl_msg(10, "Don't know enough about your terminal to use %s")), Command);
	if (WCOLS > TUBECOLS)
		error((nl_msg(11, "Terminal too wide")));
	if (WLINES >= TUBELINES || WCOLS * (WECHO - ZERO + 1) > TUBESIZE)
		error((nl_msg(12, "Screen too large")));

	vtube0 = atube;
#if defined NLS16 && defined EUC
	vclrbyte(atube, BTOCRATIO * WCOLS * (WECHO - ZERO + 1));
#else
	vclrbyte(atube, WCOLS * (WECHO - ZERO + 1));
#endif
	for (i = 0; i < ZERO; i++)

#ifndef NONLS8 /* 8bit integrity */
		vtube[i] = (short *) 0;
#else NONLS8
		vtube[i] = (char *) 0;
#endif NONLS8

	for (; i <= WECHO; i++)
#if defined NLS16 && defined EUC
		vtube[i] = atube, atube +=  BTOCRATIO * WCOLS;
#else
		vtube[i] = atube, atube += WCOLS;
#endif
	for (; i < TUBELINES; i++)

#ifndef NONLS8 /* 8bit integrity */
		vtube[i] = (short *) 0;
/* UCSqm00078: 
 * Do not reset vutmp, vundkind and vUNDdot during SIGWINCH, so that
 * the undo command will work after the SIGWINCH handler.
 */
#ifdef SIGWINCH
	if (!got_winch) 
#endif SIGWINCH
#ifndef	NLS16
	vutmp = (char *)atube;
#else
	vutmp = atube;
#endif
#else NONLS8
		vtube[i] = (char *) 0;
#ifdef SIGWINCH
	if (!got_winch) 
#endif SIGWINCH
	vutmp = atube;
#endif NONLS8

#ifdef SIGWINCH
	if (!got_winch) {
#endif SIGWINCH
	vundkind = VNONE;
	vUNDdot = 0;
#ifdef SIGWINCH
	}
#endif SIGWINCH
	OCOLUMNS = columns;
	inopen = 1;
#ifdef CBREAK
	signal(SIGINT, vintr);
#endif
	vmoving = 0;
	splitw = 0;
	doomed = 0;
	holdupd = 0;
	Peekkey = 0;
#ifdef SIGWINCH
	if (!got_winch)
#endif SIGWINCH
		vcnt = vcline = 0;

	if (vSCROLL == 0)
		vSCROLL = value(SCROLL);
}

#ifdef CBREAK
vintr()
{

	signal(SIGINT, vintr);
	if (vcatch)
		onintr();
	ungetkey(ATTN);
	draino();
}
#endif

/*
 * Set the size of the screen to size lines, to take effect the
 * next time the screen is redrawn.
 */
vsetsiz(size)
	int size;
{
	register int b;

	if (bastate != VISUAL)
		return;
	b = lines - 1 - size;
	if (b >= lines - 1)
		b = lines - 2;
	if (b < 0)
		b = 0;
	basWTOP = b;
	basWLINES = WBOT - b + 1;
}

#ifdef SIGWINCH
/*
 * SIGWINCH signal handler
 */

winch()
{
        short save_vcline;
        extern jmp_buf vappend_env;

        got_winch = 1;  /* Global flag used in subsequent routines to      */
                        /* modify behavior because we are in the           */
                        /* SIGWINCH signal handler.                        */

        vsave();        /* Save the current line as a safe guard.          */

        setsize(0);     /* Get new window values; reset lines, columns.    */
        vsetsiz(value(WINDOW));  /* Reset window variables due to resize.  */
        setwind();
        vok(atube);     /* Change the capacity of vtube/atube to match     */
                        /* the new window size.                            */

	

        /*  The code below is *almost* the same as if we had enter ^L      */
        /*  to refresh screen                                              */
        vclear();          /* clear the screen                             */
        vdirty(0, vcnt);   /* mark all lines as dirty, so they are redrawn */


        /* If the "current line [vcline]" is no longer in the new window   */
        /* (say the current line was at line 35 on the screen, and now the */
        /* new window goes to line 22), then we want to redraw the screen  */
        /* with the current line at the bottom (i.e., try to get the same  */
        /* relative position).  We accomplish this with vcontext.          */

        if (LINE(vcline) > WBOT)
                vcontext(dot, '-'); /* Redraw the screen, with the current */
                                    /* line in the "old" window as the     */
                                    /* last line in the "new" window.      */
        else
                vredraw(WTOP);  /* Redraw the screen, keeping the line     */
                                /* currently at the top of the "old" window*/
                                /* at the top of the "new" window.         */

        vfixcurs();             /* position the cursor in the correct spot */


        got_winch = 0;

        if (got_winch_insert)  /* Did we get a SIGWINCH while in insert mode? */
        {
                got_winch_insert = 0;
		TSYNC(); 
                longjmp(vappend_env, 1);  /* go back to our insert environ. */
        }
        else
	{
		ign_winch = 0;
                (void) signal (SIGWINCH, winch);
	}
}

#endif SIGWINCH

