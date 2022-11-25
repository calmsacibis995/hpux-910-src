/* $Header: faterror.c,v 4.1 85/08/14 15:50:00 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * faterror() places an error message on the standard error output
 * stream stderr and terminates the process by calling exit(127) after
 * closing and/or deleting partially updated files by calling cleanup()
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nerror;	/*counter for errors                            */

faterror(msg)
char *msg;
{       nerror++;
        fprintf(stderr,"%s error: ",cmdid);
        fprintf(stderr, msg);
        fprintf(stderr,"\n%s aborted\n",cmdid);
        cleanup();
        exit(127);
}
