/* @(#) $Revision: 70.2 $ */
#ifndef _FLOAT_INCLUDED
#define _FLOAT_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef _INCLUDE__STDC__
#  define FLT_RADIX		2
#  define FLT_ROUNDS		1

#  define FLT_MANT_DIG		24
#  ifdef _PROTOTYPES
#    define FLT_EPSILON		1.19209290E-07F
#  else /* not _PROTOTYPES */
#    define FLT_EPSILON		1.19209290E-07
#  endif /* else not _PROTOTYPES */
#  ifndef FLT_DIG
#    define FLT_DIG		6
#  endif /* FLT_DIG */
#  define FLT_MIN_EXP		(-125)
#  ifndef FLT_MIN
#    ifdef _PROTOTYPES
#      define FLT_MIN		1.17549435E-38F
#    else /* not _PROTOTYPES */
#      define FLT_MIN		1.17549435E-38
#    endif /* else not _PROTOTYPES */
#  endif /* FLT_MIN */
#  define FLT_MIN_10_EXP	(-37)
#  define FLT_MAX_EXP		128
#  ifndef FLT_MAX
#    ifdef _PROTOTYPES
#      define FLT_MAX		3.40282347E+38F
#    else /* not _PROTOTYPES */
#      define FLT_MAX		3.40282347E+38
#    endif /* else not _PROTOTYPES */
#  endif /* FLT_MAX */
#  define FLT_MAX_10_EXP	38

#  define DBL_MANT_DIG		53
#  define DBL_EPSILON		2.2204460492503131E-16
#  ifndef DBL_DIG
#    define DBL_DIG		15
#  endif /* DBL_DIG */
#  define DBL_MIN_EXP		(-1021)
#  ifndef DBL_MIN
#    define DBL_MIN		2.2250738585072014E-308
#  endif /* DBL_MIN */
#  define DBL_MIN_10_EXP	(-307)
#  define DBL_MAX_EXP		1024
#  ifndef DBL_MAX
#    define DBL_MAX		1.7976931348623157E+308
#  endif /* DBL_MAX */
#  define DBL_MAX_10_EXP	308

#  define LDBL_MANT_DIG		113
#  define LDBL_EPSILON		1.9259299443872358530559779425849273E-34L
#  define LDBL_DIG		33
#  define LDBL_MIN_EXP		(-16381)
#  define LDBL_MIN		3.3621031431120935062626778173217526026E-4932L
#  define LDBL_MIN_10_EXP	(-4931)
#  define LDBL_MAX_EXP		16384
#  define LDBL_MAX		1.1897314953572317650857593266280070162E4932L
#  define LDBL_MAX_10_EXP	4932
#endif /* _INCLUDE__STDC__ */

#endif /* _FLOAT_INCLUDED */
