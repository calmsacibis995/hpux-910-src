/* @(#) $Revision: 64.4 $ */    
#ifdef _NAMESPACE_CLEAN
#define strstr _strstr
#endif

#include <string.h>
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

#define Repeat1	loop_1:
#define Repeat2	loop_2:
#define Continue1	goto loop_1
#define Continue2	goto loop_2

#ifdef _NAMESPACE_CLEAN
#undef strstr
#pragma _HP_SECONDARY_DEF _strstr strstr
#define strstr _strstr
#endif

char *
strstr(s1,s2)		/* find s2 in s1 */
char *s1, *s2;
{
	register char *t1, *t2;

	Repeat1 {
		t1 = s1; t2 = s2;
		Repeat2 {
			if ( *t2 == '\0' ) return (s1);
			if ( *t1 != *t2 ) {
				if ( *t1 == '\0' ) return(NULL);
				s1++;
				Continue1;
				}
			t1++; t2++;
			Continue2;
		}
	}
}
