/* static char *HPUX_ID = "@(#) getwc.c $Revision: 70.2 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#undef getwc

#ifdef _NAMESPACE_CLEAN
#    pragma _HP_SECONDARY_DEF _getwc getwc
#    define getwc _getwc

#    define fgetwc _fgetwc
#endif

/*****************************************************************************
 *                                                                           *
 *  GET WIDE CHARACTER FROM STREAM                                           *
 *                                                                           *
 *  INPUTS:      stream        - the stream to read from                     *
 *                                                                           *
 *  OUTPUTS:     the character that was read                                 *
 *               WEOF on error.                                              *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

wint_t getwc(FILE *stream)

{
   return (fgetwc(stream));
}
