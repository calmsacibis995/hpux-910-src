/* static char *HPUX_ID = "@(#) putwchar.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#undef putwchar

#ifdef _NAMESPACE_CLEAN
#    pragma _HP_SECONDARY_DEF _putwchar putwchar
#    define putwchar _putwchar

#    define fputwc _fputwc
#endif

/*****************************************************************************
 *                                                                           *
 *  PUT WIDE CHARACTER ON STDOUT                                             *
 *                                                                           *
 *  INPUTS:      wc            - the character to write out                  *
 *                                                                           *
 *  OUTPUTS:     the original character wc                                   *
 *               WEOF on error.                                              *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

wint_t putwchar(wint_t wc)

{
   return (fputwc(wc, stdout));
}
