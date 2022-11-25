/* @(#) $Revision: 70.4 $ */   
/* Copyright (c) 1981 Regents of the University of California */
/* (c) Copyright Toshiba Corp. 1983,1984 */
#include "ex.h"
#include "ex_re.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifdef ED1000
#include "ex_sm.h"
int	esc_seq;
#endif ED1000

#ifndef NONLS8 /* User messages */
# define	NL_SETN	10	/* set number */
# include	<msgbuf.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

#if defined NLS || defined NLS16
# include <nl_ctype.h>
#endif

/*
 * Random routines, in alphabetical order.
 */

any(c, s)
	int c;
	register char *s;
{
	register int x;

#if defined NLS || defined NLS16
	while (x = *s++ & TRIM)
#else
	while (x = *s++)
#endif

#ifndef	NLS16
		if (x == c)
#else
		/* prevent mistaking 2nd byte of 16-bit character for a ANK. */
		if (FIRSTof2((*(s-1))&TRIM) && SECONDof2((*s)&TRIM))
			s++;
		else if (x == c)
#endif

			return (1);
	return (0);
}

backtab(i)
	register int i;
{
	register int j;

	j = i % value(SHIFTWIDTH);
	if (j == 0)
		j = value(SHIFTWIDTH);
	i -= j;
	if (i < 0)
		i = 0;
	return (i);
}

change()
{

	tchng++;
	chng = tchng;
	real_empty = 0;		/* if line was fake before, it is real now */
}

/*
 * Column returns the number of
 * columns occupied by printing the
 * characters through position cp of the
 * current line.
 */
column(cp)
	register CHAR *cp;
{

	if (cp == 0)
		cp = &linebuf[LBSIZE - 2];
	return (qcolumn(cp, (CHAR *) 0));
}

/*
 * Ignore a comment to the end of the line.
 * This routine eats the trailing newline so don't call donewline().
 */
comment()
{
	register int c;

	do {
		c = getchar();
	} while (c != '\n' && c != EOF);
	if (c == EOF)
		ungetchar(c);
}

Copy(to, from, size)
	register char *from, *to;
	register int size;
{

	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

#ifndef NONLS8 /* 8bit integrity */
copys(to, from, size)
	register short *from, *to;
	register int size;
{
	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}
#endif NONLS8

copyw(to, from, size)
	register line *from, *to;
	register int size;
{

	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

copywR(to, from, size)
	register line *from, *to;
	register int size;
{

	while (--size >= 0)
		to[size] = from[size];
}

ctlof(c)
	int c;
{

#ifndef NONLS8 /* 8bit integrity */
	return (c == DELETE ? '?' : c == 0377 ? '/' : c == 0237 ? '>' : c&0200 ? c&0177 | ('a' - 1) : c | ('A' - 1));
#else NONLS8
	return (c == TRIM ? '?' : c | ('A' - 1));
#endif NONLS8

}

dingdong()
{

	if (flash_screen && value(FLASH))
		putpad(flash_screen);
	else if (value(ERRORBELLS))
		putpad(bell);
}

fixindent(indent)
	int indent;
{
	register int i;
	register CHAR *cp;

	i = whitecnt(genbuf);
	cp = vpastwh(genbuf);
	if (*cp == 0 && i == indent && linebuf[0] == 0) {
		genbuf[0] = 0;
		return (i);
	}

#ifndef	NLS16
	CP(genindent(i), cp);
#else
	STRCPY(genindent(i), cp);
#endif

	return (i);
}

filioerr(cp)
	char *cp;
{
	register int oerrno = errno;

	lprintf("\"%s\"", cp);
	errno = oerrno;
	syserror(1);
}

CHAR *
genindent(indent)
	register int indent;
{
	register CHAR *cp;

	for (cp = genbuf; indent >= value(TABSTOP); indent -= value(TABSTOP))
		*cp++ = '\t';
	for (; indent > 0; indent--)
#if defined NLS || defined NLS16
		*cp++ = right_to_left ? alt_space : ' ';
#else
		*cp++ = ' ';
#endif
	return (cp);
}

getDOT()
{

	getline(*dot);
}

line *
getmark(c)
	register int c;
{
	register line *addr;
	
	for (addr = one; addr <= dol; addr++)
		if (names[c - 'a'] == (*addr &~ 01)) {
			return (addr);
		}
	return (0);
}

getn(cp)
	register char *cp;
{
	register int i = 0;


#ifndef NONLS8 /* Character set features */
	while ((*cp >= IS_MACRO_LOW_BOUND) && (isdigit(*cp & TRIM)))
#else NONLS8
	while ((*cp >= IS_MACRO_LOW_BOUND) && (isdigit(*cp)))
#endif NONLS8

		i = i * 10 + *cp++ - '0';
	if (*cp)
		return (0);
	return (i);
}

ignnEOF()
{
	register int c = getchar();

	if (c == EOF)
		ungetchar(c);
	else if (c=='"')
		comment();
}

iswhite(c)
	int c;
{

#if defined NLS || defined NLS16
	return (c == ' ' || c == '\t' || c == alt_space);
#else
	return (c == ' ' || c == '\t');
#endif
}

junk(c)
	register int c;
{

	if (c && !value(BEAUTIFY))
		return (0);

#ifndef NONLS8 /* 8bit integrity */
	if (c >= ' ' && c != DELETE)
#else NONLS8
	if (c >= ' ' && c != TRIM)
#endif NONLS8

		return (0);
	switch (c) {

	case '\t':
	case '\n':
	case '\f':
		return (0);

	default:
		return (1);
	}
}

killed()
{

	killcnt(addr2 - addr1 + 1);
}

killcnt(cnt)
	register int cnt;
{

	if (inopen) {
		notecnt = cnt;
		notenam = notesgn = "";
		return;
	}
	if (!notable(cnt))
		return;
	printf((nl_msg(1, "%d lines")), cnt);
	if (value(TERSE) == 0) {
		if (strcmp(Command, "copy") == 0)
			printf(" %s", "copied");
		else {
			printf(" %c%s", Command[0] | ' ', Command + 1);
			if (Command[strlen(Command) - 1] != 'e')
				putchar('e');
			putchar('d');
		}
	}
	putNFL();
}

lineno(a)
	line *a;
{

	return (a - zero);
}

lineDOL()
{

	return (lineno(dol));
}

lineDOT()
{

	return (lineno(dot));
}

markDOT()
{

	markpr(dot);
}

markpr(which)
	line *which;
{

	if ((inglobal == 0 || inopen) && which <= endcore) {
		names['z'-'a'+1] = *which & ~01;
		if (inopen)
			ncols['z'-'a'+1] = cursor;
	}
}

markreg(c)
	register int c;
{
	if (c == '\'' || c == '`')
		return ('z' + 1);
	if (c >= 'a' && c <= 'z')
		return (c);
	return (0);
}

/*
 * Mesg decodes the terse/verbose strings. Thus
 *	'xxx@yyy' -> 'xxx' if terse, else 'xxx yyy'
 *	'xxx|yyy' -> 'xxx' if terse, else 'yyy'
 * All others map to themselves.
 */
char *
mesg(str)
	register char *str;
{
	register char *cp;

#ifndef	NLS16
	str = strcpy(genbuf, str);
#else
	str = strcpy(GENBUF, str);
#endif

	for (cp = str; *cp; cp++)

#ifdef	NLS16
		/* prevent mistaking 2nd byte of 16-bit character for a ANK. */
		if (FIRSTof2((*cp)&TRIM) && SECONDof2((*(cp+1))&TRIM))
			cp++;
		else
#endif
		
		switch (*cp) {

#ifdef NONLS8 /* User messages */
		case '@':
			if (value(TERSE))
				*cp = 0;
			else
				*cp = ' ';
			break;
#endif NONLS8

		case '|':
			if (value(TERSE) == 0)
				return (cp + 1);
			*cp = 0;
			break;
		}
	return (str);
}

/*VARARGS2*/
merror(seekpt, i)
#ifdef VMUNIX
	char *seekpt;
#else
# ifdef lint
	char *seekpt;
# else
	int seekpt;
# endif
#endif
	int i;
{

#ifndef	NLS16
	register char *cp = linebuf;
#else
	register char *cp = LINEBUF;
#endif

	if (seekpt == 0)
		return;
	merror1(seekpt);
	if (*cp == '\n')
		putnl(), cp++;
	if (inopen > 0 && clr_eol)
		vclreol();
	if (enter_standout_mode && exit_standout_mode)
		putpad(enter_standout_mode);
#if defined NLS || defined NLS16
	RL_OKEY
#endif
	printf(mesg(cp), i);
	if (enter_standout_mode && exit_standout_mode)
		putpad(exit_standout_mode);
#if defined NLS || defined NLS16
	RL_OSCREEN
#endif
}

merror1(seekpt)
#ifdef VMUNIX
	char *seekpt;
#else
# ifdef lint
	char *seekpt;
# else
	int seekpt;
# endif
#endif
{

#ifdef VMUNIX
#ifndef	NLS16
	strcpy(linebuf, seekpt);
#else
	strcpy(LINEBUF, seekpt);
#endif
#else
	lseek(erfile, (long) seekpt, 0);
#ifndef	NLS16
	if (read(erfile, linebuf, 128) < 2)
		CP(linebuf, "ERROR");
#else
	if (read(erfile, LINEBUF, 128) < 2)
		CP(LINEBUF, "ERROR");
#endif
#endif
}

#define MAXDATA (56*1024)
morelines()
{
	register char *end;

	if ((int) sbrk(1024 * sizeof (line)) == -1) {
		if (endcore >= (line *) MAXDATA)
			return -1;
		end = (char *) MAXDATA;
		/*
		 * Ask for end+2 sice we want end to be the last used location.
		 */
		while (brk(end+2) == -1)
			end -= 64;
		if (end <= (char *) endcore)
			return -1;
		endcore = (line *) end;
	} else {
		endcore += 1024;
	}
	return (0);
}

nonzero()
{

	if (addr1 == zero) {
		notempty();

#ifndef NONLS8 /* User messages */
		error((nl_msg(2, "Nonzero address required|Nonzero address required on this command")));
#else NONLS8
		error("Nonzero address required@on this command");
#endif NONLS8

	}
}

notable(i)
	int i;
{

	return (hush == 0 && !inglobal && i > value(REPORT));
}


notempty()
{

	if (dol == zero)

#ifndef NONLS8 /* User messages */
		error((nl_msg(3, "No lines|No lines in the buffer")));
#else NONLS8
		error("No lines@in the buffer");
#endif NONLS8

}


netchHAD(cnt)
	int cnt;
{

	netchange(lineDOL() - cnt);
}

netchange(i)
	register int i;
{
	register char *cp;

	if (i > 0)
		notesgn = cp = "more ";
	else
		notesgn = cp = "fewer ", i = -i;
	if (inopen) {
		notecnt = i;
		notenam = "";
		return;
	}
	if (!notable(i))
		return;
#if defined NLS || defined NLS16
	RL_OKEY
#endif
#ifndef NONLS8 /* User messages */
	if (notesgn[0] == 'm')
		printf(mesg((nl_msg(4, "%d more lines|%d more lines in file after %s"))), i, Command);
	else
		printf(mesg((nl_msg(5, "%d fewer lines|%d fewer lines in file after %s"))), i, Command);
#else NONLS8
	printf(mesg("%d %slines@in file after %s"), i, cp, Command);
#endif NONLS8
#if defined NLS || defined NLS16
	RL_OSCREEN
#endif

	putNFL();
}

putmark(addr)
	line *addr;
{

	putmk1(addr, putline());
}

putmk1(addr, n)
	register line *addr;
	int n;
{
	register line *markp;
	register oldglobmk;

	oldglobmk = *addr & 1;
	*addr &= ~1;
	for (markp = (anymarks ? names : &names['z'-'a'+1]);
	  markp <= &names['z'-'a'+1]; markp++)
		if (*markp == *addr)
			*markp = n;
	*addr = n | oldglobmk;
}

char *
plural(i)
	long i;
{

	return (i == 1 ? "" : "s");
}

int	qcount();
short	vcntcol;

qcolumn(lim, gp)
	register CHAR *lim, *gp;
{
	register int x;
	int (*OO)();

	OO = Outchar;
	Outchar = qcount;
	vcntcol = 0;
#ifdef ED1000
	esc_seq = 0;
#endif ED1000
	if (lim != NULL)
		x = lim[1], lim[1] = 0;
	pline(0);
	if (lim != NULL)
		lim[1] = x;
	if (gp)
		while (*gp)
			putchar(*gp++);
	Outchar = OO;
	return (vcntcol);
}

int
qcount(c)
	int c;
{
#if defined NLS16 && defined EUC
static int cw ;	/* column width of the character */
int rem_col ;	/* the number of column remaining in the line	*/
#endif

#ifdef ED1000
	if (c == '\t'){
		if (insm && value(DISPLAY_FNTS)) {
			vcntcol++;
		}else{
			vcntcol += value(TABSTOP) - vcntcol % value(TABSTOP);
		}
		return;
	}
	if (insm && !value(DISPLAY_FNTS)){
		if (esc_seq){
			if ((c >= IS_MACRO_LOW_BOUND) && (isupper(c))){
				esc_seq = 0;
			}
			return;
		}
		if ((c < ' ') && (c != 012)){
			if (c == 033){
				esc_seq = 1;
			}
			return;
		}
	}
#else
	int num_offset;

	/*************
	* FSDlj06791
	* If :set nu has been activated, all text on the screen is moved over 
	* 8 spaces. vcntcol comes into this routine with the 8 spaces accounted
	* for, but this messes up the first tab when tabstop is not
	* a factor of 8.  The solution is to subtract out the number of
	* spaces temporarily if line numbering has been set.
	**************/
	if (c == '\t') {
	    	if (value(NUMBER)) {
			vcntcol = vcntcol - NUMBER_INDENT;
			num_offset = NUMBER_INDENT;
	    	} else 
			num_offset = 0;

		/* DTS: FSDlj07422
		 * To calculate the advancement of a tab, need to put in
		 * consideration of the line becomes longer than the screen
		 * width.
		 */
		vcntcol += value(TABSTOP) - ((vcntcol % columns) % value(TABSTOP)) + num_offset;
		return;
	}
#endif ED1000

#if defined NLS16 && defined EUC

	if (IS_FIRST(c)){
		cw  = C_COLWIDTH(c & TRIM);
		if ( WCOLS ) {
			rem_col = WCOLS - ( vcntcol % WCOLS ) ;
			if ( cw > rem_col )
       			vcntcol += rem_col ;
		}
		vcntcol += 1 ;
		cw 	-= 1 ;
	}else{
		if ( !(IS_SECOND(c)) ) {
	/* For single-byte characters	*/
			vcntcol++;
		}else{
	/* add the remaining column width*/
			vcntcol += cw ;
			cw = 0	;
		}
	}
#else
	vcntcol++;
#ifdef NLS16
	if (WCOLS && (vcntcol % WCOLS) == 0 && IS_FIRST(c))
		vcntcol++;
#endif NLS16
#endif


}

reverse(a1, a2)
	register line *a1, *a2;
{
	register line t;

	for (;;) {
		t = *--a2;
		if (a2 <= a1)
			return;
		*a2 = *a1;
		*a1++ = t;
	}
}

save(a1, a2)
	line *a1;
	register line *a2;
{
	register int more;

	if (!FIXUNDO)
		return;
#ifdef UNDOTRACE
	if (trace)
		vudump("before save");
#endif
	undkind = UNDNONE;
	undadot = dot;
	more = (a2 - a1 + 1) - (unddol - dol);
	while (more > (endcore - truedol))
		if (morelines() < 0)

#ifndef NONLS8 /* User messages */
			error((nl_msg(6, "Out of memory|Out of memory saving lines for undo - try using ed")));
#else NONLS8
			error("Out of memory@saving lines for undo - try using ed");
#endif NONLS8

	if (more)
		(*(more > 0 ? copywR : copyw))(unddol + more + 1, unddol + 1,
		    (truedol - unddol));
	unddol += more;
	truedol += more;
	copyw(dol + 1, a1, a2 - a1 + 1);
	undkind = UNDALL;
	unddel = a1 - 1;
	undap1 = a1;
	undap2 = a2 + 1;
#ifdef UNDOTRACE
	if (trace)
		vudump("after save");
#endif
}

save12()
{

	save(addr1, addr2);
}

saveall()
{

	save(one, dol);
}

span()
{

	return (addr2 - addr1 + 1);
}

sync()
{

	chng = 0;
	tchng = 0;
	xchng = 0;
}


skipwh()
{
	register int wh;

	wh = 0;
	while (iswhite(peekchar())) {
		wh++;
		ignchar();
	}
	return (wh);
}

/*VARARGS2*/
smerror(seekpt, cp)
#ifdef lint
	char *seekpt;
#else
	int seekpt;
#endif
	char *cp;
{

	if (seekpt == 0)
		return;
	merror1(seekpt);
	if (inopen && clr_eol)
		vclreol();
	if (enter_standout_mode && exit_standout_mode)
		putpad(enter_standout_mode);

#ifndef	NLS16
	lprintf(mesg(linebuf), cp);
#else
	lprintf(mesg(LINEBUF), cp);
#endif

	if (enter_standout_mode && exit_standout_mode)
		putpad(exit_standout_mode);
}

char *
strend(cp)
	register char *cp;
{

	while (*cp)
		cp++;
	return (cp);
}

strcLIN(dp)
	char *dp;
{

#ifndef	NLS16
	CP(linebuf, dp);
#else
	CP(LINEBUF, dp);
#endif

}

/*
 * A system error has occurred that we need to perror.
 * danger is true if we are unsure of the contents of
 * the file or our buffer, e.g. a write error in the
 * middle of a write operation, or a temp file error.
 */
syserror(danger)
int danger;
{
	extern char *strerror();
	register char *e;

	dirtcnt = 0;
	putchar(' ');
	if (danger)
		edited = 0;	/* for temp file errors, for example */
	if (*(e = strerror( errno)) != '\0') {
		error( e);
	}
	else {
		error((nl_msg(44, "System error %d")), errno);
	}
}

/*
 * Return the column number that results from being in column col and
 * hitting a tab, where tabs are set every ts columns.  Work right for
 * the case where col > columns, even if ts does not divide columns.
 */
tabcol(col, ts)
int col, ts;
{

	int offset, result, num_offset;

	if (col >= columns) {
		offset = columns * (col/columns);
		col -= offset;
	} else
		offset = 0;


	/*************
	* FSDlj06791
	* If :set nu has been activated, all text on the screen is moved over 
	* 8 spaces.  If col comes into this routine with the 8 spaces accounted
	* for, this messes up the first tab when tabstop is not
	* a factor of 8.  The solution is to subtract out the number of
	* spaces temporarily if line numbering has been set and col is 
	* greater than the number of indented spaces.
	**************/
	if (value(NUMBER) && (col >= NUMBER_INDENT) && (offset == 0)) {
		col = col - NUMBER_INDENT;
		num_offset = NUMBER_INDENT;
	} else
		num_offset = 0;

	result = col + ts - (col % ts) + offset + num_offset;
	return (result);
}

CHAR *
vfindcol(i)
	int i;
{
	register CHAR *cp;
	register int (*OO)() = Outchar;

	Outchar = qcount;
/*
 * ED1000 historical note: NOCHAR is (CHAR *); _NOSTR is (char *) and
 *	was used for the 7-bit version.
 */
	ignore(qcolumn(linebuf - 1, NOCHAR));
	for (cp = linebuf; *cp && vcntcol < i; cp++)
		putchar(*cp);
	if (cp != linebuf)

#ifndef	NLS16
		cp--;
#else
		PST_DEC(cp);
#endif

	Outchar = OO;
	return (cp);
}

CHAR *
vskipwh(cp)
	register CHAR *cp;
{

	while (iswhite(*cp) && cp[1])
		cp++;
	return (cp);
}

CHAR *
vpastwh(cp)
	register CHAR *cp;
{

	while (iswhite(*cp))
		cp++;
	return (cp);
}

whitecnt(cp)
	register CHAR *cp;
{
	register int i;

	i = 0;
	for (;;)
		switch (*cp++) {

		case '\t':
			i += value(TABSTOP) - i % value(TABSTOP);
			break;

		case ' ':
			i++;
			break;

		default:
#if defined NLS || defined NLS16
			if (*(cp-1) == alt_space) {
				i++;
				break;
			}
#endif
			return (i);
		}
}

#ifdef lint
Ignore(a)
	char *a;
{

	a = a;
}

Ignorf(a)
	int (*a)();
{

	a = a;
}
#endif

markit(addr)
	line *addr;
{

	if (addr != dot && addr >= one && addr <= dol)
		markDOT();
}

/*
 * The following code is defensive programming against a bug in the
 * pdp-11 overlay implementation.  Sometimes it goes nuts and asks
 * for an overlay with some garbage number, which generates an emt
 * trap.  This is a less than elegant solution, but it is somewhat
 * better than core dumping and losing your work, leaving your tty
 * in a weird state, etc.
 */
int _ovno;
onemt()
{
	int oovno;

	signal(SIGEMT, onemt);
	oovno = _ovno;
	/* 2 and 3 are valid on 11/40 type vi, so */
	if (_ovno < 0 || _ovno > 3)
		_ovno = 0;

#ifndef NONLS8 /* User messages */
	error((nl_msg(45, "emt trap, _ovno is %d |emt trap, _ovno is %d   - try again")));
#else NONLS8
	error("emt trap, _ovno is %d @ - try again");
#endif NONLS8

}

/*
 * When a hangup occurs our actions are similar to a preserve
 * command.  If the buffer has not been [Modified], then we do
 * nothing but remove the temporary files and exit.
 * Otherwise, we sync the temp file and then attempt a preserve.
 * If the preserve succeeds, we unlink our temp files.
 * If the preserve fails, we leave the temp files as they are
 * as they are a backup even without preservation if they
 * are not removed.
 */
onhup()
{

	/*
	 * USG tty driver can send multiple HUP's!!
	 */
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	if (chng == 0) {
		cleanup(1);
		exit(0);
	}
	if (setexit() == 0) {
		if (preserve()) {
			cleanup(1);
			exit(0);
		}
	}
	exit(1);
}

/*
 * Similar to onhup.  This happens when any random core dump occurs,
 * e.g. a bug in vi.  We preserve the file and then generate a core.
 */
oncore(sig)
int sig;
{
	static int timescalled = 0;

	/*
	 * USG tty driver can send multiple HUP's!!
	 */
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(sig, SIG_DFL);	/* Insure that we don't catch it again */
	if (timescalled++ == 0 && chng && setexit() == 0) {
		if (inopen)
			vsave();
		preserve();
		write(1, (nl_msg(46, "\r\nYour file has been preserved\r\n")), 32);
	}
	if (timescalled < 2) {
		normal(normf);
		cleanup(2);
		kill(getpid(), sig);	/* Resend ourselves the same signal */
		/* We won't get past here */
	}
	exit(1);
}

/*
 * An interrupt occurred.  Drain any output which
 * is still in the output buffering pipeline.
 * Catch interrupts again.  Unless we are in visual
 * reset the output state (out of -nl mode, e.g).
 * Then like a normal error (with the \n before Interrupt
 * suppressed in visual mode).
 */
onintr()
{
#ifndef CBREAK
	signal(SIGINT, onintr);
#else
	signal(SIGINT, inopen ? vintr : onintr);
#endif
	cancelalarm();
	draino();
	if (!inopen) {
		pstop();
		setlastchar('\n');
#ifdef CBREAK
	}
#else
	} else
		vraw();
#endif
	error((nl_msg(47, "\nInterrupt")) + (inopen!=0));
}

/*
 * If we are interruptible, enable interrupts again.
 * In some critical sections we turn interrupts off,
 * but not very often.
 */
setrupt()
{

	if (ruptible) {
#ifndef CBREAK
		signal(SIGINT, onintr);
#else
		signal(SIGINT, inopen ? vintr : onintr);
#endif
#ifdef SIGTSTP
		if (dosusp)
			signal(SIGTSTP, onsusp);
#endif
	}
}

preserve()
{

#ifdef VMUNIX
	tflush();
#endif
	synctmp();
	pid = fork();
	if (pid < 0)
		return (0);
	if (pid == 0) {
		close(0);
		dup(tfile);
#ifndef NAME8
		execl(EXPRESERVE, "expreserve", (char *) 0);
#else NAME8
		execl(EXPRESERVE, "expreserve8", (char *) 0);
#endif NAME8
		exit(1);
	}
#ifdef USG
	/*
	 * need to do a real wait so the returned status can be checked below
	 */
	oldchild = signal(SIGCLD, SIG_DFL);
#endif
	waitfor();
#ifdef USG
	oldchild = signal(SIGCLD, SIG_IGN);
#endif
	if (rpid == pid && status == 0)
		return (1);
	return (0);
}

#ifndef V6
exit(i)
	int i;
{

# ifdef TRACE
	if (trace)
		fclose(trace);
# endif

#ifdef ED1000
	tty.c_iflag = breakstate;
	sTTY(2);
#endif ED1000

	_exit(i);
}
#endif

#ifdef SIGTSTP
/*
 * We have just gotten a susp.  Suspend and prepare to resume.
 */
onsusp()
{
	ttymode f;
	int savenormtty;
#ifdef SIGWINCH
	struct winsize win;
#endif SIGWINCH

	f = setty(normf);
	vnfl();
	putpad(exit_ca_mode);
	flush();
	resetterm();
	savenormtty = normtty;
	normtty = 0;
	was_suspended++;

#ifdef SIGWINCH
	if (invisual)
	    if (ioctl(0, TIOCGWINSZ, &win) >= 0)
	    {
		save_win.ws_row = win.ws_row;
    		save_win.ws_col = win.ws_col;
	    }
#endif SIGWINCH
	signal(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);

	/* the pc stops here */

	signal(SIGTSTP, onsusp);
	was_suspended = 0;
	normtty = savenormtty;
	vcontin(0);
	setty(f);
	if (!inopen)
		error(0);
#ifdef SIGWINCH
	else 
	{
	    if (invisual)
		if (ioctl(0, TIOCGWINSZ, &win) >= 0)
			if (win.ws_row != save_win.ws_row ||
			    win.ws_col != save_win.ws_col)
				winch();
	}
#endif SIGWINCH
}
#endif SIGTSTP

#ifdef	NLS16
CHAR *
STREND(cp)
register CHAR *cp;
{
	while (*cp)
		cp++;
	return (cp);
}

STRCLIN(cp)
register CHAR *cp;
{
	STRCPY(linebuf, cp);
}

CHAR *
STRCPY(a, b)
register CHAR *a, *b;
{
	register CHAR *c;

	c = a;
	while (*a++ = *b++);
	return (c);
}

CHAR *
STRNCPY(s1, s2, n)
register CHAR *s1, *s2;
register int n;
{
	register CHAR *os1 = s1;

	if (!s2) {
		while (--n >= 0)
			*s1++ = '\0';
	}
	else {
		while (--n >= 0)
			if ((*s1++ = *s2++) == '\0')
				while (--n >= 0)
					*s1++ = '\0';
	}
	return (os1);
}

CHAR *
STRCHR(sp, c)
register CHAR *sp, c;
{
	if (sp == 0)
		return(0);
	do {
		if(*sp == c)
			return(sp);
	} while(*sp++);
	return(0);
}

STRCMP(a, b)
register CHAR *a, *b;
{
	while (*a == *b++)
		if (*a++ == 0)
			return(0);
	return(*a - *--b);
}

/*
 * STRNCMP() -- wonder why the vi folks haven't written this routine.
 *	This particular one is my own routine to fill the void, so any
 *	problems with it should NOT be blamed on the vi folks. And for
 *	the vi folks: please delete this routine when you decide to add
 *	your own "official" one -- svn.
 */
STRNCMP(a, b, len)
register CHAR	*a, *b;
register int	len;
{
	while((*a == *b++) && (len-- > 0))
		if (*a++ == 0)
			return(0);
	return((len == 0) ? 0 : *a - *--b);
}

CHAR *
STRCAT(a, b)
register CHAR *a, *b;
{
	register CHAR *c;
	
	c = STREND(a);
	return(STRCPY(c, b));
}

STRLEN(cp)
register CHAR *cp;
{
	register i = 0;

	while (*cp++)
		i++;
	return(i);
}

#ifdef EUC
/* This function returns the number of column occupied by	*/
/* the data indicated by the arguments				*/
int
c_strlen(cp)
char *cp ;
{
	int i = 0 ;
	while(*cp) {
		if( ( FIRSTof2( ( (int)(*cp) ) & TRIM ) ) && ( SECONDof2(( (int)(*(cp+1)) ) & TRIM ) ) ) {
			i += C_COLWIDTH( ( (int)(*cp) ) & TRIM  ) ;
			cp++ ;
		}else{
			i++ ;
		}
		cp++ ;
	}
	return(i) ;
}
#endif EUC

char_TO_CHAR(a, b)
register char *a;
register CHAR *b;
{
	while (*a)	{
		if (FIRSTof2((*a)&TRIM) && SECONDof2((*(a+1))&TRIM))	{
			*b++ = (*a++ & TRIM) | FIRST;
			*b++ = (*a++ & TRIM) | SECOND;
		} else
			*b++ = *a++ & TRIM;
	}
	*b = *a;
}

CHAR_TO_char(a, b)
register CHAR *a;
register char *b;
{
	while (*b++ = *a++);
}

PRINTF(cp)
register CHAR *cp;
{
	while (*cp)
		putchar(*cp++);
}

/* p+n */
CHAR	*NADVANCE(p, n)	/* returns the pointer to the positive n-th character from *p */
CHAR	*p;		/* pointer to character string */
int	n;		/* the number of characters to go forward on the *p */
{
	int	i;	/* dummy counter */

	if (n == 0)
		return(p);
	if (n < 0)
		return(NREVERSE(p, -n));
	for (i = 0; i < n; i++)
		PRE_INC(p);
	return(p);
}

/* p-n */
CHAR	*NREVERSE(p, n)	/* returns the pointer to the negative n-th character from *p */
CHAR	*p;		/* pointer to character string */
int	n;		/* the number of characters to go back on the *p */
{
	int	i;	/* dummy counter */

	if (n == 0)
		return(p);
	if (n < 0)
		return(NADVANCE(p, -n));
	for (i = 0; i < n; i++)
		PRE_DEC(p);
	return(p);
}

int	kstrlen(str)	/* returns number of CHARACTERS in str */
CHAR	*str;		/* A single Kanji character is counted by 1 */
{
	int	count = 0;	/* character counter */

	while (*str)	{
		count++;
		PRE_INC(str);
	}
	return(count);
}
#endif

#if defined NLS || defined NLS16

#ifdef NLS16

CHAR *
FLIP(s)
CHAR *s;
{
	extern char *strord();		/* key-screen order routine */
	extern char *strcpy();

	char new_in[BUFSIZ];
	char new_out[BUFSIZ];
	
	CHAR_TO_char(s,new_in);
	char_TO_CHAR(strcpy(new_in,strord(new_out,new_in,rl_mode)),s);
	return s;
}

#endif

char *
flip(s)
char *s;
{
	extern char *strord();		/* key-screen order routine */
	extern char *strcpy();

	char new_in[BUFSIZ];
	char new_out[BUFSIZ];
	
	return strcpy(s,strord(new_out,s,rl_mode));
}

#ifdef	NLS16
#ifdef EUC

/* This function calculates the data size from column		*/
int
CTOBSIZE( ptr, column )
CHAR *ptr ; /* starting point of the buffer */
int column ; /* the position measured by column */
	     /* on the screen                   */
{
	int bsize    ;   /* size of the data        */
	int countcol ; /* column on the screen    */
		       /* by the character              */
	for ( bsize=0,countcol=0 ; *ptr && countcol < column ; ptr++, bsize++, countcol++ ){
		if ( IS_FIRST(*ptr) ) {
			countcol++ ;
			countcol -= ( 2 - C_COLWIDTH( ( (int)(*ptr) ) & TRIM ) ) ;
			ptr++ ;
			bsize++ ;
		}
	}
	return(bsize);
}

#endif EUC


CHAR *
FLIP_LINE(buf,offset)
CHAR *buf;
int offset;
{
	CHAR temp[LBSIZE];
	register CHAR *p;
	CHAR *ptr ;
	register int num;
	int length ;

	/* leave room for the leading offset */
	for (p=temp ; (p-temp) < offset ; p++) *p = '\001';

	/* set up for loop */
	num = (STRLEN(buf) + offset) / WCOLS;
	p = buf;

	/* flip each WCOLS line segment */
	do {
		STRNCPY((temp+offset),p,WCOLS-offset );
		temp[WCOLS] = (CHAR) 0;
		STRNCPY(p, (FLIP(temp)+offset), WCOLS-offset );
		p += WCOLS - offset ;
		offset = 0;
	} while (num--);

	return buf;
}


char *
flip_line(buf,offset)
char *buf;
int offset;
{
	char temp[LBSIZE];
	register char *p;
	char *ptr ;
	register int num;
	int length ;

	/* leave room for the leading offset */
	for (p=temp ; (p-temp) < offset ; p++) *p = '\001';

	/* set up for loop */
	num = (strlen(buf) + offset) / WCOLS;
	p = buf;

	/* flip each WCOLS line segment */
	do {
		strncpy((temp+offset),p,WCOLS-offset );
		temp[WCOLS] = (CHAR) 0;
		strncpy(p, (flip(temp)+offset), WCOLS-offset );
		p += WCOLS - offset ;
		offset = 0;

	} while (num--);

	return buf;
}

#else NLS16

char *
flip_line(buf,offset)
char *buf;
int offset;
{
	char temp[LBSIZE];
	register char *p;
	register int num;

	/* leave room for the leading offset */
	for (p=temp ; (p-temp) < offset ; p++) *p = '\001';

	/* set up for loop */
	num = (strlen(buf) + offset) / WCOLS;
	p = buf;

	/* flip each WCOLS line segment */
	do {
		strncpy((temp+offset),p,WCOLS-offset);
		temp[WCOLS] = (CHAR) 0;
		strncpy(p,(flip(temp)+offset),WCOLS-offset);
		p += WCOLS - offset;
		offset = 0;
	} while (num--);

	return buf;
}

#endif NLS16

to_char(a, b)
register short *a;
register char *b;
{
	while (*b++ = *a++);
}

short *
to_short(a, b)
register CHAR *a;
register short *b;
{
	register short *p = b;
	while (*b++ = *a++ & TRIM);
	return p;
}

#if defined NLS16 && defined EUC
extern short *vbuf0 ;
#endif
upd_vtube(buf,col)
CHAR *buf;
int col;
{
#ifdef	NLS16
#ifdef EUC
short *tp, *tp_buf ;
int i, j ;
	copys(vbuf0 + vcolumn( vbuf0, col ), buf, STRLEN(buf)) ;
	/* update vtube						*/
	/* i represents the number of column			*/
	/* j represents the number of line			*/
	tp = vtube0 ;
	tp_buf = vbuf0 ;
	/* clear the first line of vtube0	*/
	vclrbyte( tp, BTOCRATIO*WCOLS ) ;
	for( i = 0, j = 0 ; *tp_buf ; ) {
		*tp++ = *tp_buf++ ;
		i++ ;
		if( i % WCOLS == 0 ) {
			j++ ;
			tp = vtube0 + j * BTOCRATIO * WCOLS ;
			vclrbyte( tp, BTOCRATIO*WCOLS ) ;
		}
	}
	/* end of update		*/
#else EUC
	copys(vtube0+col,buf,STRLEN(buf)) ;
#endif EUC
#else
	short temp[LBSIZE];
	copys(vtube0+col,to_short(buf,temp),strlen(buf));
#endif
}

chk_vtube_end(col)
int col;
{
	CHAR temp1[LBSIZE];
	CHAR temp2[LBSIZE];
	short temp3[LBSIZE];
	register int i,j;
	register CHAR *p;

#ifdef	NLS16
#ifdef EUC
	short *tp, *tp_buf ;
	copys(temp1,vbuf0,STRLEN(vbuf0));
#else EUC
	copys(temp1,vtube0,STRLEN(vtube0));
#endif EUC
#else
	to_char(vtube0,temp1);
#endif
	if (OPP_LANG(*(temp1+col))) {
		STRCPY(temp2,temp1+col);
		*(p = temp1 + col) = (CHAR) 0;
		for (i=0, p-- ; OPP_LANG(*p) ; i++, p--); p++;
#ifdef	NLS16
#ifdef EUC
		STRCAT(FLIP(temp2),p);
		copys(vbuf0+vcolumn( vbuf0, col-i ), temp2, STRLEN(temp2)) ;
	}
	/* update vtube						*/
	/* i represents the number of column			*/
	/* j represents the number of line			*/
	tp = vtube0 ;
	tp_buf = vbuf0 ;
	/* clear the first line of vtube0	*/
	vclrbyte( tp, BTOCRATIO*WCOLS ) ;
	for( i = 0, j = 0 ; *tp_buf ; ) {
		*tp++ = *tp_buf++ ;
		i++ ;
		if( i % WCOLS == 0 ) {
			j++ ;
			tp = vtube0 + j * BTOCRATIO * WCOLS ;
			vclrbyte( tp, BTOCRATIO*WCOLS ) ;
		}
	}
	/* end of update		*/
#else EUC
		STRCAT(FLIP(temp2),p);
		copys(vtube0+col-i,temp2,STRLEN(temp2)) ;
	}
#endif EUC
#else
		copys(vtube0+col-i,to_short(strcat(flip(temp2),p),temp3),strlen(temp2)+i);
	}
#endif
}

base_lang()
{
	if (rl_mode == NL_LATIN) {
		tputs(latin_lang, 1, putch);
	} else {
		if (rl_lang == HEBREW) {
			tputs(hebrew_lang, 1, putch);
		} else {
			tputs(arabic_lang, 1, putch);
		}
	}
	flusho();
}

opp_lang()
{
	if (rl_mode == NL_LATIN) {
		if (rl_lang == HEBREW) {
			tputs(hebrew_lang, 1, putch);
		} else {
			tputs(arabic_lang, 1, putch);
		}
	} else {
		tputs(latin_lang, 1, putch);
	}
	flusho();
}

base_mode()
{
	if (rl_mode == NL_LATIN) {
		tputs(l_mode, 1, putch);
		tputs(latin_lang, 1, putch);
	} else {
		tputs(n_mode, 1, putch);
		if (rl_lang == HEBREW) {
			tputs(hebrew_lang, 1, putch);
		} else {
			tputs(arabic_lang, 1, putch);
		}
	}
	flusho();
}

opp_in_mode()
{
	if (rl_mode == NL_LATIN) {
		if (rl_lang == HEBREW) {
			tputs(hebrew_lang, 1, putch);
		} else {
			tputs(arabic_lang, 1, putch);
		}
	} else {
		tputs(latin_lang, 1, putch);
	}
	flusho();
}

/*
** Flip search patterns and replacement strings.
** So far only the patterns of search commands are modified.
** These commands include "/", "?" and "s".
** Everything else is kept in keyboard order.
** In particular, filenames with slashes must not be flipped.
*/

flip_echo(s)
CHAR *s;				/* a string from the echo area */
{
	register CHAR *b;		/* beginning marker of echo string */
	register CHAR *e;		/* ending slash of echo string */

	register bool slash	= 0;	/* flags a slash */
	register bool question	= 0;	/* flags a question mark */

	register int c;

	/* be sure we have a search command */

	b = s;
	if ((*b & TRIM) == ':') {
		for (b++ ; ((*b >= IS_MACRO_LOW_BOUND) && _isspace(*b&TRIM)) ; b++ ) ;
		switch (*b & TRIM) {
		case '/':
		case '?':
			break;
		default:
			for ( ; ((*b < IS_MACRO_LOW_BOUND) || !isalpha(*b&TRIM)) ; b++ ) ;
			if (*b == 's')
				if ((*(b+1) < IS_MACRO_LOW_BOUND) || !isalpha(*(b+1)&TRIM)) 
					break;
			return;
		}
	}

	/* allow alternative slashes */

	for (b=s ; *b ; b++ ) {
		if ((*b & TRIM) == alt_slash) {
			*b = '/';
		}
	}

	/* do the flip */

	for (b=s ; c = *b ; b++) {
		switch(c) {
		case '/': slash++;	break;
		case '?': question++;	break;
		}
		if (question) {
			b++;		/* flip stuff after question mark */
			FLIP(b);
			break;
		} else if (slash) {	/* flip stuff between slashes */
			if ((e = STRCHR(++b, '/')) != 0) {
				*e = '\0';
				FLIP(b);
				*e = '/';
				b = e+1;
				if ((e = STRCHR(b, '/')) != 0) {
					*e = '\0';
					FLIP(b);
					*e = '/';
				}
			} else {
				FLIP(b);
			}
			break;
		}
	}
}

char *
alt_num(to,from)
char *to;
char *from;
{
	extern unsigned char *_nl_dgt_alt;

	register char *s;

	if (! *_nl_dgt_alt) return from;

	for (s = to ; (( *from >= IS_MACRO_LOW_BOUND) && isdigit( *from)) ; from++, s++) {
		*s = ascii_num_to_alt( *from);	
	}
	*s = '\0';

	return to;
}

ascii_num_to_alt(c)
int c;
{
	extern unsigned char *_nl_dgt_alt;
	extern unsigned char *_nl_dascii;

	register unsigned char *p;
	register int i,j;

	for (p = _nl_dascii, i = 0 ; *p ; p++, i++) if (*p == c) break;

	for (p = _nl_dgt_alt, j = 0 ; i > j ; ADVANCE( p), j++) ;

	return (int) CHARAT( p);
}

ascii_to_alt(c)
int c;
{
	extern unsigned char *_nl_punct_alt;
	extern unsigned char *_nl_pascii;

	register unsigned char *p;
	register int i,j;

	for (p = _nl_pascii, i = 0 ; *p ; p++, i++) if (*p == c) break;

	for (p = _nl_punct_alt, j = 0 ; i > j ; ADVANCE( p), j++) ;

	return (int) CHARAT( p);
}

alt_to_ascii(c)
int c;
{
	extern unsigned char *_nl_punct_alt;
	extern unsigned char *_nl_pascii;

	register unsigned char *p;
	register int i;

	for (p = _nl_punct_alt, i = 0 ; *p ; ADVANCE( p), i++) {
		if (CHARAT( p) == c) break;
	}

	return (int) *(_nl_pascii + i);
}

get_rlsent()
{
	rl_endsent[0] = '.';
	rl_endsent[1] = '!';
	rl_endsent[2] = '?';
	rl_endsent[3] = ascii_to_alt('.');
	rl_endsent[4] = ascii_to_alt('!');
	rl_endsent[5] = ascii_to_alt('?');
	rl_endsent[6] = '\0';

	rl_insent[0] = ')';
	rl_insent[1] = ']';
	rl_insent[2] = '\'';
	rl_insent[3] = ascii_to_alt(')');
	rl_insent[4] = ascii_to_alt(']');
	rl_insent[5] = ascii_to_alt('\'');
	rl_insent[6] = ascii_to_alt('"');
	rl_insent[7] = '\0';

	rl_white[0] = ' ';
	rl_white[1] = '\t';
	rl_white[2] = alt_space;
	rl_white[3] = '\0';

	rl_parens[0] = '(';
	rl_parens[1] = '[';
	rl_parens[2] = '{';
	rl_parens[3] = ')';
	rl_parens[4] = ']';
	rl_parens[5] = '}';
	rl_parens[6] = ascii_to_alt('(');
	rl_parens[7] = ascii_to_alt('[');
	rl_parens[8] = ascii_to_alt('{');
	rl_parens[9] = ascii_to_alt(')');
	rl_parens[10] = ascii_to_alt(']');
	rl_parens[11] = ascii_to_alt('}');
	rl_parens[12] = '\0';

	rl_paren_1[0] = ascii_to_alt('(');
	rl_paren_1[1] = ascii_to_alt(')');
	rl_paren_1[2] = '\0';

	rl_paren_2[0] = ascii_to_alt('[');
	rl_paren_2[1] = ascii_to_alt(']');
	rl_paren_2[2] = '\0';

	rl_paren_3[0] = ascii_to_alt('{');
	rl_paren_3[1] = ascii_to_alt('}');
	rl_paren_3[2] = '\0';
}

#define an_cap		"\033*s-1^"	/* request alpha-numeric capabilities */
#define sec_status	"\033~"		/* secondary status */
#define on_straps	"\033&s1g1H"	/* strap G & H on -- no handshake */
#define off_straps	"\033&s0g0H"	/* strap G & H off -- D1 */

#define DISPLAY		2		/* alpha-num display byte */
#define ORDER		0x10		/* alpha-num display ordering bit */
#define MEA_SEC		8		/* 2nd status byte 13 */
#define MODE		0x08		/* 2nd status mode bit */

nl_mode			term_mode;	/* mode of terminal */
nl_order		term_order;	/* order of terminal */

get_rlterm()
{
	char buf[128];
	struct termio tbuf;
	struct termio tbufsave;

	/* fetch & save current status of terminal driver */
	ioctl(1,TCGETA,&tbuf);
	tbufsave = tbuf;

	/* turn off echo to prevent status bytes from appearing on screen */
	tbuf.c_lflag &= ~ECHO;

	/* set status of terminal driver with echo off */
	ioctl(1,TCSETAF,&tbuf);

	/* turn off handshaking (G & H straps on) */
	write(1,on_straps,strlen(on_straps));

	/* get alpha-numeric capabilities: ordering is byte 2, bit 4 */
	write(1,an_cap,strlen(an_cap));
	gets(buf);
	term_order = (buf[DISPLAY] & ORDER) ? NL_KEY : NL_SCREEN;
	
	/* get secondary status: mode is byte 13, bit 3 */
	write(1,sec_status,strlen(sec_status));
	gets(buf);
	term_mode = (buf[MEA_SEC] & MODE) ? NL_NONLATIN : NL_LATIN;
	
	/* turn on D1 handshaking (G & H straps off) */
	write(1,off_straps,strlen(off_straps));

	/* restore status of terminal driver */
	ioctl(1,TCSETAF,&tbufsave);
}

reset_rlterm()
{
	nl_mode	srlm;

	/* restore terminal order */
	if (term_order == NL_KEY) {
		tputs(key_order,1,putch);
		rl_curorder = NL_KEY;
	} else {
		tputs(screen_order,1,putch);
		rl_curorder = NL_SCREEN;
	}

	/* restore terminal mode */
	srlm = rl_mode;
	rl_mode = term_mode;
	base_mode();
	rl_mode = srlm;
}

set_rlterm()
{
	/* set terminal order for command mode */
	tputs(screen_order, 1, putch);
	rl_curorder = NL_SCREEN;

	/* set terminal mode for command mode */
	base_mode();
}
#endif
