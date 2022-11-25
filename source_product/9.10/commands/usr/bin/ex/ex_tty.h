/* @(#) $Revision: 70.1 $ */      
/* Copyright (c) 1981 Regents of the University of California */
/*
 * Capabilities from termcap
 *
 * The description of terminals is a difficult business, and we only
 * attempt to summarize the capabilities here;  for a full description
 * see the paper describing termcap.
 *
 * Capabilities from termcap are of three kinds - string valued options,
 * numeric valued options, and boolean options.  The string valued options
 * are the most complicated, since they may include padding information,
 * which we describe now.
 *
 * Intelligent terminals often require padding on intelligent operations
 * at high (and sometimes even low) speed.  This is specified by
 * a number before the string in the capability, and has meaning for the
 * capabilities which have a P at the front of their comment.
 * This normally is a number of milliseconds to pad the operation.
 * In the current system which has no true programmible delays, we
 * do this by sending a sequence of pad characters (normally nulls, but
 * specifiable as "pc").  In some cases, the pad is better computed
 * as some number of milliseconds times the number of affected lines
 * (to bottom of screen usually, except when terminals have insert modes
 * which will shift several lines.)  This is specified as '12*' e.g.
 * before the capability to say 12 milliseconds per affected whatever
 * (currently always line).  Capabilities where this makes sense say P*.
 */

/*
 * From the tty modes...
 */
var	bool	NONL;		/* Terminal can't hack linefeeds doing a CR */
var	bool	UPPERCASE;	/* Ick! */
var	short	OCOLUMNS;	/* Save columns for a hack in open mode */

var	short	outcol;		/* Where the cursor is */
var	short	outline;

var	short	destcol;	/* Where the cursor should be */
var	short	destline;

/*
 * There are several kinds of tty drivers to contend with.  These include:
 * (1)	V6:		no CBREAK, no ioctl.  (Include PWB V1 here).
 *			[NO LONGER SUPPORTED]
 * (2)	V7 research:	has CBREAK, has ioctl, and has the tchars (TIOCSETC)
 *			business to change start, stop, etc. chars.
 * (3)	USG V2:		Basically like V6 but RAW mode is like V7 RAW.
 *			[NO LONGER SUPPORTED]
 * (4)	USG V3:		equivalent to V7 but totally incompatible.
 * (5)  Berkeley 4BSD:	has ltchars in addition to all of V7.
 *
 * The following attempts to decide what we are on, and declare
 * some variables in the appropriate format.  The wierd looking one (ttymode)
 * is the thing we pass to sTTY and family to turn "RAW" mode on or off
 * when we go into or out of visual mode.  In V7/4BSD it's just the flags word
 * to stty.  In USG V3 it's the whole tty structure.
 */
#ifdef	USG			/* USG V3 */
  var	struct	termio tty;	/* Use this one structure to change modes */
  typedef	struct termio ttymode;	/* Mode to contain tty flags */
# ifdef SIGTSTP		/* job control */
   var	struct	ltchars olttyc, nlttyc;	/* More of tchars style stuff */
# endif	SIGTSTP

#else				/* All others */
  var	struct	sgttyb tty;	/* Always stty/gtty using this one structure */
  typedef	int ttymode;	/* Mode to contain tty flags */
# ifdef 	TIOCSETC	/* V7 */
   var	struct	tchars ottyc, nttyc;	/* For V7 character masking */
# endif
# ifdef		TIOCLGET	/* Berkeley 4BSD */
   var	struct	ltchars olttyc, nlttyc;	/* More of tchars style stuff */
# endif

#ifdef ED1000
var int	breakstate;
#endif ED1000

#endif

var	ttymode	normf;		/* Restore tty flags to this (someday) */
var	bool	normtty;	/* Have to restore normal mode from normf */

#ifdef SIGWINCH
char	lines_env_string[15];
char	cols_env_string[15];
#endif SIGWINCH

ttymode ostart(), setty(), unixex();

var	short	costCM;	/* # chars to output a typical cursor_address, with padding etc. */
var	short	costSR;	/* likewise for scroll reverse */
var	short	costAL;	/* likewise for insert line */
var	short	costDP;	/* likewise for parm_down_cursor */
var	short	costLP;	/* likewise for parm_left_cursor */
var	short	costRP;	/* likewise for parm_right_cursor */
var	short	costCE;	/* likewise for clear to end of line */
var	short	costCD;	/* likewise for clear to end of display */

#ifdef VMUNIX
# define MAXNOMACS	128	/* max number of macros of each kind */
# define MAXCHARMACS	2048	/* max # of chars total in macros */
#else
# define MAXNOMACS	32	/* max number of macros of each kind */
# define MAXCHARMACS	512	/* max # of chars total in macros */
#endif
struct maps {
	char *cap;	/* pressing button that sends this.. */
	char *mapto;	/* .. maps to this string */
	char *descr;	/* legible description of key */
};
var	struct maps arrows[MAXNOMACS];	/* macro defs - 1st 5 built in */
var	struct maps immacs[MAXNOMACS];	/* for while in insert mode */
var	struct maps abbrevs[MAXNOMACS];	/* for word abbreviations */
var	int	arrows_start;	/* start of non-keyboard cmd mode mappings */
var	int	immacs_start;	/* start of non-keyboard imput mode mappings */
var	int	ldisc;			/* line discipline for ucb tty driver */
var	char	mapspace[MAXCHARMACS];
var	char	*msnext;	/* next free location in mapspace */
var	int	maphopcnt;	/* check for infinite mapping loops */
var	bool	anyabbrs;	/* true if abbr or unabbr has been done */
var	char	ttynbuf[20];	/* result of ttyname() */
var	int	ttymesg;	/* original mode of users tty */
var	char	allocspace[256];	/* space for terminfo mappings */
var	char	doublespace[256];	/* space for terminfo mappings if DOUBLEESCAPE selected */
