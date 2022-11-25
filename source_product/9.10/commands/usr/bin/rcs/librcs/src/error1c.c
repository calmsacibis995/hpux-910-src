/* $Header: error1c.c,v 4.1 85/08/14 15:49:29 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * error1c() places an error message, with one character parameter, on
 * the standard error output stream; stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nerror;	/*counter for errors                            */

error1c(fmt, arg)
char *fmt, arg;
{       nerror++;
        fprintf(stderr,"%s error: ",cmdid);
        fprintf(stderr, fmt, arg);
        putc('\n',stderr);
}
