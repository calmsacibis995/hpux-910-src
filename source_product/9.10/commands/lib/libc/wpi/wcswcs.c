/* static char *HPUX_ID = "@(#) wcswcs.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <wchar.h>

#   ifndef TRUE
#      define	TRUE	1
#   endif
#   ifndef FALSE
#      define	FALSE	0
#   endif

#ifdef _NAMESPACE_CLEAN
#    undef wcswcs
#    pragma _HP_SECONDARY_DEF _wcswcs wcswcs
#    define wcswcs _wcswcs
#endif

/*********** The body of strstr was created from the following model:

	while ( TRUE ) {
		t1 = s1; t2 = s2;
		while ( TRUE ) {
			if ( *t2 == '\0' ) return (s1);
			if ( *t1 != *t2 ) break;
			t1++; t2++;
		}
		if ( *t1 == '\0' ) return(NULL);
		s1++;
	}

***********/

/*****************************************************************************
 *                                                                           *
 *  FIND WIDE STRING IN ANOTHER WIDE STRING                                  *
 *                                                                           *
 *  INPUTS:      s1            - the wide string to search within            *
 *                                                                           *
 *               s2            - the wide string to seach for                *
 *                                                                           *
 *  OUTPUTS:     pointer to the first occurrence of s2                       *
 *               NULL on error.                                              *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

#define Repeat1	loop_1:
#define Repeat2	loop_2:
#define Continue1	goto loop_1
#define Continue2	goto loop_2


wchar_t *
wcswcs(const wchar_t *s1, const wchar_t *s2)		/* find s2 in s1 */
{
	wchar_t *t1, *t2;

	if (s1 == NULL || s2 == NULL)
		return NULL;

	Repeat1 {
		t1 = (wchar_t *)s1; t2 = (wchar_t *)s2;
		Repeat2 {
			if ( *t2 == L'\0' ) return (wchar_t *)s1;
			if ( *t1 != *t2 ) {
				if ( *t1 == L'\0' ) return(NULL);
				s1++;
				Continue1;
				}
			t1++; t2++;
			Continue2;
		}
	}
}
