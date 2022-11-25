/* $Header: warn2s.c,v 1.1 86/01/13 12:56:29 bob Rel $ */

/*
 * warn2s() places a warning message, with parameters, on the standard
 * error output stream stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nwarn;	/*counter for warnings                          */

warn2s(fmt, arg1, arg2)
char *fmt, *arg1, *arg2;
{       nwarn++;
        fprintf(stderr,"%s warning: ",cmdid);
        fprintf(stderr, fmt, arg1, arg2);
        putc('\n',stderr);
}
