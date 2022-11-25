static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 *	dirname.c
 *
 *	Rewritten for HP-UX by mlc
 *	Rewritten yet again for NLS by lkc 31 August 1987
 */

#include <stdio.h>
#ifdef NLS16
#include <nl_ctype.h>
#endif

#ifdef NLS
#include <locale.h>
#endif NLS

main(argc, argv)
int argc;
char **argv;
{
	extern int optind, opterr;
	int c;
	register	char	*p1, *p2, *savep, *nextc;

	opterr = 0;	/* disable getopt error message */

#if defined NLS || defined NLS16

	if (!setlocale(LC_ALL,"")) 
		fputs(_errlocale(),stderr);

#endif NLS || NLS16

	/* ignore argument "--" for backward compatibilities */
	while ((c = getopt(argc, argv, "")) != EOF) 
		if (c == '?')
			break;

	if ((argc - optind) < 1) {
		puts("");
		exit(0);
	}

	nextc = savep = p1 = p2 = argv[optind];

	while (*p1) {
#ifdef NLS16
	    ADVANCE(nextc);
#else
	    nextc = (p1 + 1);
#endif	    
	    if(((*p1 == '/') && (*nextc != '/')) && (*nextc != '\0'))
		savep= p1;
	    p1 = nextc;	
	}
	
	if (savep == p2) {
		if (*savep != '/')
			*savep = '.';
		savep++;
	}
	*savep = '\0';

	/* removing trailing slash character(s) in string */
	for (p1 = p2; *p1; ADVANCE(p1))
		if (*p1 != '/')
			savep = p1;
	ADVANCE(savep);
	*savep = '\0';

	puts(p2, stdout);
	if ((argc - optind) > 1) 
		exit(1);
	exit(0);
}

