static char *HPUX_ID = "@(#) $Revision: 66.1 $";
/*
 * rmnl -- remove extra new-line characters from input
 */
#include <stdio.h>

main()
{
    register int c;

    while ((c = getchar()) != EOF)
    {
	if (c == '\n')
	{
	    putchar('\n');
	    while ((c = getchar()) == '\n')
		continue;
	    if (c == EOF)
		break;
	}
	putchar(c);
    }
    return 0;
}
