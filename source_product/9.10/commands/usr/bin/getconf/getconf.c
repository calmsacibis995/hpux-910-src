static char *HPUX_ID = "@(#) $Revision: 70.4 $";

/*
 *   Getconf - get POSIX confuguration values.
 *   The local header file is created  at build time from unistd.h.
 *   The parameters are self-updating, by building with a changed
 *   unistd.h
 *
 *   Current limitations:
 *     Not all of the posix calls are implemented on hp-ux.
 *
 *   AW: 7 Jan 1991
 *   POSIX.2/D11.2 implemented, NLS supported.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/stdsyms.h>
#include <sys/unistd.h>
#include <limits.h>

#if defined(NLS) || defined(NLS16)
#  include <locale.h>
#  include <setlocale.h>
#endif

#include "getconf.h"

#define	PGM_NAME "getconf "      			      /* catgets 3 */
#define USAGE 	 "Usage: getconf parameter_name [pathname]\n" /* catgets 1 */

#if defined(NLS) || defined(NLS16)
#  define	NL_SETN	1		/* set number for messages */
   nl_catd	nlmsg_fd;
#else
#  define	catgets(i, sn, mn, s)	(s)
#endif

int	errno_save;			/* errno preservation across calls */

extern	char	*ltoa();		/* long to ascii */
extern	size_t	confstr();		/* predefine a function */
extern	long	pathconf();		/* predefine a function */

main(argc,argv)
int	argc;
char	*argv[];
{

	int	i;			/* working variables */
	long	v;			/* configuration value */
	size_t	buf_len;		/* used for confstr() */
	char	*buf;			/* used for confstr() */
	char	*str;			/* used to hold argv[1] */

#if defined(NLS) || defined(NLS16)
	if(!setlocale(LC_ALL, "")) {
		fputs(_errlocale("getconf"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)(-1);
	} else
		nlmsg_fd = catopen("getconf", 0);
#endif

	errno = 0;			/* initialize before any calls */
	if (argc <= 1)			/* give usage message */
	{
		Usage();
		exit(2);	/* the variable name is invalid (not given) */
	}

	str = argv[1];

	/* lookup str in the table, select its parameter value and call */
	for (i = 0; parm_table[i].pt_name != NULL; i++)
	{
#ifdef OFF
printf("main:\tstr=%20s, table=%20s\n", str, parm_table[i].pt_name);
#endif /* OFF */
	    if (strcmp(str, parm_table[i].pt_name) == 0)
		switch (parm_table[i].pt_function_type)
		{    /* call the appropriate function for this option */

		    case FT_CONFSTR:
			buf_len = confstr(parm_table[i].pt_code,
					      NULL, (size_t)0);
			buf = (char *)malloc(buf_len);
			if ((confstr(parm_table[i].pt_code,
					buf, buf_len)) != buf_len)
			{
			    if (errno == 0) /* no config- defined value */
			    {
				fprintf(stdout,"%s\n",catgets(nlmsg_fd,
				  NL_SETN, 2, "undefined")); /* catgets 2 */
			        exit(0); /* variable name valid, not defined */
			    }
			    else	/* name is invalid */
			    {
				errno_save = errno;    /* preserve for perror */
				fputs(catgets(nlmsg_fd, NL_SETN, 3, PGM_NAME),
					stderr);
				fputs(argv[1], stderr);
				errno = errno_save;    /* restore */
				perror(argv[2]);
			        exit(3); /* an error occured */
			    }
			}
			fprintf(stdout, "%s\n", buf);
			exit(0);
		    break;

		    case FT_PATHCONF:
			if (argc != 3)	/* missing or extra operands */
			{
			    Usage();
			    exit(1); /* variable name valid, too few arguments*/
			}
			if ((v = pathconf(argv[2], parm_table[i].pt_code))
			    == -1)
			{
			    if (errno == 0)
			    {	/* POSIX.2 : undefined value */
				fprintf(stdout,"%s\n",catgets(nlmsg_fd,
					NL_SETN, 2, "undefined"));
			        exit(0); /* variable name valid, not defined */
			    }
			    else
			    {
				errno_save = errno;    /* preserve for perror */
				fputs(catgets(nlmsg_fd, NL_SETN, 3, PGM_NAME),
					stderr);
				fputs(argv[1], stderr);
				fputc(' ', stderr);    /* separate the 2 args */
				errno = errno_save;    /* restore */
				perror(argv[2]);
			        exit(3); /* an error occured */
			    }
			}
			fprintf(stdout, "%d\n", v);
			exit(0);
		    break;

		    case FT_SYSCONF:
			if ((v = sysconf(parm_table[i].pt_code)) == -1)
			{
			    if (errno == 0)
			    {
				fprintf(stdout,"%s\n",catgets(nlmsg_fd,
					NL_SETN, 2, "undefined"));
			        exit(0); /* variable name valid, not defined */
			    }
			    else
			    {
				errno_save = errno;    /* preserve for perror */
				fputs(catgets(nlmsg_fd, NL_SETN, 3, PGM_NAME),
					stderr);
				perror(argv[1]);
				errno = errno_save;    /* restore */
			        exit(3); /* an error occured */
			    }
			}
			fprintf(stdout, "%d\n", v);
			exit(0);
		    break;
		}
	}

	/* _POSIX_SSIZE_MAX is not handled via table lookup; do it here */
	if (strcmp(str, "_POSIX_SSIZE_MAX") == 0)
	{
	    fprintf(stdout, "%d\n", SSIZE_MAX);
	    exit(0);
	}

	/* POSIX.2 - not in the table */
	fputs(catgets(nlmsg_fd, NL_SETN, 3, PGM_NAME), stderr);
	errno = EINVAL;
	perror(argv[1]);
	exit(2);			/* 2 = variable name invalid */
} /* main */

/*
 *   Print usage message.
 */

Usage()
{
	fputs(catgets(nlmsg_fd, NL_SETN, 1, USAGE), stderr);
} /* Usage */
