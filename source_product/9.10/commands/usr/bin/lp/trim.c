/* @(#) $Revision: 62.1 $ */    
/* trim -- trim trailing blanks from character string */

#include	"lp.h"

char *
trim(s)
char *s;
{
	int len;

	if((len = strlen(s)) != 0) {
		while(s[--len] == ' ')
			;
		s[++len] = '\0';
	}

	return(s);
}
