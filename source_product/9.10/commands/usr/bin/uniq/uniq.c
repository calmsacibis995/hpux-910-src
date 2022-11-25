static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 * Deal with duplicated lines in a file
 */

#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#include <locale.h>
#endif NLS

typedef unsigned char uchar;	/* avoid signed characters */

int	fields;
int	letters;
int	linec;
uchar	mode;
int	uniq;
uchar	*skip();
#ifdef NLS
extern int      _nl_space_alt;	/* alternate space from setlocale(3c) */
#endif NLS

extern  char    *optarg;                /* for getopt */
extern  int     optind, opterr;         /* for getopt */
	int     c;                      /* option character for getopt */

main(argc, argv)
int argc;
uchar *argv[];
{
	static uchar b1[LINE_MAX], b2[LINE_MAX];
	FILE *temp;

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("uniq"), stderr);
		putenv("LANG=");
	}
	nl_catopen("uniq");
#endif NLS || NLS16
	opterr = 0;			/* for getopt, set for local error processing */
	do {
	    while ((c=getopt(argc,argv, "cduf:s:"))!=EOF) {
/*printf("main:\tc           =%c, optarg=%s\n", c, optarg);/**/
/*printf("main:\targv[optind-1]=%s\n", argv[optind-1]);/**/
		switch (c) {		/* continue to continue the while */
		case 'c': mode = c; continue;
		case 'd': mode = c; continue;
		case 'u': mode = c; continue;
		case 'f': if (isdigit(optarg[0]))
				fields = atoi(optarg);
			continue;
		case 's': if (isdigit(optarg[0]))
				letters = atoi(optarg);
				continue;
		case '?':
			  if (isdigit(argv[optind-1][1])) {
				fields = atoi(&argv[optind-1][1]);
				continue;
			}
		default : printe((nl_msg(3,"Usage:  uniq [-c|-d|-u] [-f fields] [-s chars] [input_file[output_file]]\n")));
			  c = EOF;	/* set EOF to stop while */
		} /* switch (c) */
	    } /* while (c=getopt(argc,argv,)) */

	    /* check for + option */
	    if (optind < argc && argv[optind][0] == '+') {
		if ((letters = atoi(&argv[optind][1])) < 1)
			letters = 1;
		optind++;
		c='0';			/* ensure not EOF */
	    }
	} while (c != EOF && optind <= argc); /* do */

/*printf("main:\targc=%d, optind=%d, argv[optind]=%s\n", argc, optind, argv[optind]);/**/
	if (argc > optind) { /* input from this file */
		if ((temp = fopen(argv[optind], "r")) == NULL)
			printe((nl_msg(1, "cannot open %s\n")), argv[optind]);
		else {  fclose(temp);
			freopen(argv[optind], "r", stdin);
		     }
		++optind;
	}

	if (argc > optind) { /* output into this file */
		if(argc >= optind && freopen(argv[optind], "w", stdout) == NULL)
			printe((nl_msg(2, "cannot create %s\n")), argv[optind]);
		++optind;
	}

	if(gline(b1))
		exit(0);
	for(;;) {
		linec++;
		if(gline(b2)) {
			pline(b1);
			exit(0);
		}
		if(!equal(b1, b2)) {
			pline(b1);
			linec = 0;
			do {
				linec++;
				if(gline(b1)) {
					pline(b2);
					exit(0);
				}
			} while(equal(b1, b2));
			pline(b2);
			linec = 0;
		}
	}
}

gline(buf)
register uchar buf[];
{
	register c;
	register int len = 0;
	static unsigned long lineno = 0;

	while(((c = getchar()) != '\n') && (++len < LINE_MAX)) {
		if(c == EOF)
			return(1);
		*buf++ = c;
	}
	*buf = 0;
	lineno++;
	if (len >= LINE_MAX) {
	    fprintf(stderr,
		nl_msg(4, "input line #%lu exceeds LINE_MAX (%d) bytes\n"),
		lineno, LINE_MAX);
	    exit (1);
	}
	return(0);
}

pline(buf)
register uchar buf[];
{

	switch(mode) {

	case 'u':
		if(uniq) {
			uniq = 0;
			return;
		}
		break;

	case 'd':
		if(uniq) break;
		return;

	case 'c':
		printf("%4d ", linec);
	}
	uniq = 0;
	fputs(buf, stdout);
	putchar('\n');
}

equal(b1, b2)
register uchar b1[], b2[];
{
	register uchar c;

	b1 = skip(b1);
	b2 = skip(b2);
	while((c = *b1++) != 0)
		if(c != *b2++) return(0);
	if(*b2 != 0)
		return(0);
	uniq++;
	return(1);
}

uchar *
skip(s)
register uchar *s;
{
	register nf, nl;

	nf = nl = 0;
	while(nf++ < fields) {
#ifdef NLS
		while(*s == ' ' || *s == '\t' || *s == _nl_space_alt)
#else
		while(*s == ' ' || *s == '\t')
#endif NLS
			s++;
#ifdef NLS
		while(!(*s == ' ' || *s == '\t' || *s == _nl_space_alt || *s == 0))
#else
		while(!(*s == ' ' || *s == '\t' || *s == 0))
#endif NLS
			s++;
	}
	while(nl++ < letters && *s != 0)
			s++;
	return(s);
}

printe(p,s)
uchar *p,*s;
{
	fprintf(stderr, p, s);
	exit(1);
}
