/*	@(#) $Revision: 70.3 $	*/
static char *RCS_ID="@(#)$Revision: 70.3 $ $Date: 91/11/13 10:19:19 $";
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1
#include <nl_types.h>
#endif NLS
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	/sccs/src/cmd/uucp/s.uuname.c
	uuname.c	1.1	7/29/85 16:34:11
*/
#include "uucp.h"
VERSION(@(#)uucp:uuname.c	1.1);
 
/*
 * returns a list of all remote systems.
 * option:
 *	-l	-> returns only the local system name.
 */
main(argc,argv, envp)
int argc;
char **argv, **envp;
{
	FILE *np;
	register short lflg = 0;
	char s[BUFSIZ], prev[BUFSIZ], name[BUFSIZ];

      
#ifdef NLS
	nlmsg_fd = catopen("uucp",0);
#endif
	while (*(++argv) && *argv[0] == '-')
		switch(argv[0][1]) {
		case 'l':
			lflg++;
			break;
		default:
			(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,145, "usage: uuname [-l]\n")));
			exit(1);
		}
 
	if (lflg) {
		uucpname(name);

		/* initialize to null string */
		(void) printf("%s",name);
		(void) printf("\n");
		exit(0);
	}
	if ((np=fopen(SYSFILE, "r")) == NULL) {
		(void) sprintf(s, (catgets(nlmsg_fd,NL_SETN,146, "Cannot open file \"%s\"")), SYSFILE);
		perror(s);
		exit(1);
	}
 
	while (fgets(s, BUFSIZ, np) != NULL) {
		if((s[0] == '#') || (s[0] == ' ') || (s[0] == '\t') || 
		    (s[0] == '\n'))
			continue;
		(void) sscanf(s, "%s", name);
		if (EQUALS(name, prev))
		    continue;
		(void) printf("%s", name);
		(void) printf("\n");
		(void) strcpy(prev, name);
	}
#ifdef NLS
	catclose(nlmsg_fd);
#endif
	exit(0);
}
