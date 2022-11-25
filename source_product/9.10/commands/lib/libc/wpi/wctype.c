/* static char *HPUX_ID = "@(#) wctype.c $Revision: 70.5 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wctype
#    pragma _HP_SECONDARY_DEF _wctype wctype
#    define wctype _wctype

#    define strcmp _strcmp
#endif

/*****************************************************************************
 *                                                                           *
 *  MAP CHARACTER CLASS NAME INTO INTERNAL TYPE                              *
 *                                                                           *
 *  INPUTS:      property      - the name of a character class to map        *
 *                                                                           *
 *  OUTPUTS:     the number of the class name                                *
 *               0 on error                                                  *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

#define NUM_CLASSES	12

/* The zeroth entry is unused; a valid wctype_t can't be 0 */
static const char *classes[1+NUM_CLASSES] =
	{ "",
	"alnum", "alpha", "blank", "cntrl", "digit", "graph",
	"lower", "print", "punct", "space", "upper", "xdigit", };

wctype_t wctype(const char *property)

{
   int i;

   for (i=1; i<=NUM_CLASSES; i++)
      if (strcmp(property, classes[i]) == 0)
	 return i;
   return (wctype_t)0;

}
