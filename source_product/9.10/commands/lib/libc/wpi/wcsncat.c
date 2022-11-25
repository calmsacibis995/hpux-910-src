/* static char *HPUX_ID = "@(#) wcsncat.c $Revision: 70.4 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcsncat
#    pragma _HP_SECONDARY_DEF _wcsncat wcsncat
#    define wcsncat _wcsncat
#endif

/*****************************************************************************
 *                                                                           *
 *  COPY ONE WIDE CHARACTER STRING TO ANOTHER LIMITED BY COUNT               *
 *                                                                           *
 *  INPUTS:      ws1           - the destination buffer                      *
 *                                                                           *
 *               ws2           - the string to copy to ws1                   *
 *                                                                           *
 *               n             - maximum wchar count to transfer             *
 *                                                                           *
 *  OUTPUTS:     the original pointer ws1.                                   *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws1 is NULL, return NULL since we can't copy anything to *
 *               *NULL.  If ws2 is NULL, write a NULL char to the first char *
 *               of ws1 and return.  Otherwise, proceed to copy ws2 to ws1   *
 *               and return ws1, stopping if we exceed the character count   *
 *               'n'.                                                        *
 *                                                                           *
 *  NOTES:       The ws1 buffer must be large enough to store the result.    *
 *               No size checking is performed.                              *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

wchar_t *wcsncat(wchar_t *ws1, const wchar_t *ws2, size_t n)

{
   int     pos = 0;
   wchar_t *ws;

   ws = ws1;                            /* save position of ws1              */

   if (ws1 == NULL)
      return(NULL);			/* can't pass NULL for ws1           */

   if (ws2 == NULL || *ws2 == NULL || n==0)
      return(ws1);			/* return if nothing to concat       */

   while (*ws++)			/* find end of string 1              */
      ;

   ws--;                                /* backup one wchar_t                */

   while ((*ws++ = *ws2++) && ++pos < n) /* concat the strings              */
                                        /* but don't add more than n chars   */
      ;

   ws[0] = NULL;                        /* make sure its NULL terminated     */

   return(ws1);
}
