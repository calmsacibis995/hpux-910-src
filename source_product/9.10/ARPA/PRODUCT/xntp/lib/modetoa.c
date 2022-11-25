/* $Header: modetoa.c,v 1.2.109.2 94/10/28 17:41:41 mike Exp $
 * modetoa - return an asciized mode
 */
# include <stdio.h>

# include "lib_strbuf.h"
# include "ntp_stdlib.h"

char   *
modetoa (mode)
int     mode;
{
    char   *bp;
    static char    *modestrings[] =
        {
	"unspec",
	    "sym_active",
	    "sym_passive",
	    "client",
	    "server",
	    "broadcast",
	    "control",
	    "private",
	    "bclient",
        };

    if (mode < 0 || mode >= (sizeof modestrings) / sizeof (char    *))
        {
	LIB_GETBUF (bp);
	(void)sprintf (bp, "mode#%d", mode);
	return bp;
        }

return modestrings[mode];
}
