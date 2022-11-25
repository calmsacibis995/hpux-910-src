static char *HPUX_ID = "@(#) $Revision: 64.1 $";
/*
**	Time a command
*/

#include	<stdio.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/param.h>		/* HZ defined here */

char NOFORK[]    = "time: cannot fork -- try again.\n";
char ABNORMAL[]  = "time: command terminated abnormally.\n";

main(argc, argv)
char **argv;
{
	struct {
		time_t user;
		time_t sys;
		time_t childuser;
		time_t childsys;
	} buffer1, buffer2;        /* buffer1 and buffer2 has the starting
				   /* and ending time of a child repectively*/

	register p;
	extern	errno;
	extern	char	*sys_errlist[];
	int	status;
	long	before, after;
	extern long times();

	before = times(&buffer1);
	if(argc<=1)
		exit(0);
	if ((p = fork()) == -1) {
		fputs(NOFORK, stderr);
		exit(2);
	}

	if (p == 0) {
		execvp(argv[1], &argv[1]);
		fputs(sys_errlist[errno], stderr);
		fputs(": ", stderr);
		fputs(argv[1], stderr);
		fputc('\n', stderr);
		exit(2);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	while (wait(&status) != p)
		continue;

	if ((status & 0377) != '\0')
		fputs(ABNORMAL, stderr);

	after = times(&buffer2);

	/* make sure 'after' was successfully and correctly updated */
	if (after < before) {
		fputs(ABNORMAL, stderr);
		exit(2);
	}

	fputc('\n', stderr);
	printt("real", (after-before));
	printt("user", (buffer2.childuser - buffer1.childuser));
	printt("sys ", (buffer2.childsys  - buffer1.childsys));
	exit(status >> 8);
}

/*
The following use of HZ/10 will work correctly only if HZ is a multiple
of 10.  However the only values for HZ now in use are 100 for the 3B
and 60 for other machines.
*/
#ifdef hp9000s500
char quant[] = { 6, 10, 10, 6, 10, 6, 10, 10, 10 };
#else
char quant[] = { HZ/10, 10, 10, 6, 10, 6, 10, 10, 10 };
#endif
char *pad  = "000      ";
char *sep  = "\0\0.\0:\0:\0\0";
char *nsep = "\0\0.\0 \0 \0\0";

printt(s, a)
char *s;
long a;
{
	register i;
	char	digit[9];
	char	c;
	int	nonzero;

	for(i=0; i<9; i++) {
		digit[i] = a % quant[i];
		a /= quant[i];
	}
	fputs(s, stderr);
	nonzero = 0;
	while(--i>0) {
		c = digit[i]!=0 ? digit[i]+'0':
		    nonzero ? '0':
		    pad[i];
		if (c != '\0')
			putc (c, stderr);
		nonzero |= digit[i];
		c = nonzero?sep[i]:nsep[i];
		if (c != '\0')
			putc (c, stderr);
	}
	fputc('\n', stderr);
}
