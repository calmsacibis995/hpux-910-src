/* $Header: serror1s.c,v 4.1 85/08/14 15:52:50 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * serror1s() places a syntax error message, with parameters, on the
 * standard error output stream stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int line;	/*current line-number of input                  */
extern int nerror;	/*counter for errors                            */

serror1s(fmt, arg)
char *fmt, *arg;
{       nerror++;
        fprintf(stderr,"%s error, line %d: ", cmdid, line);
        fprintf(stderr, fmt, arg);
        putc('\n',stderr);
}
