/*

    get_next_list_item()

    Scan for next list item:
	Whitespace is skipped over.
	The next non-whitespace character is beginning-of-token.
	The next whitespace space character, comma, or end-of-line
	    is end-of-token.
	If end-of-line occurs before the beginning-of-token is found,
	    beginning-of-token is set to 0. 

*/

#include "fizz.h"

void get_next_list_item(ptr, tk_start, tk_end)
register char *ptr;
char **tk_start, **tk_end;
{
    register int c;

    while (isspace(c = *ptr++));	/* skip whitespace */
    if (!c) {				/* end-of-line? */
	*tk_start = (char *) 0;
	return;
    };

    *tk_start = --ptr;

    /* look for whitespace, comma, or end-of-line */
    while ((c = *ptr++) && !isspace(c) && (c != ','));

    *tk_end = --ptr;

    return;
}
