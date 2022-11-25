/* @(#) $Revision: 66.3 $ */

/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/* Adapted for ksh by David Korn */
/*+	VI.C			P.D. Sullivan
 *
 *	One line editor for the shell based on the vi editor.
 *
 *	Questions to:
 *		P.D. Sullivan
 *		cbosgd!pds
-*/


#include	<errno.h>

#   include	"defs.h"

#include	"history.h"
#include	"edit.h"
#include	"terminal.h"

#ifndef FIORDCHK
#   define NTICKS	5		/* number of ticks for typeahead */
#endif /* FIORDCHK */

#define	MAXCHAR	MAXLINE-2		/* max char per line */
#define	WINDOW	MAXWINDOW		/* max char in window of which */
					/* WINDOW-2 are available to user */
					/* actual window size may be smaller */



#undef isblank
#   define gencpy(a,b)	strcpy((char*)(a),(char*)(b))
#   define genncpy(a,b,n) strncpy((char*)(a),(char*)(b),n)
#   define genlen(str)	strlen(str)
#   define isalph(v)	isalnum(virtual[v])
#   define isblank(v)	isspace(virtual[v])
#   define ismetach(v)	ismeta(virtual[v])
#   define digit(c)	isdigit(c)
#   define is_print(c)	isprint(c)
#define fold(c)		((c)&~040)	/* lower and uppercase equivalent */

#undef putchar
#undef getchar
#define getchar(fd)	ed_getchar(fd)
#define putchar(fd,c)	ed_putchar(fd,c)
#define bell(fd)	ed_ringbell(fd)	/* ring terminal's bell */
#define crlf(fd)	ed_crlf(fd)	/* return and linefeed */

#define addnl		editb.e_addnl		/* boolean - add newline flag */
#define crallowed	editb.e_crlf
#define cur_virt	editb.e_cur		/* current virtual column */
#define cur_phys	editb.e_pcur	/* current phys column cursor is at */
#define curhline	editb.e_hline		/* current history line */
#define env		editb.e_env
#define fildes		editb.e_fd
#define findchar	editb.e_fchar		/* last find char */
#define first_virt	editb.e_fcol		/* first allowable column */
#define first_wind	editb.e_globals[0]	/* first column of window */
#define	globals		editb.e_globals		/* local global variables */
#define histmin		editb.e_hismin
#define histmax		editb.e_hismax
#define last_phys	editb.e_peol		/* last column in physical */
#define last_virt	editb.e_eol		/* last column */
#define last_wind	editb.e_globals[1]	/* last column in window */
#define	lastmotion	editb.e_globals[2]	/* last motion */
#define lastrepeat	editb.e_mode	/* last repeat count for motion cmds */
#define	long_char	editb.e_globals[3]	/* line bigger than window */
#define	long_line	editb.e_globals[4]	/* line bigger than window */
#define lsearch		editb.e_search		/* last search string */
#define lookahead	editb.e_index		/* characters in buffer */
#define previous	editb.e_lbuf		/* lookahead buffer */
#define max_col		editb.e_llimit		/* maximum column */
#define	ocur_phys	editb.e_globals[5]   /* old current physical position */
#define	ocur_virt	editb.e_globals[6]	/* old last virtual position */
#define	ofirst_wind	editb.e_globals[7]	/* old window first col */
#define	o_v_char	editb.e_globals[8]	/* prev virtual[ocur_virt] */
#define Prompt		editb.e_prompt		/* pointer to prompt */
#define plen		editb.e_plen		/* length of prompt */
#define physical	editb.e_physbuf		/* physical image */
#define repeat		editb.e_repeat	    /* repeat count for motion cmds */
#define ttyspeed	editb.e_ttyspeed	/* tty speed */
#define u_column	editb.e_ucol		/* undo current column */
#define U_saved		editb.e_saved		/* original virtual saved */
#define U_space		editb.e_Ubuf		/* used for U command */
#define u_space		editb.e_ubuf		/* used for u command */
#define usreof		editb.e_eof		/* user defined eof char */
#define usrerase	editb.e_erase		/* user defined erase char */
#define usrintr		editb.e_intr		/* user defined intr char */
#define usrkill		editb.e_kill		/* user defined kill char */
#define usrquit		editb.e_quit		/* user defined quit char */
#define virtual		editb.e_inbuf	/* pointer to virtual image buffer */
#define	window		editb.e_window		/* window buffer */
#define	w_size		editb.e_wsize		/* window size */
#define	inmacro		editb.e_inmacro		/* true when in macro */
#define yankbuf		editb.e_killbuf		/* yank/delete buffer */

extern clock_t times();
extern void ed_flush();

#define	ABORT	-2			/* user abort */
#define	APPEND	-10			/* append chars */
#define	BAD	-1			/* failure flag */
#define	BIGVI	-15			/* user wants real vi */
#define	CONTROL	-20			/* control mode */
#define	ENTER	-25			/* enter flag */
#define	GOOD	0			/* success flag */
#define	INPUT	-30			/* input mode */
#define	INSERT	-35			/* insert mode */
#define	REPLACE	-40			/* replace chars */
#define	SEARCH	-45			/* search flag */
#define	TRANSLATE	-50		/* translate virt to phys only */

#define	DEL	'\177'			/* interrupt char */

#define	TRUE	1
#define	FALSE	0

#define	INVALID	(-1)			/* invalid column */
#define	QUIT_C	'\34'			/* quit char */
#define	SYSERR	(-1)			/* system error */

static char last_cmd = '\0';		/* last command */
static char repeat_set;
static char nonewline;
static genchar *lastline;

#ifdef FIORDCHK
    static clock_t typeahead;		/* typeahead occurred */
#else
    static int typeahead;		/* typeahead occurred */
#endif	/* FIORDCHK */

static int	del_line();
static int	getcount();
static int	getline();
static int	mvcursor();
static void	pr_prompt();
static void	pr_string();
static int	putstring();
static int	refresh();
static int	replace();
static int	restore_v();
static int	save_last();
static int	save_v();
static int	search();
static int	sync_cursor();
static int	textmod();

/*+	VI_READ( fd, shbuf, nchar )
 *
 *	This routine implements a one line version of vi and is
 * called by _filbuf.c
 *
-*/

vi_read(fd, shbuf, nchar)
int fd;					/* input file descriptor */
register char *shbuf;			/* shell line buffer */
unsigned nchar;				/* number of chars to read */
{
	register int c;			/* general variable */
	register int i;			/* general variable */
	register int term_char;	/* read() termination character */
	char prompt[PRSIZE+2];		/* prompt */
	char Lsearch[MAXLINE];	/* last search string */
	genchar Physical[2*MAXLINE];	/* physical image */
	genchar Ubuf[MAXLINE];	/* used for U command */
	genchar ubuf[MAXLINE];	/* used for u command */
	genchar Window[WINDOW+10];	/* window image */
#ifndef ECHOCTL
	char cntl_char;			/* TRUE if control character present */
#endif	/* !ECHOCTL */
	int Globals[9];			/* local global variables */
#ifndef FIORDCHK
	clock_t oldtime, newtime;
	struct tms dummy;
#endif	/* FIORDCHK */
	
	/*** setup prompt ***/

	Prompt = prompt;
	ed_setup(fd);

	if( !is_option(VIRAW) )
	{
		/*** Change the eol characters to '\r' and eof  ***/
		/* in addition to '\n' and make eof an ESC	*/

		if( tty_alt(fd) == BAD )
		{
#ifdef NOTDEF
			return(ee_read(fd, shbuf, nchar));
#endif /* NOTDEF */
		}

#   ifdef FIORDCHK
		ioctl(fd,FIORDCHK,&typeahead);
#   else
		/* time the current line to determine typeahead */
		oldtime = times(&dummy);
#   endif /* FIORDCHK */
		/* abort of interrupt has occurred */
		if(sh.trapnote&SIGSET)
			i = -1;
		else
		/*** Read the line ***/
		i = ee_read(fd, shbuf, nchar);
#   ifndef FIORDCHK
		newtime = times(&dummy);
		typeahead = ((newtime-oldtime) < NTICKS);
#   endif /* FIORDCHK */
#   ifdef ECHOCTL
		if( i <= 0 )
		{
			/*** read error or eof typed ***/
			tty_cooked(fd);
			return(i);
		}
		term_char = shbuf[--i];
		if( term_char == '\r' )
			term_char = '\n';
		if( term_char=='\n' || term_char==ESC )
			shbuf[i--] = '\0';
		else
			shbuf[i+1] = '\0';
#   else
		c = shbuf[0];

		/*** Save and remove the last character if its an eol, ***/
		/* changing '\r' to '\n' */

		if( i == 0 )
		{
			/*** ESC was typed as first char of line ***/
			term_char = ESC;
			shbuf[i--] = '\0';	/* null terminate line */
		}
		else if( i<0 || c==usreof )
		{
			/*** read error or eof typed ***/
			tty_cooked(fd);
			if( c == usreof )
				i = 0;
			return(i);
		}
		else
		{
			term_char = shbuf[--i];
			if( term_char == '\r' )
				term_char = '\n';
			if( term_char=='\n' || term_char==usreof )
			{
				/*** remove terminator & null terminate ***/
				shbuf[i--] = '\0';
			}
			else
			{
				/** terminator was ESC, which is not xmitted **/
				term_char = ESC;
				shbuf[i+1] = '\0';
			}
		}
#   endif /* ECHOCTL */
	}
	else
	{
		/*** Set raw mode ***/

		if( ttyspeed == 0 )
		{
			/*** never did TCGETA, so do it ***/
			/* avoids problem if user does 'sh -o viraw' */
			tty_alt(fd);
		}
		if( tty_raw(fd) == BAD )
		{
#ifdef NOTDEF
			return(ee_read(fd, shbuf, nchar));
#endif /* NOTDEF */
		}
		i = INVALID;
	}

	/*** Initialize some things ***/

	virtual = (genchar*)shbuf;
#undef virtual
#define virtual		((genchar*)shbuf)
	globals = Globals;
	cur_phys = i + 1;
	cur_virt = i;
	fildes = fd;
	first_virt = 0;
	first_wind = 0;
	last_virt = i;
	last_phys = i;
	last_wind = i;
	lsearch = Lsearch;
	lsearch[0] = 0;
	long_line = ' ';
	long_char = ' ';
	o_v_char = '\0';
	ocur_phys = 0;
	ocur_virt = MAXCHAR;
	ofirst_wind = 0;
	physical = Physical;
	u_column = INVALID - 1;
	U_space = Ubuf;
	u_space = ubuf;
	window = Window;
	window[0] = '\0';

#if (2 * CHARSIZE * MAXLINE) < IOBSIZE
	yankbuf = shbuf + MAXLINE*sizeof(genchar);
#else
	if(yankbuf==0)
		yankbuf = (genchar*)malloc(sizeof(genchar)*(MAXLINE));
#endif
#if (3 * CHARSIZE * MAXLINE) < IOBSIZE
	lastline = shbuf + (MAXLINE+MAXLINE)*sizeof(genchar);
#else
	if(lastline==0)
		lastline = (genchar*)malloc(sizeof(genchar)*(MAXLINE));
#endif
	if( last_cmd == '\0' )
	{
		/*** first time for this shell ***/

		last_cmd = 'i';
		findchar = INVALID;
		lastmotion = '\0';
		lastrepeat = 1;
		repeat = 1;
		*yankbuf = 0;
	}

	/*** fiddle around with prompt length ***/
	if( nchar+plen > MAXCHAR )
		nchar = MAXCHAR - plen;
	max_col = nchar - 2;

	if( !is_option(VIRAW) )
	{
#   ifndef ECHOCTL
		int kill_erase = 0;
		cntl_char = FALSE;
#   endif /* !ECHOCTL */
		for(i=0; i<=last_virt; ++i )
		{
			/*** change \r to \n, check for control characters, ***/
			/* delete appropriate ^Vs,			*/
			/* and estimate last physical column */

			if( virtual[i] == '\r' )
				virtual[i] = '\n';
#   ifndef ECHOCTL
			c = virtual[i];
			if( c==usrerase || c==usrkill )
			{
				/*** user typed escaped erase or kill char ***/
				cntl_char = TRUE;
				if(is_print(c))
					kill_erase++;
			}
			else if( !is_print(c) )
			{
				cntl_char = TRUE;

				if( c == cntl('V') )
				{
					if( i == last_virt )
					{
						/*** eol/eof was escaped ***/
						/* so replace ^V with it */
						virtual[i] = term_char;
						break;
					}

					/*** delete ^V ***/
					gencpy((&virtual[i]), (&virtual[i+1]));
					--cur_virt;
					--last_virt;
				}
			}
#   endif /* !ECHOCTL */
		}

		/*** copy virtual image to window ***/
		if(last_virt > 0)
			last_phys = ed_virt_to_phys(virtual,physical,last_virt,0,0);
		if( last_phys >= w_size )
		{
			/*** line longer than window ***/
			last_wind = w_size - 1;
		}
		else
			last_wind = last_phys;
		genncpy(window, virtual, last_wind+1);

		if( term_char!=ESC  && (last_virt==INVALID
			|| virtual[last_virt]!=term_char) )
		{
			/*** Line not terminated with ESC or escaped (^V) ***/
			/* eol, so return after doing a total update */
			/* if( (speed is greater or equal to 1200 */
			/* and something was typed) and */
			/* (control character present */
			/* or typeahead occurred) ) */

			tty_cooked(fd);
			if( ttyspeed==FAST && last_virt!=INVALID
# ifdef ECHOCTL
				&& typeahead)
# else
				&& (typeahead || cntl_char==TRUE) )
# endif /*ECHOCTL */
			{
				refresh(fd,TRANSLATE);
				pr_prompt();
				putstring(fd,0, last_phys+1);
#   ifdef ECHOCTL
				crlf(fd);
#   else
				while(kill_erase-- > 0)
					putchar(fd,' ');
#   endif /* ECHOCTL */
			}

			if( term_char=='\n' )
			{
#   ifndef ECHOCTL
				crlf(fd);
#   endif /* !ECHOCTL */
				virtual[++last_virt] = '\n';
			}
			last_cmd = 'i';
			save_last();
			return(++last_virt);
		}

		/*** Line terminated with escape, or escaped eol/eof, ***/
		/*  so set raw mode */

		if( tty_raw(fd) == BAD )
		{
			tty_cooked(fd);
			virtual[++last_virt] = '\n';
			return(++last_virt);
		}

#   ifdef ECHOCTL
		/*** for cntl-echo erase the ^[ ***/
		pr_string("\b\b  \b\b");
#   else
#	ifdef cray
		pr_string("\r\r\r\r");
#	endif /* cray */
#   endif /* ECHOCTL */


		if( crallowed == YES )
		{
			/*** start over since there may be ***/
			/*** a control char, or cursor might not ***/
			/*** be at left margin (this lets us know ***/
			/*** where we are ***/
			cur_phys = 0;
			window[0] = '\0';
			pr_prompt();
			if( term_char==ESC && virtual[last_virt]!=ESC )
				refresh(fd,CONTROL);
			else
				refresh(fd,INPUT);
		}
		else
		{
			/*** just update everything internally ***/
			refresh(fd,TRANSLATE);
		}
	}
	else
		virtual[0] = '\0';

	/*** Handle usrintr, usrquit, or EOF ***/

	if( (i=setjmp(env)) != 0 )
	{
		virtual[0] = '\0';
		tty_cooked(fd);

		switch(i)
		{
		case UEOF:
			/*** EOF ***/
			return(0);

		case UINTR:
			/** interrupt **/
			return(SYSERR);
		}
		return(SYSERR);
	}

	/*** Get a line from the terminal ***/

	U_saved = FALSE;

	if( is_option(VIRAW) || virtual[last_virt]==term_char )
		getline(fd,APPEND);
	else
		getline(fd,ESC);

	/*** add a new line if user typed unescaped \n ***/
	/* to cause the shell to process the line */
	tty_cooked(fd);
	if( addnl )
	{
		virtual[++last_virt] = '\n';
		crlf(fd);
	}
	if( ++last_virt >= 0 )
	{
		return(last_virt);
	}
	else
		return(SYSERR);
}
#undef virtual
#define virtual		editb.e_inbuf	/* pointer to virtual image buffer */

/*{	APPEND( char, mode )
 *
 *	This routine will append char after cur_virt in the virtual image.
 * mode	=	APPEND, shift chars right before appending
 *		REPLACE, replace char if possible
 *
}*/

static int
append(fd,c, mode)
int fd;
int c;
int mode;
{
	register int i;

	if( last_virt<max_col && last_phys<max_col )
	{
		if( mode==APPEND || cur_virt==last_virt )
		{
			for(i = ++last_virt;  i > cur_virt; --i)
			{
				virtual[i] = virtual[i-1];
			}
		}
		virtual[++cur_virt] = c;
	}
	else
		bell(fd);
	return;
}

/*{	BACKWORD( nwords, cmd )
 *
 *	This routine will position cur_virt at the nth previous word.
 *
}*/

static int
backword(nwords, cmd)
int nwords;
register int  cmd;
{
	register int tcur_virt = cur_virt;
	while( nwords-- && tcur_virt > first_virt )
	{
		if( !isblank(tcur_virt) && isblank(tcur_virt-1)
			&& tcur_virt>first_virt )
			--tcur_virt;
		else if(cmd != 'B')
		{
			register int last = isalph(tcur_virt-1);
			if((!isalph(tcur_virt) && last)
			|| (isalph(tcur_virt) && !last))
				--tcur_virt;
		}
		while( isblank(tcur_virt) && tcur_virt>=first_virt )
			--tcur_virt;
		if( cmd == 'B' )
		{
			while( !isblank(tcur_virt) && tcur_virt>=first_virt )
				--tcur_virt;
		}
		else
		{
			if( isalph(tcur_virt) )
				while( isalph(tcur_virt) && tcur_virt>=first_virt )
					--tcur_virt;
			else
				while( !isalph(tcur_virt) && !isblank(tcur_virt)
					&& tcur_virt>=first_virt )
					--tcur_virt;
		}
		cur_virt = ++tcur_virt;
	}
	return;
}

/*{	CNTLMODE()
 *
 *	This routine implements the vi command subset.
 *	The cursor will always be positioned at the char of interest.
 *
}*/

static int
cntlmode(fd)
int fd;
{
	register int c;
	register int i;
	genchar tmp_u_space[MAXLINE];	/* temporary u_space */
	genchar *real_u_space;		/* points to real u_space */
	int tmp_u_column = INVALID;	/* temporary u_column */
	int was_inmacro;

	if( U_saved == FALSE )
	{
		/*** save virtual image if never done before ***/
		virtual[last_virt+1] = '\0';
		gencpy(U_space, virtual);
		U_saved = TRUE;
	}

	save_last();

	real_u_space = u_space;
	curhline = histmax;
	first_virt = 0;
	repeat = 1;
	if( cur_virt > INVALID )
	{
		/*** make sure cursor is at the last char ***/
		sync_cursor(fd);
	}

	/*** Read control char until something happens to cause a ***/
	/* return to APPEND/REPLACE mode	*/

	while( c=getchar(fd) )
	{
		repeat_set = 0;
		was_inmacro = inmacro;
		if( c == '0' )
		{
			/*** move to leftmost column ***/
			cur_virt = 0;
			sync_cursor(fd);
			continue;
		}

		if( digit(c) )
		{
			lastrepeat = repeat;
			c = getcount(fd,c);
			if( c == '.' )
				lastrepeat = repeat;
		}

		/*** see if it's a move cursor command ***/

		if( mvcursor(fd,c) == GOOD )
		{
			sync_cursor(fd);
			repeat = 1;
			continue;
		}

		/*** see if it's a repeat of the last command ***/

		if( c == '.' )
		{
			c = last_cmd;
			repeat = lastrepeat;
			i = textmod(fd,c, c);
		}
		else
		{
			i = textmod(fd,c, 0);
		}

		/*** see if it's a text modification command ***/

		switch(i)
		{
		case BAD:
			break;

		default:		/** input mode **/
			if(!was_inmacro)
			{
				last_cmd = c;
				lastrepeat = repeat;
			}
			repeat = 1;
			if( i == GOOD )
				continue;
			return(i);
		}

		switch( c )
		{
			/***** Other stuff *****/

		case cntl('L'):		/** Redraw line **/
			/*** print the prompt and ***/
			/* force a total refresh */
			if(nonewline==0)
				putchar(fd,'\n');
			nonewline = 0;
			pr_prompt();
			window[0] = '\0';
			cur_phys = first_wind;
			ofirst_wind = INVALID;
			long_line = ' ';
			break;

		case cntl('V'):
		{
			register const char *p = &e_version[5];
			save_v();
			del_line(fd,BAD);
			while(c = *p++)
				append(fd,c,APPEND);
			refresh(fd,CONTROL);
			ed_getchar(fd);
			restore_v(fd);
			break;
		}

		case '/':		/** Search **/
		case '?':
		case 'N':
		case 'n':
			save_v();
			switch( search(fd,c) )
			{
			case GOOD:
				/*** force a total refresh ***/
				window[0] = '\0';
				goto newhist;

			case BAD:
				/*** no match ***/
					bell(fd);

			default:
				if( u_column == INVALID )
					del_line(fd,BAD);
				else
					restore_v(fd);
				break;
			}
			break;

		case 'j':		/** get next command **/
		case '+':		/** get next command **/
			curhline += repeat;
			if( curhline > histmax )
			{
				curhline = histmax;
				goto ringbell;
			}
			else if(curhline==histmax && tmp_u_column!=INVALID )
			{
				u_space = tmp_u_space;
				u_column = tmp_u_column;
				restore_v(fd);
				u_space = real_u_space;
				break;
			}
			save_v();
			goto newhist;

		case 'k':		/** get previous command **/
		case '-':		/** get previous command **/
			if( curhline == histmax )
			{
				u_space = tmp_u_space;
				i = u_column;
				save_v();
				u_space = real_u_space;
				tmp_u_column = u_column;
				u_column = i;
			}

			curhline -= repeat;
			if( curhline <= histmin )
			{
				curhline = histmin + 1;
				goto ringbell;
			}
			save_v();
	newhist:
			hist_copy((char*)virtual, curhline, -1);
			if( (last_virt = genlen((char*)virtual) - 1) >= 0 )
				cur_virt = 0;
			else
				cur_virt = INVALID;
			break;

		case 'J':		/** join current line to last line */
			{
				int i,j,col;
				genchar tmp_line[MAXCHAR+2];

				if (curhline >= histmax)
					goto ringbell;	

				hist_copy(tmp_line, curhline, -1);
				j = strlen(tmp_line);

				if (tmp_u_column == INVALID) {
					i = 0;
				}
				else {
					i = strlen(tmp_u_space);
				}
				/* room for both (and ' ') ? */
				if (i+j > MAXCHAR-1)
					goto ringbell;

				curhline = histmax;
				if (tmp_u_column == INVALID) {
					virtual[0] = '\0';

					col = j-1;
				}
				else {
					/* restore out of tmp space */
					u_space = tmp_u_space;
					u_column = tmp_u_column;
					restore_v(fd);
					u_space = real_u_space;
					/* save it away */
					save_v();
					virtual[i] = ' ';
					virtual[i+1] = '\0';
					col = i;
					i = i+1;
				}

				gencpy(&virtual[i], tmp_line);
				cur_virt = col;
				last_virt = i+j-1;
			}
			break;

		case 'u':		/** undo the last thing done **/
			restore_v(fd);
			break;

		case 'U':		/** Undo everything **/
			save_v();
			if( virtual[0] == '\0' )
				goto ringbell;
			else
			{
				gencpy(virtual, U_space);
				last_virt = genlen(U_space) - 1;
				cur_virt = 0;
			}
			break;

		case 'v':
			if(repeat_set==0)
				goto vcommand;

		case 'G':		/** goto command repeat **/
			if(repeat_set==0)
				repeat = histmin+1;
			if( repeat <= histmin || repeat > histmax )
			{
				goto ringbell;
			}
			curhline = repeat;
			save_v();
			if(c == 'G')
				goto newhist;

		vcommand:
#ifdef NOTDEF
			if(ed_fulledit()==GOOD)
				return(BIGVI);
			else
#endif
				goto ringbell;

		case '#':		/** insert # to no-op command **/
			if( last_virt != INVALID ) {
				if( cur_virt != INVALID ) {
					cur_virt = INVALID;
					append(fd,'#', APPEND);
				}
				/* write and flush history */
				virtual[last_virt+1] = 0;
				hist_eof();
				fputs((char*)virtual,fc_fix->fixfd);
				hist_flush();
				del_line(fd,GOOD);
				tmp_u_column = INVALID;
				histmax++;
				curhline = histmax;
				return(INSERT);
			}
			else
				goto ringbell;

		case '\n':		/** send to shell **/
			return(ENTER);

		default:
		ringbell:
			bell(fd);
			repeat = 1;
			continue;
		}

		refresh(fd,CONTROL);
		repeat = 1;
	}
/* NOTREACHED */
}

/*{	CURSOR( new_current_physical )
 *
 *	This routine will position the virtual cursor at
 * physical column x in the window.
 *
}*/

static int
cursor(fd,x)
register int x;
{
	register int delta;

	delta = x - cur_phys;

	if( delta == 0 )
		return;

	if( delta > 0 )
	{
		/*** move to right ***/
		putstring(fd,cur_phys, delta);
	}
	else
	{
		/*** move to left ***/

		delta = -delta;

		/*** attempt to optimize cursor movement ***/
		if( crallowed==NO
			|| (delta <= ((cur_phys-first_wind)+plen)>>1) )
		{
			while( delta-- )
				putchar(fd,'\b');
		}
		else
		{
			pr_prompt();
			putstring(fd,first_wind, x - first_wind);
		}
	}
	cur_phys = x;
	return;
}

/*{	DELETE( nchars, mode )
 *
 *	Delete nchars from the virtual space and leave cur_virt positioned
 * at cur_virt-1.
 *
 *	If mode	= 'c', do not save the characters deleted
 *		= 'd', save them in yankbuf and delete.
 *		= 'y', save them in yankbuf but do not delete.
 *
}*/

static int
delete(fd,nchars, mode)
int fd;
register int nchars;
char mode;
{
	register int i;
	register genchar *vp;

	if( cur_virt < first_virt )
	{
		bell(fd);
		return;
	}
	if( nchars > 0 )
	{
		vp = virtual+cur_virt;
		o_v_char = vp[0];
		if( (cur_virt-- + nchars) > last_virt )
		{
			/*** set nchars to number actually deleted ***/
			nchars = last_virt - cur_virt;
		}

		/*** save characters to be deleted ***/

		if( mode != 'c' )
		{
			i = vp[nchars];
			vp[nchars] = 0;
			gencpy(yankbuf,vp);
			vp[nchars] = i;
		}

		/*** now delete these characters ***/

		if( mode != 'y' )
		{
			gencpy(vp,vp+nchars);
			last_virt -= nchars;
		}
	}
	return;
}

/*{	DEL_LINE( mode )
 *
 *	This routine will delete the line.
 *	mode = GOOD, do a save_v()
 *
}*/

static int
del_line(fd,mode)
int fd;
int mode;
{
	if( last_virt == INVALID )
		return;

	if( mode == GOOD )
		save_v();

	cur_virt = 0;
	first_virt = 0;
	delete(fd,last_virt+1, BAD);
	refresh(fd,CONTROL);

	cur_virt = INVALID;
	cur_phys = 0;
	findchar = INVALID;
	last_phys = INVALID;
	last_virt = INVALID;
	last_wind = INVALID;
	first_wind = 0;
	o_v_char = '\0';
	ocur_phys = 0;
	ocur_virt = MAXCHAR;
	ofirst_wind = 0;
	window[0] = '\0';
	return;
}

/*{	DELMOTION( motion, mode )
 *
 *	Delete thru motion.
 *
 *	mode	= 'd', save deleted characters, delete
 *		= 'c', do not save characters, change
 *		= 'y', save characters, yank
 *
 *	Returns GOOD if operation successful; else BAD.
 *
}*/


static int
delmotion(fd,motion, mode)
int fd;
int motion;
char mode;
{
	register int begin;
	register int end;
	/* the following saves a register */
#       define delta end

	if( cur_virt == INVALID )
		return(BAD);
	if( mode != 'y' )
		save_v();
	begin = cur_virt;

	/*** fake out the motion routines by appending a blank ***/

	virtual[++last_virt] = ' ';
	end = mvcursor(fd,motion);
	virtual[last_virt--] = 0;
	if(end==BAD)
		return(BAD);

	end = cur_virt;
	if( mode=='c' && end>begin && strchr("wW", motion) )
	{
		/*** called by change operation, user really expects ***/
		/* the effect of the eE commands, so back up to end of word */
		while( end>begin && isblank(end-1) )
			--end;
		if( end == begin )
			++end;
	}

	delta = end - begin;
	if( delta >= 0 )
	{
		cur_virt = begin;
		if( strchr("eE;,TtFf", motion) )
			++delta;
	}
	else
	{
		delta = -delta;
	}

	delete(fd,delta, mode);
	if( mode == 'y' )
		cur_virt = begin;
#       undef delta
	return(GOOD);
}


/*{	ENDWORD( nwords, cmd )
 *
 *	This routine will move cur_virt to the end of the nth word.
 *
}*/

static int
endword(nwords, cmd)
int nwords;
register int cmd;
{
	register int tcur_virt = cur_virt;
	while( nwords-- )
	{
		if( !isblank(tcur_virt) && tcur_virt<=last_virt )
			++tcur_virt;
		while( isblank(tcur_virt) && tcur_virt<=last_virt )
			++tcur_virt;	
		if( cmd == 'E' )
		{
			while( !isblank(tcur_virt) && tcur_virt<=last_virt )
				++tcur_virt;
		}
		else
		{
			if( isalph(tcur_virt) )
				while( isalph(tcur_virt) && tcur_virt<=last_virt )
					++tcur_virt;
			else
				while( !isalph(tcur_virt) && !isblank(tcur_virt)
					&& tcur_virt<=last_virt )
					++tcur_virt;
		}
		if( tcur_virt > first_virt )
			tcur_virt--;
	}
	cur_virt = tcur_virt;
	return;
}

/*{	FORWARD( nwords, cmd )
 *
 *	This routine will move cur_virt forward to the next nth word.
 *
}*/

static int
forward(nwords, cmd)
register int nwords;
char cmd;
{
	register int tcur_virt = cur_virt;
	while( nwords-- )
	{
		if( cmd == 'W' )
		{
			while( !isblank(tcur_virt) && tcur_virt < last_virt )
				++tcur_virt;
		}
		else
		{
			if( isalph(tcur_virt) )
			{
				while( isalph(tcur_virt) && tcur_virt<last_virt )
					++tcur_virt;
			}
			else
			{
				while( !isalph(tcur_virt) && !isblank(tcur_virt)
					&& tcur_virt < last_virt )
					++tcur_virt;
			}
		}
		while( isblank(tcur_virt) && tcur_virt < last_virt )
			++tcur_virt;
	}
	cur_virt = tcur_virt;
	return;
}



/*{	GETCOUNT(c)
 *
 *	Set repeat to the user typed number and return the terminating
 * character.
 *
}*/


static int
getcount(fd,c)
int fd;
register int c;
{
	register int i;

	/*** get any repeat count ***/

	if( c == '0' )
		return(c);

	repeat_set++;
	i = 0;
	while( digit(c) )
	{
		i = i*10 + c - '0';
		c = getchar(fd);
	}

	if( i > 0 )
		repeat *= i;

	return(c);
}


/*{	GETLINE( mode )
 *
 *	This routine will fetch a line.
 *	mode	= APPEND, allow escape to cntlmode subroutine
 *		  appending characters.
 *		= REPLACE, allow escape to cntlmode subroutine
 *		  replacing characters.
 *		= SEARCH, no escape allowed
 *		= ESC, enter control mode immediately
 *
 *	The cursor will always be positioned after the last
 * char printed.
 *
 *	This routine returns when cr, nl, or (eof in column 0) is
 * received (column 0 is the first char position).
 *
}*/

static int
getline(fd,mode)
int fd;
register int mode;
{
	register int c;
	register int tmp;

	addnl = TRUE;

	if( mode == ESC )
	{
		/*** go directly to control mode ***/
		goto escape;
	}

	for(;;)
	{
		if( (c = getchar(fd)) == cntl('V') )
		{
			/*** implement ^V to escape next char ***/
			c = getchar(fd);
			append(fd,c, mode);
			refresh(fd,INPUT);
			continue;
		}

		if( c == usreof )
			c = UEOF;
		else if( c == usrerase )
			c = UERASE;
		else if( c == usrkill )
			c = UKILL;

		switch( c )
		{
		case ESC:		/** enter control mode **/
			if( mode == SEARCH )
			{
				bell(fd);
				continue;
			}
			else
			{
	escape:
				if( mode == REPLACE )
					--cur_virt;
				tmp = cntlmode(fd);
				if( tmp == ENTER || tmp == BIGVI )
				{
					return;
				}
				if( tmp == INSERT )
				{
					mode = APPEND;
					continue;
				}
				mode = tmp;
			}
			break;

		case UERASE:		/** user erase char **/
				/*** treat as backspace ***/

		case '\b':		/** backspace **/
			if( virtual[cur_virt] == '\\' )
			{
				delete(fd,1, BAD);
				append(fd,usrerase, mode);
			}
			else
			{
				if( mode==SEARCH && cur_virt==0 )
				{
					first_virt = 0;
					delete(fd,1, BAD);
					return;
				}
				delete(fd,1, BAD);
			}
			break;

		case cntl('W'):		/** delete back word **/
			if( cur_virt > first_virt && isblank(cur_virt-1) )
			{
				delete(fd,1, BAD);
			}
			else
			{
				tmp = cur_virt;
				backword(1, 'b');
				delete(fd,tmp - cur_virt + 1, BAD);
			}
			break;

		case UKILL:		/** user kill line char **/
			if( virtual[cur_virt] == '\\' )
			{
				delete(fd,1, BAD);
				append(fd,usrkill, mode);
			}
			else
			{
				if( mode == SEARCH )
				{
					cur_virt = 1;
					delmotion(fd,'$', BAD);
				}
				else
					del_line(fd,GOOD);
			}
			break;

		case UEOF:		/** eof char **/
			if( cur_virt != INVALID )
				continue;
			addnl = FALSE;

		case '\n':		/** newline or return **/
			if( mode != SEARCH )
				save_last();
			return;

		default:
			if( mode == REPLACE )
			{
				if( cur_virt < last_virt )
				{
					replace(fd,c, TRUE);
					continue;
				}
				delete(fd,1, BAD);
				mode = APPEND;
			}
			append(fd,c, mode);
			break;
		}
		refresh(fd,INPUT);

	}
}

/*{	MVCURSOR( motion )
 *
 *	This routine will move the virtual cursor according to motion
 * for repeat times.
 *
 * It returns GOOD if successful; else BAD.
 *
}*/

static int
mvcursor(fd,motion)
int fd;
register int motion;
{
	register int count;
	register int tcur_virt;
	register int incr = -1;
	register int bound = 0;
	static int last_find = 0;	/* last find command */

	switch(motion)
	{
		/***** Cursor move commands *****/

	case '0':		/** First column **/
		tcur_virt = 0;
		break;

	case '^':		/** First nonblank character **/
		tcur_virt = first_virt;
		while( isblank(tcur_virt) && tcur_virt < last_virt )
			++tcur_virt;
		break;

	case '|':
		tcur_virt = repeat-1;
		if(tcur_virt <= last_virt)
			break;
		/* fall through */

	case '$':		/** End of line **/
		tcur_virt = last_virt;
		break;

	case 'h':		/** Left one **/
	case '\b':
		motion = first_virt;
		goto walk;

	case ' ':
	case 'l':		/** Right one **/
		motion = last_virt;
		incr = 1;
	walk:
		tcur_virt = cur_virt;
		if( incr*tcur_virt < motion)
		{
			tcur_virt += repeat*incr;
			if( incr*tcur_virt > motion)
				tcur_virt = motion;
		}
		else
		{
			return(BAD);
		}
		break;

	case 'B':
	case 'b':		/** back word **/
		tcur_virt = cur_virt;
		backword(repeat, motion);
		if( cur_virt == tcur_virt )
			return(BAD);
		return(GOOD);

	case 'E':
	case 'e':		/** end of word **/
		tcur_virt = cur_virt;
		if(tcur_virt >=0)
			endword(repeat, motion);
		if( cur_virt == tcur_virt )
			return(BAD);
		return(GOOD);

	case ',':		/** reverse find old char **/
	case ';':		/** find old char **/
		switch(last_find)
		{
		case 't':
		case 'f':
			if(motion==';')
			{
				bound = last_virt;
				incr = 1;
			}
			goto find_b;

		case 'T':
		case 'F':
			if(motion==',')
			{
				bound = last_virt;
				incr = 1;
			}
			goto find_b;

		default:
			return(BAD);
		}


	case 't':		/** find up to new char forward **/
	case 'f':		/** find new char forward **/
		bound = last_virt;
		incr = 1;

	case 'T':		/** find up to new char backward **/
	case 'F':		/** find new char backward **/
		last_find = motion;
		if((findchar=getchar(fd))==ESC)
			return(GOOD);
find_b:
		tcur_virt = cur_virt;
		count = repeat;
		while( count-- )
		{
			while( incr*(tcur_virt+=incr) <= bound
				&& virtual[tcur_virt] != findchar );
			if( incr*tcur_virt > bound )
			{
				return(BAD);
			}
		}
		if( fold(last_find) == 'T' )
			tcur_virt -= incr;
		break;

	case 'W':
	case 'w':		/** forward word **/
		tcur_virt = cur_virt;
		forward(repeat, motion);
		if( tcur_virt == cur_virt )
			return(BAD);
		return(GOOD);

	default:
		return(BAD);
	}
	cur_virt = tcur_virt;

	return(GOOD);
}

/*{	PR_PROMPT()
 *
 *	Print the prompt.
 *
}*/

static void
pr_prompt()
{
	pr_string(Prompt);
	return;
}

/*
 * print a string
 */

static void
pr_string(s)
register char *s;
{
	/*** copy string s ***/
	register char *ptr = editb.e_outptr;
	while(*s)
		*ptr++ = *s++;
	editb.e_outptr = ptr;
	return;
}

/*{	PUTSTRING( column, nchars )
 *
 *	Put nchars starting at column of physical into the workspace
 * to be printed.
 *
}*/

static int
putstring(fd,col, nchars)
register int col;
register int nchars;
{
	while( nchars-- )
		putchar(fd,physical[col++]);
	return;
}

/*{	REFRESH( mode )
 *
 *	This routine will refresh the crt so the physical image matches
 * the virtual image and display the proper window.
 *
 *	mode	= CONTROL, refresh in control mode, ie. leave cursor
 *			positioned at last char printed.
 *		= INPUT, refresh in input mode; leave cursor positioned
 *			after last char printed.
 *		= TRANSLATE, perform virtual to physical translation
 *			and adjust left margin only.
 *
 *		+-------------------------------+
 *		|   | |    virtual	  | |   |
 *		+-------------------------------+
 *		  cur_virt		last_virt
 *
 *		+-----------------------------------------------+
 *		|	  | |	        physical	 | |    |
 *		+-----------------------------------------------+
 *			cur_phys			last_phys
 *
 *				0			w_size - 1
 *				+-----------------------+
 *				| | |  window		|
 *				+-----------------------+
 *				cur_window = cur_phys - first_wind
}*/

static int
refresh(fd,mode)
int fd;
int mode;
{
	register int p;
	register int regb;
	register int first_w = first_wind;
	int p_differ;
	int new_lw;
	int ncur_phys;
	int opflag;			/* search optimize flag */

#	define	w	regb
#	define	v	regb

	/*** find out if it's necessary to start translating at beginning ***/

	if(lookahead>0)
	{
		p = previous[lookahead-1];
		if(p != ESC && p != '\n' && p != '\r')
			mode = TRANSLATE;
	}
	v = cur_virt;
	if( v<ocur_virt || ocur_virt==INVALID
		|| ( v==ocur_virt
			&& (!is_print(virtual[v]) || !is_print(o_v_char))) )
	{
		opflag = FALSE;
		p = 0;
		v = 0;
	}
	else
	{
		opflag = TRUE;
		p = ocur_phys;
		v = ocur_virt;
		if( !is_print(virtual[v]) )
		{
			/*** avoid double ^'s ***/
			++p;
			++v;
		}
	}
	virtual[last_virt+1] = 0;
	ncur_phys = ed_virt_to_phys(virtual,physical,cur_virt,v,p);
	p = genlen(physical);
	if( --p < 0 )
		last_phys = 0;
	else
		last_phys = p;

	/*** see if this was a translate only ***/

	if( mode == TRANSLATE )
		return;

	/*** adjust left margin if necessary ***/

	if( ncur_phys<first_w || ncur_phys>=(first_w + w_size) )
	{
		cursor(fd,first_w);
		first_w = ncur_phys - (w_size>>1);
		if( first_w < 0 )
			first_w = 0;
		first_wind = cur_phys = first_w;
	}

	/*** attempt to optimize search somewhat to find ***/
	/*** out where physical and window images differ ***/

	if( first_w==ofirst_wind && ncur_phys>=ocur_phys && opflag==TRUE )
	{
		p = ocur_phys;
		w = p - first_w;
	}
	else
	{
		p = first_w;
		w = 0;
	}

	for(; (p<=last_phys && w<=last_wind); ++p, ++w)
	{
		if( window[w] != physical[p] )
			break;
	}
	p_differ = p;

	if( (p>last_phys || p>=first_w+w_size) && w>last_wind
		&& cur_virt==ocur_virt )
	{
		/*** images are identical ***/
		return;
	}

	/*** copy the physical image to the window image ***/

	if( last_virt != INVALID )
	{
		while( p <= last_phys && w < w_size )
			window[w++] = physical[p++];
	}
	new_lw = w;

	/*** erase trailing characters if needed ***/

	while( w <= last_wind )
		window[w++] = ' ';
	last_wind = --w;

	p = p_differ;

	/*** move cursor to start of difference ***/

	cursor(fd,p);

	/*** and output difference ***/

	w = p - first_w;
	while( w <= last_wind )
		putchar(fd,window[w++]);

	cur_phys = w + first_w;
	last_wind = --new_lw;

	if( last_phys >= w_size )
	{
		if( first_w == 0 )
			long_char = '>';
		else if( last_phys < (first_w+w_size) )
			long_char = '<';
		else
			long_char = '*';
	}
	else
		long_char = ' ';

	if( long_line != long_char )
	{
		/*** indicate lines longer than window ***/
		while( w++ < w_size )
		{
			putchar(fd,' ');
			++cur_phys;
		}
		putchar(fd,long_char);
		++cur_phys;
		long_line = long_char;
	}

	ocur_phys = ncur_phys;
	ocur_virt = cur_virt;
	ofirst_wind = first_w;

	if( mode==INPUT && cur_virt>INVALID )
		++ncur_phys;

	cursor(fd,ncur_phys);
	ed_flush(fd);
	return;
}

/*{	REPLACE( char, increment )
 *
 *	Replace the cur_virt character with char.  This routine attempts
 * to avoid using refresh().
 *
 *	increment	= TRUE, increment cur_virt after replacement.
 *			= FALSE, leave cur_virt where it is.
 *
}*/

static int
replace(fd,c, increment)
int fd;
register int c;
register int increment;
{
	register int cur_window;

	cur_window = cur_phys - first_wind;

	if( ocur_virt == INVALID || (!is_print(c) || !is_print(o_v_char))
		|| (increment==TRUE && (cur_window==w_size-1)
			|| !is_print(virtual[cur_virt+1])) )
	{
		/*** must use standard refresh routine ***/

		delete(fd,1, BAD);
		append(fd,c, APPEND);
		if( increment==TRUE && cur_virt<last_virt )
			++cur_virt;
		refresh(fd,CONTROL);
	}
	else
	{
		virtual[cur_virt] = c;
		physical[cur_phys] = c;
		window[cur_window] = c;
		putchar(fd,c);
		if( increment == TRUE )
		{
			c = virtual[++cur_virt];
			++cur_phys;
		}
		else
		{
			putchar(fd,'\b');
		}
		o_v_char = c;
		ed_flush(fd);
	}
	return;
}

/*{	RESTORE_V()
 *
 *	Restore the contents of virtual space from u_space.
 *
}*/

static int
restore_v(fd)
int fd;
{
	register int tmpcol;
	genchar tmpspace[MAXLINE];

	if( u_column == INVALID-1 )
	{
		/*** never saved anything ***/
		bell(fd);
		return;
	}
	gencpy(tmpspace, u_space);
	tmpcol = u_column;
	save_v();
	gencpy(virtual, tmpspace);
	cur_virt = tmpcol;
	last_virt = genlen(tmpspace) - 1;
	ocur_virt = MAXCHAR;	/** invalidate refresh optimization **/
	return;
}

/*{	SAVE_LAST()
 *
 *	If the user has typed something, save it in last line.
 *
}*/

static int
save_last()
{
	register int i;

	if( (i = cur_virt - first_virt + 1) > 0 )
	{
		/*** save last thing user typed ***/
		genncpy(lastline, (&virtual[first_virt]), i);
		lastline[i] = '\0';
	}
	return;
}

/*{	SAVE_V()
 *
 *	This routine will save the contents of virtual in u_space.
 *
}*/

static int
save_v()
{
	if(!inmacro)
	{
		virtual[last_virt + 1] = '\0';
		gencpy(u_space, virtual);
		u_column = cur_virt;
	}
	return;
}

/*{	SEARCH( mode )
 *
 *	Search history file for regular expression.
 *
 *	mode	= '/'	require search string and search new to old
 *	mode	= '?'	require search string and search old to new
 *	mode	= 'N'	repeat last search in reverse direction
 *	mode	= 'n'	repeat last search
 *
}*/

static int
search(fd,mode)
int fd;
register char mode;
{
	register int new_direction;
	register int oldcurhline;
	static int direction = -1;
	histloc  location;

	if( mode == '/' || mode == '?')
	{
		/*** new search expression ***/
		del_line(fd,BAD);
		append(fd,mode, APPEND);
		refresh(fd,INPUT);
		first_virt = 1;
		getline(fd,SEARCH);
		first_virt = 0;
		virtual[last_virt + 1] = '\0';	/*** make null terminated ***/
		direction = mode=='/' ? -1 : 1;
	}

	if( cur_virt == INVALID )
	{
		/*** no operation ***/
		return(ABORT);
	}

	if( cur_virt==0 ||  fold(mode)=='N' )
	{
		/*** user wants repeat of last search ***/
		del_line(fd,BAD);
		strcpy( ((char*)virtual)+1, lsearch);
	}

	if( mode == 'N' )
		new_direction = -direction;
	else
		new_direction = direction;

	if( new_direction==1 && curhline >= histmax )
		curhline = histmin + 1;

	/*** now search ***/

	oldcurhline = curhline;
	location = hist_find(((char*)virtual)+1, curhline, 1, new_direction);
	strcpy(lsearch, ((char*)virtual)+1);
	if( (curhline=location.his_command) >=0 )
	{
		return(GOOD);
	}

	/*** could not find matching line ***/

	curhline = oldcurhline;
	return(BAD);
}

/*{	SYNC_CURSOR()
 *
 *	This routine will move the physical cursor to the same
 * column as the virtual cursor.
 *
}*/

static int
sync_cursor(fd)
int fd;
{
	register int p;
	register int v;
	register int c;
	int new_phys;

	if( cur_virt == INVALID )
		return;

	/*** find physical col that corresponds to virtual col ***/

	new_phys = 0;
	if(first_wind==ofirst_wind && cur_virt>ocur_virt && ocur_virt!=INVALID)
	{
		/*** try to optimize search a little ***/
		p = ocur_phys + 1;
		v = ocur_virt + 1;
	}
	else
	{
		p = 0;
		v = 0;
	}
	for(; v <= last_virt; ++p, ++v)
	{
		c = virtual[v];
		if( !isprint(c) )
		{
			if( c == '\t' )
			{
				p -= ((p+editb.e_plen)%TABSIZE);
				p += (TABSIZE-1);
			}
			else
			{
				++p;
			}
		}
		if( v == cur_virt )
		{
			new_phys = p;
			break;
		}
	}

	if( new_phys < first_wind || new_phys >= first_wind + w_size )
	{
		/*** asked to move outside of window ***/

		window[0] = '\0';
		refresh(fd,CONTROL);
		return;
	}

	cursor(fd,new_phys);
	ed_flush(fd);
	ocur_phys = cur_phys;
	ocur_virt = cur_virt;
	o_v_char = virtual[ocur_virt];

	return;
}

/*{	TEXTMOD( command, mode )
 *
 *	Modify text operations.
 *
 *	mode != 0, repeat previous operation
 *
}*/

static int
textmod(fd,c, mode)
int fd;
register int c;
int mode;
{
	register int i;
	register genchar *p = lastline;
	register int trepeat = repeat;
	static int lastmacro;
	genchar *savep;

	if(mode && (fold(lastmotion)=='F' || fold(lastmotion)=='T')) 
		lastmotion = ';';

	if( fold(c) == 'P' )
	{
		/*** change p from lastline to yankbuf ***/
		p = yankbuf;
	}

addin:
	switch( c )
	{
			/***** Input commands *****/

#ifdef NOTDEF
	case '*':		/** do file name expansion in place **/
	case '\\':		/** do file name completion in place **/
	case '\033':		/** do file name completion in place **/
		if( cur_virt == INVALID )
			return(BAD);
	case '=':		/** list file name expansions **/
		save_v();
		i = last_virt;
		++last_virt;
		virtual[last_virt] = 0;
		if( ed_expand((char*)virtual, &cur_virt, &last_virt, c) )
		{
			last_virt = i;
			bell(fd);
		}
		else if(c == '=')
		{
			last_virt = i;
			nonewline++;
			ed_ungetchar(cntl('L'));
			return(GOOD);
		}
		else
		{
			--cur_virt;
			--last_virt;
			ocur_virt = MAXCHAR;
			return(APPEND);
		}
		break;
#endif /* NOTDEF */

#ifdef DEADCODE
	case '@':		/** macro expansion **/
		if( mode )
			c = lastmacro;
		else
			if((c=getchar(fd))==ESC)
				return(GOOD);
		if(!inmacro)
			lastmacro = c;
		if(ed_macro(fd,c))
		{
			save_v();
			inmacro++;
			return(GOOD);
		}
		bell(fd);
		return(BAD);
#endif

	case '_':		/** append last argument of prev command **/
		save_v();
		{
			genchar tmpbuf[MAXLINE];
			if(repeat_set==0)
				repeat = -1;
			p = (genchar*)hist_word(tmpbuf,repeat);
			i = ' ';
			do
			{
				append(fd,i,APPEND);
			}
			while(i = *p++);
			return(APPEND);
		}

	case 'A':		/** append to end of line **/
		cur_virt = last_virt;
		sync_cursor(fd);

	case 'a':		/** append **/
		if( fold(mode) == 'A' )
		{
			c = 'p';
			goto addin;
		}
		save_v();
		if( cur_virt != INVALID )
		{
			first_virt = cur_virt + 1;
			cursor(fd,cur_phys + 1);
			ed_flush(fd);
		}
		return(APPEND);

	case 'I':		/** insert at beginning of line **/
		cur_virt = first_virt;
		sync_cursor(fd);

	case 'i':		/** insert **/
		if( fold(mode) == 'I' )
		{
			c = 'P';
			goto addin;
		}
		save_v();
		if( cur_virt != INVALID )
			first_virt = cur_virt--;
		return(INSERT);

	case 'C':		/** change to eol **/
		c = '$';
		goto chgeol;

	case 'c':		/** change **/
		if( mode )
			c = lastmotion;
		else
			c = getcount(fd,getchar(fd));
chgeol:
		lastmotion = c;
		if( c == 'c' )
		{
			del_line(fd,GOOD);
			return(APPEND);
		}

		if( delmotion(fd,c, 'c') == BAD )
			return(BAD);

		if( mode == 'c' )
		{
			c = 'p';
			trepeat = 1;
			goto addin;
		}
		first_virt = cur_virt + 1;
		return(APPEND);

	case 'D':		/** delete to eol **/
		c = '$';
		goto deleol;

	case 'd':		/** delete **/
		if( mode )
			c = lastmotion;
		else
			c = getcount(fd,getchar(fd));
deleol:
		lastmotion = c;
		if( c == 'd' )
		{
			del_line(fd,GOOD);
			break;
		}
		if( delmotion(fd,c, 'd') == BAD )
			return(BAD);
		if( cur_virt < last_virt )
			++cur_virt;
		break;

	case 'P':
		if( p[0] == '\0' )
			return(BAD);
		if( cur_virt != INVALID )
			--cur_virt;

	case 'p':		/** print **/
		if( p[0] == '\0' )
			return(BAD);

		if( mode != 's' && mode != 'c' )
		{
			save_v();
			if( c == 'P' )
			{
				/*** fix stored cur_virt ***/
				++u_column;
			}
		}
		if( mode == 'R' )
			mode = REPLACE;
		else
			mode = APPEND;
		savep = p;
		for(i=0; i<trepeat; ++i)
		{
			while(c= *p++)
				append(fd,c,mode);
			p = savep;
		}
		break;

	case 'R':		/* Replace many chars **/
		if( mode == 'R' )
		{
			c = 'P';
			goto addin;
		}
		save_v();
		if( cur_virt != INVALID )
			first_virt = cur_virt;
		return(REPLACE);

	case 'r':		/** replace **/
		if( mode )
			c = *p;
		else
			if((c=getchar(fd))==ESC)
				return(GOOD);
		*p = c;
		save_v();
		while(trepeat--)
			replace(fd,c, trepeat!=0);
		return(GOOD);

	case 'S':		/** Substitute line - cc **/
		c = 'c';
		goto chgeol;

	case 's':		/** substitute **/
		save_v();
		delete(fd,repeat, BAD);
		if( mode )
		{
			c = 'p';
			trepeat = 1;
			goto addin;
		}
		first_virt = cur_virt + 1;
		return(APPEND);

	case 'Y':		/** Yank to end of line **/
		c = '$';
		goto yankeol;

	case 'y':		/** yank thru motion **/
		if( mode )
			c = lastmotion;
		else
			c = getcount(fd,getchar(fd));
yankeol:
		lastmotion = c;
		if( c == 'y' )
		{
			gencpy(yankbuf, virtual);
		}
		else if( delmotion(fd,c, 'y') == BAD )
		{
			return(BAD);
		}
		break;

	case 'x':		/** delete repeat chars forward - dl **/
		c = 'l';
		goto deleol;

	case 'X':		/** delete repeat chars backward - dh **/
		c = 'h';
		goto deleol;

	case '~':		/** invert case and advance **/
		if( cur_virt != INVALID )
		{
			save_v();
			i = INVALID;
			while(trepeat-->0 && i!=cur_virt)
			{
				i = cur_virt;
				c = virtual[cur_virt];
				if( isupper(c) )
					c = tolower(c);
				else if( islower(c) )
					c = toupper(c);
				replace(fd, c, TRUE);
			}
			return(GOOD);
		}
		else
			return(BAD);

	default:
		return(BAD);
	}
	refresh(fd,CONTROL);
	return(GOOD);
}



