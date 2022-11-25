/* static char *HPUX_ID = "@(#) wcstod.c $Revision: 70.5 $"; */

/*LINTLIBRARY*/


/*  NOTE: This code is based upon the strtod() code revision 70.1 found
 *  in /hpux/shared/supp/usr/src/lib/libc/gen/atof.c.  It has been
 *  reworked somewhat, but shares the same structure and algorithms.
 */

#include <wchar.h>
#include <ctype.h>
#include <nl_ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <wpi.h>

extern int _nl_radix;	/* localized radix character (decimal point), */
			/* equal to '.' by default.                   */

/*****************************************************************************
 *                                                                           *
 *  CONVERT WIDE STRING INTO DOUBLE VALUE                                    *
 *                                                                           *
 *  INPUTS:      wstr          - the string to convert                       *
 *                                                                           *
 *  OUTPUTS:     the double value                                            *
 *               HUGE_VAL/-HUGE_VAL on error                                 *
 *                                                                           *
 *               endptr        - pointer set to first char not in value      *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

static unsigned long table1[27][2] = {  /* mantissas for 10^(n*25) */
	{ 0xA5CED43B,0x7E3E9188	} ,	/* 10^-325 */
	{ 0xAB70FE17,0xC79AC6CB	} ,	/* 10^-300 */
	{ 0xB1442798,0xF49FFB4B	} ,	/* 10^-275 */
	{ 0xB749FAED,0x14125D37	} ,	/* 10^-250 */
	{ 0xBD8430BD,0x8277232	} ,	/* 10^-225 */
	{ 0xC3F490AA,0x77BD60FD	} ,	/* 10^-200 */
	{ 0xCA9CF1D2,0x6FDC03C	} ,	/* 10^-175 */
	{ 0xD17F3B51,0xFCA3A7A1	} ,	/* 10^-150 */
	{ 0xD89D64D5,0x7A607745	} ,	/* 10^-125 */
	{ 0xDFF97724,0x70297EBE	} ,	/* 10^-100 */
	{ 0xE7958CB8,0x7392C2C3	} ,	/* 10^-75 */
	{ 0xEF73D256,0xA5C0F77D	} ,	/* 10^-50 */
	{ 0xF79687AE,0xD3EEC551	} ,	/* 10^-25 */
	{ 0x0,0x0 } ,			/* 10^0 */
	{ 0x84595161,0x401484A0	} ,	/* 10^25 */
	{ 0x88D8762B,0xF324CD10	} ,	/* 10^50 */
	{ 0x8D7EB760,0x70A08AED	} ,	/* 10^75 */
	{ 0x924D692C,0xA61BE759	} ,	/* 10^100 */
	{ 0x9745EB4D,0x50CE6333	} ,	/* 10^125 */
	{ 0x9C69A972,0x84B578D8	} ,	/* 10^150 */
	{ 0xA1BA1BA7,0x9E1632DD	} ,	/* 10^170 */
	{ 0xA738C6BE,0xBB12D16D	} ,	/* 10^200 */
	{ 0xACE73CBF,0xDC0BFB7C	} ,	/* 10^225 */
	{ 0xB2C71D5B,0xCA9023F9	} ,	/* 10^250 */
	{ 0xB8DA1662,0xE7B00A17	} ,	/* 10^275 */
	{ 0xBF21E440,0x3ACDD2D	} ,	/* 10^300 */
};

static short table1e[27] = {	/* exponents for 10^(n*25) */
	-1079 , -996, -913, -830, -747, -664, -581, -498, -415, -332, -249, -166, -83,
	0, 84, 167, 250, 333, 416, 499, 582, 665, 748, 831, 914, 997, 1080};

static unsigned long table2[24][2] = {	/* mantissas for 10^n */
	{ 0xA0000000,0x0	} ,	/* 10^1 */
	{ 0xC8000000,0x0	} ,	/* 10^2 */
	{ 0xFA000000,0x0	} ,	/* 10^3 */
	{ 0x9C400000,0x0	} ,	/* 10^4 */
	{ 0xC3500000,0x0	} ,	/* 10^5 */
	{ 0xF4240000,0x0	} ,	/* 10^6 */
	{ 0x98968000,0x0	} ,	/* 10^7 */
	{ 0xBEBC2000,0x0	} ,	/* 10^8 */
	{ 0xEE6B2800,0x0	} ,	/* 10^9 */
	{ 0x9502F900,0x0	} ,	/* 10^10 */
	{ 0xBA43B740,0x0	} ,	/* 10^11 */
	{ 0xE8D4A510,0x0	} ,	/* 10^12 */
	{ 0x9184E72A,0x0	} ,	/* 10^13 */
	{ 0xB5E620F4,0x80000000	} ,	/* 10^14 */
	{ 0xE35FA931,0xA0000000	} ,	/* 10^15 */
	{ 0x8E1BC9BF,0x4000000	} ,	/* 10^16 */
	{ 0xB1A2BC2E,0xC5000000	} ,	/* 10^17 */
	{ 0xDE0B6B3A,0x76400000	} ,	/* 10^18 */
	{ 0x8AC72304,0x89E80000	} ,	/* 10^19 */
	{ 0xAD78EBC5,0xAC620000	} ,	/* 10^20 */
	{ 0xD8D726B7,0x177A8000	} ,	/* 10^21 */
	{ 0x87867832,0x6EAC9000	} ,	/* 10^22 */
	{ 0xA968163F,0xA57B400	} ,	/* 10^23 */
	{ 0xD3C21BCE,0xCCEDA100	} ,	/* 10^24 */
};

static short table2e[24] = {		/* exponents for 10^n */
	4, 7, 10, 14, 17, 20, 24, 27, 30, 34, 37, 40, 44, 47, 50, 54, 57, 60, 64, 67,
	70, 74, 77, 80};

static char shiftcount[19] = {
	0, 28, 25, 22, 18, 15, 12, 8, 5, 2, 0, 27, 24, 20, 17, 14, 10, 7, 7};


#ifdef _NAMESPACE_CLEAN
#    undef  wcstod
#    pragma _HP_SECONDARY_DEF _wcstod wcstod
#    define wcstod _wcstod
#endif


#ifndef __hp9000s300
	static void mult32();	/* Forward declaration (located here */
				/* to placate non-s300 c89).         */
#endif


double wcstod(const wchar_t *wstr, wchar_t **endptr)
{
	unsigned long n1, n0;
	char sign = 0;
	short exp = 0;
	char dfound = 0;
	char shf;
#ifdef __hp9000s300
	void mult32();		/* Forward declaration (located here */
				/* to placate s300 c89).	     */
#endif
	union {
		unsigned short s;
		unsigned long l[2];
		double d;
	} 
	res;
	unsigned long m32[4];

	{
		wchar_t *p = (wchar_t *)wstr;
		wchar_t wc;
		int	dig;	/* integer representation of a numeric char. */

		/* Check for leading "space" (blanks, vertical and horizontal
		   tabs, formfeeds, carriage returns, and linefeeds). */

		while (iswspace(*p))
			p++;
		wc = *p++;	/* Start with first non-space character */
		
		/* Look for mantissa sign, either a "+" or "-". */

		if (wc == L'+') wc = *p++;
		else if (wc == L'-') {
			sign++;
			wc = *p++;
		}; 

		/* Ignore leading zeroes. However, if there are leading zeroes, set dfound to 
		indicate that a digit has been found; otherwise, values like "0.E4" would not be 
		handled correctly. */

		if (wc == L'0') {
			dfound++;
			do { wc = *p++; } while (wc == L'0');
		};

		{
			char digits = 0;
			char decpt = 0;
			char rounded = 0;
			n1 = n0 = 0;
			for(;;wc = *p++) {  /* scan forever */
				if (wc >= L'0' && wc <= L'9') {  /* digit? */

					/* The flag "rounded" is set when enough digits have been found 
					   to provide 17 digits that are properly rounded. All that 
					   needs to be done now is search for a non-digit. */

					if (rounded) {
						/* If the decimal point hasn't been found, increment 
						   exponent */
						if (!decpt) exp++;
						continue; /* continue scan */
					};

					dig = wc - L'0'; /* convert to digit */

					/* If more than 17 digits have been found, all that's left to do 
					   is properly round it */

					if (digits >= 17) {
						if (!decpt) exp++;
						if (digits == 17) {

							/* If the round digit is 5, always round up. This 
							   will be correct except if it was exactly .5 and 
							   we rounded up to an odd number */

							if (dig >= 5) if (!++n0) ++n1;
							/* For a 5, we are okay if the result is not odd */

							if (dig != 5 || !(n0 & 1)) rounded++;
							/* otherwise, go back to even, and we'll have to 
							   continue looking for sticky digits */
							else n0--;
							/* Bumping "digits" to 18 indicates we've 
							   rounded */

							digits++;
						}
						/* Looking for sticky digit */
						else if (dig) {
							/* If found, go back to odd */
							n0++;
							/* and integer is correctly rounded */
							rounded++;
						};
					}
					/* Less than 17 digits found */
					else {
						digits++; /* got one more */
						/* Adjust exponent, if decimal point has been found */
						if (decpt) exp--;
						/* Integer is collected in n1 and n0 (n1 is most 
						   significant) */
						if (digits <= 9) {
							/* Digits in only n0 so far: multiply by 10 and 
							add new digit */
#ifdef __hp9000s300
							n0 += n0;
							n0 += n0 << 2;
							n0 += dig;
#else /* __hp9000s300 */
							n0 = 10 * n0 + dig;
#endif /* __hp9000s300 */
						} 
						else {
							/* Both n1 and n0 are involved; multiply by 10 
							   and add new digit; (have to be trickier 
							   here) */
							{
								unsigned long s0;
								n1 += n1;
								if ((long) n0 < 0) n1++;
								n1 += n1 << 2;
								if ((long) (s0 = n0 += n0) < 0) n1 += 2;
								if ((long) (s0 += s0) < 0) n1++;
								if ((n0 += s0 += s0) < s0) n1++;
								if ((n0 += s0 = dig) < s0) n1++;
							};
						};
					}; 
				}
				else { /* it's not a digit */
					if (wc == (wchar_t)_nl_radix) { /* radix character ? */
						/* if we've already got one, terminate scan */
						if (decpt++) break; 
						/* If we have no digits yet, ignore zeroes after decimal
						   point */
						if (!digits) if (*p == L'0') {
							dfound++; /* we've got digit of some sort */
							do {
								p++; /* advance pointer */
								exp--; /* decrement exponent */
							}
							while (*p == L'0'); /* until non-zero */
						};
					} 
					else break; /* terminating character */
				};
			};
			/* if we didn't find anything, this is not a number */
			if (!(digits || dfound)) goto nonumber; 
			/* Based on the number of digits that were found, we know the minimum number of
			   shifts that must be done to normalize (n1,n0) so that the MSB is the MSB of
			   n1. Save that shift amount in "shf" for later use */
			shf = shiftcount[digits];
		};
		if (wc == L'e' || wc == L'E') { /* Is next character a "E" or "e" for an exponent? */
			{
				char digits = 0;
				short exponent = 0;
				char expsign = 0;
				/* this may not be an exponent, so remember where I left off */
				wchar_t *eptr = p; 
				dfound = 0;
				/* Check for exponent sign and save in "expsign" */
				if ((wc = *p++) == L'+' || wc == L' ') wc = *p++;
				else if (wc == L'-') {
					expsign++;
					wc = *p++;
				};
				/* Ignore leading zeroes on the exponent */
				if (wc == L'0') {
					dfound++; /* An exponent digit has been found */
					do { wc = *p++; } while (wc == L'0');
				};
				while (wc >= L'0' && wc <= L'9') { /* Look for digits */
					dig = wc - L'0'; /* Convert to digit */
					/* If more than four digits in the exponent indicates overflow or
					   underflow. (Cases can be contrived where this is not so, but
					   these will never come up in normal use */
					if (++digits > 4) {
						/* Look for terminating character */
						while ((wc = *p) >= L'0' && wc <= L'9') p++;
						/* If termination character is desired, tell what it is */
						if (endptr) *endptr = p; 
						/* If mantissa was 0, answer is 0 */
						if (! (n1 | n0)) return ( (sign ? -1 : 1) * 0.0 );
						/* Taken exception route */
						if (expsign) goto underflow;
						else goto overflow;
					};
					/* Multiply exponent by 10 and add new digit */
#ifdef __hp9000s300
					exponent += exponent;
					exponent += exponent << 2;
					exponent += dig;
#else /* __hp9000s300 */
					exponent = 10 * exponent + dig;
#endif /* __hp9000s300 */
					wc = *p++; /* get next character */
				};
				/* If no digits in exponent, it really wasn't an exponent, an termination
				   in on the "E" or "e" */
				if (!(digits || dfound))  p = eptr;
				else {
					if (expsign) exponent = -exponent; /* correct sign */
					exp += exponent; /* "exp" is adjusted exponent */
				};
			};
		};
		if (endptr) *endptr = --p; /* If termination pointer is desired, indicate what it is */
	};
	/* (n1,n0) must now be normalized so the MSB is the MSB of n1 */
	{
		short newexp;
		char shft;
		if (!n1) { /* shift at least 32 if nothing in n1 */
			if (!(n1 = n0)) return( (sign ? -1 : 1)*0.0 ); /* if nothing in either, answer is 0 */
			newexp = 32 + (shft = shf); /* minimum amount to shift */
			n1 <<= shft; /* shift it */
			while ((long) n1 >= 0) { /* now make final adjustment */
				n1 += n1;
				newexp++;
			};
			n0 = 0;
		}
		else {
			if (!(shft = shf)) shft = 30; /* For 10 digits, shift at least 30 */
			newexp = shft;
			n1 <<= shft; /* shift it */
			while ((long) n1 >= 0) { /* now make final adjustment */
				n1 += n1;
				newexp++;
			};
			n1 |= n0 >> (32 - newexp); /* get shift out from n0 */
			n0 <<= newexp; /* shift n0 the same amount */
		};
		/* m32[] is used for the 64 x 64 bit multiply; initialize it */
		m32[0] = n1;
		m32[1] = n0;
		m32[2] = 0;
		m32[3] = 0;
		newexp = 1085 - newexp; /* "magic" adjustment to the exponent */
		{
			short pow, pow25;
			/* Check for overflow exponent */
			pow25 = pow = exp;
			if (pow25 < -325) goto underflow;
			if (pow25 > 308) goto overflow;
			/* The exponent represents 10^(pow25+pow) where "pow25" is a multiple of 25 and
			   "pow" is a positive integer */
			pow25 /= 25;
			if ((pow %= 25) < 0) { /* Is "pow" positive? */
				pow += 25; /* If not, make proper adjustments */
				pow25--;
			};
			if (pow25) { /* If we are to multiply by 10^pow25... */
				pow25 += 13; /* Change "pow25" to an array index */
				newexp += table1e[pow25]; /* Adjust the exponent */
				m32[2] = table1[pow25][0]; /* Get mantissa of 10^pow25 */
				m32[3] = table1[pow25][1];
				mult32(m32); /* Multiply */
				if ((long) m32[2] < 0) if (!++m32[1]) ++m32[0]; /* Round properly */
			};
			if (pow) { /* If we are to multiply by 10^pow... */
				pow--; /* Change "pow" to an array index */
				newexp += table2e[pow]; /* Adjust the exponent */
				m32[2] = table2[pow][0]; /* Get mantissa of 10^pow */
				m32[3] = table2[pow][1];
				mult32(m32); /* Multiply */
			};
		};
		/* Now the result must be rounded to give exactly 53 bits. The most significant bit now 
		   set in m32[0] is either bit 31, 30 or 29 */
		{
			char shift = 11; /* assume MSB is 31 */
			long round = 0x400; /* assume MSB is 31 */
			long lsb;
			n1 = m32[0]; /* move mantissa */
			n0 = m32[1];
			if ((long) n1 >= 0) { /* Is MSB bit 31? */
				newexp--; /* Decrement exponent */
				shift--; /* Shift will be one less than expected */
				round >>= 1; /* The round bit has moved */
				if ((long) (n1 + n1) >= 0) { /* Is MSB bit 30? */
					newexp--; /* Decrement exponent again */
					shift--; /* Still less to shift */
					round >>= 1; /* Move round bit */
				};
			};
			if (n0 & round) { /* Is round bit set? */
				n0 ^= round; /* Clear round bit */
				lsb = round; /* "lsb" will be LSB of mantissa */
				lsb += lsb;
				round = n0 & ~-round; /* Get remaining sticky bits in n0 */
				n0 ^= round; /* Clear sticky bits in n0 */
				if (!(n0 += lsb)) ++n1; /* Round up */
				if (!round && !m32[2] && !m32[3]) n0 &= ~lsb; /* Clear round if no sticky */
			};
			n0 >>= shift;  /* Shift mantissa into place */
			n0 |= n1 << (32 - shift);
			n1 >>= shift;
		};
		/* Now we must check to see if the exponent is neither too big or too small. In the
		   course of rounding the mantissa, the effective exponent could have increased so that
		   must be considered */
		{
			short expfield;
			res.l[0] = n1; /* save mantissa */
			res.l[1] = n0;
			/* Get the bits that overflowed into the exponent field (must be 1 or 2). */
			expfield = res.s >> 4; 
			if (newexp < 0) { /* check for underflow and overflow */
				if ((newexp + expfield) <= 0) goto underflow;
			}
			else if ((newexp + expfield) > 0x7FE) goto overflow;
		};
		res.s += newexp << 4; /* drop in new exponent; hidden bit all accounted for */
		if (sign) res.s |= 0x8000; /* change sign if necessary */
		return(res.d); /* return result */
	};
nonumber: /* ASCII pattern is not a number: return pointer to start of string and return EINVAL and 0.0 */
	if (endptr) *endptr = (wchar_t *)wstr;
	errno = EINVAL;
	return(0.0);
overflow: /* Overflow: set errno to ERANGE and return HUGE_VAL with correct sign */
	errno = ERANGE;
	if (sign) return (-HUGE_VAL); 
	else return(HUGE_VAL);
underflow: /* Underflow: set errno to ERANGE and return 0 */
	errno = ERANGE;
	return((sign ? -1 : 1)*0.0);
}

/* "mult32" multiplies two 64 bit unsigned integers and produces a 128 unsigned 
result. The arguments are passed in (arg[0],arg[1]) and (arg[2],arg[3]) and the
result is returned in (arg[0],arg[1],arg[2],arg[3]). */

#ifdef  __hp9000s300
#pragma OPT_LEVEL 1
	asm("	text");
	asm("_mult32:");
	asm("	mov.l	4(%sp),%a0");
	asm("	mov.l	%d2,-(%sp)");
	asm("	mov.l	12(%a0),%d0");
	asm("	mov.l	%d3,-(%sp)");
	asm("	mov.l	%d0,%d2");
	asm("	mulu.l	4(%a0),%d1:%d0");
	asm("	mov.l	%d0,12(%a0)");
	asm("	mulu.l	(%a0),%d0:%d2");
	asm("	add.l	%d1,%d2");
	asm("	clr.l	%d3");	
	asm("	addx.l	%d3,%d0");
	asm("	mov.l	8(%a0),%d1");
	asm("	mov.l	%d2,8(%a0)");
	asm("	mov.l	%d1,%d2");
	asm("	mulu.l	4(%a0),%d3:%d1");
	asm("	add.l	%d1,8(%a0)");
	asm("	addx.l	%d3,%d0");
	asm("	clr.l	%d3");
	asm("	mulu.l	(%a0),%d1:%d2");
	asm("	addx.l	%d3,%d1");		/* This takes X bit from previous addx */
	asm("	add.l	%d0,%d2");
	asm("	addx.l	%d3,%d1");
	asm("	mov.l	%d2,4(%a0)");
	asm("	mov.l	(%sp)+,%d3");
	asm("	mov.l	%d1,(%a0)");
	asm("	mov.l	(%sp)+,%d2");
	asm("	rts");
#pragma	OPT_LEVEL 2

#else
static void mult32(unsigned long arg[])
{
	unsigned long r0,r1,r2,r3,x1,x0,y1,y0;
	union store {
		unsigned long l;
		unsigned short s[2];
	} 
	y0save,y1save,x0save,x1save;
	union res {
		unsigned long a[4];
		unsigned short b[8];
	} 
	result;

	x1save.l = arg[0]; 	/* The arguments are removed from the array */
	x0save.l = arg[1]; 	/* and placed in x1&x0 and y1&y0. Save temps */
	y1save.l = arg[2];      /* provide the capability of accessing any */
	y0save.l = arg[3];	/* set of 16 bits. */

	/* Given that the two numbers to be multiplied are:
				a3 a2 a1 a0
			      x b3 b2 b1 b0
			      -------------
				a2*b0 a0*b0
			     a2*b1 a0*b1
		          a2*b2	a0*b2
		       a2*b3 a0*b3 
			     a3*b0 a1*b0
			  a3*b1 a1*b1
		       a3*b2 a1*b2
		    a3*b3 a1*b3
		    -----------------------

	   Multiplication is done in two stages:
		       a2*b3 a2*b1 a0*b1
		       a3*b2 a0*b3 a1*b0
			     a3*b0
			     a1*b2

		    a3*b3 a2*b2 a2*b0 a0*b0
			  a3*b1 a0*b2
			  a1*b3 a1*b1
	*/

	x1 = x1save.s[1]; 	 	/* a2 */
	x0 = x0save.s[1];  		/* a0 */
	y1 = y1save.s[0];  		/* b3 */
	y0 = y0save.s[0];		/* b1 */
	result.b[0] = 0;		/* assume no carry */

	r3 = x1 * y1; 			/* a2*b3 */
	r2 = x1 * y0; 			/* a2*b1 */
	r1 = x0 * y1; 			/* a0*b3 */
	r0 = x0 * y0;			/* a0*b1 */
	if ((r2 += r1) < r1) ++r3;	/* a2*b1+a0*b3 with carry propagate */

	x1 = x1save.s[0]; 		/* a3 */
	x0 = x0save.s[0]; 		/* a1 */
	y1 = y1save.s[1]; 		/* b2 */
	y0 = y0save.s[1];		/* b0 */
	r1 = x0 * y0; 			/* a1*b0 */
	if ((result.a[3] = r0 += r1) < r1) if (!++r2) ++r3;	/* a0*b1+a1*b0  with carry propagate */
	r1 = x0 * y1; 			/* a1*b2 */
	if ((r2 += r1) < r1) ++r3;	/* a2*b1+a1*b2+a0*b3 with carry propagate */
	r1 = x1 * y0; 			/* a3*b0 */
	if ((result.a[2] = r2 += r1) < r1) ++r3;	/* a2*b1+a1*b2+a0*b3+a3*b0 with carry propagate */
	r1 = x1 * y1; 			/* a3*b2 */
	if ((result.a[1] = r3 += r1) < r1) result.b[0] = 1;	/* a2*b3+a3*b2 with carry propagate */

	result.b[1] = result.b[2];	/* shift result left by 16 */
	result.b[2] = result.b[3];
	result.b[3] = result.b[4];
	result.b[4] = result.b[5];
	result.b[5] = result.b[6];
	result.b[6] = result.b[7];
	result.b[7] = 0;

	y1 = y1save.s[0]; 		/* b3 */
	y0 = y0save.s[0];		/* b1 */
	r3 = x1 * y1; 			/* a3*b3 */
	r2 = x1 * y0; 			/* a3*b1 */
	r0 = x0 * y1; 			/* a1*b3 */
	r1 = x0 * y0;			/* a1*b1 */
	if ((r2 += r0) < r0) ++r3;	/* a3*b1+a1*b3 with carry propagate */

	x1 = x1save.s[1]; 		/* a2 */
	x0 = x0save.s[1]; 		/* a0 */
	y1 = y1save.s[1]; 		/* b2 */
	y0 = y0save.s[1];		/* b0 */
	r0 = x0 * y0; 			/* a0*b0 */
	x0 *= y1; 			/* a0*b2 */
	y0 *= x1; 			/* a2*b0 */
	x1 *= y1;			/* a2*b2 */

	if ((r1 += x0) < x0) if (!++r2) ++r3;	/* a1*b1+a0*b2 with carry propagate */
	if ((r1 += y0) < y0) if (!++r2) ++r3;	/* a1*b1+a0*b2+a2*b0 with carry propagate */
	if ((r2 += x1) < x1) ++r3;		/* a3*b1+a1*b3+a2*b2 with carry propagate */

	if ((arg[3] = r0 += result.a[3]) < result.a[3]) if (!++r1) if (!++r2) ++r3;	/* add a0*b0 to partial product and propagate carry */
	if ((arg[2] = r1 += result.a[2]) < result.a[2]) if (!++r2) ++r3;		/* add a1*b1+a0*b2+a2*b0 to partial product and propagate carry */
	if ((arg[1] = r2 += result.a[1]) < result.a[1]) ++r3;				/* add a3*b1+a1*b3+a2*b2 to partial product and propagate carry */
	arg[0] = r3 += result.a[0];	/* add a3*b3 to partial product */
}
#endif /* __hp9000s300 */
