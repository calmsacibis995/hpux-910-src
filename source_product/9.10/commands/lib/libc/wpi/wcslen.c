/* static char *HPUX_ID = "@(#) wcslen.c $Revision: 70.4 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcslen
#    pragma _HP_SECONDARY_DEF _wcslen wcslen
#    define wcslen _wcslen
#endif

/*****************************************************************************
 *                                                                           *
 *  DETERMINE THE LENGTH OF A WIDE CHARACTER STRING                          *
 *                                                                           *
 *  INPUTS:      ws            - the string to examine                       *
 *                                                                           *
 *  OUTPUTS:     the length of the string in wchars                          *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  ALGORITHM:   If ws is NULL, return 0.  Otherwise, loop though and add up *
 *               the number of wchars until the end.  Return this number.    *
 *                                                                           *
 *  NOTES:       none                                                        *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *               26 Jun 91   - change to handle ws being NULL                *
 *                                                                           *
 *****************************************************************************/

size_t wcslen(const wchar_t *ws)

{
   size_t len = 0;

   if (ws == NULL)
      return(0);			/* if NULL string, size is 0         */

   while (*ws++)
      len++;				/* add up the number of chars        */

   return(len);                         /* return what we found              */
}
