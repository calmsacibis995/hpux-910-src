/* @(#) $Revision: 66.2 $ */


#define NULL	      ((char *)0)
#define MAX_ADDR_SIZE 6
#define MIN_ADDR_SIZE 1

/*
 * net_aton(dstr, sstr, size) --
 *   dstr - starting address of the result (destination)
 *   sstr - starting address of the source
 *   size - size of the address
 *
 * Result
 *   dstr - contains the converted address.  The contents of dstr
 *          are undefined if an error occurs.
 *
 * Returns
 *   dstr on success
 *   NULL on failure
 *
 * Overview
 *	This function converts an ASCII network station address into
 *      its binary equivalent.
 */
#ifdef _NAMESPACE_CLEAN
#undef net_aton
#pragma _HP_SECONDARY_DEF _net_aton net_aton
#define net_aton _net_aton
#endif /* _NAMESPACE_CLEAN */

char *
net_aton(dstr, sstr, size)
char *dstr;
char *sstr;
int size;
{
    short base;
    short digit;
    short highi;
    short i;
    short j;
    unsigned char tmp[MAX_ADDR_SIZE];

    if (size > MAX_ADDR_SIZE || size < MIN_ADDR_SIZE)
	return NULL;

    for (i = 0; i < MAX_ADDR_SIZE; i++)
	tmp[i] = '\0';

    /*
     * determine whether the representation is octal, hex, or decimal
     */
    if (*sstr == '0')
    {
	base = 8;
	++sstr;
	if (*sstr == 'x' || *sstr == 'X')
	{
	    base = 16;
	    if (*(++sstr) == '\0')	/* nothing after '0x' */
		return NULL;
	}
    }
    else
	base = 10;

    highi = 0;

    while ((digit = *sstr++) != '\0')
    {
	/*
	 * loop until a null terminator is seen
	 * convert ASCII character to numeric digit
	 */
	if (digit >= '0' && digit <= '9')
	    digit -= '0';
	else if (digit >= 'a' && digit <= 'f')
	    digit += 10 - 'a';
	else if (digit >= 'A' && digit <= 'F')
	    digit += 10 - 'A';
	else
	    return NULL;

	if (digit >= base)
	    return NULL;

	/*
	 * add digit to existing address
	 */
	for (i = 0; i < size && i <= highi; i++)
	{
	    j = tmp[i] * base + digit;
	    if (j > 255)
	    {
		if (i == highi)
		    /* check for overflow */
		    if (++highi == size)
			return NULL;
		digit = j / 256;
		tmp[i] = j & 255;
	    }
	    else
	    {
		digit = 0;
		tmp[i] = j;
	    }
	}
    }

    /*
     * address is complete, but backwards; reverse it
     */
    for (i = 0; i < size; i++)
	dstr[size - i - 1] = tmp[i];

    return dstr;
}
