static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*	join F1 F2 on stuff */

#include <stdio.h>
#if defined(NLS) || defined(NLS16)
#include <locale.h>
#include <setlocale.h>
#endif


#ifdef NLS16
#include <nl_ctype.h>
#define NL_SETN 1	/* set number */
nl_catd nlmsg_fd;
extern int _nl_space_alt;
#else
#define ADVANCE(p)	(p++)
#define FIRSTof2(c)	(0)
#define CHARADV(p)	(*p++)
#define CHARAT(p)	(*p)
#define PCHAR(c, p)	(*p = c)
#define PCHARADV(c, p)	(*p++ = c)
#define SECof2(c)	(0)
#define catgets(i,sn,mn,s) (s)
#endif

/* error messages for use by error() */
#define Usage		"join: usage: join [-an] [-e s] [-jn m] [-tc] [-o list] [-1 n] [-2 n] [-v n] file1 file2\n"				/* catgets 1 */
#define LARGPARM	"join: parameter passed is too large\n"	/* catgets 2 */
#define COPEN		"join: can't open %s\n"			/* catgets 3 */

#define F1 0
#define F2 1
#define	NFLD	100	/* max field per line */
#define comp() cmp(ppi[F1][j1],ppi[F2][j2])
#define putfield(string) if(string == NULL) \
				error(LARGPARM, 2);\
			else if(*string == NULL) fputs(null, stdout); \
			else fputs(string, stdout)

FILE *f[2];
char buf[2][BUFSIZ];	/*input lines */
char *ppi[2][NFLD];	/* pointers to fields in lines */
char *s1,*s2;
int	j1	= 1;	/* join of this field of file 1 */
int	j2	= 1;	/* join of this field of file 2 */
int	olist[2*NFLD];	/* output these fields */
int	olistf[2*NFLD];	/* from these files */
int	no;	/* number of entries in olist */
int	sep1	= ' ';	/* default field separator */
int	sep2	= '\t';
int	sep3    = ' ';
char*	null	= "";
int	unpub1;
int	unpub2;
int	aflg;
int 	vflg;

main(argc, argv)
char *argv[];
{
	int i;
	int n1, n2;
	long top2, bot2;
	long ftell();
	int c, err = 0;
	char *oarg;
	extern char *optarg;
	extern int optind;

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("join"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd) (-1);
	} else
		nlmsg_fd = catopen("join", 0);

	sep3 = _nl_space_alt;            /* alternative field separator */
#endif NLS || NLS16

	    while((c = getopt(argc, argv, "a:e:j:o:t:v:1:2:")) != EOF)
		{
		switch (c) {
		case 'a':
		    if(optarg[1] == '\0')
			switch(optarg[0]) {
			case '1':
				aflg |= 1;
				break;
			case '2':
				aflg |= 2;
				break;
			default:
				err++;
			}
		    else {
			aflg |= 3;
			--optind;
		    }
		    break;
		case 'e':
			null = optarg;
			break;
		case 't':
#if defined NLS || defined NLS16
			if (FIRSTof2((unsigned char)optarg[0]))
				sep1 = sep2 = sep3 = ((((int)(unsigned char)optarg[0]) << 8) 
							+ ((int)(unsigned char)optarg[1]));
			else
#endif NLS || NLS16
				sep1 = sep2 = sep3 = (unsigned char)optarg[0];
			break;
		case 'o':
			oarg = optarg;
			for (no = 0; no < 2*NFLD; no++) {
				if (oarg[0] == '1' && oarg[1] == '.') {
					olistf[no] = F1;
					olist[no] = atoi(&oarg[2]);
				} else if (oarg[0] == '2' && oarg[1] == '.') {
					olist[no] = atoi(&oarg[2]);
					olistf[no] = F2;
				} else {
					--optind;
					break;
				}
				oarg = argv[optind++];
			}
			break;
		/* the j option gets real ugly to parse with getopt
		 * because of the non-standard syntax.  We try to prevent
		 * confusion resulting from an invocation like:
		 *     join -j 5 1file 2file
		 */
		case 'j':
			if((optarg[0] == '1' || optarg[0] == '2') &&
			   isdigit(argv[optind][0]) && argc - optind >= 3) {
			   /* we're pretty sure it's jn m */
			   if(optarg[0] == '1')
			       j1 = atoi(argv[optind++]);
			   else
			       j2 = atoi(argv[optind++]);
			}
			else {
			    if(isdigit(optarg[0]))
			    /* probably j m */
				j1 = j2 = atoi(optarg);
			    else  /* we haven't a clue */
				err++;
			}
			break;
		case '1':
			j1 = atoi(optarg);
			break;
		case '2':
			j2 = atoi(optarg);
			break;
		case 'v':
			vflg++;
			switch(optarg[0]) {
			case '1':
				aflg |= 1;
				break;
			case '2':
				aflg |= 2;
				break;
			}
			break;
		}
	    }
	for (i = 0; i < no; i++)
		olist[i]--;	/* 0 origin */
	if (argc - optind != 2 || err)
		error(Usage, 1);
	j1--;
	j2--;	/* everyone else believes in 0 origin */
	s1 = ppi[F1][j1];
	s2 = ppi[F2][j2];
	if (argv[optind][0] == '-')
		f[F1] = stdin;
	else if ((f[F1] = fopen(argv[optind], "r")) == NULL)
		error(COPEN, 3, argv[optind]);
	if ((f[F2] = fopen(argv[optind+1], "r")) == NULL)
		error(COPEN, 3, argv[optind+1]);

#define get1() n1=input(F1)
#define get2() n2=input(F2)
	get1();
	bot2 = ftell(f[F2]);
	get2();
	while(n1>0 && n2>0 || aflg!=0 && n1+n2>0) {
		if(n1>0 && n2>0 && comp()>0 || n1==0) {
			if(aflg&2) output(0, n2);
			bot2 = ftell(f[F2]);
			get2();
		} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
			if(aflg&1) output(n1, 0);
			get1();
		} else /*(n1>0 && n2>0 && comp()==0)*/ {
			while(n2>0 && comp()==0) {
				if(!vflg)
				    output(n1, n2);
				top2 = ftell(f[F2]);
				get2();
			}
			fseek(f[F2], bot2, 0);
			get2();
			get1();
			for(;;) {
				if(n1>0 && n2>0 && comp()==0) {
					if(!vflg)
					    output(n1, n2);
					get2();
				} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
					fseek(f[F2], bot2, 0);
					get2();
					get1();
				} else /*(n1>0 && n2>0 && comp()>0 || n1==0)*/{
					fseek(f[F2], top2, 0);
					bot2 = top2;
					get2();
					break;
				}
			}
		}
	}
	return(0);
}

input(n)		/* get input line and split into fields */
{
	register int i, c;
	char *bp;
	char **pp;

	bp = buf[n];
	pp = ppi[n];
	if (fgets(bp, BUFSIZ, f[n]) == NULL)
		return(0);
	i = 0;
	do {
		i++;
		if (i > NFLD) {
			fprintf(stderr,
			catgets(nlmsg_fd, NL_SETN, 4, "line has more than %d fields, exiting\n"), NFLD);		/* catgets 4 */
			exit(1);
			}
#if defined NLS || defined NLS16
		if ((sep1 == ' ') || (sep3 == _nl_space_alt) )  /* strip multiples */
			while ((c = (CHARAT(bp))) == sep1 || c == sep2 || c == sep3)
#else
		if (sep1 == ' ')   /* strip multiples */
			while ((c = *bp) == sep1 || c == sep2)
#endif	NLS || NLS16
				bp++;	/* skip blanks */
		else
			c = *bp;
		*pp++ = bp;	/* record beginning */

#if defined NLS || defined NLS16
		while ((c = (CHARAT(bp))) != sep1 && c != '\n' && c != sep2 && c != '\0' && c != sep3)
#else
		while ((c = *bp) != sep1 && c != '\n' && c != sep2 && c != '\0')
#endif  NLS || NLS16

#if defined NLS || defined NLS16
			ADVANCE(bp);
#else
			bp++;
#endif  NLS || NLS16
		*bp++ = '\0';	/* mark end by overwriting blank */
			/* fails badly if string doesn't have \n at end */
	} while (c != '\n' && c != '\0');

	*pp = 0;
	return(i);
}

output(on1, on2)	/* print items from olist */
int on1, on2;
{
	int i;

	if (no <= 0) {	/* default case */
		if (on1)
			putfield(ppi[F1][j1]);
		else
			putfield(ppi[F2][j2]);
		for (i = 0; i < on1; i++)
			if (i != j1) {
				putchar(sep1);
				putfield(ppi[F1][i]);
			}
		for (i = 0; i < on2; i++)
			if (i != j2) {
				putchar(sep1);
				putfield(ppi[F2][i]);
			}
		putchar('\n');
	} else {
		for (i = 0; i < no; i++) {
			if(olistf[i]==F1 && on1<=olist[i] ||
			   olistf[i]==F2 && on2<=olist[i])
				fputs(null, stdout);
			else
				putfield(ppi[olistf[i]][olist[i]]);
			if (i < no - 1)
				printf("%c", sep1);
			else
				putchar('\n');
		}
	}
}

error(s1, num, s2)
char *s1;
int num;
{
	fprintf(stderr, catgets(nlmsg_fd, NL_SETN, num, s1), s2);
	exit(1);
}

cmp(s1, s2)
char *s1, *s2;
{
	return(strcoll(s1, s2));
}
