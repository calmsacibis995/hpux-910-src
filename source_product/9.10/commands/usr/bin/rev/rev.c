static char *HPUX_ID = "@(#) $Revision: 64.2 $";

#include <stdio.h>

#ifdef NLS
#include <locale.h>
#endif NLS

#ifdef NLS16
#include <nl_ctype.h>
#endif NLS16

/* reverse lines of a file */

#define N 256
char line[N];
FILE *input;

main(argc,argv)
char **argv;
{
	register i,c;

#ifdef NLS || NLS16			/* initialize to the correct locale */
	register c2;
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("rev"), stderr);
		putenv("LANG=");
	}
#endif NLS || NLS16

	input = stdin;
	do {			/* while still input files */
		if(argc>1) {
			if((input=fopen(argv[1],"r"))==NULL) {
				fputs("rev: cannot open ", stderr);
				fputs(argv[1], stderr);
				fputc('\n', stderr);
				exit(1);
			}
		}
		for(;;){	/* infinite loop ? */
			for(i=0;i<N;i++) {	/* to max line size */
				line[i] = c = getc(input);
				switch(c) {	/*read to nl of EOF */
				case EOF:
					goto eof;
				default:
#ifdef NLS16
/*
**  store 2 byte characters reversed so that when whole line is reversed,
**  the 2 byte characters are in proper byte order
*/
					if (FIRSTof2(c)) {
						/* if no room for 2nd byte */
						/* then put whole char on  */
						/* the next line           */
						if (i == N-1) {
							ungetc(c, input);
							break;
						}
						if ((c2 = getc(input)) == EOF)
							goto eof;
						line[i] = c2;
						line[++i] = c;
					}
#endif NLS16
					continue;
				case '\n':
					break;
				}
				break;
			}
			while(--i>=0)	/* reverse the actual individual lines*/
				putc(line[i],stdout);
			putc('\n',stdout);
		}
eof:
		fclose(input); /* careful what if stdin used or redirected? */
		argc--;
		argv++;
	} while(argc>1);

	exit(0);		/* another successful command */
}
