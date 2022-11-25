/* $Header: emalloc.c,v 1.2.109.2 94/10/28 17:25:05 mike Exp $
 * emalloc - return new memory obtained from the system.  Belch if none.
 */
# include <syslog.h>
# include "ntp_stdlib.h"

char   *
emalloc (size)
unsigned int    size;
{
    char   *mem;

    if ((mem = malloc (size)) == 0)
        {
	syslog (LOG_ERR, "No more memory!");
	exit (1);
        }
    return mem;
}
