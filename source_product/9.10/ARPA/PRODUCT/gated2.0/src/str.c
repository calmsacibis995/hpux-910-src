/*
 *  $Header: str.c,v 1.1.109.5 92/02/28 16:01:59 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


#include "include.h"
#include "math.h"
#include <ctype.h>
#if	defined(_IBMR2)
#include <time.h>
#endif				/* defined(_IBMR2) */
#include <sys/time.h>


#ifndef vax11c
extern int sys_nerr;
extern char *sys_errlist[];

#define	gd_error(x)	x < sys_nerr ? sys_errlist[x] : "Unknown error number"
#else				/* vax11c */
char *gd_error();

#endif				/* vax11c */

/*
 *	Make a copy of a string and lowercase it
 */
char *
gd_lower(str)
char *str;
{
    char *src, *dst;
    static char *lstr;
    static u_int lstrlen;

    if (lstr && (strlen(str) > lstrlen)) {
	free(lstr);
	lstr = (char *) 0;
	lstrlen = 0;
    }
    if (!lstr) {
	lstrlen = strlen(str) + 1;
	lstr = (char *) calloc(1, lstrlen);
	if (!lstr) {
	    trace(TR_ALL, LOG_ERR, "gd_tolower: calloc: %m");
	    quit(errno);
	}
    }
    src = str;
    dst = lstr;
    while (*src) {
	*dst++ = isupper(*src) ? tolower(*src) : *src;
	src++;
    }
    *dst = (char) 0;

    return (lstr);
}


/*
 *	Gated version of fprintf
 */
#ifdef	STDARG
/*VARARGS2*/
int
fprintf(FILE * stream, const char *format,...)
#else				/* STDARG */
/*ARGSUSED*/
/*VARARGS0*/
int
fprintf(va_alist)
va_dcl

#endif				/* STDARG */
{
    int rc;
    va_list ap;
    char buffer[BUFSIZ];

#ifdef	STDARG
    va_start(ap, format);
#else				/* STDARG */
    const char *format;
    FILE *stream;

    va_start(ap);

    stream = va_arg(ap, FILE *);
    format = va_arg(ap, const char *);
#endif				/* STDARG */
    rc = vsprintf(buffer, format, &ap);
    fputs((char *) buffer, stream);

    va_end(ap);
    return (rc);
}


/*
 *	Gated version of sprintf
 */
#ifdef	STDARG
/*VARARGS2*/
int
sprintf(char *s, const char *format,...)
#else				/* STDARG */
/*ARGSUSED*/
/*VARARGS0*/
int
sprintf(va_alist)
va_dcl

#endif				/* STDARG */
{
    int rc;
    va_list ap;

#ifdef	STDARG

    va_start(ap, format);
#else				/* STDARG */
    const char *format;
    char *s;

    va_start(ap);

    s = va_arg(ap, char *);
    format = va_arg(ap, const char *);
#endif				/* STDARG */
    rc = vsprintf(s, format, &ap);

    va_end(ap);
    return (rc);
}


#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)doprnt.c	5.35 (Berkeley) 6/27/88";

#endif				/* LIBC_SCCS and not lint */

/* 11-bit exponent (VAX G floating point) is 308 decimal digits */
#define	MAXEXP		308
/* 128 bit fraction takes up 39 decimal digits; max reasonable precision */
#define	MAXFRACT	39

#define	DEFPREC		6

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */

#define	PUTC(ch)	*dp++ = ch; cnt++;

#define	ARG() \
	_ulong = flags&LONGINT ? va_arg(*argp, long) : \
	    flags&SHORTINT ? va_arg(*argp, short) : va_arg(*argp, int);

#define	todigit(c)	((c) - '0')
#define	tochar(n)	((n) + '0')

/* have to deal with the negative buffer count kludge */
#define	NEGATIVE_COUNT_KLUDGE

#define	LONGINT		0x01		/* long integer */
#define	LONGDBL		0x02		/* long double; unimplemented */
#define	SHORTINT	0x04		/* short integer */
#define	ALT		0x08		/* alternate form */
#define	LADJUST		0x10		/* left adjustment */
#define	ZEROPAD		0x20		/* zero (as opposed to blank) pad */
#define	HEXPREFIX	0x40		/* add 0x or 0X prefix */


static char *
round(fract, expon, start, end, ch, signp)
double fract;
int *expon;
register char *start, *end;
char ch, *signp;
{
    double tmp;

    if (fract)
	(void) modf(fract * 10, &tmp);
    else
	tmp = (double) todigit(ch);
    if (tmp > 4)
	for (;; --end) {
	    if (*end == '.')
		--end;
	    if (++*end <= '9')
		break;
	    *end = '0';
	    if (end == start) {
		if (expon) {		/* e/E; increment exponent */
		    *end = '1';
		    ++*expon;
		} else {		/* f; add extra digit */
		    *--end = '1';
		    --start;
		}
		break;
	    }
	}
    /* ``"%.3f", (double)-0.0004'' gives you a negative 0. */
    else if (*signp == '-')
	for (;; --end) {
	    if (*end == '.')
		--end;
	    if (*end != '0')
		break;
	    if (end == start)
		*signp = 0;
	}
    return (start);
}

static char *
exponent(p, expon, fmtch)
register char *p;
register int expon;
char fmtch;
{
    register char *t;
    char expbuf[MAXEXP];

    *p++ = fmtch;
    if (expon < 0) {
	expon = -expon;
	*p++ = '-';
    } else
	*p++ = '+';
    t = expbuf + MAXEXP;
    if (expon > 9) {
	do {
	    *--t = tochar(expon % 10);
	} while ((expon /= 10) > 9);
	*--t = tochar(expon);
	for (; t < expbuf + MAXEXP; *p++ = *t++) ;
    } else {
	*p++ = '0';
	*p++ = tochar(expon);
    }
    return (p);
}


static int
cvt(number, prec, flags, signp, fmtch, startp, endp)
double number;
register int prec;
int flags;
char fmtch;
char *signp, *startp, *endp;
{
    register char *p, *t;
    register double fract;
    int dotrim, expcnt, gformat;
    double integer, tmp;

    dotrim = expcnt = gformat = 0;
    fract = modf(number, &integer);

    /* get an extra slot for rounding. */
    t = ++startp;

    /*
     * get integer portion of number; put into the end of the buffer; the
     * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
     */
    for (p = endp - 1; integer; ++expcnt) {
	tmp = modf(integer / 10, &integer);
	*p-- = tochar((int) ((tmp + .01) * 10));
    }
    switch (fmtch) {
	case 'f':
	    /* reverse integer into beginning of buffer */
	    if (expcnt)
		for (; ++p < endp; *t++ = *p) ;
	    else
		*t++ = '0';
	    /*
	     * if precision required or alternate flag set, add in a
	     * decimal point.
	     */
	    if (prec || flags & ALT)
		*t++ = '.';
	    /* if requires more precision and some fraction left */
	    if (fract) {
		if (prec)
		    do {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    } while (--prec && fract);
		if (fract)
		    startp = round(fract, (int *) NULL, startp,
				   t - 1, (char) 0, signp);
	    }
	    for (; prec--; *t++ = '0') ;
	    break;
	case 'e':
	case 'E':
	  eformat:if (expcnt) {
		*t++ = *++p;
		if (prec || flags & ALT)
		    *t++ = '.';
		/* if requires more precision and some integer left */
		for (; prec && ++p < endp; --prec)
		    *t++ = *p;
		/*
		 * if done precision and more of the integer component,
		 * round using it; adjust fract so we don't re-round
		 * later.
		 */
		if (!prec && ++p < endp) {
		    fract = 0.0;
		    startp = round((double) 0, &expcnt, startp,
				   t - 1, *p, signp);
		}
		/* adjust expcnt for digit in front of decimal */
		--expcnt;
	    }
	    /* until first fractional digit, decrement exponent */
	    else if (fract) {
		/* adjust expcnt for digit in front of decimal */
		for (expcnt = -1;; --expcnt) {
		    fract = modf(fract * 10, &tmp);
		    if (tmp)
			break;
		}
		*t++ = tochar((int) tmp);
		if (prec || flags & ALT)
		    *t++ = '.';
	    } else {
		*t++ = '0';
		if (prec || flags & ALT)
		    *t++ = '.';
	    }
	    /* if requires more precision and some fraction left */
	    if (fract) {
		if (prec)
		    do {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    } while (--prec && fract);
		if (fract)
		    startp = round(fract, &expcnt, startp,
				   t - 1, (char) 0, signp);
	    }
	    /* if requires more precision */
	    for (; prec--; *t++ = '0') ;

	    /* unless alternate flag, trim any g/G format trailing 0's */
	    if (gformat && !(flags & ALT)) {
		while (t > startp && *--t == '0') ;
		if (*t == '.')
		    --t;
		++t;
	    }
	    t = exponent(t, expcnt, fmtch);
	    break;
	case 'g':
	case 'G':
	    /* a precision of 0 is treated as a precision of 1. */
	    if (!prec)
		++prec;
	    /*
	     * ``The style used depends on the value converted; style e
	     * will be used only if the exponent resulting from the
	     * conversion is less than -4 or greater than the precision.''
	     *	-- ANSI X3J11
	     */
	    if (expcnt > prec || !expcnt && fract && fract < .0001) {
		/*
		 * g/G format counts "significant digits, not digits of
		 * precision; for the e/E format, this just causes an
		 * off-by-one problem, i.e. g/G considers the digit
		 * before the decimal point significant and e/E doesn't
		 * count it as precision.
		 */
		--prec;
		fmtch -= 2;		/* G->E, g->e */
		gformat = 1;
		goto eformat;
	    }
	    /*
	     * reverse integer into beginning of buffer,
	     * note, decrement precision
	     */
	    if (expcnt)
		for (; ++p < endp; *t++ = *p, --prec) ;
	    else
		*t++ = '0';
	    /*
	     * if precision required or alternate flag set, add in a
	     * decimal point.  If no digits yet, add in leading 0.
	     */
	    if (prec || flags & ALT) {
		dotrim = 1;
		*t++ = '.';
	    } else
		dotrim = 0;
	    /* if requires more precision and some fraction left */
	    if (fract) {
		if (prec) {
		    do {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    } while (!tmp);
		    while (--prec && fract) {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    }
		}
		if (fract)
		    startp = round(fract, (int *) NULL, startp,
				   t - 1, (char) 0, signp);
	    }
	    /* alternate format, adds 0's for precision, else trim 0's */
	    if (flags & ALT)
		for (; prec--; *t++ = '0') ;
	    else if (dotrim) {
		while (t > startp && *--t == '0') ;
		if (*t != '.')
		    ++t;
	    }
    }
    return (t - startp);
}


int
vsprintf(dest, fmt0, argp)
char *dest;
const char *fmt0;
va_list *argp;
{
    register const char *fmt;		/* format string */
    register char *dp;			/* Destination pointer */
    register int ch;			/* character from fmt */
    register int cnt;			/* return value accumulator */
    register int n;			/* random handy integer */
    register char *t;			/* buffer pointer */
    double _double;			/* double precision arguments %[eEfgG] */
    u_long _ulong;			/* integer arguments %[diouxX] */
    int base = 10;			/* base for [diouxX] conversion */
    int dprec;				/* decimal precision in [diouxX] */
    int fieldsz;			/* field size expanded by sign, etc */
    int flags;				/* flags as above */
    int fpprec;				/* `extra' floating precision in [eEfgG] */
    int prec;				/* precision from format (%.3d), or -1 */
    int realsz;				/* field size expanded by decimal precision */
    int size;				/* size of converted field or string */
    int width;				/* width from format (%8d), or 0 */
    char sign;				/* sign prefix (' ', '+', '-', or \0) */
    char softsign;			/* temporary negative sign for floats */
    const char *digs;			/* digits for [diouxX] conversion */
    char buf[BUF];			/* space for %c, %[diouxX], %[eEfgG] */
    int error_number = errno;

    dp = dest;
    fmt = fmt0;
    digs = "0123456789abcdef";
    for (cnt = 0;; ++fmt) {
	for (; (ch = *fmt) && ch != '%'; ++fmt) {
	    PUTC(ch);
	}
	if (!ch) {
	    PUTC(ch);
	    return (--cnt);
	}
	flags = 0;
	dprec = 0;
	fpprec = 0;
	width = 0;
	prec = -1;
	sign = '\0';

      rflag:switch (*++fmt) {
	    case ' ':
		/*
		 * ``If the space and + flags both appear, the space
		 * flag will be ignored.''
		 *	-- ANSI X3J11
		 */
		if (!sign)
		    sign = ' ';
		goto rflag;
	    case '#':
		flags |= ALT;
		goto rflag;
	    case '*':
		/*
		 * ``A negative field width argument is taken as a
		 * - flag followed by a  positive field width.''
		 *	-- ANSI X3J11
		 * They don't exclude field widths read from args.
		 */
		if ((width = va_arg(*argp, int)) >= 0)
		    goto rflag;
		width = -width;
		/* FALLTHROUGH */
	    case '-':
		flags |= LADJUST;
		goto rflag;
	    case '+':
		sign = '+';
		goto rflag;
	    case '.':
		if (*++fmt == '*')
		    n = va_arg(*argp, int);
		else {
		    n = 0;
		    while (isascii(*fmt) && isdigit(*fmt))
			n = 10 * n + todigit(*fmt++);
		    --fmt;
		}
		prec = n < 0 ? -1 : n;
		goto rflag;
	    case '0':
		/*
		 * ``Note that 0 is taken as a flag, not as the
		 * beginning of a field width.''
		 *	-- ANSI X3J11
		 */
		flags |= ZEROPAD;
		goto rflag;
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		n = 0;
		do {
		    n = 10 * n + todigit(*fmt);
		} while (isascii(*++fmt) && isdigit(*fmt));
		width = n;
		--fmt;
		goto rflag;
	    case 'L':
		flags |= LONGDBL;
		goto rflag;
	    case 'h':
		flags |= SHORTINT;
		goto rflag;
	    case 'l':
		flags |= LONGINT;
		goto rflag;
	    case 'c':
		*(t = buf) = va_arg(*argp, int);
		size = 1;
		sign = '\0';
		goto pforw;
	    case 'D':
		flags |= LONGINT;
		/*FALLTHROUGH*/
	    case 'd':
	    case 'i':
		ARG();
		if ((long) _ulong < 0) {
		    _ulong = -_ulong;
		    sign = '-';
		}
		base = 10;
		goto number;
	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
	    case 'G':
		_double = va_arg(*argp, double);
		/*
		 * don't do unrealistic precision; just pad it with
		 * zeroes later, so buffer size stays rational.
		 */
		if (prec > MAXFRACT) {
		    if (*fmt != 'g' && *fmt != 'G' || (flags & ALT))
			fpprec = prec - MAXFRACT;
		    prec = MAXFRACT;
		} else if (prec == -1)
		    prec = DEFPREC;
		/*
		 * softsign avoids negative 0 if _double is < 0 and
		 * no significant digits will be shown
		 */
		if (_double < 0) {
		    softsign = '-';
		    _double = -_double;
		} else
		    softsign = 0;
		/*
		 * cvt may have to round up past the "start" of the
		 * buffer, i.e. ``intf("%.2f", (double)9.999);'';
		 * if the first char isn't NULL, it did.
		 */
		*buf = NULL;
		size = cvt(_double, prec, flags, &softsign, *fmt, buf,
			   buf + sizeof(buf));
		if (softsign)
		    sign = '-';
		t = *buf ? buf : buf + 1;
		goto pforw;
	    case 'n':
		if (flags & LONGINT)
		    *va_arg(*argp, long *) = cnt;
		else if (flags & SHORTINT)
		    *va_arg(*argp, short *) = cnt;
		else
		    *va_arg(*argp, int *) = cnt;
		break;
	    case 'O':
		flags |= LONGINT;
		/*FALLTHROUGH*/
	    case 'o':
		ARG();
		base = 8;
		goto nosign;
	    case 'p':
		/*
		 * ``The argument shall be a pointer to void.  The
		 * value of the pointer is converted to a sequence
		 * of printable characters, in an implementation-
		 * defined manner.''
		 *	-- ANSI X3J11
		 */
		/* NOSTRICT */
		_ulong = (u_long) va_arg(*argp, void *);
		base = 16;
		goto nosign;
	    case 'T':
		/* Time */
		{
		    time_t time = va_arg(*argp, time_t);
		    struct tm *tm;

		    tm = localtime(&time);
		    if (tm->tm_year < 70) {
			tm = gmtime(&time);
		    }
		    t = buf + BUF;
		    *--t = (char) 0;
		    *--t = digs[tm->tm_sec % 10];
		    *--t = digs[tm->tm_sec / 10];
		    *--t = ':';
		    *--t = digs[tm->tm_min % 10];
		    *--t = digs[tm->tm_min / 10];
		    *--t = ':';
		    *--t = digs[tm->tm_hour % 10];
		    *--t = digs[tm->tm_hour / 10];
		}
		goto string;
	    case 'A':
		/* socket address */
		{
		    register u_char *cp, *cp1;
		    register sockaddr_un *addr;

		    cp = cp1 = (u_char *) 0;
		    addr = va_arg(*argp, sockaddr_un *);

		    if (!addr) {
			strcpy(buf, "??*sockaddr??");
			t = buf;
			break;
		    }
		    t = buf + BUF;
		    *--t = (char) 0;
		    *buf = (char) 0;

		    switch (addr->a.sa_family) {
			case AF_UNSPEC:
			    strcpy(buf, "*Unspecified*");
			    t = (char *) 0;
			    break;

			case AF_INET:
			    base = 10;

			    if (flags & ALT) {
				if (_ulong = ntohs(addr->in.sin_port)) {
				    do {
					*--t = digs[_ulong % base];
				    } while (_ulong /= base);
				    *--t = '/';
				}
			    }
			    cp1 = (u_char *) & addr->in.sin_addr.s_addr;

			    cp = cp1 + sizeof(addr->in.sin_addr.s_addr);

			    break;
#ifdef	ISOPROTO_RAW
			case AF_ISO:
			    base = 16;

			    cp1 = (u_char *) addr->iso.siso_addr.isoa_genaddr;
			    cp = cp1 + addr->iso.siso_addr.isoa_len;

			    break;
#endif				/* ISOPROTO_RAW */
#ifdef	AF_LINK
			case AF_LINK:
			    /* Format link level address as: '#<index> <name> <link_address>" */

			    /* Assume index is always present */
			    strcpy(buf, "#");
			    cp = (u_char *) t;
			    base = 10;
			    _ulong = addr->dl.sdl_index;
			    do {
				*--t = digs[_ulong % base];
			    } while (_ulong /= base);
			    strcat(buf, t);
			    t = (char *) cp;

			    /* Add name if present */
			    if (addr->dl.sdl_nlen) {
				strcat(buf, " ");
				strncat(buf, addr->dl.sdl_data, addr->dl.sdl_nlen);
			    }
			    /* Set up to display the link level address in HEX if present */
			    if (addr->dl.sdl_alen + addr->dl.sdl_slen) {
				strcat(buf, " ");
				base = 16;
				cp1 = (u_char *) addr->dl.sdl_data + addr->dl.sdl_nlen;
				cp = cp1 + addr->dl.sdl_alen + addr->dl.sdl_slen;
			    } else {
				cp = cp1 = (u_char *) 0;
			    }

#endif				/* AF_LINK */
			default:
			    base = 16;

			    cp1 = (u_char *) addr;
			    cp = cp1 + (socksize(addr) ? socksize(addr) : sizeof(sockaddr_un));
		    }

		    if (t) {
			if (cp > cp1) {
			    if (cp > (cp1 + socksize(addr))) {
				cp = cp1 + socksize(addr);
			    }
			    do {
				_ulong = *--cp;
				do {
				    *--t = digs[_ulong % base];
				} while (_ulong /= base);
				*--t = '.';
			    } while (cp > cp1);
			    t++;
			}
			if (*buf) {
			    t -= strlen(buf);
			    strcpy(t, buf);
			}
		    } else {
			t = buf;
		    }
		}
		goto string;
	    case 'm':
		strcpy(t = buf, gd_error(error_number));
		goto string;
	    case 's':
		t = va_arg(*argp, char *);
		if (!t) {
		    strcpy(buf, "(null)");
		    t = buf;
		}
	      string:if (prec >= 0) {
		    for (size = 0; (size < prec) && t[size]; size++) ;
		} else
		    size = strlen(t);
		sign = '\0';
		goto pforw;
	    case 'U':
		flags |= LONGINT;
		/*FALLTHROUGH*/
	    case 'u':
		ARG();
		base = 10;
		goto nosign;
	    case 'B':
		digs = "_|";
		/* FALLTHROUGH */
	    case 'b':
		ARG();
		base = 2;
		goto binhex;
	    case 'X':
		digs = "0123456789ABCDEF";
		/* FALLTHROUGH */
	    case 'x':
		ARG();
		base = 16;
	      binhex:			/* leading 0x/X only if non-zero */
		if (flags & ALT && _ulong != 0)
		    flags |= HEXPREFIX;

		/* unsigned conversions */
	      nosign:sign = '\0';
		/*
		 * ``... diouXx conversions ... if a precision is
		 * specified, the 0 flag will be ignored.''
		 *	-- ANSI X3J11
		 */
	      number:if ((dprec = prec) >= 0)
		    flags &= ~ZEROPAD;

		/*
		 * ``The result of converting a zero value with an
		 * explicit precision of zero is no characters.''
		 *	-- ANSI X3J11
		 */
		t = buf + BUF;
		if (_ulong != 0 || prec != 0) {
		    do {
			*--t = digs[_ulong % base];
			_ulong /= base;
		    } while (_ulong);
		    if (flags & ALT && base == 8 && *t != *digs)
			*--t = *digs;	/* octal leading 0 */
		}
		size = buf + BUF - t;

	      pforw:
		/*
		 * All reasonable formats wind up here.  At this point,
		 * `t' points to a string which (if not flags&LADJUST)
		 * should be padded out to `width' places.  If
		 * flags&ZEROPAD, it should first be prefixed by any
		 * sign or other prefix; otherwise, it should be blank
		 * padded before the prefix is emitted.  After any
		 * left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print
		 * the string proper, then emit zeroes required by any
		 * leftover floating precision; finally, if LADJUST,
		 * pad with blanks.
		 */

		/*
		 * compute actual size, so we know how much to pad
		 * fieldsz excludes decimal prec; realsz includes it
		 */
		fieldsz = size + fpprec;
		if (sign)
		    fieldsz++;
		if (flags & HEXPREFIX)
		    fieldsz += 2;
		realsz = dprec > fieldsz ? dprec : fieldsz;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST | ZEROPAD)) == 0 && width)
		    for (n = realsz; n < width; n++)
			PUTC(' ');
		/* prefix */
		if (sign)
		    PUTC(sign);
		if (flags & HEXPREFIX) {
		    PUTC(*digs);
		    PUTC((char) *fmt);
		}
		/* right-adjusting zero padding */
		if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD)
		    for (n = realsz; n < width; n++)
			PUTC(*digs);
		/* leading zeroes from decimal precision */
		for (n = fieldsz; n < dprec; n++)
		    PUTC(*digs);

		/* the string or number proper */
		memcpy((char *) dp, t, size);
		dp += size;
		cnt += size;
		/* trailing f.p. zeroes */
		while (--fpprec >= 0)
		    PUTC(*digs);
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
		    for (n = realsz; n < width; n++)
			PUTC(' ');
		digs = "0123456789abcdef";
		break;
	    case '\0':			/* "%?" prints ?, unless ? is NULL */
		PUTC((char) *fmt);
		return (--cnt);
	    default:
		PUTC((char) *fmt);
	}
    }
    /* NOTREACHED */
}
