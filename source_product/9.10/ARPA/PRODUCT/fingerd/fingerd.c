/*
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)fingerd.c	5.5 (Berkeley) 4/2/89";
static char rcsid[] = "@(#)$Header: fingerd.c,v 1.6.109.1 91/11/21 11:49:04 kcs Exp $";
#endif /* not lint */

#include <stdio.h>
#include <syslog.h>
#include "pathnames.h"

main(argc, argv)
int argc;
char **argv;
{
	register FILE *fp;
	register int ch;
	register char *lp;
	int p[2];
#define	ENTRIES	50
	char **ap, *av[ENTRIES + 1], line[1024], *strtok();
#ifdef OPTIONS
        int recursion = 0;              /* no recursion */
	extern int opterr;
#endif /* OPTIONS */

	openlog("fingerd", LOG_PID | LOG_ODELAY, LOG_DAEMON);

#ifdef OPTIONS
	opterr = 0;
        while ((ch = getopt(argc, argv, "r")) != EOF) {
		switch (ch) {
		    case 'r': 
			recursion = 1;        /* allow recursion */
			break;
		    case '?':
			syslog(LOG_ERR, "usage: fingerd [-r]");
			break;
		}
        }
#endif /* OPTIONS */

#ifdef LOGGING					/* unused for now */
#include <netinet/in.h>
	struct sockaddr_in sin;
	int sval;

	sval = sizeof(sin);
	if (getpeername(0, &sin, &sval) < 0)
		fatal("getpeername");
#endif

	if (!fgets(line, sizeof(line), stdin))
		exit(1);

	av[0] = "finger";
	for (lp = line, ap = &av[1];;) {
		*ap = strtok(lp, " \t\r\n");
		if (!*ap)
			break;
#ifdef OPTIONS
		if (!recursion && strchr(*ap, '@')) {
			printf("Remote finger not allowed: %s\r\n", *ap);
			exit(1);
		}
#endif /* OPTIONS */
		/* RFC742: "/[Ww]" == "-l" */
		if ((*ap)[0] == '/' && ((*ap)[1] == 'W' || (*ap)[1] == 'w'))
			*ap = "-l";
		if (++ap == av + ENTRIES)
			break;
		lp = NULL;
	}

	if (pipe(p) < 0)
		fatal("pipe");

	switch(fork()) {
	case 0:
		(void)close(p[0]);
		if (p[1] != 1) {
			(void)dup2(p[1], 1);
			(void)close(p[1]);
		}
		execv(_PATH_LOCAL_FINGER, av);
		execv(_PATH_FINGER, av);
		syslog(LOG_ERR, "%s: %m", _PATH_FINGER);
		_exit(1);
	case -1:
		fatal("fork");
	}
	(void)close(p[1]);
	if (!(fp = fdopen(p[0], "r")))
		fatal("fdopen");
	while ((ch = getc(fp)) != EOF) {
		if (ch == '\n')
			putchar('\r');
		putchar(ch);
	}
	exit(0);
}

fatal(msg)
	char *msg;
{
	extern int errno;
	char *strerror();

	syslog(LOG_ERR, "fingerd: %s: %m", msg);
	fprintf(stderr, "fingerd: %s: %s\r\n", msg, strerror(errno));
	exit(1);
}
