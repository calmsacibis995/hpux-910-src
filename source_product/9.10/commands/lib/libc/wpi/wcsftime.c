/* static char *HPUX_ID = "@(#) wcsftime.c $Revision: 70.4 $"; */

/*LINTLIBRARY*/


#include <nl_ctype.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcsftime
#    pragma _HP_SECONDARY_DEF _wcsftime wcsftime
#    define wcsftime _wcsftime

#    define strftime _strftime
#    define mbstowcs _mbstowcs
#endif

/*****************************************************************************
 *                                                                           *
 *  FORMAT DATE/TIME AS WIDE STRING                                          *
 *                                                                           *
 *  INPUTS:      ws            - the destination string                      *
 *                                                                           *
 *               maxsize       - maximum length of ws                        *
 *                                                                           *
 *		 format        - date/time formatting info                   *
 *                                                                           *
 *               timptr        - date/time struct 			     *
 *                                                                           *
 *  OUTPUTS:     the original pointer ws1                                    *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

size_t wcsftime(wchar_t *ws, size_t maxsize, const char *format,
		const struct tm *timptr)

{
   size_t cvtlen;
   char *workspace;

   if (ws == NULL)
      return 0;

   /* Allocate the maximum possible space that strftime() will need */
   if ((workspace = (char *)malloc((maxsize*MB_CUR_MAX)+1)) == NULL) {
      /* can't allocate memory. */
      /* return 0 (errno has been set by malloc) */
      return 0;
      }

   /* Call strftime() then convert result */
   if ((cvtlen = strftime(workspace, maxsize, format, timptr)) == 0) {
      /* failed for some reason. */
      /* return 0 (errno may have been set by strftime if appropriate) */
      free(workspace);
      return 0;
      }

   if ((cvtlen = mbstowcs(ws, workspace, cvtlen+1)) == (size_t)-1) {
      /* failed for some reason. */
      /* return 0 (errno may have been set by mbstowcs if appropriate) */
      free(workspace);
      return 0;
      }

   /* Free space and return */
   free(workspace);
   return cvtlen;

}
