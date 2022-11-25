/* $Header: strfmon.c,v 72.13 94/05/05 13:06:56 ssa Exp $ */
/*LINTLIBRARY*/

/*
 * strfmon.c: It places characters into the array pointed by outbuf as
 * controlled by the string pointed by format. No more than maxsize 
 * bytes are placed into the array.
 */

#ifdef _NAMESPACE_CLEAN
#define strfmon    _strfmon
#define strlen     _strlen
#define strcpy     _strcpy
#define strncpy    _strncpy
#define localeconv _localeconv
#define memcpy     _memcpy
#define fcvt       _fcvt
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <locale.h>
#include <varargs.h>
#include <stdlib.h>
#include <monetary.h>
#include <nl_ctype.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include "../stdio/print.h"

extern int errno;

extern int _strfmon();
extern struct lconv *localeconv();

#ifdef _NAMESPACE_CLEAN
#undef strfmon
#pragma _HP_SECONDARY_DEF _strfmon strfmon
#define strfmon _strfmon
#endif /* _NAMESPACE_CLEAN */

#define CHARSIZE(x)  ((x) & 0xff00 ? 2 : 1)
#define UCHAR   unsigned char
#define UINT    unsigned int

/* bit positions for flags used in strfmon */
#define FNOGROUP 0x1	/* ^ */
#define FPNEG    0x2	/* ( */
#define FNOCUR   0x4	/* ! */
#define FLJUST   0x8	/* - */
#define FRPRECI  0x10	/* . */
#define FNATION  0x20	/* n */
#define FLPRECI  0x40	/* # */

/* number of characters in a currency symbol. For most locale, a size
 * of 5 will be enough (eg. "USD \0"), but it is made much bigger to
 * pay safe.
 */
#define SYMBOLSZ  20
#define BUFSIZE  100	/* buffer size for internal buffers */
#define PADSIZE  10 	/* buffer size for sign padding */
#define LEFT     0	/* the sign is to the left of the value */
#define RIGHT    1	/* the sign is to the right of the value */

/* -----------------------------
 * PUTBUF()
 * Put a string of n characters pointed to by p into the output buffer, outbuf.
 * Program returns as an error if outbuf is not big enough.
 * -----------------------------
 */ 
#define PUTBUF(p, n) 							\
{									\
	UCHAR *newbufptr;						\
									\
	if ((newbufptr = bufptr + n) > (UCHAR *)outbuf + maxsize - 1) {	\
		errno = E2BIG;						\
		return((ssize_t)-1);					\
	} else {							\
		(void) memcpy((void *)bufptr, (void *)p, (size_t)n);	\
		bufptr = newbufptr;					\
	}								\
}

/* -----------------------------
 * CS_PRECEDES():
 * Depending on the value is positive or negative,
 * returns if the correct [p|n]_cs_precedes value.
 * -----------------------------
 */
#define CS_PRECEDES(isnega) (isnega ? lc->n_cs_precedes : lc->p_cs_precedes)

/* -----------------------------
 * SIGN_POSN():
 * Depending on the value is positive or negative,
 * returns if the correct [p|n]_sign_posn value.
 * -----------------------------
 */
#define SIGN_POSN(isnega) (isnega ? lc->n_sign_posn : lc->p_sign_posn)

/* -----------------------------
 * str2num()
 * converts the string pointed to by *p to a numeric value.
 * Note 1: the value of the *p may be updated (ADVANCEed).
 * Note 2: We don't use CHARADV, etc. in this routine, because we knew
 *         the string pointed by p should be decimal digits (single byte).
 * Note 3: return 0 if the converted number overflows.
 * -----------------------------
 */
static int str2num(p)
UCHAR **p;
{
	int n = 0;
	int c;
	
	for (;;) {
		c = (int)**p;
		if (isdigit(c)) {
			n = n * 10 + c - '0';
			++(*p);		/* skip over the char read */
			if (n < 0)	/* numeric overflow */
				return(0);
		} else
			break;
	}
	return(n);
}

/* -----------------------------
 * put_sign():
 * put a sign (as specified by the locale) into the buf starting
 * from the position pointed to by ptr. After the sign is put,
 * the ptr is updated to reflect the next available slot in buf.
 * Note that when the left precision flag (#) is on, the sign 
 * is padded with blanks to make sure positive/negative signs are
 * of the same length.
 * -----------------------------
 */
static void put_sign(buf, ptr, lbuflen, rbuflen, signat, flagword, prnega, lc)
UCHAR        buf[];
int          *ptr;
int          *lbuflen;	/* ptr to number of padding spaces to the left */
int          *rbuflen;	/* ptr to number of padding spaces to the right */
int          signat;	/* the sign is to the left or right of value */
int          flagword;	/* conversion flags */
int          prnega;	/* print as negative? */
struct lconv *lc;	/* locale component pointer */
{
	int p, n, i, lendiff; 

	if (flagword & FPNEG)	
		/* user provided '(' flag overrides locale sign */
		return;

	/* the sign_posn overrides the sign used, so if sign_posn is 0,
	 * it uses (), which is of length 2.
	 */
	p = lc->p_sign_posn == 0 ? 2 : strlen(lc->positive_sign);
	n = lc->n_sign_posn == 0 ? 2 : strlen(lc->negative_sign);

	/* before putting the sign into buf, pad space char (if needed) 
	 * to ensure the positive and negative sign are of the same length.
	 * We need to do so only if we are printing the value with the
	 * the shorter sign length.
	 */
	/* Strangely, this padding is only done when the # option 
	 * is specified?! -fay 9/23/92
	 */
	if (flagword & FLPRECI) {
		if ((lendiff = (prnega ? p-n : n-p)) > 0) {
			/* The opposite sign is longer in length. 
			 * We put the padding according whether the opposite 
			 * sign is to the left or right of the number
			 */
			if (CS_PRECEDES(!prnega)) {
				switch(SIGN_POSN(!prnega)) {
				case 0:
				/* This is really a special case, the
				 * opposite sign uses (). If it's own sign
				 * is of length 0, then pad both sides; else
				 * it really should pad at the other side. 
				 */
					if (lendiff == 2) 
						*lbuflen = *rbuflen = 1;
					else if (signat == LEFT)
						*rbuflen = 1;
					else 
						*lbuflen = 1;
					break;
				case 1:
				case 3:
				case 4:
				/* all these cases have sign to the left
				 * of the value, so pad to the left.
				 */
					*lbuflen = lendiff;
					break;
				case 2:
					*rbuflen = lendiff;
					break;
				}		
			} else {
				switch(SIGN_POSN(!prnega)) {
				case 0:
					if (lendiff == 2) 
						*lbuflen = *rbuflen = 1;
					else if (signat == LEFT)
						*rbuflen = 1;
					else
						*lbuflen = 1;
					break;
				case 1:
					*lbuflen = lendiff;
					break;
				case 2:
				case 3:
				case 4:
					*rbuflen = lendiff;
					break;
				}
			} /* if CS_PRECEDES */
		} /* if lendiff > 0 */
	} /* if flagword & FPNEG */

	/* now put the sign into buf, and update the free slot pointer, ptr */
	if (prnega) {
		(void)strcpy((char *)&buf[*ptr], lc->negative_sign);
		*ptr += n;
	} else {
		(void)strcpy((char *)&buf[*ptr], lc->positive_sign);
		*ptr += p;
	}
} /* end put_sign() */

/* -----------------------------
 * put_sep():
 * put a space as a separator (between the currency symbol and the monetary 
 * value) into buffer, buf, at the slot pointed to by ptr. Then update 
 * ptr to reflect the next free slot.
 * -----------------------------
 */
static void put_sep(buf, ptr, flagword, prnega, lc)
UCHAR        buf[];
int          *ptr;
int          flagword;	/* conversion flags */
int          prnega;	/* print as negative? */
struct lconv *lc;	/* locale component pointer */
{
	if ((flagword & FNOCUR) ||   /* no currency symbol => no separator */
	    (prnega  && !lc->n_sep_by_space) ||
	    (prnega  &&  lc->n_sep_by_space == CHAR_MAX) ||
	    (!prnega && !lc->p_sep_by_space) ||
	    (!prnega &&  lc->p_sep_by_space == CHAR_MAX))
		return;

	buf[(*ptr)++] = ' ';
}

/* -----------------------------
 * put_symbol():
 * put a currency symbol (csymbol) to the buf.
 * -----------------------------
 */
static void put_symbol(buf, ptr, flagword, csymbol)
UCHAR buf[];
int   *ptr;
int   flagword;		/* conversion flags */
char  *csymbol;		/* the currency symbol */
{
	if (flagword & FNOCUR)
		return;

	(void)strcpy((char *)&buf[*ptr], csymbol);
	*ptr += strlen(csymbol);
}

/* -----------------------------
 * strfmon()
 * Note the logic flow of this routine is similar to that of
 * doprnt.c. In fact, some blocks of codes (eg. the grouping) 
 * are copied from there.
 * -----------------------------
 */
/*VARARGS3*/
ssize_t strfmon (outbuf, maxsize, format, va_alist)
char *outbuf;
size_t maxsize;
const char *format;
va_dcl
{
	/* -----------------------------
	 *  	VARIABLE DECLARATIONS
	 * -----------------------------
	 */

	va_list args;
	UINT fcode = 0;		/* format code */
	UCHAR *bp;		/* start pointer for printing */
	UCHAR *bufptr;		/* buffer pointer */
	int n, k, nn;		/* generic counters */
	UCHAR *fmtptr;		/* pointer to the format */
	struct lconv *lc;	/* the locale component pointer */
	int flagword;		/* conversion flags */
	char csymbol[SYMBOLSZ];	/* the currency symbol */
	int prnega;		/* print as negative? */

	/* Values are developed in this buffer */
	UCHAR valbuf[max(MAXDIGS+MAXIGRP, 10 + max(max(MAXFCVT+MAXFGRP,
	                      MAXECVT + MAXESIZ), max(MAXQFCVT+MAXQFGRP, 
	                      MAXQECVT + MAXESIZ)))];
	UCHAR *headp, *tailp;	/* head/tail pointer to valbuf */

	UCHAR prebuf[BUFSIZE];	/* buffer to hold prefix string */
	int prelen;		/* number of chars in prebuf */
	UCHAR fillchar = ' ';	/* fill char as specified by = flag */

	UCHAR sufbuf[BUFSIZE];	/* buffer to hold suffix string */
	int suflen;		/* number of chars in sufbuf */

	UCHAR fwdbuf[BUFSIZE];	/* hold field width padding spaces */
	int fwdlen;		/* number of chars in fwdbuf */
	int fwidth = 0;		/* field width */

	UCHAR padbuf[PADSIZE];	/* hold left pad space for sign identation */
	int lpadlen;		/* number of pad spaces in padbuf */
	int rpadlen;		/* number of pad spaces in padbuf */

	int lpreci, rpreci;	/* left and right precision */
	UCHAR gdigs;		/* number of digits in a group */
	char *gp;		/* numeric group pointer */
	double dval;		/* the numeric double value */
	int ndig;		/* number of digits before decimal pt */
	int decpt; 		/* decimal point for fcvt(3C) */
	int nega;		/* negative flag for fcvt(3C) */

	/* -----------------------------
	 *  	MAIN BODY
	 * -----------------------------
	 */

	bufptr = (UCHAR *)outbuf;
	fmtptr = (UCHAR *)format;

	lc = localeconv();

	/* init the argument stack */
	va_start(args); 

	/* The main loop -- this loop goes through one iteration for
	 * each string of plain characters or conversion specification.
	 */
	for (;;) {
		bp = (UCHAR *)fmtptr;
		fcode = CHARADV(fmtptr);

		/* plain characters */
		if (fcode != '\0' && fcode != '%') {
			do {
				fcode = CHARADV(fmtptr);
			} while (fcode != '\0' && fcode != '%');
			
			n = (fmtptr - bp) - 1;
			PUTBUF(bp, n);
			bp = fmtptr - 1;	/* update the begin pointer */
		}

		/* end of the format string, return */
		if (fcode == '\0') {
			/* close argument stack */
			va_end(args);
			/* plant terminating null character */
			*bufptr = '\0';
			return((ssize_t)(bufptr - (UCHAR *)outbuf));
		}

		/* % has been found */
		fcode = CHARAT(fmtptr);
		if (fcode == '%') { 	/* got %% */
			ADVANCE(fmtptr);
			n = (fmtptr - bp) - 1;
			PUTBUF(bp, n);
			continue;
		}
		
		/* The following switch is used to parse the conversion
		 * spec and to perform the operation specified by the
		 * conversion character. The program repeatedly goes
		 * back to this switch until the conversion character is
		 * encountered.
		 */
		flagword = fwidth = lpreci = rpreci = 0;  /* reset values */
		fillchar = ' ';
		*csymbol = '\0';

charswitch:
		fcode = CHARADV(fmtptr);
	
		switch(fcode) {
		case '=':
			/* for performance, no need to use CHARADV(fmtptr)
			 * since spec says fillchar is single byte.
			 */
			fillchar = *fmtptr++;
			goto charswitch;

		case '^':
			flagword |= FNOGROUP; 
			goto charswitch;
		
		case '(':
			flagword |= FPNEG;
			goto charswitch;

		case '+':
			/* if both ( and + are specified, the later is taken */
			flagword &= ~FPNEG;
			goto charswitch;

		case '!':
			flagword |= FNOCUR;
			goto charswitch;

		case '-':
			flagword |= FLJUST;
			goto charswitch;

		case '0':  /* compute the field width */
		case '1':  /* note: 0 is a valid field width */
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			fwidth = fcode - '0';
			for (;;) {
				fcode = *fmtptr;
				if (isdigit((int)fcode)) {
					fwidth = fwidth * 10 + fcode - '0';
					++fmtptr;
				} else
					break;
			}
			if (fwidth < 0)	{	/* int overflow */
				fwidth = 0;
				break;
			}
			goto charswitch;

		case '#': /* compute the left precision */
			flagword |= FLPRECI;
			lpreci = str2num(&fmtptr);
			goto charswitch;

		case '.': /* compute the right precision */
			flagword |= FRPRECI;
			/* if the decimal point is not defined (as in C locale),
			 * should not pick up the right precision value.
			 */
			rpreci = *lc->mon_decimal_point ? str2num(&fmtptr) : 0;
			goto charswitch;

		case 'i': /* got conversion char, start output */		
		case 'n':
			if (fcode == 'i') {
				(void)strncpy(csymbol, lc->int_curr_symbol, SYMBOLSZ);
				if (!(flagword & FRPRECI)) 
					/* use default right precision */	
					rpreci = (int)lc->int_frac_digits;
			} else {
				flagword |= FNATION;
				(void)strncpy(csymbol, lc->currency_symbol, SYMBOLSZ);
					if (!(flagword & FRPRECI)) 
					/* use default right precision */	
					rpreci = (int)lc->frac_digits;
			}

			csymbol[SYMBOLSZ-1] = '\0'; /* ensure null ended */
			if (rpreci == CHAR_MAX)	    /* frac_digit not defined */
				rpreci = 0;

			/* use fcvt(3C) to convert the numeric value to 
			 * string, format it with grouping and/or decimal
			 * point, and put result into valbuf.
			 */

			/* Fetch the value and do the conversion */

			/* note bp will be assigned a new location that
			 * contains the converted string after fcvt() 
			 */

			dval = va_arg(args, double);
			bp = (UCHAR *)fcvt(dval, (size_t)min(rpreci, MAXFCVT), &decpt, &nega);

			/* value printed as negative */
			prnega = nega && (decpt > -rpreci); 

			/* Initialize buffer pointer far enough in for 
			 * digits & grouping 
			 */
			headp = tailp = (decpt <= 0) ? 
					&valbuf[1] : &valbuf[decpt+decpt];
			nn = decpt;
			gp = lc->mon_grouping;
			gdigs = (!(flagword & FNOGROUP) && *gp) ? *gp : CHAR_MAX;
			ndig = 0;
			{ /* init the sign identation padding variables */
				int i;
				lpadlen = rpadlen = 0;
				for (i=0; i<PADSIZE; i++)
					padbuf[i] = ' ';
			}

			/* emit the digits before the decimal point */
			do {
				if (gdigs != CHAR_MAX && !gdigs--) {
					/* group here, copy the separator
					 * string byte by bype backwards
					 */ 
					char *sep   = lc->mon_thousands_sep;
					int  seplen = strlen(sep);
					int  p;

					for (p=seplen-1; p>=0; p--) 
						*--headp = *(sep+p);

					if (*gp && gp[1]) 
						/* get next group size */
						++gp;
					gdigs = *gp - (*gp != CHAR_MAX);
				}
	
				*--headp = (nn <= 0 || nn > MAXFSIG || 
					    bp[nn-1] == '\0') ? '0' : bp[nn-1];
				ndig++;
			} while (--nn > 0);

			if (decpt > 0) /* skip past used digits */
				bp += min(decpt, MAXFSIG);

			/* Decide whether we need a decimal point */
			if (rpreci > 0 && *lc->mon_decimal_point) {
				char *dec = lc->mon_decimal_point;
				while (*dec) 
					*tailp++ = *dec++;
			}

			/* Digits (if any) after the decimal point */
			nn = min(rpreci, MAXFCVT);

			k = 0;
			while (--nn >= 0)
				*tailp++ = (++decpt <= 0 || *bp == '\0' || 
					    k >= MAXFSIG) ? '0' : (k++, *bp++);

			/* compute the prefix and suffix strings, and store 
			 * results in the prebuf and sufbuf resp. */
			prelen = suflen = 0;
			if (flagword & FPNEG) {	/* don't use locale sign */
				/* Strangely, the blank space padding only 
				 * apply when under '#' option?! -fay 9/23/92
				 */
				if (flagword & FLPRECI) 
					prebuf[prelen++] = prnega ? '(' : ' ';
				else if (prnega)
					prebuf[prelen++] = '(';
			}

			/* put the sign, currency symbol and/or space separator
			 * in the correct sequence into the prebuf or sufbuf, 
			 * according to the various locale values. 
			 * See p_sign_posn and n_sign_posn of the localeconv(3C)
			 * man page for the meanings of the following cases.
			 */
			if (CS_PRECEDES(prnega)) {
				switch(SIGN_POSN(prnega)) {
				case 0:
					if (!(flagword & FPNEG)) 
						/* user provided '(' flag over-
					 	 * rides locale sign 
					 	 */
						prebuf[prelen++] = '(';
					put_symbol(prebuf, &prelen, flagword,
					           csymbol);
					put_sep(prebuf, &prelen, flagword,
					        prnega, lc);
					if (!(flagword & FPNEG))
						sufbuf[suflen++] = ')';
					break;
				case 1:
				case 3:
					put_sign(prebuf, &prelen, &lpadlen, 
					         &rpadlen, LEFT, flagword, 
					         prnega, lc);
					put_symbol(prebuf, &prelen, flagword,
					           csymbol);
					put_sep(prebuf, &prelen, flagword,
					        prnega, lc);
					break;
				case 2:
					put_symbol(prebuf, &prelen, flagword,
					           csymbol);
					put_sep(prebuf, &prelen, flagword,
					        prnega, lc);
					put_sign(sufbuf, &suflen, &lpadlen,
					         &rpadlen, RIGHT, flagword, 
					         prnega, lc);
					break;
				case 4:
					put_symbol(prebuf, &prelen, flagword,
					           csymbol);
					put_sep(prebuf, &prelen, flagword,
					        prnega, lc);
					put_sign(prebuf, &prelen, &lpadlen,
					         &rpadlen, LEFT, flagword, 
					         prnega, lc);
					break;
				}
			} else {
				switch(SIGN_POSN(prnega)) {
				case 0:
					if (!(flagword & FPNEG)) 
						/* user provided '(' flag over-
					 	 * rides locale sign 
					 	 */
						prebuf[prelen++] = '(';
					put_sep(sufbuf, &suflen, flagword,
					        prnega, lc);
					put_symbol(sufbuf, &suflen, flagword,
					           csymbol);
					if (!(flagword & FPNEG)) 
						sufbuf[suflen++] = ')';
					break;
				case 1:
					put_sign(prebuf, &prelen, &lpadlen,
					         &rpadlen, LEFT, flagword,
					         prnega, lc);
					put_sep(sufbuf, &suflen, flagword,
					        prnega, lc);
					put_symbol(sufbuf, &suflen, flagword,
					           csymbol);
					break;
				case 2:
				case 4:
					put_sep(sufbuf, &suflen, flagword,
					        prnega, lc);
					put_symbol(sufbuf, &suflen, flagword,
					           csymbol);
					put_sign(sufbuf, &suflen, &lpadlen,
					         &rpadlen, RIGHT, flagword, 
					         prnega, lc);
					break;
				case 3:
					put_sign(sufbuf, &suflen, &lpadlen,
					         &rpadlen, RIGHT, flagword, 
					         prnega, lc);
					put_sep(sufbuf, &suflen, flagword,
					        prnega, lc);
					put_symbol(sufbuf, &suflen, flagword,
					           csymbol);
					break;
				}
			}

			if (flagword & FPNEG) {	/* don't use locale sign */
				/* Strangely, the blank space padding only 
				 * apply when under '#' option?! -fay 9/23/92
				 */
				if (flagword & FLPRECI) 
					sufbuf[suflen++] = prnega ? ')' : ' ';
				else if (prnega)
					sufbuf[suflen++] = ')';
			}
					

			/* pad fill chars according to the left precision */
			if (lpreci > ndig) {
				if (flagword & FNOGROUP) 
					n = lpreci - ndig;
				else {
					int seplen = strlen(lc->mon_thousands_sep);
					int extralen = 0;
					int p = lpreci;
					int q = ndig;

					gp = lc->mon_grouping;
					gdigs = *gp ? *gp : CHAR_MAX;
				
					while((p -= gdigs) > 0) {
						if ((q -= gdigs) <= 0)
							extralen += seplen;
						/* get next group size? */
						if (*gp && gp[1]) 
							gdigs = *++gp;
					}
			
					n = lpreci + extralen - ndig;
				}

				for (k=0; k<n; k++)
					prebuf[prelen++] = fillchar;
			}

			n = tailp - headp; /* number of bytes in the value */

			/* handle field width formating by adding blanks, if
			 * necessary, to the buffer. Limit number of field 
			 * width padding spaces to BUFSIZE.
			 */
			if (lpadlen > PADSIZE) 
				lpadlen = PADSIZE;

			if (rpadlen > PADSIZE) 
				rpadlen = PADSIZE;

			if ((fwdlen = fwidth - (prelen + n + suflen + lpadlen +
			     rpadlen)) > BUFSIZE)
				fwdlen = BUFSIZE;

			for (k=0; k<fwdlen; k++)
				fwdbuf[k] = ' ';

			/* finally, output field width padding spaces (if 
			 * needed), prefix, value, suffix and padding spaces
			 * (if left justified).
			 */
			if (fwdlen > 0 && !(flagword & FLJUST))
				PUTBUF(fwdbuf, fwdlen);
			PUTBUF(padbuf, lpadlen);
			PUTBUF(prebuf, prelen);
			PUTBUF(headp, n);
			PUTBUF(sufbuf, suflen);
			PUTBUF(padbuf, rpadlen);
			if (fwdlen > 0 && flagword & FLJUST)
				PUTBUF(fwdbuf, fwdlen);
			break;

		default:
			/* this is technically an error; what we do is to back
			 * up the format pointer to the offending char and
			 * continue with the format scan.
			 */
			fmtptr -= CHARSIZE(fcode);
			continue;
		} /* end switch */
	} /* end for loop */
} /* end strfmon */
