/* $Header: warn1i.c,v 4.1 85/08/14 15:53:01 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * warn1i() places a warning message, with parameters, on the standard
 * error output stream stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nwarn;	/*counter for warnings                          */

warn1i(fmt, arg1)
char *fmt;
int arg1;
{       nwarn++;
        fprintf(stderr,"%s warning: ",cmdid);
        fprintf(stderr, fmt, arg1);
        putc('\n',stderr);
}
