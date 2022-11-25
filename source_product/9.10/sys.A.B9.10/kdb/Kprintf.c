/* @(#) $Revision: 10.3 $ */      

#define	NULL	0
#define	MAXDIGS	11
#define	MAXINT	2147483647
#define	HIBIT	0x80000000L
#define	tonumber(x)	((x)-'0')

typedef char *va_list;
#define va_dcl int va_alist;
#define va_start(list) list = (char *) &va_alist
#define va_end(list)
#define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]

#include <ctype.h>

#define max(a,b) ((a) > (b)? (a): (b))
#define min(a,b) ((a) < (b)? (a): (b))
static char *string;
printf (va_alist) va_dcl
{
	register char *format;
	va_list ap;

	va_start(ap);
	format = va_arg(ap, char *);
	string = 0;
	_print(format, &ap);
}

fprintf (va_alist) va_dcl
{
	register char *format;
	register int rc;
	va_list ap;

	va_start (ap);
	va_arg (ap, char *);
	format = va_arg (ap, char *);
	string = 0;
	_print (format, &ap, 0);
}

sprintf (va_alist) va_dcl
{
	register char *format;
	register int rc;
	va_list ap;

	va_start (ap);
	string = va_arg (ap, char *);
	format = va_arg (ap, char *);
	_print (format, &ap, string);
	va_end (ap);
	*string = '\0';
}

_print(format, args, string)
char *format;
va_list *args;
char *string;
{
	/* Current position in format */
	char *cp;

	/* Starting and ending points for value to be printed */
	char *bp, *p;

	/* Field width and precision */
	int width, prec;

	/* Format code */
	char fcode;

	/* Number of padding zeroes required on the left */
	int lzero;

	/* Flags - nonzero if corresponding character appears in format */
	char length;		/* l */
	char fplus;		/* + */
	char fminus;		/* - */
	char fblank;		/* blank */
	char fsharp;		/* # */
	char fzero;		/* 0 */

	/* Pointer to sign, "0x", "0X", or empty */
	char *prefix;


	/* Number of padding zeroes required on the right */
	int rzero;

	/* Scratch */
	int k;

	/* Values are developed in this buffer */
	char buf[MAXDIGS];
	/* The value being converted, if integer */
	long val;

	/* Set to point to a translate table for digits of whatever radix */
	char *tab;

	/* Work variables */
	int n, hradix, lowbit;

	cp = format;

	/*
	 *	The main loop -- this loop goes through one iteration
	 *	for each ordinary character or format specification.
	 */
	while (*cp)
		if (*cp != '%') {
			/* Ordinary (non-%) character */
			emitchar(*cp++);
		} else {
			/*
			 *	% has been found.
			 *	First, parse the format specification.
			 */

			/* Scan the <flags> */
			fplus = fminus = fblank = fsharp = 0;
			fzero = 0;
		scan:	switch (*++cp) {
			case '+':
				fplus = 1;
				goto scan;
			case '-':
				fminus = 1;
				goto scan;
			case ' ':
				fblank = 1;
				goto scan;
			case '#':
				fsharp = 1;
				goto scan;
			case '0':
				fzero = 1;
				goto scan;
			}

			/* Scan the field width */
			if (*cp == '*') {
				width = va_arg (*args, int);
				if (width < 0) {
					width = -width;
					fminus = 1;
				}
				cp++;
			} else {
				width = 0;
				while ((*cp >= '0') && (*cp <= '9')) {
					n = tonumber (*cp++);
					width = width * 10 + n;
				}
			}

			/* Scan the precision */
			if (*cp == '.') {

				/* '*' instead of digits? */
				if (*++cp == '*') {
					prec = va_arg (*args, int);
					cp++;
				} else {
					prec = 0;
					while ((*cp >= '0') && (*cp <= '9')) {
						n = tonumber (*cp++);
						prec = prec * 10 + n;
					}
				}
			} else
				prec = -1;

			/* Scan the length modifier */
			length = 0;
			switch (*cp) {
			/* Remove this since disassembler from adb has its
			   own printf routine which uses 'h' for hex
			case 'h':
				length = 1;
				/* No break */
			case 'l':
				cp++;
			}

			/*
			 *	The character addressed by cp must be the
			 *	format letter -- there is nothing left for
			 *	it to be.
			 *
			 *	The status of the +, -, #, blank, and 0
			 *	flags are reflected in the variables
			 *	"fplus", "fminus", "fsharp", "fblank",
			 *	and "fzero", respectively.
			 *	"width" and "prec" contain numbers
			 *	corresponding to the digit strings
			 *	before and after the decimal point,
			 *	respectively. If there was no decimal
			 *	point, "prec" is -1.
			 *
			 *	The following switch sets things up
			 *	for printing.  What ultimately gets
			 *	printed will be padding blanks, a prefix,
			 *	left padding zeroes, a value, right padding
			 *	zeroes, and more padding
			 *	blanks.  Padding blanks will not appear
			 *	simultaneously on both the left and the
			 *	right.  Each case in this switch will
			 *	compute the value, and leave in several
			 *	variables the information necessary to
			 *	construct what is to be printed.
			 *
			 *	The prefix is a sign, a blank, "0x", "0X",
			 *	or null, and is addressed by "prefix".
			 *
			 *	The value to be printed starts at "bp"
			 *	and continues up to and not including "p".
			 *
			 *	"lzero" and "rzero" will contain the number
			 *	of padding zeroes required on the left
			 *	and right, respectively.  If either of
			 *	these variables is negative, it will be
			 *	treated as if it were zero.
			 *
			 *	The number of padding blanks, and whether
			 *	they go on the left or the right, will be
			 *	computed on exit from the switch.
			 */
			
			lzero = 0;
			prefix = NULL;
			rzero = lzero;
			switch (fcode = *cp++) {

			/*
			 *	fixed point representations
			 *
			 *	"hradix" is half the radix for the conversion.
			 *	Conversion is unsigned unless fcode is 'd'.
			 *	HIBIT is 1000...000 binary, and is equal to
			 *		the maximum negative number.
			 *	We assume a 2's complement machine
			 */

			case 'D':
				fcode = 'd';
			case 'd':
			case 'u':
				hradix = 5;
				goto fixed;

			case 'o':
				hradix = 4;
				goto fixed;

			case 'h':
			case 'X':
			case 'x':
				hradix = 8;

			fixed:
				/* Establish default precision */
				if (prec < 0)
					prec = 1;

				/* Fetch the argument to be printed */
				if (length && (fcode != 'd'))
					val = va_arg (*args, int) & 0x0000ffff;
				else if (fcode == 'd')
					val = va_arg (*args, int);
				else
					val = va_arg (*args, unsigned);

				/* If signed conversion, establish sign */
				if (fcode == 'd') {
					if (val < 0) {
						prefix = "-";
						/*
						 *	Negate, checking in
						 *	advance for possible
						 *	overflow.
						 */
						if (val != HIBIT)
							val = -val;
					} else if (fplus)
						prefix = "+";
					else if (fblank)
						prefix = " ";
				}
				if (fcode == 'h') {
					if (val < 0) {
						prefix = "-0x";
						/*
						 *	Negate, checking in
						 *	advance for possible
						 *	overflow.
						 */
						if (val != HIBIT)
							val = -val;
					} else if (fplus)
						prefix = "+0x";
					else if (fblank)
						prefix = " 0x";
					else prefix = "0x";
				}
				if (fzero) {
					int n = width - strlen (prefix);
					if (n > prec)
						prec = n;
				}
				/* Set translate table for digits */
				if (fcode == 'X')
					tab = "0123456789ABCDEF";
				else
					tab = "0123456789abcdef";

				/* Develop the digits of the value */
				p = bp = buf + MAXDIGS;
				while (val) {
					lowbit = val & 1;
					val = (val >> 1) & ~HIBIT;
					*--bp = tab[val % hradix * 2 + lowbit];
					val /= hradix;
				}

				/* Calculate padding zero requirement */
				lzero = bp - p + prec;

				/* Handle the # flag */
				if (fsharp && bp != p)
					switch (fcode) {
					case 'o':
						if (lzero < 1)
							lzero = 1;
						break;
					case 'x':
						prefix = "0x";
						break;
					case 'X':
						prefix = "0X";
						break;
					}

				break;

			case 'c':
				buf[0] = va_arg (*args, int);
				bp = &buf[0];
				p = bp + 1;
				break;

			case 's':
				bp = va_arg (*args, char *);
				if (bp == NULL) break;
				if (prec < 0)
					prec = MAXINT;
				for (n=0; *bp++ != '\0' && n < prec; n++);
				p = --bp;
				bp -= n;
				break;

			case '\0':
				cp--;
				break;

		/*	case '%':	*/
			default:
				p = bp = &fcode;
				p++;
				break;

			}
			if (fcode != '\0') {
				/* Calculate number of padding blanks */
				int nblank;
				nblank = width
					- (rzero < 0? 0: rzero)
					- (bp == 0? 0: p - bp)
					- (lzero < 0? 0: lzero)
					- strlen (prefix);

				/* Blanks on left if required */
				if (!fminus)
					while (--nblank >= 0)
						emitchar (' ');

				/* Prefix, if any */
				if (prefix != NULL)
				while (*prefix != '\0')
					emitchar (*prefix++);

				/* Zeroes on the left */
				while (--lzero >= 0)
					emitchar ('0');
				
				/* The value itself */
				if (bp != NULL) {
					while (bp < p)
						emitchar (*bp++);
					}
				/* Zeroes on the right */
				while (--rzero >= 0)
					emitchar ('0');

				/* Blanks on the right if required */
				if (fminus)
					while (--nblank >= 0)
						emitchar (' ');
			}
		}

}

/* Send a character to the output */
static
emitchar (c)
	char c;
{
	if (string) *string++ = c;
	else kdbputchar (c);
}
