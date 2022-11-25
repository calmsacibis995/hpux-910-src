/* $Header: warn1c.c,v 4.1 85/08/14 15:52:57 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * warn1c() places a warning message, with parameters, on the standard
 * error output stream stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nwarn;	/*counter for warnings                          */

warn1c(fmt, arg1)
char *fmt, arg1;
{       nwarn++;
        fprintf(stderr,"%s warning: ",cmdid);
        fprintf(stderr, fmt, arg1);
        putc('\n',stderr);
}
