/* @(#) $Revision: 64.1 $ */
static char *HPUX_ID = "@(#) $Revision: 64.1 $";

#ifdef NLS
#define NL_SETN  11
#include	<msgbuf.h>
#include	<locale.h>
#else
#define nl_msg(i, s) (s)
#endif

#ifdef NLS16
# include	<nl_ctype.h>
#endif NLS16

# include	"stdio.h"
# include	"sys/types.h"
# include	"macros.h"

#define MINUS '-'
#define MINUS_S "-s"
#define TRUE  1
#define FALSE 0

int found = FALSE;
int silent = FALSE;

char pattern[]  =  "@(#)";
char opattern[]  =  "~|^`";

main(argc,argv)
int argc;
register char **argv;
{
	register int i;
	register FILE *iop;

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("what"), stderr);
		putenv("LANG=");
	}
	nl_catopen("sccs");
#endif NLS || NLS16

	if (argc < 2)
		dowhat(stdin);
	else
		for (i = 1; i < argc; i++) {
			if(!strcmp(argv[i],MINUS_S)) {
				silent = TRUE;
				continue;
			}
			if ((iop = fopen(argv[i],"r")) == NULL)
			{
				fprintf(stderr,nl_msg(1,"can't open %s"),argv[i]);
				fprintf(stderr," (26)\n");
			}
			else {
				printf("%s:\n",argv[i]);
				dowhat(iop);
			}
		}
	exit(!found);				/* shell return code */

}


dowhat(iop)
register FILE *iop;
{
	register int c;

	while ((c = getc(iop)) != EOF) {
		if (c == pattern[0]) {
			if(trypat(iop, &pattern[1]) && silent) break;
		} else if (c == opattern[0])
			if(trypat(iop, &opattern[1]) && silent) break;
	}
	fclose(iop);
}


trypat(iop,pat)
register FILE *iop;
register char *pat;
{
	register int c;

	for (; *pat; pat++)
		if ((c = getc(iop)) != *pat)
			break;
	if (!*pat) {
		found = TRUE;
		putchar('\t');
#ifdef NLS16
		/*
		    Don't terminate the output line if a termination character
		    is found as the 2nd byte of a kanji.

		    Note the logic assumes that there is no overlap between
		    the set of kanji 1st bytes and the set of terminating
		    characters (which is currently true because the 1st byte of
		    a kanji must be in the range from 128 to 255, while the
		    terminating characters are all <128).
		*/
		while ((c = getc(iop)) != EOF)
			if (FIRSTof2(c)) {
				putchar(c);
				if ((c = getc(iop)) != EOF)
					if (SECof2(c))
						putchar(c);
					else
						ungetc(c, iop);
				else
					break;
			}
			else if (c && !any(c,"\"\\>\n"))
				putchar(c);
			else
				break;
#else NLS16
		while ((c = getc(iop)) != EOF && c && !any(c,"\"\\>\n"))
			putchar(c);
#endif NLS16
		putchar('\n');
		if(silent)
			return(TRUE);
	}
	else if (c != EOF)
		ungetc(c, iop);
	return(FALSE);
}
