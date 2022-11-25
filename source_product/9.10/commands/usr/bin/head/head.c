static char *HPUX_ID = "@(#) $Revision: 70.2 $";
/*
 * AW:
 * 17 Jan 1992:  Modified for POSIX.2 draft 11.2 conformance
 */
#include <stdio.h>
#include <limits.h>

#if defined(NLS) || defined(NLS16)
#include <locale.h>
#include <setlocale.h>
#endif

#ifndef LINE_MAX
#define LINE_MAX	2048
#endif

#ifndef NLS		/* NLS must be defined */
#   define catgets(i,sn,mn,s) (s)
#else /* NLS */
#   define NL_SETN 1	/* set number */
    nl_catd catd;
#endif /* NLS */

/*
 * head - give the first few lines of a stream or of each of a set of files
 *
 * Bill Joy UCB August 24, 1977
 */

int	linecnt	= 10;			/* default number of lines to print*/
int	cflag, lflag;

extern int 	optind;			/*  for getopt() */
extern char 	*optarg;

main(argc, argv)
int argc;
char **argv;
{
	char *name;
	register char *argp, c;
	static int around = 0;	/* assume first time is zero */
	int Argc, errflg = 0, non_std = 0;

#if defined(NLS) || defined(NLS16)
	if(!setlocale(LC_ALL, "")) {
		putenv("LANG=");
		catd = (nl_catd)(-1);
	} else
		catd = catopen("head", 0);
#endif

	if (argv[1][0] == '-' && isdigit(argv[1][1])) {  /* old syntax */
	    non_std = 1;
	    linecnt = 0;
	    argp = &argv[1][1];
	    while (isdigit(*argp))
		linecnt = (linecnt * 10) + *argp++ - '0';
	    optind++;    /* Will cause getopt to be skipped */
	}

	while((c = getopt(argc,argv,"cln:")) != EOF) {
	    switch(c) {
	    case 'c': 	if(lflag)
			    errflg++;
			else
			    cflag++;
			break;
	    case 'l':	if(cflag)
			    errflg++;
			else
			    lflag++;
			break;
	    case 'n':	linecnt = getnum(optarg);
			break;
	    case '?':	errflg++;
			break;
	    }
	}

	if (errflg) {
		fputs(catgets(catd,NL_SETN,1,
		"usage:  head [-l | -c] [- n count] [file ...]\n\thead [-count] [file ... ]\n"),stderr);	/* catgets 1 */
		exit(1);
	}

	Argc = argc - optind;

	do {		/* for all command line arguments */
		if (optind == argc && around)
			break;
		if(non_std && argv[optind][0] == '-' && 
		   isdigit(argv[optind][1])) 
			linecnt = getnum(&argv[optind++][1]);
		if (argc > optind) {
			close(0);	/* open a file or stream for input*/
			if (freopen(argv[optind], "r", stdin) == NULL) {
				perror(argv[optind]);
				errflg++;
				optind++;
				continue;
			}
			name = argv[optind++];		/* set pointer to name arg*/
		} else
			name = 0;
		if (around)			/* if more than one file */
			putchar('\n');		/* make it pretty? */
		around++;
		if (Argc > 1 && name)
		{
			/* tell what file */
			fputs("==> ", stdout);
			fputs(name, stdout);
			fputs(" <==\n", stdout);
		}
		copyout(linecnt,cflag);
	} while (argc > optind);
	exit(errflg);
}

copyout(cnt,chars)
register int cnt;
int chars;
{
	register int c = 0;
	register int numchars;
	register int size;
	char lbuf[LINE_MAX+1], *lptr;

	if (!chars) {
	    /* POSIX.2/D11.2 : No line limit allowed */
	    while (cnt > 0) {
		if((c = fread(lbuf,1,LINE_MAX,stdin)) == 0) return;
		lptr = lbuf;
		for(size = 0; size < c; size++) {
		   if(lbuf[size] == '\n') {
			cnt--;
			numchars = lbuf[size+1];	/* save it */
			lbuf[size+1] = '\0';		/* null it */
			fputs(lptr, stdout);		/* output line */
			if(!cnt) return;
			lbuf[size+1] = numchars;	/* restore */
			lptr = &lbuf[size+1];		/* next line */
		   }
		}
		lbuf[c] = '\0';
		fputs(lptr, stdout);		/* rem. chars output */
	    }
	}
	else {		/* count chars */
	    while(c < cnt) {
		size = (cnt - c) > sizeof(lbuf) ? sizeof(lbuf) : cnt - c;
		if ((numchars = fread(lbuf,sizeof(char), size,stdin)) == 0)
		    break;
		fwrite(lbuf,sizeof(char),numchars,stdout);
		c += numchars;  
	   }
	   fputc('\n',stdout);
	}
}

getnum(cp)		/* convert ascii number to integer */ 
	register char *cp;
{
	register int i;

	for (i = 0; *cp >= '0' && *cp <= '9'; cp++)
		i *= 10, i += *cp - '0';
	if (*cp) {
		fprintf(stderr,catgets(catd,NL_SETN,2,"%s: Badly formed number\n"),cp);
		exit(1);
	}
	return (i);
}
