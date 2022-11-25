/*

    get_next_token()

    Scan the buffer, starting at the given point, searching for a token.

*/

#include "fizz.h"

void get_next_token(ptr, tk_start, tk_end)
register char *ptr; 
char **tk_start, **tk_end;
{
    register int c;

    while (isspace(c = *ptr++));	/* skip over whitespace */
    if (c) {				/* end-of-line? */
	*tk_start = ptr - 1;		/* beginning of token */
        while ((c = *ptr++) && !isspace(c)); /* find end-of-line, whitespace */
	*tk_end = ptr - 1;
    } else *tk_start = (char *) 0;
    return;
}
