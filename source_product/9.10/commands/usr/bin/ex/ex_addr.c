/* @(#) $Revision: 66.2 $ */   
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_re.h"

/*
 * Routines for address parsing and assignment and checking of address bounds
 * in command mode.  The routine address is called from ex_cmds.c
 * to parse each component of a command (terminated by , ; or the beginning
 * of the command itself.  It is also called by the scanning routine
 * in ex_voperate.c from within open/visual.
 *
 * Other routines here manipulate the externals addr1 and addr2.
 * These are the first and last lines for the current command.
 *
 * The variable bigmove remembers whether a non-local glitch of . was
 * involved in an address expression, so we can set the previous context
 * mark '' when such a motion occurs.
 */

static	bool bigmove;

#ifndef NONLS8	/* User messages */
# define	NL_SETN	2	/* set number */
# include	<msgbuf.h>
# undef	getchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Set up addr1 and addr2 for commands whose default address is dot.
 */
setdot()
{

	setdot1();
	if (bigmove)
		markDOT();
}

/*
 * Call setdot1 to set up default addresses without ever
 * setting the previous context mark.
 */
setdot1()
{

	if (addr2 == 0)
		addr1 = addr2 = dot;
	if (addr1 > addr2) {
		notempty();
		error((nl_msg(1, "Addr1 > addr2|First address exceeds second")));
	}
}

/*
 * Ex allows you to say
 *	delete 5
 * to delete 5 lines, etc.
 * Such nonsense is implemented by setcount.
 */
setcount()
{
	register int cnt;

	pastwh();
	

#ifndef NONLS8	/* Character set features */
	if ((peekchar() < IS_MACRO_LOW_BOUND) || !isdigit(peekchar() & TRIM)) { 
#else NONLS8
	if ((peekchar() < IS_MACRO_LOW_BOUND) || !isdigit(peekchar())) { 
#endif NONLS8

		setdot();
		return;
	}
	addr1 = addr2;
	setdot();
	cnt = getnum();
	if (cnt <= 0)
		error((nl_msg(2, "Bad count|Nonzero count required")));
	addr2 += cnt - 1;
	if (addr2 > dol)
		addr2 = dol;
	nonzero();
}

/*
 * Parse a number out of the command input stream.
 */
getnum()
{
	register int cnt;


#ifndef NONLS8	/* Character set featrures */
	for (cnt = 0; ((peekcd() >= IS_MACRO_LOW_BOUND) && isdigit(peekcd() & TRIM));)

#else NONLS8
	for (cnt = 0; ((peekcd() >= IS_MACRO_LOW_BOUND) && isdigit(peekcd()));)
#endif NONLS8

		cnt = cnt * 10 + getchar() - '0';
	return (cnt);
}

/*
 * Set the default addresses for commands which use the whole
 * buffer as default, notably write.
 */
setall()
{

	if (addr2 == 0) {
		addr1 = one;
		addr2 = dol;
		if (dol == zero) {
			dot = zero;
			return;
		}
	}
	/*
	 * Don't want to set previous context mark so use setdot1().
	 */
	setdot1();
}

/*
 * No address allowed on, e.g. the file command.
 */
setnoaddr()
{

	if (addr2 != 0)

#ifndef NONLS8	/* User messages */
		error((nl_msg(3, "No address allowed|No address allowed on this command")));
#else NONLS8
		error("No address allowed@on this command");
#endif NONLS8

}

/*
 * Parse an address.
 * Just about any sequence of address characters is legal.
 *
 * If you are tricky you can use this routine and the = command
 * to do simple addition and subtraction of cardinals less
 * than the number of lines in the file.
 */
line *
address(inline)
	CHAR *inline;
{
	register line *addr;
	register int offset, c;
	short lastsign;

	bigmove = 0;
	lastsign = 0;
	offset = 0;
	addr = 0;
	for (;;) {

#ifndef NONLS8	/* Character set features */
		if ((peekcd() >= IS_MACRO_LOW_BOUND) && isdigit(peekcd() & TRIM)) { 
#else NONLS8
		if ((peekcd() >= IS_MACRO_LOW_BOUND) && isdigit(peekcd())) {  
#endif NONLS8

			if (addr == 0) {
				addr = zero;
				bigmove = 1;
			}
			loc1 = 0;
			addr += offset;
			offset = getnum();
			if (lastsign >= 0)
				addr += offset;
			else
				addr -= offset;
			lastsign = 0;
			offset = 0;
		}
		switch (c = getcd()) {

#ifdef ED1000
		case '/':
		case '\'':
			dot = dot - 1; /* to make searches start with
					* the first char of the line
					*/
		case '`':
		case '$':
#else
		case '?':
		case '/':
		case '$':
		case '\'':
#endif ED1000
		case '\\':
			bigmove++;
		case '.':
			if (addr || offset)
				error((nl_msg(4, "Badly formed address")));
		}
		offset += lastsign;
		lastsign = 0;
		switch (c) {

		case ' ':
		case '\t':
			continue;

		case '+':
			lastsign = 1;
			if (addr == 0)
				addr = dot;
			continue;

		case '^':
		case '-':
			lastsign = -1;
			if (addr == 0)
				addr = dot;
			continue;

		case '\\':
#ifdef ED1000
		case '`':
		case '\'':
	     /* case '?':	dd1:rev1 comment out for ? command */
#else
		case '?':
#endif ED1000
		case '/':
			c = compile(c);
			notempty();
			savere(scanre);
			addr = dot;
			if (inline && execute(0, dot)) {
#ifdef ED1000
				if ((c == '/') || (c == '\''))
#else
				if (c == '/')
#endif ED1000
				{
					while (loc1 <= inline) {
						if (loc1 == loc2)
							loc2++;
						if (!execute(1))
							goto nope;
					}
					break;
				} else if (loc1 < inline) {
					CHAR *last;
doques:

					do {
						last = loc1;
						if (loc1 == loc2)
							loc2++;
						if (!execute(1))
							break;
					} while (loc1 < inline);
					loc1 = last;
					break;
				}
			}
nope:
			for (;;) {
#ifdef ED1000
				if ((c == '/') || (c == '\''))
#else
				if (c == '/')
#endif ED1000
				{
					addr++;
					if (addr > dol) {
						if (value(WRAPSCAN) == 0)
error((nl_msg(5, "No match to BOTTOM|Address search hit BOTTOM without matching pattern")));
						addr = zero;
					}
				} else {
					addr--;
					if (addr < zero) {
						if (value(WRAPSCAN) == 0)
error((nl_msg(6, "No match to TOP|Address search hit TOP without matching pattern")));
						addr = dol;
					}
				}
				if (execute(0, addr)) {
#ifdef ED1000
					if (inline && ((c == '?') || (c == '`')))
#else
					if (inline && c == '?')
#endif ED1000
					{
						inline = &linebuf[LBSIZE];
						goto doques;
					}
					break;
				}
				if (addr == dot)
					error((nl_msg(7, "Fail|Pattern not found")));
			}
			continue;

		case '$':
			addr = dol;
			continue;

		case '.':
			addr = dot;
			continue;

#ifdef ED1000
		case ':':
#else
		case '\'':
#endif ED1000
#if defined NLS || defined NLS16
			c = markreg(RL_KEY(getchar()));
#else
			c = markreg(getchar());
#endif
			if (c == 0)
				error((nl_msg(8, "Marks are ' and a-z")));
			addr = getmark(c);
			if (addr == 0)

#ifndef NONLS8	/* User messages */
				error((nl_msg(9, "Undefined mark|Undefined mark referenced")));
#else NONLS8
				error("Undefined mark@referenced");
#endif NONLS8

			break;

		default:
			ungetchar(c);
			if (offset) {
				if (addr == 0)
					addr = dot;
				addr += offset;
				loc1 = 0;
			}
			if (addr == 0) {
				bigmove = 0;
				return (0);
			}
			if (addr != zero)
				notempty();
			addr += lastsign;
			if (addr < zero)

#ifndef NONLS8	/* User messages */
				error((nl_msg(10, "Negative address|Negative address - first buffer line is 1")));
#else NONLS8
				error("Negative address@- first buffer line is 1");
#endif NONLS8

			if (addr > dol)

#ifndef NONLS8	/* User messages */
				error((nl_msg(11, "Not that many lines|Not that many lines in buffer")));
#else NONLS8
				error("Not that many lines@in buffer");
#endif NONLS8

			return (addr);
		}
	}
}

/*
 * Abbreviations to make code smaller
 * Left over from squashing ex version 1.1 into
 * 11/34's and 11/40's.
 */
setCNL()
{

	setcount();
	donewline();
}

setNAEOL()
{

	setnoaddr();
	eol();
}
