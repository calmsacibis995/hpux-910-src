/* @(#) $Revision: 70.1 $ */   
/*  source:  HP  author: Mark McDowell                     */
/*     /usr/src/lib/libc/vax/gen/ecvt.c                    */

/*LINTLIBRARY*/
/*
 *	ecvt converts to decimal
 *	the number of digits is specified by ndigit
 *	decpt is set to the position of the decimal point
 *	sign is set to 0 for positive, 1 for negative
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define ecvt _ecvt
#define fcvt _fcvt
#endif

#define	NDIG	350

char *cvt();

#ifdef _NAMESPACE_CLEAN
#undef ecvt
#pragma _HP_SECONDARY_DEF _ecvt ecvt
#define ecvt _ecvt
#endif

char *ecvt(arg, ndigits, decpt, sign)
double arg;
int ndigits, *decpt, *sign;
{
	return(cvt(arg, ndigits, decpt, sign, 1));
}

#ifdef _NAMESPACE_CLEAN
#undef fcvt
#pragma _HP_SECONDARY_DEF _fcvt fcvt
#define fcvt _fcvt
#endif

char *fcvt(arg, ndigits, decpt, sign)
double arg;
int ndigits, *decpt, *sign;
{
	return(cvt(arg, ndigits, decpt, sign, 0));
}

static unsigned long table1[27][2] = {
	{ 0xA5CED43B,0x7E3E9188	} ,
	{ 0xAB70FE17,0xC79AC6CB	} ,
	{ 0xB1442798,0xF49FFB4B	} ,
	{ 0xB749FAED,0x14125D37	} ,
	{ 0xBD8430BD,0x8277232	} ,
	{ 0xC3F490AA,0x77BD60FD	} ,
	{ 0xCA9CF1D2,0x6FDC03C	} ,
	{ 0xD17F3B51,0xFCA3A7A1	} ,
	{ 0xD89D64D5,0x7A607745	} ,
	{ 0xDFF97724,0x70297EBE	} ,
	{ 0xE7958CB8,0x7392C2C3	} ,
	{ 0xEF73D256,0xA5C0F77D	} ,
	{ 0xF79687AE,0xD3EEC551	} ,
	{ 0x0,0x0 } ,
	{ 0x84595161,0x401484A0	} ,
	{ 0x88D8762B,0xF324CD10	} ,
	{ 0x8D7EB760,0x70A08AED	} ,
	{ 0x924D692C,0xA61BE758	} ,
	{ 0x9745EB4D,0x50CE6333	} ,
	{ 0x9C69A972,0x84B578D8	} ,
	{ 0xA1BA1BA7,0x9E1632DC	} ,
	{ 0xA738C6BE,0xBB12D16D	} ,
	{ 0xACE73CBF,0xDC0BFB7B	} ,
	{ 0xB2C71D5B,0xCA9023F8	} ,
	{ 0xB8DA1662,0xE7B00A17	} ,
	{ 0xBF21E440,0x3ACDD2D	} ,
	{ 0xC5A05277,0x621BE294	} ,
};

static short table1e[27] = {
	-1080 , -997, -914, -831, -748, -665, -582, -499, -416, -333, -250, -167, -84,
	0, 83, 166, 249, 332, 415, 498, 581, 664, 747, 830, 913, 996, 1079};

static unsigned long table2[24][2] = {
	{ 0xA0000000,0x0	} ,
	{ 0xC8000000,0x0	} ,
	{ 0xFA000000,0x0	} ,
	{ 0x9C400000,0x0	} ,
	{ 0xC3500000,0x0	} ,
	{ 0xF4240000,0x0	} ,
	{ 0x98968000,0x0	} ,
	{ 0xBEBC2000,0x0	} ,
	{ 0xEE6B2800,0x0	} ,
	{ 0x9502F900,0x0	} ,
	{ 0xBA43B740,0x0	} ,
	{ 0xE8D4A510,0x0	} ,
	{ 0x9184E72A,0x0	} ,
	{ 0xB5E620F4,0x80000000	} ,
	{ 0xE35FA931,0xA0000000	} ,
	{ 0x8E1BC9BF,0x4000000	} ,
	{ 0xB1A2BC2E,0xC5000000	} ,
	{ 0xDE0B6B3A,0x76400000	} ,
	{ 0x8AC72304,0x89E80000	} ,
	{ 0xAD78EBC5,0xAC620000	} ,
	{ 0xD8D726B7,0x177A8000	} ,
	{ 0x87867832,0x6EAC9000	} ,
	{ 0xA968163F,0xA57B400	} ,
	{ 0xD3C21BCE,0xCCEDA100	} ,
};

static short table2e[24] = {
	3, 6, 9, 13, 16, 19, 23, 26, 29, 33, 36, 39, 43, 46, 49, 53, 56, 59, 63, 66,
	69, 73, 76, 79};

static unsigned long table3[18][2] = {
	{ 0x00000000,0x00000001	} ,
	{ 0x00000000,0x0000000A	} ,
	{ 0x00000000,0x00000064	} ,
	{ 0x00000000,0x000003E8	} ,
	{ 0x00000000,0x00002710	} ,
	{ 0x00000000,0x000186A0	} ,
	{ 0x00000000,0x000F4240	} ,
	{ 0x00000000,0x00989680	} ,
	{ 0x00000000,0x05F5E100	} ,
	{ 0x00000000,0x3B9ACA00	} ,
	{ 0x00000002,0x540BE400	} ,
	{ 0x00000017,0x4876E800	} ,
	{ 0x000000E8,0xD4A51000	} ,
	{ 0x00000918,0x4E72A000	} ,
	{ 0x00005AF3,0x107A4000	} ,
	{ 0x00038D7E,0xA4C68000	} ,
	{ 0x002386F2,0x6FC10000	} ,
	{ 0x01634578,0x5D8A0000	} 
};

static char *cvt(n, digits, decpt, sign, eflag) double n; 
int digits, *decpt, *sign, eflag;
{
	union numtype {
		struct m {
			long mant1;
			unsigned long mant2;
		} 
		mantissa;
		double value;
	} 
	number; 
	union s {
		unsigned long l;
		short s;
	} 
	swap;

	void mult32();
	int ndigits;
	register unsigned long exp, i1, i0;
	register short shift;
	register char *p1;
	unsigned long mant1, mant2;
	unsigned long m32[4];
	short ilog;
	static char buf[NDIG];


	number.value = n;				

	do {
		do {
			{ 
				register int mask = 0xFFFFF;
				register unsigned long m1;


				*sign = ((long) (exp = m1 = number.mantissa.mant1) < 0) ? 1 : 0;
				mant2 = number.mantissa.mant2;		
				m1 &= mask++;
				exp ^= m1;				
				exp += exp;				
				if (!(exp >>= 1)) {
					if (m1) {
						i0 = 1;
						i1 = 0x80000;
						while (!(m1 & i1)) {
							i0++;
							i1 >>= 1;
						};
						m1 = m1 << i0 | mant2 >> 32 - i0;
						mant2 <<= i0;
					}
					else {
						if (m1 = mant2) {
							swap.l = m1;
							if (swap.s) {
								i0 = -11;
								i1 = 0x80000000;
							}
							else {
								i0 = 5;
								i1 = 0x8000;
							};
							while (!(m1 & i1)) {
								i0++;
								i1 >>= 1;
							};
							if ((long) i0 < 0) {
								mant2 = m1 << 32 + i0;
								m1 >>= -i0;
							}
							else {
								m1 <<= i0;
								mant2 = 0;
							};
							i0 += 32;
						}
						else break;
					};
					swap.l = 0;
					swap.s = --i0 << 4;
					exp -= swap.l;


				}
				else if ((long) (mask + exp) < 0) break;
				mant1 = m1 |= mask;				
			};

			{
				register long log2 = 0x4D10;
				swap.l = exp;
				if ((long) (exp = ((long) swap.s - 0x3FF0) >> 4) < 0) log2++;
				swap.l = exp * log2;
				ilog = swap.s;
			};

			if (digits < 0) digits = 0;
			if (!eflag) digits = ilog + digits + 1;
			if (digits >= NDIG - 1) digits = NDIG - 2;
			if ((ndigits = digits) > 17) ndigits = 17;
			if (ndigits <= 0) ndigits = 0;

				do {
					do {
						register short iscale;
						iscale = ndigits - ilog - 1;		
						shift = 82 - exp;
						if ((long) (i0 = iscale - (short) (i1 = iscale / 25) * 25) < 0) {
							i1--;
							i0 += 25;
						};
						if (i0) {
							m32[0] = table2[--i0][0];
							m32[1] = table2[i0][1];
							shift -= table2e[i0];
							if (i1) {
								m32[2] = table1[i1 += 13][0];
								m32[3] = table1[i1][1];
								shift -= table1e[i1];
								mult32(m32);
								if ((long) m32[2] < 0) {
									if (!++m32[1]) ++m32[0];
									if (!(m32[2] << 1 | m32[3])) m32[1] &= ~1;
								};
							}
							else shift++;
						} 
						else {
							if (i1) {
								m32[0] = table1[i1 += 13][0];
								m32[1] = table1[i1][1];
								shift -= table1e[i1];
								shift++;
							} 
							else {
								m32[0] = mant1;
								m32[1] = mant2;
								m32[2] = 0;
								m32[3] = 0;
								shift += 2;
								break;
							};
						};
						m32[2] = mant1;
						m32[3] = mant2;
						mult32(m32);
					} 
					while (0);

					{
						register short i;
						register unsigned long round,sticky;
						i = 2 - (shift >> 5);
						if (shift & 31) {
							i1 = 1 << (shift & 31) - 1;
						} 
						else {
							i1 = 0x80000000;
							i++;
						};
						if (i1 &= m32[i]) {
							i0 = m32[i];
							i0 &= ~-i1;
							do {
								i0 |= m32[++i];
							} 
							while (i <= 2);
							sticky = i0;
						};
						round = i1;
						i1 = m32[0];
						i0 = m32[1];
						if (i = shift & 31) {
							if (shift < 32) {
								i1 = i1 << 32 - i | i0 >> i;
								i0 = i0 << 32 - i | m32[2] >> i;
							}
							else {
								if (shift > 64) {
									i0 = i1 >> i;
									i1 = 0;
								}
								else {
									i0 = i0 >> i | i1 << 32 - i;
									i1 >>= i;
								};
							};
						} 
						else {
							if (shift == 64) {
								i0 = i1;
								i1 = 0;
							};
						};
						if (round) { 
							if (!++i0) ++i1;
							if (!sticky) i0 &= ~1;
						};
					};
					{
						register int d;
						if (i1 < table3[d = ndigits][0]) break;
						if (i1 == table3[d][0]) {
							if (i0 < table3[d][1]) break;
							if (i0 == table3[d][1]) {

								ilog++;
								if (eflag) {
									if (!d) break;
									i1 = table3[d-1][0];
									i0 = table3[d-1][1];
								}
								else {
									ndigits++;
									digits++;
								};
								break;
							};
						};
						ilog++;
						if (!eflag) {
							ndigits++;
							digits++;
							break;
						};
					};
				} 
				while (1);
				if (digits <= 0) {
					*decpt = ilog + 1;
					buf[0] = 0;
					return (buf);
				};
				{ 
					short d;
					register unsigned long a1, a0;
					d = ndigits - 1;
					p1 = buf;
					while (i1) {
						*p1 = '0';
						a1 = table3[d][0];
						a0 = table3[d--][1];
						while (1) {
							if (a1 > i1) break;
							if (a1 == i1) if (a0 > i0) break;
							(*p1)++;
							if (a0 > i0) --i1;
							i1 -= a1;
							i0 -= a0;
						};
						p1++;
					};
					while (table3[d][0]) {
						d--;
						if (p1 > buf) *p1++ = '0';
					};
					while (d) {
						*p1 = '0';
						a0 = table3[d--][1];
						while (i0 >= a0) {
							(*p1)++;
							i0 -= a0;
						};
						p1++;
					};
					*p1++ = (char) i0 + '0';
				};


			{
				register i;
				i = digits - ndigits;
				while (i--) *p1++ = '0';
				*p1 = 0;
				*decpt = ilog + 1;
				if (1) return (buf);
			};
		} 
		while (0);
		if (exp) break;
		{
			register short i;
			if ((i = digits) < 0) i = 0;
			if (i >= NDIG - 1) i = NDIG - 2;
			p1 = buf;
			while (i--) *p1++ = '0';
			*p1 = 0;
			*decpt = 0;
			if (1) return (buf);
		};
	} 
	while (0);
	p1 = buf;
	if (number.mantissa.mant1 & 0xFFFFF | number.mantissa.mant2) {
		*p1++ = '?';
		*decpt = 1;
	}
	else {
		if (*sign) {
			*p1++ = '-';
			*p1++ = '-';
		}
		else {
			*p1++ = '+';
			*p1++ = '+';
		};
		*decpt = 2;
	};
	*p1 = 0;
	return (buf);
}


/* "mult32" multiplies two 64 bit unsigned integers and produces a 128 unsigned result. The arguments are passed in (arg[0],arg[1]) and (arg[2],arg[3]) and the
result is returned in (arg[0],arg[1],arg[2],arg[3]). */
#ifdef   __hp9000s300
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

#else /* __hp9000s300 */

static void mult32(arg) unsigned long arg[];
{
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

	register unsigned long r0,r1,r2,r3,x1,x0,y1,y0;
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


