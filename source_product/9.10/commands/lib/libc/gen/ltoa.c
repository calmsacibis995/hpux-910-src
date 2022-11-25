/*
 * ltoa -- convert a number to a string (base 10).  The string is in a
 *         static buffer that is changed upon each call.
 */
#include <values.h>

/*
 * MAXLEN defines the number of characters we need to represent a long
 * in base 10.  This is roughly BITS(long)/3.  For some possible sizes
 * of a long integer.  This is actually a high estimate, for example a
 * long integer that is 128 bits long only requires 39 bytes in base
 * 10 (not the 42 that MAXLEN would be).
 *
 *  16, MAXLEN is 5                                               65,535
 *  32, MAXLEN is 10                                       4,294,967,295
 *  64, MAXLEN is 20                          18,446,744,065,119,617,025
 * 128, MAXLEN is 42 340,282,366,604,025,813,516,997,721,482,669,850,625
 */
#define MAXLEN   (BITS(long)/3)

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _ltoa ltoa
#define ltoa _ltoa
#endif /* _NAMESPACE_CLEAN */

char *
ltoa(n)
register unsigned long n;
{
    static char buff[MAXLEN+2];
    register char *p = buff + MAXLEN+1;
    register char sign;

    buff[MAXLEN+1] = '\0';
    if ((n & HIBITL) == HIBITL) /* negative number */
    {
	n = (unsigned long)(-n);
	sign = 1;
    }
    else
	sign = 0;

    do
    {
	*--p = (char)((n % 10) + '0');
	n /= 10;
    } while (n);

    if (sign)
	*--p = '-';

    return p;
}

#ifdef DEBUG

#include <stdio.h>
main(argc, argv)
int argc;
char *argv[];
{
    puts(ltoa(strtol(argv[1],0,0)));
}
#endif
