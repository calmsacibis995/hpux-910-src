/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)logger.c	6.2 (Berkeley) 9/19/85";
#endif not lint

#include <stdio.h>
#include <syslog.h>
#include <ctype.h>
#include <nl_types.h>

#ifndef NLS
#   define catgets(i, sn,mn,s) (s)
#else /* NLS */
#   define NL_SETN 1	/* set number */
#   include <nl_types.h>

#define USAGE "Usage: logger [ -t tag ] [ -p pri ] [ -i ] [ -f file ] [message ...]\n"	/* catgets 1 */
#define FACIL "logger: unknown facility name: %s\n"	/* catgets 2 */
#define PRIO  "logger: unknown priority name: %s\n"	/* catgets 3 */

nl_catd	catd;
  
#endif /* NLS */

#ifdef hpux
#define MAXHOSTNAMELEN 64
#define index   strchr
#define bzero(a,b)      memset(a,0,b)
#define bcopy(a,b,c)    memcpy(b,a,c)
#define strcpyn         strncpy
#define sigmask(x)      (1 << (x - 1))
#endif

/*
**  LOGGER -- read and log utility
**
**	This routine reads from an input and arranges to write the
**	result on the system log, along with a useful tag.
*/

main(argc, argv)
	int argc;
	char **argv;
{
	char buf[1024];
	char cookedbuf[2048];
	char *bufp;
	char *cookedbufp;
	register char *p;
	char *tag;
	int pri = LOG_NOTICE;
	int logflags = 0;
	extern char *getlogin();
	extern char *optarg;
	extern int optind;
	int c, errflg = 0;

	/* initialize */
	tag = getlogin();
#ifdef NLS
	catd = catopen("logger",0);
#endif

	/* crack arguments */
	while ((c = getopt(argc, argv, "t:p:if:")) != EOF)
		switch (c)
		{
		  case 't':		/* tag */
			tag = optarg;
			break;

		  case 'p':		/* priority */
			pri = pencode(optarg);
			break;

		  case 'i':		/* log process id also */
			logflags |= LOG_PID;
			break;

		  case 'f':		/* file to log */
			if (freopen(optarg, "r", stdin) == NULL)
			{
				fputs("logger: ", stderr);
				perror(*argv);
				exit(1);
			}
			break;

		  case '?':
			errflg++;
			break;
		}

	if (errflg) {
		fputs(catgets(catd,NL_SETN,1,USAGE), stderr);
		exit(1);
	}

	/* setup for logging */
	openlog(tag, logflags, 0);
	(void) fclose(stdout);

	/* log input line if appropriate */
	if (optind < argc)
	{
		bufp = buf;
		while (optind < argc)
		{
			*bufp++ = ' ';
			for (p = argv[optind++]; *p != NULL; *bufp++ = *p++) {
			   /*
			    * Need to double up the percent signs so that
			    * syslog will not get confused.
			    */
			   if (*p == '%') *bufp++ = '%';
			}
			*bufp = '\0';
		}
		syslog(pri, buf + 1);
		exit(0);
	}

	/* main loop */
	while (fgets(buf, sizeof buf, stdin) != NULL) {
		for (bufp = buf, cookedbufp = cookedbuf;
		     *bufp != NULL; *cookedbufp++ = *bufp++) {
			   /*
			    * Need to double up the percent signs so that
			    * syslog will not get confused.
			    */
			   if (*bufp == '%') *cookedbufp++ = '%';
		}
		*cookedbufp = '\0';
		syslog(pri, cookedbuf);
	}

	exit(0);
}


struct code {
	char	*c_name;
	int	c_val;
};

struct code	PriNames[] = {
	"panic",	LOG_EMERG,
	"emerg",	LOG_EMERG,
	"alert",	LOG_ALERT,
	"crit",		LOG_CRIT,
	"err",		LOG_ERR,
	"error",	LOG_ERR,
	"warn",		LOG_WARNING,
	"warning",	LOG_WARNING,
	"notice",	LOG_NOTICE,
	"info",		LOG_INFO,
	"debug",	LOG_DEBUG,
	NULL,		-1
};

struct code	FacNames[] = {
	"kern",		LOG_KERN,
	"user",		LOG_USER,
	"mail",		LOG_MAIL,
	"daemon",	LOG_DAEMON,
	"auth",		LOG_AUTH,
	"security",	LOG_AUTH,
	"lpr",		LOG_LPR,
	"lp",		LOG_LPR,
	"local0",	LOG_LOCAL0,
	"local1",	LOG_LOCAL1,
	"local2",	LOG_LOCAL2,
	"local3",	LOG_LOCAL3,
	"local4",	LOG_LOCAL4,
	"local5",	LOG_LOCAL5,
	"local6",	LOG_LOCAL6,
	"local7",	LOG_LOCAL7,
	NULL,		-1
};


/*
 *  Decode a symbolic name to a numeric value
 */

pencode(s)
	register char *s;
{
	register char *p;
	int lev;
	int fac;
	char buf[100];

	for (p = buf; *s && *s != '.'; )
		*p++ = *s++;
	*p = '\0';
	if (*s++) {
	    fac = decode(buf, FacNames);
	    if (fac < 0)
	    {
		fprintf (stderr, catgets(catd,NL_SETN,2,FACIL), buf);
		exit (1);
	    }
	    
	    for (p = buf; *p++ = *s++; )
		continue;
	} else
	    fac = 0;
	lev = decode(buf, PriNames);
	if (lev < 0)
	{
	    fprintf (stderr, catgets(catd,NL_SETN,3,PRIO), buf);
	    exit(1);
	}
	
	return ((lev & LOG_PRIMASK) | (fac & LOG_FACMASK));
}


decode(name, codetab)
	char *name;
	struct code *codetab;
{
	register struct code *c;
	register char *p;
	char buf[40];

	if (isdigit(*name))
	    return (atoi(name));

	(void) strcpy(buf, name);
	for (p = buf; *p; p++)
		if (isupper(*p))
			*p = tolower(*p);
	for (c = codetab; c->c_name; c++)
		if (!strcmp(buf, c->c_name))
			return (c->c_val);

	return (-1);
}
