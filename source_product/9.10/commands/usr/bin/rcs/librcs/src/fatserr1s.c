/* $Header: fatserr1s.c,v 4.1 85/08/14 15:50:25 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * fatserror1s() places a syntax error message, with parameters, on the
 * standard error output stream stderr and terminates the process by
 * calling exit(127) after closing and/or deleting partially updated files
 * by calling cleanup()
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int line;	/*current line-number of input                  */
extern int nerror;	/*counter for errors                            */

fatserror1s(fmt, arg)
char *fmt, *arg;
{       nerror++;
        fprintf(stderr,"%s error, line %d: ", cmdid,line);
        fprintf(stderr, fmt, arg);
        fprintf(stderr,"\n%s aborted\n",cmdid);
        cleanup();
        exit(127);
}
