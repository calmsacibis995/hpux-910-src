/* static char *HPUX_ID = "@(#) wcsncmp.c $Revision: 70.5 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcsncmp
#    pragma _HP_SECONDARY_DEF _wcsncmp wcsncmp
#    define wcsncmp _wcsncmp
#endif

/*****************************************************************************
 *                                                                           *
 *  COMPARE TWO WIDE CHARACTER STRINGS LIMITED BY CHARACTER COUNT            *
 *                                                                           *
 *  INPUTS:      ws1           - first string to compare                     *
 *                                                                           *
 *               ws2           - second string to compare                    *
 *                                                                           *
 *               n             - limiting character count                    *
 *                                                                           *
 *  OUTPUTS:     0 if the strings are equal up to nth char                   *
 *               otherwise *ws1-*ws2 at the first unequal wchar.             *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws1 and ws2 point to the same place, we know the strings *
 *               are equal and don't need to check: just return 0.  If ws1   *
 *               is NULL, return - the first char of ws2.  If ws2 is null,   *
 *               return the first char of ws1.  Otherwise, loop though the   *
 *               string until we find a differing char, and return the       *
 *               difference.  If we get to the end of the string or the max  *
 *               char count with no differences, return 0.                   *
 *                                                                           *
 *  NOTES:       none                                                        *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

int wcsncmp(const wchar_t *ws1, const wchar_t *ws2, size_t n)

{
   int pos = 0;

   if (ws1 == ws2 || n == 0)            /* if these are the same string or   */
      return(0);                        /* n=0, we know they're identical    */

   if (ws1 == NULL)                     /* if s1 is NULL, we know what to do */
      return((int)(-(int)*ws2));

   if (ws2 == NULL)                     /* if s2 is NULL, we know what to do */
      return((int)*ws1);

   while ((*ws1 || *ws2) && pos++ < n) { /* compare each wchar of string     */
                                         /* but not more than n chars total  */
      if (*ws1 != *ws2)
	 return((int)(*ws1 - *ws2));
      ws1++;
      ws2++;
   }

   return(0);                           /* if we get this far, the are equal */
}
