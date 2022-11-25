/* static char *HPUX_ID = "@(#) fputws.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef fputws
#    pragma _HP_SECONDARY_DEF _fputws fputws
#    define fputws _fputws

#    define fputwc _fputwc
#endif

/*****************************************************************************
 *                                                                           *
 *  PUT WIDE STRING ON STREAM                                                *
 *                                                                           *
 *  INPUTS:      ws            - the character to write                      *
 *                                                                           *
 *               stream        - the stream to write on                      *
 *                                                                           *
 *  OUTPUTS:     0 upon success                                              *
 *               -1 on error                                                 *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

int fputws(const wchar_t *ws, FILE *stream)

{

   while (*ws) {
      if (fputwc(*ws++, stream) == WEOF)
         return -1;
   }
   return 0;
}
