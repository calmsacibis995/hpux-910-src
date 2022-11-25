static char *HPUX_ID = "@(#) $Revision: 70.2 $";
#include <stdio.h>
#include <nl_types.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#define NL_SETN 1

/*
 * expand - expand tabs to equivalent spaces
 */
char	obuf[BUFSIZ];
int	nstops;
int	tabstops[100];

nl_catd	catd;	/* message catalog pointer */

extern int optind;
extern char *optarg;

/**********************/
void
usage()
{
fputs ( (catgets(catd,NL_SETN,1, "usage: expand [ -t tablist ] [ file ... ]\n")), stderr );
exit ( 1 );
}
/**********************/

main(argc, argv)
	int argc;
	char *argv[];
{
	register int c, column;
	register int n;

	int 	optc;
	int 	x;
	char subarg[100];
	
	setlocale ( LC_ALL, "" );
	catd = catopen ( "expand", 0 );

	setbuf(stdout, obuf);

	/* convert -n and -n1,n2,n3 to -tn and -tn1,n2,n3 for getopt */
	/* This maintains the obsolescent option behavior */
	for ( x = 0 ; x < argc ; x++ )
		if ( argv [ x ] [ 0 ] == '-'  && isdigit ( argv [ x ] [ 1 ] ) ) {
			strcpy ( subarg , "-t" );
			strcat ( subarg, &argv [ x ] [ 1 ] );
			argv [ x ] = subarg;
			}
					

	while ( ( optc = getopt ( argc, argv, "t:" ) ) != EOF )
		switch ( optc ) {
			case 't':	getstops ( optarg );
						break;
			default:	usage();
			}

	do {
		if ( optind < argc ) {
			if (freopen(argv[ optind ], "r", stdin) == NULL) {
				perror(argv[optind]);
				exit(1);
				}
			optind++;
			}

		column = 0;
		for (;;) {
			c = getc(stdin);
			if (c == -1)
				break;
			switch (c) {

			case '\t':
				if (nstops == 0) {
					do {
						putchar(' ');
						column++;
					} while (column & 07);
					continue;
				}
				if (nstops == 1) {
					do {
						putchar(' ');
						column++;
					} while (((column - 1) % tabstops[0]) != (tabstops[0] - 1));
					continue;
				}
				for (n = 0; n < nstops; n++)
					if (tabstops[n] > column)
						break;
				if (n == nstops) {
					putchar(' ');
					column++;
					continue;
				}
				while (column < tabstops[n]) {
					putchar(' ');
					column++;
				}
				continue;

			case '\b':
				if (column)
					column--;
				putchar('\b');
				continue;

			default:
				putchar(c);
				column++;
				continue;

			case '\r':
				putchar(c);
				column = 0;
				continue;

			case '\n':
				putchar(c);
				column = 0;
				continue;
			}
		}
	} while ( optind < argc );
exit(0);
}

/*************************************************************************/
getstops(cp)
	register char *cp;
{
	register int i;

	nstops = 0;

	for (;;) {
		i = 0;
		while (*cp >= '0' && *cp <= '9')
			i = i * 10 + *cp++ - '0';
		if (i <= 0 || i > 256) {
bad:
			fputs((catgets(catd,NL_SETN,2, "Bad tab stop spec\n")), stderr);
			exit(1);
		}
		if (nstops > 0 && i <= tabstops[nstops-1])
			goto bad;
		tabstops[nstops++] = i;
		if (*cp == 0)
			break;
		if (*cp != ',' && *cp != ' ' )
			goto bad;
		cp++;
	}
}
