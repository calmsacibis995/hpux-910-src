static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 * tee-- pipe fitting
 */

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#ifdef NLS
#include	<msgbuf.h>
#define		NL_SETN 1
#else
#define nl_msg(i, s) (s)
#endif
int openf[20] = { 1 };
int n = 1;
int t = 0;
int errflg;
int aflag;

char in[BUFSIZ];

char out[BUFSIZ];

extern	int	errno;
long	lseek();

main(argc,argv)
char **argv;
{
	int register w;
	extern int optind;		/* for getopt */
	int c;
	struct stat buf;
#ifdef NLS
	nl_catopen("tee");
#endif NLS
	while ((c = getopt(argc, argv, "ai")) != EOF)
		switch(c) {
			case 'a':
				aflag++;
				break;
			case 'i':
				signal(SIGINT, SIG_IGN);
				break;
			case '?':
				errflg++;
		}
	if (errflg) {
		fprintf(stderr, (nl_msg(1,"usage: tee [ -i ] [ -a ] [file ] ...\n")));
		exit(2);
	}
	argc -= optind;
	argv = &argv[optind];
	fstat(1,&buf);
	t = (buf.st_mode&S_IFMT)==S_IFCHR;
	if(lseek(1,0L,1)==-1&&errno==ESPIPE)
		t++;
	while(argc-->0) {
		openf[n] = open(argv[0],O_WRONLY|O_CREAT|
			(aflag?O_APPEND:O_TRUNC), 0666);
		if(openf[n] == -1) {
			fprintf(stderr, (nl_msg(2,"tee: cannot open %s\n")), argv[0]);
			errflg++;
			argv++;
			continue;
		}
		if(stat(argv[0],&buf)>=0) {
			if((buf.st_mode&S_IFMT)==S_IFCHR)
				t++;
		} else {
			fprintf(stderr, (nl_msg(2,"tee: cannot open %s\n")), argv[0]);
			errflg++;
			n--;
		}
		n++;
		argv++;
	}
	w = 0;
	for(;;) {
		w = read(0, in, BUFSIZ);
		if (w > 0)
			stash(w);
		else
			break;
	}
	if (errflg)
		exit(1);
	else
		exit(0);
}

stash(p)
{
	int k;
	int i;
	int d;
	d = t ? 16 : p;
	for(i=0; i<p; i+=d)
		for(k=0;k<n;k++)
			write(openf[k], in+i, d<p-i?d:p-i);
}
