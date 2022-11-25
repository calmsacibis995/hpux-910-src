/* @(#) $Revision: 70.2 $ */
#ifndef _LOCALE_INCLUDED
#define _LOCALE_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
  extern "C" {
#endif

#ifdef _INCLUDE__STDC__
#  define LC_ALL      0
#  define LC_COLLATE  1
#  define LC_CTYPE    2
#  define LC_MONETARY 3
#  define LC_NUMERIC  4
#  define LC_TIME     5
#  define LC_MESSAGES 6

#  ifndef NULL
#    define NULL	0
#  endif

  struct lconv {
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
	};

#  ifdef _PROTOTYPES 
     extern char *setlocale(int, const char *);
     extern struct lconv *localeconv(void);
#  else  /* _PROTOTYPES */
     extern char *setlocale();
     extern struct lconv *localeconv();
#  endif  /* _PROTOTYPES */
#endif /* _INCLUDE__STDC__ */


#ifdef _INCLUDE_HPUX_SOURCE
   extern char *_errlocale();

#  define C_LANGID      99  /* Language id for C */
#  define NC_LANGID     0   /* Language id for n-computer */
#  define N_CATEGORY	7   /* Total number of setlocale categories defined */
#  define SL_NAME_SIZE  59  /* Maximum size of the LANG and LC_variables on 
                               HP-UX */
#  define MOD_NAME_SIZE	14  /* Maximum size of the modifier component of the 
                               LANG and LC_ variables on HP-UX */

#  define LC_NAME_SIZE  (SL_NAME_SIZE-MOD_NAME_SIZE)  /* Maximum size of the 
                               LC_variable without the modifier component */

#  define LC_BUFSIZ  (3 + (3+SL_NAME_SIZE)*N_CATEGORY)   /* Maximum buffer 
                               size needed for setlocale return string */


/* definitions for getlocale() */
#  define LOCALE_STATUS   1
#  define MODIFIER_STATUS 2
#  define ERROR_STATUS    3



  struct locale_data {
    char LC_ALL_D[SL_NAME_SIZE];
    char LC_COLLATE_D[SL_NAME_SIZE];
    char LC_CTYPE_D[SL_NAME_SIZE];
    char LC_MONETARY_D[SL_NAME_SIZE];
    char LC_NUMERIC_D[SL_NAME_SIZE];
    char LC_TIME_D[SL_NAME_SIZE];
    char LC_MESSAGES_D[SL_NAME_SIZE]; 
    };

#  ifdef _PROTOTYPES 
     extern struct locale_data *getlocale(int);
#  else  /* _PROTOTYPES */
     extern struct locale_data *getlocale();
#  endif /* _PROTOTYPES */
	
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _LOCALE_INCLUDED */
