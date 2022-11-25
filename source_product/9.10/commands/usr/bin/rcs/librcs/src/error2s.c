/* $Header: error2s.c,v 4.1 85/08/14 15:49:37 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * error2s() places an error message, with two string parameters, on
 * the standard error output stream; stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nerror;	/*counter for errors                            */

error2s(fmt, arg1, arg2)
char *fmt, *arg1, *arg2;
{       nerror++;
        fprintf(stderr,"%s error: ",cmdid);
        fprintf(stderr, fmt, arg1, arg2);
        putc('\n',stderr);
}
