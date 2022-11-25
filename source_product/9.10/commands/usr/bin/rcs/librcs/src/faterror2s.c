/* $Header: faterror2s.c,v 4.1 85/08/14 15:50:21 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * faterror2s() places an error message, with parameters, on the
 * standard error output stream stderr and terminates the process
 * by calling exit(127) after closing and/or deleting partially updated
 * files by calling cleanup()
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nerror;	/*counter for errors                            */

faterror2s(fmt, arg1, arg2)
char *fmt, *arg1, *arg2;
{       nerror++;
        fprintf(stderr,"%s error: ",cmdid);
        fprintf(stderr, fmt, arg1, arg2);
        fprintf(stderr,"\n%s aborted\n",cmdid);
        cleanup();
        exit(127);
}
