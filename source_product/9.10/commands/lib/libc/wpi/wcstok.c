/* static char *HPUX_ID = "@(#) wcstok.c $Revision: 70.2 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcstok
#    pragma _HP_SECONDARY_DEF _wcstok wcstok
#    define wcstok _wcstok

#    define wcsspn _wcsspn
#    define wcspbrk _wcspbrk
#endif


/*
 * uses wcspbrk and wcsspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 * `subsequent' calls are calls with first argument NULL.
 */

/*****************************************************************************
 *                                                                           *
 *  SPLIT WIDE STRING INTO TOKENS                                            *
 *                                                                           *
 *  INPUTS:      string        - the string to split                         *
 *                                                                           *
 *               setsep        - set of wide characters that delimit tokens  *
 *                                                                           *
 *  OUTPUTS:     pointer to the first (next) token                           *
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
wcstok(wchar_t *string, const wchar_t *sepset)
{
	wchar_t	*p, *q, *r;
	static wchar_t	*savept;

	/*first or subsequent call*/
	p = (string == NULL) ? savept : string;

	if(p == NULL)		/* return if no tokens remaining */
		return(NULL);

	q = p + wcsspn(p, sepset);	/* skip leading separators */

	if(*q == L'\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = (wchar_t *)wcspbrk(q, sepset)) == NULL)	/* move past token */
		savept = 0;	/* indicate this is last token */
	else {
		*r = L'\0';
		savept = ++r;
	}
	return(q);
}
