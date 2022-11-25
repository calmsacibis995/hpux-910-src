/* @(#) $Revision: 66.1 $ */    
#include <varargs.h>
/*
 * This version of printf is compatible with the Version 7 C
 * printf. The differences are only minor except that this
 * printf assumes it is to print through putchar. Version 7
 * printf is more general (and is much larger) and includes
 * provisions for floating point.
 */
 

#define MAXOCT	11	/* Maximum octal digits in a long */
#define MAXINT	32767	/* largest normal length positive integer */
#define BIG	1000000000  /* largest power of 10 less than an unsigned long */
#define MAXDIGS	10	/* number of digits in BIG */

static int width, sign, fill;

char *_p_dconv();

#ifndef NONLS8 /* User messages */
# define	NL_SETN	20	/* set number */
# include	<msgbuf.h>
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

#ifdef	NLS16
#include	<nl_ctype.h>
#define	TRIM	0xFF
#define	FIRST	0x200
#define	SECOND	0x100
#define	SECONDof2(c)	(!isspace(c) && !iscntrl(c))
#endif

printf(va_alist)
	va_dcl
{
	va_list ap;
	register char *fmt;
	char fcode;
	int prec;
	int length,mask1,nbits,n;
	long int mask2, num;
	register char *bptr;
#if defined NLS16 && defined EUC
	char *obptr ;
#endif
	char *ptr;
	char buf[134];

	va_start(ap);
	fmt = va_arg(ap,char *);
	for (;;) {
		/* process format string first */
		while ((fcode = *fmt++)!='%') {
			/* ordinary (non-%) character */
			if (fcode=='\0')
				return;

#ifndef	NLS16
			putchar(fcode);
#else
			if (FIRSTof2(fcode&TRIM) && SECONDof2((*fmt)&TRIM)) {
				putchar(fcode & (TRIM | FIRST));
				putchar((*fmt++) & (TRIM | SECOND));
			} else
				putchar(fcode & TRIM);
#endif

		}
		/* length modifier: -1 for h, 1 for l, 0 for none */
		length = 0;
		/* check for a leading - sign */
		sign = 0;
		if (*fmt == '-') {
			sign++;
			fmt++;
		}
		/* a '0' may follow the - sign */
		/* this is the requested fill character */
		fill = 1;
		if (*fmt == '0') {
			fill--;
			fmt++;
		}
		
		/* Now comes a digit string which may be a '*' */
		if (*fmt == '*') {
			width = va_arg(ap, int);
			if (width < 0) {
				width = -width;
				sign = !sign;
			}
			fmt++;
		}
		else {
			width = 0;
			while (*fmt>='0' && *fmt<='9')
				width = width * 10 + (*fmt++ - '0');
		}
		
		/* maybe a decimal point followed by more digits (or '*') */
		if (*fmt=='.') {
			if (*++fmt == '*') {
				prec = va_arg(ap, int);
				fmt++;
			}
			else {
				prec = 0;
				while (*fmt>='0' && *fmt<='9')
					prec = prec * 10 + (*fmt++ - '0');
			}
		}
		else
			prec = -1;
		
		/*
		 * At this point, "sign" is nonzero if there was
		 * a sign, "fill" is 0 if there was a leading
		 * zero and 1 otherwise, "width" and "prec"
		 * contain numbers corresponding to the digit
		 * strings before and after the decimal point,
		 * respectively, and "fmt" addresses the next
		 * character after the whole mess. If there was
		 * no decimal point, "prec" will be -1.
		 */
		switch (*fmt) {
			case 'L':
			case 'l':
				length = 2;
				/* no break!! */
			case 'h':
			case 'H':
				length--;
				fmt++;
				break;
		}
		
		/*
		 * At exit from the following switch, we will
		 * emit the characters starting at "bptr" and
		 * ending at "ptr"-1, unless fcode is '\0'.
		 */
		switch (fcode = *fmt++) {
			/* process characters and strings first */
			case 'c':
				buf[0] = va_arg(ap, int);
				ptr = bptr = &buf[0];
				if (buf[0] != '\0')
					ptr++;
				break;
			case 's':
				/* get the top of the string		*/
				bptr = va_arg(ap,char *);
				if (bptr==0)
					bptr = (nl_msg(1, "(null pointer)"));
				/* prec is the allowed column width for	*/
				/* the string 				*/
				if (prec < 0)
					prec = MAXINT;
				/* put the allowed length of the string	*/
				/* pointed by ptr			*/
#if defined NLS16 && defined EUC
				obptr = bptr ;
				for (n=0; *bptr++ && n < prec;) {
					if( FIRSTof2( ( (int)(bptr[-1]) ) & TRIM ) && ( SECof2( ( (int)(bptr[0]) ) & TRIM ) ) ) {
						n += C_COLWIDTH( ( (int)(bptr[-1]) ) & TRIM ) ;
						bptr++ ;
					} else {
						n++ ;
					}
				}
				ptr = --bptr;
				if ( FIRSTof2( bptr[-1] & TRIM ) && SECof2( bptr[0] & TRIM ) ) {
					--ptr ;
				}
				/* get the original position of bptr	*/
				bptr = obptr ;
#else
				for (n=0; *bptr++ && n < prec; n++) ;
				ptr = --bptr;
				bptr -= n ;
#endif
				break;
			case 'O':
				length = 1;
				fcode = 'o';
				/* no break */
			case 'o':
			case 'X':
			case 'x':
				if (length > 0)
					num = va_arg(ap,long);
				else
					num = (unsigned)va_arg(ap,int);
				if (fcode=='o') {
					mask1 = 0x7;
					mask2 = 0x1fffffffL;
					nbits = 3;
				}
				else {
					mask1 = 0xf;
					mask2 = 0x0fffffffL;
					nbits = 4;
				}
				n = (num!=0);
				bptr = buf + MAXOCT + 3;
				/* shift and mask for speed */
				do
				    if (((int) num & mask1) < 10)
					*--bptr = ((int) num & mask1) + 060;
				    else
					*--bptr = ((int) num & mask1) + 0127;
				while (num = (num >> nbits) & mask2);
				
				if (fcode=='o') {
					if (n)
						*--bptr = '0';
				}
				else
					if (!sign && fill <= 0) {
						putchar('0');
						putchar(fcode);
						width -= 2;
					}
					else {
						*--bptr = fcode;
						*--bptr = '0';
					}
				ptr = buf + MAXOCT + 3;
				break;
			case 'D':
			case 'U':
			case 'I':
				length = 1;
				fcode = fcode + 'a' - 'A';
				/* no break */
			case 'd':
			case 'i':
			case 'u':
				if (length > 0)
					num = va_arg(ap,long);
				else {
					n = va_arg(ap,int);
					if (fcode=='u')
						num = (unsigned) n;
					else
						num = (long) n;
				}
				if (n = (fcode != 'u' && num < 0))
					num = -num;
				/* now convert to digits */
				bptr = _p_dconv(num, buf);
				if (n)
					*--bptr = '-';
				if (fill == 0)
					fill = -1;
				ptr = buf + MAXDIGS + 1;
				break;
			default:
				/* not a control character, 
				 * print it.
				 */
				ptr = bptr = &fcode;
				ptr++;
				break;
			}
			if (fcode != '\0')
				_p_emit(bptr,ptr);
	}
	va_end(ap);
}

/* _p_dconv converts the unsigned long integer "value" to
 * printable decimal and places it in "buffer", right-justified.
 * The value returned is the address of the first non-zero character,
 * or the address of the last character if all are zero.
 * The result is NOT null terminated, and is MAXDIGS characters long,
 * starting at buffer[1] (to allow for insertion of a sign).
 *
 * This program assumes it is running on 2's complement machine
 * with reasonable overflow treatment.
 */
char *
_p_dconv(value, buffer)
	long value;
	char *buffer;
{
	register char *bp;
	register int svalue;
	int n;
	long lval;
	
	bp = buffer;
	
	/* zero is a special case */
	if (value == 0) {
		bp += MAXDIGS;
		*bp = '0';
		return(bp);
	}
	
	/* develop the leading digit of the value in "n" */
	n = 0;
	while (value < 0) {
		value -= BIG;	/* will eventually underflow */
		n++;
	}
	while ((lval = value - BIG) >= 0) {
		value = lval;
		n++;
	}
	
	/* stash it in buffer[1] to allow for a sign */
	bp[1] = n + '0';
	/*
	 * Now develop the rest of the digits. Since speed counts here,
	 * we do it in two loops. The first gets "value" down until it
	 * is no larger than MAXINT. The second one uses integer divides
	 * rather than long divides to speed it up.
	 */
	bp += MAXDIGS + 1;
	while (value > MAXINT) {
		*--bp = (int)(value % 10) + '0';
		value /= 10;
	}
	
	/* cannot lose precision */
	svalue = value;
	while (svalue > 0) {
		*--bp = (svalue % 10) + '0';
		svalue /= 10;
	}
	
	/* fill in intermediate zeroes if needed */
	if (buffer[1] != '0') {
		while (bp > buffer + 2)
			*--bp = '0';
		--bp;
	}
	return(bp);
}

/*
 * This program sends string "s" to putchar. The character after
 * the end of "s" is given by "send". This allows the size of the
 * field to be computed; it is stored in "alen". "width" contains the
 * user specified length. If width<alen, the width will be taken to
 * be alen. "sign" is zero if the string is to be right-justified
 * in the field, nonzero if it is to be left-justified. "fill" is
 * 0 if the string is to be padded with '0', positive if it is to be
 * padded with ' ', and negative if an initial '-' should appear before
 * any padding in right-justification (to avoid printing "-3" as
 * "000-3" where "-0003" was intended).
 */
_p_emit(s, send)
	register char *s;
	char *send;
{
	char cfill;
	register int alen;
	int npad;
	
#if defined NLS16 && defined EUC
	alen = c_strlen( s ) - c_strlen( send ) ;
#else
	alen = send - s;
#endif
	if (alen > width)
		width = alen;
	cfill = fill>0? ' ': '0';
	
	/* we may want to print a leading '-' before anything */
	if (*s == '-' && fill < 0) {
		putchar(*s++);
		alen--;
		width--;
	}
	npad = width - alen;
	
	/* emit any leading pad characters */
	if (!sign)
		while (--npad >= 0)
			putchar(cfill);
			
	/* emit the string itself */
	while (--alen >= 0)

#ifndef	NLS16
		putchar(*s++);
#else
		if (FIRSTof2( ( (int)(s[0]) ) & TRIM) && SECof2( ( (int)(s[1]) ) & TRIM))	{
#ifdef EUC
			alen -= ( C_COLWIDTH( ( (int)(s[0]) ) & TRIM ) - 1 ) ;
			putchar(*s++ & (TRIM | FIRST));
			putchar(*s++ & (TRIM | SECOND));
#else EUC
			putchar(*s++ & (TRIM | FIRST));
			putchar(*s++ & (TRIM | SECOND));
			alen--;
#endif EUC
		} else
			putchar(*s++ & TRIM);
#endif

	/* emit trailing pad characters */
	if (sign)
		while (--npad >= 0)
			putchar(cfill);
}

