/* @(#) $Revision: 66.1 $ */      
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#include <locale.h>
#endif NLS
# include "e.h"
#ifdef NLS16
#include <nl_ctype.h>
#endif
#define	MAXLINE	1200	/* maximum input line */

char	in[MAXLINE];	/* input buffer */
int	eqnexit();
int noeqn;
main(argc,argv) int argc; char *argv[];{

	eqnexit(eqn(argc, argv));
}

eqnexit(n) {
#ifdef gcos
	if (n)
		fprintf(stderr, (nl_msg(13, "run terminated due to eqn error\n")));
	exit(0);
#endif
	exit(n);
}

eqn(argc,argv) int argc; char *argv[];{
	int i, type;

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
#ifdef NEQN
		fputs(_errlocale("neqn"), stderr);
#else
		fputs(_errlocale("eqn"), stderr);
#endif NEQN
		putenv("LANG=");
	}
#ifdef NEQN
	nl_catopen("neqn");	/* Open user message catalog file */
#else
	nl_catopen("eqn");	/* Open user message catalog file */
#endif NEQN
#endif NLS || NLS16

	setfile(argc,argv);
	init_tbl();	/* install keywords in tables */
	while ((type=getline(in)) != EOF) {
		eqline = linect;
		if (in[0]=='.' && in[1]=='E' && in[2]=='Q') {
			for (i=11; i<100; used[i++]=0);
			printf("%s",in);
			printf(".nr 99 \\n(.s\n.nr 98 \\n(.f\n");
			markline = 0;
			init();
			yyparse();
			if (eqnreg>0) {
				printf(".nr %d \\w'\\*(%d'\n", eqnreg, eqnreg);
				/* printf(".if \\n(%d>\\n(.l .tm too-long eqn, file %s, between lines %d-%d\n",	*/
				/*	eqnreg, svargv[ifile], eqline, linect);	*/
				printf(".nr MK %d\n", markline);	/* for -ms macros */
				printf(".if %d>\\n(.v .ne %du\n", eqnht, eqnht);
				printf(".rn %d 10\n", eqnreg);
				if(!noeqn)printf("\\*(10\n");
			}
			printf(".ps \\n(99\n.ft \\n(98\n");
			printf(".EN");
			if (lastchar == EOF) {
				putchar('\n');
				break;
			}
			if (putchar(lastchar) != '\n')
				while (putchar(gtc()) != '\n');
		}
		else if (type == lefteq)
			inline();
		else
			printf("%s",in);
	}
	return(0);
}

getline(s) register char *s; {
	register c;
	while((*s++=c=gtc())!='\n' && c!=EOF && c!=lefteq)
		if (s >= in+MAXLINE) {
			error( !FATAL, (nl_msg(14, "input line too long: %.20s\n")), in);
			in[MAXLINE] = '\0';
			break;
		}
	if (c==lefteq)
		s--;
	*s++ = '\0';
	return(c);
}

inline() {
	int ds;

	printf(".nr 99 \\n(.s\n.nr 98 \\n(.f\n");
	ds = oalloc();
	printf(".rm %d \n", ds);
	do{
		if (*in)
			printf(".as %d \"%s\n", ds, in);
		init();
		yyparse();
		if (eqnreg > 0) {
			printf(".as %d \\*(%d\n", ds, eqnreg);
			ofree(eqnreg);
		}
		printf(".ps \\n(99\n.ft \\n(98\n");
	} while (getline(in) == lefteq);
	if (*in)
		printf(".as %d \"%s", ds, in);
	printf(".ps \\n(99\n.ft \\n(98\n");
	printf("\\*(%d\n", ds);
	ofree(ds);
}

putout(p1) int p1; {
	extern int gsize, gfont;
	int before, after;
	if(dbg)printf(".\tanswer <- S%d, h=%d,b=%d\n",p1, eht[p1], ebase[p1]);
	eqnht = eht[p1];
	printf(".ds %d \\x'0'", p1);
	/* suppposed to leave room for a subscript or superscript */
	before = eht[p1] - ebase[p1] - VERT(3);	/* 3 = 1.5 lines */
	if (before > 0)
		printf("\\x'0-%du'", before);
	printf("\\f%c\\s%d\\*(%d%s\\s\\n(99\\f\\n(98",
		gfont, gsize, p1, rfont[p1] == ITAL ? "\\|" : "");
	after = ebase[p1] - VERT(1);
	if (after > 0)
		printf("\\x'%du'", after);
	putchar('\n');
	eqnreg = p1;
}

max(i,j) int i,j; {
	return (i>j ? i : j);
}

oalloc() {
	int i;
	for (i=11; i<100; i++)
		if (used[i]++ == 0) return(i);
	error( FATAL, (nl_msg(15, "no eqn strings left")), i);
	return(0);
}

ofree(n) int n; {
	used[n] = 0;
}

setps(p) int p; {
	printf(".ps %d\n", EFFPS(p));
}

nrwid(n1, p, n2) int n1, p, n2; {
	printf(".nr %d \\w'\\s%d\\*(%d'\n", n1, EFFPS(p), n2);
}

setfile(argc, argv) int argc; char *argv[]; {
	static char *nullstr = "-";

	svargc = --argc;
	svargv = argv;
	while ((svargc > 0) && (svargv[1][0] == '-') && (strcmp(svargv[1],"-")!=0)) {
		switch (svargv[1][1]) {

		case 'd': lefteq=svargv[1][2]; righteq=svargv[1][3]; break;
		case 's': gsize = atoi(&svargv[1][2]); break;
		case 'p': deltaps = atoi(&svargv[1][2]); break;
		case 'f': gfont = svargv[1][2]; break;
		case 'e': noeqn++; break;
		case '+': dbg = 1; break;
		default: error(FATAL,(nl_msg(16,"neqn: illegal option %s")), svargv[1]);
		}
		svargc--;
		svargv++;
	}
	ifile = 1;
	linect = 1;
	if ((svargc <= 0)||(strcmp(svargv[1],"-")==0)) {
		curfile = stdin;
		svargv[1] = nullstr;
	}
	else if ((curfile = fopen(svargv[1], "r")) == NULL)
		error( FATAL,(nl_msg(16, "can't open file %s")), svargv[1]);
}

yyerror() {;}

init() {
	ct = 0;
	ps = gsize;
	ft = gfont;
	setps(ps);
	printf(".ft %c\n", ft);
}

error(fatal, s1, s2) int fatal; char *s1, *s2; {
	if (fatal>0)
		printf((nl_msg(17, "eqn fatal error: ")));
	printf(s1,s2);
	printmsg((nl_msg(18, "\nfile %1$s, between lines %2$d and %d\n")),
		 svargv[ifile], eqline, linect);
	fprintf(stderr, (nl_msg(19, "eqn: ")));
	if (fatal>0)
		fprintf(stderr, (nl_msg(20, "fatal error: ")));
	fprintf(stderr, s1, s2);
	fprintmsg(stderr, (nl_msg(21, "\nfile %1$s, between lines %2$d and %d\n")),
		 svargv[ifile], eqline, linect);
	if (fatal > 0)
		eqnexit(1);
}
