/* static char *HPUX_ID = "@(#) wcsrchr.c $Revision: 70.5 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcsrchr
#    pragma _HP_SECONDARY_DEF _wcsrchr wcsrchr
#    define wcsrchr _wcsrchr
#endif

/*****************************************************************************
 *                                                                           *
 *  FIND POSITION OF WIDE CHARACTER FROM END OF STRING                       *
 *                                                                           *
 *  INPUTS:      ws            - the string to search                        *
 *                                                                           *
 *               wc            - the character to search for                 *
 *                                                                           *
 *  OUTPUTS:     a pointer to the position of the last occurrence of wc in   *
 *               ws, or NULL if not found.                                   *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws is NULL, return NULL since we can't search a null     *
 *               string.  Otherwise, loop though ws keeping track of the     *
 *               last wc we've seen.  Keep looping even when we find one.    *
 *               At the end, return the position of the last one we saw      *
 *               (which may be NULL if we didn't see one).                   *
 *                                                                           *
 *  NOTES:       none.                                                       *
 *                                                                           *
 *  DEFECTS:     						     	     *
 *		 DSDe408075						     *
 *		 Fixed problem where we didn't allow for searching for L'\0' *
 *		 Jim Stratton, 11 Sep 92				     *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

wchar_t *
wcsrchr(const wchar_t *ws, wint_t wc)

{
   wchar_t *char_pos = NULL;            /* to store position of character    */

   if (ws == NULL)
      return(NULL);

   while (*ws) {                        /* loop through string, and...       */
      if (*ws == wc)
	 char_pos = (wchar_t *)ws;      /* save position for later return    */
      ws++;
   }

   if (wc == L'\0')	/* If searching for L'\0', return ptr to that. */
      char_pos = (wchar_t *)ws;

   return(char_pos);                    /* return position of match, if any  */
}
