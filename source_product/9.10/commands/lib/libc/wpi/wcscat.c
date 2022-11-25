/* static char *HPUX_ID = "@(#) wcscat.c $Revision: 70.4 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef  wcscat
#    pragma _HP_SECONDARY_DEF _wcscat wcscat
#    define wcscat _wcscat
#endif

/*****************************************************************************
 *                                                                           *
 *  CONCATENATE WIDE CHARACTER STRINGS                                       *
 *                                                                           *
 *  INPUTS:      ws1           - the destination string                      *
 *                                                                           *
 *               ws2           - the string to append                        *
 *                                                                           *
 *  OUTPUTS:     the original pointer ws1                                    *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Steve Koren                                                 *
 *                                                                           *
 *  DEFECTS:     none known                                                  *
 *                                                                           *
 *  CHANGELOG:   25 Jun 91   - original coding - steve koren                 *
 *                                                                           *
 *****************************************************************************/

wchar_t *wcscat(wchar_t *ws1, const wchar_t *ws2)

{
   wchar_t *ws = ws1;

   if (ws1 == NULL)
      return(NULL);			/* can't pass NULL for ws1           */

   if (ws2 == NULL || *ws2 == NULL)     /* return if nothing to concat       */
      return(ws1);

   while (*ws++)			/* find end of string 1              */
      ;

   ws--;                                /* move back one char to NULL        */

   while (*ws++ = *ws2++)		/* concat the strings                */
      ;

   return(ws1);
}
