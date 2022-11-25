/* @(#) $Revision: 66.3 $ */    

#include "ex.h"
#include "ex_re.h"
#include <limits.h>
#include <setlocale.h>

#ifndef NONLS8 /* User messages & Country customs */
# define	NL_SETN	8	/* set number */
# include	<msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8


#define	ANCHOR_BEGIN	0010	/* match must be anchored at beginning of the line		*/
#define	ANCHOR_END	0020	/* match must be anchored at end of the line			*/
#define	BYTE		0030	/* match a single byte character				*/
#define	KANJI		0040	/* match a multibyte character					*/
#define	RE_STRING	0050	/* match a multibyte character/string				*/
#define	DOT		0060	/* match any character						*/
#define	MB_DOT		0070	/* same as DOT but for multibyte code sets			*/
#define	MATCH_LIST	0100	/* match any char/collation element in list			*/
#define	NON_MATCH_LIST	0110	/* match any char/collation element not in list			*/
#define	PLUS		0120	/* match preceding RE one or more times				*/
#define	Q_MARK		0130	/* match preceding RE zero or one times				*/
#define	RANGE		0140	/* match RE at least m but not more than n times		*/
#define	END_RANGE	0150	/* marker at opposite end of RE from RANGE token		*/
#define	ALT		0160	/* match preceding or following RE (alternation)		*/
#define	LPAREN_SPEC	0170	/* special case of left parentheses				*/
#define	LPAREN		0200	/* left parentheses						*/
#define	RPAREN_SPEC	0210	/* special case of right parentheses				*/
#define	RPAREN		0220	/* right parentheses						*/
#define	MATCH_SAME	0230	/* match same as previous parenthesized RE			*/
#define	JMP		0240	/* jump to different location in RE "program"			*/
#define	DOT_PLUS	0250	/* combination of DOT and PLUS for optimization			*/
#define	BYTE_PLUS	0260	/* combination of BYTE and PLUS for optimization		*/
#define	PLUS_BYTE	0270	/* combination of PLUS and BYTE for optimization		*/
#define	DOT_PLUS_BYTE	0300	/* combination of DOT, PLUS and BYTE for optimization		*/
#define	MB_DOT_PLUS_BYTE 0310	/* same as DOT_PLUS_BYTE but for multibyte code sets		*/
#define	BYTE_PLUS_BYTE	0320	/* combination of BYTE, PLUS and BYTE for optimization		*/
#define	DOT_PLUS_END	0330	/* combination of DOT and PLUS at RE end for optimization	*/
#define	BYTE_PLUS_END	0340	/* combination of BYTE and PLUS at RE end for optimization	*/
#define WORD_BEGIN	0350	/* match the beginning of a word				*/
#define WORD_END	0360	/* match the end of a word					*/
#define	END_RE		0370	/* end of RE program						*/

#define	BACKSLASH	010000	/* bit flag indicating character was backslash quoted		*/
#define	MAGIC_FLAG	020000	/* bit flag indicating . ~ * and [ have their special meanings	*/

#define	STACK_SIZE	10240	/* size of execution stack					*/

extern unsigned int	_collxfrm();

struct logical_char {
	unsigned int	c1,
			c2,
			flag,
			class;
};

static unsigned char	*ctype_funcs[] = {
				(unsigned char *) "alpha",
				(unsigned char *) "upper",
				(unsigned char *) "lower",
				(unsigned char *) "digit",
				(unsigned char *) "xdigit",
				(unsigned char *) "alnum",
				(unsigned char *) "space",
				(unsigned char *) "punct",
				(unsigned char *) "print",
				(unsigned char *) "graph",
				(unsigned char *) "cntrl",
				(unsigned char *) "ascii"
			};


/* shift a string designated by head and tail ptrs cnt units towards the tail */
#define shift(head, tail, cnt)		{							\
						register unsigned char *from = (tail)-1;	\
						register unsigned char *to = from+(cnt);	\
						register unsigned char *quit = (head);		\
						do {						\
							*to-- = *from;				\
						} while (from-- != quit);			\
					}

/* split the 16-bit number "cnt" across two bytes at location "loc" */
#define split16(loc, cnt)		{							\
						register unsigned char *to = (loc);		\
						register int num = (cnt);			\
						*to++ = (num>>8) & 0377;			\
						*to = num & 0377;				\
					}

/* the inverse of split16 */
#define cvt16(ep)	((*(ep)<<8) + *((ep)+1))


regcomp(pattern)
register short	*pattern;
{
	register		c;

	register short		*pp = pattern;
	register unsigned char	*ep;
	register unsigned char	*endbuf;
	register int		not_multibyte = (__nl_char_size == 1);

	unsigned char		*last_token = 0;
	unsigned char		*opt_token = 0;
	int			closed	= 0;
	int			exprcnt;

	unsigned char		*cnt_ep;
	int			i, cflg;
	struct logical_char	lchar, pchar;
	int			c2, range, no_prev_range, two_to_1;
	short			*psave;
	unsigned char		pcopy[8];
	int			re_error;
	int			m, n;
	unsigned char		*string[5];
	unsigned char		**sp = string;
	unsigned char		*xp;
	int			magic;
	short			*rhsp;

	int			level = 0;		/* nesting level */
	int			nparen = 0;		/* number of parentheses pairs */

	ep	= expbuf;
	endbuf	= &ep[ESIZE];

	*sp = 0;					/* initialize string stack */

	magic = value(MAGIC) ? MAGIC_FLAG : 0;

	/*
	 *  Determine if any special cases apply for anchoring the RE at the front.
	 */
	if (*pp == '.' && (*(pp+1) == '*'))
		/* anchor a leading ".*" to improve performance */
		*ep++ = ANCHOR_BEGIN;		


	while (1) {

		if (ep+9 >= endbuf)			/* "9" allows for the largest token except for */
			return(REG_ESPACE);		/* MATCH_LIST which is handled separately */

		c = *pp++;

		/*
		 *  End of RE?  If so, add final token and return success.
		 */
		if (c == '\0') {

			if (level)
				return(REG_EPAREN);

			*ep++ = END_RE;				/* add final token to compiled RE */
			nbra = nparen;				/* number of parenthesized subexpressions */
			return(0);				/* and return `success' */
		}

		/*
		 *  Otherwise process new expression
		 */
		if (c == '\\') {
			c = (*pp++) | BACKSLASH;
			if (c == BACKSLASH)
				return(REG_EESCAPE);		/* can't have a trailing \ */
		}

		switch (c|magic) {

		case '.'|MAGIC_FLAG:
		case '.'|BACKSLASH:
			/*
			 *  DOT		- match any character
			 *
			 *	re:	.
			 *	comp:	DOT
			 */
			opt_token = last_token = ep;
			*ep++ = (not_multibyte) ? DOT : MB_DOT;
			continue;

		case '*'|MAGIC_FLAG:
		case '*'|BACKSLASH:
			/*
			 *  STAR	- match zero or more occurrences of the preceeding RE
			 *
			 *	re:	RE *
			 *	comp:	JMP  offset_PLUS  <RE>  PLUS  offset_<RE>
			 */
			if (last_token == 0)
				goto ord_char;

			shift(last_token, ep, 3);
			ep += 3;
			*last_token++ = JMP;
			split16(last_token, ep - last_token);	/* fill in offset following JMP (to PLUS) */
			last_token += 2;

			/* fall into PLUS ... */

			/*
			 *  ERE PLUS	- match one or more occurrences of the preceeding RE
			 *
			 *	re:	RE +
			 *	comp:	<RE>  PLUS  offset_<RE>
			 *
			 *    special case compilations handled below:
			 *
			 *	re:	. +
			 *	comp:	DOT DOT_PLUS
			 *
			 *	re:	c +
			 *	comp:	BYTE  'c'  BYTE_PLUS
			 *
			 *	re:	. + $			(or end of RE instead of '$')
			 *	comp:	DOT DOT_PLUS_END
			 *
			 *	re:	c + $			(or end of RE instead of '$')
			 *	comp:	BYTE BYTE_PLUS_END
			 *
			 *    special case compilations handled at "ord_char:":
			 *
			 *	re:	RE + c
			 *	comp:	<RE>  PLUS_BYTE  offset_<RE>  BYTE  'c'
			 *
			 *	re:	. + c
			 *	comp:	DOT DOT_PLUS_BYTE  BYTE  'c'
			 *
			 *	re:	c + d
			 *	comp:	BYTE  'c'  BYTE_PLUS_BYTE  BYTE  'd'
			 *
			 */
			*ep = PLUS;
			opt_token = ep;
			split16(ep+1, ep - last_token + 1);	/* fill in offset following PLUS (to RE) */

			/* that's the basic PLUS, now see if any optimizations can be done */
			if (*last_token == DOT || *last_token == MB_DOT) {
				if (*pp == 0 || (*pp == '$' && *(pp+1) == 0)) {
					/* we have an RE that ends with ".+" or ".+$" */
					*ep++ = DOT_PLUS_END;
					if (*pp)
						pp++;
				} else
					/* we have a ".+" in the middle of an RE */
					*ep++ = DOT_PLUS;
			} else
			if (*last_token == BYTE) {
				if (*pp == 0 || (*pp == '$' && *(pp+1) == 0)) {
					/* we have an RE that ends with "c+" or "c+$" */
					*ep++ = BYTE_PLUS_END;
				} else
					/* we have a "c+" in the middle of an RE */
					*ep++ = BYTE_PLUS;
			} else
				/* just a normal PLUS */
				ep += 3;

			last_token = 0;				/* no RE dup operators allowed next */
			continue;

		case '^':
		case '^'|MAGIC_FLAG:
			/*
			 *  CARET	- anchor RE to beginning of line
			 *
			 *	re:	^ RE
			 *	comp:	ANCHOR_BEGIN  <RE>
			 */
			if (pp-1 != pattern)			/* RE: ^ only special at start of RE */
				goto ord_char;

			*ep++ = ANCHOR_BEGIN;
			last_token = 0;
			continue;


		case '$':
		case '$'|MAGIC_FLAG:
			/*
			 *  DOLLAR	- anchor RE to end of line
			 *
			 *	re:	RE $
			 *	comp:	<RE>  ANCHOR_END
			 */
			if (*pp != '\0')			/* RE: $ only special at end of RE */
				goto ord_char;
			*ep++ = ANCHOR_END;
			last_token = 0;
			continue;

		case '['|MAGIC_FLAG:
		case '['|BACKSLASH:

			/*
			**  The compilation for '[[:digit:]a-zAB[.ch.]-]' where '[:digit:]' is
			**  the syntax for matching any char for which isdigit(char) is true and
			**  where '[.ch.]' is the syntax for specifying the 2-to-1 char "ch":
			**
			**     <TOKEN> <Bh> <Bl> <Ch> <Cl> <F> <F> x(a) x(z) <F> A <F> B <F> c h <F> -
			**
			**  with <Ch> & <Cl> specifying the number of flagged sub-expressions,
			**  <Bh> & <Bl> specifying the offset to the following expression,
			**  <F> being a flag describing the subexpression and any following
			**  characters, and x(c) being the position of the character `c' in the
			**  collation sequence.
			**
			**  <F> is defined as:
			**      b7:	if set, subexpression is a ctype match
			**      b6:	if set, subexpression is a range (2 collation numbers
			**		   following (3 bytes each))
			**      b5:	if set, subexpression is a single char match
			**	(one and only one of b5, b6, or b7 must be set)
			**
			**	if b7 is set:
			**		b0-b4:	 0 = isalpha
			**			 1 = isupper
			**			 2 = islower
			**			 3 = isdigit
			**			 4 = isxdigit
			**			 5 = isalnum
			**			 6 = isspace
			**			 7 = ispunct
			**			 8 = isprint
			**			 9 = isgraph
			**			10 = iscntrl
			**			11 = isascii
			**			12-31 = reserved for future ctype functions
			**
			**	if b5 set:
			**		b0-b1:  0 = 1st char following is a single-byte char
			**			1 =                         kanji
			**			2 =                         2-to-1
			**  
			**  Other new NLS syntax extensions are compiled as single char or range
			**  matches.  For example:
			**
			**		new syntax	compiled as
			**
			**		[=a=]		a range from "low a" to "high a"
			**		[=a=]-[=z=]	a range from "low a" to "high z"
			**		[=a=]-[.ch.]	a range from "low a" to the 2-to-1 "ch"
			**		[.a.]		a single char match of "a"
			**		[=ch=]		a range from "low ch" to "high ch"
			*/

			opt_token = last_token = ep;
			*ep++ = MATCH_LIST;

			lchar.c1 = *pp++;
			if (lchar.c1 == '^') {
			        *(ep-1) = NON_MATCH_LIST;
				lchar.c1 = *pp++;
			}

			cnt_ep = ep;
			ep += 4;
			if (ep >= endbuf)
			        return(REG_ESPACE);
			exprcnt = 0;

			range = 0;

			do {

				/* get one logical character */

				if (lchar.c1 == '\0')
					return(REG_EBRACK);

				if (IS_FIRST(lchar.c1)) {					/* kanji? */
					lchar.c2 = *pp++;
					lchar.flag = 1;						/* kanji */
					lchar.class = 1;					/* as itself */
				} else

				if (lchar.c1 == '[' && ((c = *pp) == '.' || c == ':' || c == '=')) {

					/*
					 * Eat the [.:=] and save the start of the extended syntax
					 * argument.
					 */
					psave = ++pp;

					/*
					 * scan for closing expression (".]", ":]", "=]")
					 */
					while (((c2 = *pp++) != '\0') && ((c2 != c) || (*pp != ']'))) {

						if (IS_FIRST(c2))
							pp++;
					}
					if (c2 == '\0')
						return(REG_EBRACK);				/* no closing expression found */

					pp++;							/* eat the closing ']' */

					/* process each type of new syntax as appropriate */
					lchar.class = 1;					/* presume a collating symbol */
					switch (c) {

						/*
						 * Equivalence Class
						 */
					case '=':
						lchar.class++;					/* equivalence class code is = 2 */

						/*
						 * Collating Symbol
						 */
					case '.':
						/*
						 * both coll sym & equiv class need a coll ele argument
						 * hence the common code
						 */
						lchar.c1 = *psave;
						switch ((pp - psave) - 2) {

						case 0:
							/* found "[..]" or "[==]" */
							return(REG_ECOLLATE);

						case 1:
							/* found a single one-byte character */
							lchar.flag = 0;				/* single char */
							/* error if not a collating element */
							if (!_collxfrm(lchar.c1 & TRIM, 0, 0, (int *)0))
								return(REG_ECOLLATE);		/* not a collating element */
							break;

						case 2:
							/* found a 2-to-1 or a kanji or 2 bytes of junk */
							lchar.c2 = *(psave+1);
							/* is 1st char possibly the 1st char of a 2-to-1? */
							if (_nl_map21) {
								/* check if a 2-to-1 collating element */
								if (!_collxfrm(lchar.c1 & TRIM, lchar.c2 & TRIM, 0, &two_to_1) || !two_to_1)
									return(REG_ECOLLATE);	/* not a 2-to-1 collating element */
								lchar.flag = 2;			/* 2-to-1 */
							} else if (IS_FIRST(lchar.c1)) {
								/* error if not a collating element */
								if (!_collxfrm(((lchar.c1 & TRIM)<<8) | (lchar.c2 & TRIM), 0, 0, (int *)0))
									return(REG_ECOLLATE);	/* not a collating element */
								lchar.flag = 1;			/* kanji */
							} else
								return(REG_ECOLLATE);		/* found "[.ju.]" or "[=ju=]" */
							break;

						default:
							return(REG_ECOLLATE);			/* found "[.junk.]" or "[=junk=]" */
						}
						break;

					/*
					 * Character Class
					 */
					case ':':
						lchar.class = 3;				/* character class */

						if (pp - psave == 2)
							return(REG_ECTYPE);			/* found "[::]" */

						/*
						 * Match name in RE against supported ctype functions
						 */
						for (i=0; i<7;) {				/* make name into regular string */
							pcopy[i] = psave[i] & TRIM;
							if (psave+(i++) == pp-3)
								break;
						}
						pcopy[i] = '\0';
						i = 0;
						while ((i < sizeof(ctype_funcs)/sizeof(char *)) && strcmp((char *)pcopy, (char *)ctype_funcs[i]))
							i++;

						if (i >= sizeof(ctype_funcs)/sizeof(char *))
							return(REG_ECTYPE);			/* found "[:garbage:]" */
						else {
							lchar.flag = i;				/* ctype macro */
						}

						break;
					}

				} else {
					/*
					 * Got just a character by itself
					 */

					lchar.flag = 0;						/* single char */
					lchar.class = 1;					/* as itself */
				}

				/*
				 * Now we have one logical character, figure out what
				 * to do with it.
				 */

				if (lchar.class == 3) {						/* character class? */

					if (range)
						return(REG_ERANGE);				/* ctype can't be range endpoint */
					/*
					 * generate compiled subexpression for a ctype class
					 */
					ep += gen_ctype(&lchar, ep, endbuf, &re_error);
					if (re_error)
						return(re_error);
					exprcnt++;
					if (*pp == '-') {					/* dash following? */
						pp++;
						if (*pp != ']')
							return(REG_ERANGE);			/* ctype can't be range endpoint */
						else {
							/* output dash as single match */
							lchar.c1 = '-';
							lchar.flag = 0;
							lchar.class = 1;
							ep += gen_single_match(&lchar, ep, endbuf, &re_error);
							if (re_error)
								return(re_error);
							exprcnt++;
						}
					}
				}

				else if (range) {

					/*
					 * generate compiled subexpression for a range
					 */
					ep += gen_range(&pchar, &lchar, ep, endbuf, &re_error);
					if (re_error)
						return(re_error);
					exprcnt++;
					range = 0;
					if (*pp == '-') {					/* dash following? */
						pp++;
						pchar = lchar;
						range++;
						no_prev_range = 0;
					}
				}

				else if (*pp == '-') {						/* dash following? */
					pp++;
					pchar = lchar;
					range++;
					no_prev_range++;
				}

				else {
					/*
					 * generate compiled subexpression for a single match
					 */
					ep += gen_single_match(&lchar, ep, endbuf, &re_error);
					if (re_error)
						return(re_error);
					exprcnt++;
				}

			} while ((lchar.c1 = *pp++) != ']' && ep < endbuf);

			if (ep >= endbuf)
			        return(REG_ESPACE);

			if (range) {
				if (no_prev_range) {
					/* output last char as single match */
					ep += gen_single_match(&pchar, ep, endbuf, &re_error);
					if (re_error)
						return (re_error);
					exprcnt++;
				}

				/* output dash as single match */
				lchar.c1 = '-';
				lchar.flag = 0;
				lchar.class = 1;
				ep += gen_single_match(&lchar, ep, endbuf, &re_error);
				if (re_error)
					return (re_error);
				exprcnt++;
			}

			split16(cnt_ep, ep - cnt_ep);
			split16(cnt_ep+2, exprcnt);

			continue;


		case '('|BACKSLASH:
		case '('|BACKSLASH|MAGIC_FLAG:
			/*
			 *  Open Parentheses:
			 *
			 *     RE:    \( re1 \( re2 \) \)   ->
			 *                    ^
			 *                    pp
			 *
			 *     current:    LP  re1
			 *                          ^
			 *                          ep
			 *
			 *     next:       LP  re1  LP
			 *                              ^
			 *                              ep
			 *
			 *     final:      LP  re1  LP  re2  RP  RP
			 *
			 *   where LP is "LPAREN"
			 *   and   RP is "RPAREN"
			 */
			if (++nparen > REG_NPAREN)		/* if number of parens exceeds limit */
				return(REG_ENSUB);		/* report error: too many \( \) pairs */

			level++;
			*ep++ = LPAREN;				/* write the token */
			opt_token = last_token = 0;

			continue;


		case ')'|BACKSLASH:
		case ')'|BACKSLASH|MAGIC_FLAG:
			/*
			 *  Close Parentheses:
			 *
			 *     RE:    \( re1 \( re2 \) \)   ->
			 *                           ^
			 *                           pp
			 *
			 *     current:    LP  re1  LP  re2
			 *                                   ^
			 *                                   ep
			 *
			 *     next:       LP  re1  LP  re2  RP
			 *                                       ^
			 *                                       ep
			 *
			 *     final:      LP  re1  LP  re2  RP  RP
			 *
			 *   where LP is "LPAREN"
			 *   and   RP is "RPAREN"
			 */
			if (level == 0)				/* if no preceeding open paren */
				return(REG_EPAREN);		/* report error: parentheses imbalance */

			*ep++ = RPAREN;				/* write regular paren token */
			closed++;				/* another paren pair completed */

			opt_token = last_token = 0;
			level--;
			continue;


		case '<'|BACKSLASH:
		case '<'|BACKSLASH|MAGIC_FLAG:
			/*
			 *  Begin word - match the beginning of a word:
			 *
			 *	re:	\<
			 *	comp:	WORD_BEGIN
			 */
			opt_token = last_token = ep;
			*ep++ = WORD_BEGIN;
			continue;


		case '>'|BACKSLASH:
		case '>'|BACKSLASH|MAGIC_FLAG:
			/*
			 *  End word - match the end of a word:
			 *
			 *	re:	\>
			 *	comp:	WORD_END
			 */
			opt_token = last_token = ep;
			*ep++ = WORD_END;
			continue;


		case '~'|MAGIC_FLAG:
		case '~'|BACKSLASH:
			/*
			 *  Match replacement part of last substitute command
			 *
			 *	re:	~
			 *	comp:	RE_STRING  byte_count  repl_chars
			 */
			opt_token = last_token = ep;
			*ep++ = RE_STRING;
			ep++;

			rhsp = rhsbuf;
			i = 0;
			while (*rhsp) {
				if (*rhsp & QUOTE) {
					c = *rhsp & (TRIM | FIRST | SECOND);
					if (c == '&')
error((nl_msg(23, "Replacement pattern contains &|Replacement pattern contains & - cannot use in re")));
					if (c >= '1' && c <= '9')
error((nl_msg(24, "Replacement pattern contains \\d|Replacement pattern contains \\d - cannot use in re")));
				}
				if (ep >= endbuf)
					return REG_ESPACE;
				if (i > 253) {
					*(opt_token+1) = i;
					opt_token = ep;
					*ep++ = RE_STRING;
					i = 0;
				}
				i++;
				*ep++ = *rhsp++ & TRIM;
				/* 2nd byte of 16-bit char */
				if (IS_SECOND(*rhsp)) {
					i++;
					*ep++ = *rhsp++ & TRIM;
				}
			}
			*(opt_token+1) = i;
			continue;


/* Not supported by ex/vi at this time:
/*
/*		case '{'|BACKSLASH:
/*		case '{'|BACKSLASH|MAGIC_FLAG:
/*			if (last_token == 0) 
/*				goto ord_char;
/*
/*			cflg = 0;
/*		nlim:
/*			c = *pp++;
/*			i = 0;
/*			do {
/*				if ('0' <= c && c <= '9')
/*					i = 10 * i + c - '0';
/*				else
/*					return(REG_EABRACE);
/*			} while (((c = *pp++) != '\\') && (c != ','));
/*			if (i > 255)
/*				return(REG_EBBRACE);
/*			n = i;
/*			if (c == ',') {
/*				if (cflg++)
/*					return(REG_ECBRACE);
/*				m = n;
/*				if (*pp == '\\') {
/*					pp++;
/*					n = 65534+m;
/*				} else
/*					goto nlim;					/* get 2'nd number */
/*			}
/*			if (*pp++ != '}')
/*				return(REG_EBRACE);
/*			if (!cflg)							/* one number */
/*				m = i;
/*			else if (n < m)
/*				return(REG_EDBRACE);
/*		/*
/*		 *  re1 \{m,n\} -> RANGE m re1 END_RANGE n-m+1 offset_to_re1
/*		 *
/*		 *  re1 \{0,n\} -> RANGE 0 offset_END_RANGE+1 re1 END_RANGE n+1 offset_to_re1
/*		 */
/*			*ep++ = END_RANGE;
/*			split16(ep, n-m+1);
/*			split16(ep+2, ep - last_token);	/* offset to re1 from END_RANGE index */
/*			if (m) {
/*				shift(last_token, ep+4, 2);
/*				ep += 6;
/*			} else {
/*				shift(last_token, ep+4, 4);
/*				ep += 8;
/*			}
/*			*last_token++ = RANGE;
/*			*last_token++ = m;
/*			if (!m)
/*				split16(last_token, ep - last_token - 4);
/*			opt_token = last_token = 0;
/*			continue;
*/

		ord_char:
		default:

/* Not supported by ex/vi at this time:
/*
/*			c &= ~MAGIC_FLAG;
/*			if (re && c >= ('1'|BACKSLASH) && c <= ('9'|BACKSLASH)) {
/*				if ((c -= ('1'|BACKSLASH)) >= closed)
/*					return(REG_ESUBREG);
/*				last_token = ep;
/*				*ep++ = MATCH_SAME;
/*				*ep++ = c & TRIM;
/*				continue;
/*			}
*/

			c = c & 0xffff;				/* strip off flags to get just the char */

			last_token = ep;

			if (IS_FIRST(c)) {
				*ep++ = KANJI;
				*ep++ = c & TRIM;
				*ep++ = *pp++ & TRIM;
			} else {
				*ep++ = BYTE;
				*ep++ = c & TRIM;
			}

			/*
			 *  Basic work done, now set up and/or do some optimizations
			 *  for performance if appropriate.
			 */

			if (last_token == expbuf)		/* if first token of RE leave alone */
				continue;

			/* if dup operator other than PLUS following can't do remaining optimizations */
			if (*pp == '*' || (*pp == '\\' && *(pp+1) == '{')) {
				sp = string;
				continue;
			}

			/* convert various PLUS forms followed by BYTE to special versions */
			if (opt_token && *last_token == BYTE) {
				if (*opt_token == PLUS) {
					*opt_token = PLUS_BYTE;
					opt_token = 0;
					sp = string;
					continue;
				} else
				if (*opt_token == DOT_PLUS) {
					*opt_token = (not_multibyte) ? DOT_PLUS_BYTE : MB_DOT_PLUS_BYTE;
					opt_token = 0;
					sp = string;
					continue;
				} else
				if (*opt_token == BYTE_PLUS) {
					*opt_token = BYTE_PLUS_BYTE;
					opt_token = 0;
					sp = string;
					continue;
				}
			}

			/* the next optimizations are for consecutive RE_STRING/BYTE/KANJI tokens */
			if (*sp && (!opt_token || (*opt_token != RE_STRING && *opt_token != BYTE && *opt_token != KANJI))) {
				sp = string;
				*++sp = opt_token = last_token;
				continue;
			}

			/* nothing to merge if stack is empty, put current token on stack and move on */
			if (*sp == 0) {
				*++sp = opt_token = last_token;
				continue;
			}

			/* try to merge BYTEs and KANJIs into RE_STRINGs */
			xp = *sp--;					/* pop the string stack */

			if (*xp == RE_STRING) {
				/* Add to existing RE_STRING */
				int cnt = *++xp;			/* length of RE_STRING */
				if (*last_token == BYTE) {
					if (cnt < 65535) {		/* if not too long already */
						*xp = cnt + 1;		/* merge BYTE /w existing */
						last_token = xp-1;	/* RE_STRING */
						*(ep-2) = *(ep-1);
						ep--;
					}
				} else {
					if (cnt < 65534) {		/* if not too long already */
						*xp = cnt + 2;		/* merge KANJI /w existing */
						last_token = xp-1;	/* RE_STRING */
						*(ep-3) = *(ep-2);
						*(ep-2) = *(ep-1);
						ep--;
					}
				}

			} else if (*xp == KANJI) {
				/* Convert KANJI-BYTE or KANJI-KANJI into a new RE_STRING */
				int cnt = *last_token == BYTE ? 3 : 4;	/* KANJI-BYTE vs KANJI-KANJI */
				last_token = xp;
				*xp++ = RE_STRING;
				*(xp+2) = *(xp+1);			/* shift 1st kanji to make */
				*(xp+1) = *xp;				/* room for string count */
				*xp = cnt;				/* length of RE_STRING */

			} else if (*last_token == KANJI) {
				switch (sp - string) {
				case 0:
					/* Convert BYTE-KANJI into a new RE_STRING */
					last_token = xp;
					*xp++ = RE_STRING;
					*(xp+1) = *xp;
					*xp = 3;
					break;
				case 1:
					/* Convert BYTE-BYTE-KANJI into a new RE_STRING */
					sp--;
					*xp = *(xp-1);
					*--xp = 4;
					*--xp = RE_STRING;
					*last_token = *(last_token+1);
					*(last_token+1) = *(last_token+2);
					last_token = xp;
					ep--;
					break;
				case 2:
					/* Convert BYTE-BYTE-BYTE-KANJI into a new RE_STRING */
					*xp = *(xp+1);
					*(xp+1) = *(last_token+1);
					*last_token = *(last_token+2);
					xp = *sp--;
					sp--;
					*xp-- = *(xp-1);
					*xp-- = 5;
					*xp = RE_STRING;
					last_token = xp;
					ep -= 2;
					break;
				}

			} else if (sp - string > 1) {
				/* Convert BYTE BYTE BYTE BYTE into a new RE_STRING */
				unsigned char *np = *--sp;		/* 1st BYTE */
				sp--;					/* empty stack */
				last_token = np;
				*np++ = RE_STRING;
				*(np+1) = *np;				/* move chars around */
				*xp = *(xp+1);
				*(xp+1) = *(xp+3);
				*np = 4;				/* string count */
				ep -= 2;

			} else
				/* Nothing for now */
				*++sp = xp;				/* didn't use, return to stack */

			/* add new token to stack */
			*++sp = opt_token = last_token;
		}
	}
}


#define SAME(a, b)	((a) == (b) || value(IGNORECASE) && _tolower(a) == _tolower(b))

#define MB_SAME(a, b)	((a) == (b) || value(IGNORECASE) && _ISANK(a) && _ISANK(b)	\
			&& _tolower(a) == _tolower(b))

#define push(xp)	{								\
				register int parens_cnt;				\
				/* bring data on top of stack up to date */		\
				sip->ep = xp;						\
				sip->lp = lp;						\
				sip->parens_cnt = parens_cnt = parens - sip->parens;	\
				sip->lparen = lparen;					\
				sip->ignore_paren = ignore_paren;			\
				/* not supported: sip->range_cnt = range_cnt;	*/	\
				/* move stack up one level */				\
				sp += sizeof(stack_items)/sizeof(unsigned char *);	\
				/* copy basic items to new top of stack */		\
				*(stack_items *)sp = *sip;				\
				/* reset ptrs to use new top of stack as work area */	\
				sip = (stack_items *)sp;				\
				parens = sip->parens + parens_cnt;			\
				lparens = sip->lparens;					\
				rparens = sip->rparens;					\
			}

#define pop		{								\
				sp -= sizeof(stack_items)/sizeof(unsigned char *);	\
				sip = (stack_items *)sp;				\
				ep = sip->ep;						\
				lp = sip->lp;						\
				lparen = sip->lparen;					\
				ignore_paren = sip->ignore_paren;			\
				/* not supported: range_cnt = sip->range_cnt;	*/	\
				parens = sip->parens + sip->parens_cnt;			\
				lparens = sip->lparens;					\
				rparens = sip->rparens;					\
			}

typedef struct {
	unsigned char	*ep;
	CHAR		*lp;
	CHAR		*lparens[REG_NPAREN];
	CHAR		*rparens[REG_NPAREN];
	unsigned char	lparen;
	unsigned char	parens[REG_NPAREN];
	unsigned char	parens_cnt;
	unsigned int	ignore_paren;
/* Not supported by ex/vi at this time:
/*	int		range_cnt;
*/
} stack_items;


regexec(string)
const CHAR	*string;
{
	CHAR			*string_p = string;
	register CHAR		*lp = string;
	register unsigned char	*start_ep = expbuf;
	register unsigned char	*ep = start_ep;

	unsigned int	c;
	int		negate = 0;
	register int	not_multibyte = (__nl_char_size == 1);
	unsigned int	start_c = 0;
	unsigned int	start_kanji = 0;

	unsigned char	*stack[STACK_SIZE];
	unsigned char	**sp = stack;			/* ptr to top element on stack */
	stack_items	*sip;				/* ptr to misc portion of top stack element */
	unsigned char	*parens;
	CHAR		**lparens;
	CHAR		**rparens;
	unsigned char	lparen = 0;
	unsigned int	ignore_paren = 0;
	CHAR		*best_lp = 0;
	unsigned char	best_lparen;
/* Not supported by ex/vi at this time:
/*	int		range_cnt = 0;
*/

	int		cnt;

	sip = (stack_items *)sp;
	parens = sip->parens;
	lparens = sip->lparens;
	rparens = sip->rparens;

	if (*ep == BYTE && not_multibyte && !circf) {
		start_c = *++ep;
		start_ep = ++ep;
		while (*string_p && !SAME(*string_p & TRIM, start_c))
			string_p++;
		if (*string_p)
			lp = string_p + 1;
		else
			return (REG_NOMATCH);
	} else if (!not_multibyte && (*ep == KANJI || *ep == BYTE) && !circf) {
		if (*ep == KANJI) {
			start_kanji = cvt16(ep+1);
			start_ep = ep += 3;
		} else {
			start_kanji = *++ep;
			start_ep = ++ep;
		}
		do {
			string_p = lp;
			c = _CHARADV(lp);
		} while (!MB_SAME(c, start_kanji) && *string_p);
		if (!*string_p)
			return (REG_NOMATCH);
	}

	while (1) {
		switch(*ep++) {

		case ANCHOR_BEGIN:
			if (lp == string)
				continue;
			else {
				return (REG_NOMATCH);
			}

		case ANCHOR_END:
			if (*lp == '\0')
				continue;
			goto fail;

		case BYTE:
			if (SAME(*ep, *lp & TRIM)) {
				ep++;
				lp++;
				continue;
			}
			goto fail;

		case KANJI:
			c = cvt16(ep);
			if (c == _CHARADV(lp)) {
				ep += 2;
				continue;
			}
			goto fail;

		case RE_STRING:
			/*
			 * no more than 65535 bytes per string
			 *  RE_STRING byte_count string_data */
			cnt = *ep++;
			while (SAME(*ep, *lp & TRIM) && --cnt) {
				ep++;
				lp++;
			}
			if (cnt)
				goto fail;
			else {
				ep++;
				lp++;
				continue;
			}

		case DOT:
			if (*lp++)
				continue;
			goto fail;

		case MB_DOT:
			if (_CHARADV(lp))
				continue;
			goto fail;

# define	LDU(c)	(((c >= IS_MACRO_LOW_BOUND) && (isalpha(c & TRIM) || isdigit(c & TRIM))) || c == '_')

	        case WORD_BEGIN:
#ifndef	NLS16
			if (LDU(*lp) && (lp == linebuf || !LDU(lp[-1])))
#else
			if (wordch(lp) && (lp == linebuf || wordch(lp - 1) != wordch(lp)))
#endif
				continue;
			goto fail;

	        case WORD_END:
#ifndef	NLS16
			if (!LDU(*lp))
#else
			if (!wordch(lp) || (lp != linebuf && wordch(lp - 1) && wordch(lp) != wordch(lp - 1)))
#endif
				continue;
			goto fail;

	        case NON_MATCH_LIST:
			negate = 1;
		case MATCH_LIST:
			c = (not_multibyte) ? *lp++ & TRIM : _CHARADV(lp);
			if (c = isthere(c, *lp & TRIM, ep+2, negate)) {
				negate = 0;
				ep += cvt16(ep);
				/*
				 * three possibilities at this point:
				 *    1) a single char was matched
				 *    2) a 2-to-1 was matched
				 *    3) the 2-to-1 was matched and its 1st char was
				 *       also matched as a single char
				 */
				if (c == 3)			/* both case 1 and 2? */
					push(ep);		/* yes, save case 1 for later */
				if (c & 2)
					lp++;			/* matched a 2-to-1, advance past 2nd char */
				continue;
			}
			negate = 0;
			goto fail;

/* Not supported by ex/vi at this time:
/*
/*		case MATCH_SAME:
/*			{
/*				CHAR	*cp = lparens[*ep];
/*
/*				cnt = rparens[*ep++] - cp;
/*
/*				while (cnt && *cp++ == (*lp++ & TRIM)) cnt--;
/*
/*				if (cnt)
/*					goto fail;
/*				else
/*					continue;
/*			}
*/

		case PLUS:
			/*
			 *  Plus operator: match the RE one or more times
			 *
			 *  re1 + re2 -> re1 PLUS offset_to_re1 re2
			 *                               |
			 *                               ep
			 *
			 *  Completed one iteration through the RE.
			 *  Go back and try another iteration but first save the start of
			 *  the following RE on the top of stack.  When a final iteration of
			 *  the PLUS'd RE doesn't produce a match we will process the remainder
			 *  of the total RE by popping the saved ep/lp values one by one.
			 */
			push(ep+2);
			ep -= cvt16(ep);
			continue;

		case DOT_PLUS:
			/*	. + re2		DOT DOT_PLUS <re2>	*/
			while (*lp) {
				push(ep);
				lp++;
			}
			continue;

		case BYTE_PLUS:
			/*	a + re2		BYTE a BYTE_PLUS <re2>	*/
			while (SAME(*lp & TRIM, *(ep-2))) {
				push(ep);
				lp++;
			}
			continue;

		case PLUS_BYTE:
			/*	re1 + a	re2	<re1> PLUS_BYTE offset_to_re1 BYTE a <re2>	*/
			if (SAME(*(ep+3), *lp & TRIM))
				push(ep+2);
			ep -= cvt16(ep);
			continue;

		case DOT_PLUS_BYTE:
			/*	. + a		DOT DOT_PLUS_BYTE BYTE a	*/
			while (*lp) {
				if (SAME(*(ep+1), *lp & TRIM))
					push(ep);
				lp++;
			}
			goto fail;

		case MB_DOT_PLUS_BYTE:
			/*	. + a		DOT MB_DOT_PLUS_BYTE BYTE a	*/
			while (*lp) {
				if (SAME(*(ep+1), *lp & TRIM))
					push(ep);
				PST_INC(lp);
			}
			goto fail;

		case BYTE_PLUS_BYTE:
			/*	a + b		BYTE a BYTE_PLUS_BYTE BYTE b	*/
			while (SAME(*lp & TRIM, *(ep-2))) {
				lp++;
				if (SAME(*(ep+1), *lp & TRIM))
					push(ep);
			}
			continue;

		case DOT_PLUS_END:
			/*	. + $		DOT DOT_PLUS_END END_RE		*/
			while (*lp)
				lp++;
			continue;

		case BYTE_PLUS_END:
			/*	a + $ \0	BYTE a BYTE_PLUS_END ANCHOR_END END_RE		*/
			/*	a + \0		BYTE a BYTE_PLUS_END END_RE			*/
			while (SAME(*(ep-2), *lp & TRIM))
				lp++;
			continue;

/* Not supported by ex/vi at this time:
/*
/*		case RANGE:
/*			/*
/*			 *  Start of Range: match the RE at least m but not more than n times
/*			 *
/*			 *  re1 \{m,n\} -> RANGE m re1 END_RANGE n-m+1 offset_re1
/*			 *                       |
/*			 *                       ep
/*			 *
/*			 *  re1 \{0,n\} -> RANGE 0 offset_END_RANGE+1 re1 END_RANGE n+1 offset_re1
/*			 *                       |
/*			 *                       ep
/*			 *
/*			 *  Activate the required iteration count for this range expression.
/*			 *  If there are minimum required iterations, proceed to process the RE.
/*			 *  Otherwise skip to the end of the range to begin processing the optional
/*			 *  iterations.
/*			 */
/*			if (range_cnt = -(*ep++))		/* init required iteration count */
/*				continue;				/* and continue with re1 */
/*			else {					/* but if no required iterations */
/*				range_cnt = -1;				/* pretend we had just one */
/*				ep += cvt16(ep);			/* skip to END_RANGE token */
/*			}						/* and fall into its processing */
/*
/*		case END_RANGE:
/*			/*
/*			 *  End of Range
/*			 *
/*			 *  re1 \{m,n\} -> RANGE m re1 END_RANGE n-m+1 offset_to_re1
/*			 *                                         |
/*			 *                                         ep
/*			 *
/*			 *  re1 \{0,n\} -> RANGE 0 offset_END_RANGE+1 re1 END_RANGE n+1 offset_to_re1
/*			 *                                                           |
/*			 *                                                           ep
/*			 *
/*			 *  If working on required iterations:
/*			 *     count one more as done
/*			 *     if more required iterations are needed go back and do another
/*			 *     otherwise move on to optional iterations
/*			 *  If working on optional iterations:
/*			 *     count one more as done
/*			 *     if more optional iterations are needed
/*			 *        save position of following RE on stack in case the additional iterations
/*			 *           don't work out
/*			 *        go back and do another iteration
/*			 *     continue with RE following range expression
/*			 */
/*			if (++range_cnt < 0) {			/* matched another required iteration */
/*				ep -= cvt16(ep+2);		/* if more, go back to start of re1 */
/*				continue;
/*			}
/*
/*			if (range_cnt == 0)			/* matched all required iterations */
/*				range_cnt = cvt16(ep);		/* get number of optional iterations + 1 */
/*
/*			if (--range_cnt) {			/* if more remain after this one */
/*				range_cnt--;			/* decrement the number remaining */
/*				push(ep+4);			/* save the following RE for later and */
/*				ep -= cvt16(ep+2);		/* go back for another iteration of re1 */
/*				continue;
/*			}
/*
/*			ep += 4;
/*			continue;
*/

		case LPAREN:
			if (lparen < REG_NPAREN) {
				*parens++ = lparen;
				lparens[lparen++] = lp;
			} else
				ignore_paren++;
			continue;

		case RPAREN:
			if (ignore_paren)
				ignore_paren--;
			else
				rparens[*--parens] = lp;
			continue;

		case JMP:
			ep += cvt16(ep);
			continue;

		case END_RE:
			if (lp > best_lp) {
				/* got a match longer than any previous match */
				best_lp = lp;
				best_lparen = lparen;
				/* copy working paren arrays to best paren arrays */
				best_parens = *(lr_parens *)(sip->lparens);
			}

			/* see if just any match will do or if we've already got the whole line */
			if (*lp == 0)
				goto success;

		fail:
			if (sp != stack) {
				/*
				 *  Some other iterations or alternatives remain to be tested.  Pop the
				 *  the most recent one and continue.
				 */
				pop;
				continue;
			}

			/* done */
			if (best_lp) {
				/* return success, ... */
		success:
				loc1 = string_p;				/* start of matched string */
				loc2 = best_lp;					/* end of matched string */
				return (0);

			} else {
				/* return fail */

				if (circf || *string_p == 0)
					return (REG_NOMATCH);
				else {

					/*
					 * Note: this is set up so that a null string gets processed
					 * which is necessary to allow REs such as "x*$" to succeed.
					 */

					if (not_multibyte)
						string_p++;
					else
						PST_INC(string_p);

					lp = string_p;
					ep = start_ep;
					lparen = 0;
					ignore_paren = 0;
					parens = sip->parens;

					if (start_c) {
						while (*string_p && !SAME(*string_p & TRIM, start_c))
							string_p++;
						if (*string_p)
							lp = string_p + 1;
						else
							return (REG_NOMATCH);
					} else if (start_kanji) {
						do {
							string_p = lp;
							c = _CHARADV(lp);
						} while (!MB_SAME(c, start_kanji) && *string_p);
						if (!*string_p)
							return (REG_NOMATCH);
					}

					continue;
				}
			}
		}
        }
}



static isthere(c1, c2, ep, negate)
register unsigned int		c1, c2;
register const unsigned char	*ep;
{
	int exprcnt;
	unsigned int x1, x2, e1, e2, flag;
	int match_1_char, match_2_to_1, two_to_1;

	if (c1 == 0)
	        return 0;

	if (value(IGNORECASE) && _ISANK(c1))
		c1 = _tolower(c1);
	if (value(IGNORECASE) && _ISANK(c2))
		c2 = _tolower(c2);

	exprcnt = (*ep++)<<8;
	exprcnt += *ep++;

	x1 = match_1_char = match_2_to_1 = 0;
	if (_nl_map21 && (_pritab[c1] & 0x40))
		x2 = _collxfrm(c1, c2, 0, &two_to_1);
	else
		x2 = two_to_1 = 0;

	while (exprcnt-- && !(match_1_char && (!two_to_1 || match_2_to_1))) {

		/* extract subexpression to be matched */
		flag = *ep++;

		/* if subexpression type is single */
		if (flag & 040) {
			e1 = *ep++;
			if (value(IGNORECASE))
				e1 = _tolower(e1);
			if (!(flag & 3))
				match_1_char = (c1 == e1);		/* single char match wanted */
			else if (flag & 1) {
				e1 = (e1 << 8) | *ep++;			/* kanji match wanted */
				match_1_char = (c1 == e1);
			} else {
				if (value(IGNORECASE))
					e2 = _tolower(e2);
				e2 = *ep++;				/* 2-to-1 match wanted */
				match_2_to_1 = (c1 == e1 && c2 == e2);
			}
		}

		/* else if subexpression type is ctype */
		else if (flag & 0200) {
/*			if (c1 < 256)	/* no ctype for kanji */
			if ((c1 >= IS_MACRO_LOW_BOUND) && (c1 < 256))	/* no ctype for kanji */
				switch (flag & 037) {
					case 0:		match_1_char = isalpha(c1); break;
					case 1:		match_1_char = isupper(c1); break;
					case 2:		match_1_char = islower(c1); break;
					case 3:		match_1_char = isdigit(c1); break;
					case 4:		match_1_char = isxdigit(c1); break;
					case 5:		match_1_char = isalnum(c1); break;
					case 6:		match_1_char = isspace(c1); break;
					case 7:		match_1_char = ispunct(c1); break;
					case 8:		match_1_char = isprint(c1); break;
					case 9:		match_1_char = isgraph(c1); break;
					case 10:	match_1_char = iscntrl(c1); break;
					case 11:	match_1_char = isascii(c1); break;
				}
		}

		/* else if subexpression type is range */
		else {
			/* 7/8-bit language with machine collation */
			if (!_nl_collate_on) {
				e1 = *ep++;
				e2 = *ep++;
				match_1_char = (c1 >= e1) && (c1 <= e2);
			} else {
			/* 8-bit NLS collation or 16-bit machine collation */
			/* on first range subexpression get collation transform */
				if (!x1)
					x1 = _collxfrm(c1, 0, 0, (int *)0);
				e1 = *ep++ << 16;
				e1 += *ep++ << 8;
				e1 += *ep++;
				e2 = *ep++ << 16;
				e2 += *ep++ << 8;
				e2 += *ep++;
				match_1_char = (x1 >= e1) && (x1 <= e2);
				match_2_to_1 = two_to_1 && (x2 >= e1) && (x2 <= e2);
			}
		}
	}

	/* negate matches as appropriate */
	if (negate) {
		match_1_char = !match_1_char;
		match_2_to_1 = !match_2_to_1 && two_to_1;
	}

	return ((match_1_char ? 1 : 0) | (match_2_to_1 ? 2 : 0));
}



/*
 *  Generate compiled subexpression for a range
 */
static gen_range(bchar, echar, ep, endbuf, re_error)
const struct logical_char	*bchar, *echar;
register unsigned char		*ep;
const unsigned char		*endbuf;
int				*re_error;
{

	register unsigned int c1, c2;
	register unsigned xfrm1, xfrm2;

	*re_error = 0;

	if (ep >= endbuf-7) {			/* need 7 bytes (1 for flag, 3 each for endpoints */
		*re_error = REG_ESPACE;
		return (0);
	}

	/* store flag for this range expression */
	*ep++ = 0100;

	/* 7/8-bit language with machine collation */
	if (!_nl_collate_on) {
		if ((bchar->c1 & TRIM) > (echar->c1 & TRIM)) {
			*re_error = REG_ERANGE;	/* 1st endpoint must be <= 2nd endpoint */
			return (0);
		}
		*ep++ = (unsigned char)(bchar->c1 & TRIM);
		*ep++ = (unsigned char)(echar->c1 & TRIM);
		return (3);
	}

	/* 8-bit NLS collation or 16-bit machine collation */

	/* get collation transformation of 1st range endpoint */
	c1 = bchar->c1 & TRIM;
	if (bchar->flag == 1)
		c1 = c1 << 8 | bchar->c2 & TRIM;
	c2 = (bchar->flag == 2) ? bchar->c2 & TRIM : 0;
	xfrm1 = _collxfrm(c1, c2, (bchar->class == 2) ? -1 : 0, (int *)0);
	if (xfrm1 == 0) {
		*re_error = REG_ECOLLATE;	/* range endpoint non-collating */
		return (0);
	}

	/* store 1st range endpoint */
	*ep++ = (unsigned char)(xfrm1 >> 16);
	*ep++ = (unsigned char)(xfrm1 >> 8);
	*ep++ = (unsigned char)(xfrm1);

	/* get collation transformation of 2nd range endpoint */
	c1 = echar->c1 & TRIM;
	if (echar->flag == 1)
		c1 = c1 << 8 | echar->c2 & TRIM;
	c2 = (echar->flag == 2) ? echar->c2 & TRIM : 0;
	xfrm2 = _collxfrm(c1, c2, (echar->class == 2) ? 1 : 0, (int *)0);
	if (xfrm2 == 0) {
		*re_error = REG_ECOLLATE;	/* range endpoint non-collating */
		return (0);
	}

	/* store 2nd range endpoint */
	*ep++ = (unsigned char)(xfrm2 >> 16);
	*ep++ = (unsigned char)(xfrm2 >> 8);
	*ep++ = (unsigned char)(xfrm2);

	if (xfrm1 > xfrm2) {
		*re_error = REG_ERANGE;		/* 1st endpoint must be <= 2nd endpoint */
		return (0);
	}

	return (7);
}



/*
 *  Generate compiled subexpression for a single match
 */
static gen_single_match(lchar, ep, endbuf, re_error)
const struct logical_char	*lchar;
register unsigned char		*ep;
const unsigned char		*endbuf;
int				*re_error;
{
	unsigned char *orig_ep = ep;

	*re_error = 0;

	if (lchar->class == 2) {
		/* output equiv class as a range */
		return gen_range(lchar, lchar, ep, endbuf, re_error);
	} else {
		/* output: char */
		if (ep >= endbuf-2) {
			*re_error = REG_ESPACE;
			return (0);
		}

		*ep++ = lchar->flag | 040;
		*ep++ = lchar->c1 & TRIM;
		if (lchar->flag) {		/* if kanji or 2-to-1 */
			*ep++ = lchar->c2 & TRIM;
			if (ep >= endbuf-1) {
				*re_error = REG_ESPACE;
				return (0);
			}
		}
	}
	return (ep - orig_ep);
}



/*
 *  Generate compiled subexpression for a ctype class
 */
static gen_ctype(lchar, ep, endbuf, re_error)
const struct logical_char	*lchar;
register unsigned char		*ep;
const unsigned char		*endbuf;
int				*re_error;
{
	*re_error = 0;

	if (ep >= endbuf-1) {
		*re_error = REG_ESPACE;
		return (0);
	}

	*ep = lchar->flag | 0200;
	return (1);
}
