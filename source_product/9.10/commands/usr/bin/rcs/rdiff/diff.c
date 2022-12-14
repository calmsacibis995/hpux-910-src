/*  $Revision: 66.2 $  */

#include "diff.h"
#include "copyright.h"
/*
 * diff - driver and subroutines
 */

char	diff[] = DIFF;
char	diffh[] = DIFFH;
char	pr[] = PR;
char	diffbuf[BUFSIZ];

main(argc, argv)
	int argc;
	char **argv;
{
	register char *argp;

	ifdef1 = "FILE1"; ifdef2 = "FILE2";
	status = 2;
	diffargv = argv;
	setbuf(stdout, diffbuf);
	argc--, argv++;
	while (argc > 2 && argv[0][0] == '-') {
		argp = &argv[0][1];
		argv++, argc--;
		while (*argp) switch(*argp++) {

#ifdef notdef
		case 'I':
			opt = D_IFDEF;
			wantelses = 0;
			continue;
		case 'E':
			opt = D_IFDEF;
			wantelses = 1;
			continue;
		case '1':
			opt = D_IFDEF;
			ifdef1 = argp;
			*--argp = 0;
			continue;
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
			ifdef2 = argp;
			*--argp = 0;
			continue;
		case 'e':
			opt = D_EDIT;
			continue;
		case 'f':
			opt = D_REVERSE;
			continue;
                case 'n':
                        opt = D_NREVERSE;
                        continue;
		case 'b':
			bflag = 1;
			continue;
		case 'c':
			opt = D_CONTEXT;
			if (isdigit(*argp)) {
				context = atoi(argp);
				while (isdigit(*argp))
					argp++;
				if (*argp) {
					fprintf(stderr,
					    "rdiff: -c: bad count\n");
					done();
				}
				argp = "";
			} else
				context = 3;
			continue;
		case 'h':
			hflag++;
			continue;
		case 'S':
			if (*argp == 0) {
				fprintf(stderr, "rdiff: use -Sstart\n");
				done();
			}
			start = argp;
			*--argp = 0;		/* don't pass it on */
			continue;
		case 'r':
			rflag++;
			continue;
		case 's':
			sflag++;
			continue;
		case 'l':
			lflag++;
			continue;
		default:
			fprintf(stderr, "rdiff: -%s: unknown option\n",
			    --argp);
			done();
		}
	}
	if (argc != 2) {
		fprintf(stderr, "rdiff: two filename arguments required\n");
		done();
	}
	file1 = argv[0];
	file2 = argv[1];
	if (hflag && opt) {
		fprintf(stderr,
                    "rdiff: -h doesn't support -e, -f, -n, -c, or -I\n");
		done();
	}
	if (!strcmp(file1, "-"))
		stb1.st_mode = S_IFREG;
	else if (stat(file1, &stb1) < 0) {
		fprintf(stderr, "rdiff: ");
		perror(file1);
		done();
	}
	if (!strcmp(file2, "-"))
		stb2.st_mode = S_IFREG;
	else if (stat(file2, &stb2) < 0) {
		fprintf(stderr, "rdiff: ");
		perror(file2);
		done();
	}
	if ((stb1.st_mode & S_IFMT) == S_IFDIR ||
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		fprintf(stderr, "rdiff: RCS cannot compare directories\n");
		done();
	} else
		diffreg();
	done();
}

char *
savestr(cp)
	register char *cp;
{
	char *strcpy();

	register char *dp = malloc((unsigned)strlen(cp)+1);

	if (dp == 0) {
		fprintf(stderr, "rdiff: ran out of memory\n");
		done();
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

done()
{
	unlink(tempfile);
	exit(status);
}

char *
talloc(n)
{
	register char *p;
	p = malloc((unsigned)n);
	if(p!=NULL)
		return(p);
	noroom();
return(NULL);
}

char *
ralloc(p,n)	/*compacting reallocation */
char *p;
{
	register char *q;
	char *realloc();
	q = realloc(p, (unsigned)n);
	if(q==NULL)
		noroom();
	return(q);
}

noroom()
{
	fprintf(stderr, "rdiff: files too big, try -h\n");
	done();
}
