/* $Header: error4s.c,v 4.1 85/08/14 15:49:41 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * error4s() places an error message, with four string parameters, on
 * the standard error output stream; stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nerror;	/*counter for errors                            */

error4s(fmt, arg1, arg2, arg3, arg4)
char *fmt, *arg1, *arg2, *arg3, *arg4;
{       nerror++;
        fprintf(stderr,"%s error: ",cmdid);
        fprintf(stderr, fmt, arg1, arg2, arg3, arg4);
        putc('\n',stderr);
}
