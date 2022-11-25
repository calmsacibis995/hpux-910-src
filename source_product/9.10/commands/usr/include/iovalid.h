/* @(#) $Revision: 70.1 $ */      
#ifndef _IOVALID_INCLUDED /* allows multiple inclusions */
#define _IOVALID_INCLUDED

#ifdef __hp9000s300
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define LBSIZ	5001

int stout = 1;
int errout = 2;

#define START(x)	write(errout, "START  :  ", 10); write(errout, x, strlen(x)); write(errout, ".\n", 2);

#define FINISH(x)	write(errout, "FINISH :  ", 10); write(errout, x, strlen(x)); write(errout, ".\n", 2);

ERROR(s, c)
char *s;
char c;
{

        (void) write(errout, s,strlen(s));
	(void) write(errout, "\t", 1);
	(void) write(errout, &c, 1);
	(void) write(errout, "\n", 1);
}
#endif /* __hp9000s300 */
#endif /* _IOVALID_INCLUDED */
