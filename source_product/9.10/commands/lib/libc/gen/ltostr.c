/*
 * ltostr -- convert a number to a string.  The string is in a static
 *           buffer that is changed upon each call.  The base must be
 *           between 2 and 36.
 *
 *           We just call ultostr() and then prepend a '-' if the
 *           number was negative.
 * NOTE:     ultostr() always leaves us a byte for this purpose.
 */
#include <values.h>

#ifdef _NAMESPACE_CLEAN
#define ultostr _ultostr
#pragma _HP_SECONDARY_DEF _ltostr ltostr
#define ltostr _ltostr
#endif /* _NAMESPACE_CLEAN */

char *
ltostr(n, base)
unsigned long n;
unsigned long base;
{
    extern char *ultostr();


    if ((n & HIBITL) == HIBITL) /* negative number */
    {
	char *p;

	if ((p = ultostr((unsigned long)(-n), base)) == (char *)0)
	    return (char *)0;
	*--p = '-';
	return p;
    }
    return ultostr(n, base);
}

#ifdef DEBUG

#include <stdio.h>
main(argc, argv)
int argc;
char *argv[];
{
    puts(ltostr(strtol(argv[1],0,0), strtol(argv[2],0,0)));
}
#endif
