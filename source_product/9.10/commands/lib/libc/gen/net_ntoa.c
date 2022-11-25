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
 *	This function converts a binary network station address into
 *      its hex ASCII equivalent.
 */
#ifdef _NAMESPACE_CLEAN
#undef net_ntoa
#pragma _HP_SECONDARY_DEF _net_ntoa net_ntoa
#define net_ntoa _net_ntoa
#endif /* _NAMESPACE_CLEAN */

char *
net_ntoa(dstr, sstr, size)
char *dstr;
char *sstr;
int size;
{
    static char HEXCHAR[] = "0123456789ABCDEF";
    int i;
    char *save_dstr;

    if (size > MAX_ADDR_SIZE || size < MIN_ADDR_SIZE)
	return NULL;

    /*
     * setup the beginning of the string
     */
    save_dstr = dstr;
    *dstr++ = '0';
    *dstr++ = 'x';

    /*
     * convert each byte of the address
     */
    for (i = 0; i < size; i++)
    {
	/*
	 * top half of byte
	 */
	*dstr++ = HEXCHAR[(*sstr >> 4) & 0xF];

	/*
	 * bottom half of byte
	 */
	*dstr++ = HEXCHAR[*sstr++ & 0xF];
    }

    /*
     * tack on the NULL before returning
     */
    *dstr = '\0';
    return save_dstr;
}
