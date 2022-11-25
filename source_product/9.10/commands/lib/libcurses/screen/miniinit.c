/* @(#) $Revision: 66.2 $ */     

# include	"curses.ext"
# include	<signal.h>
#include	<nl_ctype.h>

char	*calloc();
extern	char	*getenv();

static WINDOW	*makenew();

struct screen *m_newterm();

/*
 *	This routine initializes the current and standard screen.
 *
 */

WINDOW *
m_initscr() {
	reg char	*sp;
	struct screen	*scp;
	
#ifdef DEBUG
	if (outf == NULL) {
		outf = fopen("trace", "w");
		if (outf == NULL) {
			perror("trace");
			exit(-1);
		}
	}
#endif

	if (isatty(2)) {
		if ((sp = getenv("TERM")) == NULL)
			sp = Def_term;
#ifdef DEBUG
		if(outf) fprintf(outf, "INITSCR: term = %s\n", sp);
#endif
	}
	else {
		sp = Def_term;
	}
	if((scp=m_newterm(sp, stdout, stdin))==NULL) {
		exit(1);
	}

	nl_init(getenv("LANG"));

	return stdscr;
}

struct screen *
m_newterm(type, outfd, infd)
char *type;
FILE *outfd, *infd;
{
#ifdef	SIGTSTP
	int		m_tstp();
# ifdef	hpux
	struct sigvec vec;
# endif	hpux
#endif	SIGTSTP
	struct screen *scp;
	struct screen *_new_tty();
	extern int _endwin;

#ifdef DEBUG
	if(outf) fprintf(outf, "NEWTERM() isatty(2) %d, getenv %s\n",
		isatty(2), getenv("TERM"));
#endif
	SP = (struct screen *) calloc(1, sizeof (struct screen));
	if (SP == (struct screen *)0)
		return (struct screen *)0;
	SP->term_file = outfd;
	SP->input_file = infd;
	SP->fl_echoit = 1;
	savetty();
	scp = _new_tty(type, outfd);
	if (scp == NULL)
		return (struct screen *)NULL;
#ifdef USG
	(cur_term->Nttyb).c_lflag &= ~ECHO;
	(cur_term->Nttyb).c_oflag &= ~OCRNL;
	reset_prog_mode();
#endif USG
#ifdef SIGTSTP
# ifndef hpux
	(void) signal(SIGTSTP, m_tstp);
# else   hpux
	vec.sv_handler = m_tstp;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvector(SIGTSTP, &vec, &vec);
	if (vec.sv_handler == SIG_IGN)
	(void) sigvector(SIGTSTP, &vec, (struct sigvec *)0);
# endif   hpux
#endif

	LINES =	lines;
	COLS =	columns;
#ifdef DEBUG
	if(outf) fprintf(outf, "LINES = %d, COLS = %d\n", LINES, COLS);
#endif
	curscr = makenew(LINES, COLS, 0, 0);
	stdscr = makenew(LINES, COLS, 0, 0);
#ifdef DEBUG
	if(outf) fprintf(outf, "SP %x, stdscr %x, curscr %x\n", SP, stdscr, curscr);
#endif
	SP->std_scr = stdscr;
	SP->cur_scr = curscr;
	_endwin = FALSE;
	return scp;
}

/*
 *	This routine sets up a _window buffer and returns a pointer to it.
 */

static WINDOW *
makenew(num_lines, num_cols, begy, begx)
int	num_lines, num_cols, begy, begx;
{
	reg WINDOW	*win;
	char *calloc();

	m_showctrl();

#ifdef	DEBUG
	if(outf) fprintf(outf, "MAKENEW(%d, %d, %d, %d)\n", num_lines, num_cols, begy, begx);
#endif
	if ((win = (WINDOW *) calloc(1, sizeof (WINDOW))) == NULL)
		return (WINDOW *)NULL;
#ifdef DEBUG
	if(outf) fprintf(outf, "MAKENEW: num_lines = %d\n", num_lines);
#endif

	win->_cury = win->_curx = 0;
	win->_maxy = num_lines;
	win->_maxx = num_cols;
	win->_begy = begy;
	win->_begx = begx;
	win->_scroll = win->_leave = win->_use_idl = FALSE;
	return win;
}
