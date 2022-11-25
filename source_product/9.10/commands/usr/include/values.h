/* @(#) $Revision: 64.2 $ */     

#ifndef _VALUES_INCLUDED /* allow multiple inclusions */
#define _VALUES_INCLUDED

/*
 * These values work with any binary representation of integers
 * where the high-order bit contains the sign.
 */

/* a number used normally for size of a shift */
#define BITSPERBYTE	8
#define BITS(type)	(BITSPERBYTE * (int)sizeof(type))

/*
 * short, regular and long ints with only the high-order bit turned on
 */
#define HIBITS	((short)(1 << BITS(short) - 1))
#define HIBITI	(1 << BITS(int) - 1)
#define HIBITL	(1L << BITS(long) - 1)

/* largest short, regular and long int */
#define MAXSHORT	((short)~HIBITS)
#define MAXINT	(~HIBITI)
#define MAXLONG	(~HIBITL)

/* various values that describe the binary floating-point representation
 * _EXPBASE	- the exponent base
 * DMAXEXP 	- the maximum exponent of a double (as returned by frexp())
 * FMAXEXP 	- the maximum exponent of a float  (as returned by frexp())
 * DMINEXP 	- the minimum exponent of a double (as returned by frexp())
 * FMINEXP 	- the minimum exponent of a float  (as returned by frexp())
 * MAXDOUBLE	- the largest double
			((_EXPBASE ** DMAXEXP) * (1 - (_EXPBASE ** -DSIGNIF)))
 * MAXFLOAT	- the largest float
			((_EXPBASE ** FMAXEXP) * (1 - (_EXPBASE ** -FSIGNIF)))
 * MINDOUBLE	- the smallest double (_EXPBASE ** (DMINEXP - 1))
 * MINFLOAT	- the smallest float (_EXPBASE ** (FMINEXP - 1))
 * DSIGNIF	- the number of significant bits in a double
 * FSIGNIF	- the number of significant bits in a float
 * DMAXPOWTWO	- the largest power of two exactly representable as a double
 * FMAXPOWTWO	- the largest power of two exactly representable as a float
 * _IEEE	- 1 if IEEE standard representation is used
 * _DEXPLEN	- the number of bits for the exponent of a double
 * _FEXPLEN	- the number of bits for the exponent of a float
 * _HIDDENBIT	- 1 if high-significance bit of mantissa is implicit
 * LN_MAXDOUBLE	- the natural log of the largest double  -- log(MAXDOUBLE)
 * LN_MINDOUBLE	- the natural log of the smallest double -- log(MINDOUBLE)
 * LN_MAXFLOAT	- the natural log of the largest single-precision
 *		  floating-point number  -- log(MAXFLOAT)
 * LN_MINFLOAT	- the natural log of the smallest single-precision
 *		  floating-point number -- log(MINFLOAT)
 */

#define	_IEEE		1
#define _DEXPLEN	11
#define _HIDDENBIT	1

#define MAXDOUBLE	1.7976931348623157e+308
#define MAXFLOAT	((float)3.40282346638528860e+38)
#define LN_MAXFLOAT	88.7228394
#ifdef __hp9000s800
#define	MINDOUBLE	(double)4.94065645841246544e-324
#define MINFLOAT        ((float)1.40129846432481707e-45) /* min positive value
							    of a float */
#define LN_MINFLOAT	(-103.278931)
#define DMINEXP	(-(DMAXEXP + DSIGNIF - _HIDDENBIT - 3))
#define FMINEXP	(-(FMAXEXP + FSIGNIF - _HIDDENBIT - 3))
#else
#define MINDOUBLE       2.2250738585072014e-308
#define MINFLOAT        ((float)1.1754943508222875e-38)
#define LN_MINFLOAT     (-87.336544750553102)
#define DMINEXP	(-(DMAXEXP - 3))
#define FMINEXP	(-(FMAXEXP - 3))
#endif /* else __hp9000s800 */

#define _LENBASE	1
#define _EXPBASE	(1 << _LENBASE)
#define _FEXPLEN	8
#define DSIGNIF	(BITS(double) - _DEXPLEN + _HIDDENBIT - 1)
#define FSIGNIF	(BITS(float)  - _FEXPLEN + _HIDDENBIT - 1)
#define DMAXPOWTWO	((double)(1L << BITS(long) - 2) * \
				(1L << DSIGNIF - BITS(long) + 1))
#define FMAXPOWTWO	((float)(1L << FSIGNIF - 1))
#define DMAXEXP	((1 << _DEXPLEN - 1) - 1 + _IEEE)
#define FMAXEXP	((1 << _FEXPLEN - 1) - 1 + _IEEE)
#define LN_MAXDOUBLE	(M_LN2 * DMAXEXP)
#define LN_MINDOUBLE	(M_LN2 * (DMINEXP - 1))
#define H_PREC	(DSIGNIF % 2 ? (1L << DSIGNIF/2) * M_SQRT2 : 1L << DSIGNIF/2)
#define X_EPS	(1.0/H_PREC)
#define X_PLOSS	((double)(long)(M_PI * H_PREC))
#define X_TLOSS	(M_PI * DMAXPOWTWO)
#define M_LN2	0.69314718055994530942
#define M_PI	3.14159265358979323846
#define M_SQRT2	1.41421356237309504880
#define MAXBEXP	DMAXEXP /* for backward compatibility */
#define MINBEXP	DMINEXP /* for backward compatibility */
#define MAXPOWTWO	DMAXPOWTWO /* for backward compatibility */

#endif /* _VALUES_INCLUDED */
