#ifndef lint
    static const char *HPUX_ID = "@(#) $Revision: 72.2 $";
#endif


/*
 * fold - fold long lines for finite output devices
 *
 * Bill Joy UCB June 28, 1977
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef NLS16
#  include <nl_ctype.h>
#  define MAXCHAR 2			/* largest possible char byte size */
#  define        BSTATUS b_status = BYTE_STATUS((unsigned char)c, b_status)
      int                b_status = ONEBYTE;       /*  last was '\n' */
#endif

#ifdef NLS
#include <locale.h>
#include <nl_ctype.h>
#include <nl_types.h>
#define NL_SETN 1
#define NL_SETN 1
nl_catd	catd;
#endif NLS

int	fold =  80;
int	bflag=0,sflag=0;
int	col,cnt;
char	*buf;
char	*colsave;
int	errflg=0;

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	char *p;
	FILE *f;
	int i;

	if (!setlocale(LC_ALL,"")) 
		(void) fputs(_errlocale("fold"),stderr);
else
	 catd = catopen("fold",0);

	fold=80;	/* default */
	f=stdin;
	opterr=0;

	/* Transform -50 to -w50 */
	for (i=1; i<argc; i++) {
		p = argv[i];
		if (strcmp(p, "--")==0)		/* end of options */
			break;
		if (p[0]=='-' && '0'<=p[1] && p[1]<='9') {
			p = (char *)malloc(strlen(p)+2);
			if (p==NULL) {
				(void) fputs( catgets(catd,NL_SETN,1,"fold: can't allocate space\n"), stderr);
				exit(1);
			}
			p[0]='-'; p[1]='w';
			(void) strcpy(&p[2], &argv[i][1]);
			argv[i] = p;
		}
	}

	while ((c=getopt(argc,argv,"bsw:"))!=EOF) {
		switch(c) {
			case 'b': bflag++; break;
			case 's': sflag++; break;
			case 'w': fold=0;
				  while (*optarg>='0' && *optarg<='9')
					fold *= 10, fold += *optarg++ - '0';
				  if (*optarg) {
					(void) fputs(catgets(catd,NL_SETN,2,"Bad number for fold\n"), stderr);
					exit(1);
				  }
				  break;
			case '?': errflg++; break;
		}
	}
	if (errflg) {
		(void) fputs(catgets(catd,NL_SETN,3,"usage: fold [-s] [-b] [-w width] [file...]\n"),stderr);
		exit(1);
	}
	buf = (char *) malloc((size_t)fold+10);
	if (buf==NULL) {
		(void) fputs(catgets(catd,NL_SETN,4,"fold: can't allocate space\n"), stderr);
		exit(1);
	}
	colsave = (short*) malloc((size_t)fold+10);
	if (colsave==NULL) {
		(void) fputs(catgets(catd,NL_SETN,5,"fold: can't allocate space\n"), stderr);
		exit(1);
	}
	do {
		if (optind != argc) {
			if (strcmp(argv[optind],"-")==0)
				f=stdin;
			else if ((f=fopen(argv[optind], "r")) == NULL) {
				perror(argv[optind]);
				errflg=1;
				optind++;
				continue;
			}
			optind++;
		}
		dofold(f);
	} while (optind < argc);
	for (i=0;i<cnt;i++)
		putchar(buf[i]);
	exit(errflg);
	/*NOTREACHED*/
}


dofold(f)
FILE *f;
{
	register ncol;
	register c;
	register i,j,dump=0;


	for (;;) {
		if ((c=getc(f))== -1)
			break;

#ifdef NLS16
		if (!bflag) {
			BSTATUS;
			if ((fold < MAXCHAR) && (b_status == FIRSTOF2)) {
				/* if column size < 2-byte char then ... */
				(void) fputs(catgets(catd,NL_SETN,6,"Fold size given too narrow for data.\n"), stdout);
				exit(1);
			}
		}
#endif

		switch (c) {
			case '\n':
				dump=1;
				ncol=0;
				break;
			case '\t':
				if (bflag)
					ncol++;
				else
					ncol = (col + 8) &~ 7;
				break;
			case '\b':
				if (bflag)
					ncol++;
				else if (ncol)
					ncol--;
				break;
			case '\r':
				dump=1;
				ncol = 0;
				break;
			default:
#ifdef NLS16
				/* if 2-byte char, save 2 spaces, else save */
				/* 1 space, unless byte flag is specified!  */
				if (!bflag && b_status == FIRSTOF2)
					ncol = col + 2;
                        	else
					ncol = col + 1;
#else
				ncol = col + 1;
#endif
		}
		if (ncol > fold && sflag) {
			/* fold to last space before fold */
			if (c==' ') {
				j=cnt-1;
			} else {
				for (j=cnt-1;j>0;j--)
					if (buf[j]==' ') break;
			}
			j=j?j:cnt-1;
			for (i=0;i<=j;i++)
				putchar(buf[i]);
			for (i=j+1;i<cnt;i++)
				buf[i-(j+1)]=buf[i];
			ncol -= colsave[j+1];
			col -= colsave[j+1];
			cnt -= (j+1);
			putchar('\n');
		} else if (ncol>fold || dump) {
			/* dump buffer */
			for (i=0;i<cnt;i++)
				putchar(buf[i]);
			if (dump) {
				ncol = 0;
				putchar(c);
			} else {
				ncol -= col;
				putchar('\n');
			}
			cnt = col = 0;
		}
		if (!dump) {	/* add character to buffer */
			buf[cnt++]=c;
		} else {
			dump = 0;
			continue;
		}
		switch (c) {
			case '\n':
				col = 0;
				break;
			case '\t':
				if (bflag)
					col++;
				else
					col = (col+8)&~7;
				break;
			case '\b':
				if (bflag)
					col++;
				else if (col)
					col--;
				break;
			case '\r':
				col = 0;
				break;
			default:
				col++;
				break;
		}
		colsave[cnt]=col;
	}
}
