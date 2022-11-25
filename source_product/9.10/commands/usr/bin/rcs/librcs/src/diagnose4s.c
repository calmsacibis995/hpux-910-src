/* $Header: diagnose4s.c,v 4.1 85/08/14 15:49:21 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * diagnose4s() places a diagnostic message, with four string parameters,
 * on the standard error output stream stderr if the rcs "quiet" flag
 * is not set
 */
#include <stdio.h>

extern int quietflag;

diagnose4s(fmt, arg1, arg2, arg3, arg4)
char *fmt, *arg1, *arg2, *arg3, *arg4;
{
        if (!quietflag) {
                fprintf(stderr, fmt, arg1, arg2, arg3, arg4);
                putc('\n',stderr);
        }
}
