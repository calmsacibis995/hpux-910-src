/* static char *HPUX_ID = "@(#) wcsncpy.c $Revision: 70.5 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcsncpy
#    pragma _HP_SECONDARY_DEF _wcsncpy wcsncpy
#    define wcsncpy _wcsncpy
#endif

/*****************************************************************************
 *                                                                           *
 *  COPY ONE WIDE CHARACTER STRING TO ANOTHER LIMITED BY COUNT               *
 *                                                                           *
 *  INPUTS:      ws1           - the destination buffer                      *
 *                                                                           *
 *               ws2           - the string to copy to ws1                   *
 *                                                                           *
 *               n             - the maximum wchar count to copy             *
 *                                                                           *
 *  OUTPUTS:     the original pointer ws1.                                   *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws1 is NULL, return NULL since we can't copy anything to *
 *               *NULL.  If ws2 is NULL, write a NULL char to the first char *
 *               of ws1 and return.  Otherwise, proceed to copy ws2 to ws1   *
 *               and return ws1, but don't copy more than n wchars           *
 *                                                                           *
 *  NOTES:       The ws1 buffer must be large enough to store the result.    *
 *               No size checking is performed.                              *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

wchar_t *wcsncpy(wchar_t *ws1, const wchar_t *ws2, size_t n)

{
   wchar_t *ws = ws1;

   if (ws1 == NULL)                     /* can't copy to NULL ws1            */
      return(NULL);

   if (ws2 == NULL || n == 0) {         /* if NULL ws2, return ws1           */
      *ws1 = NULL;
      return(ws1);
   }

   while (n-- > 0) {
      if (*ws2)				/* If not end of string, copy element */
	 *ws++ = *ws2++;
      else
	 *ws++ = L'\0';			/* Else copy null filler */
   }

   return(ws1);   
}
