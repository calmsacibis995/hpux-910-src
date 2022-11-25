/* @(#) $Revision: 64.3 $ */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define localeconv _localeconv
#endif

#include <locale.h>
#include <limits.h>

struct lconv _lconv_data =  {           /* Initial values for the "C" locale */
                            ".",
                            "",
                            "",
                            "",
                            "",
                            "",
                            "",
                            "",
                            "",
                            "",
                            CHAR_MAX,
                            CHAR_MAX,
                            CHAR_MAX,
                            CHAR_MAX,
                            CHAR_MAX,
                            CHAR_MAX,
                            CHAR_MAX,
                            CHAR_MAX
                            };

struct lconv *_lconv = &_lconv_data;


/* Routine to return pointer to structure containing monetary and */
/* numeric formatting information for the current locale. */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef localeconv
#pragma _HP_SECONDARY_DEF _localeconv localeconv
#define localeconv _localeconv
#endif

struct lconv *localeconv()
{
    return(_lconv);
}
