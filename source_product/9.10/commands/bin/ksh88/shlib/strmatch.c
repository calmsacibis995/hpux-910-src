/* HPUX_ID: @(#) $Revision: 70.1 $ */
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
 * D. G. Korn
 * G. S. Fowler
 * AT&T Bell Laboratories
 *
 * match shell file patterns -- derived from Bourne and Korn shell gmatch()
 *
 *	sh pattern	egrep RE	description
 *	----------	--------	-----------
 *	*		.*		0 or more chars
 *	?		.		any single char
 *	[.]		[.]		char class
 *	[!.]		[^.]		negated char class
 *	*(.|.)		(.|.)*		0 or more of
 *	+(.|.)		(.|.)+		1 or more of
 *	?(.|.)		(.|.)?		0 or 1 of
 *	@(.|.)		(.|.)		1 of
 *	!(.|.)				none of
 *
 * \ used to escape metacharacters
 *
 *	*, ?, (, |, ), [, \ must be \'d outside of [...]
 *	only ] must be \'d inside [...]
 */
/*
 *  The following for fnmatch()
 */
#include <fnmatch.h>
#define FOUND_CLOSE 1
#define DIDNT_FIND_CLOSE 2

#ifdef MULTIBYTE

#include "national.h"

#define REGISTER

#define C_MASK		(3<<(7*ESS_MAXCHAR))	/* character classes	*/
#define getchar(x)	mb_getchar((unsigned char**)(&(x)))

static int		mb_getchar();

#else

#define REGISTER	register

#define getchar(x)	(*x++)

#endif

#define getsource(s,e)	(((s)>=(e))?0:getchar(s))

static char*		gobble();

static int		onematch();

extern int		submatch();

/*
 * strmatch compares the string s with the shell pattern p
 * returns 1 for match 0 otherwise
 */

int
strmatch(s, p)
register char*	s;
register char*	p;
{
	return(submatch(s, p, s + strlen(s), (char*)0));
}

/*
 * match any pattern in a | separated group
 */

int
submatch(s, p, e, g)
char*		s;
register char*	p;
char*		e;
char*		g;
{
	do
	{
/*		if (!fnmatch(p, s, FNM_PATHNAME|FNM_PERIOD)) return(1); */
		if (onematch(s, p, e, g)) return(1);
	} while (p = gobble(p, 1));
	return(0);
}

/*
 * match a single pattern
 * e is the end (0) of the substring in s
 * g marks the start of a repeated subgroup pattern
 */

static int
onematch(s, p, e, g)
char*		s;
REGISTER char*	p;
char*		e;
char*		g;
{
	register int 	pc;
	register int 	sc;
	register int	n;
	char*		olds;
	char*		oldp;

	do
	{
		olds = s;
		sc = getsource(s, e);
		switch (pc = getchar(p))
		{
		case '[':
		{
			int pat_char, done=0;
			char save;
			char *cp=p, *first_close=0;

			/*
			 *  Take care of leading ']' or
			 *  "!]" in the expression.
			 */
			if(*cp == ']')
				cp++;
			else if (*cp == '!' && *(cp+1) == ']')
				cp+=2;

			while (!done && (pat_char=getchar(cp)))
			{
				switch(pat_char)
				{
				    case '\\':
					/*
					 *  '\' is an escape character within BEs
					 *  for pattern matching notation.
					 */
					if (!(pat_char=getchar(cp)))
						return(0);
					break;
				    case ']':
					/* found the end */
					done++;
					break;
				    case '[':
				    {
					/*
					 *  If next character is not
					 *  ':', '=', or '.', then treat
					 *  this as a normal character,
					 *  otherwise it is the start of
					 *  a equiv class etc.
					 */
					if((pat_char=*cp) == '.' || pat_char == ':' || pat_char == '=')
					{
						int type=pat_char;
						int done2=0;

						/* skip over [.x */
						cp++;
						if (*cp == ']' &&  
						    (*(cp+1) != pat_char || *(cp+2) != ']'))
							/*
							 *  The first ']' not part of a closing
							 *  ".]", "=]", or ":]" is the end of the
							 *  BE.
							 */
							continue;
						cp++;
						while(!done2 && (pat_char=getchar(cp)))
						{
							if(pat_char == ']' && !first_close)
								first_close=cp-1;
							if(pat_char == type && *cp == ']')
							{
								/* found closing .] */
								pat_char=getchar(cp);
								done2=FOUND_CLOSE;
							}
							else if(!pat_char)
							{
								/* fell off end */
								done2=DIDNT_FIND_CLOSE;
								cp=0;
							}
						}
						/*
						 *  Now decide if we found closing pattern
						 *  or not.
						 */
						if((done2 && done2 != FOUND_CLOSE) || *(cp-1) == 0)
						{
							if(first_close)
								cp=first_close;
							else
								return(0);
						}
					}
					break;
				    }
				}
			}
			/*
			 *  If cp is non null, then it points to
			 *  the character after the closing ']'.
			 */
			if(cp && sc)
			{
				int retval;
				char save_s=*s;
				save=*cp;
				*cp=0;
				*s=0;

				/*
				 *  We do NOT use the FNM_PATHNAME nor
				 *  FNM_PERIOD flags since we check for
				 *  those cases in expand.c:glob_dir()
				 */
				retval = fnmatch(p-1, s-1, 0);
				*s=save_s;
				*cp=save;
				/*
				 *  If the one character matched the bracket
				 *  expression, and the rest of the string
				 *  matches the rest of the pattern, return
				 *  true.
				 */
				if ((retval == 0) && onematch(s, cp, e, g))
					return(1);
				
				/*
				 * The single character did not match, or
				 * the rest of the pattern did not match.
				 * Try to match two characters of the string
				 * against the bracket expression and then
				 * the rest of the string with the rest of
				 * the pattern.
				 */
				save_s=*(s+1);
				save=*cp;
				*(s+1)=0;
				*cp=0;
				retval = fnmatch(p-1,s-1, 0);
				*cp=save;
				*(s+1)=save_s;
				if(retval)
					/* no match either, so fail */
					return(0);
				/*
				 *  So now return the results of 
				 *  matching the rest of the string
				 */
				s++;
				return(onematch(s, cp, e, g));
				/* Not reached */
			}
			else if(!sc)
				/*
				 *  If we have a bracket expression, but
				 *  no character in the string, then we
				 *  do not have a match.
				 */
				return(0);
		}
		break;
	        case '\\':
			if (!(pc = getchar(p))) return(0);
			/*FALLTHROUGH*/
		default:
			if (pc != sc) return(0);
			break;
		case '(':
		case '*':
		case '?':
		case '+':
		case '@':
		case '!':
			if (pc == '(' || *p == '(')
			{
				char*	subp;

				s = olds;
				oldp = p - 1;
				subp = p;
				if (pc == '(') pc = '@';
				else subp++;
				if (!(p = gobble(subp, 0))) return(0);
				if ((pc == '*' || pc == '?' || pc == '+' && oldp == g))
				{
					if (onematch(s, p, e, (char*)0)) return(1);
					if (!getsource(s, e)) return(0);
				}
				if (pc == '*' || pc == '+') p = oldp;
				do
				{
					if (submatch(olds, subp, s, (char*)0) == (pc != '!') && onematch(s, p, e, oldp)) return(1);
				} while (s < e && getchar(s));
				return(0);
			}
			else switch (pc)
			{
			case '*':
				/*
				 * several stars are the same as one
				 */

				while (*p == '*')
					if (*(p + 1) == '(') break;
					else p++;
				oldp = p;
				switch (pc = getchar(p))
				{
				case '@':
				case '!':
				case '+':
					n = *p == '(';
					break;
				case '(':
				case '[':
				case '?':
				case '*':
					n = 1;
					break;
				case '\\':
					if (!(pc = getchar(p))) return(0);
					/*FALLTHROUGH*/
				default:
					n = 0;
					break;
				case '|':
				case ')':
				case 0:
					return(1);
				}
				p = oldp;
				do
				{
					if ((n || pc == sc) && onematch(olds, p, e, (char*)0)) return(1);
					olds = s;
				} while (sc && (sc = getsource(s, e)));
				return(0);
				break;
			case '?':
				break;
			default:
				if (pc != sc) return(0);
				break;
			}
			break;
		case '|':
		case ')':
		case 0:
			return(!sc);
		}
	} while (sc);
	return(0);
}

/*
 * gobble chars up to [ | ] ) keeping track of (...) nesting
 * 0 returned if s runs out
 */

static char*
gobble(s, sub)
register char*	s;
register int	sub;
{
	register int	n;

	n = 0;
	for (;;) switch (getchar(s))
	{
	case '\\':
		if (getchar(s)) break;
		/*FALLTHROUGH*/
	case 0:
		return(0);
	case '(':
		n++;
		break;
	case ')':
		if (n-- <= 0) return(sub ? 0 : s);
		break;
	case '|':
		if (sub && !n) return(s);
		break;
	}
}

#ifdef MULTIBYTE

/*
 * return the next char in (*address) which may be from one to three bytes
 * the character set designation is in the bits defined by C_MASK
 */

static int
mb_getchar(address)
unsigned char**	address;
{
	register unsigned char*	cp = *(unsigned char**)address;
	register int		c = *cp++;
	register int		size;
	int			d;

	if (size = echarset(c))
	{
		d = (size == 1 ? c : 0);
		c = size;
		size = in_csize(c);
		c <<= 7 * (ESS_MAXCHAR - size);
		if (d)
		{
			size--;
			c = (c << 7) | (d & ~HIGHBIT);
		}
		while (size-- > 0)
			c = (c << 7) | ((*cp++) & ~HIGHBIT);
	}
	*address = cp;
	return(c);
}

#endif
