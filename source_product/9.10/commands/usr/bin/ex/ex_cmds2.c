/* @(#) $Revision: 70.5 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifdef ED1000
#include "ex_sm.h"
#endif ED1000

extern bool	pflag, nflag;		/* mjm: extern; also in ex_cmds.c */
extern int	poffset;		/* mjm: extern; also in ex_cmds.c */

#ifndef NONLS8 /* User messages */
# define	NL_SETN	4	/* set number */
# include	<msgbuf.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Subroutines for major command loop.
 */

/*
 * Is there a single letter indicating a named buffer next?
 */
cmdreg()
{
	register int c = 0;
	register int wh = skipwh();

#ifndef NONLS8 /* 8bit integrity */
	/*
	 * Buffers have been allocated for ONLY the ASCII letters, not all
	 * of the 8 or 16-bit characters that might be defined as alpha.
	 */
   	if (wh && (peekchar() >= IS_MACRO_LOW_BOUND) && isalpha(peekchar() & TRIM) && isascii(peekchar() & TRIM))
#else NONLS8
	if (wh && (peekchar() >= IS_MACRO_LOW_BOUND) && isalpha(peekchar()))
#endif NONLS8

		c = getchar();
	return (c);
}

/*
 * Tell whether the character ends a command
 */
endcmd(ch)
	int ch;
{
	switch (ch) {
	
	case '\n':
	case EOF:
		endline = 1;
		return (1);
	
	case '|':
	case '"':
		endline = 0;
		return (1);
	}
	return (0);
}

/*
 * Insist on the end of the command.
 */
eol()
{

	if (!skipend())
		error((nl_msg(1, "Extra chars|Extra characters at end of command")));
	ignnEOF();
}

/*
 * Print out the message in the error message file at str,
 * with i an integer argument to printf.
 */
/*VARARGS2*/
error(str, i)
#ifdef lint
	register char *str;
#else
	register int str;
#endif
	int i;
{

	error0();
	merror(str, i);
	if (writing) {
		serror((nl_msg(2, " [Warning - %s is incomplete]\n                                   \nYour edited changes will be lost if you do not complete a successful\nwrite command before exiting.  If you were writing out to your original\nfile, the previous contents of that file have been destroyed.  Contact\nyour System Administrator BEFORE exiting if you need assistance.\n")), file);
		writing = 0;
	}
	error1(str);
}

/*
 * Rewind the argument list.
 */
erewind()
{

	argc = argc0;
	argv = argv0;
	args = args0;
	if (argc > 1 && !hush && cur_term) {

#ifndef NONLS8 /* User messages */
		printf(mesg((nl_msg(3, "%d files|%d files to edit"))), argc);
#else NONLS8
		printf(mesg("%d files@to edit"), argc);
#endif NONLS8

		if (inopen)
			putchar(' ');
		else
			putNFL();
	}
}

/*
 * Guts of the pre-printing error processing.
 * If in visual and catching errors, then we dont mung up the internals,
 * just fixing up the echo area for the print.
 * Otherwise we reset a number of externals, and discard unused input.
 */
error0()
{

#if defined NLS || defined NLS16
	RL_OKEY
#endif
	if (laste) {
#ifdef VMUNIX
		tlaste();
#endif
		laste = 0;
		sync();
	}
	if (vcatch) {
		if (splitw == 0)
			fixech();
		if (!enter_standout_mode || !exit_standout_mode)
			dingdong();
		return;
	}
	if (input) {
		input = strend(input) - 1;
		if (*input == '\n')
			setlastchar('\n');
		input = 0;
	}
	setoutt();
	flush();
	resetflav();
	if (!enter_standout_mode || !exit_standout_mode)
		dingdong();
	if (inopen) {
		/*
		 * We are coming out of open/visual ungracefully.
		 * Restore columns, undo, and fix tty mode.
		 */
		columns = OCOLUMNS;
		undvis();
		ostop(normf);
		/* ostop should be doing this
		putpad(cursor_normal);
		putpad(key_eol);
		*/
		putnl();
	}
	inopen = 0;
	holdcm = 0;
}

/*
 * Post error printing processing.
 * Close the i/o file if left open.
 * If catching in visual then throw to the visual catch,
 * else if a child after a fork, then exit.
 * Otherwise, in the normal command mode error case,
 * finish state reset, and throw to top.
 */
error1(str)
	char *str;
{
	bool die;

#if defined NLS || defined NLS16
	RL_OSCREEN
#endif
	if (io > 0) {
		close(io);
		io = -1;
	}
	die = (getpid() != ppid);	/* Only children die */
	inappend = inglobal = 0;
#ifndef	NLS16
	globp = vglobp = vmacp = 0;
#else	NLS16
	globp = 0; vglobp = vmacp = 0;
#endif	NLS16
	if (vcatch && !die) {
		inopen = 1;
		vcatch = 0;
		if (str)
			noonl();
		fixol();
		longjmp(vreslab,1);
	}

#ifdef ED1000
	if ( insmcmd) {
		insmcmd = 0;
		if (str) noonl();
			flush();
		outline = destline = 0;
		Outchar = termchar;
		longjmp(sm_reslab,1);
	}
#endif ED1000

	if (str && !vcatch)
		putNFL();
	if (die) {
#ifndef ED1000
		flush();		/* Any last words? */
#endif ED1000
		exit(1);
	}
	lseek(0, 0L, 2);
	if (inglobal)
		setlastchar('\n');
	while (lastchar() != '\n' && lastchar() != EOF)
		ignchar();
#ifndef	NLS16
	ungetchar(0);
#else	NLS16
	/*
	 * It is necessary to clear not only peekc but also secondchar.
	 */
	clrpeekc();
#endif	NLS16
	endline = 1;

	/*
	 * UCSqm00778: this is to prevent vi gets into an infinite loop
	 * when it's calling parent is killed (therefore, vi is inherited
	 * by init). Without this checking, the reset() (longjmp(resetlab,1)) 
	 * just simply jumps to the code that try to read from stdin again, 
	 * and of course, it'll fail and come back to this routine...
	 */
	if (errno == EIO && getppid() == 1) {
		flush();	/* flush out any last words */
		exit(1);
	}
	reset();
}

fixol()
{
	if (Outchar != vputchar) {
		flush();
		if (state == ONEOPEN || state == HARDOPEN)
			outline = destline = 0;
		Outchar = vputchar;
		vcontin(1);
	} else {
		if (destcol)
			vclreol();
		vclean();
	}
}

/*
 * Does an ! character follow in the command stream?
 */
exclam()
{

	if (peekchar() == '!') {
		ignchar();
		return (1);
	}
	return (0);
}

/*
 * Make an argument list for e.g. next.
 */
makargs()
{

	glob(&frob);
	argc0 = frob.argc0;
	argv0 = frob.argv;
	args0 = argv0[0];
	erewind();
}

/*
 * Advance to next file in argument list.
 */
next()
{
	extern short isalt;	/* defined in ex_io.c */

	if (argc == 0)

#ifndef NONLS8 /* User messages */
		error((nl_msg(4, "No more files|No more files to edit")));
#else NONLS8
		error("No more files@to edit");
#endif NONLS8

	morargc = argc;
	isalt = (strcmp(altfile, args)==0) + 1;
	if (savedfile[0])
		CP(altfile, savedfile);
	CP(savedfile, args);
	argc--;
	args = argv ? *++argv : strend(args) + 1;
}

/*
 * Eat trailing flags and offsets after a command,
 * saving for possible later post-command prints.
 */
donewline()
{
	register int c;

	resetflav();
	for (;;) {
		c = getchar();
		switch (c) {

		case '^':
		case '-':
			poffset--;
			break;

		case '+':
			poffset++;
			break;

		case 'l':
			listf++;
			break;

		case '#':
			nflag++;
			break;

		case 'p':
			listf = 0;
			break;

		case ' ':
		case '\t':
			continue;

		case '"':
			comment();
			setflav();
			return;

		default:
			if (!endcmd(c))
serror((nl_msg(5, "Extra chars|Extra characters at end of \"%s\" command")), Command);
			if (c == EOF)
				ungetchar(c);
			setflav();
			return;
		}
		pflag++;
	}
}

/*
 * Before quit or respec of arg list, check that there are
 * no more files in the arg list.
 */
nomore()
{

	if (argc == 0 || morargc == argc)
		return;
	morargc = argc;

#ifndef NONLS8 /* User messages */
	if (argc == 1)
	    serror((nl_msg(6, "%d more file|%d more file to edit")), argc);
	else
	    serror((nl_msg(6, "%d more files|%d more files to edit")), argc);
#else NONLS8
	merror("%d more file", argc);
	serror("%s@to edit", plural((long) argc));
#endif NONLS8

}

/*
 * Before edit of new file check that either an ! follows
 * or the file has not been changed.
 */
quickly()
{

	if (exclam())
		return (1);
	if (chng && dol > zero) {
/*
		chng = 0;
*/
		xchng = 0;

#ifndef NONLS8 /* User messages */
		error((nl_msg(7, "No write|No write since last change (:%s! overrides)")), Command);
#else NONLS8
		error("No write@since last change (:%s! overrides)", Command);
#endif NONLS8

	}
	return (0);
}

/*
 * Reset the flavor of the output to print mode with no numbering.
 */
resetflav()
{

	if (inopen)
		return;
	listf = 0;
	nflag = 0;
	pflag = 0;
	poffset = 0;
	setflav();
}

/*
 * Print an error message with a %s type argument to printf.
 * Message text comes from error message file.
 */
serror(str, cp)
#ifdef lint
	register char *str;
#else
	register int str;
#endif
	char *cp;
{

	error0();
	smerror(str, cp);
	error1(str);
}

/*
 * Set the flavor of the output based on the flags given
 * and the number and list options to either number or not number lines
 * and either use normally decoded (ARPAnet standard) characters or list mode,
 * where end of lines are marked and tabs print as ^I.
 */
setflav()
{

	if (inopen)
		return;
	setnumb(nflag || value(NUMBER));
	setlist(listf || value(LIST));
	setoutt();
}

/*
 * Skip white space and tell whether command ends then.
 */
skipend()
{

	pastwh();
	return (endcmd(peekchar()) && peekchar() != '"');
}

/*
 * Set the command name for non-word commands.
 */
tailspec(c)
	int c;
{
	static char foocmd[2];

	foocmd[0] = c;
	Command = foocmd;
}

/*
 * Try to read off the rest of the command word.
 * If alphabetics follow, then this is not the command we seek.
 */
tail(comm)
	char *comm;
{

	tailprim(comm, 1, 0);
}

tail2of(comm)
	char *comm;
{

	tailprim(comm, 2, 0);
}

char	tcommand[20];

tailprim(comm, i, notinvis)
	register char *comm;
	int i;
	bool notinvis;
{
	register char *cp;
	register int c;

	Command = comm;
	for (cp = tcommand; i > 0; i--)
		*cp++ = *comm++;
	while (*comm && peekchar() == *comm)
		*cp++ = getchar(), comm++;
	c = peekchar();
#ifndef	NLS16
# ifndef NONLS8 /* Character set features */
	if (notinvis || ((c >= IS_MACRO_LOW_BOUND) && isalpha(c & TRIM))) {
# else NONLS8
	if (notinvis || ((c >= IS_MACRO_LOW_BOUND) && isalpha(c))) {
# endif NONLS8
#else	NLS16
	if (notinvis || IS_KANJI(c) || ((c >= IS_MACRO_LOW_BOUND) && isalpha(c & TRIM))) {
#endif	NLS16
		/*
		 * Of the trailing lp funny business, only dl and dp
		 * survive the move from ed to ex.
		 */
		if (tcommand[0] == 'd' && any(c, "lp"))
			goto ret;
		if (tcommand[0] == 's' && any(c, "gcr"))
			goto ret;
#ifndef	NLS16
# ifndef NONLS8 /* Character set features */
		while (cp < &tcommand[19] && (peekchar() >= IS_MACRO_LOW_BOUND) && isalpha(peekchar() & TRIM))
# else NONLS8
		while (cp < &tcommand[19] && (peekchar() >= IS_MACRO_LOW_BOUND) && isalpha(peekchar()))
# endif NONLS8
			*cp++ = getchar();
#else	NLS16
		/* regard 16-bit character as a possible command character */
		while (cp < &tcommand[19] && (IS_KANJI(peekchar()) || 
		( (peekchar() >= IS_MACRO_LOW_BOUND) && isalpha(peekchar() & TRIM))))
			*cp++ = c = getchar();
		if (IS_FIRST(c))
			cp--;	/* erase the 1st byte at the last of tcommand */
#endif	NLS16
		*cp = 0;
		if (notinvis)
			serror((nl_msg(8, "What?|%s: No such command from open/visual")), tcommand);
		else
			serror((nl_msg(9, "What?|%s: Not an editor command")), tcommand);
	}
ret:
	*cp = 0;
}

/*
 * Continue after a : command from open/visual.
 */
vcontin(ask)
	bool ask;
{

#ifdef SIGWINCH
        struct winsize win;
#endif SIGWINCH

	if (vcnt > 0)
		vcnt = -vcnt;
	if (inopen) {
		if (state != VISUAL) {
			/*
			 * We don't know what a shell command may have left on
			 * the screen, so we move the cursor to the right place
			 * and then put out a newline.  But this makes an extra
			 * blank line most of the time so we only do it for :sh
			 * since the prompt gets left on the screen.
			 *
			 * BUG: :!echo longer than current line \\c
			 * will screw it up, but be reasonable!
			 */
			if (state == CRTOPEN) {
				termreset();
				vgoto(WECHO, 0);
			}
			if (!ask) {
				putch('\r');
				putch('\n');
			}
			return;
		}
		if (ask) {
			merror((nl_msg(10, "[Hit return to continue] ")));
			flush();
		}
#ifndef CBREAK
		vraw();
#endif
		if (ask) {
#ifdef notdef
			/*
			 * Gobble ^Q/^S since the tty driver should be eating
			 * them (as far as the user can see)
			 */
			while (peekkey() == CTRL(Q) || peekkey() == CTRL(S))
				ignore(getkey());
#endif
			if(getkey() == ':') {
				/* Ugh. Extra newlines, but no other way */
				putch('\n');
				outline = WECHO;
				ungetkey(':');
			}
		}
		vclrech(1);
		if (Peekkey != ':') {
			fixterm();
			putpad(enter_ca_mode);
			tostart();
		}

#ifdef SIGWINCH
		/* FSDlj09637: restore window size and signal handler if needed */
		if (ign_winch) {
                	if (ioctl(0, TIOCGWINSZ, &win) >= 0)
               	    	if (win.ws_row != save_win.ws_row ||
                        	win.ws_col != save_win.ws_col)
                            	winch();
			ign_winch = 0;
                	signal(SIGWINCH, winch);
		}
#endif SIGWINCH

	}
}

/*
 * Put out a newline (before a shell escape)
 * if in open/visual.
 */
vnfl()
{

	if (inopen) {
		if (state != VISUAL && state != CRTOPEN && destline <= WECHO)
			vclean();
		else
			vmoveitup(1, 0);
		vgoto(WECHO, 0);
#if defined NLS16 && defined EUC
		vclrbyte(vtube[WECHO], BTOCRATIO * WCOLS);
#else
		vclrbyte(vtube[WECHO], WCOLS);
#endif
		tostop();
		/* replaced by the ostop above
		putpad(cursor_normal);
		putpad(key_eol);
		*/
	}
	flush();
}
