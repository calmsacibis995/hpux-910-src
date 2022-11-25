static char *HPUX_ID = "@(#) $Revision: 70.1 $";

#ifdef NLS16
#include <nl_ctype.h>
#endif

#ifdef NLS
#include <locale.h>
#endif NLS

#include <stdio.h>

main(argc, argv)
char **argv;
{
	register char	*cp;
	register int	c, wd;
	register int	i	= 1;
	register int	qflag	= 0;	/* quote flag for POSIX wordexp */
	register char	terminator = '\n';  /* last character to put out */
	int	j;

#if defined NLS || defined NLS16	/* initialize to the right language */
	if (!setlocale(LC_ALL,"")) 
		fputs(_errlocale(),stderr);
#endif NLS || NLS16

	if(--argc == 0) {
		putchar('\n');
		exit(0);
	}

	if(strcmp(argv[i], "-q") == 0) {/* quote flag for POSIX wordexp */
		++qflag;
		++i;			/* skip argument */
		terminator='\0';        /* last character to be put out */
	}
	for(; i <= argc; i++) {
#ifndef NLS16
		for(cp = argv[i]; *cp; cp++) {
#else NLS16
		for(cp = argv[i]; *cp; ADVANCE(cp)) { /*}*/
#endif NLS16
			if(qflag &&
			  (*cp == '\\' || *cp == ' ')){
				putchar('\\');
				putchar(*cp);
				continue;
			}
				

#ifndef NLS16
			if(*cp == '\\')
			switch(*++cp) {
#else NLS16
#ifdef EUC
		        c = _CHARAT(cp);
#else  EUC
		        c = CHARAT(cp);
#endif EUC
			if(c == '\\') {
			++cp;
#ifdef EUC
			switch(_CHARAT(cp)) { /*}*/
#else  EUC
			switch(CHARAT(cp)) { /*}*/
#endif EUC

#endif NLS16
				case 'b':
					putchar('\b');
					continue;

				case 'c':
					exit(0);

				case 'f':
					putchar('\f');
					continue;

				case 'n':
					putchar('\n');
					continue;

				case 'r':
					putchar('\r');
					continue;

				case 't':
					putchar('\t');
					continue;

				case 'v':
					putchar('\v');
					continue;

				case '\\':
					putchar('\\');
					continue;
				case '0':
					j = wd = 0;
					while ((*++cp >= '0' && *cp <= '7') && j++ < 3) {
						wd <<= 3;
						wd |= (*cp - '0');
					}
					putchar(wd);
					--cp;
					continue;

				default:
					cp--;
			}
#ifndef NLS16
			putchar(*cp);
#else NLS16
		        }
			putChar(c);
#endif NLS16
		}
		putchar(i == argc?terminator:' ');
	}
	exit(0);
}

#ifdef NLS16
putChar(c)
{
	if (c > 0377) putchar ((c>>8) & 0377);
	putchar (c & 0377);
}
#endif
