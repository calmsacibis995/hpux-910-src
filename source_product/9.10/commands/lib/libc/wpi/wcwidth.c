/* static char *HPUX_ID = "@(#) wcwidth.c $Revision: 70.5 $"; */

/*LINTLIBRARY*/


#include <nl_ctype.h>
#include <wchar.h>
#include <wpi.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcwidth
#    pragma _HP_SECONDARY_DEF _wcwidth wcwidth
#    define wcwidth _wcwidth
#endif

/*****************************************************************************
 *                                                                           *
 *  DETERMINE THE NUMBER OF COLUMN POSITIONS A WIDE CHARACTER IS             *
 *                                                                           *
 *  INPUTS:      wc            - the character to examine                    *
 *                                                                           *
 *  OUTPUTS:     the number of column positions                              *
 *               -1 on error   						     *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

int wcwidth(wint_t wc)

{

   /* Null character? */
   if (wc == L'\0')
      return 0;

   /* Check for invalid wide characters */
   if ((__cs_SBYTE && (wc & 0xffffff00)) || (wc & 0xffff0000))
      return -1;

   /* Must be printable */
   if (!iswprint(wc))
      return -1;

   return WC_COLWIDTH(wc);

}
