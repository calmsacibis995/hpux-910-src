/* $Header: error.c,v 66.2 89/11/02 16:23:21 ssa Exp $ */

/*
 * extracted from rcslex.c
 */

/*
 * error() places an error message on the standard error output stream
 * stderr
 */
#include <stdio.h>

extern char *cmdid;	/*command identification for error messages     */
extern int nerror;	/*counter for errors                            */

error(msg)
char *msg;
{       nerror++;
        fprintf(stderr,"%s error: %s\n",cmdid, msg);
}
