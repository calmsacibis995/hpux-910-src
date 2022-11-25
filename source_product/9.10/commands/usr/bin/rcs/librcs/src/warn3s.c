/* $Header: warn3s.c,v 4.1 85/08/14 15:53:11 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * warn3s() places a warning message, with parameters, on the standard
 * error output stream stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nwarn;	/*counter for warnings                          */

warn3s(fmt, arg1, arg2, arg3)
char *fmt, *arg1, *arg2, *arg3;
{       nwarn++;
        fprintf(stderr,"%s warning: ",cmdid);
        fprintf(stderr, fmt, arg1, arg2, arg3);
        putc('\n',stderr);
}
