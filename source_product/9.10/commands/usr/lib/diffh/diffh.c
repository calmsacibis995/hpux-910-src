static char *HPUX_ID = "@(#) $Revision: 70.2 $";

/*****************************************************************
**  As far as I can figure, this code never returns a value	**
**  greater than 1.  I added some return statements so that	**
**  a return value is always specified.  Previously, Register   **
**  D0 contents were returned.  This should help ensure		**
**  that no bad values are returned.				**
**  I *would* trace through this code to comment decently, but  **
**  it is being obsoleted in POSIX 1003.2, so no big deal.      **
*****************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef NLS
#define NL_SETN 1
#include <locale.h>
#include <nl_types.h>
nl_catd nl_fn;
#else NLS
#define catgets(i,sn,mn, s) (s)
#endif NLS

#define C 3
#define RANGE 30
#define LEN 255
#define INF 16384

char *text[2][RANGE];
long lineno[2] = {1, 1};	/*no. of 1st stored line in each file*/
int ntext[2];		/*number of stored lines in each*/
long n0,n1;		/*scan pointer in each*/
int bflag;
int debug = 0;
FILE *file[2];

	/* return pointer to line n of file f*/
char *getl(f,n)
long n;
{
	register char *t;
	char *malloc();
	register delta, nt;
again:
	delta = n - lineno[f];
	nt = ntext[f];
	if(delta<0)
		progerr("1");
	if(delta<nt)
		return(text[f][delta]);
	if(delta>nt)
		progerr("2");
	if(nt>=RANGE)
		progerr("3");
	if(feof(file[f]))
		return(NULL);
	t = text[f][nt];
	if(t==0) {
		t = text[f][nt] = malloc(LEN+1);
		if(t==NULL)
			if(hardsynch())
				goto again;
			else
				progerr("5");
	}
	t = fgets(t,LEN,file[f]);
	if(t!=NULL)
		ntext[f]++;
	return(t);
}

	/*remove thru line n of file f from storage*/
clrl(f,n)
long n;
{
	register i,j;
	j = n-lineno[f]+1;
	for(i=0;i+j<ntext[f];i++)
		movstr(text[f][i+j],text[f][i]);
	lineno[f] = n+1;
	ntext[f] -= j;
}

movstr(s,t)
register char *s, *t;
{
	while(*t++= *s++)
		continue;
}

main(argc,argv)
char **argv;
{
	extern int getopt();
	extern int optind;

	char *s0,*s1;
	int c;
	int errflag = 0;
	int differ = 0;
	FILE *dopen();

#if defined NLS || defined NLS16	/* initialize to the right language */
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		nl_fn=(nl_catd)-1;
	}
	else
		nl_fn=catopen("diffh",0);
#endif NLS || NLS16

	while ((c = getopt(argc, argv, "efbh")) != EOF)
		switch (c) {
		case 'e': /* Ignore -e, -f and -h, might be from diff */
		case 'f':
		case 'h':
			break;
		case 'b':
			bflag = 1;
			break;
		case '?':
			errflag = 1;
			break;
		}

	/*
	 * Adjust argv[] array to appear to have "diffh" in argv[0],
	 * file1 in argv[1] and file2 in argv[2].
	 */
	argc -= optind;
	argv[--optind] = argv[0];
	argv = &argv[optind];

	if (argc != 2 || errflag)
		error(catgets(nl_fn,NL_SETN,1, "Usage: diffh [-b] file1 file2"), "");
	file[0] = dopen(argv[1],argv[2]);
	file[1] = dopen(argv[2],argv[1]);
	for(;;) {
		s0 = getl(0,++n0);
		s1 = getl(1,++n1);
		if(s0==NULL||s1==NULL)
			break;
		if(cmp(s0,s1)!=0) {
			differ = 1;
			if(!easysynch()&&!hardsynch())
				progerr("5");
		} else {
			clrl(0,n0);
			clrl(1,n1);
		}
	}
	if(s0==NULL&&s1==NULL)
		return(differ);
	if(s0==NULL)
		output(-1,INF);
	if(s1==NULL)
		output(INF,-1);
	return(1);
}

	/* synch on C successive matches*/
easysynch()
{
	int i,j;
	register k,m;
	char *s0,*s1;
	for(i=j=1;i<RANGE&&j<RANGE;i++,j++) {
		s0 = getl(0,n0+i);
		if(s0==NULL)
			return(output(INF,INF));
		for(k=C-1;k<j;k++) {
			for(m=0;m<C;m++)
				if(cmp(getl(0,n0+i-m),
					getl(1,n1+k-m))!=0)
					goto cont1;
			return(output(i-C,k-C));
cont1:			;
		}
		s1 = getl(1,n1+j);
		if(s1==NULL)
			return(output(INF,INF));
		for(k=C-1;k<=i;k++) {
			for(m=0;m<C;m++)
				if(cmp(getl(0,n0+k-m),
					getl(1,n1+j-m))!=0)
					goto cont2;
			return(output(k-C,j-C));
cont2:			;
		}
	}
	return(0);
}

output(a,b)
{
	register i;
	char *s;
	if(a<0)
		change(n0-1,0,n1,b,"a");
	else if(b<0)
		change(n0,a,n1-1,0,"d");
	else
		change(n0,a,n1,b,"c");
	for(i=0;i<=a;i++) {
		s = getl(0,n0+i);
		if(s==NULL)
			break;
		fputs("< ", stdout);
		fputs(s, stdout);
		clrl(0,n0+i);
	}
	n0 += i-1;
	if(a>=0&&b>=0)
		fputs("---\n", stdout);
	for(i=0;i<=b;i++) {
		s = getl(1,n1+i);
		if(s==NULL)
			break;
		fputs("> ", stdout);
		fputs(s, stdout);
		clrl(1,n1+i);
	}
	n1 += i-1;
	return(1);
}

change(a,b,c,d,s)
long a,c;
char *s;
{
	range(a,b);
	fputs(s, stdout);
	range(c,d);
	fputc('\n', stdout);
}

range(a,b)
long a;
{
	extern char *ltoa();

	if(b==INF) {
		fputs(ltoa(a), stdout);
		fputs(",$", stdout);
	}
	else
		if(b==0)
			fputs(ltoa(a), stdout);
		else {
			fputs(ltoa(a), stdout);
			fputc(',', stdout);
			fputs(ltoa(a+b), stdout);
		}
}

cmp(s,t)
unsigned char *s,*t;
{
	if(debug) {
		fputs(s, stdout);
		fputc(':', stdout);
		fputs(t, stdout);
		fputc('\n', stdout);
	}

	for(;;){
		if(bflag&&isspace(*s)&&isspace(*t)) {
			while(isspace(*++s))
			    continue;
			while(isspace(*++t))
			    continue;
		}
		if(*s!=*t||*s==0)
		{
			break;
		}
		s++;
		t++;
	}
	return(*s-*t);
}

FILE *dopen(f1,f2)
char *f1,*f2;
{
	FILE *f;
	char b[100],*bptr,*eptr;
	struct stat statbuf;
	if(cmp(f1,"-")==0)
		if(cmp(f2,"-")==0)
			error((catgets(nl_fn,NL_SETN,2,"can't do - -")),"");
		else
			return(stdin);
	if(stat(f1,&statbuf)==-1)
		error((catgets(nl_fn,NL_SETN,3,"can't access ")),f1);
	if((statbuf.st_mode&S_IFMT)==S_IFDIR) {
		for(bptr=b;*bptr= *f1++;bptr++)
		    continue;
		*bptr++ = '/';
		for(eptr=f2;*eptr;eptr++)
			if(*eptr=='/'&&eptr[1]!=0&&eptr[1]!='/')
				f2 = eptr+1;
		while(*bptr++= *f2++) ;
		f1 = b;
	}
	f = fopen(f1,"r");
	if(f==NULL)
		error((catgets(nl_fn,NL_SETN,4,"can't open")),f1);
	return(f);
}


progerr(s)
char *s;
{
	error((catgets(nl_fn,NL_SETN,5,"program error ")),s);
}

error(s,t)
char *s,*t;
{
#ifndef NLS
	fputs("diffh: ", stderr);
	fputs(s, stderr);
	fputs(t, stderr);
	fputc('\n', stderr);
#else
	fprintmsg(stderr,"%1$s %2$s\n",s,t);
#endif
	exit(1);
}

	/*stub for resychronization beyond limits of text buf*/
hardsynch()
{
	change(n0,INF,n1,INF,"c");
	fputs(catgets(nl_fn,NL_SETN,6,"---change record omitted\n"), stdout);
	error((catgets(nl_fn,NL_SETN,7,"can't resynchronize")),"");
	return(0);
}
