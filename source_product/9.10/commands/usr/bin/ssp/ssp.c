static char *HPUX_ID = "@(#) $Revision: 27.1 $";

#include <stdio.h>
/*
 * ssp - single space output
 *
 * Bill Joy UCB August 25, 1977
 *
 * Compress multiple empty lines to a single empty line.
 * Option - compresses to nothing.
 */

char	poof, hadsome;

int	ibuf[256];


main(argc, argv)
	int argc;
	char *argv[];
{
	register int c;
	FILE *f;

	argc--, argv++;
	do {		/* while still files left */
		while (argc > 0 && argv[0][0] == '-') {
			poof = 1;	/* compress to nothing this file?*/
			argc--, argv++;
		}
	f = stdin;		/* default input stream */
		if (argc > 0) {
			if ((f=fopen(argv[0], "r")) == NULL) {
				fflush(f);	/*if can't open clean and tell*/
				perror(argv[0]);
				exit(1);
			}
			argc--, argv++;
		}
		for (;;) {	/* go all the way thru file */
			c = getc(f);
			if (c == -1)	/* if no more input done */
				break;
			if (c != '\n') {
				hadsome = 1;	/* set yes there was some flag*/
				putchar(c);	/* put non-nl char to stdout */
				continue;
			}
			/*
			 * Eat em up	c is nl below here
			 */
			if (hadsome)		/* even if poof set??? */
				putchar('\n');	/* yes need to separate lines
						  at least */
			c = getc(f);
			if (c == -1)
				break;
			if (c != '\n') {
				putchar(c);
				hadsome = 1;
				continue;
			}
			do			/* nl eating loop */
				c = getc(f);
			while (c == '\n');
			if (!poof && hadsome)	/* here put out one line if */
				putchar('\n');	/* wanted ...poof not set */
			if (c == -1)
				break;
			putchar(c);
			hadsome = 1;
		}			/* end infinite read loop */
	} while (argc > 0);

	exit(0);			/* another successful command */
}
