/* $Revision: 70.1 $ */
/*
Copyright (c) 1984, 19885, 1986, 1987 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include "awk.h"
#include "y.tab.h"

#if defined NLS || defined NLS16
	extern nl_catd catopen();	/* open message catalog */
	extern char *_errlocale();	/* get bad locale settings */
	nl_catd _nl_fn;			/* catalog descriptor */
#else
	int	_nl_fn;
#endif


int	dbg	= 0;
uchar	*cmdname;	/* gets argv[0] for error messages */
extern	FILE *yyin;	/* lex input file */
uchar	*lexprog;	/* points to program argument if it exists */
extern	int errorflag;	/* non-zero if any syntax errors; set by yyerror */
int	compile_time = 1;	/* for error printing: */
                                /* 2 = cmdline, 1 = compile, 0 = running */
uchar   *pfile[100];     /* program filenames from -f's */
int     npfile = 0;     /* number of filenames */
int     curpfile = 0;   /* current filename */


main(argc, argv, envp)
	int argc;
	uchar *argv[], *envp[];
{
	uchar *fs = NULL;
	extern int fpecatch();
        char c;
	extern char *optarg;
	extern int optind,opterr;       /* opterr=0 for no errors */
	uchar *s;
	uchar *getenv();
	static char buf[MAXPATHLEN];
	uchar *strtok();
	extern char *HPUX_ID;


	strcpy(buf,argv[0]);
	cmdname=s=strtok(buf,"/");
	while (s=strtok(NULL,"/")) cmdname=s;
	if (argc == 1)
		error(VERYFATAL,ERR13, cmdname);

#if defined NLS || defined NLS16
	/* initialize the locale */
	if (!setlocale(LC_ALL,"")) {
		/* bad initialization */
		(void) fputs( _errlocale(), stderr);
		_nl_fn = (nl_catd) -1;
		putenv("LANG=");	/* for perror */
	}
	else {
		/* good initialization: open message catalog,
			... keep on going if it isn't there */
		if (((s=getenv("LANG")) && *s) || ((s=getenv("NLSPATH")) && *s))
			_nl_fn = catopen("awk",0);
		else
			_nl_fn = (nl_catd) -1;
	}
#endif NLS || NLS16

	signal(SIGFPE, fpecatch);
	yyin = NULL;
	syminit();
        while ((c=getopt(argc,argv,"f:F:dD:v:"))!=EOF) {
		switch (c) {
		case 'f':	/* next argument is program filename */
			if (npfile==100)
				error(WARNING,ERR131,optarg);
			else
				pfile[npfile++] = (uchar *)optarg;
			break;
		case 'F':	/* set field separator */
			if (*optarg == 't')
				fs= (uchar *)"\t";
			else
				fs= (uchar *)optarg;
			break;
		case 'd':
                        dbg = 1;
                        printf("awk %s\n", HPUX_ID);
                        break;
		case 'D':
			if ((dbg = atoi(optarg))==0)
                        	dbg = 1;
                        printf("awk %s\n", HPUX_ID);
                        break;
		case 'v':
			if (isclvar(optarg))
				setclvar(optarg);
			break;
		}
	}
	if (npfile == 0) {	/* no -f; next arg is program */
		if (optind==argc)
			error(VERYFATAL,ERR120);
		lexprog=argv[optind++];
		dprintf("program = |%s|\n", argv[optind]);
	}
	compile_time = 1;
#ifdef DO_BEFORE_BEGIN
	while (optind < argc) {	/* do leading "name=val" before BEGIN block */
		if (!isclvar(argv[optind]))
			break;
		setclvar(argv[optind++]);
	}
#endif
	argv[optind-1] = argv[0];
	dprintf("argc=%d, argv[0]=%s\n", argc-optind+1, argv[optind-1]);
	arginit(argc-optind+1, &argv[optind-1]);
	envinit(envp);
	yyparse();
	if (fs)
		*FS = tostring(qstring(fs, '\0'));
        dprintf( ("errorflag=%d\n", errorflag) );
        if (errorflag == 0) {
                compile_time = 0;
                run(winner);
        } else
                bracecheck();
        exit(errorflag);
}

pgetc()		/* get program character */
{
	int c;

	for (;;) {
		if (yyin == NULL) {
			if (curpfile >= npfile)
				return EOF;
			if ((yyin = fopen((char *) pfile[curpfile], "r")) == NULL)
				error(VERYFATAL,ERR27,pfile[curpfile]);
		}
		if ((c = getc(yyin)) != EOF)
			return c;
		yyin = NULL;
		curpfile++;
	}
}
