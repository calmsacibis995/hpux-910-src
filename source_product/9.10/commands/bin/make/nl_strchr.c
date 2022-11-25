/*
 * This file contains the two alternate string comparison routines
 * usually found in string(LIB).  
 */

#define NULL 0

#include <nl_ctype.h>

char *
nl_strchr(sp, c)
register char *sp, c;
{

	if (sp == NULL)
		return(NULL);
	do {
		if(*sp == c)
			return(sp);
	} while(CHARADV(sp));
	return(NULL);
}



/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
*/
#define CNULL  (char *)0

char *
nl_strrchr(sp, c)
register char *sp, c;
{
	register char *r;

	if (sp == CNULL)
		return(CNULL);

	r = CNULL;
	do {
		if(*sp == c)
			r = sp;
	} while(CHARADV(sp));
	return(r);
}
