/* @(#) $Revision: 38.1 $ */     
static char *HPUX_ID = "@(#) $Revision: 38.1 $";
/*
** make directory
** (BELL FILE SYSTEM VERSION)
*/

#include	<stdio.h>

int	Errors = 0;
char	*strcat();
char	*strcpy();
char	*strchr();
char	*strrchr();

#ifdef	DEBUG
int debug=0;
#endif	DEBUG

char *pname;
char *sdftail();
void mkdir();

main(argc, argv)
char *argv[];
{

	pname = argv[0];

	if (argc < 2) {
		fprintf(stderr, "Usage: %s device:directory [...]\n", pname);
		exit(2);
	}
	argv++;

#ifdef	DEBUG
	if (**argv == '-')
		if (*++*argv == 'x') {
			debug++;
			argc--;
			argv++;
		}
#endif	DEBUG

	while(--argc)
		mkdir(*argv++);
	exit(Errors? 2: 0);
}

void
mkdir(dir)
register char *dir;
{
	char papa[128], dot[128], dotdot[128];
	int mask;

	if (!sdfpath(dir)) {
		fprintf(stderr, "%s: use mkdir to make %s\n", pname, dir);
		++Errors;
		return;
	}

	mask = umask(0);
	umask(mask);
	if ((sdfmknod(dir, (040777 & ~mask), 0)) < 0) {
		perror(pname);
		fprintf(stderr,"%s: cannot make directory %s\n", pname, dir);
		++Errors;
		return;
	}
	sdfchown(dir, geteuid(), getegid());	/* should use effective ids? */
	return;
}
