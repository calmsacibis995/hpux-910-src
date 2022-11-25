/*	@(#)util.c	$Revision: 1.19.109.1 $	$Date: 91/11/19 14:22:44 $  
util.c	2.1 86/04/16 NFSSRC 
static  char sccsid[] = "util.c 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc.";
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
#include <stdio.h>
#include "util.h"


#ifdef NLS
extern nl_catd nlmsg_fd;
#endif NLS


/*
 * This is just like fgets, but recognizes that "\\n" signals a continuation
 * of a line
 */
char *
getline(line,maxlen,fp)
	char *line;
	int maxlen;
	FILE *fp;
{
	register char *p;
	register char *start;


	start = line;

nextline:
	if (fgets(start,maxlen,fp) == NULL) {
		return(NULL);
	}	
	for (p = start; ; p++) {
		if (*p == '\n') {       
			if (*(p-1) == '\\') {
				start = p - 1;
				goto nextline;
			} else {
				return(line);	
			}
		}
	}
}	




	
void
fatal(message)
	char *message;
{
	(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,5, "fatal error: %s\n")),message);
	exit(1);
}
