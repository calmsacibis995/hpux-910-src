static char *HPUX_ID = "@(#) $Revision: 66.1 $";
/*
**	process common lines of two files
*/

#include	<stdio.h>

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#include <locale.h>
#endif NLS

#define	LB	256

int	one;
int	two;
int	three;

char	*ldr[3];

FILE	*ib1;
FILE	*ib2;
FILE	*openfil();

main(argc,argv)
int	argc;
char	**argv;
{
	extern	char	*optarg;
	extern	int	optind, opterr;
	char	letter;			/* getopt current key letter */

	int	l;
	char	lb1[LB],lb2[LB];

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("comm"), stderr);
		putenv("LANG=");
	}
	nl_catopen("comm");
#endif NLS || NLS16

	ldr[0] = "";
	ldr[1] = "\t";
	ldr[2] = "\t\t";

	l = 1;

	while ((letter = getopt(argc, argv, "123")) != EOF) {
				switch(letter) {

				case '1':
					if(!one) {
						one = 1;
						ldr[1][0] = ldr[2][l--] = '\0';
					}
					break;

				case '2':
					if(!two) {
						two = 1;
						ldr[2][l--] = '\0';
					}
					break;

				case '3':
					if(!three) {
						three = 1;
					}
					break;

				default:
					usage();
				}
	}

	if(argc < optind + 2)
		usage();
	ib1 = openfil(argv[optind++]);
	ib2 = openfil(argv[optind++]);
	if(rd(ib1,lb1) < 0) {
		if(rd(ib2,lb2) < 0)
			exit(0);
		copy(ib2,lb2,2);
	}
	if(rd(ib2,lb2) < 0)
		copy(ib1, lb1, 1);
	while(1) {
		switch(compare(lb1,lb2)) {
			case 0:
				wr(lb1,3);
				if(rd(ib1,lb1) < 0) {
					if(rd(ib2,lb2) < 0)
						exit(0);
					copy(ib2,lb2,2);
				}
				if(rd(ib2,lb2) < 0)
					copy(ib1, lb1, 1);
				continue;

			case 1:
				wr(lb1,1);
				if(rd(ib1,lb1) < 0)
					copy(ib2, lb2, 2);
				continue;

			case 2:
				wr(lb2,2);
				if(rd(ib2,lb2) < 0)
					copy(ib1, lb1, 1);
				continue;
		}
	}
}

rd(file,buf)
FILE *file;
char *buf;
{

	register int i, j;
	i = j = 0;
	while((j = getc(file)) != EOF) {
		*buf = j;
		if(*buf == '\n' || i > LB-2) {
			*buf = '\0';
			return(0);
		}
		i++;
		buf++;
	}
	return(-1);
}

wr(str,n)
char *str;
{
	switch(n) {
		case 1:
			if(one)
				return;
			break;

		case 2:
			if(two)
				return;
			break;

		case 3:
			if(three)
				return;
	}
	printf("%s%s\n",ldr[n-1],str);
}

copy(ibuf,lbuf,n)
FILE *ibuf;
char *lbuf;
{
	do {
		wr(lbuf,n);
	} while(rd(ibuf,lbuf) >= 0);

	exit(0);
}

compare(a,b)
char *a,*b;
{
	register int i;

	i = strcoll(a, b);
	if (i < 0)
		return(1);
	if (i == 0)
		return(0);
	return(2);
}
FILE *openfil(s)
char *s;
{
	FILE *b;
	if(s[0]=='-' && s[1]==0)	/* if it is a single character string */
		b = stdin;
	else if((b=fopen(s,"r")) == NULL) {
		fprintf(stderr, (nl_msg(1, "comm: cannot open %s\n")), s);
		exit(2);
	}
	return(b);
}

usage()
{
	fprintf(stderr, (nl_msg(2, "usage: comm [ -[123] ] file1 file2\n")));
	exit(2);
}
