#ifndef lint
static  char rcsid[] = "@(#)stdhosts:	$Revision: 1.19.109.1 $	$Date: 91/11/19 14:20:56 $  ";
#endif
/* stdhosts.c	2.1 86/04/16 NFSSRC */
/*static  char sccsid[] = "stdhosts.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include <stdio.h>
#include <sys/types.h>

/* 
 * Filter to convert addresses in /etc/hosts file to standard form
 */

main(argc, argv)
	char **argv;
{
	char line[256];
	char adr[256];
	char *any(), *trailer;
	FILE *fp;
	u_long	num;
	
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("stdhosts",0);
#endif NLS

	if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "stdhosts: can't open %s\n")), argv[1]);
			exit(1);
		}
	}
	else
		fp = stdin;
	while (fgets(line, sizeof(line), fp)) {
		if (line[0] == '#')
			continue;
		if ((trailer = any(line, " \t")) == NULL)
			continue;
		sscanf(line, "%s", adr);
		if ((num = inet_addr(adr)) == -1)
			continue;
		fputs(inet_ntoa(num), stdout);
		fputs(trailer, stdout);
	}
}

/* 
 * scans cp, looking for a match with any character
 * in match.  Returns pointer to place in cp that matched
 * (or NULL if no match)
 */
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return (NULL);
}
