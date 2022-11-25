/*
 * ultostr -- convert an unsigned number to a string.  The string is in
 *            a static buffer that is changed upon each call.  The base
 *            must be between 2 and 36.
 *
 * NOTE:      This routine must leave at least one free byte before the
 *            value it returns for use by ltostr().
 */
#include <values.h>
#include <sys/errno.h>
extern int errno;

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _ultostr ultostr
#define ultostr _ultostr
#endif /* _NAMESPACE_CLEAN */

char *
ultostr(n, base)
register unsigned long n;
register unsigned long base;
{
    static char buff[BITS(long) + 2];
    register char *p = buff + BITS(long)+1;

    if (base < 2 || base > 36)
    {
	errno = ERANGE;
	return (char *)0;
    }

    buff[BITS(long)+1] = '\0';

    do
    {
	*--p = "0123456789abcdefghijklmnopqrstuvwxyz"[n % base];
	n    = n / base;
    } while (n);

    return p;
}

#ifdef DEBUG

#include <stdio.h>
main(argc, argv)
int argc;
char *argv[];
{
    puts(ultostr(strtol(argv[1],0,0), strtol(argv[2],0,0)));
}
#endif
