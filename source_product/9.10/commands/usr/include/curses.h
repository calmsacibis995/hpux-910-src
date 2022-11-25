/* @(#) $Revision: 70.5 $ */
#ifndef _CURSES_INCLUDED /* allow multiple inclusions */
#define _CURSES_INCLUDED

#include <signal.h>

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINDOW

# ifndef 	NONSTANDARD
#  include  <stdio.h>
  /*
   * This trick is used to distinguish between USG and V7 systems.
   * We assume that L_ctermid is only defined in stdio.h in USG
   * systems, but not in V7 or Berkeley UNIX.
   */
#  ifdef L_ctermid
#  define USG
#  endif
#  include  <unctrl.h>
#  ifdef USG
   /*
    * This is a duplicate of the termio structure found in <termio.h>.
    * This header file no longer includes <termio.h> in order to allow
    * curses users to include either <termio.h> or <termios.h> in their
    * code without having any problems.
    */
     typedef struct {
         unsigned short c_iflag;	/* input modes		*/
         unsigned short c_oflag;	/* output modes		*/
         unsigned short c_cflag;	/* control modes	*/
         unsigned short c_lflag;	/* line discipline modes*/
         char 		c_line;		/* line discipline	*/
	 unsigned char	c_cc[8];	/* control chars	*/
     } SGTTY;
#  else
#   include <sgtty.h>
   typedef struct sgttyb SGTTY;
#  endif
# else		/* NONSTANDARD */
/*
 * NONSTANDARD is intended for a standalone program (no UNIX)
 * that manages screens.  The specific program is Alan Hewett's
 * ITC, which runs standalone on an 11/23 (at least for now).
 * It is unclear whether this code needs to be supported anymore.
 */
# define NULL 0
# endif		/* NONSTANDARD */

# define	bool	char
# define	reg	register

/*
 * chtype is the type used to store a character together with attributes.
 * It can be set to "char" to save space, or "long" to get more attributes.
 */
# ifdef	CHTYPE
	typedef	CHTYPE chtype;
# else
	typedef unsigned int chtype;
# endif /* CHTYPE */

# define	TRUE	(1)
# define	FALSE	(0)
# define	ERR	(-1)
# define	OK	(0)

# define	_SUBWIN		01
# define	_ENDLINE	02
# define	_FULLWIN	04
# define	_SCROLLWIN	010
# define	_FLUSH		020
# define	_ISPAD		040
# define	_STANDOUT	0200
# define	_HASSUBWIN	0400

# define	_NOCHANGE	-1

struct _win_st {
	short	_cury, _curx;
	short	_maxy, _maxx;
	short	_begy, _begx;
	short	_flags;
	chtype	_attrs;
	bool	_clear;
	bool	_leave;
	bool	_scroll;
	bool	_use_idl;
	bool	_use_keypad;	/* 0=no, 1=yes, 2=yes/timeout */
	bool	_use_meta;	/* T=use the meta key */
	bool	_nodelay;	/* T=don't wait for tty input */
	chtype	**_y;
	short	*_firstch;
	short	*_lastch;
	short	_tmarg,_bmarg;
#ifdef  SIGWINCH
        short   _allocy, _allocx;  /* size allocated for the window */
#endif
};

extern int	LINES, COLS;

typedef struct _win_st	WINDOW;
extern WINDOW	*stdscr, *curscr;

extern char	*Def_term, ttytype[];

typedef struct screen	SCREEN;
typedef struct term;

#if defined(__STDC__) || defined(__cplusplus)
  extern int attroff(int);
  extern int attron(int);
  extern int attrset(int);
  extern int baudrate(void);
  extern int beep(void);
  extern int box (WINDOW *, chtype, chtype);
  extern int cbreak(void);
  extern int clearok(WINDOW *, int);
  extern int clrtobot(void);
  extern int clrtoeol(void);
  extern int def_prog_mode(void);
  extern int def_shell_mode(void);
  extern int del_curterm(struct term *);
  extern int delay_output(int);
  extern int delwin(WINDOW *);
  extern int doupdate(void);
  extern int draino(int);
  extern int echo(void);
  extern int endwin(void);
  extern char erasechar(void);
  extern int fixterm(void);
  extern int flash(void);
  extern int flushinp(void);
  extern int gettmode(void);
  extern int has_ic(void);
  extern int has_il(void);
  extern int idlok(WINDOW *, int);
  extern WINDOW * initscr(void);
  extern int insch(chtype);
  extern int intrflush(WINDOW *, int);
  extern int keypad(WINDOW *, int);
  extern char killchar(void);
  extern int leaveok(WINDOW *, int);
  extern char * longname(void);
  extern int meta(WINDOW *, int);
  extern int mvcur(int, int, int, int);
  extern int mvprintw(int, int, const char *, ...);
  extern int mvscanw(int, int, const char *, ...);
  extern int mvwin(WINDOW *, int, int);
  extern int mvwprintw(WINDOW *, int, int, const char *, ...);
  extern int mvwscanw(WINDOW *, int, int, const char *, ...);
  extern WINDOW * newpad(int, int);
  extern SCREEN * newterm(const char *, FILE *, FILE *);
  extern WINDOW * newwin(int, int, int, int);
  extern int nl(void);
  extern int nocbreak(void);
  extern int nodelay(WINDOW *, int);
  extern int noecho(void);
  extern int nonl(void);
  extern int noraw(void);
  extern int m_addch(chtype);
  extern int m_addstr(const char *);
  extern int m_clear(void);
  extern int m_erase(void);
  extern WINDOW * m_initscr(void);
  extern int m_move(int, int);
  extern SCREEN * m_newterm(const char *, FILE *, FILE *);
  extern int m_refresh(void);
  extern int overlay(WINDOW *, WINDOW *);
  extern int overwrite(WINDOW *, WINDOW *);
  extern int pnoutrefresh(WINDOW *, int, int, int, int, int, int);
  extern int prefresh(WINDOW *, int, int, int, int, int, int);
  extern int printw(const char *, ...);
  extern int raw(void);
  extern int resetterm(void);
  extern int resetty(void);
  extern int reset_prog_mode(void);
  extern int reset_shell_mode(void);
  extern int saveterm(void);
  extern int savetty(void);
  extern int scanw(const char *, ...);
  extern int scroll(WINDOW *);
  extern int scrollok(WINDOW *, char);
  extern int set_curterm(struct term *);
  extern SCREEN * set_term(SCREEN *);
  extern int setterm(const char *);
  extern int setupterm(const char *, int, int *);
  extern WINDOW * subwin(WINDOW *, int, int, int, int);
  extern int touchwin(WINDOW *);
  extern int traceoff(void);
  extern int traceon(void);
  extern int typeahead(int);
  extern int waddch(WINDOW *, chtype);
  extern int waddstr(WINDOW *, const char *);
  extern int wattroff(WINDOW *, int);
  extern int wattron(WINDOW *, int);
  extern int wattrset(WINDOW *, int);
  extern int wclear(WINDOW *);
  extern int wclrtobot(WINDOW *);
  extern int wclrtoeol(WINDOW *);
  extern int wdelch(WINDOW *);
  extern int wdeleteln(WINDOW *);
  extern int werase(WINDOW *);
  extern int wgetch(WINDOW *);        /* because it can return KEY_*, for 
					 instance */
  extern int wgetstr(WINDOW *, char *);
  extern int winsch(WINDOW *, chtype);
  extern int winsertln(WINDOW *);
  extern int wmove(WINDOW *, int, int);
  extern int wnoutrefresh(WINDOW *);
  extern int wprintw(WINDOW *, const char *, ...);
  extern int wrefresh(WINDOW *);
  extern int wscanw(WINDOW *, const char *, ...);
  extern int wstandend(WINDOW *);
  extern int wstandout(WINDOW *);
  extern char *tparm(const char *, ...);
  extern int tputs(const char *, int, int(*)(char));
  extern int putp(const char *);
  extern int vidputs(int, int(*)(char));
  extern int vidattr(int);
  extern int tgetent(const char *, const char *);
  extern int tgetflag(const char *);
  extern int tgetnum(const char *);
  extern char * tgetstr(const char *, char **);
  extern char * tgoto(const char *, int, int);
#else /* not __STDC__ || __cplusplus */
   WINDOW * initscr();
   WINDOW * newwin();
   WINDOW * subwin();
   WINDOW * newpad();
   char	*longname();
   char	erasechar();
   char killchar();
   int wgetch();	/* because it can return KEY_*, for instance. */
   SCREEN * newterm();
#endif /* __STDC__ || __cplusplus */

# ifndef NOMACROS
#  ifndef MINICURSES
/*
 * psuedo functions for standard screen
 */
# define	addch(ch)	waddch(stdscr, ch)
# define	getch()		wgetch(stdscr)
# define	addstr(str)	waddstr(stdscr, str)
# define	getstr(str)	wgetstr(stdscr, str)
# define	move(y, x)	wmove(stdscr, y, x)
# define	clear()		wclear(stdscr)
# define	erase()		werase(stdscr)
# define	clrtobot()	wclrtobot(stdscr)
# define	clrtoeol()	wclrtoeol(stdscr)
# define	insertln()	winsertln(stdscr)
# define	deleteln()	wdeleteln(stdscr)
# define	refresh()	wrefresh(stdscr)
# define	inch()		winch(stdscr)
# define	insch(c)	winsch(stdscr,c)
# define	delch()		wdelch(stdscr)
# define	standout()	wstandout(stdscr)
# define	standend()	wstandend(stdscr)
# define	attron(at)	wattron(stdscr,at)
# define	attroff(at)	wattroff(stdscr,at)
# define	attrset(at)	wattrset(stdscr,at)

# define	setscrreg(t,b)	wsetscrreg(stdscr, t, b)
# define	wsetscrreg(win,t,b)	(win->_tmarg=(t),win->_bmarg=(b))

/*
 * mv functions
 */
#define	mvwaddch(win,y,x,ch)	(wmove(win,y,x)==ERR?ERR:waddch(win,ch))
#define	mvwgetch(win,y,x)	(wmove(win,y,x)==ERR?ERR:wgetch(win))
#define	mvwaddstr(win,y,x,str)	(wmove(win,y,x)==ERR?ERR:waddstr(win,str))
#define	mvwgetstr(win,y,x,str)	(wmove(win,y,x)==ERR?ERR:wgetstr(win,str))
#define	mvwinch(win,y,x)	(wmove(win,y,x)==ERR?ERR:winch(win))
#define	mvwdelch(win,y,x)	(wmove(win,y,x)==ERR?ERR:wdelch(win))
#define	mvwinsch(win,y,x,c)	(wmove(win,y,x)==ERR?ERR:winsch(win,c))
#define	mvaddch(y,x,ch)		mvwaddch(stdscr,y,x,ch)
#define	mvgetch(y,x)		mvwgetch(stdscr,y,x)
#define	mvaddstr(y,x,str)	mvwaddstr(stdscr,y,x,str)
#define	mvgetstr(y,x,str)	mvwgetstr(stdscr,y,x,str)
#define	mvinch(y,x)		mvwinch(stdscr,y,x)
#define	mvdelch(y,x)		mvwdelch(stdscr,y,x)
#define	mvinsch(y,x,c)		mvwinsch(stdscr,y,x,c)

#define	winch(win)	 (win->_y[win->_cury][win->_curx])

#  else /* MINICURSES */

# define	addch(ch)		m_addch(ch)
# define	addstr(str)		m_addstr(str)
# define	move(y, x)		m_move(y, x)
# define	clear()			m_clear()
# define	erase()			m_erase()
# define	refresh()		m_refresh()
# define	getch()			m_getch()
# define	standout()		wstandout(stdscr)
# define	standend()		wstandend(stdscr)
# define	attron(at)		wattron(stdscr,at)
# define	attroff(at)		wattroff(stdscr,at)
# define	attrset(at)		wattrset(stdscr,at)
# define	mvaddch(y,x,ch)		move(y, x), addch(ch)
# define	mvaddstr(y,x,str)	move(y, x), addstr(str)
# define	initscr			m_initscr
# define	newterm			m_newterm

/*
 * These functions don't exist in minicurses, so we define them
 * to nonexistent functions to help the user catch the error.
 */
#define	getstr		m_getstr
#define	clrtobot	m_clrtobot
#define	clrtoeol	m_clrtoeol
#define	insertln	m_insertln
#define	deleteln	m_deleteln
#define	insch		m_insch
#define	delch		m_delch
#define	inch		m_inch
/* mv functions that aren't valid */
#define	mvwaddch	m_mvwaddch
#define	mvwgetch	m_mvwgetch
#define	mvwaddstr	m_mvaddstr
#define	mvwgetstr	m_mvwgetstr
#define	mvwinch		m_mvwinch
#define	mvwdelch	m_mvwdelch
#define	mvwinsch	m_mvwinsch
#define	mvgetch		m_mvwgetch
#define	mvgetstr	m_mvwgetstr
#define	mvinch		m_mvwinch
#define	mvdelch		m_mvwdelch
#define	mvinsch		m_mvwinsch
/* Real functions that aren't valid */
#define box		m_box
#define delwin		m_delwin
#define longname	m_longname
#define makenew		m_makenew
#define mvprintw	m_mvprintw
#define mvscanw		m_mvscanw
#define mvwin		m_mvwin
#define mvwprintw	m_mvwprintw
#define mvwscanw	m_mvwscanw
#define newwin		m_newwin
#define _outchar		m_outchar
#define overlay		m_overlay
#define overwrite	m_overwrite
#define printw		m_printw
#define putp		m_putp
#define scanw		m_scanw
#define scroll		m_scroll
#define subwin		m_subwin
#define touchwin	m_touchwin
#define _tscroll		m_tscroll
#define _tstp		m_tstp
#define vidattr		m_vidattr
#define waddch		m_waddch
#define waddstr		m_waddstr
#define wclear		m_wclear
#define wclrtobot	m_wclrtobot
#define wclrtoeol	m_wclrtoeol
#define wdelch		m_wdelch
#define wdeleteln	m_wdeleteln
#define werase		m_werase
#define wgetch		m_wgetch
#define wgetstr		m_wgetstr
#define winsch		m_winsch
#define winsertln	m_winsertln
#define wmove		m_wmove
#define wprintw		m_wprintw
#define wrefresh	m_wrefresh
#define wscanw		m_wscanw
#define setscrreg	m_setscrreg
#define wsetscrreg	m_wsetscrreg

#  endif /* MINICURSES */

/*
 * psuedo functions
 */

#define	getyx(win,y,x)	 y = win->_cury, x = win->_curx

/* NLS attributes */
#define A_FIRSTOF2	000000040000
#define A_SECOF2	000000100000

#define A_NLSATTR	000000177400

/* Various video attributes */
#define A_STANDOUT	000040000000
#define A_UNDERLINE	000100000000
#define A_REVERSE	000200000000
#define A_BLINK		000400000000
#define A_DIM		001000000000
#define A_BOLD		002000000000

/* The next three are subject to change (perhaps to colors) so don't depend on them */
#define A_INVIS		004000000000
#define A_PROTECT	010000000000
#define A_ALTCHARSET	020000000000

/* A_INVIS is documented as A_BLANK in curses(3x), System V PRM */
#define A_BLANK		A_INVIS

#define A_NORMAL	000000000000
#define A_ATTRIBUTES	037740000000
#define A_CHARTEXT	000000000377

/* Funny "characters" enabled for various special function keys for input */
#define KEY_BREAK	0401		/* break key (unreliable) */
#define KEY_DOWN	0402		/* The four arrow keys ... */
#define KEY_UP		0403
#define KEY_LEFT	0404
#define KEY_RIGHT	0405		/* ... */
#define KEY_HOME	0406		/* Home key (upward+left arrow) */
#define KEY_BACKSPACE	0407		/* backspace (unreliable) */
#define KEY_F0		0410		/* Function keys.  Space for 64 */
#define KEY_F(n)	(KEY_F0+(n))	/* keys is reserved. */
#define KEY_DL		0510		/* Delete line */
#define KEY_IL		0511		/* Insert line */
#define KEY_DC		0512		/* Delete character */
#define KEY_IC		0513		/* Insert char or enter insert mode */
#define KEY_EIC		0514		/* Exit insert char mode */
#define KEY_CLEAR	0515		/* Clear screen */
#define KEY_EOS		0516		/* Clear to end of screen */
#define KEY_EOL		0517		/* Clear to end of line */
#define KEY_SF		0520		/* Scroll 1 line forward */
#define KEY_SR		0521		/* Scroll 1 line backwards (reverse) */
#define KEY_NPAGE	0522		/* Next page */
#define KEY_PPAGE	0523		/* Previous page */
#define KEY_STAB	0524		/* Set tab */
#define KEY_CTAB	0525		/* Clear tab */
#define KEY_CATAB	0526		/* Clear all tabs */
#define KEY_ENTER	0527		/* Enter or send (unreliable) */
#define KEY_SRESET	0530		/* soft (partial) reset (unreliable) */
#define KEY_RESET	0531		/* reset or hard reset (unreliable) */
#define KEY_PRINT	0532		/* print or copy */
#define KEY_LL		0533		/* home down or bottom (lower left) */
					/* The keypad is arranged like this: */
					/*    a1    up    a3   */
					/*   left   b2  right  */
					/*    c1   down   c3   */
#define KEY_A1		0534		/* upper left of keypad */
#define KEY_A3		0535		/* upper right of keypad */
#define KEY_B2		0536		/* center of keypad */
#define KEY_C1		0537		/* lower left of keypad */
#define KEY_C3		0540		/* lower right of keypad */

#define KEY_BTAB	0541		/* back tab key*/

# endif /* NOMACROS */
#endif /* WINDOW */

#ifdef __cplusplus
}
#endif

#endif /* _CURSES_INCLUDED */
