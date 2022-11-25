/* static char *HPUX_ID = "@(#) wcscspn.c $Revision: 70.4 $"; */

/*LINTLIBRARY*/


#include <nl_ctype.h>
#include <wchar.h>

#   ifndef TRUE
#      define	TRUE	1
#   endif
#   ifndef FALSE
#      define	FALSE	0
#   endif

#ifdef _NAMESPACE_CLEAN
#    undef wcscspn
#    pragma _HP_SECONDARY_DEF _wcscspn wcscspn
#    define wcscspn _wcscspn

#    define wcslen _wcslen
#endif

/*****************************************************************************
 *                                                                           *
 *  FIND LENGTH OF INITIAL SEGMENT OF WIDE CHARACTER STRING                  *
 *                                                                           *
 *  INPUTS:      ws1           - the string to search                        *
 *                                                                           *
 *               ws2           - the string containing characters for the    *
 *                               initial segment of ws1                      *
 *                                                                           *
 *  OUTPUTS:     the length of the initial substring of ws1 which contains   *
 *               characters NOT found in the string ws2.                     *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws1 or ws2 is NULL, return 0, since we can't look for    *
 *               substrings.   Otherwise, loop though ws1 character by       *
 *               character.  For each position, see if we can find the wchar *
 *               in ws2.  If not, increment the count and go on.  If so,     *
 *               return the count.                                           *
 *                                                                           *
 *  NOTES:       none                                                        *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

size_t wcscspn(const wchar_t *ws1, const wchar_t *ws2)

{
   wchar_t *ws;
   size_t  size = 0;
   int     found;

   if (ws1 == NULL || *ws1 == NULL)     /* return 0 if null first string     */
      return(0);

   if (ws2 == NULL || *ws2 == NULL)     /* return length of null 2nd string  */
      return(wcslen(ws1));

   while (*ws1) {                       /* loop though string ws1            */
      ws = (wchar_t *)ws2;
      found = FALSE;
      for ( ; *ws && !found; ws++)
         if (*ws1 == *ws)
	    found = TRUE;		/* see if we found the character     */

      if (found)
	 return(size);			/* if char is found, return count    */
      size++;                           /* else increment count and proceed  */
      ws1++;
   }

   return(size);                        /* if here, we reached the end of the */
                                        /* string                             */
}
