/* @(#) $Revision: 70.6 $ */
#ifndef _MATH_INCLUDED
#define _MATH_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__BUILTIN_MATH) && !defined(__cplusplus) && !defined(_SVID2) && !defined(_XPG2)

# pragma NO_SIDE_EFFECTS acos, asin, atan, atan2, cos, sin, tan
# pragma NO_SIDE_EFFECTS acosd, asind, atand, atan2d, cosd, sind, tand
# pragma NO_SIDE_EFFECTS cosh, sinh, tanh, acosh, asinh, atanh
# pragma NO_SIDE_EFFECTS exp, frexp, ldexp, log, log10, log2, pow
# pragma NO_SIDE_EFFECTS sqrt, ceil, fabs, floor, fmod
# pragma NO_SIDE_EFFECTS acosf, asinf, atanf, atan2f, cosf, sinf, tanf
# pragma NO_SIDE_EFFECTS sinhf, coshf, tanhf, acosdf, asindf, atandf, atan2df
# pragma NO_SIDE_EFFECTS cosdf, sindf, tandf, expf, fabsf, logf, log10f, log2f
# pragma NO_SIDE_EFFECTS powf, sqrtf, fmodf
# pragma NO_SIDE_EFFECTS rint, finite, finitef, copysign, copysignf
# pragma NO_SIDE_EFFECTS scalb, logb, drem

#endif /* #if for pragma */

#ifdef _INCLUDE__STDC__
#  define HUGE_VAL    1.7976931348623157e+308
#  if defined(__STDC__) || defined(__cplusplus)
     extern double acos(double);
     extern double asin(double);
     extern double atan(double);
     extern double atan2(double, double);
     extern double cos(double);
     extern double sin(double);
     extern double tan(double);
/* Degree-value trigonometric functions */
#ifdef _INCLUDE_HPUX_SOURCE
     extern double acosd(double);
     extern double asind(double);
     extern double atand(double);
     extern double atan2d(double, double);
     extern double cosd(double);
     extern double sind(double);
     extern double tand(double);
#endif
     extern double cosh(double);
     extern double sinh(double);
     extern double tanh(double);
#ifdef _INCLUDE_HPUX_SOURCE
     extern double acosh(double);
     extern double asinh(double);
     extern double atanh(double);
#endif
     extern double exp(double);
     extern double frexp(double, int *);
     extern double ldexp(double, int);
     extern double log(double);
     extern double log10(double);
#ifdef _INCLUDE_HPUX_SOURCE
     extern double log2(double);
#endif
     extern double modf(double, double *);
     extern double pow(double, double);
#    ifdef __cplusplus
     }
     inline double pow(double __d,int __expon) {
	 return pow(__d,(double)__expon);
     }
     extern "C" {
#else
#    endif
     extern double sqrt(double);
     extern double ceil(double);
     extern double fabs(double);
     extern double floor(double);
     extern double fmod(double, double);

#ifdef _INCLUDE_HPUX_SOURCE
/* 
 * Float-type Mathematical functions:
 * The following mathematical functions are provided for high performance
 * low-precision float-type applications. These functions will correctly 
 * set errno and return a reasonable result on error, but the error handling 
 * features are not guaranteed to be completely standards compliant. These 
 * functions will NOT invoke matherr().
 */
     extern float acosf( float);
     extern float asinf( float);
     extern float atanf( float);
     extern float atan2f( float, float);
     extern float cosf( float);
     extern float sinf( float);
     extern float tanf( float);
     extern float sinhf( float);     
     extern float coshf( float);
     extern float tanhf( float);
     extern float acosdf( float);
     extern float asindf( float);
     extern float atandf( float);
     extern float atan2df( float, float);
     extern float cosdf( float);
     extern float sindf( float);
     extern float tandf( float);
     extern float expf( float);
     extern float fabsf( float);
     extern float logf( float);
     extern float log10f( float);
     extern float log2f( float);
     extern float powf( float, float );
     extern float sqrtf(float);
     extern float fmodf(float,float);

/* 
 * IEEE Recommended Functions (double and some float type versions):
 * The following functions are recommended in the IEEE 754 Floating Point Standard:
 *
 */

     extern double rint(double);
     extern int    finite(double);
     extern int    finitef(float);
     extern double copysign(double,double);
     extern float  copysignf(float,float);
     extern double scalb(double,int);
     extern int    logb(double);
     extern double drem(double, double);

#endif /* _HPUX_SOURCE */

#  else /* not __STDC__ || __cplusplus */
     extern double acos();
     extern double asin();
     extern double atan();
     extern double atan2();
     extern double cos();
     extern double sin();
     extern double tan();
/* Degree-value trigonometric functions */
#ifdef _INCLUDE_HPUX_SOURCE
     extern double acosd();
     extern double asind();
     extern double atand();
     extern double atan2d(); 
     extern double cosd();
     extern double sind();
     extern double tand();
#endif
     extern double cosh();
     extern double sinh();
     extern double tanh();
#ifdef _INCLUDE_HPUX_SOURCE
     extern double acosh();
     extern double asinh();
     extern double atanh();
#endif
     extern double exp();
     extern double frexp();
     extern double ldexp();
     extern double log();
     extern double log10();
#ifdef _INCLUDE_HPUX_SOURCE
     extern double log2();
#endif
     extern double modf();
     extern double pow();
     extern double sqrt();
     extern double ceil();
     extern double fabs();
     extern double floor();
     extern double fmod();

#ifdef _INCLUDE_HPUX_SOURCE
     extern double rint();
     extern int    finite();
     extern double copysign();
     extern double scalb();
     extern int    logb();
     extern double drem();
#endif
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE__STDC__ */

#ifdef _INCLUDE_XOPEN_SOURCE

/* some useful constants */
#  define M_E		2.7182818284590452354
#  define M_LOG2E	1.4426950408889634074
#  define M_LOG10E	0.43429448190325182765
#  define M_LN2		0.69314718055994530942
#  define M_LN10	2.30258509299404568402
#  define M_PI		3.14159265358979323846
#  define M_PI_2	1.57079632679489661923
#  define M_PI_4	0.78539816339744830962
#  define M_1_PI	0.31830988618379067154
#  define M_2_PI	0.63661977236758134308
#  define M_2_SQRTPI	1.12837916709551257390
#  define M_SQRT2	1.41421356237309504880
#  define M_SQRT1_2	0.70710678118654752440
#  define MAXFLOAT	((float)3.40282346638528860e+38)
#if defined(__BUILTIN_MATH) && !defined(__cplusplus) && !defined(_SVID2) && !defined(_XPG2)

# pragma NO_SIDE_EFFECTS erf, erfc, gamma, lgamma, isnan
# pragma NO_SIDE_EFFECTS isnanf, isinf, isinff
# pragma NO_SIDE_EFFECTS hypot, j0, j1, jn, y0, y1, yn

#endif /* #if for pragma */

#  if defined(__STDC__) || defined(__cplusplus)
     extern double erf(double);
     extern double erfc(double);
     extern double gamma(double);
     extern double lgamma(double);
     extern int    isnan(double);
#ifdef _INCLUDE_HPUX_SOURCE
     extern int    isnanf(float);
     extern int    isinf(double);
     extern int    isinff(float);
#endif
     extern double hypot(double, double);
     extern double j0(double);
     extern double j1(double);
     extern double jn(int, double);
     extern double y0(double);
     extern double y1(double);
     extern double yn(int, double);
#  else /* not __STDC__ || __cplusplus */
     extern double erf();
     extern double erfc();
     extern double gamma();
     extern double lgamma();
     extern int    isnan();
     extern int    isinf();                
     extern double hypot();
     extern double j0();
     extern double j1();
     extern double jn();
     extern double y0();
     extern double y1();
     extern double yn();
#  endif /* __STDC__ || __cplusplus */

   extern int signgam;

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE

#  define HUGE    	HUGE_VAL

#  ifndef _LONG_DOUBLE
#    define _LONG_DOUBLE
     typedef struct {
       unsigned int word1, word2, word3, word4;
     } long_double;
#  endif /* _LONG_DOUBLE */

   struct exception {
	int type;
	char *name;
	double arg1;
	double arg2;
	double retval;
   };

   extern int errno;

#if defined(__BUILTIN_MATH) && !defined(__cplusplus) && !defined(_SVID2) && !defined(_XPG2)

# pragma NO_SIDE_EFFECTS cabs, atof

#endif /* #if for pragma */

#  if defined(__STDC__) || defined(__cplusplus)
     extern int matherr(struct exception *);
     extern double cabs();
#    ifndef _ATOF_DEFINED
#      define _ATOF_DEFINED
       extern double atof(const char *);
#    endif /* _ATOF_DEFINED */
#  else /* not __STDC__ || __cplusplus */
     extern int matherr();
     extern double cabs();
#    ifndef _ATOF_DEFINED
#      define _ATOF_DEFINED
       extern double atof();
#    endif /* _ATOF_DEFINED */
#  endif /* __STDC__ || __cplusplus */

#  define _ABS(x)	((x) < 0 ? -(x) : (x))
#  define _REDUCE(TYPE, X, XN, C1, C2)	{ \
      	double x1 = (double)(TYPE)X, x2 = X - x1; \
	X = x1 - (XN) * (C1); X += x2; X -= (XN) * (C2); }
#  define _POLY1(x, c)	((c)[0] * (x) + (c)[1])
#  define _POLY2(x, c)	(_POLY1((x), (c)) * (x) + (c)[2])
#  define _POLY3(x, c)	(_POLY2((x), (c)) * (x) + (c)[3])
#  define _POLY4(x, c)	(_POLY3((x), (c)) * (x) + (c)[4])
#  define _POLY5(x, c)	(_POLY4((x), (c)) * (x) + (c)[5])
#  define _POLY6(x, c)	(_POLY5((x), (c)) * (x) + (c)[6])
#  define _POLY7(x, c)	(_POLY6((x), (c)) * (x) + (c)[7])
#  define _POLY8(x, c)	(_POLY7((x), (c)) * (x) + (c)[8])
#  define _POLY9(x, c)	(_POLY8((x), (c)) * (x) + (c)[9])

#  define	DOMAIN		1
#  define	SING		2
#  define	OVERFLOW	3
#  define	UNDERFLOW	4
#  define	TLOSS		5
#  define	PLOSS		6

/********************************************/
/* fpclassify operand analysis routines     */
/********************************************/

/* These codes are returned by the fpclassify(f) routines. */

#define FP_PLUS_NORM      0
#define FP_MINUS_NORM     1
#define FP_PLUS_ZERO      2
#define FP_MINUS_ZERO     3
#define FP_PLUS_INF       4
#define FP_MINUS_INF      5
#define FP_PLUS_DENORM    6
#define FP_MINUS_DENORM   7
#define FP_SNAN           8
#define FP_QNAN           9

#  if defined(__STDC__) || defined(__cplusplus)
     extern int fpclassify(double);
     extern int fpclassifyf(float);
     extern int isnan(double);
     extern int isnanf(float);
#else
     extern int fpclassify(); 
     extern int isnan();
#endif

/********************************************/
/* fpgetround suite of FPU service routines */
/********************************************/

/* fpgetround Types */

typedef long fp_control;
typedef int fp_except;
typedef enum {
    FP_RZ=0,
    FP_RN,
    FP_RP,
    FP_RM 
    } fp_rnd;

#  if defined(__STDC__) || defined(__cplusplus)
extern fp_rnd fpgetround(void);
extern fp_rnd fpsetround(fp_rnd);
#  else /* not __STDC__ || __cplusplus */      
extern fp_rnd fpgetround();
extern fp_rnd fpsetround();
#  endif /* __STDC__ || __cplusplus */


/* Floating Point Exception Flags */
#define FP_X_INV       0x10  /* invalid operation exceptions */
#define FP_X_DZ        0x08  /* divide-by-zero exception */
#define FP_X_OFL       0x04  /* overflow exception */
#define FP_X_UFL       0x02  /* underflow exception */
#define FP_X_IMP       0x01  /* imprecise (inexact result) */
#define FP_X_CLEAR     0x00  /* simply zero to clear all flags */
                             
extern fp_except fpgetmask();

#  if defined(__STDC__) || defined(__cplusplus)
extern fp_except fpgetmask(void);
extern fp_except fpsetmask(fp_except);
extern fp_except fpgetsticky(void);
extern fp_except fpsetsticky(fp_except);  
#  else /* not __STDC__ || __cplusplus */
extern fp_except fpgetmask();
extern fp_except fpsetmask();
extern fp_except fpgetsticky();
extern fp_except fpsetsticky();  
#  endif /* __STDC__ || __cplusplus */
                        
/* 
 * fpsetdefaults sets modes as follows:                     
 *         rounding mode : FP_RN                             
 *         sticky flags  : FP_X_CLEAR                        
 *         mask          : FP_X_INV+FP_X_DZ+FP_X_OFL         
 *         fast mode     : TRUE (eg denorms treated as zero) 
 */


#  if defined(__STDC__) || defined(__cplusplus)
extern void       fpsetdefaults(void);
extern int        fpgetfastmode(void);
extern int        fpsetfastmode(int);
extern fp_control fpgetcontrol(void);
extern fp_control fpsetcontrol(fp_control);
#  else /* not __STDC__ || __cplusplus */
extern void       fpsetdefaults();
extern int        fpgetfastmode();
extern int        fpsetfastmode(); 
extern fp_control fpgetcontrol();
extern fp_control fpsetcontrol();
#  endif /* __STDC__ || __cplusplus */

/*
 * Fortran applications: The above fpgetround suite of calls can be 
 * used in fortran applications by inserting the following into the 
 * fortran source code:
 *
 * $ALIAS fpsetround    = 'fpsetround'    (%val)
 * $ALIAS fpsetsticky   = 'fpsetsticky'   (%val)
 * $ALIAS fpsetmask     = 'fpsetmask'     (%val)
 * $ALIAS fpsetfastmode = 'fpsetfastmode' (%val)
 * $ALIAS fpsetcontrol  = 'fpsetcontrol'  (%val)
 * $ALIAS fpsetdefaults = 'fpsetdefaults'
 *
 *        integer fpgetround
 *        integer fpgetsticky
 *        integer fpgetmask
 *        integer fpgetfastmode
 *        external fpgetround,fpgetsticky,fpgetmask,fpgetfastmode
 *
 * C Note that fpsetdefaults has no parameters or return value.       
 *
 */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
inline int sqr(int x) {return(x*x);}  /* For AT&T compatibility */
inline double sqr(double x) {return(x*x);}

#ifndef _STDLIB_INCLUDED
inline int abs(int d) { return (d>0)?d:-d; }
#endif /* _STDLIB_INCLUDED */

inline double abs(double d) { return fabs(d); }
#endif


#endif /* _MATH_INCLUDED */


