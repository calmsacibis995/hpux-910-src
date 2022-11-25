/* static char *HPUX_ID = "@(#) getwchar.c $Revision: 70.2 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#undef getwchar

#ifdef _NAMESPACE_CLEAN
#    pragma _HP_SECONDARY_DEF _getwchar getwchar
#    define getwchar _getwchar

#    define fgetwc _fgetwc
#endif

/*****************************************************************************
 *                                                                           *
 *  GET WIDE CHARACTER FROM STDIN                                            *
 *                                                                           *
 *  INPUTS:      <none>                                                      *
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

wint_t getwchar(void)

{
   return (fgetwc(stdin));
}
