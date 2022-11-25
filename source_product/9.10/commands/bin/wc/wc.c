static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
**	wc -- word and line count
*/

#include	<stdio.h>

#ifdef NLS
#   define NL_SETN 1			/* set number */
#   include <msgbuf.h>
#   include <locale.h>
#   include <nl_ctype.h>
#else
#   define nl_msg(i, s) (s)
#endif

unsigned char	b[BUFSIZ];

FILE *fptr = stdin;
long	wordct;
long	twordct;
long	linect;
long	tlinect;
long	charct;
long	tcharct;

#if defined(NLS) || defined(NLS16)
extern int _nl_space_alt;
#endif
int       c;                            /* for getopt */
extern    char    *optarg;              /* for getopt */
extern    int     optind;		/* for getopt */

#define	OPT_LIMIT	20		/* for option_list */

main(argc,argv)
int argc;
char *argv[];
{
	static char	option_default[] = "lwc"; /* default options */

	/* default options should be change to "lwm" (POSIX.2/D11.3) */
	/* and multibyte support should be added. For 9.0 release    */
	/* multibye support is not required, so "m" option has been  */
	/* added and it works just like "c" option.		     */

	register unsigned char *p1, *p2;
	register int c;
	int	i, token;
	int	opt_ctr = 0;		/* for option_list and OPT_LIMIT */
	int	status = 0;
	char	option_list[OPT_LIMIT+1];/* arbritary number, might be BUFSIZ */
	char	*wd = option_list;

#if defined(NLS) || defined(NLS16)
	/*
	 * initialize to the correct locale
	 */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("wc"), stderr);
		putenv("LANG=");
	}
	nl_catopen("wc");
#endif /* NLS || NLS16 */

	while ((c = getopt(argc, argv, "clmw")) != EOF)
	switch(c) {
		case 'l': 		/* fall thru to 'c' */
		case 'm': 		/* fall thru to 'c' */
		case 'w':		/* fall thru to 'c' */
		case 'c': if (opt_ctr >= OPT_LIMIT)
				usage();
			  option_list[opt_ctr++] = c;
			  break;

		default:  usage();
			  break;
	}
	option_list[opt_ctr] = '\0';		/* ensure null terminated */

	if (! option_list[0]) {		/* default option list */
		wd = option_default;
	}

	i = optind;			/* for following do..while */
	do {
		if((argc > i) && (fptr=fopen(argv[i], "r")) == NULL) {
			fprintf(stderr, (nl_msg(1,"wc: cannot open %s\n")), argv[i]);
			status = 2;
			continue;
		}
		p1 = p2 = b;
		linect = 0;
		wordct = 0;
		charct = 0;
		token = 0;
		for(;;) {
			if(p1 >= p2) {
				p1 = b;
				if( feof(fptr) )
					break;
				c = fread(p1, 1, BUFSIZ, fptr);
				if(c <= 0)
					break;
				charct += c;
				p2 = p1+c;
			}
			c = *p1++;
#ifdef NLS
			if( (isgraph(c)) && !(isspace(c)) )
			/* Make sure there are no blanks, */
			/* for 7bit and 8bit lang. (i.e.  */
			/* LANG=Arabic, isgraph will 	  */
			/* return true for 'xa0')   	  */
#else
			if(' '<c&&c<0177)
#endif
			{
				if(!token) {
					wordct++;
					token++;
				}
				continue;
			}
			if(c=='\n')
				linect++;
#if defined(NLS) || defined(NLS16)
			else if(c!=' '&&c!='\t' && (unsigned int)c != _nl_space_alt)
#else
			else if(c!=' '&&c!='\t')
#endif /* NLS || NLS16 */
				continue;
			token = 0;
		}

		/* print lines, words, chars */
		wcp(wd, charct, wordct, linect);
		if(argc > optind)
			printf(" %s\n", argv[i]);
		else
			fputc('\n', stdout);
		fclose(fptr);
		tlinect += linect;
		twordct += wordct;
		tcharct += charct;
	} while(++i<argc);
	/* print totals if more than one file counted */
	if((argc - 1) > optind) {
		wcp(wd, tcharct, twordct, tlinect);
		fputs(nl_msg(2," total\n"), stdout);
	}
	exit(status);
}

wcp(wd, charct, wordct, linect)
	char *wd;
	long charct; long wordct; long linect;
{
	/* POSIX.2/D11.3 Standard Output Form: "%d %d %d %s\n"	*/
	static char	format1[] = "%ld";	/* use for first field */
	static char	format2[] = " %ld";	/* use for second and on */
	char	*format = format1;		/* use for first field */
	register char *wdp=wd;

	while(*wdp) {
	    switch(*wdp++) {
		case 'l':
			printf(format, linect);
			break;

		case 'w':
			printf(format, wordct);
			break;

		case 'm':			/* POSIX.2/D11.3 added this */
			printf(format, charct); /* option, to count number  */
			break;			/* of characters for multi- */
						/* bytes character set.     */
		/* wc does not support multibytes character set yet! */

		case 'c':
			printf(format, charct);
			break;

	    }
	    format = format2;	/* use for second and on... fields */
	}
}

usage()					/* print usage message */
{
	fputs(nl_msg(3,"usage: wc [-lwc] [name ...]\n"), stderr);
	exit(2);
}
