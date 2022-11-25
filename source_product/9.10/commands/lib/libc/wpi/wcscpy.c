/* static char *HPUX_ID = "@(#) wcscpy.c $Revision: 70.4 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcscpy
#    pragma _HP_SECONDARY_DEF _wcscpy wcscpy
#    define wcscpy _wcscpy
#endif

/*****************************************************************************
 *                                                                           *
 *  COPY ONE WIDE CHARACTER STRING TO ANOTHER                                *
 *                                                                           *
 *  INPUTS:      ws1           - the destination buffer                      *
 *                                                                           *
 *               ws2           - the string to copy to ws1                   *
 *                                                                           *
 *  OUTPUTS:     the original pointer ws1.                                   *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws1 is NULL, return NULL since we can't copy anything to *
 *               *NULL.  If ws2 is NULL, write a NULL char to the first char *
 *               of ws1 and return.  Otherwise, proceed to copy ws2 to ws1   *
 *               and return ws1.                                             *
 *                                                                           *
 *  NOTES:       The ws1 buffer must be large enough to store the result.    *
 *               No size checking is performed.                              *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

wchar_t *wcscpy(wchar_t *ws1, const wchar_t *ws2)

{
   wchar_t *ws = ws1;

   if (ws1 == NULL)                     /* can't copy to NULL ws1            */
      return(NULL);

   if (ws2 == NULL) {                   /* if NULL ws2, return ws1           */
      *ws1 = NULL;
      return(ws1);
   }

   while (*ws++ = *ws2++)		/* concat the strings                */
      ;

   return(ws1);
}
