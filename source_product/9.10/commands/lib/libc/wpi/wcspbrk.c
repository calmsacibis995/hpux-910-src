/* static char *HPUX_ID = "@(#) wcspbrk.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#   undef wcspbrk
#   pragma _HP_SECONDARY_DEF _wcspbrk wcspbrk
#   define wcspbrk _wcspbrk
#endif

/*
 * Return ptr to first occurance of any character from `brkset'
 * in the character string `string'; NULL if none exists.
 */

/*****************************************************************************
 *                                                                           *
 *  SCAN WIDE STRING FOR WIDE CHARACTER                                      *
 *                                                                           *
 *  INPUTS:      string        - the string to scan                          *
 *                                                                           *
 *               brkset        - the set of wide characters to scan for      *
 *                                                                           *
 *  OUTPUTS:     the wide character from brkset that was found               *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

wchar_t *
wcspbrk(const wchar_t *string, const wchar_t *brkset)
{
	wchar_t *p;

	if (string == NULL || brkset == NULL)
		return NULL;
	
	do {
		for(p=(wchar_t *)brkset; *p != L'\0' && *p != *string; ++p)
			;
		if(*p != L'\0')
			return (wchar_t *)string;
	} while(*string++);

	return NULL;
}
