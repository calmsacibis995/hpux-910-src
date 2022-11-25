/* @(#) $Revision: 70.3 $ */       
/* Maximum number of digits in any integer representation */
#define MAXDIGS 11

/* Maximum total number of digits in E format */
#define MAXECVT 17
#define MAXQECVT 34

/* Maximum number of digits after decimal point in F format */
/* make sure this agrees with range of DBL_MIN/DBL_MAX in limits.h */
#define MAXFCVT 308
#define MAXQFCVT MAXFCVT

#if defined(NLS) || defined(NLS16)
/* Maximum number of grouping chars in any integer representation and locale */
#define MAXIGRP (MAXDIGS-1)

/* Maximum total number of grouping chars in F format */
#define MAXFGRP (MAXFCVT-1)
#define MAXQFGRP (MAXQFCVT-1)

#else /* not NLS || NLS16 */

#define MAXIGRP 0
#define MAXFGRP 0
#define MAXQFGRP 0

#endif /* not NLS || NLS16 */

/* Maximum significant figures in a floating-point number */
#define MAXFSIG MAXECVT
#define MAXQFSIG MAXQECVT

/* Maximum number of characters in an exponent */
/* The same value is used for doubles and long doubles, but in reality */
/* doubles are max 5 chars; long doubles are max 6 chars. (i.e. "E+4933") */
#define MAXESIZ 6

/* Maximum (positive) exponent */
#define MAXEXP 310
#define MAXQEXP 4933

/* Data type for flags */
typedef char bool;

/* Convert a digit character to the corresponding number */
#define tonumber(x) ((x)-'0')

/* Convert a number between 0 and 9 to the corresponding digit */
#define todigit(x) ((x)+'0')

/* Max and Min macros */
#define max(a,b) ((a) > (b)? (a): (b))
#define min(a,b) ((a) < (b)? (a): (b))

#ifndef _LONG_DOUBLE
#define _LONG_DOUBLE
/* kludge for long double type */
typedef struct {
	unsigned int word1, word2, word3, word4;
} long_double;
#endif /* _LONG_DOUBLE */
