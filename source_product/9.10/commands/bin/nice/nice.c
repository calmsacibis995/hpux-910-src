static char *HPUX_ID = "@(#) $Revision: 64.1 $";
/*
**	nice
*/


#include	<stdio.h>
#include	<ctype.h>

main(argc, argv)
int argc;
char *argv[];
{
	int	nicarg = 10;
	extern	errno;
	extern	char *sys_errlist[];

	if(argc > 1 && argv[1][0] == '-') {
		register char	*p = argv[1];

		if(*++p != '-') {
			--p;
		}
		while(*++p)
			if(!isdigit(*p)) {
				fputs("nice: argument must be numeric.\n", stderr);
				exit(2);
			}
		nicarg = atoi(&argv[1][1]);
		argc--;
		argv++;
	}
	if(argc < 2) {
		fputs("nice: usage: nice [-num] command\n", stderr);
		exit(2);
	}
	nice(nicarg);
	execvp(argv[1], &argv[1]);

	fputs(sys_errlist[errno], stderr);
	fputs(": ", stderr);
	fputs(argv[1], stderr);
	fputc('\n', stderr);
	exit(2);
}
