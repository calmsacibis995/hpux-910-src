/* static char *HPUX_ID = "@(#) wcswidth.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <sys/stdsyms.h>
#include <errno.h>
#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcswidth
#    pragma _HP_SECONDARY_DEF _wcswidth wcswidth
#    define wcswidth _wcswidth

#    define wcwidth _wcwidth
#endif

/*****************************************************************************
 *                                                                           *
 *  FIND NUMBER OF COLUMN POSITIONS IN A WIDE STRING                         *
 *                                                                           *
 *  INPUTS:      ws            - the string to check                         *
 *                                                                           *
 *               n             - max number of characters to examine         *
 *                                                                           *
 *  OUTPUTS:     number of column positions                                  *
 *               -1 on error                                                 *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

int wcswidth(const wchar_t *ws, size_t n)

{

   int i, count = 0;

   if (ws == NULL) {
      errno = EINVAL;
      return -1;
      }

   while (*ws && n-- > 0) {             /* loop through string, adding up */
      if ((i = wcwidth(*ws)) == -1)	/* individual char widths */
	 return -1;
      count += i;
      ws++;
      }

   return count;

}
