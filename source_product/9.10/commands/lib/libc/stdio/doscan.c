/* @(#) $Revision: 70.3 $ */    
/*LINTLIBRARY*/

/*
**	 03/19/92 Release 9.0 changes.
**
**	 Added support for wide characters (%C) and wide strings (%S).
**	 This is based upon SC22/WG14/N169R (1991-09-15), proposed
**	 addendum to ISO/IEC 9899-1990, adopted by X/Open XPG4.
**
**       04/21/89 Release 7.0 changes.
**
**       Changed multibyte character support.  Old interpretation of
**       ANSI-C Draft assumed multibyte support for ordinary character
**       directive, 's' conversion character, 'c' conversion character,
**       '[' conversion character.  Now only ordinary character directive
**       is supported.  Also the language sensitive scanset is gone.
**
**       01/24/89 Release 7.0 changes.
**
**       Changed numbered argument code.  Under the old scheme
**       if the 1st spec had a "digit$" construct, then a new format
**       and argument list was created.  In the new scheme a pointer
**       to any object is assumed to be the same size.  This allows
**       the stack to be accessed directly without creating a new format
**       and argument list.  This new method produces better performance
**       and smaller code size.
**
**       01/20/89 Release 7.0 changes.
**
**       We now have some long double <--> string library routines.
**       The L type specifier is changed to mean long double and not
**       double precision.  Long doubles implemented as a 4 unsigned
**       integer typedef in both doscan and the long double <--> string
**       library routines.
**
**       11/3/88 Release 7.0 changes.
**
**       The scanf library routines will be changed to include the
**       features found in the X/Open Portability Guide, Issue 3 and
**       the ANSI/X3.159-1988 C Programming Language Standard.  The
**       new features for these routines include:
**
**             1.  Numbered arguments for vanilla scanf (X/OPEN): The
**                 numbered argument features found in the nl_scanf
**                 functions will be rolled into the regular scanf
**                 functions.  If 1st spec has "digit$" construct,
**                 then a new format and argument list is created.
**
**             2.  Multi-byte format strings (ANSI): Ordinary
**                 characters within the format string can be either
**                 single- or multi-byte characters as determined by
**                 the program's locale (LC_CTYPE category).
**
**             3.  The "numerical maximum-field width" will be
**                 interpreted as bytes (not whole characters) until
**                 the ANSI/X3.159-1988 C Programming Language
**                 Standard declares otherwise.
**
**             4.  L type specifier (ANSI): The conversion characters
**                 e, f and g may be preceded by L to indicate that
**                 the corresponding argument is a pointer to long
**                 double.  If an L appears before any other
**                 conversion character, it is ignored.
**
**                 Currently, HP compilers have not implemented the
**                 new long double type.  Until it has been implemented,
**                 the L type specifier means double precision.
**
**             5.  p conversion character (ANSI): Matches a sequence
**                 of hexadecimal numbers produced by the %p
**                 conversion of printf.  The corresponding argument
**                 shall be a pointer to a pointer to void.  The
**                 behavior of the %p conversion is undefined for any
**                 input item other than the value converted earlier
**                 during the same program execution.
**
**             6.  Language sensitive scanset (HP): This is a want
**                 and not a requirement of the X/Open Portability
**                 Guide, Issue 3.  A range of characters in a
**                 scanset may be represented by the construct
**                 first-last, enabling [0123456789] to be expressed
**                 [0-9].  Using this convention, first must be
**                 lexically less than or equal to last; otherwise,
**                 the dash stands for itself.  The lexical order is
**                 determined by the program's locale (LC_COLLATE
**                 category).  In the "C" locale, or in a locale
**                 where the collation is not defined, the collation
**                 order defaults to machine collation.  Only ranges
**                 will be supported: no character class expressions
**                 ([:alpha:]), equivalence classes ([=A=]) or
**                 collating symbols ([.ch.]).  Note that for spanish
**                 the range [a-d] includes 2-to-1 character [.ch.].
*/

#ifdef _NAMESPACE_CLEAN
#define atof 	_atof
#define ungetc 	_ungetc
#define at_EOF 	_at_EOF
#define read 	_read
#define memset 	_memset
#  ifdef __lint
#    define fileno 	_fileno
#    define isdigit 	_isdigit
#    define isspace 	_isspace
#    define isupper 	_isupper
#    define isxdigit 	_isxdigit
#    define getc 	_getc
#  endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <sys/stdsyms.h>
#include <stdio.h>
#include "stdiom.h"
#include <ctype.h>
#include <varargs.h>
#include <values.h>
#include <math.h>
#include <wchar.h>
#include <wpi.h>

extern long_double _atold();
extern double atof();
extern char *memset();

#define GETC(iop)		(nscan++ , getc(iop))
#define UNGETC(c,iop)		(nscan-- , ungetc((c),(iop)))
#define NCHARS			(1 << BITSPERBYTE)
#define NB_SIZE			1024

#if defined(NLS) || defined(NLS16)

	extern int _nl_radix;

#	include <limits.h>
#	include <nl_ctype.h>

	static int _t;
#	define ISUPPER(c)	(((_t = (c)) > 255) ? 0 : isupper(_t)) 
#	define ISDIGIT(c)	(((_t = (c)) > 255) ? 0 : isdigit(_t))
#	define ISSPACE(c)	(((_t = (c)) > 255) ? 0 : isspace(_t))
#	define _TOLOWER(c)	(((_t = (c)) > 255) ? (_t) : _tolower(_t))

#else

#	define ADVANCE(p)	(++p)
#	define CHARAT(p)	(*p)
#	define CHARADV(p)	(*p++)
#	define ISUPPER(c)	isupper(c) 
#	define ISSPACE(c)	isspace(c) 
#	define ISDIGIT(c)	isdigit(c)
#	define _TOLOWER(c)	_tolower(c)

#endif

typedef unsigned char uchar;

static long int nscan;		/* number of bytes scanned this call */
static int at_EOF;		/* true if at end-of-file */

int
_doscan(iop, fmt, va_alist)
register FILE *iop;
register uchar *fmt;
va_list va_alist;
{
	extern uchar *setup();

	register int ch;
	int nmatch = 0, len, inchar, stow, size;
	char tab[NCHARS];

#if defined(NLS) || defined(NLS16)
	int numspec = 0;		/* first conversion spec flag */
	int numarg = 0;			/* numbered argument flag */
	uchar *savfmt;			/* save place in format string */
	char *arg;			/* ptr to an argument */
	va_list start_list = va_alist;	/* beginning of argument list */
#endif
	
	nscan = at_EOF = 0;

	/*******************************************************
	 * Main loop: reads format to determine a pattern,
	 *		and then goes to read input stream
	 *		in attempt to match the pattern.
	 *******************************************************/

	for( ; ; ) {
		if((ch = CHARADV(fmt)) == '\0')
			/* if EOF only character read,
			   then return EOF, else return number of matches */
			return ((at_EOF && !nscan) ? EOF : nmatch); /* end of format */
		if(ISSPACE(ch)) {
			if (!at_EOF) {
				while(ISSPACE(inchar = GETC(iop)))
					;
				if(UNGETC(inchar, iop) == EOF)
					at_EOF = 1;
			}
			continue;
		}

		if((ch != '%' || (ch = CHARADV(fmt)) == '%') && !at_EOF) {
			/* ordinary character directive */
			/* multibyte characters permitted here */
			if((inchar = nl_getc(iop)) == ch)
				continue;
			if(nl_ungetc(inchar, iop) == EOF)
			{
				return (!nmatch ? EOF : nmatch); /* end of input */
			}
			else
				return(nmatch); /* failed to match input */
		}

		if (ch == '*') {
			stow = 0;
			ch = *fmt++;
		} else 
			stow = 1;

		for( len = 0 ; ISDIGIT(ch) ; ch = CHARADV(fmt)) {
#if defined(NLS) || defined(NLS16)
			savfmt = fmt;
#endif
			len = len * 10 + ch - '0';
		}

#if defined(NLS) || defined(NLS16)
		if (!stow) {
			/* see if we have a *%n$ */
			if (len && (CHARAT(savfmt) == '$')) {
				ch = CHARADV(fmt);	/* move past $ */
			}
		}
		else {
			if (++numspec == 1) {
				if (len && (CHARAT(savfmt) == '$')) {
					numarg++;
				}
			}
			if (len && (CHARAT(savfmt) == '$')) {
				int n;
				/* if mix plain spec & numbered spec: quit */
				if (!numarg || len > NL_ARGMAX) {
					return nmatch;
				}
				/* see if we have a %n$* */
				ch = CHARADV(fmt);
				if (ch == '*') {
					stow = 0;
					ch = *fmt++;
					if (numspec == 1) {
						numspec--;
						numarg--;
					}
				}
				else {
					stow = 1;
					/* move to argument n in %n$ */
					len--;		/* base 0 */
					va_alist = start_list;
					for (n=0 ; n < len ; n++) {
						arg = va_arg( va_alist, char *);
					}
				}
				/* get field width, if any */
				for( len = 0 ; ISDIGIT(ch) ; ch = CHARADV(fmt)) {
					len = len * 10 + ch - '0';
				}
			}
			else if (numarg) {
				/* can't mix plain spec & numbered spec: quit */
				return nmatch;
			}
		}
#endif
		if (len == 0)
			len = MAXINT;

		if((size = ch) == 'l' || size == 'h' || size == 'L')
			ch = CHARADV(fmt);

		if(ch == '\0' ||
		    ch == '[' && (fmt = setup( fmt, tab)) == NULL)
			return(EOF); /* unexpected end of format */

		/* Don't downshift C or S!  They are different from c and s */
		if (ch != 'C' && ch != 'S' && ISUPPER(ch)) {
			switch (ch = _TOLOWER( ch)) {
			case 'e':
			case 'g':
			case 'x':
				break;
			default:
				/* no longer documented */
				size = 'l';
			}
		}

		if(ch != 'c' && ch != 'C' && ch != '[' && ch != 'n' && !at_EOF) {
			while(ISSPACE(inchar = GETC(iop)))
				;
			if(UNGETC(inchar, iop) == EOF)
				at_EOF = 1;
		}

		if(ch == 'c' || ch == 's' || ch == '[') {
			if (at_EOF) break;
			size = string(stow, ch, len, tab, iop, &va_alist) ;
		} else if(ch == 'C' || ch == 'S') {
			if (at_EOF) break;
			size = wstring(stow, ch, len, iop, &va_alist) ;
		} else if (ch == 'n') {
			numscan(stow, size, &va_alist);
			continue;
		} else {
			if (at_EOF) break;
			size = number(stow, ch, len, size, iop, &va_alist);
		}

		if (size) 
			nmatch += stow;
		else
			return(nmatch ? nmatch : (at_EOF ? EOF : 0)); /* failed to match input */
	}
	return(nmatch ? nmatch : EOF); /* end of input */
}

/***************************************************************
 * Functions to read the input stream in an attempt to match incoming
 * data to the current pattern from the main loop of _doscan().
 ***************************************************************/

static int
number(stow, type, len, size, iop, listp)
int stow, type, len, size;
register FILE *iop;
va_list *listp;
{
	char numbuf[NB_SIZE];
	register char *np = numbuf;
	register int c, base;
	int digitseen = 0, dotseen = 0, expseen = 0, floater = 0, negflg = 0, pointer = 0;
	long lcval = 0;

	switch(type) {
	case 'e':
	case 'f':
	case 'g':
		floater++;
	case 'd':
	case 'u':
	case 'i':
		base = 10;
		break;
	case 'o':
		base = 8;
		break;
	case 'p':
		pointer++;
	case 'x':
		base = 16;
		break;
	default:
		return(0); /* unrecognized conversion character */
	}
	switch(c = GETC(iop)) {
	case '-':
		negflg++;
		if (size == 'L') {
			/* sign must be part of numbuf for _atold() */
			*np++ = '-';
		}
	case '+': /* fall-through */
		len--;
		c = GETC(iop);
	}

 	if (type == 'i') {
		/* get the base from the input stream */
 		if (c == '0') {
 			base = 8;
 			if (len > 1 && hex_prefix( iop)) {
				base = 16;
				len--;
 			}
 		}
	}
 	else if (type == 'x' || type == 'p') {
		/* may have optional 0x or 0X prefix */
 		if (c == '0' && len > 1 && hex_prefix( iop)) {
			len--;
 		}
 	}
 
	for( ; --len >= 0; *np++ = c, c = GETC(iop)) {
		/* must be room for 1-byte char and NULL */
		if ((np - numbuf) > (NB_SIZE - 2)) {
			return 0;
		}

		/* process a string of digits */
		if(isdigit(c) || base == 16 && isxdigit(c)) {
			int digit = c - (isdigit(c) ? '0' :
			    isupper(c) ? 'A' - 10 : 'a' - 10);
			if(digit >= base)
				break;
			if(stow && !floater)
				lcval = base * lcval + digit;
			digitseen++;
			continue;
		}

		/* stop if integer, otherwise get floating part of number */
		if(!floater)
			break;
#if defined(NLS) || defined(NLS16)
		if(c == _nl_radix && !dotseen++)
#else
		if(c == '.' && !dotseen++)
#endif
			continue;
		if((c == 'e' || c == 'E') && digitseen && !expseen++) {
			*np++ = c;
			len--;
			c = GETC(iop);
			if(isdigit(c) || c == '+' || c == '-' || c == ' ')
				continue;
		}
		break;
	}
	if(stow && digitseen)
		if (floater) {
			*np = '\0';
			if (size == 'L') {
				*(va_arg(*listp, long_double *)) = _atold( numbuf);
			}
			else {
				register double dval;
				dval = atof(numbuf);
				if ((size != 'l') && (dval > MAXFLOAT))
					dval = MAXFLOAT;
				if (negflg)
					dval = -dval;
				if (size == 'l')
					*(va_arg(*listp, double *)) = dval;
				else 
					*(va_arg(*listp, float *)) = (float)dval;
			}
		}
		else if (pointer) {
			*(va_arg(*listp, void **)) = (void *)lcval;
		}
		else {
			/* suppress possible overflow on 2's-comp negation */
			if(negflg && lcval != HIBITL)
				lcval = -lcval;
			if(size == 'l')
				*(va_arg(*listp, long *)) = lcval;
			else if(size == 'h')
				*(va_arg(*listp, short *)) = (short)lcval;
			else
				*(va_arg(*listp, int *)) = (int)lcval;
		}
	if(UNGETC(c, iop) == EOF) {
		at_EOF = 1;	/* end of input */
	}
	return(digitseen); /* successful match if non-zero */
}

static int
string(stow, type, len, tab, iop, listp)
register int stow, type, len;
register char *tab;
register FILE *iop;
va_list *listp;
{
	register int ch;
	register char *ptr;
	char *start;

	start = ptr = stow ? va_arg(*listp, char *) : NULL;
	if(type == 'c' && len == MAXINT)
		len = 1;
	while((ch = GETC(iop)) != EOF &&
	    !(type == 's' && isspace(ch) || type == '[' && tab[ch])) {
		if(stow)
			*ptr = ch;
		ptr++;
		if(--len <= 0)
			break;
	}
	if(ch == EOF) {
		UNGETC(ch, iop);	/* push back EOF */
		at_EOF = 1;		/* end of input */
	} else if (len > 0) {
		UNGETC(ch, iop);	/* push back non-EOF */
	}
	if(ptr == start)
		return(0); /* no match */
	if(stow && type != 'c')
		*ptr = '\0';
	return(1); /* successful match */
}

static int
wstring(stow, type, len, iop, listp)
int stow, type, len;
FILE *iop;
va_list *listp;
{
	int ch;
	wchar_t *ptr;
	wchar_t *start;

	start = ptr = stow ? va_arg(*listp, wchar_t *) : NULL;
	if(type == 'C' && len == MAXINT)
		len = 1;
	while((ch = nl_getc(iop)) != EOF && !(type == 'S' && iswspace(ch))) {
		if(stow)
			*ptr = (wchar_t)ch;
		ptr++;
		if (ch & 0xff00)
			len -= 2;	/* 2 byte character */
		else
			len -= 1;	/* 1 byte character */
		if(len <= 0)
			break;
	}
	if(ch == EOF) {
		nl_ungetc(ch, iop);	/* push back EOF */
		at_EOF = 1;		/* end of input */
	} else if (len > 0) {
		nl_ungetc(ch, iop);	/* push back non-EOF */
	}
	if(ptr == start)
		return(0);		/* no match */
	if(stow && type != 'C')
		*ptr = (wchar_t)'\0';	/* terminate string */
	return(1);			/* successful match */
}

static uchar *
setup(fmt, tab)
register uchar *fmt;
register char *tab;
{
	register int b, c, d, t = 0;

	if(*fmt == '^') {
		t++;
		fmt++;
	}
	(void)memset(tab, !t, NCHARS);
	if((c = *fmt) == ']' || c == '-') { /* first char is special */
		tab[c] = t;
		fmt++;
	}
	while((c = *fmt++) != ']') {
		if(c == '\0')
			return(NULL); /* unexpected end of format */
		if(c == '-' && (d = *fmt) != ']' && (b = fmt[-2]) < d) {
			(void)memset(&tab[b], t, d - b + 1);
			fmt++;
		} else
			tab[c] = t;
	}
	return(fmt);
}

static int
numscan(stow, size, listp)	/* handle %n */
register int stow;			/* true if store into variable */
register int size;			/* long, short, regular */
va_list *listp;				/* argument list */
{
	if (stow) {
		if (size == 'l') {
			*(va_arg(*listp, long *)) = nscan;
		} else if (size == 'h') {
			*(va_arg(*listp, short *)) = (short) nscan;
		} else {
			*(va_arg(*listp, int *)) = (int) nscan;
		}
	} 	
}

/*
** hex_prefix: if input stream contains a hex digit prefix, eat the prefix
**	and return true.  Otherwise leave the input stram alone and return 
**	false.
*/

static int
hex_prefix( iop)
register FILE *iop;
{
	register int c;

	if ((c = GETC(iop)) == 'x' || c == 'X') {
		register int x = c;
 		if (isxdigit(c = GETC(iop))) {
 			UNGETC(c, iop);
			return 1;
		}
		else {
 			UNGETC(c, iop);
 			UNGETC(x, iop);
		}
 	}
	else {
 		UNGETC(c, iop);
 	}
	return 0;
}

#if defined(NLS) || defined(NLS16)

/*
******************************************************************************
** The nl_getc and nl_ungetc routines get and unget multi-byte characters.
** These routines:
** 	1. Do not require any changes to the FILE structure.
** 	2. Do not need to save two-byte characters that span
** 	   file buffer boundaries in a local structure.
** 	3. Should not have any problems with close/dup/freopen.
** 
** The main idea is found in nl_getc with the following statement:
** 
** 		second = (iop->_cnt != 0) ? getc(iop) :
** 			 (read(fileno(iop), &ch, 1) == 1) ? ch : EOF;
** 
** If the two-byte character is not split across the file buffer boundary,
** then the character count is not zero and both bytes of the character
** are fetched with getc(3s).
** 
** If the two-byte character is split, then the first byte is retrieved by
** getc(3s) and the second byte is read into the variable "second" by read(2).
** The file buffer is not affected by reading the second byte.
** 
** Let's assume a two-byte character split across the buffer boundary is
** retrieved by nl_getc and immediately pushed back by nl_ungetc.
** The second byte is placed in the last element of the file buffer
** and the first byte is placed in the next to last element.
** A subsquent nl_getc will correctly fetch the two-byte character.
** 
** Here's an example using these routines with a file buffer modified
** to be 3 bytes long.  The prt() function prints things out in hex
** (printf("%s = %.4x\t\tbuf = %.2x %.2x %.2x\n",s,c,buf[0],buf[1],buf[2]);).
** 
** LANG=japanese; export LANG
** 
** CODE:
** 	nl_init(getenv("LANG"));
** 
** 	prt((c = nl_getc(stdin)), "get");
** 	prt((c = nl_getc(stdin)), "get");
** 	prt((c = nl_getc(stdin)), "get");
** 	prt(nl_ungetc(c,stdin), "unget");
** 	prt((c = nl_getc(stdin)), "get");
** 	prt((c = nl_getc(stdin)), "get");
** 	prt(nl_ungetc(c,stdin), "unget");
** 	prt((c = nl_getc(stdin)), "get");
** 	prt((c = nl_getc(stdin)), "get");
** 	prt((c = nl_getc(stdin)), "get");
** INPUT:
**	61 62 f7 63 64 0a
** OUTPUT:
** 	get = 0061		buf = 61 62 f7
** 	get = 0062		buf = 61 62 f7
** 	get = f763		buf = 61 62 f7
** 	unget = f763		buf = 61 f7 63
** 	get = f763		buf = 61 f7 63
** 	get = 0064		buf = 64 0a 63
** 	unget = 0064		buf = 64 0a 63
** 	get = 0064		buf = 64 0a 63
** 	get = 000a		buf = 64 0a 63
** 	get = ffffffff		buf = 64 0a 63
** 
** The main disadvantage of this approach is the contents of the file buffer
** will change if when a split two-byte character is pushed back:
** 
** 	get = f763		buf = 61 62 f7
** 	unget = f763		buf = 61 f7 63
** 
** The file buffer pointers get updated so usually you'll be ok.
** There is a problem if you unget all the characters fetched from the
** buffer: there won't be any room for the first character at the beginning.
** This isn't a problem with _doscan.
******************************************************************************
*/

static int
nl_getc( iop)
register FILE *iop;			/* input stream */
{
	extern int __nl_char_size;	/* setlocale character size */

	register int c;			/* return value */
	register int second;		/* possible 2nd of 2 */
	uchar ch;			/* lookahead character buffer */

	c = getc(iop);
	nscan++;

	if ((__nl_char_size == 2) && FIRSTof2( c)) {
		second = (iop->_cnt != 0) ? getc( iop) :
			 (read( fileno( iop), &ch, 1) == 1) ? ch : EOF;
		if (SECof2( second)) {
			c = (c << 8) | second;
			nscan++;
		}
		else {
			ungetc( second, iop);
		}
	}

	return c;
}

static int
nl_ungetc( c, iop)
register int c;				/* character to push back */
register FILE *iop;			/* input stream */
{
	if (c < 256) {
		nscan--;
		return ungetc(c, iop);
	}
	else {
		nscan -= 2;
		(void) ungetc((c & 0377), iop);
		(void) ungetc(((c >> 8) & 0377), iop);
		return c;
	}
}

#endif
