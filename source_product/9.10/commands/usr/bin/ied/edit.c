/* @(#) $Revision: 66.5 $ */

/* For ied, it's not possible to use:
   ed_expand: built on top of all of ksh's "*" expansion.
   ed_fulledit: built on top of ksh's ability to exec things.
   ed_macro: no alias database
*/
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
/*
 *  edit.c - common routines for vi and emacs one line editors in shell
 *
 *   David Korn				P.D. Sullivan
 *   AT&T Bell Laboratories		AT&T Bell Laboratories
 *   Room 3C-526B			Room 1D-245
 *   Murray Hill, N. J. 07974		Columbus, OH 43213
 *   Tel. x7975				Tel. x 2655
 *
 *   Coded April 1983.
 */

#include	<errno.h>

#   include	"defs.h"
#   include	"terminal.h"
/* #   include	"builtins.h" */
/* #   include	"sym.h" */
#include	"history.h"
#include	"edit.h"

int opt_flags;

char workbuffer[IOBSIZE];

#define BAD	-1
#define GOOD	0
#define	TRUE	(-1)
#define	FALSE	0
#define	SYSERR	-1

#ifdef RT
#   define VENIX 1
#endif	/* RT */

#define lookahead	editb.e_index
#define env		editb.e_env
#define previous	editb.e_lbuf
#define fildes		editb.e_fd


#ifdef _sgtty_
#   ifdef TIOCGETP
	static int l_mask;
	static struct tchars l_ttychars;
	static struct ltchars l_chars;
	static  char  l_changed;	/* set if mode bits changed */
#	define L_CHARS	4
#	define T_CHARS	2
#	define L_MASK	1
#   endif /* TIOCGETP */
#endif /* _sgtty_ */

#ifdef _SELECT_
#   ifndef included_sys_time_
#	include	<sys/time.h>
#   endif /* included_sys_time_ */
	static int delay;
#   ifndef KSHELL
    	    int tty_speeds[] = {0, 50, 75, 110, 134, 150, 200, 300,
		600,1200,1800,2400,9600,19200,0};
#   endif	/* KSHELL */
#endif /* _SELECT_ */

#ifdef KSHELL
    extern int		f_complete();
    extern char		*sh_tilde();
    static char macro[]	= "_??";
#   define slowsig()	(sh.trapnote&SIGSLOW)
#else
    struct edit editb;
    extern int errno;
#   define slowsig()	(0)
#endif	/* KSHELL */


static struct termios savetty;
static int savefd = -1;
static int compare();
#if VSH || ESH
    extern char *strrchr();
    extern char *strcpy();
    static struct termios ttyparm;	/* initial tty parameters */
    static struct termios nttyparm;	/* raw tty parameters */
    static char bellchr[] = "\7";	/* bell char */
#   define tenex 1
#   ifdef tenex
	static char *overlay();
#   endif /* tenex */
#endif /* VSH || ESH */


/*
 * This routine returns true if fd refers to a terminal
 * This should be equivalent to isatty
 */

#ifdef DEAD_CODE
int tty_check(fd)
{
	return(tty_get(fd,(struct termios*)0)==0);
}
#endif

/*
 * Get the current terminal attributes
 * This routine remembers the attributes and just returns them if it
 *   is called again without an intervening tty_set()
 */

int tty_get(fd, tty)
struct termios *tty;
{
	if(fd != savefd)
	{
#ifndef SIG_NORESTART
		VOID (*savint)() = st.intfn;
		st.intfn = 0;
#endif	/* SIG_NORESTART */
		while(tcgetattr(fd,&savetty) == SYSERR)
		{
			if(errno !=EINTR)
			{
#ifndef SIG_NORESTART
				st.intfn = savint;
#endif	/* SIG_NORESTART */
				return(SYSERR);
			}
			errno = 0;
		}
#ifndef SIG_NORESTART
		st.intfn = savint;
#endif	/* SIG_NORESTART */
		savefd = fd;
	}
	if(tty)
		*tty = savetty;
	return(0);
}

/*
 * Set the terminal attributes
 * If fd<0, then current attributes are invalidated
 */

/* VARARGS 2 */
int tty_set(fd, action, tty)
struct termios *tty;
{
	if(fd >=0)
	{
#ifndef SIG_NORESTART
		VOID (*savint)() = st.intfn;
#endif	/* SIG_NORESTART */
#ifdef future
		if(savefd>=0 && compare(&savetty,tty,sizeof(struct termios)))
			return(0);
#endif
#ifndef SIG_NORESTART
		st.intfn = 0;
#endif	/* SIG_NORESTART */
		while(tcsetattr(fd, action, tty) == SYSERR)
		{
			if(errno !=EINTR)
			{
#ifndef SIG_NORESTART
				st.intfn = savint;
#endif	/* SIG_NORESTART */
				return(SYSERR);
			}
			errno = 0;
		}
#ifndef SIG_NORESTART
		st.intfn = savint;
#endif	/* SIG_NORESTART */
		savetty = *tty;
	}
	savefd = fd;
	return(0);
}

#if ESH || VSH
/*{	TTY_COOKED( fd )
 *
 *	This routine will set the tty in cooked mode.
 *	It is also called by error.done().
 *
}*/

void tty_cooked(fd)
register int fd;
{
	/*** don't do tty_set unless ttyparm has valid data ***/
	/* or in raw mode */

	if(savefd<0 || editb.e_raw==0)
		return;

#ifdef TIOCGETC
	if(editb.e_raw!=RAWMODE)
		return;
#endif /* TIOCGETC */
	if(tty_set(fd, TCSANOW, &ttyparm) == SYSERR )
		return;
#ifdef L_MASK
	/* restore flags */
	if(l_changed&L_MASK)
		ioctl(fd,TIOCLSET,&l_mask);
	if(l_changed&T_CHARS)
		/* restore alternate break character */
		ioctl(fd,TIOCSETC,&l_ttychars);
	if(l_changed&L_CHARS)
		/* restore alternate break character */
		ioctl(fd,TIOCSLTC,&l_chars);
	l_changed = 0;
#endif	/* L_MASK */
	editb.e_raw = 0;
	return;
}

/*{	TTY_RAW( fd )
 *
 *	This routine will set the tty in raw mode.
 *
}*/

tty_raw(fd)
register int fd;
{
	long oldmask;
#ifdef L_MASK
	struct ltchars lchars;
#endif	/* L_MASK */
	if(editb.e_raw==RAWMODE)
		return(GOOD);
	oldmask = sigblock(sigmask(SIGCLD));
#ifndef RAWONLY
	if(editb.e_raw != ALTMODE)
#endif /* RAWONLY */
	{
		if(tty_get(fd,&ttyparm) == SYSERR) {
			sigsetmask(oldmask);
			return(BAD);
		}
	}
#if  L_MASK || VENIX
	if(!(ttyparm.sg_flags&ECHO) || (ttyparm.sg_flags&LCASE)) {
		sigsetmask(oldmask);
		return(BAD);
	}
	nttyparm = ttyparm;
	nttyparm.sg_flags &= ~(ECHO | TBDELAY);
#   ifdef CBREAK
	nttyparm.sg_flags |= CBREAK;
#   else
	nttyparm.sg_flags |= RAW;
#   endif /* CBREAK */
	editb.e_erase = ttyparm.sg_erase;
	editb.e_kill = ttyparm.sg_kill;
	editb.e_eof = cntl('D');
	if( tty_set(fd, TCSADRAIN, &nttyparm) == SYSERR ) {
		sigsetmask(oldmask);
		return(BAD);
	}
	editb.e_ttyspeed = (ttyparm.sg_ospeed>=B1200?FAST:SLOW);
#   ifdef _SELECT_
	    delay = tty_speeds[ttyparm.sg_ospeed];
#   endif /* _SELECT_ */
#   ifdef TIOCGLTC
	/* try to remove effect of ^V  and ^Y and ^O */
	if(ioctl(fd,TIOCGLTC,&l_chars) != SYSERR)
	{
		lchars = l_chars;
		lchars.t_lnextc = -1;
		lchars.t_flushc = -1;
		lchars.t_dsuspc = -1;	/* no delayed stop process signal */
		if(ioctl(fd,TIOCSLTC,&lchars) != SYSERR)
			l_changed |= L_CHARS;
	}
#   endif	/* TIOCGLTC */
#else

	if (!(ttyparm.c_lflag & ECHO )) {
		sigsetmask(oldmask);
		return(BAD);
	}

	nttyparm = ttyparm;
#  ifndef u370
	nttyparm.c_iflag &= ~(IGNPAR|PARMRK|INLCR|IGNCR|ICRNL);
	nttyparm.c_iflag |= BRKINT;
	nttyparm.c_lflag &= ~(ICANON|ECHO);
#   else
	nttyparm.c_iflag &= 
			~(IGNBRK|PARMRK|INLCR|IGNCR|ICRNL|INPCK);
	nttyparm.c_iflag |= (BRKINT|IGNPAR);
	nttyparm.c_lflag &= ~(ICANON|ECHO);
#   endif	/* u370 */
	nttyparm.c_cc[VTIME] = 0;
	nttyparm.c_cc[VMIN] = 1;
	editb.e_eof = ttyparm.c_cc[VEOF];
	editb.e_erase = ttyparm.c_cc[VERASE];
	editb.e_kill = ttyparm.c_cc[VKILL];
	if( tty_set(fd, TCSADRAIN, &nttyparm) == SYSERR ) {
		sigsetmask(oldmask);
		return(BAD);
	}
	editb.e_ttyspeed = ttyparm.c_cflag & CBAUD;
#endif
	editb.e_raw = RAWMODE;
	sigsetmask(oldmask);
	return(GOOD);
}

#ifndef RAWONLY

/*
 *
 *	Get tty parameters and make ESC and '\r' wakeup characters.
 *
 */

#   ifdef TIOCGETC
tty_alt(fd)
register int fd;
{
	int mask;
	struct tchars ttychars;
	if(editb.e_raw==ALTMODE)
		return(GOOD);
	if(editb.e_raw==RAWMODE)
		tty_cooked(fd);
	l_changed = 0;
	if( editb.e_ttyspeed == 0)
	{
		if((tty_get(fd,&ttyparm) != SYSERR))
			editb.e_ttyspeed = (ttyparm.sg_ospeed>=B1200?FAST:SLOW);
	}
	if(ioctl(fd,TIOCGETC,&l_ttychars) == SYSERR)
		return(BAD);
	if(ioctl(fd,TIOCLGET,&l_mask)==SYSERR)
		return(BAD);
	ttychars = l_ttychars;
	mask =  LCRTBS|LCRTERA|LCTLECH|LPENDIN|LCRTKIL;
	if((l_mask|mask) != l_mask)
		l_changed = L_MASK;
	if(ioctl(fd,TIOCLBIS,&mask)==SYSERR)
		return(BAD);
	ttychars.t_brkc = ESC;
	l_changed |= T_CHARS;
	if(ioctl(fd,TIOCSETC,&ttychars) == SYSERR)
		return(BAD);
	editb.e_raw = ALTMODE;
	return(GOOD);
}
#   else
tty_alt(fd)
register int fd;
{
	if(editb.e_raw==ALTMODE)
		return(GOOD);
	if(editb.e_raw==RAWMODE)
		tty_cooked(fd);
	if((tty_get(fd, &ttyparm)==SYSERR) || (!(ttyparm.c_lflag&ECHO)))
		return(BAD);
	nttyparm = ttyparm;
	editb.e_eof = ttyparm.c_cc[VEOF];
#	ifdef ECHOCTL
	    /* escape character echos as ^[ */
#	    ifdef PENDIN
		nttyparm.c_lflag |= (ECHOE|ECHOK|ECHOCTL|PENDIN);
#	    else
		nttyparm.c_lflag |= (ECHOE|ECHOK|ECHOCTL);
#	    endif /* PENDIN */
	    nttyparm.c_cc[VEOL2] = ESC;
#	else
	    /* switch VEOL2 and EOF, since EOF isn't echo'd by driver */
	    nttyparm.c_iflag &= ~(IGNCR|ICRNL);
	    nttyparm.c_iflag |= INLCR;
	    nttyparm.c_lflag |= (ECHOE|ECHOK);
	    nttyparm.c_cc[VEOF] = ESC;	/* make ESC the eof char */
	    nttyparm.c_cc[VEOL] = '\r';	/* make CR an eol char */
	    nttyparm.c_cc[VEOL2] = editb.e_eof;	/* make EOF an eol char */
#	endif /* ECHOCTL */
#	ifdef VWERASE
	    nttyparm.c_cc[VWERASE] = cntl('W');
#	endif /* VWERASE */
#	ifdef VLNEXT
	    nttyparm.c_cc[VLNEXT] = cntl('V');
#	endif /* VLNEXT */
	editb.e_erase = ttyparm.c_cc[VERASE];
	editb.e_kill = ttyparm.c_cc[VKILL];
	if( tty_set(fd, TCSADRAIN, &nttyparm) == SYSERR )
		return(BAD);
	editb.e_ttyspeed = ((ttyparm.c_cflag&CBAUD)>=B1200?FAST:SLOW);
	editb.e_raw = ALTMODE;
	return(GOOD);
}

#   endif /* TIOCGETC */
#endif	/* RAWONLY */

/*
 *	E_WINDOW()
 *
 *	return the window size
 */

#ifndef WINSIZE
#   undef TIOCGWINSZ
#endif /* !WINSIZE */
#ifdef TIOCGWINSZ
#   ifdef _sys_stream_
#	include	<sys/stream.h>
#   endif /* _sys_ptem_ */
#   ifdef _sys_ptem_
#	include	<sys/ptem.h>
#   endif /* _sys_stream_ */
#else
#   ifdef _sys_jioctl_
#	include	<sys/jioctl.h>
#	define winsize		jwinsize
#	define ws_col		bytesx
#	define TIOCGWINSZ	JWINSIZE
#   endif /* _sys_jioctl_ */
#endif /*TIOCGWINSZ */
int ed_window(fd)
int fd;
{
	register int n = DFLTWINDOW-1;
	register char *cp = nam_strval(COLUMNS);
	if(cp)
	{
		n = atoi(cp)-1;
		if(n > MAXWINDOW)
			n = MAXWINDOW;
	}
#ifdef TIOCGWINSZ
	else
	{
		/* for 5620's and 630's */
		struct winsize size;
		if (ioctl(fd, TIOCGWINSZ, &size) != -1)
			if(size.ws_col > 0)
				n = size.ws_col - 1;
	}
#endif /*TIOCGWINSZ */
	if(n < MINWINDOW)
		n = MINWINDOW;
	return(n);
}

/*	E_FLUSH()
 *
 *	Flush the output buffer.
 *
 */

void ed_flush(fd)
register int fd;
{
	register int n = editb.e_outptr-editb.e_outbase;
	if(n<=0)
		return;
	write(fd,editb.e_outbase,(unsigned)n);
	editb.e_outptr = editb.e_outbase;
#ifdef _SELECT_
	if(delay && n > delay/100)
	{
		/* delay until output drains */
		struct timeval timeloc;
		n *= 10;
		timeloc.tv_sec = n/delay;
		timeloc.tv_usec = (1000000*(n%delay))/delay;
		select(0,(fd_set*)0,(fd_set*)0,(fd_set*)0,&timeloc);
	}
#else
#   ifdef IODELAY
	if(editb.e_raw==RAWMODE && n > 16)
		tty_set(fd, TCSADRAIN, &nttyparm);
#   endif /* IODELAY */
#endif /* _SELECT_ */
}

/*
 * send the bell character ^G to the terminal
 */

void ed_ringbell(fd)
int fd;
{
	write(fd,bellchr,1);
}

/*
 * send a carriage return line feed to the terminal
 */

void ed_crlf(fd)
int fd;
{
#ifdef cray
	ed_putchar(fd,'\r');
#endif /* cray */
#ifdef u370
	ed_putchar(fd,'\r');
#endif	/* u370 */
#ifdef VENIX
	ed_putchar(fd,'\r');
#endif /* VENIX */
	ed_putchar(fd,'\n');
	ed_flush(fd);
}
 
/*	E_SETUP( max_prompt_size )
 *
 *	This routine sets up the prompt string
 *	The following is an unadvertised feature.
 *	  Escape sequences in the prompt can be excluded from the calculated
 *	  prompt length.  This is accomplished as follows:
 *	  - if the prompt string starts with "%\r, or contains \r%\r", where %
 *	    represents any char, then % is taken to be the quote character.
 *	  - strings enclosed by this quote character, and the quote character,
 *	    are not counted as part of the prompt length.
 */

void	ed_setup(fd)
{
	register char *pp;
	register char *last;
	char *ppmax;
	int myquote = 0;
	int qlen = 1;
	char inquote = 0;
	editb.e_fd = fd;
	/* p_setout(fd); */
#ifdef KSHELL
	last = _sobuf;
#else
	last = editb.e_prbuff;
#endif /* KSHELL */
	pp = editb.e_prompt;
	ppmax = pp+PRSIZE-1;
	*pp++ = '\r';
	{
		register int c;
		while(c= *last++) switch(c)
		{
			case '\r':
				if(pp == (editb.e_prompt+2)) /* quote char */
					myquote = *(pp-1);
				/*FALLTHROUGH*/

			case '\n':
				/* start again */
				editb.e_crlf = YES;
				qlen = 1;
				inquote = 0;
				pp = editb.e_prompt+1;
				break;

			case '\t':
				/* expand tabs */
				while((pp-editb.e_prompt)%TABSIZE) {
					if(pp >= ppmax) break;
					*pp++ = ' ';
				}
				break;

			case BELL:
				/* cut out bells */
				break;

			default:
				if(c==myquote)
				{
					qlen += inquote;
					inquote ^= 1;
				}
				if(pp < ppmax)
				{
					qlen += inquote;
					*pp++ = c;
					if(!inquote && !isprint(c))
						editb.e_crlf = NO;
				}
		}
	}
	editb.e_wsize = editb.e_wmaxsize;
	editb.e_plen = pp - editb.e_prompt - qlen;
	*pp = 0;
	if((editb.e_wsize -= editb.e_plen) < 7)
	{
		register int shift = 7-editb.e_wsize;
		editb.e_wsize = 7;
		pp = editb.e_prompt+1;
		strcpy(pp,pp+shift);
		editb.e_plen -= shift;
		last[-editb.e_plen-2] = '\r';
	}
	/* p_flush(); */
#ifdef KSHELL
	editb.e_outptr = _sobuf;
#else
	editb.e_outptr = workbuffer;
#endif /* KSHELL */
	editb.e_outbase = editb.e_outptr;
	editb.e_outlast = editb.e_outptr + IOBSIZE-3;
}

#ifdef KSHELL
/*
 * look for edit macro named _i
 * if found, puts the macro definition into lookahead buffer and returns 1
 */

ed_macro(fd,i)
register int i;
{
	register char *out;
	struct namnod *np;
	genchar buff[LOOKAHEAD+1];
	if(i != '@')
		macro[1] = i;
	/* undocumented feature, macros of the form <ESC>[c evoke alias __c */
	if(i=='_')
		macro[2] = ed_getchar(fd);
	else
		macro[2] = 0;
	if (isalnum(i)&&(np=nam_search(macro,sh.alias_tree,N_NOSCOPE))&&(out=nam_strval(np)))
	{
#ifdef MULTIBYTE
		/* copy to buff in internal representation */
		int c = out[LOOKAHEAD];
		out[LOOKAHEAD] = 0;
		i = ed_internal(out,buff);
		out[LOOKAHEAD] = c;
#else
		strncpy((char*)buff,out,LOOKAHEAD);
		i = strlen((char*)buff);
#endif /* MULTIBYTE */
		while(i-- > 0)
			ed_ungetchar(buff[i]);
		return(1);
	} 
	return(0);
}
#else
ed_macro(fd)
int fd;
{
	/* stub for later? */
}
#endif	/* KSHELL */

#ifdef KSHELL
/*
 * file name generation for edit modes
 * non-zero exit for error, <0 ring bell
 * don't search back past beginning of the buffer
 * mode is '*' for inline expansion,
 * mode is '\' for filename completion
 * mode is '=' cause files to be listed in select format
 */

ed_expand(outbuff,cur,eol,mode)
char outbuff[];
int *cur;
int *eol;
int mode;
{
	STKPTR staksav = sh.stakbot;
	struct comnod  *comptr = (struct comnod*)stak_alloc(sizeof(struct comnod));
	struct argnod *ap = (struct argnod*)stak_begin();
	register char *out;
	char *begin;
	int addstar;
	int istilde = 0;
	int rval = 0;
	int strip;
	optflag savflags = opt_flags;
#ifdef MULTIBYTE
	{
		register int c = *cur;
		register genchar *cp;
		/* adjust cur */
		cp = (genchar *)outbuff + *cur;
		c = *cp;
		*cp = 0;
		*cur = ed_external((genchar*)outbuff,(char*)sh.stakbot);
		*cp = c;
		*eol = ed_external((genchar*)outbuff,outbuff);
	}
#endif /* MULTIBYTE */
	out = outbuff + *cur;
	comptr->comtyp = COMSCAN;
	comptr->comarg = ap;
	ap->argflag = (A_MAC|A_EXP);
	ap->argnxt.ap = 0;
	{
		register int c;
		register char *ptr = ap->argval;
		int chktilde = 0;
		char *cp;
		if(out>outbuff)
		{
			/* go to beginning of word */
			do
			{
				out--;
				c = *(unsigned char*)out;
			}
			while(out>outbuff && !isqmeta(c));
			/* copy word into arg */
			if(isqmeta(c))
				out++;
		}
		else
			out = outbuff;
		begin = out;
		/* addstar set to zero if * should not be added */
		addstar = '*';
		strip = TRUE;
		/* copy word to arg and do ~ expansion */
		do
		{
			c = *(unsigned char*)out;
			if(isexp(c))
				addstar = 0;
			if ((c == '/') && (addstar == 0))
				strip = FALSE;
			*ptr++ = c;
			if(chktilde==0 && (c==0 || c == '/'))
			{
				chktilde++;
				*out = 0;
				if(cp=sh_tilde(begin))
				{
					istilde++;
					ptr = sh_copy(cp,ap->argval);
					*ptr++ = c;
					if(c==0)
					{
						addstar = 0;
						strip = FALSE;
					}
				}
				*out = c;
			}
			out++;

		} while (c && !isqmeta(c));

		out--;
#ifdef tenex
		if(mode=='\\' || mode=='\033')
			addstar = '*';
#endif /* tenex */
		*(ptr-1) = addstar;
		stak_end(ptr);
	}
	if(mode!='*')
		on_option(MARKDIR);
	{
		register char **com;
		int	 narg;
		register int size;
		VOID (*savfn)();
		savfn = st.intfn;
		com = arg_build(&narg,comptr);
		st.intfn = savfn;
		/*  match? */
		if (*com==0 || (!istilde && narg <= 1 && eq(ap->argval,*com)))
		{
			rval = -1;
			goto done;
		}
		if(mode=='=')
		{
			if (strip)
			{
				register char **ptrcom;
				for(ptrcom=com;*ptrcom;ptrcom++)
					/* trim directory prefix */
					*ptrcom = path_basename(*ptrcom);
			}
			p_setout(ERRIO);
			newline();
			p_list(narg,com);
			p_flush();
			goto done;
		}
		/* see if there is enough room */
		size = *eol - (out-begin);
#ifdef tenex
		if(mode=='\\' || mode=='\033')
		{
			/* just expand until name is unique */
			size += strlen(*com);
		}
		else
#endif
		{
			size += narg;
			{
				char **savcom = com;
				while (*com)
					size += strlen(*com++);
				com = savcom;
			}
		}
		/* see if room for expansion */
		if(outbuff+size >= &outbuff[MAXLINE])
		{
			com[0] = ap->argval;
			com[1] = 0;
		}
		/* save remainder of the buffer */
		strcpy(sh.stakbot,out);
		out = sh_copy(*com++, begin);
#ifdef tenex
		if(mode=='\\' || mode=='\033')
		{
			if(*com==0 && out[-1]!='/')
				*out++ = ' ';
			while (*com && *begin)
				out = overlay(begin,*com++);
			if(*begin==0)
				ed_ringbell(fd);
		}
		else
#endif
			while (*com)
			{
				*out++  = ' ';
				out = sh_copy(*com++,out);
			}
		*cur = (out-outbuff);
		/* restore rest of buffer */
		out = sh_copy(sh.stakbot,out);
		*eol = (out-outbuff);
	}
 done:
	stak_reset(staksav);
	opt_flags = savflags;
#ifdef MULTIBYTE
	{
		register int c;
		/* first re-adjust cur */
		out = outbuff + *cur;
		c = *out;
		*out = 0;
		*cur = ed_internal(outbuff,(genchar*)sh.stakbot);
		*out = c;
		outbuff[*eol+1] = 0;
		*eol = ed_internal(outbuff,(genchar*)outbuff);
	}
#endif /* MULTIBYTE */
	return(rval);
}

#   ifdef tenex
static char *overlay(str,newstr)
register char *str,*newstr;
{
	while(*str && *str == *newstr++)
		str++;
	*str = 0;
	return(str);
}
#   endif
#endif /* KSHELL */

#ifdef KSHELL
/*
 * Enter the fc command on the current history line
 */
ed_fulledit()
{
	register char *cp;
	/* use EDITOR on current command */
	if(editb.e_hline == editb.e_hismax)
	{
		if(editb.e_eol<=0)
			return(BAD);
		editb.e_inbuf[editb.e_eol+1] = 0;
		p_setout(hist_ptr->fixfd);
		p_str((char*)editb.e_inbuf,0);
		st.states |= FIXFLG;
		hist_flush();
	}
	cp = sh_copy(e_runvi, (char*)editb.e_inbuf);
	cp = sh_copy(sh_itos(editb.e_hline), cp);
	editb.e_eol = (unsigned char*)cp - (unsigned char*)editb.e_inbuf;
	return(GOOD);
}
#endif /*KSHELL*/
 

/*
 * routine to perform read from terminal for vi and emacs mode
 */


int 
ed_getchar(fd)
int fd;
{
	register int i;
	register int c;
	register int maxtry = MAXTRY;
	unsigned nchar = READAHEAD; /* number of characters to read at a time */
#ifdef MULTIBYTE
	static int curchar;
	static int cursize;
#endif /* MULTIBYTE */
	char readin[LOOKAHEAD] ;
	if (lookahead)
	{
		c = previous[--lookahead];
		/*** map '\r' to '\n' ***/
		if(c == '\r')
			c = '\n';
		return(c);
	}
	
	ed_flush(fd) ;
	/*
	 * you can't chance read ahead at the end of line
	 * or when the input is a pipe
	 */
#ifdef KSHELL
	if((editb.e_cur>=editb.e_eol) || fnobuff(io_ftable[fildes]))
#else
	if(editb.e_cur>=editb.e_eol)
#endif /* KSHELL */
		nchar = 1;
	/* Set 'i' to indicate read failed, in case intr set */
retry:
	i = -1;
	errno = 0;
	editb.e_inmacro = 0;
	while(slowsig()==0 && maxtry--)
	{
		errno=0;
		if ((i = ee_read(fildes,readin, nchar)) != -1)
			break;
	}
#ifdef MULTIBYTE
	lookahead = maxtry = i;
	i = 0;
	while (i < maxtry)
	{
		c = readin[i++] & STRIP;
	next:
		if(cursize-- > 0)
		{
			curchar = (curchar<<7) | (c&~HIGHBIT);
			if(cursize==0)
			{
				c = curchar;
				goto gotit;
			}
			else if(i>=maxtry)
				goto retry;
			continue;
		}
		else if(curchar = echarset(c))
		{
			cursize = in_csize(curchar);
			if(curchar != 1)
				c = 0;
			curchar <<= 7*(ESS_MAXCHAR-cursize);
			if(c)
				goto next;
			else if(i>=maxtry)
				goto retry;
			continue;
		}
	gotit:
		previous[--lookahead] = c;
#else
	while (i > 0)
	{
		c = readin[--i] & STRIP;
		previous[lookahead++] = c;
#endif /* MULTIBYTE */
#ifndef CBREAK
		if( c == '\0' )
		{
			/*** user break key ***/
			lookahead = 0;
# ifdef KSHELL
			sh_fault(SIGINT);
			longjmp(env, UINTR);
# endif	/* KSHELL */
		}
#endif	/* !CBREAK */
	}
#ifdef MULTIBYTE
	/* shift lookahead buffer if necessary */
	if(lookahead)
	{
		for(i=lookahead;i < maxtry;i++)
			previous[i-lookahead] = previous[i];
	}
	lookahead = maxtry-lookahead;
#endif /* MULTIBYTE */
	if (lookahead > 0)
		return(ed_getchar(fd));
	longjmp(env,(i==0?UEOF:UINTR)); /* What a mess! Give up */
	/* NOTREACHED */
}

void ed_ungetchar(c)
register int c;
{
	if (lookahead < LOOKAHEAD)
		previous[lookahead++] = c;
	return;
}

/*
 * put a character into the output buffer
 */

void	ed_putchar(fd,c)
int fd;
register int c;
{
	register char *dp = editb.e_outptr;
#ifdef MULTIBYTE
	register int d;
	/* check for place holder */
	if(c == MARKER)
		return;
	if(d = icharset(c))
	{
		if(d == 2)
			*dp++ = ESS2;
		else if(d == 3)
			*dp++ = ESS3;
		d = in_csize(d);
		while(--d>0)
			*dp++ = HIGHBIT|(c>>(7*d));
		c |= HIGHBIT;
	}
#endif	/* MULTIBYTE */
	if (c == '_')
	{
		*dp++ = ' ';
		*dp++ = '\b';
	}
	*dp++ = c;
	*dp = '\0';
	if(dp >= editb.e_outlast)
		ed_flush(fd);
	else
		editb.e_outptr = dp;
}

/*
 * copy virtual to physical and return the index for cursor in physical buffer
 */
ed_virt_to_phys(virt,phys,cur,voff,poff)
genchar *virt;
genchar *phys;
int cur;
{
	register genchar *sp = virt;
	register genchar *dp = phys;
	register int c;
	genchar *curp = sp + cur;
	genchar *dpmax = phys+MAXLINE;
	int r;
#ifdef MULTIBYTE
	int d;
#endif /* MULTIBYTE */
	sp += voff;
	dp += poff;
	for(r=poff;c= *sp;sp++)
	{
		if(curp == sp)
			r = dp - phys;
#ifdef MULTIBYTE
		d = out_csize(icharset(c));
		if(d>1)
		{
			/* multiple width character put in place holders */
			*dp++ = c;
			while(--d >0)
				*dp++ = MARKER;
			/* in vi mode the cursor is at the last character */
			if(dp>=dpmax)
				break;
			continue;
		}
		else
#endif	/* MULTIBYTE */
		if(!isprint(c))
		{
			if(c=='\t')
			{
				c = dp-phys;
				if(is_option(EDITVI))
					c += editb.e_plen;
				c = TABSIZE - c%TABSIZE;
				while(--c>0)
					*dp++ = ' ';
				c = ' ';
			}
			else
			{
				*dp++ = '^';
				c ^= TO_PRINT;
			}
			/* in vi mode the cursor is at the last character */
			if(curp == sp && is_option(EDITVI))
				r = dp - phys;
		}
		*dp++ = c;
		if(dp>=dpmax)
			break;
	}
	*dp = 0;
	return(r);
}

#ifdef MULTIBYTE
/*
 * convert external representation <src> to an array of genchars <dest>
 * <src> and <dest> can be the same
 * returns number of chars in dest
 */

int	ed_internal(src,dest)
register unsigned char *src;
genchar *dest;
{
	register int c;
	register genchar *dp = dest;
	register int d;
	register int size;
	if((unsigned char*)dest == src)
	{
		genchar buffer[MAXLINE];
		c = ed_internal(src,buffer);
		ed_gencpy(dp,buffer);
		return(c);
	}
	while(c = *src++)
	{
		if(size = echarset(c))
		{
			d = (size==1?c:0);
			c = size;
			size = in_csize(c);
			c <<= 7*(ESS_MAXCHAR-size);
			if(d)
			{
				size--;
				c = (c<<7) | (d&~HIGHBIT);
			}
			while(size-- >0)
				c = (c<<7) | ((*src++)&~HIGHBIT);
		}
		*dp++ = c;
	}
	*dp = 0;
	return(dp-dest);
}

/*
 * convert internal representation <src> into character array <dest>.
 * The <src> and <dest> may be the same.
 * returns number of chars in dest.
 */

int	ed_external(src,dest)
genchar *src;
char *dest;
{
	register int c;
	register char *dp = dest;
	register int d;
	char *dpmax = dp+sizeof(genchar)*MAXLINE-2;
	if((char*)src == dp)
	{
		char buffer[MAXLINE*sizeof(genchar)];
		c = ed_external(src,buffer);
		strcpy(dest,buffer);
		return(c);
	}
	while((c = *src++) && dp<dpmax)
	{
		if(d = icharset(c))
		{
			if(d == 2)
				*dp++ = ESS2;
			else if(d == 3)
				*dp++ = ESS3;
			d = in_csize(d);
			while(--d>0)
				*dp++ = HIGHBIT|(c>>(7*d));
			c |= HIGHBIT;
		}
		*dp++ = c;
	}
	*dp = 0;
	return(dp-dest);
}

/*
 * copy <sp> to <dp>
 */

int	ed_gencpy(dp,sp)
register genchar *dp;
register genchar *sp;
{
	while(*dp++ = *sp++);
}

/*
 * copy at most <n> items from <sp> to <dp>
 */

int	ed_genncpy(dp,sp, n)
register genchar *dp;
register genchar *sp;
register int n;
{
	while(n-->0 && (*dp++ = *sp++));
}

/*
 * find the string length of <str>
 */

int	ed_genlen(str)
register genchar *str;
{
	register genchar *sp = str;
	while(*sp++);
	return(sp-str-1);
}
#endif /* MULTIBYTE */
#endif /* ESH || VSH */

#ifdef MULTIBYTE
/*
 * set the multibyte widths
 * format of string is x1[:y1][,x2[:y2][,x3[:y3]]]
 * returns 1 if string in not in this format, 0 otherwise.
 */

extern char int_charsize[];
ed_setwidth(string)
char *string;
{
	register int indx = 0;
	register int state = 0;
	register int c;
	register int n = 0;
	static char widths[6] = {1,1};
	while(1) switch(c = *string++)
	{
		case ':':
			if(state!=1)
				return(1);
			state++;
			/* fall through */

		case 0:
		case ',':
			if(state==0)
				return(1);
			widths[indx++] = n;
			if(state==1)
				widths[indx++] = n;
			if(c==0)
			{
				for(n=1;n<= 3;n++)
				{
					int_charsize[n] = widths[c++];
					int_charsize[n+4] = widths[c++];
				}
				return(0);
			}
			else if(c==',')
				state = 0;
			n = 0;
			break;

		case '0': case '1': case '2': case '3': case '4':
			if(state&1)
				return(1);
			n = c - '0';
			state++;
			break;
			
		default:
			return(1);
	}
	/* NOTREACHED */
}
#endif /* MULTIBYTE */

#ifdef future
/*
 * returns 1 when <n> bytes starting at <a> and <b> are equal
 */
static int compare(a,b,n)
register char *a;
register char *b;
register int n;
{
	while(n-->0)
	{
		if(*a++ != *b++)
			return(0);
	}
	return(1);
}
#endif

/* The stuff below here is not properly part of ksh88 edit.c, but fits
 * better here than anywhere else.
 * /

/*
 * enable edit mode <mode>; figure out what the environment is.
 */

int	set_edit(mode)
int mode;
{
	register char *sp;

	hist_open();
	sp  = getenv("VISUAL");
	if(sp==NULL)
		sp = getenv("EDITOR");
	if(sp)
	{
		if(strrchr(sp,'/'))
			sp = strrchr(sp,'/')+1;
		if(strcmp(sp,"vi") == 0) {
			opt_flags = EDITVI;
			if (mode != 0) 
				opt_flags |= VIRAW;
		}
		else if(strcmp(sp,"emacs")==0)
			opt_flags = EMACS;
		else if(strcmp(sp,"gmacs")==0)
			opt_flags = GMACS;
	}
}

int e_read(fd, buf, len)
int fd;
char *buf;
int len;
{
	register int r;
	register int flag;

	if(fc_fix)
	{
		register struct fixcmd *fp = fc_fix;
		editb.e_hismax = fp->fixind;
		editb.e_hloff = 0;
		editb.e_hismin = fp->fixind-fp->fixmax;
		if(editb.e_hismin<0)
			editb.e_hismin = 0;
	}
	else
	{
		editb.e_hismax = editb.e_hismin = editb.e_hloff = 0;
	}
	editb.e_hline = editb.e_hismax;
	editb.e_wmaxsize = editb.e_wsize = ed_window(fd)-2;
	editb.e_crlf = YES;

	if (opt_flags & (EDITVI|VIRAW))
		r = vi_read(fd, buf, len);
	else if (opt_flags & (EMACS|GMACS))
		r = emacs_read(fd, buf, len);
	else {
		char prompt[PRSIZE+2];

		editb.e_prompt = prompt;
		r = ee_read(fd,buf,len);
	}
	if(fc_fix && (opt_flags&NOHIST)==0 && r>0)
	{
		/* write and flush history */
		int c = buf[r];
		buf[r] = 0;
		hist_eof();
		fputs(buf,fc_fix->fixfd);
		hist_flush();
		buf[r] = c;
	}
	return(r);
}

/*
 * move the file number on stream fd to unit fb
 */

FILE *hist_rename(fd, fb)
register FILE *fd;
register int 	fb;
{
	register int fa = fileno(fd);
#ifdef BSD
	dup(fa|DUPFLG, fb);
	ioctl(fb, FIOCLEX, 0);
#else	/*	TS lacks two-arg dup, ioctl	*/
	if(fa >= 0)
	{
		close(fb);
		fcntl(fa,0,fb); /* normal dup */
		fcntl(fb,2,1);	/* autoclose for fb */
	}
#endif	/* BSD */

#ifdef old_HP
	fd->__file = fb;
#else
	/*
	 * WARNING: -- This code is NOT PORTABLE and only works on
	 *             HP-UX
	 */
	{
	    unsigned short x = fb;

	    fd->__fileH = ((unsigned char *)(&x))[0];
	    fd->__fileL = ((unsigned char *)(&x))[1];
	}
#endif

	return(fd);
}

p_flush(fd)
{
	write(fd,editb.e_outbase,editb.e_outptr-editb.e_outbase);
}