/* $Header: fatserror.c,v 4.1 85/08/14 15:50:29 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * fatserror() places a syntax error message on the standard error
 * output stream stderr and terminates the process by calling exit(127)
 * after closing and/or deleting partially updated files by calling
 * cleanup()
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int line;	/*current line-number of input                  */
extern int nerror;	/*counter for errors                            */

fatserror(msg)
char *msg;
{       nerror++;
        fprintf(stderr,"%s error, line %d: ", cmdid,line);
        fprintf(stderr,"%s\n%s aborted\n", msg, cmdid);
        cleanup();
        exit(127);
}
