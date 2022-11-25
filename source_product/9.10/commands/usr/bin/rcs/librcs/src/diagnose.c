/* $Header: diagnose.c,v 4.1 85/08/14 15:49:09 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * diagnose() places a diagnostic message on the standard error output
 * stream stderr if the rcs "quiet" flag is not set
 */
#include <stdio.h>

extern int quietflag;

diagnose(msg)
char *msg;
{
        if (!quietflag) {
                fprintf(stderr, msg);
                putc('\n',stderr);
        }
}
