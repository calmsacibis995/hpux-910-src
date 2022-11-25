/* $Revision: 70.1 $; */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#include <locale.h>
#include <nl_ctype.h>
nl_catd  nl_fn;
#endif NLS

#include "diff.h"
/*
 * diff - driver and subroutines
 */

/* So the routine done() can be used both as a signal handler and a
 * general-purpose exit routine...
 */
#define NOSIG   0


char	diff[] = DIFF;
char	diffh[] = DIFFH;
char	pr[] = PR;

main(argc, argv)
	int argc;
	char **argv;
{
	int   c;         /* variables used by getopt(3c) */
	extern	char *optarg;
        extern	int  optind;
	int errflg = 0;
	register char *argp;
	char *usage = "\tusage: diff [ -C n ] [ -S name ] [ -bcefhilnrstw ] dir1 dir2\n\t\tdiff [-C n ] [ -bcefhintw ] file1 file2\n\t\tdiff [ -D string ] [ -biw ] file1 file2\n";

#ifdef DBUG
	char **aptr;
	aptr = argv;
	printf("\n\tEntering main() in diff.c\n");
	printf("\tArguments:\n");
	while(*aptr)
	    printf("\t\t%s\n",*aptr++);
#endif

#if defined NLS || defined NLS16        /* initialize to the right language */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale(), stderr);
		nl_fn=(nl_catd)-1;
	}
	else
		nl_fn=catopen("diff", 0);
#endif /* NLS || NLS16 */

	ifdef1 = "FILE1"; ifdef2 = "FILE2";
	status = 2;
	diffargv = argv;
	diffargc = argc;
	opt = D_NORMAL;  /* will be default value if not reassigned */
	while((c = getopt(argc, argv, "bcC:D:efhilMnrsS:twIE1:2:")) != EOF)
	    switch(c) {
#ifdef notdef
		case 'I':
			opt = D_IFDEF;
			wantelses = 0;
			break;
		case 'E':
			opt = D_IFDEF;
			wantelses = 1;
			break;
		case '1':
			opt = D_IFDEF;
			if ( optarg && *optarg != '-')
			    ifdef1 = optarg;
			else
			    ++errflg;	    
			break;
#endif
		case 'D':
			/* -Dfoo = -E -1 -2foo */
			wantelses = 1;
			ifdef1 = "";
			/* fall through */
#ifdef notdef
		case '2':
#endif
			opt = D_IFDEF;
			if (optarg && *optarg != '-')
			    ifdef2 = optarg;
			else
			    ++errflg;
			break;
		case 'e':
			opt = D_EDIT;
			break;
		case 'f':
			opt = D_REVERSE;
			break;
		case 'M':		/* added for merge */
			++Mflag;
			break;
		case 'n':
			opt = D_NREVERSE;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		case 'i':
			iflag = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'c':
			opt = D_CONTEXT;
			context = 3;
			break;
		case 'C':
			opt = D_CONTEXT;
			if (optarg && atoi(optarg) >= 0)
			    context = atoi(optarg);
			else
			    ++errflg;
			break;
		case 'h':
			hflag++;
			break;
		case 'S':
			start = optarg;
			break;
		case 'r':
			rflag++;
			break;
		case 's':
			sflag++;
			break;
		case 'l':
			lflag++;
			break;
		case '?':
			fprintf(stderr, (catgets(nl_fn,NL_SETN,20, usage)));
			done(NOSIG);
	}
	if (argc - optind != 2 || errflg) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,20, usage)));
		done(NOSIG);
	}
	file1 = argv[optind++];
	file2 = argv[optind];
	name1 = file1;
	name2 = file2;
	if (hflag && opt) {
		fprintf(stderr,
		    (catgets(nl_fn,NL_SETN,2, "diff: -h doesn't support -e, -f, -n, -c, or -I\n")));
		done(NOSIG);
	}
	if (!strcmp(file1, "-"))
		stb1.st_mode = S_IFREG;
	else if (stat(file1, &stb1) < 0) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,1, "diff: input file ")));
		perror(file1);
		done(NOSIG);
	}
	if (!strcmp(file2, "-"))
		stb2.st_mode = S_IFREG;
	else if (stat(file2, &stb2) < 0) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,1, "diff: input file ")));
		perror(file2);
		done(NOSIG);
	}
	if ((stb1.st_mode & S_IFMT) == S_IFDIR &&
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		diffdir(diffargv);
	} else
		diffreg();
	done(NOSIG);
}

char *
savestr(cp)
	register char *cp;
{
	register char *dp = malloc(strlen(cp)+1);

	if (dp == 0) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,3, "diff: out of memory\n")));
		done(NOSIG);
	}
	strcpy(dp, cp);
	return (dp);
}

min(a,b)
	int a,b;
{

	return (a < b ? a : b);
}

max(a,b)
	int a,b;
{

	return (a > b ? a : b);
}

done(sig)
int sig;
{
	if (tempfile)
		unlink(tempfile);
	if (sig)
	    kill(getpid(), sig);
	exit(status);
}

void *
talloc(n)
{
	register void *p;

	if ((p = malloc((size_t) n)) != NULL)
		return(p);
	noroom();
}

void *
ralloc(p,n)
char *p;
{
	register void *q;

	if ((q = realloc((void *) p, (size_t) n)) == NULL)
		noroom();
	return(q);
}

noroom()
{
	fprintf(stderr, (catgets(nl_fn,NL_SETN,4, "diff: files too big, try -h\n")));
	done(NOSIG);
}

