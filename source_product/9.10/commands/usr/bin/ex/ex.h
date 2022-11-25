/* @(#) $Revision: 70.5 $ */      
/* Copyright (c) 1981 Regents of the University of California */

/*
 * Ex version 3 (see exact version in ex_cmds.c, search for /Version/)
 *
 * Mark Horton, U.C. Berkeley
 * Bill Joy, U.C. Berkeley
 * November 1979
 *
 * This file contains most of the declarations common to a large number
 * of routines.  The file ex_vis.h contains declarations
 * which are used only inside the screen editor.
 * The file ex_tune.h contains parameters which can be diddled per installation.
 *
 * The declarations relating to the argument list, regular expressions,
 * the temporary file data structure used by the editor
 * and the data describing terminals are each fairly substantial and
 * are kept in the files ex_{argv,re,temp,tty}.h which
 * we #include separately.
 *
 * If you are going to dig into ex, you should look at the outline of the
 * distribution of the code into files at the beginning of ex.c and ex_v.c.
 * Code which is similar to that of ed is lightly or undocumented in spots
 * (e.g. the regular expression code).  Newer code (e.g. open and visual)
 * is much more carefully documented, and still rough in spots.
 *
 * Please forward bug reports to
 *
 *	Computer Systems Research Group
 *	Computer Science Division, EECS
 *	EVANS HALL
 *	U.C. Berkeley 94720
 *	(415) 642-7780 (project office)
 *
 * or to 4bsd-bugs@Berkeley on the ARPA-net or ucbvax!4bsd-bugs on UUCP.
 * We would particularly like to hear of additional terminal descriptions
 * you add to the terminfo data base.
 */

#ifdef UCBV7
# include <whoami.h>
#endif
#include <sys/types.h>

#ifndef NONLS8	/* Character set features */
# include <nl_ctype.h>
#else NONLS8
# include <ctype.h>
#endif NONLS8

#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <limits.h>

#if defined (hp9000s200) || defined (hp9000s500)
#include "local/port.h"
#endif

#ifdef USG
# include <termio.h>
# ifdef	SIGTSTP
#  include <bsdtty.h>
# endif
typedef struct termio SGTTY;
#else not USG
# include <sgtty.h>
typedef struct sgttyb SGTTY;
#endif not USG

#ifdef PAVEL
#define SGTTY struct sgttyb	/* trick Pavel curses to not include <curses.h> */
#endif
#include <term.h>
#ifdef PAVEL
#undef SGTTY
#endif

#ifndef var
#define var	extern
#endif
/*
 *	The following little dance copes with the new USG tty handling.
 *	This stuff has the advantage of considerable flexibility, and
 *	the disadvantage of being incompatible with anything else.
 *	The presence of the symbol USG will indicate the new code:
 *	in this case, we define CBREAK (because we can simulate it exactly),
 *	but we won't actually use it, so we set it to a value that will
 *	probably blow the compilation if we goof up.
 */
#ifdef USG
#undef CBREAK
#define CBREAK xxxxx
#endif

extern	int errno;

#ifndef VMUNIX
typedef	short	line;
#else
typedef	int	line;
#endif
typedef	short	bool;

#include "ex_tune.h"
#include "ex_vars.h"

/***************
 * In the editor, calls to isxxxx macros (such as isdigit) could be
 * indexing ctype with a negative indice, thus possibly giving a segment
 * violation.  So, we will do a lower bounds checking to make sure
 * the index is greater than IS_MACRO_LOW_BOUND before calling isxxxx macros.
 ***************/

#define IS_MACRO_LOW_BOUND 0

/***************
 * NUMBER_INDENT is the number of columns that text is automatically
 * moved over when :set nu has been activated (i.e., value(NUMBER) = 1).
 ***************/

#define NUMBER_INDENT 8


/*
 * Options in the editor are referred to usually by "value(name)" where
 * name is all uppercase, i.e. "value(PROMPT)".  This is actually a macro
 * which expands to a fixed field in a static structure and so generates
 * very little code.  The offsets for the option names in the structure
 * are generated automagically from the structure initializing them in
 * ex_data.c... see the shell script "makeoptions".
 */
struct	option {
	char	*oname;
	char	*oabbrev;
	short	otype;		/* Types -- see below */
	short	odefault;	/* Default value */
	short	ovalue;		/* Current value */
	char	*osvalue;
};

#define	ONOFF	0
#define	NUMERIC	1
#define	STRING	2		/* SHELL or DIRECTORY */
#define	OTERM	3

#define	value(a)	options[a].ovalue
#define	svalue(a)	options[a].osvalue

extern	 struct	option options[NOPTS + 1];


/*
 * The editor does not normally use the standard i/o library.  Because
 * we expect the editor to be a heavily used program and because it
 * does a substantial amount of input/output processing it is appropriate
 * for it to call low level read/write primitives directly.  In fact,
 * when debugging the editor we use the standard i/o library.  In any
 * case the editor needs a printf which prints through "putchar" ala the
 * old version 6 printf.  Thus we normally steal a copy of the "printf.c"
 * and "strout" code from the standard i/o library and mung it for our
 * purposes to avoid dragging in the stdio library headers, etc if we
 * are not debugging.  Such a modified printf exists in "printf.c" here.
 */
#ifdef TRACE
#	include <stdio.h>
# define BUFSIZ   1024
	var	FILE	*trace;
	var	bool	trubble;
	var	bool	techoin;
	var	char	tracbuf[BUFSIZ];
#	undef	putchar
#	undef	getchar
#else
/*
 * Warning: do not change BUFSIZ without also changing LBSIZE in ex_tune.h
 * Running with BUFSIZ set to anything besides what is in <stdio.h> is
 * not recommended, if you use stdio.
 */
# ifdef u370
#	define	BUFSIZ	4096
# else
/* BUFSIZ is changed from 1024 to LINE_MAX (2048 defined in limits.h) */
#       if !defined(BUFSIZ) || BUFSIZ != LINE_MAX
#	    define	BUFSIZ	LINE_MAX
#       endif
# endif
#	undef	NULL
#	define	NULL	0
#	undef	EOF
#	define	EOF	-1
#endif

/*
 * Character constants and bits
 *
 * The editor uses the QUOTE bit as a flag to pass on with characters
 * e.g. to the putchar routine.  The editor never uses a simple char variable.
 * Only arrays of and pointers to characters are used and parameters and
 * registers are never declared character.
 */

#ifndef NONLS8	/* 8bit integrity */
# define	QUOTE	0x8000
# define	TRIM	0xff
#else NONLS8
# define	QUOTE	0200
# define	TRIM	0177
#endif NONLS8

#define	NL	CTRL(j)
#define	CR	CTRL(m)
#define	DELETE	0177		/* See also ATTN, QUIT in ex_tune.h */
#define	ESCAPE	033
#undef	CTRL
#define	CTRL(c)	('c' & 037)

#ifndef	NLS16
typedef	char	CHAR;		/* CHAR requires 8-bit space */
#else	NLS16
typedef	short	CHAR;		/* CHAR requires 16-bit space */
#endif	NLS16

/*
 * Miscellaneous random variables used in more than one place
 */
var	bool	aiflag;		/* Append/change/insert with autoindent */
var	bool	anymarks;	/* We have used '[a-z] */
var	int	chng;		/* Warn "No write" */
var	char	*Command;
var	short	defwind;	/* -w# change default window size */
var	bool	dir_chg;	/* true if directory change should be done immediately */
var	int	dirtcnt;	/* When >= MAXDIRT, should sync temporary */
#ifdef TIOCLGET
var	bool	dosusp;		/* Do SIGTSTP in visual when ^Z typed */
#endif
var	bool	edited;		/* Current file is [Edited] */
var	line	*endcore;	/* Last available core location */
extern	 bool	endline;	/* Last cmd mode command ended with \n */
#ifndef VMUNIX
var	short	erfile;		/* Error message file unit */
#endif
var	line	*fendcore;	/* First address in line pointer space */
var	char	file[FNSIZE];	/* Working file name */
var	CHAR	genbuf[LBSIZE];	/* Working buffer when manipulating linebuf */
#ifdef	NLS16
var	char	GENBUF[LBSIZE];	/* Working buffer when manipulating LINEBUF */
var	char	vcmd[LBSIZE];	/* Command list from visual mode */
#endif	NLS16
var	bool	hush;		/* Command line option - was given, hush up! */
var	char	*globp;		/* (Untyped) input string to command mode */
var	bool	holdcm;		/* Don't cursor address */
var	bool	inappend;	/* in ex command append mode */
var	bool	inglobal;	/* Inside g//... or v//... */
var	char	*initev;	/* Initial : escape for visual */
var	bool	inopen;		/* Inside open or visual */
var	char	*input;		/* Current position in cmd line input buffer */
var	bool	intty;		/* Input is a tty */
var	short	io;		/* General i/o unit (auto-closed on error!) */
#ifdef SIGWINCH
var	bool	invisual;	/* In vi mode (i.e., not ex mode)  */
var	bool    ign_winch;      /* Indicates SIGWINCH is ignored */
#endif SIGWINCH
extern	short	lastc;		/* Last character ret'd from cmd input */
var	bool	laste;		/* Last command was an "e" (or "rec") */
var	char	lastmac;	/* Last macro called for ** */
var	char	lasttag[TAGSIZE];	/* Last argument to a tag command */
var	char	*linebp;	/* Used in substituting in \n */
var	CHAR	linebuf[LBSIZE];	/* The primary line buffer */
#ifdef	NLS16
var	char	LINEBUF[LBSIZE];	/* The primary line buffer */
#endif	NLS16
var	bool	listf;		/* Command should run in list mode */
var	CHAR	*loc1;		/* Where re began to match (in LINEBUF) */
var	CHAR	*loc2;		/* First char after re match (") */
var	line	names['z'-'a'+2];	/* Mark registers a-z,' */
var	int	notecnt;	/* Count for notify (to visual from cmd) */
var	bool	numberf;	/* Command should run in number mode */
var	char	obuf[BUFSIZ];	/* Buffer for tty output */
var	short	oprompt;	/* Saved during source */
var	bool	oreal_empty;	/* Copy real_empty to fix blank after file rd */
var	short	ospeed;		/* Output speed (from gtty) */
var	int	otchng;		/* Backup tchng to find changes in macros */
var	short	peekc;		/* Peek ahead character (cmd mode input) */
#ifdef	NLS16
var	short	secondchar;	/* Peek ahead Kanji 2nd byte character */
#endif	NLS16
var	CHAR	*pkill[2];	/* Trim for put with ragged (LISP) delete */
var	bool	pfast;		/* Have stty -nl'ed to go faster */
var	int	pid;		/* Process id of child */
var	int	ppid;		/* Process id of parent (e.g. main ex proc) */
var	bool	real_empty;	/* File contains only dummy line for visual */
var	jmp_buf	resetlab;	/* For error throws to top level (cmd mode) */
var	int	rpid;		/* Pid returned from wait() */
var	bool	ruptible;	/* Interruptible is normal state */
var	bool	seenprompt;	/* 1 if have gotten user input */
var	bool	shudclob;	/* Have a prompt to clobber (e.g. on ^D) */
var	int	status;		/* Status returned from wait() */
var	int	tchng;		/* If nonzero, then [Modified] */
extern	short	tfile;		/* Temporary file unit */
var	bool	vcatch;		/* Want to catch an error (open/visual) */
var	jmp_buf	vreslab;	/* For error throws to a visual catch */
#ifdef SIGTSTP
var	bool	was_suspended;	/* used in onsusp() and cleanup() */
#endif SIGTSTP
var	bool	writing;	/* 1 if in middle of a file write */
var	int	xchng;		/* Suppresses multiple "No writes" in !cmd */

/*
 * Macros
 */
#define	CP(a, b)	(ignore(strcpy(a, b)))
			/*
			 * FIXUNDO: do we want to mung undo vars?
			 * Usually yes unless in a macro or global.
			 */
#define FIXUNDO		(inopen >= 0 && (inopen || !inglobal))
#define ckaw()		{if (chng && value(AUTOWRITE) && !value(READONLY)) wop(0);}
#define	copy(a,b,c)	Copy((char *) a, (char *) b, c)
#define	eq(a, b)	((a) && (b) && strcmp(a, b) == 0)
#define	getexit(a)	copy(a, resetlab, sizeof (jmp_buf))
#define	lastchar()	lastc
#define	outchar(c)	(*Outchar)(c)
#define	pastwh()	(ignore(skipwh()))
#define	pline(no)	(*Pline)(no)
#define	reset()		longjmp(resetlab,1)
#define	resexit(a)	copy(resetlab, a, sizeof (jmp_buf))
#define	setexit()	setjmp(resetlab)
#define	setlastchar(c)	lastc = c
#define	ungetchar(c)	peekc = c
#ifdef	NLS16
#define	clrpeekc()	peekc = secondchar = 0
#endif	NLS16

#define	CATCH		if (setjmp(vreslab) == 0 && (vcatch = 1)) {
#define	ONERR		} else { vcatch = 0;
#define	ENDCATCH	} vcatch = 0;

/*
 * Environment like memory
 */
var	char	altfile[FNSIZE];	/* Alternate file name */
extern	char	direct[ONMSZ];		/* Temp file goes here */
extern	char	shell[ONMSZ];		/* Copied to be settable */
var	char	uxb[UXBSIZE + 2];	/* Last !command for !! */
#ifdef	NLS16
var	CHAR	UXB[UXBSIZE + 2];	/* Last !command for !! */
#endif	NLS16

/*
 * The editor data structure for accessing the current file consists
 * of an incore array of pointers into the temporary file tfile.
 * Each pointer is 15 bits (the low bit is used by global) and is
 * padded with zeroes to make an index into the temp file where the
 * actual text of the line is stored.
 *
 * To effect undo, copies of affected lines are saved after the last
 * line considered to be in the buffer, between dol and unddol.
 * During an open or visual, which uses the command mode undo between
 * dol and unddol, a copy of the entire, pre-command buffer state
 * is saved between unddol and truedol.
 */
var	line	*addr1;			/* First addressed line in a command */
var	line	*addr2;			/* Second addressed line */
var	line	*dol;			/* Last line in buffer */
var	line	*dot;			/* Current line */
var	line	*one;			/* First line */
var	line	*truedol;		/* End of all lines, including saves */
var	line	*unddol;		/* End of undo saved lines */
var	line	*zero;			/* Points to empty slot before one */

/*
 * Undo information
 *
 * For most commands we save lines changed by salting them away between
 * dol and unddol before they are changed (i.e. we save the descriptors
 * into the temp file tfile which is never garbage collected).  The
 * lines put here go back after unddel, and to complete the undo
 * we delete the lines [undap1,undap2).
 *
 * Undoing a move is much easier and we treat this as a special case.
 * Similarly undoing a "put" is a special case for although there
 * are lines saved between dol and unddol we don't stick these back
 * into the buffer.
 */
var	short	undkind;

var	line	*unddel;	/* Saved deleted lines go after here */
var	line	*undap1;	/* Beginning of new lines */
var	line	*undap2;	/* New lines end before undap2 */
var	line	*undadot;	/* If we saved all lines, dot reverts here */

#define	UNDCHANGE	0
#define	UNDMOVE		1
#define	UNDALL		2
#define	UNDNONE		3
#define	UNDPUT		4

/* ========================================================================= */
/*
** CRYPT block 1
*/
#ifdef CRYPT
/*
 * Various miscellaneous flags and buffers needed by the encryption routines.
 */
#define	KSIZE   9       /* key size for encryption */
#define	KEYPROMPT       "Key: "
var	int	xflag;		/* True if we are in encryption mode */
var	int	xeflag;		/* True if we should exit on domakekey error */
var	int	xtflag;		/* True if the temp file is being encrypted */
var	int	kflag;		/* True if the key has been accepted */
var	char	perm[768];
var	char	tperm[768];
var	char	*key;
var	char	crbuf[CRSIZE];
char	*getpass();
#endif CRYPT
/* ========================================================================= */

/*
 * Function type definitions
 */
#define	_NOSTR	(char *) 0
#define	NOLINE	(line *) 0
#define	NOCHAR	(CHAR *) 0

extern	int	(*Outchar)();
extern	int	(*Pline)();
extern	int	(*Putchar)();
var	void	(*oldhup)();
int	(*setlist())();
int	(*setnorm())();
int	(*setnorm())();
int	(*setnumb())();
line	*address();
char	*cgoto();
CHAR	*genindent();
char	*getblock();
char	*getenv();
line	*getmark();
char	*mesg();
CHAR	*place();
char	*plural();
line	*scanfor();
line	*setin();
char	*strcat();
char	*strcpy();
char	*strend();
char	*tailpath();
char	*tgetstr();
char	*tgoto();
char	*ttyname();
line	*vback();
CHAR	*vfindcol();
CHAR	*vgetline();
char	*vinit();
CHAR	*vpastwh();
CHAR	*vskipwh();
int	put();
int	putreg();
int	YANKreg();
int	delete();
int	execl();
int	filter();
int	getfile();
int	getsub();
int	gettty();
int	join();
int	listchar();
off_t	lseek();
int	normchar();
int	normline();
int	numbline();
var	void	(*oldquit)();
#ifdef USG
var	void	(*oldchild)();
#endif USG
int	onhup();
int	onintr();
int	onsusp();
int	putch();
int	shift();
int	termchar();
int	vfilter();
#ifdef CBREAK
int	vintr();
#endif
#ifdef TRACE
int	vputch();
#endif
int	vshftop();
int	yank();
#ifdef SIGWINCH
int     winch();
#endif SIGWINCH

/*
 * C doesn't have a (void) cast, so we have to fake it for lint's sake.
 */
#ifdef lint
#	define	ignore(a)	Ignore((char *) (a))
#	define	ignorf(a)	Ignorf((int (*) ()) (a))
#else
#	define	ignore(a)	a
#	define	ignorf(a)	a
#endif

#ifdef	NLS16
CHAR	*STRCAT();
CHAR	*STRCPY();
CHAR	*STREND();
CHAR	*NADVANCE();
CHAR	*NREVERSE();
int	kstrlen();	/* Number of CHARACTERS until the end of string */

#define	FIRST		0x200	/* Kanji first byte */
#define	SECOND		0x100	/* Kanji second byte */
#define	DUMMY_BLANK	0x400	/* pseudo space for terminal right margin */

#define	SECONDof2(c)	(SECof2(c))
#define	IS_FIRST(c)	(((c) & ~(QUOTE | TRIM | DUMMY_BLANK)) == FIRST)
#define	IS_SECOND(c)	(((c) & ~(QUOTE | TRIM | DUMMY_BLANK)) == SECOND)
#define	IS_KANJI(c)	((IS_FIRST(c)) || (IS_SECOND(c)))
#define	IS_ANK(c)	(!(IS_KANJI(c)))

/* P++	post-increment */
#define	PST_INC(p)	(IS_FIRST(*(p)) ? ((p)+=2, (p)-2) : (p)++)
/* ++p	pre-increment */
#define	PRE_INC(p)	(IS_FIRST(*(p)) ? (p)+=2 : ++(p))
/* p--	post-decrement */
#define	PST_DEC(p)	(IS_FIRST(*((p)-2)) ? ((p)-=2, (p)+2) : (p)--)
/* --p 	pre-decrement */
#define	PRE_DEC(p)	(IS_FIRST(*((p)-2)) ? (p)-=2 : --(p))

#undef	ADVANCE
/* p+1 */
#define	ADVANCE(p)	(IS_FIRST(*(p)) ? ((p)+2) : ((p)+1))
/* p-1 */
#define	REVERSE(p)	(IS_FIRST(*((p)-2)) ? ((p)-2) : ((p)-1))
/*  *p  */
#undef _CHARAT(p)
#define	_CHARAT(p)	(IS_FIRST(*(p)) ? (((*(p) & TRIM) << 8) | (*((p)+1) & TRIM)) : (*(p) & TRIM))
#endif	NLS16

#if defined NLS || defined NLS16

/* right-to-left language section */

#include <nl_types.h>

typedef enum {HEBREW,ARABIC,OTHER} LANG;

/* externals for right-to-left languages set by setlocale(3c) */

extern nl_mode 		_nl_mode;		/* non-Latin or Latin mode */
extern nl_order		_nl_order;		/* keyboard or screen order */
extern nl_direct	_nl_direct;		/* l-to-r or r-to-l direction */
extern int		_nl_space_alt;		/* alternative space */

/* global right-to-left language variables */

var	int	alt_space;	/* alternative space */
var	int	alt_uparrow;	/* alternative up arrow ^ */
var	int	alt_tilde;	/* alternative tilde ~ */
var	int	alt_amp;	/* alternative ampersand @ */
var	int	alt_quote;	/* alternative double quote " */
var	int	alt_slash;	/* alternative slash / */
var	int	alt_backslash;	/* alternative backslash \ */
var	int	alt_zero;	/* alternative zero 0 */

var	bool	right_to_left;	/* true if right-to-left lang */
var	bool	opp_jump_insert;/* true if insert-on-off cause the cursor to jump during an opp lang insert */
var	bool	opp_terminate;	/* true if base language escape seq terminates an opp lang insert */

var	bool	opp_insert;	/* true if opp lang insert */
var	bool	opp_fix;	/* true if vtube fixed for opp lang insert */
var	int	opp_back;	/* number of consecutive opp lang backspaces */
var	int	opp_len;	/* length of opp lang string */
var	int	opp_line;	/* absolute line (row) of opp lang insert */
var	int	opp_col;	/* absolute column of opp lang insert */
var	int	opp_inscol;	/* opp lang start column within line */

var	bool	opp_eattab;	/* eat tabslack during opp lang */
var	int	opp_numtab;	/* num spaces during opp lang tab expansion */
var	int	opp_spctab;	/* space column during opp lang tab expansion */

var	nl_mode	rl_mode;	/* right-to-left file mode (latin - nonlatin */
var	nl_order rl_order;	/* right-to-left file order (screen - key) */
var	nl_order rl_curorder;	/* current right-to-left order (screen - key) */
var	LANG	rl_lang;	/* right-to-left language of file */
var	bool	rl_inflip;	/* true if need input flip for temp file */
var	char	rl_endsent[7];	/* right-to-left sentence delimiters */
var	char	rl_insent[8];	/* right-to-left non-delimiters */
var	char	rl_white[4];	/* right-to-left white space */
var	char	rl_parens[13];	/* right-to-left ({[)}] */
var	char	rl_paren_1[3];	/* right-to-left () */
var	char	rl_paren_2[3];	/* right-to-left [] */
var	char	rl_paren_3[3];	/* right-to-left {} */
var	int	*rl_keys;	/* right-to-left key map pointer */
extern	int	arb_keys[];	/* arabic r-to-l key map */
extern	int	heb_keys[];	/* arabic r-to-l key map */

/* right-to-left language escape sequences */

#define key_order	"\033&x1K"		/* set term to keyboard order */
#define screen_order	"\033&x0K"		/* set term to screen order */
#define latin_lang	"\033(8U"		/* set term to latin */
#define hebrew_lang	"\033(8H"		/* set term to hebrew */
#define arabic_lang	"\033(8V"		/* set term to arabic */
#define lock_keys	"\033c"			/* lock keyboard */
#define unlock_keys	"\033b"			/* unlock keyboard */
#define opp_backspace	"\033D"			/* opp lang backspace */
#define l_mode		"\033&a1D"		/* set term to latin mode */
#define n_mode		"\033&a2D"		/* set term to non-latin mode */
#define d_mode		"\033&j1D"		/* disable base mode from keyboard */
#define e_mode		"\033&j0D"		/* enable base mode from keyboard */

/* special mapping for right-to-left characters */

#define RL_KEY(c)	(right_to_left ? (rl_mode==NL_NONLATIN ? (rl_lang!=OTHER ? *(rl_keys+((c)&TRIM)) : (c)) : (c)) : (c))

/* opposite language character macro */

#define OPP_LANG(c)	(rl_mode==NL_LATIN ? (((c)&TRIM)>128) : ((((c)&TRIM)<128)&&(((c)&TRIM)>31)))

/* key-screen ordering macros */

#define RL_OKEY		if (right_to_left) {\
				tputs(key_order, 1, putch);\
				flusho();\
				rl_curorder = NL_KEY;\
			}

#define RL_OSCREEN	if (right_to_left) {\
				tputs(screen_order, 1, putch);\
				flusho();\
				rl_curorder = NL_SCREEN;\
			}

/* hindi digit macro */

#define HINDI(c)	((((c)&TRIM) > 175) && (((c)&TRIM) < 186))

/* seen queue family group macro */

#define SEEN_Q(c)	((((c)&TRIM) >= 211) && (((c)&TRIM) <= 214))

/* baa queue family group macro */

#define BAA_Q(c)	((((c)&TRIM) == 200) || (((c)&TRIM) == 202) || (((c)&TRIM) == 208) || (((c)&TRIM) == 225))

/* tamdeed macro */

#define TAMDEED(c)	((((c)&TRIM) == 224))

/* string routines */

#ifndef	NLS16

#define	STRLEN(s)		strlen(s)
#define	STRNCPY(s1,s2,n)	strncpy(s1,s2,n)
#define	STRCPY(s1,s2)		strcpy(s1,s2)
#define	STRCHR(s,c)		strchr(s,c)
#define FLIP(s)			flip(s)
#define FLIP_LINE(s)		flip_line(s)

#endif

#define _isspace(c)	(isspace(c) || (c) == alt_space)
#else
#define _isspace(c)	isspace(c)

#endif right-to-left
