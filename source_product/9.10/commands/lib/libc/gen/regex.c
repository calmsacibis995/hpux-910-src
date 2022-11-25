/* @(#) $Revision: 72.3 $ */    

#ifdef PIC
#pragma HP_SHLIB_VERSION "4/92"
#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define strcmp _strcmp
#define strncpy _strncpy
#define strlen _strlen
#define memset _memset
#define memcpy _memcpy
#define regcomp _regcomp
#define regexec _regexec
#define regfree _regfree
#define regerror _regerror
#define catopen _catopen
#define catgets _catgets
#define catclose _catclose
#define strchr  _strchr
#   ifdef _ANSIC_CLEAN
#define malloc _malloc
#define realloc _realloc
#define free _free
#   endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */


#include <sys/types.h>
#include <nl_types.h>
#include <nl_ctype.h>
#include "regex.h"
#include <limits.h>
#include <setlocale.h>

#define NL_SETN 1


#define uchar unsigned char
#define	ANCHOR_BEGIN	0010	/* match must be anchored at beginning of the line		*/
#define	ANCHOR_END	0020	/* match must be anchored at end of the line			*/
#define	BYTE		0030	/* match a single byte character				*/
#define	KANJI		0040	/* match a multibyte character					*/
#define	STRING		0050	/* match a multibyte character/string				*/
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
#define	MB_DOT_PLUS	0251	/* combination of MB_DOT and PLUS for optimization		*/
#define	BYTE_PLUS	0260	/* combination of BYTE and PLUS for optimization		*/
#define	PLUS_BYTE	0270	/* combination of PLUS and BYTE for optimization		*/
#define	DOT_PLUS_BYTE	0300	/* combination of DOT, PLUS and BYTE for optimization		*/
#define	MB_DOT_PLUS_BYTE 0310	/* same as DOT_PLUS_BYTE but for multibyte code sets		*/
#define	BYTE_PLUS_BYTE	0320	/* combination of BYTE, PLUS and BYTE for optimization		*/
#define	DOT_PLUS_END	0330	/* combination of DOT and PLUS at RE end for optimization	*/
#define	BYTE_PLUS_END	0340	/* combination of BYTE and PLUS at RE end for optimization	*/
#define	END_RE		0370	/* end of RE program						*/

#define	RE		01000	/* bit flag indicating basic regular expression processing	*/
#define	ERE		02000	/* bit flag indicating extended regular expression processing	*/

#define	BACKSLASH	010000	/* bit flag indicating character was backslash quoted		*/

#define	STACK_SIZE	2*10240	/* size of execution stack					*/
#define	NLEVELS		128	/* depth of parentheses nesting level allowed			*/
				/* NLEVELS cannot exceed REG_NPAREN !!!				*/
#define	REG_BUF_SIZE	1024	/* size of chunks allocated for RE buffer			*/

#ifdef DEBUG
int malloc_cnt=0;
uchar *Malloc(n)
{
	uchar *p;
	
	p=(uchar *)malloc(n);
	if (p) malloc_cnt++;
	printf("%11x Malloc(%d)\n",p,n);
	return(p);
}
uchar *Realloc(p,n)
uchar *p;
{
	printf("%11x ->Realloc(%d)\n",p,n);
	p=realloc(p,n);
	printf("%11x <-Realloc(%d)\n",p,n);
	return(p);
}
void Free(p)
uchar *p;
{
	printf("%11x Free\n",p);
	if (p) malloc_cnt--;
	free(p);
}
#else /* DEBUG */
#define Malloc malloc
#define Free free
#define Realloc realloc
uchar *malloc();
uchar *realloc();
#endif /* DEBUG */


extern unsigned int	_collxfrm();

struct logical_char {
	unsigned int	c1,
			c2,
			flag,
			class;
};

static uchar	*ctype_funcs[] = {
				(uchar *) "alpha",
				(uchar *) "upper",
				(uchar *) "lower",
				(uchar *) "digit",
				(uchar *) "xdigit",
				(uchar *) "alnum",
				(uchar *) "space",
				(uchar *) "punct",
				(uchar *) "print",
				(uchar *) "graph",
				(uchar *) "cntrl",
				(uchar *) "ascii",
				(uchar *) "blank"
			};


/* shift a string designated by head and tail ptrs cnt units towards the tail */
#define shift(head, tail, cnt)		{	\
	register uchar *from = (tail)-1;	\
	register uchar *to = from+(cnt);	\
	register uchar *quit = (head);		\
	do {					\
		*to-- = *from;			\
	} while (from-- != quit);		\
	}

/* split the 16-bit number "cnt" across two bytes at location "loc" */
#define split16(loc, cnt)	{		\
	register uchar *to = (loc);		\
	register int num = (cnt);		\
	*to++ = (num>>8) & 0377;		\
	*to = num & 0377;			\
	}

/* the inverse of split16 */
#define cvt16(ep)	((*(ep)<<8) + *((ep)+1))

#define increase \
	if (flags&_REG_NOALLOC ||					\
		(p=Realloc(preg->__c_re,bufsize+REG_BUF_SIZE))==NULL)	\
		return(REG_ESPACE);					\
	else {								\
		offset = (p - preg->__c_re);				\
		ep += offset;		/* move current pointer */	\
		preg->__c_re = p;	/* reset start of RE */		\
		bufsize += REG_BUF_SIZE;/* increment bufsize */		\
		endbuf = preg->__c_re + bufsize - 1;/* reset endbuf */	\
		/* reset other positional parameters */			\
		for (i=level;i>=0;i--) {				\
			alt_next[i] += (alt_next[i]) ? offset:0;	\
			token[i] += (token[i]) ? offset:0;		\
		}							\
		opt_token += (opt_token) ? offset:0;			\
		last_token += (last_token) ? offset:0;			\
		cnt_ep += (cnt_ep) ? offset:0;				\
	}

/* case insensitive character comparison */
static icasecmp(p,s)
uchar p,s;
{
	return(p==s||_toupper(p)==s||_tolower(p)==s);
	/* This was the old (and better) way to do this.  POSIX won't let us */
	/*
	return(p==_tolower(s)||_tolower(p)==s||p==_toupper(s)||_toupper(p)==s);
	*/
}

/* 
   braces -- match a brace, br 
		 if extended regular expression
		    match a br 
                 else
		    match a '\' and br pair
*/
static uchar *braces(re_ere, p, br)
int re_ere;
uchar *p;
char br;
{
	char ch;
	uchar *ptr;

	ch=(re_ere == ERE? br: '\\');
	if ((ptr=(uchar *) strchr(p, ch)) != NULL) {
		if (re_ere != ERE && *(ptr+1) != br)
			return (uchar *) NULL;
		return ptr;
	}
	return (uchar *) NULL;
}
static int openBrace(re_ere, p)
int re_ere;
uchar *p;
{
	return (re_ere == ERE? *p == '{': (*p == '\\' && *(p+1) == '{'));
}
static uchar *closeBrace(re_ere, p)
int re_ere;
uchar *p;
{
	return braces(re_ere, p, '}');
}


static uchar *getNumber(p, np)
uchar *p;
int *np;
{

	register int n;
	register int c;

	if (!isdigit(c = *p)) {
		return ((uchar *) NULL);
	}
	for (n = '0' - c; isdigit(c = *++p); ) {
		n *= 10; /* two steps to avoid unnecessary overflow */
		n += '0' - c; /* accum neg to avoid surprises at MAX */
	}
	*np=n=-n;
	return (n > 255? (uchar *)NULL : p);
}

#ifdef _NAMESPACE_CLEAN
#undef regcomp
#pragma _HP_SECONDARY_DEF _regcomp regcomp
#define regcomp _regcomp
#endif /* _NAMESPACE_CLEAN */

#pragma OPT_LEVEL 1	/* regcomp doesn't optimize well... */
int
regcomp(preg, pattern, flags)
regex_t		*preg;
register uchar	*pattern;
unsigned		flags;
{
	register		c;

	register uchar	*pp = pattern;
	register uchar	*ep,*p;
	register uchar	*endbuf;
	register uchar	*ptr;
	register int	not_multibyte = (__nl_char_size == 1);

	uchar		*last_token = 0;
	uchar		*opt_token = 0;
	int		closed	= 0;
	int		exprcnt;
	int		c_esc = flags & _REG_C_ESC;
	int		alt = flags & _REG_ALT;
	int		icase = flags & REG_ICASE;
	int		expf = flags & _REG_EXP; /* flag for regexp(3C) */

	uchar		*cnt_ep;
	int		i, cflg;
	int		backslash;
	struct logical_char	lchar, pchar;
	int		c2, range, no_prev_range, two_to_1;
	uchar		*psave;
	int		error;
	int		m, n;
	int		re, ere, re_ere;
	uchar		*string[5];
	uchar		**sp = string;
	uchar		*xp;
	int		allowed_nparen;		/* number of allowed parens */

	int		level = 0;		/* nesting level */
	int		nparen = 0;		/* number of parentheses pairs */

	uchar	*first_alt[NLEVELS+1];		/* ptr to first ALT token at this level */
	uchar	*alt_last[NLEVELS+1];		/* ptr to last ALT token at this level */
	uchar	*alt_next[NLEVELS+1];		/* ptr to where next ALT token would go */
	uchar	non_null[NLEVELS+1];		/* flag ERE could potentially match null */
	uchar	alt_null[NLEVELS+1];		/* flag ALT could potentially match null */
	uchar	*token[NLEVELS+1];		/* remember where last token was */

	uchar	*last_alt;			/* ptr to last ALT token at this level */
	uchar	*this_alt;			/* ptr to where new ALT token goes */
	int	bufsize;			/* size of RE buffer */
	int	offset;
	int	old;

	if (flags & _REG_NOALLOC) {
		ep	= preg->__c_re;
		endbuf	= preg->__c_buf_end;
	} else {
		if ((preg->__c_re = ep = Malloc(REG_BUF_SIZE))==NULL)
			return(REG_ESPACE);
		bufsize = REG_BUF_SIZE;
		endbuf = ep + REG_BUF_SIZE - 1;
	}

	*sp = 0;					/* initialize string stack */

	alt_last[0] = 0;
	alt_next[0] = ep;
	first_alt[0] = 0;
	non_null[0] = 0;				/* assume ERE can potentially match null */
	token[0] = 0;

	/*
	 *  Determine which type of regular expression processing
	 *  desired and the set the flags as appropriate.
	 */
	re = ere = 0;
	if (flags & REG_EXTENDED)
		ere = re_ere = ERE;
	else
		re = re_ere = RE;

	allowed_nparen = (re||ere) ? REG_NPAREN : INT_MAX;	/* number of allowed parens */

	/*
	 *  Determine if any special cases apply for anchoring the RE at the front.
	 */
	if (*pp == '.' && (*(pp+1) == '*' || (ere && *(pp+1) == '+')))
		/* anchor a leading ".*" or ".+" to improve performance */
		*ep++ = ANCHOR_BEGIN;		


	while (1) {

		if (ep+9 >= endbuf) {			/* "9" allows for the largest token except for	*/
							/* MATCH_LIST which is handled separately	*/
							/* So... try for more space			*/
			increase;
		}
		c = *pp++;
		/*
		 *  End of RE?  If so, add final token and return success.
		 */
		if (c == '\0') {

			if (level)
				return(REG_EPAREN);

			if (first_alt[0]) {

				last_alt = alt_last[0];		/* ptr to last ALT token at this level */
				this_alt = alt_next[0];		/* ptr to beginning of latest RE */

				if (this_alt == ep)		/* empty expression? */
					return(REG_ENOEXPR);

				/* update previous ALT by filling in its offset to this one */
				split16(last_alt+1, this_alt-last_alt-1);

				/* fill in the offsets of all the alt JMPs to point to the closing paren */
				last_alt = first_alt[0];
				while (last_alt != this_alt) {
					last_alt += cvt16(last_alt+1) + 1;
					split16(last_alt-2, ep-last_alt+2);
				}
			}

			*ep++ = END_RE;				/* add final token to compiled RE */
			if (!(flags&_REG_NOALLOC)) {
				p=Realloc(preg->__c_re,ep-preg->__c_re); /* release unused memory */
				if (p) {
					offset= p-preg->__c_re;
					preg->__c_re = p;
					ep += offset;
					preg->__c_buf_end = ep;	/* ptr to byte past end of buffer1 */
				}
			}
			preg->__flags	= flags;		/* save the flags */
			preg->__anchor	= 0;			/* null out forced anchor option */
			preg->__c_re_end	= ep;		/* ptr to end of compiled RE + 1 byte */
			preg->re_nsub	= nparen;		/* number of parenthesized subexpressions */
			return(0);				/* and return `success' */
		}

		/*
		 *  Otherwise process new expression
		 */

		if (c == '\\') {
			c = (*pp++) | BACKSLASH;
			if (c_esc) {				/* use C escaping conventions */
				switch (c) {
					case 'a'|BACKSLASH: c='\007';break;
					case 'b'|BACKSLASH: c='\b';break;
					case 'f'|BACKSLASH: c='\f';break;
					case 'n'|BACKSLASH: c='\n';break;
					case 'r'|BACKSLASH: c='\r';break;
					case 't'|BACKSLASH: c='\t';break;
					case 'v'|BACKSLASH: c='\v';break;
					case 'x'|BACKSLASH:
						n=i=0;
						while (1) {
                                                	c= *pp++;
                                                	if (c>=0 && c<=9)
                                                        	n=n*16+c-'0';
                                                	else if (c>='a'&&c<='f')
                                                        	n=n*16+c-'a'+10;
                                                	else if (c>='A'&&c<='F')
                                                        	n=n*16+c-'A'+10;
							else {
								if (i==0)
									c='x'|BACKSLASH;
								else
									c=(char)n|BACKSLASH;
								pp--;
								break;
							}
							i++;
                                        	}
					default:	/* \ddd */
						if (c>=('0'|BACKSLASH) && c<=('7'|BACKSLASH)) {
							n = c - ('0'|BACKSLASH);
							for (i=0;i<2;i++) {	/* up to 3 digits */
								if (*pp>='0' && *pp<='7')
									n= n*8 + (*pp++)-'0';
								else
									break;
							}
							c = (uchar)n|BACKSLASH;
						}
						break;
				}
			}
			else if (c == BACKSLASH)		/* what about \000 in pattern? */
				return(REG_EESCAPE);		/* can't have a trailing \ */
		}

		if (c == '|' && alt && re) {
			c |= _REG_ALT;
		}

		switch (c|re_ere) {

		case '.'|RE:
		case '.'|ERE:
			/*
			 *  DOT		- match any character
			 *
			 *	re:	.
			 *	comp:	DOT
			 */
			opt_token = last_token = ep;
			*ep++ = (not_multibyte) ? DOT : MB_DOT;
			if (ere)
				non_null[level] |= (*pp != '*' && *pp != '?');
			continue;

		case '*'|RE:
		case '*'|ERE:
			/*
			 *  STAR	- match zero or more occurrences of the preceeding RE
			 *
			 *	re:	RE *
			 *	comp:	JMP  offset_PLUS  <RE>  PLUS  offset_<RE>
			 */
			if (last_token == 0) {
				if (ere)
					return (REG_BADRPT);
				goto ord_char;
			}
		star:						/* PLUS and Q_MARK jump here */
			if (ere)
				/* eliminate redundant duplication operators */
				while (*pp == '*' || *pp == '+' || *pp == '?')
					pp++;

			shift(last_token, ep, 3);
			ep += 3;
			*last_token++ = JMP;
			split16(last_token, ep - last_token);	/* fill in offset following JMP (to PLUS) */
			last_token += 2;

			/* fall into PLUS ... */

		case '+'|ERE:
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
			 *	or
			 *	comp:	MB_DOT MB_DOT_PLUS
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
			if (last_token == 0)
				return (REG_BADRPT);

			if (ere) {
				while (*pp == '+')		/* eliminate redundant PLUS operators */
					pp++;
				if (*pp == '*' || *pp == '?') {	/* "+*" or "+?" is the same as "*" */
					pp++;
					goto star;
				}
			}

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
				/* we have a ".+" in the middle of an RE */
				} else if (*last_token == DOT) {
					*ep++ = DOT_PLUS;
				} else { /* MB_DOT */
					*ep++ = MB_DOT_PLUS;
				}
			} else
			if (*last_token == BYTE) {
				if (*pp == 0) {
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

		case '?'|ERE:
			/*
			 *  ERE Q_MARK	- match zero or one occurrences of the preceeding RE
			 *
			 *	re:	RE ?
			 *	comp:	Q_MARK  offset_<NEXT>  <RE>  <NEXT>
			 */
			if (last_token == 0)
				return (REG_BADRPT);

			while (*pp == '?')			/* eliminate redundant Q_MARK operators */
				pp++;
			if (*pp == '*' || *pp == '+') {		/* "?*" or "?+" is the same as "*" */
				pp++;
				goto star;
			}

		qmark:						/* CARET and DOLLAR jump here */
			opt_token = last_token;
			shift(last_token, ep, 3);
			ep += 3;
			*last_token++ = Q_MARK;
			split16(last_token, ep - last_token);	/* fill in offset to following ERE */
			last_token = 0;

			continue;


		case '^'|RE:
		case '^'|ERE:
			/*
			 *  CARET	- anchor RE to beginning of line
			 *
			 *	re:	^ RE
			 *	comp:	ANCHOR_BEGIN  <RE>
			 */
			if (!alt && re && pp-1 != pattern)		/* RE: ^ only special at start of RE */
				goto ord_char;
			/*
			 * (ERE: compiles anywhere but will never produce a match unless it
			 * is at the beginning of the ERE or is only preceeded by STAR'd or Q_MARK'd
			 * expressions.)
			 */
			opt_token = last_token = ep;
			*ep++ = ANCHOR_BEGIN;
			if (ere) {
				if (*pp == '*' || *pp == '?' || *pp == '+') {
					return(REG_BADRPT);
				}
			} else
				last_token = 0;
			continue;


		case '$'|RE:
		case '$'|ERE:
			/*
			 *  DOLLAR	- anchor RE to end of line
			 *
			 *	re:	RE $
			 *	comp:	<RE>  ANCHOR_END
			 */
			if (!alt && re && *pp != '\0')			/* RE: $ only special at end of RE */
				goto ord_char;
			/*
			 * (ERE: compiles anywhere but will never produce a match unless it
			 * is at the end of the ERE or is only followed by STAR'd or Q_MARK'd
			 * expressions.)
			 */
			opt_token = last_token = ep;
			*ep++ = ANCHOR_END;
			if (ere) {
				while (*pp == '+')		/* eliminate meaningless PLUS operators */
					pp++;
				if (*pp == '*' || *pp == '?') {	/* but STAR or QMARK have some effect */
					while (*pp == '*' || *pp == '?' || *pp == '+')
						pp++;		/* eliminate duplicates */
					goto qmark;		/* process as zero $ or one $ */
				}
			} else
				last_token = 0;
			continue;

		case '['|RE:
		case '['|ERE:

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
			**			12 = isblank
			**			13-31 = reserved for future ctype functions
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
			if ((re || ere) && lchar.c1 == '^') {
			        *(ep-1) = NON_MATCH_LIST;
				lchar.c1 = *pp++;
			}

			cnt_ep = ep;
			ep += 4;
			exprcnt = 0;

			range = 0;

			do {
				int bracket_seen;

				if (ep+9 > endbuf) { increase; }
				/* get one logical character */

				if (lchar.c1 == '\0')
					return(REG_EBRACK);

				if (FIRSTof2(lchar.c1) && SECof2(*pp)) {			/* kanji? */
					lchar.c2 = *pp++;
					lchar.flag = 1;						/* kanji */
					lchar.class = 1;					/* as itself */
					goto have_logical_char;
				} else if (lchar.c1 == '[' && ((c = *pp) == '.' || c == ':' || c == '=')) {

					/*
					 * Eat the [.:=] and save the start of the extended syntax
					 * argument.
					 */
					psave = ++pp;

					/*
					 * scan for closing expression (".]", ":]", "=]")
					 */
					bracket_seen=0;
					while (!((c2 = *pp++)=='\0' || (c2 == ']' && pp-1 != psave))) {

						if (c2 == ']')
							bracket_seen++;

						if (FIRSTof2(c2) && SECof2(*pp))
							pp++;
					}

					if (c2 == '\0' && !bracket_seen)
						return(REG_EBRACK);				/* no closing expression found */

					if (*(pp-2) != c || pp-1 == psave) {			/* not a class expression */
						pp = psave - 1;					/* reset to beginning */
						c='x';		/* This is to fool the following code so that we	*/
								/* can drop out of this if statement and fall into	*/
								/* the next.						*/
					}

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
						switch (pp - psave - 2) {

						case 0:
							/* found "[..]" or "[==]" */
							return(REG_ECOLLATE);

						case 1:
							/* found a single one-byte character */
							lchar.flag = 0;				/* single char */
							/* error if not a collating element */
							if (!_collxfrm(lchar.c1, 0, 0, (int *)0))
								return(REG_ECOLLATE);		/* not a collating element */
							break;

						case 2:
							/* found a 2-to-1 or a kanji or 2 bytes of junk */
							lchar.c2 = *(psave+1);
							/* is 1st char possibly the 1st char of a 2-to-1? */
							if (_nl_map21) {
								/* check if a 2-to-1 collating element */
								if (!_collxfrm(lchar.c1, lchar.c2, 0, &two_to_1) || !two_to_1)
									return(REG_ECOLLATE);	/* not a 2-to-1 collating element */
								lchar.flag = 2;			/* 2-to-1 */
							} else if (FIRSTof2(lchar.c1) && SECof2(lchar.c2)) {
								/* error if not a collating element */
								if (!_collxfrm(lchar.c1<<8 | lchar.c2, 0, 0, (int *)0))
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

						if (pp - psave - 2 == 0)
							return(REG_ECTYPE);			/* found "[::]" */

						/*
						 * Match name in RE against supported ctype functions
						 */
						*(pp-2) = '\0';					/* make the name a string */
						i = 0;
						while ((i < sizeof(ctype_funcs)/sizeof(char *)) && strcmp((char *)psave, (char *)ctype_funcs[i]))
							i++;
						*(pp-2) = ':';					/* restore RE */

						if (i >= sizeof(ctype_funcs)/sizeof(char *))
							return(REG_ECTYPE);			/* found "[:garbage:]" */
						else {
							lchar.flag = i;				/* ctype macro */
						}

						break;
					}
					if (c!='x')
						goto have_logical_char;
				}
				if (c_esc && lchar.c1=='\\') {
					uchar c;

					/* UCSqm00293: need to skip \\ after read */
					switch (c = *pp++) {
						case 'a': c='\007';break;
						case 'b': c='\b';break;
						case 'f': c='\f';break;
						case 'n': c='\n';break;
						case 'r': c='\r';break;
						case 't': c='\t';break;
						case 'v': c='\v';break;
						case 'x':
							n=i=0;
							while (1) {
                                                		c= *pp++;
                                                		if (c>=0 && c<=9)
                                                        		n=n*16+c-'0';
                                                		else if (c>='a'&&c<='f')
                                                        		n=n*16+c-'a'+10;
                                                		else if (c>='A'&&c<='F')
                                                        		n=n*16+c-'A'+10;
								else {
									if (i==0)
										c='x';
									else
										c=(char)n;
									pp--;
									break;
								}
								i++;
                                        		}
						default:	/* \ddd */
							if (c>='0' && c<='7') {
								n = c - '0';
								for (i=0;i<2;i++) {	/* up to 3 digits */
									if (*pp>='0' && *pp<='7')
										n= n*8 + (*pp++)-'0';
									else
										break;
								}
								c = (uchar)n;
							}
							break;
					}
					lchar.flag = 0;   /* single char */
					lchar.c1 = c;
				} else {
					/*
					 * Got just a character by itself
					 */

					lchar.flag = 0;    /* single char */
					lchar.class = 1;   /* as itself */
				}

				/*
				 * Now we have one logical character, figure out what
				 * to do with it.
				 */
		have_logical_char:
				if (lchar.class == 3) {						/* character class? */

					if (range)
						return(REG_ERANGE);				/* ctype can't be range endpoint */
					/*
					 * generate compiled subexpression for a ctype class
					 */
					ep += gen_ctype(&lchar, ep, endbuf, &error);
					if (error)
						return(error);
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
							ep += gen_single_match(&lchar, ep, endbuf, &error, icase);
							if (error)
								return(error);
							exprcnt++;
						}
					}
				}

				else if (range) {

					/*
					 * generate compiled subexpression for a range
					 */
					ep += gen_range(&pchar, &lchar, ep, endbuf, &error, icase);
					if (error)
						return(error);
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
					ep += gen_single_match(&lchar, ep, endbuf, &error, icase);
					if (error)
						return(error);
					exprcnt++;
				}

			} while ((lchar.c1 = *pp++) != ']');

			if (range) {
				if (no_prev_range) {
					/* output last char as single match */
					ep += gen_single_match(&pchar, ep, endbuf, &error, icase);
					if (error)
						return (error);
					exprcnt++;
				}

				/* output dash as single match */
				lchar.c1 = '-';
				lchar.flag = 0;
				lchar.class = 1;
				ep += gen_single_match(&lchar, ep, endbuf, &error, icase);
				if (error)
					return (error);
				exprcnt++;
			}

			split16(cnt_ep, ep - cnt_ep);
			split16(cnt_ep+2, exprcnt);

			if (ere)
				non_null[level] |= (*pp != '*' && *pp != '?');

			continue;


		case '|'|ERE:
		case '|'|RE|_REG_ALT:
			/*
			 *  Alternation (ERE only):
			 *
			 *      re1 | re2 | re3   ->
			 *                ^
			 *                pp
			 *
			 *
			 *   current:    ALT ________ re1 JMP ___ re2
			 *                ^                       ^    ^
			 *                last_alt           this_alt  ep
			 *
			 *   next:       ALT next_alt re1 JMP ___ ALT ________ re2 JMP ___
			 *                                         ^                        ^
			 *                                         last_alt                 this_alt, ep
			 *
			 *   final:      ALT next_alt re1 JMP end ALT next_alt re2 JMP end re3 [END|RP]
			 *
			 */
			last_alt = alt_last[level];		/* ptr to last ALT token at this level */
			this_alt = alt_next[level];		/* ptr to beginning of re2 */

			if (this_alt == ep)			/* empty expression? */
				return(REG_ENOEXPR);

			if (last_alt) {
				/* update previous ALT by filling in its offset to this one */
				split16(last_alt+1, this_alt-last_alt-1);
				/* accumulate non_null status for all ALT'd ERE's */
				alt_null[level] = alt_null[level] && non_null[level];
			} else {
				/* if there was no previous ALT remember this one as the first */
				first_alt[level] = this_alt;
				/* and start off the accumulated non_null status */
				alt_null[level] = non_null[level];
			}

			shift(this_alt, ep, 3);			/* make room to insert new ALT */
			ep += 3;

			*this_alt = ALT;			/* write tokens, the offsets will */
			*ep = JMP;				/* be filled in later */
			ep += 3;

			alt_last[level] = this_alt;		/* save new values for the next time */
			alt_next[level] = ep;

			non_null[level] = 0;			/* reset for next ALT'd ere */
			opt_token = last_token = token[level] = 0;

			continue;

		case '('|RE|BACKSLASH:
		case '('|ERE:
			/*
			 *  Open Parentheses:
			 *
			 *     RE:    \( re1 \( re2 \) \)   ->
			 *                    ^
			 *                    pp
			 *
			 *     ERE:   ( re1 ( re2 ) )   ->
			 *                  ^
			 *                  pp
			 *
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
			 *   where LP is either "LPAREN index" or "LPAREN_SPEC index"
			 *   and   RP is either "RPAREN index" or "RPAREN_SPEC index"
			 *
			 *   LPAREN is used initially and the ')' processing determines if it should
			 *   be converted to LPAREN_SPEC.
			 */
			if (++nparen > allowed_nparen		/* if number of parens exceeds limit */
			   || ++level > NLEVELS)		/* or nesting level is too deep */
				return(REG_ENSUB);		/* report error: too many \( \) pairs */

			non_null[level] = 0;			/* assume re2 can potentially match null */
			token[level-1] = ep;			/* remember where this paren pair begins */
			*ep++ = LPAREN;				/* and write the token there */
			*ep++ = nparen-1;			/* paren index */
			opt_token = last_token = token[level] = 0;

			alt_last[level] = 0;			/* initialize a new level of ALTs */
			alt_next[level] = ep;
			first_alt[level] = 0;

			continue;


		case ')'|RE|BACKSLASH:
		case ')'|ERE:
			/*
			 *  Close Parentheses:
			 *
			 *     RE:    \( re1 \( re2 \) \)   ->
			 *                           ^
			 *                           pp
			 *
			 *     ERE:   ( re1 ( re2 ) )   ->
			 *                        ^
			 *                        pp
			 *
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
			 *   where LP is either "LPAREN index" or "LPAREN_SPEC index"
			 *   and   RP is either "RPAREN index" or "RPAREN_SPEC index"
			 *
			 *   LPAREN is used initially and the ')' processing determines if it should
			 *   be converted to LPAREN_SPEC.
			 */
			if (level == 0)				/* if no preceeding open paren */
				return(REG_EPAREN);		/* report error: parentheses imbalance */

			*ep++ = RPAREN;				/* write regular paren token */
			*ep++ = *(token[level-1]+1);		/* paren index */
			closed++;				/* another paren pair completed */

			if (first_alt[level]) {

				last_alt = alt_last[level];	/* ptr to last ALT token at this level */
				this_alt = alt_next[level];	/* ptr to beginning of latest RE */

				if (this_alt+1 == ep)		/* empty expression? */
					return(REG_ENOEXPR);

				/* update previous ALT by filling in its offset to this one */
				split16(last_alt+1, this_alt-last_alt-1);

				/* fill in the offsets of all the alt JMPs to point to the closing paren */
				last_alt = first_alt[level];
				while (last_alt != this_alt) {
					last_alt += cvt16(last_alt+1) + 1;
					split16(last_alt-2, ep-last_alt);
				}

				/* accumulate the non_null status for the last ALT'd ERE */
				non_null[level] = alt_null[level] && non_null[level];
			}

			opt_token = last_token = token[--level];

			if (last_token+2 == ep)				/* empty expression? */
				return(REG_ENOEXPR);

			if (*pp == '*' || openBrace(ere, pp) ||
			    (ere && (*pp == '+' || *pp == '?')))
			{
				if ( non_null[level+1] == 0) 
				{
				/*
				 *  This ERE could match null and it's STAR'd, PLUS'd or QMARK'd.
				 *  Convert from regular to special paren tokens
				 */
					*last_token = LPAREN_SPEC;
					ep -= 2;
					*ep++ = RPAREN_SPEC;
					*ep++ = *(last_token+1);
				}
			}

			/*
			 *  Except for STAR'd and QMARK'd expressions (which potentially null out
			 *  an ERE, accumulate the final non_null status of this subexpression
			 *  into the next higher one.
			 */
			if (*pp != '*' && !openBrace(ere, pp) && 
			    (re || *pp != '?'))
				non_null[level] = non_null[level] || non_null[level+1];

			continue;


		case '{'|RE|BACKSLASH:
		case '{'|ERE:
			if (last_token == 0) 
				return REG_BADRPT;

			if ((ptr=closeBrace(re_ere, pp)) == NULL)
				return REG_EBRACE;

			m=n=0;
			if ((pp=getNumber(pp, &m)) == NULL)
				/* 
				  if called by regexp
					return either 
					11 - range endpoint too large
					16 - bad number
				  else
					return REG_BADBR
				*/	
				return(expf?(m>255?11:16):REG_BADBR);

			if (pp == ptr)                   /* case {m} */
				n=m;                     /* same as {m,m} */
			else                       
			if (*pp == ',') {
				pp++;
				if (pp == ptr)         /* case {m,} */    
					n=65534+m;       /* same as {m,large} */
				else                     /* case {m,n} */
				{
					if ((pp=getNumber(pp, &n)) == NULL || pp != ptr) {
						if (!expf)
							return REG_BADBR;
						if (pp != NULL && *pp == ',' && isdigit(*(pp+1)))
							/* return more than two number given in brace */
							return 44;
						return(n>255?11:16);
					}
				}
			}
			else
				return(expf?16:REG_BADBR);

			if (n < m)
				/* 
				  if called by regexp(3C)
					return first number exceeds second 
				  else
					return REG_BADBR
				*/
				return(expf?46:REG_BADBR);

			/* make pp points to '}' because closeBrace
			   might be pointing to '\\' for RE         */
			pp += (re_ere == RE? 2: 1);

		/*
		 *  re1 \{m,n\} -> RANGE m re1 END_RANGE n-m+1 offset_to_re1
		 *
		 *  re1 \{0,n\} -> RANGE 0 offset_END_RANGE+1 re1 END_RANGE n+1 offset_to_re1
		 */
			*ep++ = END_RANGE;
			split16(ep, n-m+1);
			split16(ep+2, ep - last_token);	/* offset to re1 from END_RANGE index */
			if (m) {
				shift(last_token, ep+4, 2);
				ep += 6;
			} else {
				shift(last_token, ep+4, 4);
				ep += 8;
			}
			*last_token++ = RANGE;
			*last_token++ = m;
			if (!m)
				split16(last_token, ep - last_token - 4);
			opt_token = last_token = 0;
			continue;
		ord_char:
		default:

			if (re && c >= ('1'|BACKSLASH) && c <= ('9'|BACKSLASH)) {
				if ((c -= ('1'|BACKSLASH)) >= closed)
					return(REG_ESUBREG);
				opt_token=last_token = ep;
				*ep++ = MATCH_SAME;
				*ep++ = c;
				continue;
			}

			backslash = (c&BACKSLASH);		/* save flags */
			c &= 0377;				/* strip off flags to get just the char */

			last_token = ep;

			/* Force BYTE token if first character is escaped.   */
			/* This prevents constructs like /200/041 and /200[  */
			/* from being misinterpreted.			     */

			if (!backslash && FIRSTof2(c) && SECof2(*pp)) {
				*ep++ = KANJI;
				*ep++ = c;
				*ep++ = *pp++;
			} else {
				*ep++ = BYTE;
				*ep++ = c;
			}

			if (ere)
				non_null[level] |= (*pp != '*' && *pp != '?');
			else
				non_null[level] |= *pp != '*';

			/*
			 *  Basic work done, now set up and/or do some optimizations
			 *  for performance if appropriate.
			 */

			if (last_token == preg->__c_re)		/* if first token of RE leave alone */
				continue;

			/* if dup operator other than PLUS following can't do remaining optimizations */
			if ((ere && (*pp == '*' || *pp == '?')) ||
			   (re && (*pp == '*' || (*pp == '\\' && *(pp+1) == '{')))) {
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
				}
			}

			/* if PLUS operator following can't do remaining optimizations */
			if (ere && *pp == '+') {
				sp = string;
				continue;
			}

			/* the next optimizations are for consecutive STRING/BYTE/KANJI tokens */
			if (*sp && (!opt_token || (*opt_token != STRING && *opt_token != BYTE && *opt_token != KANJI))) {
				sp = string;
				*++sp = opt_token = last_token;
				continue;
			}

			/* nothing to merge if stack is empty, put current token on stack and move on */
			if (*sp == 0) {
				*++sp = opt_token = last_token;
				continue;
			}

			/* try to merge BYTEs and KANJIs into STRINGs */
			xp = *sp--;					/* pop the string stack */

			if (*xp == STRING) {
				/* Add to existing STRING */
				int cnt = *++xp;			/* length of STRING */
				if (*last_token == BYTE) {
					if (cnt < 255) {		/* if not too long already */
						*xp = cnt + 1;		/* merge BYTE /w existing */
						last_token = xp-1;	/* STRING */
						*(ep-2) = *(ep-1);
						ep--;
					}
				} else {
					if (cnt < 254) {		/* if not too long already */
						*xp = cnt + 2;		/* merge KANJI /w existing */
						last_token = xp-1;	/* STRING */
						*(ep-3) = *(ep-2);
						*(ep-2) = *(ep-1);
						ep--;
					}
				}

			} else if (*xp == KANJI) {
				/* Convert KANJI-BYTE or KANJI-KANJI into a new STRING */
				int cnt = *last_token == BYTE ? 3 : 4;	/* KANJI-BYTE vs KANJI-KANJI */
				last_token = xp;
				*xp++ = STRING;
				*(xp+2) = *(xp+1);			/* shift 1st kanji to make */
				*(xp+1) = *xp;				/* room for string count */
				*xp = cnt;				/* length of STRING */

			} else if (*last_token == KANJI) {
				switch (sp - string) {
				case 0:
					/* Convert BYTE-KANJI into a new STRING */
					last_token = xp;
					*xp++ = STRING;
					*(xp+1) = *xp;
					*xp = 3;
					break;
				case 1:
					/* Convert BYTE-BYTE-KANJI into a new STRING */
					sp--;
					*xp = *(xp-1);
					*--xp = 4;
					*--xp = STRING;
					*last_token = *(last_token+1);
					*(last_token+1) = *(last_token+2);
					last_token = xp;
					ep--;
					break;
				case 2:
					/* Convert BYTE-BYTE-BYTE-KANJI into a new STRING */
					*xp = *(xp+1);
					*(xp+1) = *(last_token+1);
					*last_token = *(last_token+2);
					xp = *sp--;
					sp--;
					*xp-- = *(xp-1);
					*xp-- = 5;
					*xp = STRING;
					last_token = xp;
					ep -= 2;
					break;
				}

			} else if (sp - string > 1) {
				/* Convert BYTE BYTE BYTE BYTE into a new STRING */
				uchar *np = *--sp;		/* 1st BYTE */
				sp--;					/* empty stack */
				last_token = np;
				*np++ = STRING;
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


/* workaround for malloc(0) differences between malloc(3C) and malloc(3X) */
#define xFree(p)	if ((unsigned)p!=-1) Free(p);
#define xMalloc(n)	(n?Malloc(n):(uchar *)-1)


#define push(xp) {							\
		register int i;						\
		stack_items *old_sip;					\
		/* bring data on top of stack up to date */		\
		sip->ep = xp;						\
		sip->lp = lp;						\
		sip->range_cnt = range_cnt;				\
		/* move stack up one level */				\
		sp += sizeof(stack_items)/sizeof(uchar *);		\
		/* copy basic items to new top of stack */		\
		*(stack_items *)sp = *(old_sip=sip);			\
		/* reset ptrs to use new top of stack as work area */	\
		sip = (stack_items *)sp;				\
		/* get storage for new paren array */			\
		lparens = sip->lparens					\
			= (uchar **)xMalloc(sizeof(uchar *)*nparens*2);	\
		if (!lparens)						\
			goto memcleanup;				\
		/* both arrays in a single malloc'ed block */		\
		rparens = sip->rparens = lparens + nparens;		\
		memcpy(lparens,old_sip->lparens,sizeof(uchar *)*nparens*2);\
	}

#define pop {								\
		if (best_parens.lparens != lparens)			\
			xFree(lparens);					\
		sp -= sizeof(stack_items)/sizeof(uchar *);		\
		sip = (stack_items *)sp;				\
		ep = sip->ep;						\
		lp = sip->lp;						\
		range_cnt = sip->range_cnt;				\
		lparens = sip->lparens;					\
		rparens = sip->rparens;					\
	}

typedef struct {
	uchar	**lparens;
	uchar	**rparens;
} lr_parens;

typedef struct {
	uchar	*ep;
	uchar	*lp;
	uchar	**lparens;
	uchar	**rparens;
	int	range_cnt;
} stack_items;


#ifdef _NAMESPACE_CLEAN
#undef regexec
#pragma _HP_SECONDARY_DEF _regexec regexec
#define regexec _regexec
#endif /* _NAMESPACE_CLEAN */

int
regexec(preg, string, nmatch, pmatch, eflags)
regex_t		*preg;
const uchar	*string;
int		nmatch;
regmatch_t	*pmatch;
unsigned	eflags;
{
	uchar		*string_p = string;
	uchar		*prev_string_p = string;
	register uchar	*lp = string;
	register uchar	*start_ep = preg->__c_re;
	register uchar	*ep = start_ep;
	register	nosub = preg->__flags & REG_NOSUB;
	register 	icase = preg->__flags & REG_ICASE;
	register	newline	= preg->__flags & REG_NEWLINE;
	int		nparens = preg->re_nsub;

	unsigned int	c;
	int		negate = 0;
	register int	not_multibyte = (__nl_char_size == 1);
	unsigned int	start_c = 0;
	unsigned int	start_kanji = 0;

	uchar	*stack[STACK_SIZE];
	uchar	**sp = stack;			/* ptr to top element on stack */
	stack_items	*sip;				/* ptr to misc portion of top stack element */
	uchar	**lparens;
	uchar	**rparens;
	uchar	*best_lp = 0;
	lr_parens	best_parens;
	int		range_cnt = 0;

	int		cnt;
	uchar firstKanji=0;                     /* first character is a kanji */


	sip = (stack_items *)sp;

	best_parens.lparens = lparens = sip->lparens
		= (uchar **)xMalloc(sizeof(uchar *)*nparens*2);
	if (!lparens) return(REG_EMEM);
	rparens = sip->rparens = lparens + nparens;
	

	/* Bug Fix! */
	if (!nosub && nmatch > 0)
		memset(lparens,0,sizeof(uchar *)*nparens*2);

	if (*ep == BYTE && not_multibyte && !preg->__anchor) {
		start_c = *++ep;
		start_ep = ++ep;
		if (icase) {
			while (*string_p && !icasecmp(start_c,*string_p))
				string_p++;
		} else {
			while (*string_p && *string_p != start_c)
				string_p++;
		}
		if (string_p != string)
			prev_string_p = string_p - 1;
		if (*string_p)
			lp = string_p + 1;
		else
			goto errcleanup;

	} else if (!not_multibyte && (*ep == KANJI || *ep == BYTE) && !preg->__anchor) {
		int   save;

		if (firstKanji=(*ep == KANJI)) {
			start_kanji = cvt16(ep+1);
			start_ep = ep += 3;
		} else {
			start_kanji = *++ep;
			start_ep = ++ep;
		}
		do {
			prev_string_p = string_p;
			string_p = lp;
			save=_CHARADV(lp);
			if (!firstKanji && icase && lp==(string_p+1)) {
				if (icasecmp(save, start_kanji))
					break;
			}
			else if (save == start_kanji)
				break;
				
		} while (*string_p);
		if (!*string_p)
			goto errcleanup;
	}
	while (1) {
		switch(*ep++) {

		case ANCHOR_BEGIN:
			if (newline) {
				if (!(eflags&REG_NOTBOL) && lp==string)
					continue;
				if (*prev_string_p=='\n' && lp==string_p) {
					continue;
				}
				goto fail;
			} else if (eflags&REG_NOTBOL) {
				continue;
			} else if (lp == string)
				continue;
			else {
				if (preg->__flags & (REG_EXTENDED|_REG_ALT))
					goto fail; /* ERE's live to try again */
				goto errcleanup;
			}

		case ANCHOR_END:
			if (newline) {
				if (!(eflags&REG_NOTEOL) && *lp=='\0')
					continue;
				if (*lp=='\n')
					continue;
				goto fail;
			} else if (eflags&REG_NOTEOL) { /* can't match $ if not EOL */
				continue;
			}
			if (*lp == '\0')
				continue;
			goto fail;

		case BYTE:
			if (icase) {
				if (icasecmp(*ep++,*lp++))
					continue;
				goto fail;
			}
			if (*ep++ == *lp++)
				continue;
			goto fail;

		case KANJI:
			c = cvt16(ep);
			if (c == _CHARADV(lp)) {
				ep += 2;
				continue;
			}
			goto fail;

		case STRING:
			/*
			 * no more than 255 bytes per string
			 *  STRING byte_count string_data */
			cnt = *ep++;
			if (icase)
			    while (icasecmp(*ep++,*lp++) && --cnt);
			else
			    while (*ep++ == *lp++ && --cnt);
			if (cnt)
				goto fail;
			continue;

		case DOT:
			c = *lp++;
			/* dot cannot match newline if REG_NEWLINE set */
			if ((newline && c=='\n') || !c)
				goto fail;
			continue;

		case MB_DOT:
			c = _CHARADV(lp);
			/* dot cannot match newline if REG_NEWLINE set */
			if ((newline && c=='\n') || !c)
				goto fail;
			continue;

	        case NON_MATCH_LIST:
			negate = 1;
		case MATCH_LIST:
			c = (not_multibyte) ? *lp++ : _CHARADV(lp);
			/* if REG_NEWLINE is set, non-matching lists cannot
			   match newlines */
			if (newline && negate && c=='\n')
				goto fail;
			if (c = isthere(c, *lp, ep+2, negate, icase)) {
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

		case MATCH_SAME:
			{
				uchar	*cp = lparens[*ep];

				cnt = rparens[*ep++] - cp;

				/* this handle null match recursively */
				if (lp==cp)
					goto fail;

				if (icase)
					while (cnt && icasecmp(*cp++,*lp++)) cnt--;
				else
					while (cnt && *cp++ == *lp++) cnt--;

				if (cnt)
					goto fail;
				continue;
			}

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
			while ((!newline && *lp)||(newline && *lp && *lp != '\n')) {
				push(ep);
				lp++;
			}
			continue;

		case MB_DOT_PLUS:
			/*	. + re2		MB_DOT MB_DOT_PLUS <re2> */
			while ((!newline && *lp)||(newline && *lp && *lp != '\n')) {
				push(ep);
				ADVANCE(lp);
			}
			continue;

		case BYTE_PLUS:
			/*	a + re2		BYTE a BYTE_PLUS <re2>	*/
			if (icase) {
				while (icasecmp(*(ep-2),*lp)) {
					push(ep);
					lp++;
				}
			} else {
				while (*lp == *(ep-2)) {
					push(ep);
					lp++;
				}
			}
			continue;

		case PLUS_BYTE:
			/*	re1 + a	re2	<re1> PLUS_BYTE offset_to_re1 BYTE a <re2>	*/
			if (icase) {
				if (icasecmp(*(ep+3),*lp))
					push(ep+2);
			} else {
				if (*(ep+3) == *lp)
					push(ep+2);
			}
			ep -= cvt16(ep);
			continue;

		case DOT_PLUS_BYTE:
			/*	. + a		DOT DOT_PLUS_BYTE BYTE a	*/
			if (icase) {
				while ((!newline && *lp) ||
				       (newline && *lp && *lp != '\n')) {
					if (icasecmp(*(ep+1),*lp)) {
						push(ep);
						if (newline && *lp=='\n') {
							lp++;
							break;
						}
					}
					lp++;
				}
			} else {
				while ((!newline && *lp) ||
				       (newline && *lp && *lp != '\n')) {
					if (*(ep+1) == *lp) {
						push(ep);
						if (newline && *lp=='\n') {
							lp++;
							break;
						}
					}
					lp++;
				}
			}
			goto fail;

		case MB_DOT_PLUS_BYTE:
			/*	. + a		DOT MB_DOT_PLUS_BYTE BYTE a	*/
			while ((!newline && *lp) ||
			       (newline && *lp && *lp != '\n')) {
				if (*(ep+1) == *lp) {
					push(ep);
					if (newline && *lp=='\n') {
						ADVANCE(lp);
						break;
					}
				}
				ADVANCE(lp);
			}
			goto fail;

		case BYTE_PLUS_BYTE:
			/*	a + b		BYTE a BYTE_PLUS_BYTE BYTE b	*/
			if (icase) {
				while (icasecmp(*(ep-2),*lp)) {
					if (icasecmp(*(ep+1),*++lp))
						push(ep);
				}
			} else {
				while (*lp == *(ep-2)) {
					if (*(ep+1) == *++lp)
						push(ep);
				}
			}
			continue;

		case DOT_PLUS_END:
			/*	. + $		DOT DOT_PLUS_END END_RE		*/
				/*
				   We need to either skip a byte or a
				   multibyte character.
				   This is done because newline is a valid
				   second-of-two.
				*/
			if (not_multibyte) {
				while (*lp) {
					if (newline && *lp=='\n') break;
					lp++;
				}
			} else {
				while (*lp) {
					if (newline && *lp=='\n') break;
					ADVANCE(lp);
				}
			}
			continue;

		case BYTE_PLUS_END:
			/*	a + $ \0	BYTE a BYTE_PLUS_END ANCHOR_END END_RE		*/
			/*	a + \0		BYTE a BYTE_PLUS_END END_RE			*/
			if (icase) {
				while (icasecmp(*(ep-2),*lp))
					lp++;
			} else {
				while (*(ep-2) == *lp)
					lp++;
			}
			continue;

		case Q_MARK:
			/*
			 *  Question mark operator: match the RE zero or one times
			 *
			 *  re1 ? re2 -> Q_MARK offset_to_re2 re1 re2
			 *                            |
			 *                            ep
			 *
			 *  zero times:  save start of re2 on stack for processing later
			 *  one time:    start processing re1 (followed by re2) now
			 */
			push(ep + cvt16(ep));
			ep += 2;
			continue;

		case RANGE:
			/*
			 *  Start of Range: match the RE at least m but not more than n times
			 *
			 *  re1 \{m,n\} -> RANGE m re1 END_RANGE n-m+1 offset_re1
			 *                       |
			 *                       ep
			 *
			 *  re1 \{0,n\} -> RANGE 0 offset_END_RANGE+1 re1 END_RANGE n+1 offset_re1
			 *                       |
			 *                       ep
			 *
			 *  Activate the required iteration count for this range expression.
			 *  If there are minimum required iterations, proceed to process the RE.
			 *  Otherwise skip to the end of the range to begin processing the optional
			 *  iterations.
			 */
			if (range_cnt = -(*ep++))		/* init required iteration count */
				continue;				/* and continue with re1 */
			else {					/* but if no required iterations */
				range_cnt = -1;				/* pretend we had just one */
				ep += cvt16(ep);			/* skip to END_RANGE token */
			}						/* and fall into its processing */

		case END_RANGE:
			/*
			 *  End of Range
			 *
			 *  re1 \{m,n\} -> RANGE m re1 END_RANGE n-m+1 offset_to_re1
			 *                                         |
			 *                                         ep
			 *
			 *  re1 \{0,n\} -> RANGE 0 offset_END_RANGE+1 re1 END_RANGE n+1 offset_to_re1
			 *                                                           |
			 *                                                           ep
			 *
			 *  If working on required iterations:
			 *     count one more as done
			 *     if more required iterations are needed go back and do another
			 *     otherwise move on to optional iterations
			 *  If working on optional iterations:
			 *     count one more as done
			 *     if more optional iterations are needed
			 *        save position of following RE on stack in case the additional iterations
			 *           don't work out
			 *        go back and do another iteration
			 *     continue with RE following range expression
			 */
			if (++range_cnt < 0) {			/* matched another required iteration */
				ep -= cvt16(ep+2);		/* if more, go back to start of re1 */
				continue;
			}

			if (range_cnt == 0)			/* matched all required iterations */
				range_cnt = cvt16(ep);		/* get number of optional iterations + 1 */

			if (--range_cnt) {			/* if more remain after this one */
				range_cnt--;			/* decrement the number remaining */
				push(ep+4);			/* save the following RE for later and */
				ep -= cvt16(ep+2);		/* go back for another iteration of re1 */
				continue;
			}

			ep += 4;
			continue;

		case ALT:
			/*
			 *  Alternation operator: match either the preceeding or the following ERE
			 *
			 *  ere1 | ere2 | ... | ereN ->
			 *       ALT offset_ALT ere1 JMP end ALT offset_ALT ere2 JMP end ... ALT 0 ereN
			 *                |
			 *                ep
			 *
			 *  ("offset_ALT" is the offset to the next ALT or "0" if this is the last alternation
			 *  subexpression; "end" is the offset to whatever follows ereN)
			 *
			 *  Save the start of the next ALT'd subexpression on the stack.
			 *  Advance to the first ALT'd subexpression and continuing matching from there.
			 */
			push(ep + cvt16(ep));
			ep += 2;
			continue;

		case LPAREN_SPEC:
			/*
			 *  Special left parenthesis:  If RE of the expression (RE)+ or (RE)* could
			 *  potentially match a null string, we have to guard against the PLUS
			 *  generating an infinite number of null string matches.
			 *
			 *  ( re1 ) +  ->  LPAREN_SPEC index re1 RPAREN_SPEC index PLUS offset_LPAREN_SPEC
			 *                               |
			 *                               ep
			 *
			 *  Save current value of lp.  Fall into regular LPAREN processing.
			 */
		case LPAREN:
			lparens[*ep++] = lp;
			continue;

		case RPAREN_SPEC:
			/*
			 *  Special right parenthesis:  If RE of the expression (RE)+ or (RE)* could
			 *  potentially match a null string, we have to guard against the PLUS
			 *  generating an infinite number of null string matches.
			 *
			 *  ( re1 ) +  ->  LPAREN_SPEC index re1 RPAREN_SPEC index PLUS offset_LPAREN_SPEC
			 *                                                     |
			 *                                                     ep
			 *
			 *  If no change occurred between the saved and current value of lp, we have a
			 *  null match.  Don't continue any further along this path.
			 *  Otherwise (if non-null match), fall into regular RPAREN processing.
			 */
			if (lparens[*ep] == lp)
				goto fail;
			rparens[*ep++] = lp;
			continue;

		case RPAREN:
			rparens[*ep++] = lp;
			continue;

		case JMP:
			ep += cvt16(ep);
			continue;

		case END_RE:
			if (lp > best_lp) {
				/* got a match longer than any previous match */
				best_lp = lp;
				/* copy working paren arrays to best paren arrays */
				best_parens.lparens = lparens;
				best_parens.rparens = sip->rparens;
			}

			/* see if just any match will do or if we've already got the whole line */
			if (nosub || *lp == 0)
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
				if (!nosub && nmatch>0) {
					/* unused subexpression pointers have to be NULL */
					for (cnt=nmatch-1;cnt>nparens;cnt--)
						pmatch[cnt].rm_so = pmatch[cnt].rm_eo = (off_t)-1;

					while (cnt) {
						pmatch[cnt].rm_so = (best_parens.lparens[cnt-1])?(off_t)best_parens.lparens[cnt-1]-(off_t)string:-1;
						pmatch[cnt].rm_eo = (best_parens.rparens[cnt-1])?(off_t)best_parens.rparens[cnt-1]-(off_t)string:-1;
						cnt--;
					}
				pmatch[0].rm_so = (off_t)string_p-(off_t)string; /* byte offset of matched string */
				pmatch[0].rm_eo = (off_t)best_lp-(off_t)string; /* byte offset past end of best match in string */
				}

				xFree(best_parens.lparens);
				/* clean up any remaining stack */
				while (sp > stack) {
					xFree(sip->lparens);
					sp -= sizeof(stack_items)/sizeof(uchar *);
					sip = (stack_items *)sp;
				}
				xFree(sip->lparens);
				return (0);

			} else {
				/* return fail */

				if (preg->__anchor || *string_p == 0)
					goto errcleanup;
				/*
				 * Note: this is set up so that a null string gets processed
				 * which is necessary to allow REs such as "x*$" to succeed.
				 */

				if (not_multibyte) {
					prev_string_p = string_p;
					string_p++;
				} else {
					prev_string_p = string_p;
					ADVANCE(string_p);
				}

				lp = string_p;
				ep = start_ep;

				if (start_c) {
					if (icase) {
						while (*string_p && !icasecmp(start_c,*string_p))
							string_p++;
					} else {
						while (*string_p && *string_p != start_c)
							string_p++;
					}
					if (string_p != string)
						prev_string_p = string_p - 1;
					if (*string_p)
						lp = string_p + 1;
					else
						goto errcleanup;
				} else if (start_kanji) {
					int save;
					do {
						prev_string_p = string_p;
						string_p = lp;
						save=_CHARADV(lp);
						if (!firstKanji && icase && lp==(string_p+1)) {
							if (icasecmp(save, start_kanji))
								break;
						}
						else if (save == start_kanji)
							break;
							
					} while (*string_p);
					if (!*string_p)
						goto errcleanup;
				}

				continue;
			}
		}
        }
	/* something went wrong, cleanup and return error */
errcleanup:
	c=REG_NOMATCH;
	goto cleanup;
memcleanup:
	c=REG_EMEM;
cleanup:
	while (sp > stack) {
		sp -= sizeof(stack_items)/sizeof(uchar *);
		sip = (stack_items *)sp;
		xFree(sip->lparens);
	}
	xFree(best_parens.lparens);
 	return(c);
}

/*
   isblank -- this is a local version of ctype function which has
	      not been implemented in libc.  
	      When there is a libc version, this function should
	      be taken out.
*/
static int isblank(c)
int c;
{
	return(__ctype[c]&_B);
}


static isthere(c1, c2, ep, negate, icase)
register unsigned int		c1, c2;
register const uchar	*ep;
{
	int exprcnt;
	unsigned int x1, x2, e1, e2, flag;
	int match_1_char, match_2_to_1, two_to_1;

	if (c1 == 0)
	        return 0;

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
			if (!(flag & 3)) {				/* single char match wanted */
				if (icase)
					match_1_char = icasecmp(e1,c1);
				else
					 match_1_char = (c1 == e1);
			} else if (flag & 1) {
				e1 = (e1 << 8) | *ep++;			/* kanji match wanted */
				match_1_char = (c1 == e1);
			} else {
				e2 = *ep++;				/* 2-to-1 match wanted */
				match_2_to_1 = (c1 == e1 && c2 == e2);
			}
		}

		/* else if subexpression type is ctype */
		else if (flag & 0200) {
			if (c1 < 256)					/* no ctype for kanji */
				switch (flag & 037) {
					case 0:		match_1_char = isalpha(c1); break;
					case 1:		match_1_char = (isupper(c1)||(icase&&islower(c1))); break;
					case 2:		match_1_char = (islower(c1)||(icase&&isupper(c1))); break;
					case 3:		match_1_char = isdigit(c1); break;
					case 4:		match_1_char = isxdigit(c1); break;
					case 5:		match_1_char = isalnum(c1); break;
					case 6:		match_1_char = isspace(c1); break;
					case 7:		match_1_char = ispunct(c1); break;
					case 8:		match_1_char = isprint(c1); break;
					case 9:		match_1_char = isgraph(c1); break;
					case 10:	match_1_char = iscntrl(c1); break;
					case 11:	match_1_char = isascii(c1); break;
					case 12:	match_1_char = isblank(c1); break;
				}
		}

		/* else if subexpression type is range */
		else {
			/* 7/8-bit language with machine collation */
			if (!_nl_collate_on) {
				if (icase) {
					e1 = *ep++;
					e2 = *ep++;
					match_1_char = ((_tolower(c1) >= e1) && (_tolower(c1) <= e2)||(c1 >= _tolower(e1)) && (c1 <= _tolower(e2)));
				} else {
					e1 = *ep++;
					e2 = *ep++;
					match_1_char = (c1 >= e1) && (c1 <= e2);
				}
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
static gen_range(bchar, echar, ep, endbuf, error, icase)
const struct logical_char	*bchar, *echar;
register uchar		*ep;
const uchar		*endbuf;
int				*error;
{

	register unsigned int c1, c2;
	register unsigned xfrm1, xfrm2;

	*error = 0;

	/* store flag for this range expression */
	*ep++ = 0100;

	/* 7/8-bit language with machine collation */
	if (!_nl_collate_on) {
		if (bchar->c1 > echar->c1) {
			*error = REG_ERANGE;	/* 1st endpoint must be <= 2nd endpoint */
			return (0);
		}
		*ep++ = (uchar)(bchar->c1);
		*ep++ = (uchar)(echar->c1);
		return (3);
	}

	/* 8-bit NLS collation or 16-bit machine collation */

	/* get collation transformation of 1st range endpoint */
	c1 = bchar->c1;
	if (bchar->flag == 1)
		c1 = c1 << 8 | bchar->c2;
	c2 = (bchar->flag == 2) ? bchar->c2 : 0;
	xfrm1 = _collxfrm(c1, c2, (bchar->class == 2) ? -1 : 0, (int *)0);
	if (xfrm1 == 0) {
		*error = REG_ECOLLATE;		/* range endpoint non-collating */
		return (0);
	}

	/* store 1st range endpoint */
	*ep++ = (uchar)(xfrm1 >> 16);
	*ep++ = (uchar)(xfrm1 >> 8);
	*ep++ = (uchar)(xfrm1);

	/* get collation transformation of 2nd range endpoint */
	c1 = echar->c1;
	if (echar->flag == 1)
		c1 = c1 << 8 | echar->c2;
	c2 = (echar->flag == 2) ? echar->c2 : 0;
	xfrm2 = _collxfrm(c1, c2, (echar->class == 2) ? 1 : 0, (int *)0);
	if (xfrm2 == 0) {
		*error = REG_ECOLLATE;		/* range endpoint non-collating */
		return (0);
	}

	/* store 2nd range endpoint */
	*ep++ = (uchar)(xfrm2 >> 16);
	*ep++ = (uchar)(xfrm2 >> 8);
	*ep++ = (uchar)(xfrm2);

	if (xfrm1 > xfrm2) {
		*error = REG_ERANGE;		/* 1st endpoint must be <= 2nd endpoint */
		return (0);
	}

	return (7);
}



/*
 *  Generate compiled subexpression for a single match
 */
static gen_single_match(lchar, ep, endbuf, error, icase)
const struct logical_char	*lchar;
register uchar		*ep;
const uchar		*endbuf;
int				*error;
{
	uchar *orig_ep = ep;

	*error = 0;

	if (lchar->class == 2) {
		/* output equiv class as a range */
		return gen_range(lchar, lchar, ep, endbuf, error, icase);
	} else {
		/* output: char */
		*ep++ = lchar->flag | 040;
		if (!lchar->flag) {
			*ep++ = (uchar)(lchar->c1);
		} else {			/* if kanji or 2-to-1 */
			*ep++ = lchar->c1;
			*ep++ = lchar->c2;
		}
	}
	return (ep - orig_ep);
}



/*
 *  Generate compiled subexpression for a ctype class
 */
static gen_ctype(lchar, ep, endbuf, error)
const struct logical_char	*lchar;
register uchar			*ep;
const uchar			*endbuf;
int				*error;
{
	*error = 0;
	*ep = lchar->flag | 0200;
	return (1);
}



#ifdef _NAMESPACE_CLEAN
#undef regfree
#pragma _HP_SECONDARY_DEF _regfree regfree
#define regfree _regfree
#endif /* _NAMESPACE_CLEAN */


void
regfree(preg)
regex_t	*preg;
{
	if (!(preg->__flags&_REG_NOALLOC) && preg->__c_re) {
		Free(preg->__c_re);
		preg->__c_re=NULL;
	}
}


static char *regerr_errlist[] = {
/* UNKNOWN	catgets 01 */ "Unknown error",
/* REG_BADPAT	catgets 02 */ "Invalid regular expression",
/* REG_BADBR	catgets 03 */ "Contents of \\{\\} invalid: Not a number, number to large, more than two numbers, or first larger than second",
/* catgets 04 */ "",
/* catgets 05 */ "",
/* catgets 06 */ "",
/* catgets 07 */ "",
/* catgets 08 */ "",
/* catgets 09 */ "",
/* catgets 10 */ "",
/* catgets 11 */ "",
/* catgets 12 */ "",
/* catgets 13 */ "",
/* catgets 14 */ "",
/* catgets 15 */ "",
/* catgets 16 */ "",
/* catgets 17 */ "",
/* catgets 18 */ "",
/* catgets 19 */ "",
/* REG_NOMATCH	catgets 20 */ "regexec() failed to match",	
/* REG_ECOLLATE	catgets 21 */ "Invalid collating element referenced",
/* REG_EESCAPE	catgets 22 */ "Trailing \\ in pattern",
/* REG_ERANGE	catgets 23 */ "Invalid endpoint in range expression",
/* REG_ECTYPE	catgets 24 */ "Invalid character class type referenced",
/* REG_ESUBREG	catgets 25 */ "Number in \\digit invalid or in error",
/* REG_BADRPT	catgets 26 */ "?, *, or + not preceded by valid regular expression",
/* REG_ENOEXPR	catgets 27 */ "no expression within ( ) or on one side of an |",
/* catgets 28 */ "",
/* catgets 29 */ "",
/* catgets 30 */ "",
/* catgets 31 */ "",
/* catgets 32 */ "",
/* catgets 33 */ "",
/* catgets 34 */ "",
/* catgets 35 */ "",
/* REG_ENEWLINE	catgets 36 */ "\\n found before end of pattern and REG_NEWLINE flag not set",
/* catgets 37 */ "",
/* catgets 38 */ "",
/* catgets 39 */ "",
/* catgets 40 */ "",
/* REG_ENOSEARCH catgets 41 */ "no remembered search string",
/* REG_EPAREN	catgets 42 */ "\\( \\) or ( ) imbalance",
/* REG_ENSUB	catgets 43 */ "more than nine \\( \\) pairs or nesting level too deep",
/* catgets 44 */ "",
/* REG_EBRACE	catgets 45 */ "\\{ \\} imbalance",
/* catgets 46 */ "",
/* catgets 47 */ "",
/* catgets 48 */ "",
/* REG_EBRACK	catgets 49 */ "[ ] imbalance",
/* REG_ESPACE catgets 50 */ "out of memory",
/* REG_EMEM	catgets 51 */ "out of memory while matching expression",
};

#ifdef _NAMESPACE_CLEAN
#undef regerror
#pragma _HP_SECONDARY_DEF _regerror regerror
#define regerror _regerror
#endif /* _NAMESPACE_CLEAN */

size_t
regerror(errcode,preg,errbuf,errbuf_size)
int errcode;
regex_t *preg;
char *errbuf;
size_t errbuf_size;
{
	nl_catd fd;
	char *s;

	if (errcode<1 || errcode>REG_EMEM || *regerr_errlist[errcode-1]=='\0')
		errcode=1;
	s=regerr_errlist[errcode-1];
	if ((fd=catopen("regcomp",0))!=(nl_catd)-1) {
		char *msg;
		msg = catgets(fd,NL_SETN,errcode,regerr_errlist[NL_SETN]);
		if (msg && *msg)
			s = msg;
		(void) catclose(fd);
	}
	if (errbuf_size!=0) {
		strncpy(errbuf,s,errbuf_size);
		errbuf[errbuf_size-1]='\0';
	}
	return strlen(s)+1;
}
