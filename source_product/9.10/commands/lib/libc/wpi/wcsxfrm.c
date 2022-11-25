/* static char *HPUX_ID = "@(#) wcsxfrm.c $Revision: 70.2 $"; */

/*LINTLIBRARY*/


#include <wchar.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#ifdef _NAMESPACE_CLEAN
#    undef  wcsxfrm
#    pragma _HP_SECONDARY_DEF _wcsxfrm wcsxfrm
#    define wcsxfrm _wcsxfrm
#endif

/*****************************************************************************
 *                                                                           *
 *  TRANSFORM A WIDE STRING                                                  *
 *                                                                           *
 *  INPUTS:      ws1           - the destination string                      *
 *                                                                           *
 *               ws2           - the string to transform                     *
 *									     *
 *		 n	       - max length of ws1                           *
 *                                                                           *
 *  OUTPUTS:     the length of the converted string                          *
 *               -1 on error.                                                *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  ALGORITHM:   (wcsxfrm is not currently supported.)                       *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

size_t
wcsxfrm(wchar_t *ws1, const wchar_t *ws2, size_t n)
{

   errno = ENOSYS;
   return (size_t)-1;

}
