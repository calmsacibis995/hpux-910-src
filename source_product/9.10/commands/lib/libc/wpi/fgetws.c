/* static char *HPUX_ID = "@(#) fgetws.c $Revision: 70.4 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef fgetws
#    pragma _HP_SECONDARY_DEF _fgetws fgetws
#    define fgetws _fgetws

#    define fgetwc _fgetwc
#endif

/*****************************************************************************
 *                                                                           *
 *  GET WIDE STRING FROM STREAM                                              *
 *                                                                           *
 *  INPUTS:      ws            - the destination string                      *
 *                                                                           *
 *               n             - max # characters to get                     *
 *									     *
 *               stream        - the stream to read from                     *
 *                                                                           *
 *  OUTPUTS:     the original pointer ws                                     *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

wchar_t *
fgetws(wchar_t *ws, int n, FILE *stream)

{
   wchar_t wc = L'\0';
   wchar_t *wsp = ws;

   /* Read from the stream until n-1 chars are read, we get a newline
      (which is copied), or we reach EOF */
   while ((--n > 0) && ((wc = fgetwc(stream)) != WEOF)) {
      *wsp++ = wc;
      if (wc == L'\n')
	 break;
      }
   /* Don't write a null wchar if we haven't read anything in.  VSX4 tests
      for this. */
   if (ws != wsp)
      *wsp = L'\0';
   return (wc == WEOF ? NULL : ws);
}
