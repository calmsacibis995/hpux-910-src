/* $Header: diagnose2s.c,v 4.1 85/08/14 15:49:17 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * diagnose2s() places a diagnostic message, with two string parameters,
 * on the standard error output stream stderr if the rcs "quiet" flag
 * is not set
 */
#include <stdio.h>

extern int quietflag;

diagnose2s(fmt, arg1, arg2)
char *fmt, *arg1, *arg2;
{
        if (!quietflag) {
                fprintf(stderr, fmt, arg1, arg2);
                putc('\n',stderr);
        }
}
