/* @(#) $Revision: 72.3 $ */
/*LINTLIBRARY*/
/*
    _doprnt: common code for printf, fprintf, sprintf

  8/21/87:
    1)  Modified to handle HP15 text.  Presumes that
	the format string and %c and %s text will be
	HP15.
    2)  Field width/precision values indicate bytes,
	not whole characters.  This gives the caller
	more control of the output.
    3)  Only whole characters are output via %s. If a
	width/precision specification might result
	in truncation of a character, the entire
	character is dropped from the output.
    4)  The return value indicates the number of
	bytes output, not whole characters.
    5)  Added %i (same as %d) for SVVS conformance.

  10/03/88: -- Rel. 7.0 features: XPG 3 and ANSI/X3J11 C
    1)  Numbered arguments for vanilla printf functions (XPG).
	Preprocessing of format statement and argument list now
	done in _doprnt.  If 1st spec has "digit$" construct,
	a new format and argument list is created.

    2)  Numbered width/precision specifiers (%3$*6$.*7$d, etc) (XPG).
	You can still have "associative" but can not mix the two
	in the same specification.
	Handled in _printmsg preprocessing routine.

    3)  Multiple references to numbered arguments (XPG).
	Handled in _printmsg preprocessing routine.

    4)  "0" flag (ANSI). Leading zeros.

    5)  "L" flag (ANSI). Long doubles.
	Long double type will not compile with a pre 7.0 compiler.
	Temporarily assume "long double" is equivalent to "double"
	If "long double" means "quad" precision, then _doprnt needs quad
	precision equivalents of: ecvt(3c), fcvt(3c), MAXECVT, MAXFCVT,
	MAXFSIG, MAXESIZ, MAXEXP.  Coding changes will be needed
	if and when these quad precision features become available.

    6)  "n" conversion character (ANSI).  Store number of bytes scanned.

    7)  "p" conversion character (ANSI).  Print a pointer.

    Note on error handling:  If a bad numbered argument format string
    is found by _printmsg, it is just handed off to _doprnt and is
    not pre-processed.  This is different from the previous behavior
    of nl_printf.

  01/20/89: -- more Rel. 7.0 features:
    We now have some long double <--> string library routines.
    The L type specifier is changed to mean long double and not
    double precision.  Long doubles implemented as a 4 unsigned
    integer structure.  There is a machine dependency.  For S800
    machines, the long double structure is passed by reference.
    For S300 machines, the long double structure is passed by value.

  04/21/89:
    Changed multibyte character support.  Old interpretation of
    ANSI-C Draft assumed multibyte support for the "s" conversion
    character.  New interpretation assumes nothing special is done
    for multibyte characters for "%s".  This breaks backwards
    compatability.  Old "s" functionality is move in the new "S"
    conversion character as an extention.

  03/18/92:
    Added support for %C and %S wide character formatting.

  06/18/92:
    Added support for ' (locale-specific grouping)

    ============================ NOTE =============================
    | _doprnt now alloc's space for a temporary conversion buffer |
    | when a %S is encountered that requires more space than is   |
    | available in valbuf.  If you add any new return statements  |
    | to _doprnt, be sure to check for and free mb_buf if the ptr |
    | is not null... otherwise _doprnt will become memory leaky.  |
    ===============================================================

 */

#ifdef _NAMESPACE_CLEAN
#ifndef _THREAD_SAFE
#define ecvt _ecvt
#define fcvt _fcvt
#define fwrite _fwrite
#else
#define ecvt_r _ecvt_r
#define fcvt_r _fcvt_r
#define fwrite_unlocked _fwrite_unlocked
#endif
#define mblen _mblen
#define memcpy _memcpy
#define memchr _memchr
#define strlen _strlen
#define wctomb _wctomb
#define wcstombs _wcstombs
#ifdef __lint
#   define ferror _ferror
#   define isascii _isascii
#   define isdigit _isdigit
#   define isupper _isupper
#endif /* __lint */
#endif

#include <stdio.h>
#ifdef _THREAD_SAFE
#include "stdio_lock.h"
#endif
#include <string.h>
#include <ctype.h>
#include <varargs.h>
#include <values.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include "stdiom.h"
#include "print.h"	/* parameters & macros for doprnt */
extern int errno;

#if defined NLS || defined NLS16
#   include <nl_ctype.h>
#   include <limits.h>
#   define CHARSIZE(x)	((x) & 0xff00 ? 2 : 1)
    extern int _nl_radix;
    extern struct lconv *_lconv;
#   define groupinfo	_lconv->grouping
#   define thousep	_lconv->thousands_sep
#else
#   define _nl_radix       	'.'
#   define CHARSIZE(x)		(1)
#   define ADVANCE(p)		(++p)
#   define CHARAT(p)		(*p)
#   define CHARADV(p)		(*p++)
#   define PCHAR(c,p)		(*p = c)
#   define PCHARADV(c,p)	(*p++ = c)
#endif

#define TRUE	1
#define FALSE	0
#define UCHAR	unsigned char
#define UINT	unsigned int
#define DEFAULT_FLOAT_PREC	6
#define DEFAULT_INT_PREC	1

#define I	int		/* abbreviations for lint worship */
#define CP	char *
#define UP	UCHAR *

#ifndef NO_LONG_DOUBLE
#define LDZERO(ld)	(((ld).word1 == 0) && ((ld).word2 == 0) && \
			 ((ld).word3 == 0) && ((ld).word4 == 0))
#endif

/* Test for non-null mb_buf pointer so that we won't
 * return without having freed the alloc'd memory first.
 */
#define PUT(p, n) \
{							\
    register unsigned char *newbufptr;			\
							\
    numchar += n;					\
    if ((newbufptr = bufptr + n) > bufferend)		\
    {							\
	if (dowrite(p, n, iop, &bufptr) == EOF)		\
        {						\
		if (mb_buf)				\
		    free(mb_buf);			\
		return(EOF);				\
	}						\
    }							\
    else						\
    {							\
	(void)memcpy((CP)bufptr, (CP)p, n);		\
	bufptr = newbufptr;				\
    }							\
}


/* Test for non-null mb_buf pointer so that we won't
 * return without having freed the alloc'd memory first.
 */
#define PAD(s, n) \
{							\
    register int nn;					\
							\
    for (nn = n; nn > 20; nn -= 20, numchar += 20) {	\
	if (dowrite(s, 20, iop, &bufptr) == EOF) {	\
		if (mb_buf)				\
		    free(mb_buf);			\
		return(EOF);				\
	}						\
    }							\
    PUT(s, nn);						\
}

/* bit positions for flags used in doprnt */
#define HLENGTH	1	/* h */
#define FPLUS	2	/* + */
#define FMINUS	4	/* - */
#define FBLANK	8	/* blank */
#define FSHARP	16	/* # */
#define PADZERO 32	/* padding zeroes requested via '0' */
#define DOTSEEN 64	/* dot appeared in format specification */
#define SUFFIX	128	/* a suffix is to appear in the output */
#define RZERO	256	/* there will be trailing zeros in output */
#define LZERO	512	/* there will be leading zeroes in output */
#ifndef NO_LONG_DOUBLE
#define LDOUBLE	1024	/* L -- long double */
#endif
#define FQUOTE	2048	/* ' (grouping flag) */
#ifdef _THREAD_SAFE
#define NDIG	350
#endif

/*
 *	C-Library routines for floating conversion
 */
#ifdef _THREAD_SAFE
extern int fcvt_r(), ecvt_r();
#else
extern char *fcvt(), *ecvt();
#endif
#ifndef NO_LONG_DOUBLE
#ifdef _THREAD_SAFE
extern int _ldecvt_r(), _ldfcvt_r();
#else
extern char *_ldecvt(), *_ldfcvt();
#endif
#endif

/*
 * lowdigit() --
 *    This function computes the decimal low-order digit of the number
 *    pointed to by valptr, and returns this digit after dividing
 *    *valptr by ten.  This function is called ONLY to compute the
 *    low-order digit of a long whose high-order bit is set.
 */
static int
lowdigit(valptr)
long *valptr;
{
    int lowbit = *valptr & 1;
    long value = (*valptr >> 1) & ~HIBITL;

    *valptr = value / 5;
    return value % 5 * 2 + lowbit + '0';
}

/*
 * dowrite() --
 *     This function carries out buffer pointer bookkeeping
 *     surrounding a call to fwrite.  It is called only when the end
 *     of the file output buffer is approached or in other unusual
 *     situations.
 */
static
dowrite(p, n, iop, ptrptr)
register UCHAR *p;
register int	n;
register FILE	*iop;
register UCHAR **ptrptr;
{
    if (iop->_flag & _IODUMMY)
	*ptrptr = (UP)memcpy((CP)*ptrptr, (CP)p, n) + n;
    else
    {
	iop->_cnt -= (*ptrptr - (UP)iop->_ptr);
	iop->_ptr = *ptrptr;
	_bufsync(iop);
#ifdef _THREAD_SAFE
	if (fwrite_unlocked((CP)p, 1, n, iop) != n) {
#else
	if (fwrite((CP)p, 1, n, iop) != n) {
#endif
	    iop->_flag |= _IOERR;
	    return(EOF);
	}
	*ptrptr = (UP)iop->_ptr;
    }
    return 0;
}

/*
 * _doprnt() --
 *    Formatting routine underlying printf(), fprintf(), sprintf()
 *    and similar routines.
 *
 * NOTE:
 *    _doprnt() makes no assumption that "args" refers to stack
 *    arguments.  It does not invoke "va_start" to "aim" args at the
 *    stack, but presumes instead that "args" already has been
 *    "started", i.e. that it aims at the first argument in a list of
 *    arguments.  "va_arg" is used to access that first argument and
 *    "re-aim" args at the next argument int that list.  The caller's
 *    argument list must follow the same ordering convention as is used
 *    by the stack.
 *
 *    #ifdef _THREAD_SAFE
 *       It is assumed that the caller has locked the FILE object.
 *    #endif
 */
int
_doprnt(fmt, args, iop)
char *fmt;
va_list	args;
register FILE	*iop;
{
    static UCHAR _blanks[] = "                    ";
    static UCHAR _zeroes[] = "00000000000000000000";
    static UCHAR _hexCVT[] = "0123456789ABCDEF";
    static UCHAR _hexcvt[] = "0123456789abcdef";

    /* Format code */
    register UINT fcode;

    /* 
     * bufptr is used inside of doprnt instead of iop->_ptr;
     * bufferend is a copy of _bufend(iop), if it exists.  For
     * dummy file descriptors (iop->_flag & _IODUMMY), bufferend
     * may be meaningless.
     */
    UCHAR *bufptr;		/* can't be a register variable */
    register UCHAR *bufferend;

    /* to keep lint happy... */
    register UCHAR *format = (UCHAR *)fmt;

#if defined NLS || defined NLS16
    /* numbered conversion specification variables */
    register int firstspec = 1;
    register UCHAR *startspec;
    UCHAR newfmt[BUFSIZ];
    va_list newarg[NL_SPECMAX * 4];
    va_list newargs;
#endif

    /* This variable counts output characters. */
    register int count = 0;

    /* Starting, ending, and radix points for value to be printed */
    register UCHAR *bp, *p, *m;

    /* generic counter */
    register int n;

    /* number of characters scanned (used for %n) */
    register int numchar = 0;

    /* 
     * Flags - bit positions defined by HLENGTH, FPLUS, FMINUS, FQUOTE,
     * FBLANK, and FSHARP are set if corresponding character is in
     * format.
     * Bit position defined by PADZERO means extra space in the
     * field should be padded with leading zeroes rather than with
     * blanks.
     */
    register int flagword;

    /* was the last format character truncated */
    register int truncated;

    /* Field width and precision */
    register int width, prec;

    /* Number of padding zeroes required on the left and right */
    register int lzero, rzero;

    /* Pointer to sign, "0x", "0X", or empty */
    register UCHAR *prefix;

    /* Exponent or empty */
    register UCHAR *suffix;

    /* Buffer to create exponent */
    UCHAR expbuf[MAXESIZ + 1];

    /* Length of prefix and of suffix */
    register int prefixlength, suffixlength;

    /* combined length of leading zeroes, trailing zeroes, and suffix */
    register int otherlength;

    /* The value being converted, if integer */
    long val;			/* can't be a register variable */

    /* The value being converted, if real */
    double dval;

#ifndef NO_LONG_DOUBLE
    /* The value being converted, if long double */
    long_double ldval;
#endif

#ifdef _THREAD_SAFE
    UCHAR cvtbuf[NDIG];
#endif

    /* Some maximum `%Lf' and `%f' floating point values */
    int maxfcvt, maxfsig;

    /* The value being converted, if ptr to void */
    void *pval;

    /* Output values from fcvt and ecvt */
    int decpt, sign;

    /* Pointer to a translate table for digits of whatever radix */
    register UCHAR *tab;

    /* Work variables */
    register int k, lradix, mradix;

    /* Values are developed in this buffer */
    UCHAR valbuf[max(MAXDIGS+MAXIGRP, 10 + max(max(MAXFCVT+MAXFGRP, MAXECVT + MAXESIZ), max(MAXQFCVT+MAXQFGRP, MAXQECVT + MAXESIZ)))];

    /* Pointers used to step through %S wide character strings */
    wchar_t *ws_start, *ws;

    /* Pointer to a dynamically allocated buffer for long '%S' strings
     * that cannot fit into the space available in valbuf.
     */
    UCHAR *mb_buf = 0;

    /* initialize buffer pointer and buffer end pointer */
    {
	bufptr = iop->_ptr;
	bufferend = (iop->_flag & _IODUMMY)
	    ? (unsigned char *)((long)bufptr | (-1L & ~HIBITL))
	    : _bufend(iop);
    }

    /* 
     * The main loop -- this loop goes through one iteration
     * for each string of ordinary characters or format
     * specification.
     */
    for (;;)
    {
	/*  Write out format character until '\0' or '%' */

	bp = format;
	fcode = CHARADV(format);
	if (fcode && fcode != '%')
	{
	    do
	    {
		fcode = CHARADV(format);
	    } while (fcode && fcode != '%');

	    count += (n = format - bp - 1); /* n = no. of non-% chars */
	    PUT(bp, n);
	}

	if (fcode == '\0')
	{			/* end of format; return */
	    register int nn = bufptr - iop->_ptr;
	    iop->_cnt -= nn;
	    iop->_ptr = bufptr;

	    /* 
	     * This code is here in case an interrupt
	     * occurred during the last several lines
	     */
	    if (bufptr + iop->_cnt > bufferend &&
		!(iop->_flag & _IODUMMY))
		_bufsync(iop);

	    /* 
	     * Flush if unbuffered, or if linebuffered and
	     * buffer contains a newline (really an
	     * "approximation" to line-buffered behavior).
	     * This test is a little naive since it looks
	     * at entire buffer instead of trying to
	     * determine how many chars are in the buffer
	     * as the result of this most recent call
	     * to _doprnt.
	     */
	    if ((iop->_flag & _IONBF) ||
		((iop->_flag & _IOLBF) &&
		 memchr(iop->_base, '\n', iop->_ptr - iop->_base)))
		(void)_xflsbuf(iop);
#ifdef _THREAD_SAFE
	    return ferror_unlocked(iop) ? EOF : count;
#else
	    return ferror(iop) ? EOF : count;
#endif
	}

	/* 
	 * % has been found.
	 * The following switch is used to parse the format
	 * specification and to perform the operation specified
	 * by the format letter.  The program repeatedly goes
	 * back to this switch until the format letter is
	 * encountered.
	 */
	width = prefixlength = otherlength = flagword = 0;
#if defined NLS || defined NLS16
	startspec = format - CHARSIZE(fcode);
#endif

    charswitch: 
	fcode = CHARADV(format);
	truncated = FALSE;

	switch (fcode)
	{
	case '+': 
	    flagword |= FPLUS;
	    goto charswitch;
	case '-': 
	    flagword |= FMINUS;
	    flagword &= ~PADZERO;
	    goto charswitch;
	case ' ': 
	    flagword |= FBLANK;
	    goto charswitch;
	case '#': 
	    flagword |= FSHARP;
	    goto charswitch;
	case '\'': 
	    flagword |= FQUOTE;
	    goto charswitch;
	case '0': 
	    if (!(flagword & (DOTSEEN | FMINUS)))
		flagword |= PADZERO;
	    goto charswitch;

	    /* Scan the field width and precision */
	case '.': 
	    flagword |= DOTSEEN;
	    prec = 0;
	    goto charswitch;

	case '*': 
	    if (!(flagword & DOTSEEN))
	    {
		width = va_arg(args, int);
		if (width < 0)
		{
		    width = -width;
		    flagword ^= FMINUS;
		}
	    }
	    else
	    {
		prec = va_arg(args, int);
		if (prec < 0)
		{
		    prec = -1; /* Flags that default precision should be used 
				* i.e. - as if no precision were specified.
				* (Per ANSI C and XPG4).
				*/
		}
	    }
	    goto charswitch;

	case '1': 
	case '2': 
	case '3': 
	case '4': 
	case '5': 
	case '6': 
	case '7': 
	case '8': 
	case '9': 
	    {
		register num = fcode - '0';
		for (;;)
		{
		    fcode = CHARAT(format);
		    if (isascii((I)fcode) && isdigit((I)fcode))
		    {
			num = num * 10 + fcode - '0';
			ADVANCE(format);
		    }
		    else
			break;
		}
#if defined NLS || defined NLS16
		if (firstspec && (CHARAT(format) == '$'))
		{
		    newargs = (va_list)&newarg[NL_SPECMAX * 2];
		    if ((strlen((CP)startspec) < BUFSIZ) &&
			    _printmsg(startspec, newfmt, args, newargs))
		    {
			register UCHAR *startfmt = newfmt;
			ADVANCE(startfmt);
			format = startfmt;
			args = newargs;
			goto charswitch;
		    }
		}
#endif
		if (flagword & DOTSEEN)
		    prec = num;
		else
		    width = num;
		goto charswitch;
	    }

	    /* Scan the length modifier */
	case 'h': 
	    flagword |= HLENGTH;
	    /* No break */
	case 'l': 
	    goto charswitch;
#ifndef NO_LONG_DOUBLE
	case 'L': 
	    flagword |= LDOUBLE;
	    goto charswitch;
#endif

	/* 
	 * The character addressed by format must be the format letter
	 * -- there is nothing left for it to be.
	 *
	 * The status of the +, -, #, and blank flags are reflected in
	 * the variable "flagword".  "width" and "prec" contain numbers
	 * corresponding to the digit strings before and after the
	 * decimal point, respectively.   If there was no decimal point,
	 * then flagword & DOTSEEN is false and the value of prec is
	 * meaningless.
	 *
	 * The following switch cases set things up for printing.  What
	 * ultimately gets printed will be padding blanks, a prefix,
	 * left padding zeroes, a value, right padding zeroes, a suffix,
	 * and more padding blanks.  Padding blanks will not appear
	 * simultaneously on both the left and the right.  Each case in
	 * this switch will compute the value, and leave in several
	 * variables the information necessary to construct what is to
	 * be printed.
	 *
	 * The prefix is a sign, a blank, "0x", "0X", or null, and is
	 * addressed by "prefix".
	 *
	 * The suffix is either null or an exponent, and is addressed
	 * by "suffix".  If there is a suffix, the flagword bit SUFFIX
	 * will be set.
	 *
	 * The value to be printed starts at "bp" and continues up to
	 * and not including "p".
	 *
	 * "lzero" and "rzero" will contain the number of padding
	 * zeroes required on the left and right, respectively.
	 * The flagword bits LZERO and RZERO tell whether padding zeros
	 * are required.
	 *
	 * The number of padding blanks, and whether they go on the
	 * left or the right, will be computed on exit from the switch.
	 */

	/* 
	 * decimal fixed point representations
	 *
	 * HIBITL is 100...000 binary, and is equal to the maximum
	 * negative number.
	 * We assume a 2's complement machine
	 */
	case 'd': 
	case 'i': 
	    /* Fetch the argument to be printed */
	    val = va_arg(args, long);

	    /* Set buffer pointer to last digit */
	    p = bp = valbuf + MAXDIGS;

	    /* If signed conversion, make sign */
	    if (val < 0)
	    {
		prefix = (UP)"-";
		prefixlength = 1;
		/* 
		 * Negate, checking in advance for possible overflow.
		 */
		if (val != HIBITL)
		    val = -val;
		else
		    /*
		     * number is -HIBITL; convert last digit now and
		     * get positive number
		     */
		    *--bp = lowdigit(&val);
	    }
	    else if (flagword & FPLUS)
	    {
		prefix = (UP)"+";
		prefixlength = 1;
	    }
	    else if (flagword & FBLANK)
	    {
		prefix = (UP)" ";
		prefixlength = 1;
	    }

	decimal: 
	    {
		register long qval = val;
#if defined(NLS) || defined(NLS16)
		register char *gp = groupinfo;
		register unsigned char gdigs;
		gdigs = ((flagword & FQUOTE) && *gp)? *gp - (p - bp): CHAR_MAX;
#endif
		if (qval <= 9)
		{
		    if (qval != 0 || !(flagword & DOTSEEN))
		    {
#if defined(NLS) || defined(NLS16)
			if (!gdigs--)	    /* time to add grouping? */
			    *--bp = *thousep;
#endif
			*--bp = qval + '0';
		    }
		}
		else
		{
		    do
		    {
			n = qval;
			qval /= 10;
#if defined(NLS) || defined(NLS16)
			if (!gdigs--)	    /* time to add grouping? */
			{
			    *--bp = *thousep;
			    if (*gp && gp[1])   /* get next group size */
				++gp;
			    gdigs = *gp - (*gp != CHAR_MAX);
			}
#endif
			*--bp = n - qval * 10 + '0';
		    } while (qval > 9);
#if defined(NLS) || defined(NLS16)
			if (!gdigs)	    /* time to add grouping? */
			    *--bp = *thousep;
#endif
		    *--bp = qval + '0';
		}
	    }

	    /* Calculate minimum padding zero requirement */
	    if (flagword & DOTSEEN)
	    {
		register leadzeroes;
		if (prec < 0)
		    prec = DEFAULT_INT_PREC;
		leadzeroes = prec - (p - bp);
		if (leadzeroes > 0)
		{
		    otherlength = lzero = leadzeroes;
		    flagword |= LZERO;
		}
		flagword &= ~PADZERO;
	    }

	    break;

	case 'u': 
	    /* Fetch the argument to be printed */
	    val = va_arg(args, long);
	    if (flagword & HLENGTH)
		val &= 0x0000ffff;

	    p = bp = valbuf + MAXDIGS;

	    if (val & HIBITL)
		*--bp = lowdigit(&val);

	    goto decimal;

	    /* 
	     * non-decimal fixed point representations for radix equal
	     * to a power of two
	     *
	     *	"mradix" is one less than the radix for the conversion.
	     *	"lradix" is one less than the base 2 log of the radix
	     *  for the conversion.   Conversion is unsigned.
	     *
	     *	HIBITL is 100...000 binary, and is equal to the maximum
	     *	negative number.
	     *	We assume a 2's complement machine
	     */
	case 'o': 
	    mradix = 7;
	    lradix = 2;
	    goto fixed;

	case 'X': 
	case 'x': 
	    mradix = 15;
	    lradix = 3;

	fixed: 
	    /* Fetch the argument to be printed */
	    val = va_arg(args, long);

	    if (flagword & HLENGTH)
		val &= 0x0000ffff;

	    /* Set translate table for digits */
	    tab = (UP)((fcode == 'X') ? _hexCVT : _hexcvt);

	    /* Develop the digits of the value */
	    p = bp = valbuf + MAXDIGS;
	    {
		register long qval = val;
		if (qval == 0)
		{
		    if (!(flagword & DOTSEEN))
		    {
			otherlength = lzero = 1;
			flagword |= LZERO;
		    }
		}
		else
		    do
		    {
			*--bp = tab[qval & mradix];
			qval = ((qval >> 1) & ~HIBITL) >> lradix;
		    } while (qval != 0);
	    }

	    /* Calculate minimum padding zero requirement */
	    if (flagword & DOTSEEN)
	    {
		register leadzeroes;
		if (prec < 0)
		    prec = DEFAULT_INT_PREC;
		leadzeroes = prec - (p - bp);
		if (leadzeroes > 0)
		{
		    otherlength = lzero = leadzeroes;
		    flagword |= LZERO;
		}
		flagword &= ~PADZERO;
	    }

	    /* Handle the # flag */
	    if (flagword & FSHARP && val != 0)
		switch (fcode)
		{
		case 'o': 
		    if (!(flagword & LZERO))
		    {
			otherlength = lzero = 1;
			flagword |= LZERO;
		    }
		    break;
		case 'x': 
		    prefix = (UP)"0x";
		    prefixlength = 2;
		    break;
		case 'X': 
		    prefix = (UP)"0X";
		    prefixlength = 2;
		    break;
		}

	    break;

	case 'P': 
	case 'p': 
	    /* 
	     * p-format: print pointer to void.
	     * Similar to 'x' except:
	     *    1. pointer to void argument rather than
	     *       long
	     *    2. "h" flag (short) ignored
	     *    3. "#" flag recognized for NULL pointer
	     *    4. All values printed out as a sequence of
	     *       2 hex digits for each byte in the
	     *       pointer.
	     */

	    /* get the value */
	    pval = va_arg(args, void *);

	    /* Set translate table for digits */
	    tab = (UP)((fcode == 'P') ? _hexCVT : _hexcvt);

	    /* Develop the digits of the value */
	    p = bp = valbuf + MAXDIGS;
	    {
		register int i;

		for (i = (sizeof (void *)*BITSPERBYTE) / 4; i > 0; i--)
		{
		    *--bp = tab[(int)pval & 0xf];
		    pval = (void *)(((int)pval) >> 4);
		}
	    }

	    /* Calculate minimum padding zero requirement */
	    if (flagword & DOTSEEN)
	    {
		register leadzeroes;

		if (prec < 0)
		    prec = DEFAULT_INT_PREC;
		leadzeroes = prec - (p - bp);
		if (leadzeroes > 0)
		{
		    otherlength = lzero = leadzeroes;
		    flagword |= LZERO;
		}
	    }

	    /* Handle the # flag */
	    if (flagword & FSHARP)
	    {
		if (fcode == 'p')
		    prefix = (UP)"0x";
		else
		    prefix = (UP)"0X";
		prefixlength = 2;
	    }
	    break;

	case 'E': 
	case 'e': 
	    /* 
	     * E-format.  The general strategy
	     * here is fairly easy: we take
	     * what ecvt gives us and re-format it.
	     */

	    /* Establish default precision */
	    if ((!(flagword & DOTSEEN)) || (prec < 0))
		prec = DEFAULT_FLOAT_PREC;

	    /* Fetch the value and develop the mantissa */
#ifndef NO_LONG_DOUBLE
	    if (flagword & LDOUBLE)
	    {
		ldval = va_arg(args, long_double);
#ifdef _THREAD_SAFE
		_ldecvt_r(ldval, min(prec + 1, MAXQECVT), &decpt, &sign, 
			cvtbuf, NDIG);

		bp = (UP)cvtbuf;
#else
		bp = (UP)_ldecvt(ldval,
				min(prec + 1, MAXQECVT), &decpt, &sign);
#endif
		dval = (double)!LDZERO(ldval);
	    }
	    else
#endif
	    {
		dval = va_arg(args, double);
#ifdef _THREAD_SAFE
		ecvt_r(dval, min(prec + 1, MAXECVT), &decpt, &sign, 
			cvtbuf, NDIG);

		bp = (UP)cvtbuf;
#else
		bp = (UP)ecvt(dval,
				min(prec + 1, MAXECVT), &decpt, &sign);
#endif
	    }

	    /* Determine the prefix */
	e_merge: 
	    if (sign)
	    {
		prefix = (UP)"-";
		prefixlength = 1;
	    }
	    else if (flagword & FPLUS)
	    {
		prefix = (UP)"+";
		prefixlength = 1;
	    }
	    else if (flagword & FBLANK)
	    {
		prefix = (UP)" ";
		prefixlength = 1;
	    }

	    /* Place the first digit in the buffer */
	    p = &valbuf[0];
	    *p++ = (*bp != '\0') ? *bp++ : '0';

	    /* Put in a decimal point if needed */
	    if (prec != 0 || (flagword & FSHARP))
		*p++ = _nl_radix;

	    /* Create the rest of the mantissa */
	    {
		register rz = prec;

		for (; rz > 0 && *bp != '\0'; --rz)
		    *p++ = *bp++;

		if (rz > 0)
		{
		    otherlength = rzero = rz;
		    flagword |= RZERO;
		}
	    }

	    bp = &valbuf[0];

	    /* Create the exponent */
	    *(suffix = &expbuf[MAXESIZ]) = '\0';
	    if (dval != 0)
	    {
		register int nn = decpt - 1;
		if (nn < 0)
		    nn = -nn;
		for (; nn > 9; nn /= 10)
		    *--suffix = todigit(nn % 10);
		*--suffix = todigit(nn);
	    }

	    /* Prefix leading zeroes to the exponent */
	    while (suffix > &expbuf[MAXESIZ - 2])
		*--suffix = '0';

	    /* Put in the exponent sign */
	    *--suffix = (decpt > 0 || dval == 0) ? '+' : '-';

	    /* Put in the e */
	    *--suffix = isupper((I)fcode) ? 'E' : 'e';

	    /* compute size of suffix */
	    otherlength += (suffixlength = &expbuf[MAXESIZ]
		    - suffix);
	    flagword |= SUFFIX;

	    break;

	case 'f': 
	    /* 
	     * F-format floating point.
	     * This is a good deal less simple than E-format.
	     * The overall strategy will be to call fcvt, reformat its
	     * result into buf, and calculate how many trailing zeroes
	     * will be required.  There will never be any leading
	     * zeroes needed.
	     */

	    /* Establish default precision */
	    if ((!(flagword & DOTSEEN)) || (prec < 0))
		prec = DEFAULT_FLOAT_PREC;

	    /* Fetch the value and do the conversion */
#ifndef NO_LONG_DOUBLE
	    if (flagword & LDOUBLE)
	    {
		maxfcvt = MAXQFCVT;
		maxfsig = MAXQFSIG;
		ldval = va_arg(args, long_double);
#ifdef _THREAD_SAFE
		_ldfcvt_r(ldval, min(prec, MAXQFCVT), &decpt, &sign, 
			cvtbuf, NDIG);

		bp = (UP)cvtbuf;
#else
		bp = (UP)_ldfcvt(ldval,
				    min(prec, MAXQFCVT), &decpt, &sign);
#endif
	    }
	    else
#endif
	    {
		maxfcvt = MAXFCVT;
		maxfsig = MAXFSIG;
		dval = va_arg(args, double);
#ifdef _THREAD_SAFE
		fcvt_r(dval, min(prec, MAXFCVT), &decpt, &sign, 
			cvtbuf, NDIG);

		bp = (UP)cvtbuf;
#else
		bp = (UP)fcvt(dval, min(prec, MAXFCVT), &decpt, &sign);
#endif
	    }

	    /* Determine the prefix */
	f_merge: 
	    if (sign && decpt > -prec)
	    {
		prefix = (UP)"-";
		prefixlength = 1;
	    }
	    else if (flagword & FPLUS)
	    {
		prefix = (UP)"+";
		prefixlength = 1;
	    }
	    else if (flagword & FBLANK)
	    {
		prefix = (UP)" ";
		prefixlength = 1;
	    }

	    /* Initialize buffer pointer far enough in for digits & grouping */
	    p = m = decpt <= 0 ? &valbuf[1] : &valbuf[decpt+decpt];

	    {
		register int nn = decpt;
#if defined(NLS) || defined(NLS16)
		register char *gp = groupinfo;
		register unsigned char gdigs;
		gdigs = ((flagword & FQUOTE) && *gp)? *gp : CHAR_MAX;
#endif

		/* Emit the digits before the decimal point */
		k = 0;
		do
		{
#if defined(NLS) || defined(NLS16)
		    if (gdigs != CHAR_MAX && !gdigs--) /* grouping here? */
		    {
			*--p = *thousep;
			if (*gp && gp[1])   /* get next group size */
			    ++gp;
			gdigs = *gp - (*gp != CHAR_MAX);
		    }
#endif
		    *--p = (nn <= 0 || nn > maxfsig || bp[nn-1] == '\0')
			   ? '0' : (k++, bp[nn-1]);
		} while (--nn > 0);

		if (decpt > 0)			/* skip past used digits */
		    bp += min(decpt, maxfsig);

		/* Decide whether we need a decimal point */
		if ((flagword & FSHARP) || prec > 0 && _nl_radix)
		    *m++ = _nl_radix;

		/* Digits (if any) after the decimal point */
		nn = min(prec, maxfcvt);
		if (prec > nn)
		{
		    flagword |= RZERO;
		    otherlength = rzero = prec - nn;
		}
		while (--nn >= 0)
		    *m++ = (++decpt <= 0 || *bp == '\0' || k >= maxfsig)
			   ? '0' : (k++, *bp++);
	    }

	    bp = p;
	    p = m;
	    break;

	case 'G': 
	case 'g': 
	    /* 
	     * g-format.
	     * We play around a bit and then jump into e or f,
	     * as needed.
	     */

	    /* Establish default precision */
	    if ((!(flagword & DOTSEEN)) || (prec < 0))
		prec = DEFAULT_FLOAT_PREC;
	    else if (prec == 0)
		prec = 1;

	    /* Fetch the value and do the conversion */
#ifndef NO_LONG_DOUBLE
	    if (flagword & LDOUBLE)
	    {
		maxfcvt = MAXQFCVT;
		maxfsig = MAXQFSIG;
		ldval = va_arg(args, long_double);
#ifdef _THREAD_SAFE
		_ldecvt_r(ldval, min(prec, MAXQECVT), &decpt, &sign, 
				cvtbuf, NDIG);

		bp = (UP)cvtbuf;
#else
		bp = (UP)_ldecvt(ldval,
			min(prec, MAXQECVT), &decpt, &sign);
#endif
		dval = (double)!LDZERO(ldval);
	    }
	    else
#endif
	    {
		maxfcvt = MAXFCVT;
		maxfsig = MAXFSIG;
		dval = va_arg(args, double);
#ifdef _THREAD_SAFE
		ecvt_r(dval, min(prec, MAXECVT), &decpt, &sign, 
			cvtbuf, NDIG);

		bp = (UP)cvtbuf;
#else
		bp = (UP)ecvt(dval,
			min(prec, MAXECVT), &decpt, &sign);
#endif
	    }
	    if (dval == 0)
		decpt = 1;

	    {
		register int kk = prec;
		if (!(flagword & FSHARP))
		{
		    n = strlen((CP)bp);
		    if (n < kk)
			kk = n;
		    while (kk >= 1 && bp[kk - 1] == '0')
			--kk;
		}

		if (decpt < -3 || decpt >= prec + 1)
		{
		    prec = kk - 1;
		    goto e_merge;
		}
		prec = kk - decpt;
		goto f_merge;
	    }

	case '%': 
	    valbuf[0] = fcode;
	    p = (bp = valbuf) + 1;
	    break;

	case 'c': 		/* 8 bit char only, pending ANSI */
	    valbuf[0] = va_arg(args, int);
	    p = (bp = valbuf) + 1;
	    break;

	case 's': 		/* 8 bit char only, pending ANSI */
	    bp = (UP)va_arg(args, char *);
	    p = bp;
	    if (!bp)
		break;
	    if (!(flagword & DOTSEEN) || (prec < 0))
	    {
		while (*p)
		    ++p;
	    }
	    else
	    {
		while (*p && prec-- > 0)
		    ++p;
	    }
	    break;

	case 'C': 		/* Wide character */
	    bp = valbuf;
	    /* convert into multibyte string */
	    if ((n = wctomb(valbuf, va_arg(args, wchar_t))) == -1)
	        return -1;	/* invalid wide character; errno has been set */
	    else
	        p = bp + n;	/* valid wide character, n bytes in length */
	    break;

	case 'S': 		/* Wide character string */
	    ws = ws_start = va_arg(args, wchar_t *);
	    /* Count up the number of bytes that will be in the converted */
	    /* wide string. */
            if ((n = (int)wcstombs((char *)NULL, ws, INT_MAX)) == -1)
		    return -1;	/* errno has been set (probably to EILSEQ) */
	    n++;		/* Add one for null terminating byte */

	    /* Allocate a buffer if there isn't enough static space. */
            if (n > sizeof(valbuf)) {
		if ((mb_buf = (UCHAR *)malloc(n)) == (UCHAR *)0) {
		    return -1;	/* errno has been set */
		}
		bp = mb_buf;
	    }
	    else
		bp = valbuf;

	    if (wcstombs(bp, ws_start, n) == -1) {
		if (mb_buf)
	    	    free(mb_buf);
		return -1;	/* cannot convert to multibyte string */
	    }

	    p = bp;
	    if (!bp)
		break;
	    if (!(flagword & DOTSEEN) || (prec < 0))
	    {
		while (*p)
		    ++p;
	    }
	    else
	    {
		UCHAR *lastp;
		while (*p && prec > 0)
		{
		    prec -= mblen(p, MB_CUR_MAX);	/* Count byte length */
							/*  of this character */
		    lastp = p;				/* Save ptr for later */
		    ADVANCE(p);				/* Go on */
		}
		if (prec < 0)
		{
		    truncated = TRUE;			/* Oops.  Too wide. */
		    p -= mblen(lastp, MB_CUR_MAX);	/* Too much: drop the */
							/*  last character */
		}
	    }
	    break;

	case 'n': 
	    if (flagword & HLENGTH)
	    {
		*va_arg(args, short int *) = numchar;
	    }
	    else
	    {
		*va_arg(args, int *) = numchar;
	    }
#if defined NLS || defined NLS16
	    firstspec = 0;
#endif
	    continue;

	default:
	    /*
	     * this is technically an error; what we do is to back up
	     * the format pointer to the offending char and continue
	     * with the format scan
	     */
	    format -= CHARSIZE(fcode);
#if defined NLS || defined NLS16
	    firstspec = 0;
#endif
	    continue;
	}

#if defined NLS || defined NLS16
	firstspec = 0;
#endif
	k = (n = p - bp) + prefixlength + otherlength;
	if (width <= k)
	    count += k;
	else
	{
	    count += width;
	    truncated = FALSE;

	    /*
	     * Set up for padding zeroes if requested.  Otherwise emit
	     * padding blanks unless output is to be left-justified.
	     */
	    if (flagword & PADZERO)
	    {
		if (!(flagword & LZERO))
		{
		    flagword |= LZERO;
		    lzero = width - k;
		}
		else
		    lzero += width - k;
		k = width;	/* cancel padding blanks */
	    }
	    else
		/* Blanks on left if required */
		if (!(flagword & FMINUS))
		    PAD(_blanks, width - k);
	}

	/* Prefix, if any */
	if (prefixlength != 0)
	    PUT(prefix, prefixlength);

	/* Zeroes on the left */
	if (flagword & LZERO)
	{
	    if (truncated)
	    {
		++lzero;
		truncated = FALSE;
	    }
	    PAD(_zeroes, lzero);
	}

	if (n > 0)
	    PUT(bp, n);

	if (flagword & (RZERO|SUFFIX|FMINUS))
	{
	    /* Zeroes on the right */
	    if (flagword & RZERO)
		PAD(_zeroes, rzero);

	    /* The suffix */
	    if (flagword & SUFFIX)
		PUT(suffix, suffixlength);


	    /* Blanks on the right if required */
	    if (flagword & FMINUS && width > k)
		PAD(_blanks, width - k);
	}

	/* free any allocated multibyte buffer; reset pointer */
	if (mb_buf) {
	    free(mb_buf);
	    mb_buf = 0;
	}
    }
}
