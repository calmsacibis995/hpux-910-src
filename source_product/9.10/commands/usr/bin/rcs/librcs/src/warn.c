/* $Header: warn.c,v 4.1 85/08/14 15:52:53 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * warn() places a warning message on the standard error output
 * stream stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nwarn;	/*counter for warnings                          */

warn(msg)
char *msg;
{       nwarn++;
        fprintf(stderr,"%s warning: ",cmdid);
        fprintf(stderr, msg);
        putc('\n',stderr);
}
