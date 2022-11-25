/* $Header: diagnose1s.c,v 4.1 85/08/14 15:49:13 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * diagnose1s() places a diagnostic message, with one string parameter,
 * on the standard error output stream stderr if the rcs "quiet" flag
 * is not set
 */
#include <stdio.h>

extern int quietflag;

diagnose1s(fmt, arg)
char *fmt, *arg;
{
        if (!quietflag) {
                fprintf(stderr, fmt, arg);
                putc('\n',stderr);
        }
}
