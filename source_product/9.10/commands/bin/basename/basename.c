static char *HPUX_ID = "@(#) $Revision: 70.5 $";

#include	<stdio.h>
#ifdef NLS16
#include	<nl_ctype.h>
#include 	<locale.h>
#endif

main(argc, argv)
int argc;
char **argv;
{
	register char *p1, *p2, *p3, *p4;
	char c;
	extern int optind, opterr;
	int	slashonly;
#define	TRUE	1;
#define	FALSE	0;
	opterr = 0;     /* disable getopt error message */

#ifdef NLS16
	if (!setlocale(LC_ALL,"")) 
		fputs(_errlocale(),stderr);
#endif

	while ((c=getopt(argc,argv,""))!=EOF)
		if (c == '?')
			break;

	if (argc-optind > 2) {
		putchar('\n');
		exit(1);
	}
	p1 = argv[optind];
	p2 = p1;
	/*  Remove any leading '/' */
	while (*p1) {
		if (*p1 == '/') { 
			p2 = p1;
			slashonly = TRUE;
			p1++;
		}
		else {
			if (*p2 == '/') 
				p2 = p1;
			slashonly = FALSE;
			break;
		}
	}
	while (*p1) {
		/* Capture the last string before the '/' if present */
		if (*p1++ == '/') 
			if ((*p1 != '\0')  && (*p1 != '/')) 
				p2 = p1;
	}

	/* Remove any trailing '/' */
	if (!slashonly) {
		p4 = p2;
		while (*p4) { 
			if (*p4 == '/')
				*p4 = '\0';
			p4++;
		}
	}

	if (argc-optind == 2) {
#ifndef NLS16
		for(p3=argv[optind+1]; *p3; p3++) 
			;
		/* First check if suffix is equivalent to string */
		p4 = p1;   /* save p1 */
		while(p1>p2 && p3>argv[2])
			if(*--p3 != *--p1)
				break;

		/* If so, don't strip */
		if (p1 <= p2)
			goto output;
		else
			/* reset p1 */
			p1 = p4;

		while(p1>p2 && p3>argv[2])
			if(*--p3 != *--p1)
				goto output;
		*p1 = '\0';
#else
		p3 = argv[optind+1];
		p1 = p2;
		if (strcmp (p1, p3) == 0)
			goto output;

		while (*p1) {
			if (strcmp(p1, p3) == 0)
				*p1 = '\0';
			CHARADV(p1);
		}
#endif
	}

output:
	puts (p2, stdout);
	exit(0);
}
