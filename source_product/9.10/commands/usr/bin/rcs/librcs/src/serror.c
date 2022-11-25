/* $Header: serror.c,v 4.1 85/08/14 15:52:43 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * serror() places a syntax error message on the standard error output
 * stream stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int line;	/*current line-number of input                  */
extern int nerror;	/*counter for errors                            */

serror(msg)
char *msg;
{       nerror++;
        fprintf(stderr,"%s error, line %d: %s\n", cmdid, line, msg);
}
