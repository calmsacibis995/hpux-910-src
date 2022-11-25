/* static char *HPUX_ID = "@(#) putwc.c $Revision: 70.2 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#undef putwc

#ifdef _NAMESPACE_CLEAN
#    pragma _HP_SECONDARY_DEF _putwc putwc
#    define putwc _putwc

#    define fputwc _fputwc
#endif

/*****************************************************************************
 *                                                                           *
 *  PUT WIDE CHARACTER ON STREAM                                             *
 *                                                                           *
 *  INPUTS:      wc            - the character to write out                  *
 *                                                                           *
 *               stream        - the stream to write on                      *
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

wint_t putwc(wint_t wc, FILE *stream)

{
   return (fputwc(wc, stream));
}
