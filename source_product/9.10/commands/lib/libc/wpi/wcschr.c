/* static char *HPUX_ID = "@(#) wcschr.c $Revision: 70.6 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcschr
#    pragma _HP_SECONDARY_DEF _wcschr wcschr
#    define wcschr _wcschr
#endif

/*****************************************************************************
 *                                                                           *
 *  FIND POSITION OF CHARACTER IN A WIDE CHARACTER STRING                    *
 *                                                                           *
 *  INPUTS:      ws            - the string to search                        *
 *                                                                           *
 *               wc            - the character to search for                 *
 *                                                                           *
 *  OUTPUTS:     a pointer to the position of wc in ws, or NULL if not       *
 *               found.                                                      *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws is NULL, return NULL since we can't find anything in  *
 *               a NULL string.  Otherwise, keep looping though ws looking   *
 *               for wc.  If we find it, return our current position.  If we *
 *               get to the end without finding it, return NULL.             *
 *                                                                           *
 *  NOTES:       none                                                        *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

wchar_t *wcschr(const wchar_t *ws, wint_t wc)

{
   if (ws == NULL)
      return(NULL);

   while (*ws) {                        /* loop through string, and...       */
      if (*ws == wc)
          return((wchar_t *)ws);	/* return at first match          */
      ws++;
   }

   if (wc == L'\0')	/* If searching for L'\0', return ptr to that. */
      return((wchar_t *)ws);

   return(NULL);			/* No match. */

}
