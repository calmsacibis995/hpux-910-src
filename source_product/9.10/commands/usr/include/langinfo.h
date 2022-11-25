/* @(#) $Revision: 70.6 $ */
#ifndef _LANGINFO_INCLUDED /* allow multiple inclusions */
#define _LANGINFO_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE

#ifndef _NL_ITEM
#  define _NL_ITEM
   typedef int  nl_item;        /* used by nl_langinfo() to identify items of
	      			   data in a langinfo database */
#endif /* _NL_ITEM */

/*
**  `item' parameter values to the langinfo(3C) routines.
**
**  IMPORTANT NOTE!
**  Whenever an nl_langinfo `item' is added to this file, increase the value
**  of _NL_ITEM_MAX accordingly.
*/
#  define D_T_FMT	1	/* string for formatting data and time */
#  define D_FMT		2	/* date format string */
#  define T_FMT		3	/* time format string */
#  define DAY_1		6	/* name of the 1st day of the week (sunday) */
#  define DAY_2		7	/* name of the 2nd day of the week (monday) */
#  define DAY_3		8	/* name of the 3rd day of the week (thesday) */
#  define DAY_4		9	/* name of the 4th day of the week (wednesday)*/
#  define DAY_5		10	/* name of the 5th day of the week (thursday) */
#  define DAY_6		11	/* name of the 6th day of the week (friday) */
#  define DAY_7		12	/* name of the 7th day of the week (saturday) */
#  define ABDAY_1 13	/* abbreviated name of the first day of the week */	
#  define ABDAY_2 14	/* abbreviated name of the second day of the week */
#  define ABDAY_3 15	/* abbreviated name of the third day of the week */
#  define ABDAY_4 16	/* abbreviated name of the fourth day of the week */
#  define ABDAY_5 17	/* abbreviated name of the fifth day of the week */
#  define ABDAY_6 18	/* abbreviated name of the sixth day of the week */
#  define ABDAY_7 19	/* abbreviated name of the seventh day of the week */
#  define MON_1	  20	/* name of the 1st month in the Gregorian calendar */
#  define MON_2	  21	/* name of the 2nd month in the Gregorian calendar */
#  define MON_3	  22	/* name of the 3rd month in the Gregorian calendar */
#  define MON_4	  23	/* name of the 4th month in the Gregorian calendar */
#  define MON_5	  24	/* name of the 5th month in the Gregorian calendar */
#  define MON_6	  25	/* name of the 6th month in the Gregorian calendar */
#  define MON_7	  26	/* name of the 7th month in the Gregorian calendar*/
#  define MON_8	  27	/* name of the 8th month in the Gregorian calendar */
#  define MON_9	  28	/* name of the 9th month in the Gregorian calendar */
#  define MON_10  29	/* name of the 10th month in the Gregorian calendar */
#  define MON_11  30	/* name of the 11th month in the Gregorian calendar */
#  define MON_12  31	/* name of the 12th month in the Gregorian calendar */
#  define ABMON_1 32	/* abbreviated name of the first month */	
#  define ABMON_2 33	/* abbreviated name of the second month */	
#  define ABMON_3 34	/* abbreviated name of the third month */	
#  define ABMON_4 35	/* abbreviated name of the fourth month */	
#  define ABMON_5 36	/* abbreviated name of the fifth month */	
#  define ABMON_6 37	/* abbreviated name of the sixth month */	
#  define ABMON_7 38	/* abbreviated name of the seventh month */	
#  define ABMON_8 39	/* abbreviated name of the eighth month */	
#  define ABMON_9 40	/* abbreviated name of the ninth month */	
#  define ABMON_10	41	/* abbreviated name of the tenth month */	
#  define ABMON_11	42	/* abbreviated name of the eleventh month */	
#  define ABMON_12	43	/* abbreviated name of the twelfth month */	
#  define RADIXCHAR	44      /* radix character */
#  define THOUSEP	45	/* separator for thousands */
#  define YESSTR	46	/* affirmative response for yes/no queries */
#  define NOSTR		47	/* negative response for yes/no queries */
#  define CRNCYSTR	48	/* currency symbol */
		/* preceded by -, if the symbol should appear before the value
		               +, if the symbol should appear after the value
		               ., if the symbol should replace the radix char 
				*/	
#  define AM_STR 	53	/* ante meridiem suffix */	
#  define PM_STR 	54	/* post meridiem suffix */
#  define ERA_D_FMT	61	/* era date format string */
#  define CODESET       62	/* codeset name - LC_CTYPE */
#  define YESEXPR       72	/* affirmative response expression */
#  define NOEXPR        73	/* negative response expression */
#  define T_FMT_AMPM    74	/* am/pm time format string */
#  define ALT_DIGITS    75      /* alternative symbols for digits - LC_TIME */
#  define ERA           76      /* era description segments */
#  define ERA_D_T_FMT   77      /* era date and time format string */
#  define ERA_T_FMT     78      /* era time format string */

#  if defined(__STDC__) || defined(__cplusplus)
     extern char *nl_langinfo(nl_item);
#  else /* not __STDC__ || __cplusplus */
     extern char *nl_langinfo();
#  endif /* __STDC__ || __cplusplus */

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE

#include <nl_types.h>

/*
**  IMPORTANT NOTE!
**  Whenever an nl_langinfo `item' is added to this file, increase the value
**  of _NL_ITEM_MAX accordingly.
*/
#define _NL_ITEM_MAX	78

/*
**
** HP specific items - keep these separate from standards adopted items
*/
#  define BYTES_CHAR	49
#  define DIRECTION	50		/* direction of language */
#  define ALT_DIGIT	51		/* use these digits instead  - Arabic */
#  define ALT_PUNCT	52
#  define YEAR_UNIT	55
#  define MON_UNIT	56
#  define DAY_UNIT	57
#  define HOUR_UNIT	58
#  define MIN_UNIT	59
#  define SEC_UNIT	60
#  define ERA_FMT	ERA_D_FMT	/* for backwards compatibility */

/* WARNING!!! - do not use items 63-66, 68-71 as they will be reassigned */
#  define LANGNAME      63		/* obsolete to be removed */
#  define REVISION      64		/* obsolete to be removed */
#  define LANGID        65		/* obsolete to be removed language ID */
#  define CODE_SCHEME   66		/* obsolete to be removed LC_CTYPE */
#  define CSWIDTH       67		/* LC_CTYPE */
#  define CONTEXT       68		/* obsolete to be removed */
#  define ESC_CHAR      69		/* obsolete to be removed escape char */
#  define COMM_CHAR     70		/* obsolete to be removed comment char*/
#  define CHARMAP       71		/* obsolete to be removed charmap name*/

/*
**  Error codes returned in the status variable by a call to strcmp8(3C)
**  and strncmp8(3C).
*/
#  define	ENOCFFILE	1	
#  define	ENOCONV		2
#  define	ENOLFILE	3	

#if defined(__STDC__) || defined(__cplusplus)
   extern int currlangid(void);
   extern int langtoid(const char *);
   extern char *idtolang(int);
   extern int nl_init(char *);
   extern int langinit(char *);
#else
   extern int currlangid();
   extern int langtoid();
   extern char *idtolang();
   extern int nl_init();
   extern int langinit();
#endif /* defined(__STDC__) || defined(__cplusplus) */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _LANGINFO_INCLUDED */
